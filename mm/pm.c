#include "../include/pm.h"
#include "../include/util.h"
#include "../include/spinlk.h"

/*
    DRAM layout:

        +------------------+  <-  _ram_end
        |       free       |
        +------------------+  <-  _bss_end (4K aligned)
        |       bss        |
        +------------------+
        |       data       |
        +------------------+
        |      rodata      |
        +------------------+  <-  _text_end
        |       text       |
        +------------------+  <-  0x80000000

    Kernel code lives below the free section

    Free section is avaiable for allocation 
    and managed by the physical memory manager
*/

static void init();
static pa_t alloc();
static void free(pa_t);

pmmngr_t pmmngr = {init, alloc, free};

typedef struct blk blk_t;
struct blk {
    blk_t *next;
    char _[4096 - sizeof(blk_t *)];
};

extern char _bss_end[]; // Defined in linker script
extern char _ram_end[]; // Defined in linker script

// A "stack" of free blocks
// A block is added to the top when freed
// and is taken from the top when allocated
static struct {
    spinlk_t lk;
    blk_t*  top;
} bstack = {SPINLK_INITIALIZER, 0};

// init will be called only once in _main,
// and is only executed by hart 0, so we do
// not need to lock freeblks (Other harts 
// are blocked until hart 0 finishes the
// entire initilization process)
void init() {
    // Divide the free region into 4K blocks
    // and push them all into the free block stack

    // First block
    if ((blk_t *)_bss_end < (blk_t *)_ram_end) {
        bstack.top = (blk_t *)_bss_end;
        bstack.top->next = 0;
    }

    for (blk_t *p = (blk_t *)_bss_end; p < (blk_t *)_ram_end; p += 1) {
        p->next = bstack.top;
        bstack.top = p;
    }
}

pa_t alloc() {
    spinlk_acquire(&bstack.lk);
    pa_t b = (pa_t)bstack.top;
    bstack.top = bstack.top->next;
    spinlk_release(&bstack.lk);
    return b; // will return 0 if bstack is empty meaning on free blocks
}

void free(pa_t pa) {
    blk_t *p = (blk_t*)pa;

    spinlk_acquire(&bstack.lk);
    if (!bstack.top) {
        p->next = 0;
        bstack.top = p;
    }
    else {
        p->next = bstack.top;
        bstack.top = bstack.top->next;
    }
    spinlk_release(&bstack.lk);
}
