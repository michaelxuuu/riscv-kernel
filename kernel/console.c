#include "../include/console.h"
#include "../include/spinlk.h"
#include "../include/uart.h"
#include "../include/types.h"
#include "../include/kprintf.h"

#define LINESIZE 16
static char line[LINESIZE];
static int head = 0;
static int tail = 0;
static int edit = 0;
static spinlk_t lk = SPINLK_INITIALIZER;

static void putc(char c);
static void isr(char c);

console_t console = {0, putc, isr};

static void putc(char c) {
    if(c == '\b'){
        uart.putc_sync('\b'); 
        uart.putc_sync(' '); 
        uart.putc_sync('\b');
    } 
    else uart.putc_sync(c);
}

#define BACKSPACE '\b'
#define Ctrl(x)  ((x)-'@')  // Control-x

// called by uart isr
void isr(char c) {
    spinlk_acquire(&lk);

    switch (c)
    {
        case Ctrl('P'):
            kprintf("procdump\n");
            break;
        
        case Ctrl('U'):
            while(edit != head &&
                  line[(edit-1) % LINESIZE] != '\n'){
              edit--;
              putc(BACKSPACE);
            }
        
        case Ctrl('H'): // Backspace
        case '\x7f': // Delete key
            if(edit != head){
              edit--;
              putc(BACKSPACE);
            }
            break;
        default:
            if (c != 0 && edit - tail < LINESIZE) {
                c = (c == '\r') ? '\n' : c;

                // always echo back to the user
                putc(c);

                // store for consumption by processes who's waiting on console input
                line[edit++ % LINESIZE] = c;

                // wake up
                if (c == '\n')
                    edit = 0;
            }
            else {
                putc('\n');
                putc(c);
                edit = 0;
            }
            break;
    }

    spinlk_release(&lk);
}

