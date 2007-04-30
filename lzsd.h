#ifndef _LZSD_H_
#define _LZSD_H_

#include <stdint.h>

uint32_t lzs_unpack(void *src, uint32_t srcsize, void *dst, uint32_t dstsize);

#endif /* _LZSD_H_ */
