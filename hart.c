#include "hart.h"

#define MK_REG_OBJ_RO(name) \
uint64_t name##_read() { \
    uint64_t name; \
    asm ("csrr %0,"#name : "=r"(name)); \
    return name; \
} \
name##_t name = {name##_read};

#define MK_REG_OBJ_WO(name) \
void name##_write(uint64_t name) { \
    asm ("csrw " #name ",%0" :: "r"(name)); \
} \
name##_t name = {name##_write};

#define MK_CSR_OBJ_RW(name) \
uint64_t name##_read() { \
    uint64_t name; \
    asm ("csrr %0,"#name : "=r"(name)); \
    return name; \
} \
void name##_write(uint64_t name) { \
    asm ("csrw " #name ",%0" :: "r"(name)); \
} \
name##_t name = {name##_read, name##_write};

#define MK_GPR_OBJ_RW(name) \
uint64_t name##_read() { \
    uint64_t name; \
    asm ("mv %0,"#name : "=r"(name)); \
    return name; \
} \
void name##_write(uint64_t name) { \
    asm ("mv " #name ",%0" :: "r"(name)); \
} \
name##_t name = {name##_read, name##_write};

MK_REG_OBJ_RO(mhartid)
MK_CSR_OBJ_RW(mscratch)
MK_CSR_OBJ_RW(mtvec)
MK_CSR_OBJ_RW(mstatus)
MK_CSR_OBJ_RW(mie)
MK_CSR_OBJ_RW(mepc)
MK_CSR_OBJ_RW(pmpaddr0)
MK_CSR_OBJ_RW(pmpcfg0)
MK_CSR_OBJ_RW(medeleg)
MK_CSR_OBJ_RW(mideleg)
MK_CSR_OBJ_RW(satp)
MK_CSR_OBJ_RW(scause)
MK_CSR_OBJ_RW(sie)
MK_CSR_OBJ_RW(sstatus)
MK_CSR_OBJ_RW(stvec)
MK_GPR_OBJ_RW(tp)
MK_GPR_OBJ_RW(sp)
MK_GPR_OBJ_RW(ra)

