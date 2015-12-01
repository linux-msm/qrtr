#ifndef offsetof
#define offsetof(type, md) ((unsigned long)&((type *)0)->md)
#endif

#ifndef container_of
#define container_of(ptr, type, member) \
  ((type *)((char *)(ptr) - offsetof(type, member)))
#endif
