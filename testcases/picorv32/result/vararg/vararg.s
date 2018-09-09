	.align 4
	.text
	.globl printargs
	.align	4
printargs:
	addi x2,x2,-64
	sw  x8,60(x2)
	addi  x8,x2,48
	sw x1,24(x2)
	sw x26,28(x2)
	sw x27,32(x2)
	sw x12,16(x8)
	sw x13,20(x8)
	sw x14,24(x8)
	sw x15,28(x8)
	sw x16,32(x8)
	sw x17,36(x8)
	addi x27,x8,4+16
	lw x26,0+16(x8)
	jal x0,L.7
L.4:
	addi x12,x26,0
	jal x1,puti
L.5:
	li x30,4
	add x30,x27,x30
	addi x27,x30,0
	li x29,-4
	add x30,x30,x29
	lw x26,0(x30)
L.7:
	li x0,0
	bge x26,x0,L.4
	li x12,10
	jal x1,putc
L.1:
	lw x1,24(x2)
	lw x26,28(x2)
	lw x27,32(x2)
	lw  x8,60(x2)
	addi  x2,x2,64
	jalr x0,x1,0

	.globl main
	.align	4
main:
	addi x2,x2,-64
	sw  x8,60(x2)
	addi  x8,x2,48
	sw x1,36(x2)
	li x12,5
	li x13,2
	li x14,14
	li x15,84
	li x16,97
	li x17,15
	li x30,-1
	sw x30,24(x2)
	li x29,48
	sw x29,28(x2)
	sw x30,32(x2)
	jal x1,printargs
	li x12,84
	li x13,51
	li x14,-1
	jal x1,printargs
	li x0,0
	addi x10,x0,0
L.8:
	lw x1,36(x2)
	lw  x8,60(x2)
	addi  x2,x2,64
	jalr x0,x1,0

	.align 4
