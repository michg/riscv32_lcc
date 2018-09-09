	.align 4
	.text
	.globl main
	.align	4
main:
	addi x2,x2,-48
	sw  x8,44(x2)
	addi  x8,x2,32
	sw x1,24(x2)
	la x30,p
	li x29,123
	sw x29,0(x30)
	la x30,p+4
	li x29,456
	sw x29,0(x30)
	la x30,p+4
	lw x30,0(x30)
	la x29,p
	lw x29,0(x29)
	add x30,x30,x29
	sw x30,-8+16(x8)
	lw x12,-8+16(x8)
	jal x1,puti
	li x12,10
	jal x1,putc
	li x0,0
	addi x10,x0,0
L.1:
	lw x1,24(x2)
	lw  x8,44(x2)
	addi  x2,x2,48
	jalr x0,x1,0

	.align 4
	.bss
	.globl p
	.align	4
p:
	.space	8
	.align 4
