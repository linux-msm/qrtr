#ifndef _l_TYPES_H__
#define _l_TYPES_H__

typedef unsigned long long u64;
typedef   signed long long s64;
typedef unsigned int       u32;
typedef   signed int       s32;
typedef unsigned short     u16;
typedef   signed short     s16;
typedef unsigned char      u8;
typedef   signed char      s8;

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#define min MIN
#endif

#ifndef MAX
#define MAX(x,y) (((x)>(y))?(x):(y))
#define max MAX
#endif


#endif
