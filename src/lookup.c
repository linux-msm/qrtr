#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

#include "qrtr.h"
#include "util.h"
#include "ns.h"

static const struct {
	unsigned int service;
	unsigned int ifilter;
	const char *name;
} common_names[] = {
	{ 0x000f, 0x0, "test" },
};

static unsigned int read_num_le(const char *str, int *rcp)
{
	unsigned int ret;
	char *e;

	if (*rcp)
		return 0;

	errno = 0;
	ret = strtoul(str, &e, 0);
	*rcp = -(errno || *e);

	return cpu_to_le32(ret);
}

int main(int argc, char **argv)
{
	struct sockaddr_qrtr sq;
	struct ns_pkt pkt;
	struct timeval tv;
	int sock;
	int len;
	int rc;

	rc = 0;
	memset(&pkt, 0, sizeof(pkt));

	switch (argc) {
	default:
		rc = -1;
		break;
	case 4: pkt.query.ifilter = read_num_le(argv[3], &rc);
	case 3: pkt.query.instance = read_num_le(argv[2], &rc);
	case 2: pkt.query.service = read_num_le(argv[1], &rc);
	case 1: break;
	}
	if (rc)
		errx(1, "Usage: %s [<service> [<instance> [<filter>]]]", argv[0]);

	sock = socket(AF_QIPCRTR, SOCK_DGRAM, 0);
	if (sock < 0)
		err(1, "sock(AF_QIPCRTR)");

	sq.sq_family = AF_QIPCRTR;
	sq.sq_node = 0;
	sq.sq_port = NS_PORT;

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	pkt.type = cpu_to_le32(NS_PKT_QUERY);

	rc = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (rc)
		err(1, "setsockopt(SO_RCVTIMEO)");

	rc = sendto(sock, &pkt, sizeof(pkt), 0, (void *)&sq, sizeof(sq));
	if (rc < 0)
		err(1, "sendto()");

	while ((len = recv(sock, &pkt, sizeof(pkt), 0)) > 0) {
		unsigned int type = le32_to_cpu(pkt.type);
		const char *name = NULL;
		unsigned int i;

		if (len < sizeof(pkt) || type != NS_PKT_NOTICE) {
			warn("invalid/short packet");
			continue;
		}

		if (pkt.notice.seq == 0)
			break;

		pkt.notice.service = le32_to_cpu(pkt.notice.service);
		pkt.notice.instance = le32_to_cpu(pkt.notice.instance);
		pkt.notice.node = le32_to_cpu(pkt.notice.node);
		pkt.notice.port = le32_to_cpu(pkt.notice.port);

		for (i = 0; i < sizeof(common_names)/sizeof(common_names[0]); ++i) {
			if (pkt.notice.service != common_names[i].service)
				continue;
			if (pkt.notice.instance &&
			   (pkt.notice.instance & common_names[i].ifilter) != common_names[i].ifilter)
				continue;
			name = common_names[i].name;
		}

		printf("[%d:%x]@[%d:%d] (%s)\n",
				pkt.notice.service,
				pkt.notice.instance,
				pkt.notice.node,
				pkt.notice.port,
				name ? name : "<unknown>");
	}

	if (len < 0)
		err(1, "recv()");

	close(sock);

	return 0;
}
