#ifndef __QRTR_H_
#define __QRTR_H_

#include "types.h"

#define AF_QIPCRTR 41
struct sockaddr_qrtr {
	unsigned short sq_family;
	u32 sq_node;
	u32 sq_port;
};

#define QRTRADDR_ANY ((u32)-1)
#endif
