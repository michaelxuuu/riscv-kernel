#ifndef _hart_h_
#define _hart_h_

#include "types.h"

#define REG_OBJ_RO(name) \
struct name { \
    uint64_t (*read) (void); \
}; \
typedef struct name name##_t; \
extern name##_t name; \

#define REG_OBJ_WO(name) \
struct name { \
    void (*write) (uint64_t); \
}; \
typedef struct name name##_t; \
extern name##_t name; \

#define REG_OBJ_RW(name) \
struct name { \
    uint64_t (*read) (void); \
    void (*write) (uint64_t); \
}; \
typedef struct name name##_t; \
extern name##_t name; \

REG_OBJ_RO(mhartid)
REG_OBJ_RW(mscratch)
REG_OBJ_RW(mtvec)
REG_OBJ_RW(mstatus)
REG_OBJ_RW(mie)
REG_OBJ_RW(mepc)
REG_OBJ_RW(pmpaddr0)
REG_OBJ_RW(pmpcfg0)
REG_OBJ_RW(medeleg)
REG_OBJ_RW(mideleg)
REG_OBJ_RW(satp)
REG_OBJ_RW(sie)
REG_OBJ_RW(sstatus)
REG_OBJ_RW(scause)
REG_OBJ_RW(stvec)
REG_OBJ_RW(tp)
REG_OBJ_RW(sp)
REG_OBJ_RW(ra)

#endif
