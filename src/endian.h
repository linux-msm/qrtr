#ifndef __ENDIAN_H
#define __ENDIAN_H

#include "types.h"

typedef u16 __le16;
typedef u16 __be16;
typedef u32 __le32;
typedef u32 __be32;
typedef u64 __le64;
typedef u64 __be64;

#if !defined(_BIG_ENDIAN) || (_BIG_ENDIAN == 0)
# define __ENDIAN_IS_BIG 0
#else /* _BIG_ENDIAN && _BIG_ENDIAN != 0 */
# if defined(_BYTE_ORDER) && (_BYTE_ORDER != _BIG_ENDIAN)
#  define __ENDIAN_IS_BIG 0
# else /* !_BYTE_ORDER || _BYTE_ORDER == _BIG_ENDIAN */
#  define __ENDIAN_IS_BIG 1
# endif /* !_BYTE_ORDER || _BYTE_ORDER == _BIG_ENDIAN */
#endif /* _BIG_ENDIAN && _BIG_ENDIAN != 0 */

#define endian_swap_u64(x)                 \
  ((((x) & 0xff00000000000000ULL) >> 56) | \
   (((x) & 0x00ff000000000000ULL) >> 40) | \
   (((x) & 0x0000ff0000000000ULL) >> 24) | \
   (((x) & 0x000000ff00000000ULL) >>  8) | \
   (((x) & 0x00000000ff000000ULL) <<  8) | \
   (((x) & 0x0000000000ff0000ULL) << 24) | \
   (((x) & 0x000000000000ff00ULL) << 40) | \
   (((x) & 0x00000000000000ffULL) << 56))
#define endian_swap_u32(x)      \
  ((((x) & 0xff000000) >> 24) | \
   (((x) & 0x00ff0000) >>  8) | \
   (((x) & 0x0000ff00) <<  8) | \
   (((x) & 0x000000ff) << 24))
#define endian_swap_u16(x) \
  ((((x) & 0xff00) >> 8) | \
   (((x) & 0x00ff) << 8))

#if (__ENDIAN_IS_BIG == 0)
#define be64_to_cpu(x) endian_swap_u64(x)
#define be32_to_cpu(x) endian_swap_u32(x)
#define be16_to_cpu(x) endian_swap_u16(x)
#define le64_to_cpu(x) (x)
#define le32_to_cpu(x) (x)
#define le16_to_cpu(x) (x)
#define cpu_to_be64(x) endian_swap_u64(x)
#define cpu_to_be32(x) endian_swap_u32(x)
#define cpu_to_be16(x) endian_swap_u16(x)
#define cpu_to_le64(x) (x)
#define cpu_to_le32(x) (x)
#define cpu_to_le16(x) (x)
#else /* __ENDIAN_IS_BIG != 0 */
#define be64_to_cpu(x) (x)
#define be32_to_cpu(x) (x)
#define be16_to_cpu(x) (x)
#define le64_to_cpu(x) endian_swap_u64(x)
#define le32_to_cpu(x) endian_swap_u32(x)
#define le16_to_cpu(x) endian_swap_u16(x)
#define cpu_to_be64(x) (x)
#define cpu_to_be32(x) (x)
#define cpu_to_be16(x) (x)
#define cpu_to_le64(x) endian_swap_u64(x)
#define cpu_to_le32(x) endian_swap_u32(x)
#define cpu_to_le16(x) endian_swap_u16(x)
#endif /* __ENDIAN_IS_BIG != 0 */

#define be64p_to_cpu(x) be64_to_cpu(*(u64 *)(x))
#define be32p_to_cpu(x) be32_to_cpu(*(u32 *)(x))
#define be16p_to_cpu(x) be16_to_cpu(*(u16 *)(x))
#define le64p_to_cpu(x) le64_to_cpu(*(u64 *)(x))
#define le32p_to_cpu(x) le32_to_cpu(*(u32 *)(x))
#define le16p_to_cpu(x) le16_to_cpu(*(u16 *)(x))
#define cpup_to_be64(x) cpu_to_be64(*(u64 *)(x))
#define cpup_to_be32(x) cpu_to_be32(*(u32 *)(x))
#define cpup_to_be16(x) cpu_to_be16(*(u16 *)(x))
#define cpup_to_le64(x) cpu_to_le64(*(u64 *)(x))
#define cpup_to_le32(x) cpu_to_le32(*(u32 *)(x))
#define cpup_to_le16(x) cpu_to_le16(*(u16 *)(x))

#define cpu_to_le(x) \
	((sizeof(x) == sizeof(u64)) ? cpu_to_le64(x) : \
	 (sizeof(x) == sizeof(u32)) ? cpu_to_le32(x) : \
	 (sizeof(x) == sizeof(u16)) ? cpu_to_le16(x) : (x))
#define cpu_to_be(x) \
	((sizeof(x) == sizeof(u64)) ? cpu_to_be64(x) : \
	 (sizeof(x) == sizeof(u32)) ? cpu_to_be32(x) : \
	 (sizeof(x) == sizeof(u16)) ? cpu_to_be16(x) : (x))
#define le_to_cpu(x) cpu_to_le(x)
#define be_to_cpu(x) cpu_to_be(x)

#define cpup_to_le(x) cpu_to_le(*(x))
#define cpup_to_be(x) cpu_to_be(*(x))
#define lep_to_cpu(x) cpup_to_le(x)
#define bep_to_cpu(x) cpup_to_be(x)

#define net64_to_cpu(x) be64_to_cpu(x)
#define net32_to_cpu(x) be32_to_cpu(x)
#define net16_to_cpu(x) be16_to_cpu(x)
#define cpu_to_net64(x) cpu_to_be64(x)
#define cpu_to_net32(x) cpu_to_be32(x)
#define cpu_to_net16(x) cpu_to_be16(x)

#define net64p_to_cpu(x) be64p_to_cpu(x)
#define net32p_to_cpu(x) be32p_to_cpu(x)
#define net16p_to_cpu(x) be16p_to_cpu(x)
#define cpup_to_net64(x) cpup_to_be64(x)
#define cpup_to_net32(x) cpup_to_be32(x)
#define cpup_to_net16(x) cpup_to_be16(x)

#define cpu_to_net(x) cpu_to_be(x)
#define net_to_cpu(x) be_to_cpu(x)

#define cpup_to_net(x) cpup_to_be(x)
#define netp_to_cpu(x) bep_to_cpu(x)

#endif
