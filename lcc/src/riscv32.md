%{

/*
 * riscv32.md -- RISCV-32bit back-end specification
 *
 * register usage:
 *   x0   always zero
 *   x1   proc/func return address (caller-save)
 *   x2   stack pointer (callee-save)
 *   x3   global pointer
 *   x4   thread pointer
 *   x5   temporary register (caller-save)
 *   x6   temporary register (caller-save)
 *   x7   temporary register (caller-save)
 *   x8   frame pointer (callee-save)
 *   x9   register variable  (callee-save)
 *   x10  return value
 *   x11  return value
 *   x12  functions argument
 *   x13  functions argument
 *   x14  functions argument
 *   x15  functions argument
 *   x16  functions argument
 *   x17  functions argument
 *   x18  register variable  (callee-save)
 *   x19  register variable  (callee-save)
 *   x20  register variable  (callee-save)
 *   x21  register variable  (callee-save)
 *   x22  register variable  (callee-save)
 *   x23  register variable  (callee-save)
 *   x24  register variable  (callee-save)
 *   x25  register variable  (callee-save)
 *   x26  register variable  (callee-save)
 *   x27  register variable  (callee-save)
 *   x28   temporary register (caller-save)
 *   x29   temporary register (caller-save)
 *   x30   temporary register (caller-save)
 *   x31   temporary register (caller-save)
 * caller-save registers are not preserved across procedure calls
 * callee-save registers are preserved across procedure calls
 *
 * tree grammar terminals produced by:
 *   ops c=1 s=2 i=4 l=4 h=4 f=4 d=4 x=4 p=4
 */

#include "c.h"

#define NODEPTR_TYPE	Node
#define OP_LABEL(p)	((p)->op)
#define LEFT_CHILD(p)	((p)->kids[0])
#define RIGHT_CHILD(p)	((p)->kids[1])
#define STATE_LABEL(p)	((p)->x.state)

static void address(Symbol, Symbol, long);
static void defaddress(Symbol);
static void defconst(int, int, Value);
static void defstring(int, char *);
static void defsymbol(Symbol);
static void export(Symbol);
static void function(Symbol, Symbol [], Symbol [], int);
static void global(Symbol);
static void import(Symbol);
static void local(Symbol);
static void progbeg(int, char * []);
static void progend(void);
static void segment(int);
static void space(int);
static Symbol rmap(int);
static void blkfetch(int, int, int, int);
static void blkstore(int, int, int, int);
static void blkloop(int, int, int, int, int, int []);
static void emit2(Node);
static void doarg(Node);
static void target(Node);
static void clobber(Node);
static int mulops_calls(int op);

#define INTTMP	0x700000E0
#define INTVAR	0x0FFC0000
#define INTRET	0x00000400
#define FLTTMP	0x000F0FF0
#define FLTVAR	0xFFF00000
#define FLTRET	0x00000003

static Symbol ireg[32];
static Symbol iregw;
static Symbol freg2[32];
static Symbol freg2w;
static Symbol blkreg;
static int tmpregs[] = { 11, 6, 7 };

%}

%start stmt

%term CNSTF4=4113
%term CNSTI1=1045 CNSTI2=2069 CNSTI4=4117
%term CNSTP4=4119
%term CNSTU1=1046 CNSTU2=2070 CNSTU4=4118

%term ARGB=41
%term ARGF4=4129
%term ARGI4=4133
%term ARGP4=4135
%term ARGU4=4134

%term ASGNB=57
%term ASGNF4=4145
%term ASGNI1=1077 ASGNI2=2101 ASGNI4=4149
%term ASGNP4=4151
%term ASGNU1=1078 ASGNU2=2102 ASGNU4=4150

%term INDIRB=73
%term INDIRF4=4161
%term INDIRI1=1093 INDIRI2=2117 INDIRI4=4165
%term INDIRP4=4167
%term INDIRU1=1094 INDIRU2=2118 INDIRU4=4166

%term CVFF4=4209
%term CVFI4=4213

%term CVIF4=4225
%term CVII1=1157 CVII2=2181 CVII4=4229
%term CVIU1=1158 CVIU2=2182 CVIU4=4230

%term CVPU4=4246

%term CVUI1=1205 CVUI2=2229 CVUI4=4277
%term CVUP4=4279
%term CVUU1=1206 CVUU2=2230 CVUU4=4278

%term NEGF4=4289
%term NEGI4=4293

%term CALLB=217
%term CALLF4=4305
%term CALLI4=4309
%term CALLP4=4311
%term CALLU4=4310
%term CALLV=216

%term RETF4=4337
%term RETI4=4341
%term RETP4=4343
%term RETU4=4342
%term RETV=248

%term ADDRGP4=4359

%term ADDRFP4=4375

%term ADDRLP4=4391

%term ADDF4=4401
%term ADDI4=4405
%term ADDP4=4407
%term ADDU4=4406

%term SUBF4=4417
%term SUBI4=4421
%term SUBP4=4423
%term SUBU4=4422

%term LSHI4=4437
%term LSHU4=4438

%term MODI4=4453
%term MODU4=4454

%term RSHI4=4469
%term RSHU4=4470

