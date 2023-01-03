#include "../include/timer.h"
#include "../include/types.h"
#include "../include/hart.h"

#define MTIME 0x200bff8
#define MTIMECMP_BASE 0x2004000
#define TIMER_INTERVAL 1000000

uint64_t scratch[8][5];

void timer_init() {
    uint64_t hartid = r_mhartid();
    uint64_t volatile mtime = *(uint64_t *)MTIME;
    uint64_t volatile mtimecmp = mtime + TIMER_INTERVAL;
    uint64_t volatile mtimecmp_addr = MTIMECMP_BASE + hartid * sizeof(uint64_t);
    *(uint64_t *)mtimecmp_addr = mtimecmp;
    scratch[hartid][3] = mtimecmp_addr;
    scratch[hartid][4] = TIMER_INTERVAL;
    w_mscratch((uint64_t)(scratch + hartid));
}
