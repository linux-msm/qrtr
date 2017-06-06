#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>

#include "map.h"
#include "hash.h"
#include "waiter.h"
#include "list.h"
#include "container.h"
#include "qrtr.h"
#include "util.h"
#include "ns.h"

static const char *ctrl_pkt_strings[] = {
	[QRTR_CMD_HELLO]	= "hello",
	[QRTR_CMD_BYE]		= "bye",
	[QRTR_CMD_NEW_SERVER]	= "new-server",
	[QRTR_CMD_DEL_SERVER]	= "del-server",
	[QRTR_CMD_DEL_CLIENT]	= "del-client",
	[QRTR_CMD_RESUME_TX]	= "resume-tx",
	[QRTR_CMD_EXIT]		= "exit",
	[QRTR_CMD_PING]		= "ping",
};

#define dprintf(...)

struct context {
	int ctrl_sock;
	int ns_sock;

	int local_node;

	struct sockaddr_qrtr bcast_sq;
};

struct server_filter {
	unsigned int service;
	unsigned int instance;
	unsigned int ifilter;
};

struct server {
	unsigned int service;
	unsigned int instance;

	unsigned int node;
	unsigned int port;
	struct map_item mi;
	struct list_item qli;
};

struct node {
	unsigned int id;

	struct map_item mi;
	struct map services;
};

static struct map nodes;

static struct node *node_get(unsigned int node_id)
{
	struct map_item *mi;
	struct node *node;
	int rc;

	mi = map_get(&nodes, hash_u32(node_id));
	if (mi)
		return container_of(mi, struct node, mi);

	node = calloc(1, sizeof(*node));
	if (!node)
		return NULL;

	node->id = node_id;

	rc = map_create(&node->services);
	if (rc)
		errx(1, "unable to create map");

	rc = map_put(&nodes, hash_u32(node_id), &node->mi);
	if (rc) {
		map_destroy(&node->services);
		free(node);
		return NULL;
	}

	return node;
}

static int server_match(const struct server *srv, const struct server_filter *f)
{
	unsigned int ifilter = f->ifilter;

	if (f->service != 0 && srv->service != f->service)
		return 0;
	if (!ifilter && f->instance)
		ifilter = ~0;
	return (srv->instance & ifilter) == f->instance;
}

static int server_query(const struct server_filter *f, struct list *list)
{
	struct map_entry *node_me;
	struct map_entry *me;
	struct node *node;
	int count = 0;

	list_init(list);
	map_for_each(&nodes, node_me) {
		node = map_iter_data(node_me, struct node, mi);

		map_for_each(&node->services, me) {
			struct server *srv;

			srv = map_iter_data(me, struct server, mi);
			if (!server_match(srv, f))
				continue;

			list_append(list, &srv->qli);
			++count;
		}
	}

	return count;
}

static int service_announce_new(struct context *ctx,
				struct sockaddr_qrtr *dest,
				struct server *srv)
{
	struct qrtr_ctrl_pkt cmsg;
	int rc;

	dprintf("advertising new server [%d:%x]@[%d:%d]\n",
		srv->service, srv->instance, srv->node, srv->port);

	cmsg.cmd = cpu_to_le32(QRTR_CMD_NEW_SERVER);
	cmsg.server.service = cpu_to_le32(srv->service);
	cmsg.server.instance = cpu_to_le32(srv->instance);
	cmsg.server.node = cpu_to_le32(srv->node);
	cmsg.server.port = cpu_to_le32(srv->port);

	rc = sendto(ctx->ctrl_sock, &cmsg, sizeof(cmsg), 0,
		    (struct sockaddr *)dest, sizeof(*dest));
	if (rc < 0)
		warn("sendto()");

	return rc;
}

static int service_announce_del(struct context *ctx,
				struct sockaddr_qrtr *dest,
				struct server *srv)
{
	struct qrtr_ctrl_pkt cmsg;
	int rc;

	dprintf("advertising removal of server [%d:%x]@[%d:%d]\n",
		srv->service, srv->instance, srv->node, srv->port);

