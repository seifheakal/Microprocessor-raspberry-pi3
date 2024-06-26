.section ".text.boot"

.global _start

_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f

    //cmp     x1, #1
    //bne     1f

second_startup:
	wfe
	mrs	    x19, mpidr_el1
	and	    x19, x19, #0xFF		// Check processor id
	ldr 	x0, =__stack_start
	mul 	x2, x0, x19		    // calculate SP for each core
	mov 	sp, x2			    // setup sp for each core
    mov	    x1, #30000
	mul	    x0, x1, x19		    // calc delay for each core
	bl	    sub_delay
	mov	    x0, x19
	bl	    kernel_secondary

1:  wfe
	b	1b

2:  // cpu id == 0

    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, =_start

    // set up EL1
    mrs     x0, CurrentEL
    and     x0, x0, #12 // clear reserved bits

    // running at EL3?
    cmp     x0, #12
    bne     5f
    // should never be executed, just for completeness
    mov     x2, #0x5b1
    msr     scr_el3, x2
    mov     x2, #0x3c9
    msr     spsr_el3, x2
    adr     x2, 5f
    msr     elr_el3, x2
    eret

    // running at EL2?
5:  cmp     x0, #4
    beq     5f
    msr     sp_el1, x1
    // enable CNTP for EL1
    mrs     x0, cnthctl_el2
    orr     x0, x0, #3
    msr     cnthctl_el2, x0
    msr     cntvoff_el2, xzr
    // disable coprocessor traps
    mov     x0, #0x33FF
    msr     cptr_el2, x0
    msr     hstr_el2, xzr
    mov     x0, #(3 << 20)
    msr     cpacr_el1, x0
    // enable AArch64 in EL1
    mov     x0, #(1 << 31)      // AArch64
    orr     x0, x0, #(1 << 1)   // SWIO hardwired on Pi3
    msr     hcr_el2, x0
    mrs     x0, hcr_el2
    
    // Setup SCTLR access
    mov     x2, #0x0800
    movk    x2, #0x30d0, lsl #16
    msr     sctlr_el1, x2
    // set up exception handlers
    //ldr     x2, =_vectors
    //msr     vbar_el1, x2
    // change execution level to EL1
    mov     x2, #0x3c4
    msr     spsr_el2, x2
    adr     x2, 5f
    msr     elr_el2, x2
    // clear EL1 system registers
    msr     elr_el1, xzr
    msr     far_el1, xzr
    eret

5:  mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b

    //mov 	x0, #1			    // core id
	//adr	    x1, second_startup	// where to start
    //bl	    wakeup_core

    // jump to C code, should not return
4:  bl      main
    // for failsafe, halt this core too
    b       1b

.globl wakeup_core
wakeup_core:
        mov x2, 0xd8
        str x1, [x2, x0, LSL #3]
        sev
        ret

sub_delay:
	subs x0, x0, #1
	bne sub_delay
	ret

    // save registers before we call any C code
dbg_saveregs:
    str     x0, [sp, #-16]!     // push x0
    ldr     x0, =dbg_regs+8
    str     x1, [x0], #8        // dbg_regs[1]=x1
    ldr     x1, [sp, #16]       // pop x1
    str     x1, [x0, #-16]!     // dbg_regs[0]=x1 (x0)
    add     x0, x0, #16
    str     x2, [x0], #8        // dbg_regs[2]=x2
    str     x3, [x0], #8        // ...etc.
    str     x4, [x0], #8
    str     x5, [x0], #8
    str     x6, [x0], #8
    str     x7, [x0], #8
    str     x8, [x0], #8
    str     x9, [x0], #8
    str     x10, [x0], #8
    str     x11, [x0], #8
    str     x12, [x0], #8
    str     x13, [x0], #8
    str     x14, [x0], #8
    str     x15, [x0], #8
    str     x16, [x0], #8
    str     x17, [x0], #8
    str     x18, [x0], #8
    str     x19, [x0], #8
    str     x20, [x0], #8
    str     x21, [x0], #8
    str     x22, [x0], #8
    str     x23, [x0], #8
    str     x24, [x0], #8
    str     x25, [x0], #8
    str     x26, [x0], #8
    str     x27, [x0], #8
    str     x28, [x0], #8
    str     x29, [x0], #8
    ldr     x1, [sp, #16]       // pop x30
    str     x1, [x0], #8
    // also read and store some system registers
    mrs     x1, elr_el1
    str     x1, [x0], #8
    mrs     x1, spsr_el1
    str     x1, [x0], #8
    mrs     x1, esr_el1
    str     x1, [x0], #8
    mrs     x1, far_el1
    str     x1, [x0], #8
    mrs     x1, sctlr_el1
    str     x1, [x0], #8
    mrs     x1, tcr_el1
    str     x1, [x0], #8
    ret

    // important, code has to be properly aligned
    .align 11
_vectors:
    // synchronous
    .align  7
    str     x30, [sp, #-16]!     // push x30
    bl      dbg_saveregs
    mov     x0, #0
    bl      dbg_decodeexc
    bl      dbg_main
    eret

    // IRQ
    .align  7
    str     x30, [sp, #-16]!     // push x30
    bl      dbg_saveregs
    mov     x0, #1
    bl      dbg_decodeexc
    bl      dbg_main
    eret

    // FIQ
    .align  7
    str     x30, [sp, #-16]!     // push x30
    bl      dbg_saveregs
    mov     x0, #2
    bl      dbg_decodeexc
    bl      dbg_main
    eret

    // SError
    .align  7
    str     x30, [sp, #-16]!     // push x30
    bl      dbg_saveregs
    mov     x0, #3
    bl      dbg_decodeexc
    bl      dbg_main
    eret