%term BANDI4=4485
%term BANDU4=4486

%term BCOMI4=4501
%term BCOMU4=4502

%term BORI4=4517
%term BORU4=4518

%term BXORI4=4533
%term BXORU4=4534

%term DIVF4=4545
%term DIVI4=4549
%term DIVU4=4550

%term MULF4=4561
%term MULI4=4565
%term MULU4=4566

%term EQF4=4577
%term EQI4=4581
%term EQU4=4582

%term GEF4=4593
%term GEI4=4597
%term GEU4=4598

%term GTF4=4609
%term GTI4=4613
%term GTU4=4614

%term LEF4=4625
%term LEI4=4629
%term LEU4=4630

%term LTF4=4641
%term LTI4=4645
%term LTU4=4646

%term NEF4=4657
%term NEI4=4661
%term NEU4=4662

%term JUMPV=584

%term LABELV=600

%term LOADB=233
%term LOADF4=4321 LOADF8=8417 LOADF16=16609
%term LOADI1=1253 LOADI2=2277 LOADI4=4325 LOADI8=8421
%term LOADP4=4327 LOADP8=8423
%term LOADU1=1254 LOADU2=2278 LOADU4=4326 LOADU8=8422

%term VREGP=711


%%


reg:	INDIRI1(VREGP)		"# read register\n"
reg:	INDIRI2(VREGP)		"# read register\n"
reg:	INDIRI4(VREGP)		"# read register\n"
reg:	INDIRP4(VREGP)		"# read register\n"
reg:	INDIRU1(VREGP)		"# read register\n"
reg:	INDIRU2(VREGP)		"# read register\n"
reg:	INDIRU4(VREGP)		"# read register\n"

stmt:	ASGNI1(VREGP,reg)	"# write register\n"
stmt:	ASGNI2(VREGP,reg)	"# write register\n"
stmt:	ASGNI4(VREGP,reg)	"# write register\n"
stmt:	ASGNP4(VREGP,reg)	"# write register\n"
stmt:	ASGNU1(VREGP,reg)	"# write register\n"
stmt:	ASGNU2(VREGP,reg)	"# write register\n"
stmt:	ASGNU4(VREGP,reg)	"# write register\n"

con:	CNSTI1			"%a"
con:	CNSTI2			"%a"
con:	CNSTI4			"%a"
con:	CNSTP4			"%a"
con:	CNSTU1			"%a"
con:	CNSTU2			"%a"
con:	CNSTU4			"%a"

stmt:	reg			""

scon:	ADDRGP4			"%a"

saddr:	ADDI4(scon,con)		"%0+%1"
saddr:	ADDP4(scon,con)		"%0+%1"
saddr:	ADDU4(scon,con)		"%0+%1"
saddr:  scon "%0"

reg:	saddr			"\tla x%c,%0\n"	1
reg:    con                     "\tli x%c,%0\n" 1
stmt:	ASGNI1(saddr,reg)	"\tsb x%1,%0,x10\n"	1
stmt:	ASGNI2(saddr,reg)	"\tsh x%1,%0,x10\n"	1
stmt:	ASGNI4(saddr,reg)	"\tsw x%1,%0,x10\n"	1
stmt:	ASGNP4(saddr,reg)	"\tsw x%1,%0,x10\n"	1
stmt:	ASGNU1(saddr,reg)	"\tsb x%1,%0,x10\n"	1
stmt:	ASGNU2(saddr,reg)	"\tsh x%1,%0,x10\n"	1
stmt:	ASGNU4(saddr,reg)	"\tsw x%1,%0,x10\n"	1
reg:	INDIRI1(saddr)		"\tlb x%c,%0\n"	1
reg:	INDIRI2(saddr)		"\tlh x%c,%0\n"	1
reg:	INDIRI4(saddr)		"\tlw x%c,%0\n"	1
reg:	INDIRP4(saddr)		"\tlw x%c,%0\n"	1
reg:	INDIRU1(saddr)		"\tlbu x%c,%0\n"	1
reg:	INDIRU2(saddr)		"\tlhu x%c,%0\n"	1
reg:	INDIRU4(saddr)		"\tlw x%c,%0\n"	1
reg:	CVII4(INDIRI1(saddr))	"\tlb x%c,%0\n"	1
reg:	CVII4(INDIRI2(saddr))	"\tlh x%c,%0\n"	1
reg:	CVUU4(INDIRU1(saddr))	"\tlbu x%c,%0\n"	1
reg:	CVUU4(INDIRU2(saddr))	"\tlhu x%c,%0\n"	1
reg:	CVUI4(INDIRU1(saddr))	"\tlbu x%c,%0\n"	1
reg:	CVUI4(INDIRU2(saddr))	"\tlhu x%c,%0\n"	1

acon12: CNSTP4 "%a" range(a,0,4095)
acon12: CNSTP4 "\tauipc x%c,(%a>>12) "   1

addr:	acon12			"%0(x0)"
addr:	reg			"0(x%0)"
addr:	ADDRFP4			"%a+%F(x2)"
addr:	ADDRLP4			"%a+%F(x2)"

