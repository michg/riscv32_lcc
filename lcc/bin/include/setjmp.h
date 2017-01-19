#ifndef SETJMP_H
#define SETJMP_H

typedef struct
{
	int __reg_16;
	int __reg_17;
	int __reg_18;
	int __reg_19;
	int __reg_20;
	int __reg_21;
	int __reg_22;
	int __reg_23;
	int __reg_28;
	int __reg_29;
	int __reg_30;
	int __reg_31;
	int __lo, __hi;
} __jmp_buf[1];

typedef struct __jmp_buf_tag
{
	__jmp_buf __jmpbuf;
} jmp_buf_mips[1];


typedef jmp_buf_mips jmp_buf;

#if __cplusplus
extern "C"
{
#endif

	int  setjmp(jmp_buf env);
	void longjmp(jmp_buf env, int val);

#if __cplusplus
}
#endif

#endif
