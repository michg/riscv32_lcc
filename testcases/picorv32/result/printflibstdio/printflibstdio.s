	.align 4
	.text
	.align 4
	.data
	.globl x
	.align	4
x:
	.word	0x40490e56
	.globl y
	.align	4
y:
	.word	0x401d3405
	.globl main
	.align 4
	.text
	.align	4
main:
	addi x2,x2,-48
	sw  x8,44(x2)
	addi  x8,x2,32
	sw x1,24(x2)
	la x30,x
	lw x12,0(x30)
	la x30,y
	lw x13,0(x30)
	jal x1,float32_add
	addi x30,x10,0
	la x29,z
	sw x30,0(x29)
	la x12,L.2
	lw x13,0(x29)
	jal x1,printf
	la x30,x
	lw x12,0(x30)
	la x30,y
	lw x13,0(x30)
	jal x1,float32_sub
	addi x30,x10,0
	la x29,z
	sw x30,0(x29)
	la x12,L.3
	lw x13,0(x29)
	jal x1,printf
	la x30,x
	lw x12,0(x30)
	la x30,y
	lw x13,0(x30)
	jal x1,float32_mul
	addi x30,x10,0
	la x29,z
	sw x30,0(x29)
	la x12,L.4
	lw x13,0(x29)
	jal x1,printf
	la x30,x
	lw x12,0(x30)
	la x30,y
	lw x13,0(x30)
	jal x1,float32_div
	addi x30,x10,0
	la x29,z
	sw x30,0(x29)
	la x12,L.5
	lw x13,0(x29)
	jal x1,printf
	li x0,0
	addi x10,x0,0
L.1:
	lw x1,24(x2)
	lw  x8,44(x2)
	addi  x2,x2,48
	jalr x0,x1,0

	.align 4
	.bss
	.globl z
	.align	4
z:
	.space	4
	.align 4
	.data
	.align	1
L.5:
	.byte	0x78
	.byte	0x20
	.byte	0x2f
	.byte	0x20
	.byte	0x79
	.byte	0x20
	.byte	0x3d
	.byte	0x20
	.byte	0x25
	.byte	0x32
	.byte	0x2e
	.byte	0x34
	.byte	0x66
	.byte	0xa
	.byte	0x0
	.align	1
L.4:
	.byte	0x78
	.byte	0x20
	.byte	0x2a
	.byte	0x20
	.byte	0x79
	.byte	0x20
	.byte	0x3d
	.byte	0x20
	.byte	0x25
	.byte	0x32
	.byte	0x2e
	.byte	0x34
	.byte	0x66
	.byte	0xa
	.byte	0x0
	.align	1
L.3:
	.byte	0x78
	.byte	0x20
	.byte	0x2d
	.byte	0x20
	.byte	0x79
	.byte	0x20
	.byte	0x3d
	.byte	0x20
	.byte	0x25
	.byte	0x32
	.byte	0x2e
	.byte	0x34
	.byte	0x66
	.byte	0xa
	.byte	0x0
	.align	1
L.2:
	.byte	0x78
	.byte	0x20
	.byte	0x2b
	.byte	0x20
	.byte	0x79
	.byte	0x20
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
