#ifndef _QRTR_LIB_H_
#define _QRTR_LIB_H_

#include <stdint.h>

int qrtr_open(int rport);
void qrtr_close(int sock);

int qrtr_sendto(int sock, uint32_t node, uint32_t port, const void *data, unsigned int sz);
int qrtr_recvfrom(int sock, void *buf, unsigned int bsz, uint32_t *node, uint32_t *port);
int qrtr_recv(int sock, void *buf, unsigned int bsz);

int qrtr_publish(int sock, uint32_t service, uint32_t instance);
int qrtr_bye(int sock, uint32_t service, uint32_t instance);

int qrtr_poll(int sock, unsigned int ms);
int qrtr_lookup(int sock, uint32_t service, uint32_t instance, uint32_t ifilter,
		void (* cb)(void *,uint32_t,uint32_t,uint32_t,uint32_t), void *udata);

#endif
