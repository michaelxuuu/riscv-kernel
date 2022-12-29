#ifndef _pm_h_
#define _pm_h_

#include "types.h"

typedef uint64_t pa_t;  // pa: physical address

// Physical memory manager interface
struct pmmngr{
    void (*init)(void);
    pa_t (*alloc)(void);
    void (*free)(pa_t);
};

typedef struct pmmngr pmmngr_t;

extern pmmngr_t pmmngr; // Defined in pm.c

#endif
