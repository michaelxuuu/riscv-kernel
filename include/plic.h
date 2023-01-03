#ifndef _plic_h_
#define _plic_h_

#define UART0_IRQ 10
#define VIRTIO0_IRQ 1

typedef struct plic {
    void (*init)(void); // enable irq lines (global)
    void (*inithart)(void); // enable irq (per hart)
    int (*query)(void); // query interrupt id
    void (*eoi)(int);  // send EOI (end of interrupt)
} plic_t;

extern plic_t plic;

#endif
