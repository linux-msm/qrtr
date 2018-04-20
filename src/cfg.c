#include <err.h>
#include <errno.h>
#include <limits.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/qrtr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "libqrtr.h"

static void usage(void)
{
	extern char *__progname;

	fprintf(stderr, "%s <node-id>\n", __progname);
	exit(1);
}

int main(int argc, char **argv)
{
	struct {
		struct nlmsghdr nh;
		struct ifaddrmsg ifa;
		char attrbuf[32];
	} req;
	struct {
		struct nlmsghdr nh;
		struct nlmsgerr err;
	} resp;
	struct rtattr *rta;
	unsigned long addrul;
	uint32_t addr;
	char *ep;
	int sock;
	int ret;

	if (argc != 2)
		usage();

	addrul = strtoul(argv[1], &ep, 10);
	if (argv[1][0] == '\0' || *ep != '\0' || addrul >= UINT_MAX)
		usage();
	addr = addrul;

	/* Trigger loading of the qrtr kernel module */
	sock = socket(AF_QIPCRTR, SOCK_DGRAM, 0);
	if (sock < 0)
		err(1, "failed to create AF_QIPCRTR socket");
	close(sock);

	sock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
	if (sock < 0)
		err(1, "failed to create netlink socket");

	memset(&req, 0, sizeof(req));
	req.nh.nlmsg_len = NLMSG_SPACE(sizeof(struct ifaddrmsg));
	req.nh.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nh.nlmsg_type = RTM_NEWADDR;
	req.ifa.ifa_family = AF_QIPCRTR;

	rta = (struct rtattr *)(((char *) &req) + req.nh.nlmsg_len);
	rta->rta_type = IFA_LOCAL;
	rta->rta_len = RTA_LENGTH(sizeof(addr));
	memcpy(RTA_DATA(rta), &addr, sizeof(addr));

	req.nh.nlmsg_len += rta->rta_len;

	ret = send(sock, &req, req.nh.nlmsg_len, 0);
	if (ret < 0)
		err(1, "failed to send netlink request");

	ret = recv(sock, &resp, sizeof(resp), 0);
	if (ret < 0)
		err(1, "failed to receive netlink response");

	if (resp.nh.nlmsg_type == NLMSG_ERROR && resp.err.error != 0) {
		errno = -resp.err.error;
		err(1, "failed to configure node id");
	}

	return 0;
}
