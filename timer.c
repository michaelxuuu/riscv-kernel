#include "timer.h"
#include "types.h"
#include "hart.h"

#define MTIME_ADDR 0x200bff8
#define MTIMECMP_BASE_ADDR 0x2004000
#define TIMER_INTERVAL 1000000

uint64_t scratch[8][5];

void timer_init() {
    uint64_t hartid = mhartid.read();
    uint64_t volatile mtime = *(uint64_t *)MTIME_ADDR;
    uint64_t volatile mtimecmp = mtime + TIMER_INTERVAL;
    uint64_t volatile mtimecmp_addr = MTIMECMP_BASE_ADDR + hartid * sizeof(uint64_t);
    *(uint64_t *)mtimecmp_addr = mtimecmp;
    scratch[hartid][3] = mtimecmp_addr;
    scratch[hartid][4] = TIMER_INTERVAL;
    mscratch.write((uint64_t)(scratch + hartid));
}
