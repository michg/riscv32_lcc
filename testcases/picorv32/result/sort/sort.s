	.align 4
	.text
	.globl main
	.align	4
main:
	addi x2,x2,-96
	sw  x8,92(x2)
	addi  x8,x2,48
	sw x1,24(x2)
	sw x25,28(x2)
	sw x26,32(x2)
	sw x27,36(x2)
	li x30,32
	sw x30,-44+48(x8)
	li x30,1
	sw x30,-40+48(x8)
	li x30,3
	sw x30,-36+48(x8)
	li x30,2
	sw x30,-32+48(x8)
	li x30,22
	sw x30,-28+48(x8)
	li x30,15
	sw x30,-24+48(x8)
	li x30,10
	sw x30,-20+48(x8)
	li x30,14
	sw x30,-16+48(x8)
	li x30,30
	sw x30,-12+48(x8)
	li x30,5
	sw x30,-8+48(x8)
	li x0,0
	addi x26,x0,0
	jal x0,L.12
L.11:
	li x27,9
	jal x0,L.15
L.14:
	li x30,2
	sll x30,x27,x30
	addi x29,x8,-44+48
	add x29,x30,x29
	lw x29,0(x29)
	addi x28,x8,-48+48
	add x30,x30,x28
	lw x30,0(x30)
	bge x29,x30,L.17
	li x30,2
	sll x30,x27,x30
	addi x29,x8,-48+48
	add x29,x30,x29
	lw x25,0(x29)
	addi x29,x8,-48+48
	add x29,x30,x29
	addi x28,x8,-44+48
	add x30,x30,x28
	lw x30,0(x30)
	sw x30,0(x29)
	li x30,2
	sll x30,x27,x30
	addi x29,x8,-44+48
	add x30,x30,x29
	sw x25,0(x30)
L.17:
	addi x27,x27,-1
L.15:
	addi x30,x26,1
	bge x27,x30,L.14
	addi x26,x26,1
L.12:
	li x30,9
	blt x26,x30,L.11
	li x0,0
	addi x26,x0,0
	jal x0,L.23
L.22:
	li x30,2
	sll x30,x26,x30
	addi x29,x8,-44+48
	add x30,x30,x29
	lw x12,0(x30)
	jal x1,puti
	li x12,10
	jal x1,putc
	addi x26,x26,1
L.23:
	li x30,10
	blt x26,x30,L.22
	li x0,0
	addi x10,x0,0
L.1:
	lw x1,24(x2)
	lw x25,28(x2)
	lw x26,32(x2)
	lw x27,36(x2)
	lw  x8,92(x2)
	addi  x2,x2,96
	jalr x0,x1,0

	.align 4