	cmsg.cmd = cpu_to_le32(QRTR_CMD_DEL_SERVER);
	cmsg.server.service = cpu_to_le32(srv->service);
	cmsg.server.instance = cpu_to_le32(srv->instance);
	cmsg.server.node = cpu_to_le32(srv->node);
	cmsg.server.port = cpu_to_le32(srv->port);

	rc = sendto(ctx->ctrl_sock, &cmsg, sizeof(cmsg), 0,
		    (struct sockaddr *)dest, sizeof(*dest));
	if (rc < 0)
		warn("sendto()");

	return rc;
}

static int annouce_servers(struct context *ctx, struct sockaddr_qrtr *sq)
{
	struct map_entry *me;
	struct server *srv;
	struct node *node;
	int rc;

	node = node_get(ctx->local_node);
	if (!node)
		return 0;

	map_for_each(&node->services, me) {
		srv = map_iter_data(me, struct server, mi);

		rc = service_announce_new(ctx, sq, srv);
		if (rc < 0)
			return rc;
	}

	return 0;
}

static struct server *server_add(unsigned int service, unsigned int instance,
	unsigned int node_id, unsigned int port)
{
	struct map_item *mi;
	struct server *srv;
	struct node *node;
	int rc;

	if (!service || !port)
		return NULL;

	srv = calloc(1, sizeof(*srv));
	if (srv == NULL)
		return NULL;

	srv->service = service;
	srv->instance = instance;
	srv->node = node_id;
	srv->port = port;

	node = node_get(node_id);
	if (!node)
		goto err;

	rc = map_reput(&node->services, hash_u32(port), &srv->mi, &mi);
	if (rc)
		goto err;

	dprintf("add server [%d:%x]@[%d:%d]\n", srv->service, srv->instance,
			srv->node, srv->port);

	if (mi) { /* we replaced someone */
		struct server *old = container_of(mi, struct server, mi);
		free(old);
	}

	return srv;

err:
	free(srv);
	return NULL;
}

static struct server *server_del(unsigned int node_id, unsigned int port)
{
	struct map_item *mi;
	struct server *srv;
	struct node *node;

	node = node_get(node_id);
	if (!node)
		return NULL;

	mi = map_get(&node->services, hash_u32(port));
	if (!mi)
		return NULL;

	srv = container_of(mi, struct server, mi);
	map_remove(&node->services, srv->mi.key);

	return srv;
}

static int ctrl_cmd_hello(struct context *ctx, struct sockaddr_qrtr *sq,
			  const void *buf, size_t len)
{
	int rc;

	rc = sendto(ctx->ctrl_sock, buf, len, 0, (void *)sq, sizeof(*sq));
	if (rc > 0)
		rc = annouce_servers(ctx, sq);

	return rc;
}

static int ctrl_cmd_bye(struct context *ctx, struct sockaddr_qrtr *sq)
{
	return 0;
}

static int ctrl_cmd_del_client(struct context *ctx, unsigned node_id,
			       unsigned port)
{
	struct qrtr_ctrl_pkt pkt;
	struct sockaddr_qrtr sq;
	struct map_entry *me;
	struct server *srv;
	struct node *node;
	int rc;

	srv = server_del(node_id, port);
	if (srv) {
		if (srv->node == ctx->local_node)
			service_announce_del(ctx, &ctx->bcast_sq, srv);

		free(srv);
	} else {
		node = node_get(ctx->local_node);
		if (!node)
			return 0;

		pkt.cmd = QRTR_CMD_DEL_CLIENT;
		pkt.client.node = node_id;
		pkt.client.port = port;

		map_for_each(&node->services, me) {
			srv = map_iter_data(me, struct server, mi);

			printf("letting %d:%d know about the dying client\n", srv->node, srv->port);

			sq.sq_family = AF_QIPCRTR;
			sq.sq_node = srv->node;
			sq.sq_port = srv->port;

			rc = sendto(ctx->ctrl_sock, &pkt, sizeof(pkt), 0,
				    (struct sockaddr *)&sq, sizeof(sq));
			if (rc < 0)
				warn("del_client propagation failed");
		}
	}

	return 0;
}

