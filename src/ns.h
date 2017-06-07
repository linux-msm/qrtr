#ifndef __NS_H_
#define __NS_H_

#include <endian.h>
#include <stdint.h>

typedef uint16_t __le16;
typedef uint16_t __be16;
typedef uint32_t __le32;
typedef uint32_t __be32;
typedef uint64_t __le64;
typedef uint64_t __be64;

static inline __le32 cpu_to_le32(uint32_t x) { return htole32(x); }
static inline uint32_t le32_to_cpu(__le32 x) { return le32toh(x); }

#define NS_PORT 53
#define QRTR_CTRL_PORT ((unsigned int)-2)

enum ns_pkt_type {
	NS_PKT_RESET		= 0,
	NS_PKT_PUBLISH		= 1,
	NS_PKT_QUERY		= 3,
	NS_PKT_NOTICE		= 4,
	NS_PKT_BYE		= 5,
};

struct ns_pkt {
	__le32 type;
	union {
		struct {
			__le32 service;
			__le32 instance;
			__le32 ifilter;
		} query;

		struct {
			__le32 service;
			__le32 instance;
		} publish, bye;

		struct {
			__le32 seq;
			__le32 service;
			__le32 instance;
			__le32 node;
			__le32 port;
		} notice;
	};
} __attribute__((packed));

enum ctrl_pkt_cmd {
	QRTR_CMD_HELLO		= 2,
	QRTR_CMD_BYE		= 3,
	QRTR_CMD_NEW_SERVER	= 4,
	QRTR_CMD_DEL_SERVER	= 5,
	QRTR_CMD_DEL_CLIENT	= 6,
	QRTR_CMD_RESUME_TX	= 7,
	QRTR_CMD_EXIT		= 8,
	QRTR_CMD_PING		= 9,
	QRTR_CMD_NEW_LOOKUP	= 10,
	QRTR_CMD_DEL_LOOKUP	= 11,
	QRTR_CMD_LOOKUP_RESULT	= 12,
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
} __attribute__((packed));

#endif
