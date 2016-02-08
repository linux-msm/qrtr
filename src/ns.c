#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <err.h>

#include "map.h"
#include "hash.h"
#include "waiter.h"
#include "list.h"
#include "container.h"
#include "qrtr.h"
#include "util.h"
#include "ns.h"

enum ctrl_pkt_cmd {
	QRTR_CMD_HELLO		= 2,
	QRTR_CMD_BYE		= 3,
	QRTR_CMD_NEW_SERVER	= 4,
	QRTR_CMD_DEL_SERVER	= 5,
	QRTR_CMD_DEL_CLIENT	= 6,
	QRTR_CMD_RESUME_TX	= 7,
	QRTR_CMD_EXIT		= 8,
	QRTR_CMD_PING		= 9,

	_QRTR_CMD_CNT,
	_QRTR_CMD_MAX = _QRTR_CMD_CNT - 1
};

struct ctrl_pkt {
	__le32 cmd;

	union {
		struct {
			__le32 service;
			__le32 instance;
			__le32 node;
			__le32 port;
		} server;

		struct {
			__le32 node;
			__le32 port;
		} client;
	};
} __attribute__((packed));

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

#define QRTR_CTRL_PORT ((unsigned int)-2)

#define dprintf(...)

struct context {
	int ctrl_sock;
	int ns_sock;
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

static struct map servers;

static int server_match(const struct server *srv, const struct server_filter *f)
{
	unsigned int ifilter = f->ifilter;

	if (f->service != 0 && srv->service != f->service)
		return 0;
	if (!ifilter && f->instance)
		ifilter = ~0;
	return (srv->instance & ifilter) == f->instance;
}

static struct server *server_lookup(unsigned int node, unsigned int port)
{
	struct map_item *mi;
	unsigned int key;

	key = hash_u64(((uint64_t)node << 16) ^ port);
	mi = map_get(&servers, key);
	if (mi == NULL)
		return NULL;

	return container_of(mi, struct server, mi);
}

static int server_query(const struct server_filter *f, struct list *list)
{
	struct map_entry *me;
	int count = 0;

	list_init(list);
	map_for_each(&servers, me) {
		struct server *srv;

		srv = map_iter_data(me, struct server, mi);
		if (!server_match(srv, f))
			continue;

		list_append(list, &srv->qli);
		++count;
	}

	return count;
}

static struct server *server_add(unsigned int service, unsigned int instance,
	unsigned int node, unsigned int port)
{
	struct map_item *mi;
	struct server *srv;
	unsigned int key;
	int rc;

	srv = calloc(1, sizeof(*srv));
	if (srv == NULL)
		return NULL;

	srv->service = service;
	srv->instance = instance;
	srv->node = node;
	srv->port = port;

	key = hash_u64(((uint64_t)srv->node << 16) ^ srv->port);
	rc = map_reput(&servers, key, &srv->mi, &mi);
	if (rc) {
		free(srv);
		return NULL;
	}

	dprintf("add server [%d:%x]@[%d:%d]\n", srv->service, srv->instance,
			srv->node, srv->port);

	if (mi) { /* we replaced someone */
		struct server *old = container_of(mi, struct server, mi);
		free(old);
	}

	return srv;
}

static struct server *server_del(unsigned int node, unsigned int port)
{
	struct server *srv;

	srv = server_lookup(node, port);
	if (srv != NULL)
		map_remove(&servers, srv->mi.key);

	return srv;
}

static void ctrl_port_fn(void *vcontext, struct waiter_ticket *tkt)
{
	struct context *ctx = vcontext;
	struct sockaddr_qrtr sq;
	int sock = ctx->ctrl_sock;
	struct ctrl_pkt *msg;
	struct server *srv;
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

	dprintf("new packet; from: %d:%d\n", sq.sq_node, sq.sq_port);

	if (len < 4) {
		warn("short packet");
		goto out;
	}

	cmd = le32_to_cpu(msg->cmd);
	if (cmd <= _QRTR_CMD_MAX && ctrl_pkt_strings[cmd])
		dprintf("packet type: %s\n", ctrl_pkt_strings[cmd]);
	else
		dprintf("packet type: UNK (%08x)\n", cmd);

	rc = 0;
	switch (cmd) {
	case QRTR_CMD_HELLO:
		rc = sendto(sock, buf, len, 0, (void *)&sq, sizeof(sq));
		break;
	case QRTR_CMD_EXIT:
	case QRTR_CMD_PING:
	case QRTR_CMD_BYE:
	case QRTR_CMD_RESUME_TX:
	case QRTR_CMD_DEL_CLIENT:
		break;
	case QRTR_CMD_NEW_SERVER:
		server_add(le32_to_cpu(msg->server.service),
				le32_to_cpu(msg->server.instance),
				le32_to_cpu(msg->server.node),
				le32_to_cpu(msg->server.port));
		break;
	case QRTR_CMD_DEL_SERVER:
		srv = server_del(le32_to_cpu(msg->server.node),
				le32_to_cpu(msg->server.port));
		if (srv)
			free(srv);
		break;
	}

	if (rc < 0)
		warn("failed while handling packet");
out:
	waiter_ticket_clear(tkt);
}

static int say_hello(int sock)
{
	struct sockaddr_qrtr sq;
	struct ctrl_pkt pkt;
	int rc;

	sq.sq_family = AF_QIPCRTR;
	sq.sq_node = QRTRADDR_ANY;
	sq.sq_port = QRTR_CTRL_PORT;

	memset(&pkt, 0, sizeof(pkt));
	pkt.cmd = cpu_to_le32(QRTR_CMD_HELLO);

	rc = sendto(sock, &pkt, sizeof(pkt), 0, (void *)&sq, sizeof(sq));
	if (rc < 0)
		return rc;

	return 0;
}

static int announce_reset(int sock)
{
	struct sockaddr_qrtr sq;
	struct ns_pkt pkt;
	int rc;

	sq.sq_family = AF_QIPCRTR;
	sq.sq_node = QRTRADDR_ANY;
	sq.sq_port = NS_PORT;

	memset(&pkt, 0, sizeof(pkt));
	pkt.type = cpu_to_le32(NS_PKT_RESET);

	rc = sendto(sock, &pkt, sizeof(pkt), 0, (void *)&sq, sizeof(sq));
	if (rc < 0)
		return rc;

	return 0;

}

static void ns_port_fn(void *vcontext, struct waiter_ticket *tkt)
{
	struct context *ctx = vcontext;
	struct sockaddr_qrtr sq;
	int sock = ctx->ns_sock;
	struct ctrl_pkt cmsg;
	struct server *srv;
	struct ns_pkt *msg;
	char buf[4096];
	socklen_t sl;
	ssize_t len;
	int rc;

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
		warn("short packet");
		goto out;
	}