static int ctrl_cmd_new_server(struct context *ctx, struct sockaddr_qrtr *from,
			       unsigned int service, unsigned int instance,
			       unsigned int node_id, unsigned int port)
{
	struct server *srv;
	int rc = 0;

	srv = server_add(service, instance, node_id, port);
	if (!srv)
		return -EINVAL;

	if (srv->node == ctx->local_node)
		rc = service_announce_new(ctx, &ctx->bcast_sq, srv);

	return rc;
}

static int ctrl_cmd_del_server(struct context *ctx, unsigned int service,
			       unsigned int instance, unsigned int node_id,
			       unsigned int port)
{
	struct server *srv;
	int rc = 0;

	srv = server_del(node_id, port);
	if (!srv)
		return -EINVAL;

	if (srv->node == ctx->local_node)
		rc = service_announce_del(ctx, &ctx->bcast_sq, srv);

	free(srv);

	return rc;
}

static void ctrl_port_fn(void *vcontext, struct waiter_ticket *tkt)
{
	struct context *ctx = vcontext;
	struct sockaddr_qrtr sq;
	int sock = ctx->ctrl_sock;
	struct qrtr_ctrl_pkt *msg;
	unsigned int cmd;
	char buf[4096];
	socklen_t sl;
	ssize_t len;
	int rc;

	sl = sizeof(sq);
	len = recvfrom(sock, buf, sizeof(buf), 0, (void *)&sq, &sl);
	if (len <= 0) {
		warn("recvfrom()");
		close(sock);
		ctx->ctrl_sock = -1;
		goto out;
	}
	msg = (void *)buf;


	if (len < 4) {
		warnx("short packet from %d:%d", sq.sq_node, sq.sq_port);
		goto out;
	}

	cmd = le32_to_cpu(msg->cmd);
	if (cmd <= _QRTR_CMD_MAX && ctrl_pkt_strings[cmd])
		dprintf("%s from %d:%d\n", ctrl_pkt_strings[cmd], sq.sq_node, sq.sq_port);
	else
		dprintf("UNK (%08x) from %d:%d\n", cmd, sq.sq_node, sq.sq_port);

	rc = 0;
	switch (cmd) {
	case QRTR_CMD_HELLO:
		rc = ctrl_cmd_hello(ctx, &sq, buf, len);
		break;
	case QRTR_CMD_BYE:
		rc = ctrl_cmd_bye(ctx, &sq);
		break;
	case QRTR_CMD_DEL_CLIENT:
		rc = ctrl_cmd_del_client(ctx, le32_to_cpu(msg->client.node),
					 le32_to_cpu(msg->client.port));
		break;
	case QRTR_CMD_NEW_SERVER:
		rc = ctrl_cmd_new_server(ctx, &sq,
					 le32_to_cpu(msg->server.service),
					 le32_to_cpu(msg->server.instance),
					 le32_to_cpu(msg->server.node),
					 le32_to_cpu(msg->server.port));
		break;
	case QRTR_CMD_DEL_SERVER:
		rc = ctrl_cmd_del_server(ctx, le32_to_cpu(msg->server.service),
					 le32_to_cpu(msg->server.instance),
					 le32_to_cpu(msg->server.node),
					 le32_to_cpu(msg->server.port));
		break;
	case QRTR_CMD_EXIT:
	case QRTR_CMD_PING:
	case QRTR_CMD_RESUME_TX:
		break;
	}

	if (rc < 0)
		warnx("failed while handling packet from %d:%d",
		      sq.sq_node, sq.sq_port);
out:
	waiter_ticket_clear(tkt);
}

static int say_hello(struct context *ctx)
{
	struct qrtr_ctrl_pkt pkt;
	int rc;

	memset(&pkt, 0, sizeof(pkt));
	pkt.cmd = cpu_to_le32(QRTR_CMD_HELLO);

	rc = sendto(ctx->ctrl_sock, &pkt, sizeof(pkt), 0,
		    (struct sockaddr *)&ctx->bcast_sq, sizeof(ctx->bcast_sq));
	if (rc < 0)
		return rc;

	return 0;
}

