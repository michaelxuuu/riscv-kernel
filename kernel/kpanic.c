#include "../include/kpanic.h"
#include "../include/kprintf.h"

bool paniced = 0;

void kpanic(const char *info) {
    kprintf("Panic: %s\n", info);
    for (;;);
}
