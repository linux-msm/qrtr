#ifndef _QRTR_LIB_H_
#define _QRTR_LIB_H_

#include <stdint.h>

struct sockaddr_qrtr;

struct qrtr_ind_ops {
	int (*bye)(uint32_t node, void *data);
	int (*del_client)(uint32_t node, uint32_t port, void *data);
};

int qrtr_open(int rport);
void qrtr_close(int sock);

int qrtr_sendto(int sock, uint32_t node, uint32_t port, const void *data, unsigned int sz);
int qrtr_recvfrom(int sock, void *buf, unsigned int bsz, uint32_t *node, uint32_t *port);
int qrtr_recv(int sock, void *buf, unsigned int bsz);

int qrtr_new_server(int sock, uint32_t service, uint16_t version, uint16_t instance);
int qrtr_remove_server(int sock, uint32_t service, uint16_t version, uint16_t instance);

int qrtr_publish(int sock, uint32_t service, uint16_t version, uint16_t instance);
int qrtr_bye(int sock, uint32_t service, uint16_t version, uint16_t instance);

int qrtr_new_lookup(int sock, uint32_t service, uint16_t version, uint16_t instance);
int qrtr_remove_lookup(int sock, uint32_t service, uint16_t version, uint16_t instance);

int qrtr_poll(int sock, unsigned int ms);
int qrtr_lookup(int sock, uint32_t service, uint16_t version, uint16_t instance, uint32_t ifilter,
		void (* cb)(void *,uint32_t,uint32_t,uint32_t,uint32_t), void *udata);

int qrtr_is_ctrl_addr(struct sockaddr_qrtr *sq);
int qrtr_handle_ctrl_msg(struct sockaddr_qrtr *sq,
			 const void *buf,
			 size_t len,
			 struct qrtr_ind_ops *ops,
			 void *data);

#endif