static void ns_pkt_publish(int sock, struct sockaddr_qrtr *sq_src,
			   unsigned int service, unsigned int instance)
{
	struct qrtr_ctrl_pkt cmsg;
	struct sockaddr_qrtr sq;
	struct server *srv;
	int rc;

	srv = server_add(service, instance, sq_src->sq_node, sq_src->sq_port);
	if (srv == NULL) {
		warn("unable to add server");
		return;
	}

	cmsg.cmd = cpu_to_le32(QRTR_CMD_NEW_SERVER);
	cmsg.server.service = cpu_to_le32(srv->service);
	cmsg.server.instance = cpu_to_le32(srv->instance);
	cmsg.server.node = cpu_to_le32(srv->node);
	cmsg.server.port = cpu_to_le32(srv->port);

	sq.sq_family = AF_QIPCRTR;
	sq.sq_node = QRTRADDR_ANY;
	sq.sq_port = QRTR_CTRL_PORT;

	rc = sendto(sock, &cmsg, sizeof(cmsg), 0, (void *)&sq, sizeof(sq));
	if (rc < 0)
		warn("sendto()");
}

static void ns_pkt_bye(int sock, struct sockaddr_qrtr *sq_src)
{
	struct qrtr_ctrl_pkt cmsg;
	struct sockaddr_qrtr sq;
	struct server *srv;
	int rc;

	srv = server_del(sq_src->sq_node, sq_src->sq_port);
	if (srv == NULL) {
		warnx("bye from to unregistered server");
		return;
	}
	cmsg.cmd = cpu_to_le32(QRTR_CMD_DEL_SERVER);
	cmsg.server.service = cpu_to_le32(srv->service);
	cmsg.server.instance = cpu_to_le32(srv->instance);
	cmsg.server.node = cpu_to_le32(srv->node);
	cmsg.server.port = cpu_to_le32(srv->port);
	free(srv);

	sq.sq_family = AF_QIPCRTR;
	sq.sq_node = QRTRADDR_ANY;
	sq.sq_port = QRTR_CTRL_PORT;

	rc = sendto(sock, &cmsg, sizeof(cmsg), 0, (void *)&sq, sizeof(sq));
	if (rc < 0)
		warn("sendto()");
}

static void ns_pkt_query(int sock, struct sockaddr_qrtr *sq,
			 struct server_filter *filter)
{
	struct list reply_list;
	struct list_item *li;
	struct ns_pkt opkt;
	int seq;
	int rc;

	seq = server_query(filter, &reply_list);

	memset(&opkt, 0, sizeof(opkt));
	opkt.type = NS_PKT_NOTICE;
	list_for_each(&reply_list, li) {
		struct server *srv = container_of(li, struct server, qli);

		opkt.notice.seq = cpu_to_le32(seq);
		opkt.notice.service = cpu_to_le32(srv->service);
		opkt.notice.instance = cpu_to_le32(srv->instance);
		opkt.notice.node = cpu_to_le32(srv->node);
		opkt.notice.port = cpu_to_le32(srv->port);
		rc = sendto(sock, &opkt, sizeof(opkt),
				0, (void *)sq, sizeof(*sq));
		if (rc < 0) {
			warn("sendto()");
			break;
		}
		--seq;
	}
	if (rc < 0)
		return;

	memset(&opkt, 0, sizeof(opkt));
	opkt.type = NS_PKT_NOTICE;
	rc = sendto(sock, &opkt, sizeof(opkt), 0, (void *)sq, sizeof(*sq));
	if (rc < 0)
		warn("sendto()");
}

