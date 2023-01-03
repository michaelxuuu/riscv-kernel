#ifndef _sync_h_
#define _sync_h_

#define sync() asm volatile ("fence")

#endif
