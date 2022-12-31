#include "../include/kprintf.h"
#include "../include/spinlk.h"
#include "../include/uart.h"
#include "../include/types.h"

static spinlk_t lk = SPINLK_INITIALIZER;

void printint(int x, int base) {
    char buf[16] = {'0'}; // longest integer takes up to 10 digites but stack is 16-byte aligned...
    long xx = x;
    if (xx >> 31) // if MSB is 1 we convert the negative integer to positive
        xx = -xx;

    int i = 0;
    for (; xx; i++, xx /= base) {
        int rem = xx % base;
        char c = '0' + rem;
        if (rem > 9)
            c = 'a' + rem - 10;
        buf[i] = c;
    }

    for (; i >= 0; i--)
        uart.putc_sync(buf[i]);
}

void printstr(char *s) {
    for (int i = 0; s[i]; i++)
        uart.putc_sync(s[i]);
}

void printptr(uint64_t x) {
    uart.putc_sync('0');
    uart.putc_sync('x');

    char buf[16] = {'0'};
    
    int i = 0;
    for (; x; i++, x /= 16) {
        int rem = x % 16;
        char c = '0' + rem;
        if (rem > 9)
            c = 'a' + rem - 10;
        buf[i] = c;
    }

    for (; i >= 0; i--)
        uart.putc_sync(buf[i]);
}

// %d, %x, %p, %s
void kprintf(const char *fmt, ...) {
    spinlk_acquire(&lk); // No interleaving priniting (Must use synchronous uart.putc to avoid deadlock)
    va_list ap;
    va_start(ap, fmt);
    for (int i = 0; fmt[i]; i++) {
        if (fmt[i] == '%') {
            char c = fmt[++i];
            if (c == 'd') printint(va_arg(ap, int), 10);
            else if (c == 's') printstr(va_arg(ap, char*));
            else if (c == 'x') printint(va_arg(ap, int), 16);
            else if (c == 'p') printptr(va_arg(ap, uint64_t));
            else if (c == 0) break;
            else uart.putc_sync(fmt[i]);
        }
        else uart.putc_sync(fmt[i]);
    }
    va_end(ap);
    spinlk_release(&lk);
}