reg:    ADDRFP4			"\taddi x%c,x2,%a+%F\n" 1
reg:    ADDRLP4			"\taddi x%c,x2,%a+%F\n" 1

reg:	CNSTI1			"# reg\n"		range(a, 0, 0)
reg:	CNSTI2			"# reg\n"		range(a, 0, 0)
reg:	CNSTI4			"# reg\n"		range(a, 0, 0)
reg:	CNSTP4			"# reg\n"		range(a, 0, 0)
reg:	CNSTU1			"# reg\n"		range(a, 0, 0)
reg:	CNSTU2			"# reg\n"		range(a, 0, 0)
reg:	CNSTU4			"# reg\n"		range(a, 0, 0)

stmt:	ASGNI1(addr,reg)	"\tsb x%1,%0\n"	1
stmt:	ASGNI2(addr,reg)	"\tsh x%1,%0\n"	1
stmt:	ASGNI4(addr,reg)	"\tsw x%1,%0\n"	1
stmt:	ASGNP4(addr,reg)	"\tsw x%1,%0\n"	1
stmt:	ASGNU1(addr,reg)	"\tsb x%1,%0\n"	1
stmt:	ASGNU2(addr,reg)	"\tsh x%1,%0\n"	1
stmt:	ASGNU4(addr,reg)	"\tsw x%1,%0\n"	1
stmt:	ASGNF4(addr,reg)	"\tsw x%1,%0\n"	1

reg:	INDIRI1(addr)		"\tlb x%c,%0\n"	1
reg:	INDIRI2(addr)		"\tlh x%c,%0\n"	1
reg:	INDIRI4(addr)		"\tlw x%c,%0\n"	1
reg:	INDIRP4(addr)		"\tlw x%c,%0\n"	1
reg:	INDIRU1(addr)		"\tlbu x%c,%0\n"	1
reg:	INDIRU2(addr)		"\tlhu x%c,%0\n"	1
reg:	INDIRU4(addr)		"\tlw x%c,%0\n"	1
reg:	INDIRF4(addr)		"\tlw x%c,%0\n"	1


reg:	CVII4(INDIRI1(addr))	"\tlb x%c,%0\n"	1
reg:	CVII4(INDIRI2(addr))	"\tlh x%c,%0\n"	1
reg:	CVUU4(INDIRU1(addr))	"\tlbu x%c,%0\n"	1
reg:	CVUU4(INDIRU2(addr))	"\tlhu x%c,%0\n"	1
reg:	CVUI4(INDIRU1(addr))	"\tlbu x%c,%0\n"	1
reg:	CVUI4(INDIRU2(addr))	"\tlhu x%c,%0\n"	1

reg: DIVI4(reg,reg)  "\tdiv x%c,x%0,x%1\n"   1
reg: DIVU4(reg,reg)  "\tdivu x%c,x%0,x%1\n"  1
reg: MODI4(reg,reg)  "\trem x%c,x%0,x%1\n"   1
reg: MODU4(reg,reg)  "\tremu x%c,x%0,x%1\n"  1
reg: MULI4(reg,reg)  "\tmul x%c,x%0,x%1\n"   1
reg: MULU4(reg,reg)  "\tmul x%c,x%0,x%1\n"   1

con12: CNSTI4 "%a" range(a,-2048,2047)
con12: CNSTU4 "%a" range(a,0,4095)

reg: ADDI4(reg,reg)   "\tadd x%c,x%0,x%1\n"  1
reg: ADDI4(reg,con12) "\taddi x%c,x%0,%1\n"  1
reg: ADDP4(reg,reg)   "\tadd x%c,x%0,x%1\n"  1
reg: ADDP4(reg,con12) "\taddi x%c,x%0,%1\n"  1
reg: ADDU4(reg,reg)   "\tadd x%c,x%0,x%1\n"  1
reg: ADDU4(reg,con12) "\taddi x%c,x%0,%1\n"  1

reg: BANDI4(reg,reg)  "\tand x%c,x%0,x%1\n"   1
reg: BANDI4(reg,con12)  "\tandi x%c,x%0,%1\n"   1
reg: BORI4(reg,reg)   "\tor x%c,x%0,x%1\n"    1
reg: BORI4(reg,con12)   "\tori x%c,x%0,%1\n"    1
reg: BXORI4(reg,reg)  "\txor x%c,x%0,x%1\n"   1
reg: BXORI4(reg,con12)  "\txori x%c,x%0,%1\n"   1

reg: BANDU4(reg,reg)  "\tand x%c,x%0,x%1\n"   1
reg: BANDU4(reg,con12)  "\tandi x%c,x%0,%1\n"   1
reg: BORU4(reg,reg)   "\tor x%c,x%0,x%1\n"    1
reg: BORU4(reg,con12)   "\tori x%c,x%0,%1\n"    1
reg: BXORU4(reg,reg)  "\txor x%c,x%0,x%1\n"   1
reg: BXORU4(reg,con12)  "\txori x%c,x%0,%1\n"   1
reg: SUBI4(reg,reg)   "\tsub x%c,x%0,x%1\n"  1
reg: SUBI4(reg,con12)   "\taddi x%c,x%0,-%1\n"  1
reg: SUBP4(reg,reg)   "\tsub x%c,x%0,x%1\n"  1
reg: SUBP4(reg,con12)   "\taddi x%c,x%0,-%1\n"  1
reg: SUBU4(reg,reg)   "\tsub x%c,x%0,x%1\n"  1
reg: SUBU4(reg,con12)   "\taddi x%c,x%0,-%1\n"  1
reg: NEGI4(reg)   "\tsub x%c,x0,x%0\n" 1
reg: BCOMI4(reg)   "\txori x%c,x%0,-1\n" 1
reg: BCOMU4(reg)   "\txori x%c,x%0,-1\n" 1

