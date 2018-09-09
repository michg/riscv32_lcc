	.align 4
	.text
	.globl main
	.align	4
main:
	addi x2,x2,-48
	sw  x8,44(x2)
	addi  x8,x2,32
	sw x1,24(x2)
	la x30,L.3
	lw x12,0(x30)
	jal x1,sqrt
	addi x30,x10,0
	la x12,L.2
	addi x13,x30,0
	jal x1,printf
	li x0,0
	addi x10,x0,0
L.1:
	lw x1,24(x2)
	lw  x8,44(x2)
	addi  x2,x2,48
	jalr x0,x1,0

	.align 4
	.data
	.align	4
L.3:
	.word	0x40400000
	.align	1
L.2:
	.byte	0x73
	.byte	0x71
	.byte	0x72
	.byte	0x74
	.byte	0x28
	.byte	0x33
	.byte	0x29
	.byte	0x3d
	.byte	0x20
	.byte	0x25
	.byte	0x32
	.byte	0x2e
	.byte	0x34
	.byte	0x66
	.byte	0xa
	.byte	0x0
	.align 4
