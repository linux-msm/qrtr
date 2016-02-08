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