con5:	CNSTI4			"%a"			range(a, 0, 31)

reg:	LSHI4(reg,con5)		"\tslli x%c,x%0,%1\n"	1
reg:	LSHI4(reg,reg)		"\tsll x%c,x%0,x%1\n"	1
reg:	LSHU4(reg,con5)		"\tslli x%c,x%0,%1\n"	1
reg:	LSHU4(reg,reg)		"\tsll x%c,x%0,x%1\n"	1
reg:	RSHI4(reg,con5)		"\tsrai x%c,x%0,%1\n"	1
reg:	RSHI4(reg,reg)		"\tsra x%c,x%0,x%1\n"	1
reg:	RSHU4(reg,con5)		"\tsrli x%c,x%0,%1\n"	1
reg:	RSHU4(reg,reg)		"\tsrl x%c,x%0,x%1\n"	1

reg: LOADI1(reg)  "\taddi x%c,x%0,0\n"  move(a)
reg: LOADU1(reg)  "\taddi x%c,x%0,0\n"  move(a)
reg: LOADI2(reg)  "\taddi x%c,x%0,0\n"  move(a)
reg: LOADU2(reg)  "\taddi x%c,x%0,0\n"  move(a)
reg: LOADI4(reg)  "\taddi x%c,x%0,0\n"  move(a)
reg: LOADP4(reg)  "\taddi x%c,x%0,0\n"  move(a)
reg: LOADU4(reg)  "\taddi x%c,x%0,0\n"  move(a)



reg:	CVII4(reg)  "\tslli x%c,x%0,8*(4-%a)\n\tsrai x%c,x%c,8*(4-%a)\n"  2
reg:	CVUI4(reg)  "\tslli x%c,x%0,8*(4-%a)\n\tsrli x%c,x%c,8*(4-%a)\n"  2
reg:	CVUU4(reg)  "\tslli x%c,x%0,8*(4-%a)\n\tsrli x%c,x%c,8*(4-%a)\n"  2

stmt: LABELV  "%a:\n"
stmt: JUMPV(scon)  "\tjal x0,%0\n"   1
stmt: JUMPV(reg)   "\tjalr x0,x%0,0\n"  1

stmt: EQI4(reg,reg)  "\tbeq x%0,x%1,%a\n"   1
stmt: EQU4(reg,reg)  "\tbeq x%0,x%1,%a\n"   1
stmt: GEI4(reg,reg)  "\tbge x%0,x%1,%a\n"   1
stmt: GEU4(reg,reg)  "\tbgeu x%0,x%1,%a\n"  1
stmt: GTI4(reg,reg)  "\tbgt x%0,x%1,%a\n"   1
stmt: GTU4(reg,reg)  "\tbgtu x%0,x%1,%a\n"  1
stmt: LEI4(reg,reg)  "\tble x%0,x%1,%a\n"   1
stmt: LEU4(reg,reg)  "\tbleu x%0,x%1,%a\n"  1
stmt: LTI4(reg,reg)  "\tblt x%0,x%1,%a\n"   1
stmt: LTU4(reg,reg)  "\tbltu x%0,x%1,%a\n"  1
stmt: NEI4(reg,reg)  "\tbne x%0,x%1,%a\n"   1
stmt: NEU4(reg,reg)  "\tbne x%0,x%1,%a\n"   1

lab:   ADDRGP4     "%a"
reg:  CALLF4(lab)  "\tjal x1,%0\n"  1

reg:  CALLI4(lab)  "\tjal x1,%0\n"  1
reg:  CALLP4(lab)  "\tjal x1,%0\n"  1
reg:  CALLU4(lab)  "\tjal x1,%0\n"  1
stmt: CALLV(lab)  "\tjal x1,%0\n"  1

reg:  CALLF4(reg)  "\tjalr x1,x%0,0\n"  1
reg:  CALLI4(reg)  "\tjalr x1,x%0,0\n"  1
reg:  CALLP4(reg)  "\tjalr x1,x%0,0\n"  1
reg:  CALLU4(reg)  "\tjalr x1,x%0,0\n"  1
stmt: CALLV(reg)  "\tjalr x1,x%0,0\n"  1


stmt:	RETI4(reg)		"# ret\n"		1
stmt:	RETP4(reg)		"# ret\n"		1
stmt:	RETU4(reg)		"# ret\n"		1
stmt:	RETV(reg)		"# ret\n"		1
stmt:	RETF4(reg)		"# ret\n"		1