static void ns_port_fn(void *vcontext, struct waiter_ticket *tkt)
{
	struct context *ctx = vcontext;
	struct server_filter filter;
	struct sockaddr_qrtr sq;
	int sock = ctx->ns_sock;
	struct ns_pkt *msg;
	char buf[4096];
	socklen_t sl;
	ssize_t len;

	sl = sizeof(sq);
	len = recvfrom(sock, buf, sizeof(buf), 0, (void *)&sq, &sl);
	if (len <= 0) {
		warn("recvfrom()");
		close(sock);
		ctx->ns_sock = -1;
		goto out;
	}
	msg = (void *)buf;

	dprintf("new packet; from: %d:%d\n", sq.sq_node, sq.sq_port);

	if (len < 4) {
		warnx("short packet from %d:%d", sq.sq_node, sq.sq_port);
		goto out;
	}

	switch (le32_to_cpu(msg->type)) {
	case NS_PKT_PUBLISH:
		ns_pkt_publish(ctx->ctrl_sock, &sq,
			       le32_to_cpu(msg->publish.service),
			       le32_to_cpu(msg->publish.instance));
		break;
	case NS_PKT_BYE:
		ns_pkt_bye(ctx->ctrl_sock, &sq);
		break;
	case NS_PKT_QUERY:
		filter.service = le32_to_cpu(msg->query.service);
		filter.instance = le32_to_cpu(msg->query.instance);
		filter.ifilter = le32_to_cpu(msg->query.ifilter);

		ns_pkt_query(sock, &sq, &filter);
		break;
	case NS_PKT_NOTICE:
		break;
	}

out:
	waiter_ticket_clear(tkt);
}

static int qrtr_socket(int port)
{
	struct sockaddr_qrtr sq;
	socklen_t sl = sizeof(sq);
	int sock;
	int rc;

	sock = socket(AF_QIPCRTR, SOCK_DGRAM, 0);
	if (sock < 0) {
		warn("sock(AF_QIPCRTR)");
		return -1;
	}

	rc = getsockname(sock, (void*)&sq, &sl);
	if (rc < 0 || sq.sq_node == -1) {
		warn("getsockname()");
		return -1;
	}
	sq.sq_port = port;

	rc = bind(sock, (void *)&sq, sizeof(sq));
	if (rc < 0) {
		warn("bind(sock)");
		close(sock);
		return -1;
	}

	return sock;
}

static void server_mi_free(struct map_item *mi)
{
	free(container_of(mi, struct server, mi));
}

static void node_mi_free(struct map_item *mi)
{
	struct node *node = container_of(mi, struct node, mi);

	map_clear(&node->services, server_mi_free);
	map_destroy(&node->services);

	free(node);
}

int main(int argc, char **argv)
{
	struct waiter_ticket *tkt;
	struct sockaddr_qrtr sq;
	struct context ctx;
	struct waiter *w;
	socklen_t sl = sizeof(sq);
	int rc;

	w = waiter_create();
	if (w == NULL)
		errx(1, "unable to create waiter");

	rc = map_create(&nodes);
	if (rc)
		errx(1, "unable to create node map");

	ctx.ctrl_sock = qrtr_socket(QRTR_CTRL_PORT);
	if (ctx.ctrl_sock < 0)
		errx(1, "unable to create control socket");

	ctx.ns_sock = qrtr_socket(NS_PORT);
	if (ctx.ns_sock < 0)
		errx(1, "unable to create nameserver socket");

	rc = getsockname(ctx.ctrl_sock, (void*)&sq, &sl);
	if (rc < 0)
		err(1, "getsockname()");

	ctx.local_node = sq.sq_node;

	ctx.bcast_sq.sq_family = AF_QIPCRTR;
	ctx.bcast_sq.sq_node = QRTRADDR_ANY;
	ctx.bcast_sq.sq_port = QRTR_CTRL_PORT;

	rc = say_hello(&ctx);
	if (rc)
		err(1, "unable to say hello");

	if (fork() != 0) {
		close(ctx.ctrl_sock);
		close(ctx.ns_sock);
		exit(0);
	}

	tkt = waiter_add_fd(w, ctx.ctrl_sock);
	waiter_ticket_callback(tkt, ctrl_port_fn, &ctx);

	tkt = waiter_add_fd(w, ctx.ns_sock);
	waiter_ticket_callback(tkt, ns_port_fn, &ctx);

	while (ctx.ctrl_sock >= 0 && ctx.ns_sock >= 0)
		waiter_wait(w);

	puts("exiting cleanly");

	waiter_destroy(w);

	map_clear(&nodes, node_mi_free);
	map_destroy(&nodes);

	return 0;
}
