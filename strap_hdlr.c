#include "console.h"
#include "hart.h"

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
    if (scause.read() & msb)
        printf("%s\n", icause[scause.read() & ~msb]);
    else
        printf("%s\n", ecause[scause.read() & ~msb]);
}   
