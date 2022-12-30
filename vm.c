#include "vm.h"
#include "types.h"
#include "pm.h"
#include "kpanic.h"
#include "util.h"
#include "hart.h"

typedef struct pte {
    uint64_t valid:1;
    uint64_t readable:1;
    uint64_t writable:1;
    uint64_t executable:1;
    uint64_t user:1;
    uint64_t global:1;
    uint64_t accessed:1;
    uint64_t dirty:1;
    uint64_t reserved1:2;
    uint64_t ppn:44;
    uint64_t reserved2:10;
} __attribute__((packed)) pte_t;

typedef pte_t pteflgs_t;

typedef struct pt {
    pte_t arr[512];
} __attribute__((packed)) pt_t;

typedef struct va_vpn {
    uint64_t l:9;
    uint64_t m:9;
    uint64_t h:9;
} __attribute__((packed)) va_vpn_t;

typedef struct _va {
    uint64_t vpo:12;
    uint64_t vpn1:9;
    uint64_t vpn2:9;
    uint64_t vpn3:9;
    uint64_t paddinh:25;
} __attribute__((packed)) _va_t;

static pt_t *kernelpt = 0;

static void init(void);

vmmngr_t vmmngr = {init};

static void init_map(pa_t pa, va_t va, int flags) {

    // starting from pt1
    // recursively find pt3
    // and write pa to
    // the corresponding offset.
    // allocate page tables
    // along the way if necessary

    // break va into segments
    _va_t* _va = (_va_t*)&va;

    pt_t *pt1, *pt2, *pt3;
    pte_t *pte1, *pte2, *pte3;

    if (!kernelpt) {
        kernelpt = (pt_t*)pmmngr.alloc();
        memset((void*)kernelpt, 0, 4096);
    }

    // obtain pt1 and pte1
    pt1 = kernelpt;
    /*
        vpn[30:39]
            |        pt1 --> +--------+
            |                |        |
            |                |        |
            +--------------> |  pte1  | ---------> pt2 (could be unallocated) 
                             |        |
                             +--------+
    */
    pte1 = &pt1->arr[_va->vpn3];

    // allocate pt2 if unallocated
    if (!pte1->valid) {
        pte1->valid = 1;
        pa_t pa = pmmngr.alloc();
        pte1->ppn = pa >> 12;
        memset((void*)(uint64_t)pa, 0, 4096);
    }

    // obtain pt2 and pte2
    pt2 = (pt_t *)((uint64_t)pte1->ppn << 12);
    pte2 = &pt2->arr[_va->vpn2];

    if (!pte2->valid) {
        pte2->valid = 1;
        pa_t pa = pmmngr.alloc();
        pte2->ppn = pa >> 12;
        memset((void*)(uint64_t)pa, 0, 4096);
    }

    // obtain pt3 and pte3
    pt3 = (pt_t *)((uint64_t)pte2->ppn << 12);
    pte3 = &pt3->arr[_va->vpn1];
    // Panic if already mapped
    // Write pte3 with the intended PPN and flags otherwise
    if (pte3->valid)
        kpanic("remap");

    pte3->valid = 1;
    pte3->ppn = pa >> 12;
    pte3->readable = !!(flags & PTE_R);
    pte3->writable = !!(flags & PTE_W);
    pte3->executable = !!(flags & PTE_X);
}


/*
    kernel address space layout

    +-------------------------------+ 0x8000000000 (512G)
    |         Trampoline            |
    +-------------------------------+
    |         Guard page            |
    +-------------------------------+
    |         K stack 0             |
    +-------------------------------+
    |         Guard page            |
    +-------------------------------+
    |         K stack 1             |
    +-------------------------------+
    |                               |
    |                               |
    |             ...               |
    |                               |
    |                               |
    +-------------------------------+ 0x88000000 (2G + 128M)
    |         Kernel data           | RW
    +-------------------------------+
    |         Kernel Text           | RX
    +-------------------------------+ 0x80000000 (2G)
    |         Unmapped              |
    +-------------------------------+
    |         VIRTO Disk            |
    +-------------------------------+ 0x10001000
    |         Uart 0                |
    +-------------------------------+ 0x10000000
    |         Unmapped              |
    +-------------------------------+ 0x0C400000
    |         PLIC                  |
    +-------------------------------+ 0x0C000000
    |         Unmapped              |
    +-------------------------------+
    |         CLINT                 |
    +-------------------------------+ 0x02000000
    |         Unmapped              |
    +-------------------------------+
*/

extern char _text_end[];
extern char _ram_end[];

void init() {
    // PLIC
    for (pa_t pa = 0xC000000; pa < 0xC400000; pa += 4096)
        init_map(pa, pa, PTE_R | PTE_W);
    
    // UART 0
    for (pa_t pa = 0x10000000; pa < 0x10001000; pa += 4096)
        init_map(pa, pa, PTE_R | PTE_W);

    // VIRTO Disk
    for (pa_t pa = 0x10001000; pa < 0x10002000; pa += 4096)
        init_map(pa, pa, PTE_R | PTE_W);
    
    // kernel code (text)
    asm ("de:");
    for (pa_t pa = 0x80000000; pa < (pa_t)_text_end; pa += 4096)
        init_map(pa, pa, PTE_R | PTE_W | PTE_X);
    
    // kernel data
    for (pa_t pa = (pa_t)_text_end; pa < (pa_t)_ram_end; pa += 4096)
        init_map(pa, pa, PTE_R | PTE_W);

    asm ("sfence.vma zero, zero");
    satp.write((pa_t)kernelpt >> 12 | (1L << 63));
    asm ("sfence.vma zero, zero");
}

