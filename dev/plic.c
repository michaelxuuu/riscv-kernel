#include "../include/plic.h"
#include "../include/hart.h"
#include "../include/mmio.h"

static void init();
static void inithart();
static int query();
static void eoi(int irq);

plic_t plic = {init, inithart, query, eoi};

#define PLIC_SRC_EN_BASE 0xC000000 // irq line (source) enable (global), word size, contiguous
#define PLIC_IRQ_EN_BASE 0xC002080 // irq enable (per hart), word size, 0x100 spacing
#define PLIC_THRES_BASE 0xC201000 // irq threashold (per hart), word size, 0x2000 spacing
#define PLIC_CLIAM_BASE 0xC201004 // claim register (per hart), word size, 0x2000 spacing

// enable irq lines by setting their priorities.
// here we enable line 1 for virtio0 and 10 uart0
// and froce no priority by which the priority is
// automatically decided by the irq number.
// interrupt source priority registers are of WORD
// sizes and mapped contigously in memory
void init() {
    mmio_writew(PLIC_SRC_EN_BASE + 4 * UART0_IRQ, 1);
    mmio_writew(PLIC_SRC_EN_BASE + 4 * VIRTIO0_IRQ, 1);
}

// enable uart0 and virtio0 interrupt for a specific hart
// The enables registers are accessed as a contiguous array of 2 × 32-bit words
// indexed by hart id
// The first word is for machine mode and the second supervisor
// We only alter the second word since we delegated all interrupts to S-mode
void inithart() {
    uint64_t hartid = r_tp();
    mmio_writew(PLIC_IRQ_EN_BASE + hartid * 0x100, 1 << VIRTIO0_IRQ | 1 << UART0_IRQ);
    mmio_writew(PLIC_THRES_BASE + hartid * 0x2000, 0);
}

int query(void) {
    return mmio_readw(PLIC_CLIAM_BASE + r_tp() * 0x2000);
}

void eoi(int irq) {
    mmio_writew(PLIC_CLIAM_BASE + r_tp() * 0x2000, irq);
}
