#ifndef __NS_H_
#define __NS_H_

#include "endian.h"

#define NS_PORT 53

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

#endif