stmt:	ARGI4(reg)		"# arg\n"		1
stmt:	ARGP4(reg)		"# arg\n"		1
stmt:	ARGU4(reg)		"# arg\n"		1
stmt:	ARGF4(reg)		"# arg\n"		1
stmt:	ARGB(INDIRB(reg))	"# argb %0\n"		1
stmt:	ASGNB(reg,INDIRB(reg))	"# asgnb %0 %1\n"	1

reg:	INDIRF4(VREGP)		"# read register\n"
stmt:	ASGNF4(VREGP,reg)	"# write register\n"
reg:	ADDF4(reg,reg)		"\tjal x1,float32_add\n"	100
reg:	SUBF4(reg,reg)		"\tjal x1,float32_sub\n"	100
reg:	MULF4(reg,reg)		"\tjal x1,float32_mul\n"	100
reg:	DIVF4(reg,reg)		"\tjal x1,float32_div\n"	100
reg:	LOADF4(reg)		"\taddi x%c,x%0,0\n"	move(a)
reg:	NEGF4(reg)		"\tjal x1,float32_neg\n"	100
reg:	CVFF4(reg)		"\t"	100
reg:	CVIF4(reg)		"\tjal x1,int32_to_float32\n"	100
reg:	CVFI4(reg)		"\tjal x1,float32_to_int32\n" 	100
stmt:	EQF4(reg,reg)		"\tbeq x%0,x%1,%a\n"   1
stmt:	LEF4(reg,reg)           "\tand x10,x%0,x%1\n\tsrli x10,x10,31\n\tbne x10,x0,.+12\n\tble x%0,x%1,%a\n\tbeq x0,x0,.+8\n\tble x%1,x%0,%a\n"   4
stmt:	LTF4(reg,reg)           "\tand x10,x%0,x%1\n\tsrli x10,x10,31\n\tbne x10,x0,.+12\n\tblt x%0,x%1,%a\n\tbeq x0,x0,.+8\n\tblt x%1,x%0,%a\n"   4
stmt:	GEF4(reg,reg)           "\tand x10,x%0,x%1\n\tsrli x10,x10,31\n\tbne x10,x0,.+12\n\tbge x%0,x%1,%a\n\tbeq x0,x0,.+8\n\tbge x%1,x%0,%a\n"   4
stmt:	GTF4(reg,reg)           "\tand x10,x%0,x%1\n\tsrli x10,x10,31\n\tbne x10,x0,.+12\n\tbgt x%0,x%1,%a\n\tbeq x0,x0,.+8\n\tbgt x%1,x%0,%a\n"   4
stmt:	NEF4(reg,reg)		"\tbne x%0,x%1,%a\n"   1



%%


static void address(Symbol s1, Symbol s2, long n) {
  if (s2->scope == GLOBAL ||
      s2->sclass == STATIC ||
      s2->sclass == EXTERN) {
    s1->x.name = stringf("%s%s%D", s2->x.name, n >= 0 ? "+" : "", n);
  } else {
    assert(n >= INT_MIN && n <= INT_MAX);
    s1->x.offset = s2->x.offset + n;
    s1->x.name = stringd(s1->x.offset);
  }
}


static void defaddress(Symbol s) {
  print("\t.word\t%s\n", s->x.name);
}


static void defconst(int suffix, int size, Value v) {
  float f;
  double d;
  unsigned *p;

  if (suffix == F && size == 4) {
    f = v.d;
    print("\t.word\t0x%x\n", * (unsigned *) &f);
  } else
  if (suffix == F && size == 8) {
    d = v.d;
    p = (unsigned *) &d;
    print("\t.word\t0x%x\n", p[swap]);
    print("\t.word\t0x%x\n", p[1 - swap]);
  } else
  if (suffix == P) {
    print("\t.word\t0x%X\n", (unsigned long) v.p);
  } else
  if (size == 1) {
    print("\t.byte\t0x%x\n",
          (unsigned) ((unsigned char) (suffix == I ? v.i : v.u)));
  } else
  if (size == 2) {
    print("\t.half\t0x%x\n",
          (unsigned) ((unsigned short) (suffix == I ? v.i : v.u)));
  } else
  if (size == 4) {
    print("\t.word\t0x%x\n", (unsigned) (suffix == I ? v.i : v.u));
  }
}


static void defstring(int n, char *str) {
  char *s;

  for (s = str; s < str + n; s++) {
    print("\t.byte\t0x%x\n", (*s) & 0xFF);
  }
}


static void defsymbol(Symbol s) {
  if (s->scope >= LOCAL && s->sclass == STATIC) {
    s->x.name = stringf("L.%d", genlabel(1));
  } else
  if (s->generated) {
    s->x.name = stringf("L.%s", s->name);
  } else {
    assert(s->scope != CONSTANTS || isint(s->type) || isptr(s->type));
    s->x.name = s->name;
  }
}


static void export(Symbol s) {
  print("\t.globl %s\n", s->name);
}


static int bitcount(unsigned mask) {
  unsigned i, n;

  n = 0;
  for (i = 1; i != 0; i <<= 1) {
    if (mask & i) {
      n++;
    }
  }
  return n;
}


static Symbol argreg(int argno, int offset, int ty, int sz, int ty0) {
  assert((offset & 3) == 0);
  if (offset > 20) {
    return NULL;
  }
  return ireg[(offset / 4) + 12];
}


