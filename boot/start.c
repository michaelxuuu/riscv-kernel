#include "../include/types.h"
#include "../include/timer.h"
#include "../include/hart.h"

void _main();
void _mti();

__attribute__ ((aligned(16))) char stack0[8 * 4096];

void _start() {
    // Save hartid in tp as instructed by the ABI
    tp.write(mhartid.read());
    // Initialize timer for each hart
    timer_init();
    // Install timer interrupt handler
    mtvec.write((uint64_t)_mti);
    // Enable machine interrupt (global)
    mstatus.write(mstatus.read() | 1 << 3);
    // Enable machine interrupt (timer)
    mie.write(mie.read() | 1 << 7);
    // Allow supervisor mode full asscess to all physical addresses
    // by defining the entire physical address space as one range
    // and set it to be readable, writable, and exeutable and lock
    // -ing that configuration against any future writes to pmp regs
    pmpaddr0.write(0x3FFFFFFFFFFFFF); // Highest physical address as TOP (top of range)
    pmpcfg0.write(1 << 7 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0); // Lock(7)|A(3-4)|X(2)|W(1)|R(0), A field set to 1, meaning pmpaddr0 holds TOP
    // Delegate all traps in M-mode and S-mode to S-mode
    medeleg.write(0xFFFF);
    mideleg.write(0xFFFF);
    // mret to _main in S-mode
    mepc.write((uint64_t)_main);
    mstatus.write((mstatus.read() & ~(3 << 11)) | 1 << 11);
    asm ("mret");
}
