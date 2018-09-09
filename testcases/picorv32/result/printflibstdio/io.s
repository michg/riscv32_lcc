	.align 4
	.text
	.globl putc
	.align	4
putc:
	addi x2,x2,-16
	sw  x8,12(x2)
	addi  x8,x2,0
	li x30,0x20000000
	sw x30,-8+16(x8)
	lw x30,-8+16(x8)
	slli x29,x12,8*(4-1)
	srai x29,x29,8*(4-1)
	sw x29,0(x30)
L.1:
	lw  x8,12(x2)
	addi  x2,x2,16
	jalr x0,x1,0

	.globl puti
	.align	4
puti:
	addi x2,x2,-64
	sw  x8,60(x2)
	addi  x8,x2,48
	sw x1,24(x2)
	sw x25,28(x2)
	sw x26,32(x2)
	sw x27,36(x2)
	mv x27,x12
	li x0,0
	addi x26,x0,0
	li x0,0
	addi x25,x0,0
	li x26,28
L.3:
	sra x30,x27,x26
	andi x25,x30,15
	li x30,10
	bge x25,x30,L.7
	addi x30,x25,48
	slli x12,x30,8*(4-1)
	srai x12,x12,8*(4-1)
	jal x1,putc
	jal x0,L.8
L.7:
	addi x30,x25,55
	slli x12,x30,8*(4-1)
	srai x12,x12,8*(4-1)
	jal x1,putc
L.8:
L.4:
	addi x26,x26,-4
	li x0,0
	bge x26,x0,L.3
	li x12,10
	jal x1,putc
L.2:
	lw x1,24(x2)
	lw x25,28(x2)
	lw x26,32(x2)
	lw x27,36(x2)
	lw  x8,60(x2)
	addi  x2,x2,64
	jalr x0,x1,0

	.align 4
