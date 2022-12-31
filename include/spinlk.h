#ifndef _spinlk_h_
#define _spinlk_h_

struct spinlk
{
    int lk; // The lock varible one has to auquire to enter the guarded region
};

typedef struct spinlk spinlk_t;

#define SPINLK_INITIALIZER {0}

void spinlk_init(spinlk_t *lk);

// BLOCK (spin) until lk is =acquired
// Return if lk is acquired
void spinlk_acquire(spinlk_t *lk);

// Release the lock
void spinlk_release(spinlk_t *lk);


#endif
