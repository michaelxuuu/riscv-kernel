#ifndef _mmio_h_
#define _mmio_h_

#include "types.h"

#define mmio_writeb(addr, val) \
    *(volatile uint8_t *)(addr) = val

#define mmio_readb(addr) \
    *(volatile uint8_t *)(addr)

#define mmio_writew(addr, val) \
    *(volatile uint32_t *)(addr) = val

#define mmio_readw(addr) \
    *(volatile uint32_t *)(addr)

#endif