static void function(Symbol f, Symbol caller[], Symbol callee[], int ncalls) {
  int i;
  Symbol p, q;
  Symbol r;
  int sizeisave;
  int saved;
  Symbol argregs[6];

  usedmask[0] = usedmask[1] = 0;
  freemask[0] = freemask[1] = ~((unsigned) 0);
  offset = 0;
  maxoffset = 0;
  maxargoffset = 0;
  for (i = 0; callee[i] != NULL; i++) {
    p = callee[i];
    q = caller[i];
    assert(q != NULL);
    offset = roundup(offset, q->type->align);
    p->x.offset = q->x.offset = offset;
    p->x.name = q->x.name = stringd(offset);
    r = argreg(i, offset, optype(ttob(q->type)),
               q->type->size, optype(ttob(caller[0]->type)));
    if (i < 6) {
      argregs[i] = r;
    }
    offset = roundup(offset + q->type->size, 4);
    if (variadic(f->type)) {
      p->sclass = AUTO;
    } else
    if (r != NULL && ncalls == 0 && !isstruct(q->type) &&
        !p->addressed && !(isfloat(q->type) && r->x.regnode->set == IREG)) {
      p->sclass = q->sclass = REGISTER;
      askregvar(p, r);
      assert(p->x.regnode && p->x.regnode->vbl == p);
      q->x = p->x;
      q->type = p->type;
    } else
    if (askregvar(p, rmap(ttob(p->type))) &&
        r != NULL && (isint(p->type) || p->type == q->type)) {
      assert(q->sclass != REGISTER);
      p->sclass = q->sclass = REGISTER;
      q->type = p->type;
    }
  }
  assert(caller[i] == NULL);
  offset = 0;
  gencode(caller, callee);
  if (ncalls != 0) {
    usedmask[IREG] |= ((unsigned) 1) << 1;
  }
  //usedmask[IREG] &= 0x80FF0000;
  usedmask[IREG] &= 0x0FFC0002;
  usedmask[FREG] &= 0xFFF00000;
  maxargoffset = roundup(maxargoffset, 4);
  if (ncalls != 0 && maxargoffset < 16) {
    maxargoffset = 24;
  }
  sizeisave = 4 * bitcount(usedmask[IREG]);
  framesize = roundup(maxargoffset + sizeisave + maxoffset, 16);
  segment(CODE);
  print("\t.align\t4\n");
  print("%s:\n", f->x.name);
  if (framesize > 0) {
    print("\taddi x2,x2,-%d\n", framesize);
  }
  saved = maxargoffset;
  for (i = 0; i < 32; i++) {
    if (usedmask[IREG] & (1 << i)) {
      print("\tsw x%d,%d(x2)\n", i, saved);
      saved += 4;
    }
  }
  for (i = 0; i < 6 && callee[i] != NULL; i++) {
    r = argregs[i];
    if (r && r->x.regnode != callee[i]->x.regnode) {
      Symbol out = callee[i];
      Symbol in = caller[i];
      int rn = r->x.regnode->number;
      int rs = r->x.regnode->set;
      int tyin = ttob(in->type);
      assert(out && in && r && r->x.regnode);
      assert(out->sclass != REGISTER || out->x.regnode);
      if (out->sclass == REGISTER &&
          (isint(out->type) || out->type == in->type)) {
        int outn = out->x.regnode->number;
        print("\tadd x%d,x0,x%d\n", outn, rn);
      } else {
        int off = in->x.offset + framesize;
        int n = (in->type->size + 3) / 4;
        int i;
        for (i = rn; i < rn + n; i++) {
          print("\tsw x%d,%d(x2)\n", i, off + (i - rn) * 4);
        }
      }
    }
  }
  if (variadic(f->type) && callee[i - 1] != NULL) {
    i = callee[i - 1]->x.offset + callee[i - 1]->type->size;
    for (i = roundup(i, 4)/4; i < 6; i++) {
      print("\tsw x%d,%d(x2)\n", i + 12, framesize + 4 * i);
    }
  }
  emitcode();
  saved = maxargoffset;
  for (i = 0; i < 32; i++) {
    if (usedmask[IREG] & (1 << i)) {
      print("\tlw x%d,%d(x2)\n", i, saved);
      saved += 4;
    }
  }
  if (framesize > 0) {
    print("\taddi x2,x2,%d\n", framesize);
  }
  print("\tjalr x0,x1,0\n");
  print("\n");
}


static void global(Symbol s) {
  if (s->type->align != 0) {
    print("\t.align\t%d\n", s->type->align);
  } else {
    print("\t.align\t%d\n", 4);
  }
  print("%s:\n", s->x.name);
}


static void import(Symbol s) {
  //print("\t.import %s\n", s->name);
}


static void local(Symbol s) {
  if (askregvar(s, rmap(ttob(s->type))) == 0) {
    mkauto(s);
  }
}


static void setSwap(void) {
  union {
    char c;
    int i;
  } u;

  u.i = 0;
  u.c = 1;
  swap = ((u.i == 1) != IR->little_endian);
}


