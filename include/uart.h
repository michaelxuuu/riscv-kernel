#ifndef _uart_h_
#define _uart_h_

// Uart interface
struct uart
{
    void (*init)(void);
    void (*putc)(char);
    void (*putc_sync)(char c);
    char (*getc)(void);
    void (*isr)(void);
};

typedef struct uart uart_t;

extern uart_t uart; // Defined in uart.c

#endif