	rc = 0;
	switch (le32_to_cpu(msg->type)) {
	case NS_PKT_PUBLISH:
		srv = server_add(le32_to_cpu(msg->publish.service),
				le32_to_cpu(msg->publish.instance),
				sq.sq_node, sq.sq_port);
		if (srv == NULL) {
			warn("unable to add server");
			break;
		}
		cmsg.cmd = cpu_to_le32(QRTR_CMD_NEW_SERVER);
		cmsg.server.service = cpu_to_le32(srv->service);
		cmsg.server.instance = cpu_to_le32(srv->instance);
		cmsg.server.node = cpu_to_le32(srv->node);
		cmsg.server.port = cpu_to_le32(srv->port);
		sq.sq_node = QRTRADDR_ANY;
		sq.sq_port = QRTR_CTRL_PORT;
		rc = sendto(ctx->ctrl_sock, &cmsg, sizeof(cmsg), 0, (void *)&sq, sizeof(sq));
		if (rc < 0)
			warn("sendto()");
		break;
	case NS_PKT_BYE:
		srv = server_del(sq.sq_node, sq.sq_port);
		if (srv == NULL) {
			warn("bye from to unregistered server");
			break;
		}
		cmsg.cmd = cpu_to_le32(QRTR_CMD_DEL_SERVER);
		cmsg.server.service = cpu_to_le32(srv->service);
		cmsg.server.instance = cpu_to_le32(srv->instance);
		cmsg.server.node = cpu_to_le32(srv->node);
		cmsg.server.port = cpu_to_le32(srv->port);
		free(srv);
		sq.sq_node = QRTRADDR_ANY;
		sq.sq_port = QRTR_CTRL_PORT;
		rc = sendto(ctx->ctrl_sock, &cmsg, sizeof(cmsg), 0, (void *)&sq, sizeof(sq));
		if (rc < 0)
			warn("sendto()");
		break;
	case NS_PKT_QUERY: {
		struct server_filter filter = {
			le32_to_cpu(msg->query.service),
			le32_to_cpu(msg->query.instance),
			le32_to_cpu(msg->query.ifilter),
		};
		struct list reply_list;
		struct list_item *li;
		struct ns_pkt opkt;
		int seq;

		seq = server_query(&filter, &reply_list);

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
					0, (void *)&sq, sizeof(sq));
			if (rc < 0) {
				warn("sendto()");
				break;
			}
			--seq;
		}
		if (rc < 0)
			break;

		memset(&opkt, 0, sizeof(opkt));
		opkt.type = NS_PKT_NOTICE;
		rc = sendto(sock, &opkt, sizeof(opkt), 0, (void *)&sq, sizeof(sq));
		if (rc < 0)
			warn("sendto()");
		break;
	}
	case NS_PKT_NOTICE:
		break;
	}

out:
	waiter_ticket_clear(tkt);
}

static int qrtr_socket(int port)
{
	struct sockaddr_qrtr sq;
	int sock;
	int rc;

	sock = socket(AF_QIPCRTR, SOCK_DGRAM, 0);
	if (sock < 0) {
		warn("sock(AF_QIPCRTR)");
		return -1;
	}

	sq.sq_family = AF_QIPCRTR;
	sq.sq_node = 1;
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

int main(int argc, char **argv)
{
	struct waiter_ticket *tkt;
	struct context ctx;
	struct waiter *w;
	int rc;

	w = waiter_create();
	if (w == NULL)
		errx(1, "unable to create waiter");

	rc = map_create(&servers);
	if (rc)
		errx(1, "unable to create map");

	ctx.ctrl_sock = qrtr_socket(QRTR_CTRL_PORT);
	if (ctx.ctrl_sock < 0)
		errx(1, "unable to create control socket");

	ctx.ns_sock = qrtr_socket(NS_PORT);
	if (ctx.ns_sock < 0)
		errx(1, "unable to create nameserver socket");

	rc = say_hello(ctx.ctrl_sock);
	if (rc)
		err(1, "unable to say hello");

	rc = announce_reset(ctx.ns_sock);
	if (rc)
		err(1, "unable to announce reset");

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
	map_clear(&servers, server_mi_free);
	map_destroy(&servers);

	return 0;
}
