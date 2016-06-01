#ifndef __QRTR_H_
#define __QRTR_H_

#include <stdint.h>

#define AF_QIPCRTR 42
struct sockaddr_qrtr {
	unsigned short sq_family;
	uint32_t sq_node;
	uint32_t sq_port;
};

#define QRTRADDR_ANY ((uint32_t)-1)
#endif
