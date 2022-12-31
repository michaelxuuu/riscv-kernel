#include "../include/uart.h"
#include "../include/spinlk.h"
#include "../include/mmio.h"
#include "../include/console.h"

#define UART_BASE 0x10000000L

#define REG_ADDR(offset) UART_BASE + offset

// 7 memory-mapped uart registers used
enum uart_regs {
    RDR = REG_ADDR(0), // Rx data reg
    TDR = REG_ADDR(0), // Tx data reg

    DLL = REG_ADDR(0), // Divisor latch low
    DLH = REG_ADDR(1), // Divisor latch high

    IER = REG_ADDR(1), // Intr enable reg
    ISR = REG_ADDR(2), // Intr stat reg

    FCR = REG_ADDR(2), // FIFO ctrl reg

    LCR = REG_ADDR(3), // Line ctrl reg
    LSR = REG_ADDR(5)  // Line stat reg
};

enum uart_masks {
    IER_RX = 1 << 0, // Rx Intr enable bit (Interrupt when data recieved)
    IER_TX = 1 << 1, // Tx Intr enable bit (Interrupt when there is space in transmit buffer)

    FCR_EN = 1 << 0, // FIFO enable bit
    FCR_RS = 1 << 2, // FIFO reset bit

    LCR_LATCH = 1 << 7, // Divisor latch bit
    LCR_WLEN8 = 3 << 0, // Word length of 8 bits

    LSR_RDR_READY = 1 << 0, // Input's waiting in RDR to be read
    LSR_TDR_EMPTY = 1 << 5  // THR can accept another character to send
};

static void init(void);
static void putc(char c);
static void putc_sync(char c);
static char getc(void);
static void isr(void);
static void bflush(void);

// Initialize interface
uart_t uart = {init, putc, putc_sync, getc, isr};

// Spinlock for mutual exclusion between cores
spinlk_t lk;

// Transmit buffer
#define BUFSIZE 32
char buf[BUFSIZE];
int head;
int tail;

/*
    This transmit buffer sits between the user and the uart device,
    and calls to uart.putc() will not result in a byte immediately
    handed to uart but put in `buf` temporarily and will later
    be transmitted all at once with potentially other leftover bytes
    strayed in the buffer from the past
*/

void init() {
    spinlk_init(&lk);
    mmio_writeb(IER, 0);
    mmio_writeb(LCR, LCR_LATCH);
    mmio_writeb(DLL, 3);
    mmio_writeb(DLH, 0);
    mmio_writeb(LCR, LCR_WLEN8);
    mmio_writeb(FCR, FCR_RS | FCR_EN);
    mmio_writeb(IER, IER_RX);
}

void putc(char c) {
    spinlk_acquire(&lk);

    while (tail == head + BUFSIZE) // full
        ; // sleep and wait to be woken by bdrain which opens up space in xtbuf

    buf[tail++ % BUFSIZE] = c;

    bflush(); // try to flush all bytes in buffer to uart all at once

    spinlk_release(&lk);
}

void putc_sync(char c) {
    // Trun off interrupt

    while (!(mmio_readb(LSR) & LSR_TDR_EMPTY));

    mmio_writeb(TDR, c);

    // Turn on interrupt
}

char getc() {
    if (mmio_readb(LSR) & LSR_RDR_READY)
        return mmio_readb(RDR);
    else
        return -1;
}

// Uart interrupt handler
void isr() {
    // data arrived or ready for more input or both
    char c;
    while ((c = getc()) != (char)-1)
    {
        // data for console
        console.isr(c);
    }

    spinlk_acquire(&lk);
    bflush();
    spinlk_release(&lk);
}

// Try to flush all bytes in buffer into uart all at once
void bflush() {

    // buf is not empty && TDR is idle
    while (head != tail)
    {
        if (!(mmio_readb(LSR) & LSR_TDR_EMPTY))
            return;

        char c = buf[head++];

        // wake up threads waiting for space in the buffer

        mmio_writeb(TDR, c);
    }
    
}
