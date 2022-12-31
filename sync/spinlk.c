#include "../include/spinlk.h"

void spinlk_init(spinlk_t *lk) {
    lk->lk = 0;
}

// BLOCK (spin) until lk is acquired
// Return if lk is acquired
void spinlk_acquire(spinlk_t *lk) {
    asm (
        "try_lk:"
            "li t0, 1;"
            "mv t1, %0;"
            "amoswap.w.aq t0, t0, (t1);"
            "bnez t0, try_lk;"
        :: "r"(&lk->lk)
    );
}

// Release the lock
void spinlk_release(spinlk_t *lk) {
    asm (
        "mv t0, %0;"
        "amoswap.w.rl zero, zero, (t0);"
        :: "r"(&lk->lk)
    );
}
