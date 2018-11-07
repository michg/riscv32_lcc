boot:
    li x31, 0x1800
    csrw mstatus, x31
    jal x0, _mstart
    .align 6
	.globl _mstart
;/* Startup code */
_mstart:
    la x5, trap_entry
    csrw mtvec, x5
    li	x1, 0
    li	x2, 0
    li	x3, 0
    li	x4, 0
    li	x5, 0
    li	x6, 0
    li	x7, 0
    li	x8, 0
    li	x9, 0
    li	x10, 0
    li	x11, 0
    li	x12, 0
    li	x13, 0
    li	x14, 0
    li	x15, 0
    li	x16, 0
    li	x17, 0
    li	x18, 0
    li	x19, 0
    li	x20, 0
    li	x21, 0
    li	x22, 0
    li	x23, 0
    li	x24, 0
    li	x25, 0
    li	x26, 0
    li	x27, 0
    li	x28, 0
    li	x29, 0
    li	x30, 0
    li	x31, 0

write_stack_pattern:
;   /* init stack section */
    li	x10, 0x80014000  ;/* note the stack grows from top to bottom */
    li	x11, 0x80015000   ;/* section end is actually the start of the next section */
    li	x12, 0xABABABAB
    jal	x1, fill_block
;    /* set stack pointer */
    li	x2, 0x80015000
init_bss:
;    /* init bss section */
    li	x10, 0x8001C000
    li	x11, 0x80020000 ;/* section end is actually the start of the next section */
    li	x12, 0x0
    jal	x1, fill_block
    init_stack:

    jal x1, main

;/* When trap is an interrupt, this function is called */
interrupt:
	slli    x5,x5,1
	srli    x5,x5,1
	addi    x5,x5,-3
	beq		x5,x0,softwareInterrupt
	lw  	x5, 0x0(x2)
	addi	x2, x2, 4

;	/* Interupt is timer interrupt */
	jal		x0, TIMER_CMP_ISR
	mret

softwareInterrupt:
;	/* Interupt is software interrupt */
	lw  x5, 0x0(x2)
	addi  x2, x2, 4
	mret

;/* For when a trap is fired */
trap_entry:
;	/* Check for interrupt */
	addi	x2, x2, -4
	sw	    x5, 0x0(x2)
	csrr	x5, mcause
	blt	    x5,x0,interrupt
	lw	    x5, 0x0(x2)
	addi	x2, x2, 4

;	/* System call and other traps */
	addi x2, x2, -4*31
	sw x1, 1*4(x2)
	sw x2, 2*4(x2)
	sw x3, 3*4(x2)
	sw x4, 4*4(x2)
	sw x5, 5*4(x2)
	sw x6, 6*4(x2)
	sw x7, 7*4(x2)
	sw x8, 8*4(x2)
	sw x9, 9*4(x2)
	sw x10, 10*4(x2)
	sw x11, 11*4(x2)
	sw x12, 12*4(x2)
	sw x13, 13*4(x2)
	sw x14, 14*4(x2)
	sw x15, 15*4(x2)
	sw x16, 16*4(x2)
	sw x17, 17*4(x2)
	sw x18, 18*4(x2)
	sw x19, 19*4(x2)
	sw x20, 20*4(x2)
	sw x21, 21*4(x2)
	sw x22, 22*4(x2)
	sw x23, 23*4(x2)
	sw x24, 24*4(x2)
	sw x25, 25*4(x2)
	sw x26, 26*4(x2)
	sw x27, 27*4(x2)
	sw x28, 28*4(x2)
	sw x29, 29*4(x2)
	sw x30, 30*4(x2)
	sw x31, 31*4(x2)

	csrr x10, mcause
	csrr x11, mepc


	mv x12, x2

fatalException:
	jal x0, fatalException

	csrw mepc, x10

	lw x1, 1*4(x2)
	lw x2, 2*4(x2)
	lw x3, 3*4(x2)
	lw x4, 4*4(x2)
	lw x5, 5*4(x2)
	lw x6, 6*4(x2)
	lw x7, 7*4(x2)
	lw x8, 8*4(x2)
	lw x9, 9*4(x2)
	lw x10, 10*4(x2)
	lw x11, 11*4(x2)
	lw x12, 12*4(x2)
	lw x13, 13*4(x2)
	lw x14, 14*4(x2)
	lw x15, 15*4(x2)
	lw x16, 16*4(x2)
	lw x17, 17*4(x2)
	lw x18, 18*4(x2)
	lw x19, 19*4(x2)
	lw x20, 20*4(x2)
	lw x21, 21*4(x2)
	lw x22, 22*4(x2)
	lw x23, 23*4(x2)
	lw x24, 24*4(x2)
	lw x25, 25*4(x2)
	lw x26, 26*4(x2)
	lw x27, 27*4(x2)
	lw x28, 28*4(x2)
	lw x29, 29*4(x2)
	lw x30, 30*4(x2)
	lw x31, 31*4(x2)

	addi x2, x2, 4*31
	mret

;/* Fills memory blocks */
fill_block:
    sw		x12, 0(x10)
    bgeu	x10, x11, fb_end
    addi	x10, x10, 4
    jal x0, fill_block
fb_end:
    jalr x0, x1, 0