static void progbeg(int argc, char *argv[]) {
  int i;

  setSwap();
  segment(CODE);
  parseflags(argc, argv);
  for (i = 0; i < 32; i++) {
    ireg[i] = mkreg("%d", i, 1, IREG);
  }
  iregw = mkwildcard(ireg);
  for (i = 0; i < 32; i += 2) {
    freg2[i] = mkreg("%d", i, 3, FREG);
  }
  ireg[2]->x.name = "x2";
  freg2w = mkwildcard(freg2);
  tmask[IREG] = INTTMP;
  vmask[IREG] = INTVAR;
  tmask[FREG] = FLTTMP;
  vmask[FREG] = FLTVAR;
  blkreg = mkreg("5", 5, 7, IREG);
}


static void progend(void) {
 print("\t.align 4\n");
}


static void segment(int n) {
  static int currSeg = -1;
  int newSeg;

  switch (n) {
    case CODE:
      newSeg = CODE;
      break;
    case BSS:
      newSeg = BSS;
      break;
    case DATA:
      newSeg = DATA;
      break;
    case LIT:
      newSeg = DATA;
      break;
  }
  if (currSeg == newSeg) {
    return;
  }
  print("\t.align 4\n");
  switch (newSeg) {
    case CODE:
      print("\t.text\n");
      break;
    case BSS:
      print("\t.bss\n");
      break;
    case DATA:
      print("\t.data\n");
      break;
  }
  currSeg = newSeg;
}


static void space(int n) {
  print("\t.space\t%d\n", n);
}


static Symbol rmap(int opk) {
  switch (optype(opk)) {
    case I:
    case U:
    case P:
    case B:
    case F:
      return iregw;
    //case F:
    //  return freg2w;
    default:
      return 0;
  }
}


static void blkfetch(int size, int off, int reg, int tmp) {
  assert(size == 1 || size == 2 || size == 4);
  if (size == 1) {
    print("\tlb x%d,%d(x%d)\n", tmp, off, reg);
  } else
  if (size == 2) {
    print("\tlh x%d,%d(x%d)\n", tmp, off, reg);
  } else
  if (size == 4) {
    print("\tlw x%d,%d(x%d)\n", tmp, off, reg);
  }
}


static void blkstore(int size, int off, int reg, int tmp) {
  assert(size == 1 || size == 2 || size == 4);
  if (size == 1) {
    print("\tsb\tx%d, %d(x%d)\n", tmp, off, reg);
  } else
  if (size == 2) {
    print("\tsh\tx%d, %d(x%d)\n", tmp, off, reg);
  } else
  if (size == 4) {
    print("\tsw\tx%d, %d(x%d)\n", tmp, off, reg);
  }
}


static void blkloop(int dreg, int doff,
                    int sreg, int soff,
                    int size, int tmps[]) {
  int label;

  label = genlabel(1);
  print("\taddi\tx%d,x%d,%d\n", sreg, sreg, size & ~7);
  print("\taddi\tx%d,x%d,%d\n", tmps[2], dreg, size & ~7);
  blkcopy(tmps[2], doff, sreg, soff, size & 7, tmps);
  print("L.%d:\n", label);
  print("\taddi\tx%d,x%d,-%d\n", sreg, sreg, 8);
  print("\taddi\tx%d,x%d,-%d\n", tmps[2], tmps[2], 8);
  blkcopy(tmps[2], doff, sreg, soff, 8, tmps);
  print("\tbltu\tx%d,x%d,L.%d\n", dreg, tmps[2], label);
}


static void emit2(Node p) {
  static int ty0;
  int ty, sz;
  Symbol q;
  int src;
  int dst, n;

  switch (specific(p->op)) {
    case ARG+F:
    case ARG+I:
    case ARG+P:
    case ARG+U:
      ty = optype(p->op);
      sz = opsize(p->op);
      if (p->x.argno == 0) {
        ty0 = ty;
      }
      q = argreg(p->x.argno, p->syms[2]->u.c.v.i, ty, sz, ty0);
      src = getregnum(p->x.kids[0]);
      if (q == NULL) {
        print("\tsw x%d,%d(x2)\n", src, p->syms[2]->u.c.v.i);
      }
      break;
    case ASGN+B:
      dalign = p->syms[1]->u.c.v.i;
      salign = p->syms[1]->u.c.v.i;
      blkcopy(getregnum(p->x.kids[0]), 0,
              getregnum(p->x.kids[1]), 0,
              p->syms[0]->u.c.v.i, tmpregs);
      break;
  }
}


static void doarg(Node p) {
  static int argno;
  int align;
  int size;
  int offset;

  if (argoffset == 0) {
    argno = 0;
  }
  p->x.argno = argno++;
  align = p->syms[1]->u.c.v.i;
  if (align < 4) {
    align = 4;
  }
  size = p->syms[0]->u.c.v.i;
  offset = mkactual(align, size);
  p->syms[2] = intconst(offset);
}


