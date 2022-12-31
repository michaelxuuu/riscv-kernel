#ifndef _console_h_
#define _console_h_

typedef struct console {
    void (*init)(void);
    void (*putc)(char);
    void (*isr)(char);
} console_t;

extern console_t console;

#endif
