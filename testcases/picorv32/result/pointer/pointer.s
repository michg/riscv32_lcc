	.align 4
	.text
	.globl main
	.align	4
main:
	addi x2,x2,-48
	sw  x8,44(x2)
	addi  x8,x2,32
	sw x1,24(x2)
	sw x27,28(x2)
	li x30,33
	sw x30,-8+16(x8)
	addi x27,x8,-8+16
	lw x30,0(x27)
	sw x30,-12+16(x8)
	lw x12,-8+16(x8)
	jal x1,puti
	li x12,10
	jal x1,putc
	lw x12,0(x27)
	jal x1,puti
	li x12,10
	jal x1,putc
	lw x12,-12+16(x8)
	jal x1,puti
	li x12,10
	jal x1,putc
	li x0,0
	addi x10,x0,0
L.1:
	lw x1,24(x2)
	lw x27,28(x2)
	lw  x8,44(x2)
	addi  x2,x2,48
	jalr x0,x1,0

	.align 4
	.bss
	.globl a
	.align	4
a:
	.space	4
	.align 4
