#ifndef _QRTR_LIB_H_
#define _QRTR_LIB_H_

#include <stdint.h>

struct sockaddr_qrtr;

struct qrtr_packet {
	int type;

	unsigned int node;
	unsigned int port;

	unsigned int service;
	unsigned int instance;
	unsigned int version;

	void *data;
	size_t data_len;
};

struct qrtr_ind_ops {
	int (*bye)(uint32_t node, void *data);
	int (*del_client)(uint32_t node, uint32_t port, void *data);
	int (*new_server)(uint32_t service, uint16_t version, uint16_t instance,
			  uint32_t node, uint32_t port, void *data);
	int (*del_server)(uint32_t service, uint16_t version, uint16_t instance,
			  uint32_t node, uint32_t port, void *data);
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

int qrtr_decode(struct qrtr_packet *dest, void *buf, size_t len,
		const struct sockaddr_qrtr *sq);

/* Initial kernel header didn't expose these */
#ifndef QRTR_NODE_BCAST

#define QRTR_NODE_BCAST 0xffffffffu
#define QRTR_PORT_CTRL  0xfffffffeu

enum qrtr_pkt_type {
        QRTR_TYPE_DATA          = 1,
        QRTR_TYPE_HELLO         = 2,
        QRTR_TYPE_BYE           = 3,
        QRTR_TYPE_NEW_SERVER    = 4,
        QRTR_TYPE_DEL_SERVER    = 5,
        QRTR_TYPE_DEL_CLIENT    = 6,
        QRTR_TYPE_RESUME_TX     = 7,
        QRTR_TYPE_EXIT          = 8,
        QRTR_TYPE_PING          = 9,
        QRTR_TYPE_NEW_LOOKUP    = 10,
        QRTR_TYPE_DEL_LOOKUP    = 11,
};

struct qrtr_ctrl_pkt {
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
} __packed;

#endif
#endif
