# Machine timer interrupt handler

.globl _mti
.align 4
_mti:
    # mscratch holds the address of the scratch space for the current core
    # This space is used for saving context to avoid the use of stack
    # This handler is meticulously designed to use 4 registers only whereby
    # we just need to save the 4 registers instead of the entire context
    # The 4 registers choosen to use are t0, t1, t2, and t3, where t0 is 
    # saved into mscratch register, and the others into the scratch space

    csrrw t0, mscratch, t0 # Sawp the value in t0 and mscratch

    # t0 now holds the address of the scratch space and its value is saved into t0
    # Save context (t1, t2, t3)
    sd t1, 0(t0)
    sd t2, 8(t0)
    sd t3, 16(t0)

    # Reset mtimecmp
    # mtimecmp := mtimecmp + interval --- (1)
    # Note that the timer intrrupt is fired whenever mtime >= mtimecmp
    # so at the time of the timer interrupt, mtime = mtimecmp
    # Therefore, mtimecmp := mtime + interval can be wriiten as (1)
    ld t1, 24(t0) # t1: mtimecmp addr
    ld t2, 32(t0) # t2: interval
    ld t3, (t1)   # t3: mtimecmp

    add t3, t2, t3 # t3: mtimecmp + interval
    sw t3, (t1)    # Update mtimecmp with t3

    # Pass the interrupt to the supervisor mode
    # Set the SSIP (supervisor software interrupt pending) bit in SIP register
    csrw sip, 1 << 1

    # Restore context
    ld t1, 0(t0)
    ld t2, 8(t0)
    ld t3, 16(t0)
    csrrw t0, mscratch, t0

    mret

