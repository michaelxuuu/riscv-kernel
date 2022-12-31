#include "../include/vm.h"
#include "../include/types.h"
#include "../include/pm.h"
#include "../include/kpanic.h"
#include "../include/util.h"
#include "../include/hart.h"

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

typedef struct pt {
    pte_t arr[512];
} __attribute__((packed)) pt_t;

typedef struct _va {
    uint64_t vpo:12;
    uint64_t vpn1:9;
    uint64_t vpn2:9;
    uint64_t vpn3:9;
    uint64_t paddinh:25;
} __attribute__((packed)) _va_t;

static pt_t *kernelpt = 0;

static void init(void);
static void init_map(pa_t pa, va_t va, int flags);

vmmngr_t vmmngr = {init};

void init_map(pa_t pa, va_t va, int flags) {

    // starting from level-1 page table
    // iteratively find level-3 table
    // and the entry that corresponds to
    // the physical address
    // and write pa to the ppn field.
    // will allocate page tables
    // along the way if necessary

    // break vpn into three 9-bit parts
    // which will be used to index page tables
    _va_t* _va = (_va_t*)&va;
    uint64_t vpns[3] = {_va->vpn3, _va->vpn2, _va->vpn1};

    pt_t* pt = kernelpt;
    pte_t* pte;

    for (int i = 0; i < 3; i++) {
        pte = &pt->arr[vpns[i]];
        // break if reached level 3
        // pte ends up corresponding to the intended physical address
        if (i != 2) {
            if (!pte->valid) { // allocate the page table if unallocated
                pa_t tmp = (pa_t)pmmngr.alloc();
                memset((void *)tmp, 0, 4096);
                pte->valid = 1;
                pte->ppn = tmp >> 12;
            }
            pt = (pt_t*)(uint64_t)(pte->ppn << 12); // get the next level page table
        }
    }

    pte->valid = 1;
    pte->ppn = pa >> 12;
    pte->readable = !!(flags & PTE_R);
    pte->writable = !!(flags & PTE_W);
    pte->executable = !!(flags & PTE_X);
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
    // allocate kernel page table
    kernelpt = (pt_t*)pmmngr.alloc();
    memset((void*)kernelpt, 0, 4096);

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
    for (pa_t pa = 0x80000000; pa < (pa_t)_text_end; pa += 4096)
        init_map(pa, pa, PTE_R | PTE_W | PTE_X);
    
    // kernel data
    for (pa_t pa = (pa_t)_text_end; pa < (pa_t)_ram_end; pa += 4096)
        init_map(pa, pa, PTE_R | PTE_W);

    // Install kernel page table

    // Flush TLB to make sure it starts off clean
    // Use memory fence to make sure memory operations that needs phsycial addresses
    // happend strictly before paging's turned on, and vice versa
    asm ("sfence.vma zero, zero");
    satp.write((pa_t)kernelpt >> 12 | (1L << 63));
    asm ("sfence.vma zero, zero");
}

