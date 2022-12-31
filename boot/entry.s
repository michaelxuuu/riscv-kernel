.global _entry
.extern stack0
.extern _start


.text
_entry:
    csrr t0, mhartid
    addi t0, t0, 1
    slli t0, t0, 12
    la t1, stack0
    add sp, t0, t1
    j _start
