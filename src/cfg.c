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

#include "addr.h"
#include "libqrtr.h"

static void usage(void)
{
	extern char *__progname;

	fprintf(stderr, "%s <node-id>\n", __progname);
	exit(1);
}

int main(int argc, char **argv)
{
	unsigned long addrul;
	uint32_t addr;
	char *ep;

	if (argc != 2)
		usage();

	addrul = strtoul(argv[1], &ep, 10);
	if (argv[1][0] == '\0' || *ep != '\0' || addrul >= UINT_MAX)
		usage();
	addr = addrul;
	qrtr_set_address(addr);

	return 0;
}
