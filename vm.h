#ifndef _vm_h_
#define _vm_h_

#include "types.h"

enum pte_flgs {
    PTE_R = 0x2,
    PTE_W = 0x4,
    PTE_X = 0x8
};

typedef uint64_t va_t;

typedef struct vmmngr {
    void (*init)(void);
} vmmngr_t;

extern vmmngr_t vmmngr;

#endif
