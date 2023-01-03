#ifndef _disk_h_
#define _disk_h_

#include "bio.h"

typedef struct disk {
    void (*init)(void);
    void (*rw)(buf_t *b, bool w);
    void (*isr)(void);
} disk_t;


extern disk_t disk;

#endif
