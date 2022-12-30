#include "hart.h"
#include "uart.h"
#include "pm.h"
#include "kprintf.h"
#include "vm.h"

void _strap_stub();

void _main () {
    // Enable supervisor-mode interrupt
    sie.write(sie.read() | 1 << 9 | 1 << 5 | 1 << 1);
    if (!tp.read()) {
        uart.init();
        kprintf("booting...");
        pmmngr.init();
        stvec.write((uint64_t)_strap_stub);
        vmmngr.init();
    }
    for(;;);
}
