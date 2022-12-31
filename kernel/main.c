#include "../include/hart.h"
#include "../include/uart.h"
#include "../include/pm.h"
#include "../include/kprintf.h"
#include "../include/vm.h"
#include "../include/plic.h"

void _strap_stub();

void _main () {
    if (!tp.read()) {
        uart.init();
        kprintf("booting...");
        pmmngr.init();
        stvec.write((uint64_t)_strap_stub);
        vmmngr.init();
        plic.init();
        plic.inithart();
        // Enable supervisor-mode interrupt
        sstatus.write(sstatus.read() | 1 << 1);
        sie.write(sie.read() | 1 << 9);
        asm("de:");
    }
    for(;;);
}
