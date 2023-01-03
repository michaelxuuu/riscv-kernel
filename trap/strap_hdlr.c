#include "../include/kprintf.h"
#include "../include/hart.h"
#include "../include/plic.h"
#include "../include/uart.h"
#include "../include/disk.h"

char* icause[] = {
    "User software interrupt",
    "Supervisor software interrupt",
    "Reserved",
    "Reserved",
    "User timer interrupt",
    "Supervisor timer interrupt",
    "Reserved",
    "Reserved",
    "User external interrupt",
    "Supervisor external interrupt"
};

char* ecause[] = {
    "Instruction address misaligned",
    "Instruction access fault",
    "Illegal instruction",
    "Breakpoint",
    "Load address misaligned",
    "Load access fault",
    "Store/AMO address misaligned",
    "Store/AMO access fault",
    "Environment call from U-mode",
    "Environment call from S-mode",
    "Reserved",
    "Reserved",
    "Instruction page fault",
    "Load page fault",
    "Reserved",
    "Store/AMO page fault"
};

void _strap_hdlr() {
    uint64_t msb = 1L << 63;
    uint64_t cause = scause.read();
    uint64_t intr = cause & msb;
    uint64_t no = cause & ~msb;
    if (intr) {
        int irq = plic.query();
        if (irq == UART0_IRQ)
            uart.isr();
        else if (irq == VIRTIO0_IRQ)
            disk.isr();
        else kprintf("%s\n", icause[no]);
        plic.eoi(irq);
    }
    else
        kprintf("%s\n", ecause[no]);
}   
