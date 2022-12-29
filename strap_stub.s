# Supervisor trap bandler stub

.macro save, r
    sd x\r, (\r-1) * 8(sp)
.endm

.macro restore, r
    ld x\r, (\r-1) * 8(sp)
.endm

.global _strap_stub # stub to invoke C-level trap handler
.extern _strap_hdlr # C-level supervisor trap handler
.align 4
_strap_stub:
    addi sp, sp, -256 # Extend stack
    save 1
    save 2
    save 3
    save 4
    save 5
    save 6
    save 7
    save 8
    save 9
    save 10
    save 11
    save 12
    save 13
    save 14
    save 15
    save 16
    save 17
    save 18
    save 19
    save 20
    save 21
    save 22
    save 23
    save 24
    save 25
    save 26
    save 27
    save 28
    save 29
    save 30
    save 31

    call _strap_hdlr

    restore 1
    restore 2
    restore 3
    restore 4
    restore 5
    restore 6
    restore 7
    restore 8
    restore 9
    restore 10
    restore 11
    restore 12
    restore 13
    restore 14
    restore 15
    restore 16
    restore 17
    restore 18
    restore 19
    restore 20
    restore 21
    restore 22
    restore 23
    restore 24
    restore 25
    restore 26
    restore 27
    restore 28
    restore 29
    restore 30
    restore 31
    addi sp, sp, 256
    sret