static void target(Node p) {
  static int ty0;
  int ty;
  Symbol q;

  assert(p);
  switch (specific(p->op)) {
    case DIV + F:
    case MOD + F:
    case MUL + F:
    case ADD + F:
    case SUB + F:
      setreg (p, ireg[10]);
      rtarget (p, 0, ireg[12]);
      rtarget (p, 1, ireg[13]);
      break;
    case NEG + F:
    case CVI + F:
    case CVF + I:
      setreg (p, ireg[10]);
      rtarget (p, 0, ireg[12]);
      break;
    case CNST+I:
    case CNST+P:
    case CNST+U:
      if (range(p, 0, 0) == 0) {
        setreg(p, ireg[0]);
        p->x.registered = 1;
      }
      break;
    case CALL+I:
    case CALL+P:
    case CALL+U:
    case CALL+F:
      rtarget(p, 0, ireg[31]);
      setreg(p, ireg[10]);
      break;
    //case CALL+F:
    //  rtarget(p, 0, ireg[31]);
    //  setreg(p, freg2[0]);
    //  break;
    case CALL+V:
      rtarget(p, 0, ireg[31]);
      break;
    case RET+I:
    case RET+P:
    case RET+U:
    case RET+F:
      rtarget(p, 0, ireg[10]);
      break;
    //case RET+F:
    //  rtarget(p, 0, freg2[0]);
    //  break;
    case ARG+I:
    case ARG+P:
    case ARG+U:
    case ARG+F:
      ty = optype(p->op);
      q = argreg(p->x.argno, p->syms[2]->u.c.v.i, ty, opsize(p->op), ty0);
      if (p->x.argno == 0) {
        ty0 = ty;
      }
      if (q) {
        rtarget(p, 0, q);
      }
      break;
    case ASGN+B:
      rtarget(p->kids[1], 0, blkreg);
      break;
    case ARG+B:
      rtarget(p->kids[0], 0, blkreg);
      break;
  }
}


static void clobber(Node p) {
  assert(p);
  switch (specific(p->op)) {
    //case CALL+F:
    //  spill(INTTMP | INTRET, IREG, p);
    // spill(FLTTMP, FREG, p);
    //  break;
    case CALL+I:
    case CALL+P:
    case CALL+U:
    case CALL+F:
    case ADD+F:
    case SUB+F:
    case NEG+F:
    case DIV+F:
    case MOD+F:
    case MUL+F:
    case CVI+F:
    case CVF+I:
      spill(INTTMP, IREG, p);
      //spill(FLTTMP | FLTRET, FREG, p);
      break;
    case CALL+V:
      spill(INTTMP | INTRET, IREG, p);
      //spill(FLTTMP | FLTRET, FREG, p);
      break;
  }
}

static int
mulops_calls (int op)
{

  if ((generic (op) == ADD || generic (op) == SUB || generic (op) == DIV
  || generic (op) == MOD || generic (op) == MUL) &&   optype (op) == F)
    return 1;

  if (generic (op) == NEG && optype (op) == F)
    return 1;

  if (generic (op) == CVI && optype (op) == F)
    return 1;

  if (generic (op) == CVF && optype (op) == I)
    return 1;

  return 0;
}

static void stabinit(char *, int, char *[]);
static void stabline(Coordinate *);
extern void stabsym(Symbol);
extern void stabtype(Symbol);
extern void stabblock(int, int, Symbol*);
extern void stabfend(Symbol, int);
static char *currentfile;

/* stabinit - initialize stab output */
static void stabinit(char *file, int argc, char *argv[]) {
        if (file) {
                print("\t.file 1 \"%s\"\n", file);
                currentfile = file;
        }
}

/* stabline - emit stab entry for source coordinate *cp */
static void stabline(Coordinate *cp) {
        if (cp->file && cp->file != currentfile) {
                print("\t.file 1 \"%s\"\n", cp->file);
                currentfile = cp->file;
        }
        print("\t.loc 1 %d\n", cp->y);
}

/* stabsym - output a stab entry for symbol p */
//static void stabsym(Symbol p) {
//        if (p == cfunc && IR->stabline)
//                (*IR->stabline)(&p->src);
//}

Interface riscv32IR = {
  1, 1, 0,      /* char */
  2, 2, 0,      /* short */
  4, 4, 0,      /* int */
  4, 4, 0,      /* long */
  4, 4, 0,      /* long long */
  4, 4, 1,      /* float */
  4, 4, 1,      /* double */
  4, 4, 1,      /* long double */
  4, 4, 0,      /* T * */
  0, 1, 0,      /* struct */
  1,            /* little_endian */
  mulops_calls, /* mulops_calls */
  0,            /* wants_callb */
  0,            /* wants_argb */
  1,            /* left_to_right */
  0,            /* wants_dag */
  0,            /* unsigned_char */
  address,
  blockbeg,
  blockend,
  defaddress,
  defconst,
  defstring,
  defsymbol,
  emit,
  export,
  function,
  gen,
  global,
  import,
  local,
  progbeg,
  progend,
  segment,
  space,
  stabblock, 0, stabfend, stabinit, stabline, stabsym, stabtype,
  {
    1,      /* max_unaligned_load */
    rmap,
    blkfetch, blkstore, blkloop,
    _label,
    _rule,
    _nts,
    _kids,
    _string,
    _templates,
    _isinstruction,
    _ntname,
    emit2,
    doarg,
    target,
    clobber
  }
};
