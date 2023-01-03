#include "../include/types.h"
#include "../include/timer.h"
#include "../include/hart.h"

void main();
void _mti();

__attribute__ ((aligned(16))) char stack0[8 * 4096];

void start() {
    // Save hartid in tp as instructed by the ABI
    w_tp(r_mhartid());
    // Initialize timer for each hart
    timer_init();
    // Install timer interrupt handler
    w_mtvec((uint64_t)_mti);
    // Enable machine interrupt (global)
    w_mstatus(r_mstatus()|1<<3);
    // Enable machine interrupt (timer)
    w_mie(r_mie()|1<<7);
    // Allow supervisor mode full asscess to all physical addresses
    // by defining the entire physical address space as one range
    // and set it to be readable, writable, and exeutable and lock
    // -ing that configuration against any future writes to pmp regs
    r_pmpaddr0(0x3FFFFFFFFFFFFF); // Highest physical address as TOP (top of range)
    w_pmpcfg0(1<<7|1<<3|1<<2|1<<1|1<<0); // Lock(7)|A(3-4)|X(2)|W(1)|R(0), A field set to 1, meaning pmpaddr0 holds TOP
    // Delegate all traps in M-mode and S-mode to S-mode
    w_medeleg(0xFFFF);
    w_mideleg(0xFFFF);
    // mret to _main in S-mode
    w_mepc((uint64_t)main);
    w_mstatus((r_mstatus()&~(3<<11))|(1<<11));
    asm ("mret");
}
