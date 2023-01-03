#include "../include/bio.h"
#include "../include/spinlk.h"
#include "../include/kpanic.h"
#include "../include/disk.h"

static buf_t bufs[30];
static buf_t *mru;
static buf_t *lru;
static spinlk_t lk = SPINLK_INITIALIZER;

static void init();
static buf_t* bget(uint32_t, uint32_t);
static buf_t* bread(uint32_t, uint32_t);
static void bwrite(buf_t*);
static void brelease(buf_t*);

bio_t bio = {init, bread, bwrite, brelease};

void init () {
    for (int i = 0; i < 30; i++) {
        bufs[i].prev = (i == 0) ? 0 : &bufs[i-1];
        bufs[i].next = (i == 29) ? 0 : &bufs[i+1];
    }
    mru = &bufs[0];
    lru = &bufs[29];
}

buf_t* bget(uint32_t dev, uint32_t blockno) {
    spinlk_acquire(&lk);

    buf_t *p = 0;

    for (p = mru; p; p = p->next)
        if (p->dev == dev && p->blockno == blockno) {
            p->refct++;
            spinlk_release(&lk);
            return p;
        }
    
    for (p = lru; p; p = p->prev)
        if (!p->refct) {
            p->dev = dev;
            p->blockno = blockno;
            p->valid = 0;
            p->refct = 1;
            spinlk_release(&lk);
            return p;
        }
    
    kpanic("bget: no buffers\n");
    return 0;
}

buf_t* bread(uint32_t dev, uint32_t blockno) {
    buf_t* b = bget(dev, blockno);
    if (!b->valid) {
        b->valid = 1;
        // disk read
        disk.rw(b, 0);
    }
    return b;
}

void bwrite(buf_t* b)
{
    // disk write
    disk.rw(b, 1);
}

void brelease(buf_t *b) {
    spinlk_acquire(&lk);

    b->refct--;

    if (!b->refct) {
        b->next = mru;
        b->prev = 0;
        mru->prev = b;
        mru = b;
        if (b == lru)
            lru = lru->prev;
    }

    spinlk_release(&lk);
}
