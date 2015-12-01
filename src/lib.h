#ifndef _QRTR_LIB_H_
#define _QRTR_LIB_H_

#include "types.h"

int qrtr_open(int rport);
void qrtr_close(int sock);

int qrtr_sendto(int sock, u32 node, u32 port, const void *data, unsigned int sz);
int qrtr_recvfrom(int sock, void *buf, unsigned int bsz, u32 *node, u32 *port);
int qrtr_recv(int sock, void *buf, unsigned int bsz);

int qrtr_publish(int sock, u32 service, u32 instance);
int qrtr_bye(int sock, u32 service, u32 instance);

int qrtr_poll(int sock, unsigned int ms);
int qrtr_lookup(int sock, u32 service, u32 instance, u32 ifilter,
		void (* cb)(void *,u32,u32,u32,u32), void *udata);

#endif
