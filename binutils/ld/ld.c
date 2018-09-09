/*
 * ld.c -- RISCV32 linking loader
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>

#include "../include/a.out.h"
#include "../include/ar.h"
#include "../include/ranlib.h"

#include "set.h"
#include "atom.h"
#include "mem.h"


/**************************************************************/


#define MAX_STRLEN	400

#define PAGE_SHIFT	12
#define PAGE_SIZE	(1 << PAGE_SHIFT)
#define PAGE_MASK	(PAGE_SIZE - 1)
#define PAGE_ROUND(n)	(((n) + PAGE_SIZE - 1) & ~PAGE_MASK)

#define MSB	((unsigned int) 1 << (sizeof(unsigned int) * 8 - 1))
#define SEGALIGN 4

#define DBG_LINE 1
#define DBG_FUNC 2
#define DBG_VARGLO 3
#define DBG_TYPEDEF 4
#define DBG_VARLOCSTACK 5
#define DBG_VARLOCREG 6
/**************************************************************/


int debugLink = 0;
int debugFixup = 0;

int withHeader = 1;
int withDebug = 0;
int rvc = 0;

char codeName[L_tmpnam];
char dataName[L_tmpnam];
char debName[L_tmpnam];

char *outName = NULL;
char *libName = NULL;
char *mapName = NULL;
char *inName = NULL;
char outpath[40];
char destfile[40];

FILE *codeFile = NULL;
FILE *dataFile = NULL;
FILE *outFile = NULL;
FILE *debFile =NULL;
FILE *mapFile = NULL;
FILE *inFile = NULL;

unsigned int segPtr[4] = { 0, 0, 0, 0 };
int segStartDefined[4] = { 0, 0, 0, 0 };
unsigned int segStart[4] = { 0, 0, 0, 0 };
char *segName[4] = { "ABS", "CODE", "DATA", "BSS" };
char *methodName[6] = { "W32", "R12", "RL12", "RH20", "RS12", "J20" };
unsigned int *rvcadr;


typedef struct reloc {
  int segment;			/* in which segment to relocate */
  unsigned int offset;		/* where in the segment to relocate */
  int method;			/* how to relocate */
  int value;			/* additive part of value */
  int type;			/* 0: base is a segment */
				/* 1: base is a symbol */
  union {			/* relocation relative to .. */
    int segment;		/* .. a segment */
    struct symbol *symbol;	/* .. a symbol */
  } base;
  struct reloc *next;		/* next relocation */
} Reloc;


typedef struct symbol {
  char *name;			/* name of symbol */
  char *filename;
  int type;			/* if MSB = 0: the symbol's segment */
				/* if MSB = 1: the symbol is undefined */
  int value;			/* if symbol defined: the symbol's value */
				/* if symbol not defined: meaningless */
  struct symbol *left;		/* left son in binary search tree */
  struct symbol *right;		/* right son in binary search tree */
  int debug;
  int rvcdelta;
} Symbol;

typedef struct lelem {
  void *valptr;
  int (*cmp)(const void *x, const void *y); 
  struct lelem *next;
} Lelem;

typedef struct {
  unsigned int name;	/* name of symbol (as offset into string space) */
  long position;	/* position of member which defines the symbol */
			/* (as file offset to the member's ArHeader) */
} Entry;
ArHeader arhdr;

void updatesymbols(void);

int cmpstring(const void *x, const void *y) {
	return strcmp((char *)x, (char *)y);
}
 
int cmpint(const void *x, const void *y) {
	if (*(int *)x < *(int *)y)
		return -1;
	else if (*(int *)x > *(int *)y)
		return +1;
	else
		return 0;
}

int cmpint2(const void *x, const void *y) {
	if (**(int **)x < **(int **)y)
		return -1;
	else if (**(int **)x > **(int **)y)
		return +1;
	else
		return 0;
}

int intcmp(const void *x, const void *y) {
	return cmpint2(&x, &y);
} 

unsigned inthash(const void *x) {
	return *(int *)x;
} 

int signext32(int n, unsigned int val)
{
  int res = val | ((val & 2<<(n-1)) ? (0xFFFFFFFF & ((2<<n)-1)): 0);
  return(res);
}

int insrange(int bits, int val) {
  unsigned int msb = 1<<(bits-1);
  int ll = -msb;
  return((val<=(msb-1) && val>=ll) ? 1 : 0);
}

int rvcreg(int reg) {
 return((reg>=8 && reg<=15)? 1 : 0);
}

void error(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "Error: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  if (codeFile != NULL) {
    fclose(codeFile);
    codeFile = NULL;
  }
  if (dataFile != NULL) {
    fclose(dataFile);
    dataFile = NULL;
  }
  if (outFile != NULL) {
    fclose(outFile);
    outFile = NULL;
  }
  if (debFile != NULL) {
    fclose(debFile);
    debFile = NULL;
  }
  if (mapFile != NULL) {
    fclose(mapFile);
    mapFile = NULL;
  }
  if (inFile != NULL) {
    fclose(inFile);
    inFile = NULL;
  }
  if (codeName != NULL) {
    unlink(codeName);
  }
  if (dataName != NULL) {
    unlink(dataName);
  }
  if (outName != NULL) {
    unlink(outName);
  }
  if (mapName != NULL) {
    unlink(mapName);
  }
  exit(1);
}

/**************************************************************/
FILE *inFile;
ExecHeader inHeader; 

#define CODE_START(h)	(sizeof(ExecHeader))
#define DATA_START(h)	(CODE_START(h) + (h).csize)
#define CRELOC_START(h)	(DATA_START(h) + (h).dsize)
#define DRELOC_START(h)	(CRELOC_START(h) + (h).crsize)
#define SYMTBL_START(h)	(DRELOC_START(h) + (h).drsize)
#define STRING_START(h)	(SYMTBL_START(h) + (h).symsize)

 

void dumpString(unsigned int offset) {
  long pos;
  int c;

  pos = ftell(inFile);
  if (fseek(inFile, STRING_START(inHeader) + offset, SEEK_SET) < 0) {
    error("cannot seek to string");
  }
  while (1) {
    c = fgetc(inFile);
    if (c == EOF) {
      error("unexpected end of file");
    }
    if (c == 0) {
      break;
    }
    fputc(c, stdout);
  }
  fseek(inFile, pos, SEEK_SET);
}

 
 



void *allocateMemory(unsigned int size) {
  void *p;

  p = malloc(size);
  if (p == NULL) {
    error("out of memory");
  }
  return p;
}


void freeMemory(void *p) {
  free(p);
}


/**************************************************************/


Reloc *relocs = NULL;
Lelem *clist = NULL;
Lelem *dlist = NULL;
Set_T undefset;
Set_T defset;
Set_T rvcset;
void **rvcarray;


void addReloc(int segment, RelocRecord *relRec, Symbol **symMap) {
  Reloc *rel;

  rel = allocateMemory(sizeof(Reloc));
  rel->segment = segment;
  rel->offset = relRec->offset + segPtr[segment];
  rel->method = relRec->method;
  rel->value = relRec->value;
  if ((relRec->base & MSB) == 0) {
    /* relocation is relative to a segment */
    rel->type = 0;
    rel->base.segment = relRec->base;
    rel->value += segPtr[relRec->base];
  } else {
    /* relocation is relative to a symbol */
    rel->type = 1;
    rel->base.symbol = symMap[relRec->base & ~MSB];
  }
  rel->next = relocs;
  relocs = rel;
}


void linkSymbol(Reloc *rel) {
  Symbol *sym;

  /* check if this is a reference to a symbol */
  if (rel->type != 1) {
    /* no: nothing to do here */
    return;
  }
  /* get information from the symbol table record */
  sym = rel->base.symbol;
  if (sym->type & MSB) {
    error("undefined symbol '%s'", sym->name);
  }
  /* output debugging info */
  if (debugLink) {
    printf("DEBUG: link '%s' (s:%s, v:%08X)\n",
           sym->name, segName[sym->type], sym->value);
    printf("       (s:%s, o:%08X, m:%s, v:%08X --> %08X, b:%s)\n",
           segName[rel->segment], rel->offset, methodName[rel->method],
           rel->value, rel->value + sym->value, segName[sym->type]);
  }
  /* update relocation information */
  rel->value += sym->value;
  rel->type = 0;
  rel->base.segment = sym->type;
}


void linkSymbols(void) {
  Reloc *rel;
  rel = relocs;
  while (rel != NULL) {
    linkSymbol(rel);
    rel = rel->next;
  }
}

Lelem *newLelem(int cmp(const void *x, const void *y)) {
  Lelem *p;
  p = allocateMemory(sizeof(Lelem));
  p->next = NULL;
  p->valptr = NULL;
  p->cmp = cmp;
  return p;
}


void foreachLelem(Lelem *list, void op(Lelem *x)) {
    Lelem *p = list;
    while(p!=NULL) {
        op(p);
        p = p->next;
    }
}


int getmaxpos2(int number) {
 int i=0;
 while((i<Set_length(rvcset)) && (number > *(unsigned int*)rvcarray[i])) i++;
 return(i);
}

void fixupRef(Reloc *rel, int scan) {
  FILE *file;
  unsigned int value;
  unsigned int final;
  unsigned int type, src1, src2, dst;
  unsigned int rs1, rs2, bne;
  int offset;
  int rvcmatch=0;
  int rvcdelta=0;
  int deltadst=0,deltaofs=0;
  

  if(scan==0){
    if(rel->segment==SEGMENT_CODE) {
    switch (rel->method) {
      case METHOD_RL12: case METHOD_RS12: deltaofs = 2*getmaxpos2(rel->offset-4);break;
      default: deltaofs = 2*getmaxpos2(rel->offset);
      }
    }
    else deltaofs = 0;
    if(rel->base.segment==SEGMENT_CODE)
      deltadst = 2*getmaxpos2(rel->value);
    else
      deltadst=2*getmaxpos2(segStart[SEGMENT_DATA])&~(SEGALIGN-1);
    rvcdelta = deltadst-deltaofs;
   }

  /* determine the segment in which to do fixup */
  switch (rel->segment) {
    case SEGMENT_ABS:
      /* this should never happen */
      error("cannot do fixup in ABS");
      break;
    case SEGMENT_CODE:
      file = codeFile;
      break;
    case SEGMENT_DATA:
      file = dataFile;
      break;
    case SEGMENT_BSS:
      /* this should never happen */
      error("cannot do fixup in BSS");
      break;
    default:
      /* this should never happen */
      error("illegal segment in doFixup()");
      break;
  }
  /* check that the base is indeed a segment */
  if (rel->type != 0) {
    /* this should never happen */
    error("fixup cannot handle reference to symbol");
  }
  /* now patch according to method */
  switch (rel->method) {

    case METHOD_W32:
      value = rel->value + segStart[rel->base.segment] - rvcdelta;
      final = value;
      if(scan==0) {
       fseek(file, rel->offset, SEEK_SET);
       fputc((final >> 0) & 0xFF, file);
       fputc((final >> 8) & 0xFF, file);
       fputc((final >> 16) & 0xFF, file);
       fputc((final >> 24) & 0xFF, file);
      }
      break;
    case METHOD_R12:
      value = (rel->value - rel->offset - rvcdelta);
      if((value&1)) error("jal not half word aligned");
      fseek(file, rel->offset, SEEK_SET);
      fread(&final, sizeof(final), 1, file);
      rs1 = (final>>15) & 0x1F;
      rs2 = (final>>20) & 0x1F;
      value &= 0x1FFF;
      offset = signext32(13, value);
      type = (final>>14) & 1;
      bne =  (final>>12) & 1;
      if((rvc==0) || (rvcreg(rs1)==0) || (insrange(9, offset)==0) || type!=0 || rs2!=0 ) {
      final |= (((value>>12)&1)<<31 |((value>>5)&0x3F)<<25 | ((value>>1)&0xF)<<8 | ((value>>11)&1)<<7 );
      } else {
      final = (3<<14 | bne<<13 | ((value>>8)&1)<<12 | ((value>>3)&3)<<10 | (rs1-8)<<7 | ((value>>6)&3)<<5 | ((value>>1)&3)<<3 | ((value>>5)&1)<<2 | 1 );
      final &= 0xFFFF;
      final |= 0x10000;
      rvcmatch = 1;
      }
      if(scan==0) {
       fseek(file, rel->offset, SEEK_SET);
       fputc((final >> 0) & 0xFF, file);
       fputc((final >> 8) & 0xFF, file);
       fputc((final >> 16) & 0xFF, file);
       fputc((final >> 24) & 0xFF, file);
      }
      break;
    case METHOD_RL12:
      value = rel->value - rel->offset  - rvcdelta + 4 + segStart[rel->base.segment] - segStart[rel->segment];
      fseek(file, rel->offset, SEEK_SET);
      fread(&final, sizeof(final), 1, file);
      value &= 0xFFF;
      offset = signext32(12, value);
      src1 = (final>>15) & 0x1F;
      dst = (final>>7) & 0x1F;
      type = (final>>13) & 1;
      if((rvc==0) || (insrange(7, offset)==0) || (rvcreg(dst)==0) || (rvcreg(src1)==0) || type==0)
        final |= (value<<20);
      else {
        final = 2<<13 | ((value>>3)&0x7)<<10 | (src1-8)<<7 | ((value>>2)&0x1)<<6 | ((value>>6)&0x1)<<5 | (dst-8)<<2 | 0x0;
        final &= 0xFFFF;
        final |= 0x10000;
        rvcmatch = 1;
      }
      if(scan==0) {
       fseek(file, rel->offset, SEEK_SET);
       fputc((final >> 0) & 0xFF, file);
       fputc((final >> 8) & 0xFF, file);
       fputc((final >> 16) & 0xFF, file);
       fputc((final >> 24) & 0xFF, file);
      }
      break;
    case METHOD_RH20:
      value = rel->value - rel->offset - rvcdelta + segStart[rel->base.segment] - segStart[rel->segment];
      fseek(file, rel->offset, SEEK_SET);
      fread(&final, sizeof(final), 1, file);
      if((value & 0x800)!=0) value+=0x1000;
      value &= 0xFFFFF000;
      final |= value;
      if(scan == 0) {
       fseek(file, rel->offset, SEEK_SET);
       fputc((final >> 0) & 0xFF, file);
       fputc((final >> 8) & 0xFF, file);
       fputc((final >> 16) & 0xFF, file);
       fputc((final >> 24) & 0xFF, file);
      }
      break;
    case METHOD_RS12:
      value = rel->value - rel->offset  - rvcdelta + 4 + segStart[rel->base.segment] - segStart[rel->segment];
      fseek(file, rel->offset, SEEK_SET);
      fread(&final, sizeof(final), 1, file);
      value &= 0xFFF;
      offset = signext32(12, value);
      src1 = (final>>15) & 0x1F;
      src2 = (final>>20) & 0x1F;
      type = (final>>13) & 1;
      if((rvc==0) || (insrange(7, offset)==0) || (rvcreg(src2)==0) || (rvcreg(src1)==0) || type==0)
        final |= (value&0x1F)<<7 | (value>>5)<<25;
      else {
        final = 6<<13 | ((value>>3)&0x7)<<10 | (src1-8)<<7 | ((value>>2)&0x1)<<6 | ((value>>6)&0x1)<<5 | (src2-8)<<2 | 0x0;
        final &= 0xFFFF;
        final |= 0x10000;
        rvcmatch = 1;
      }
      if(scan==0) {
       fseek(file, rel->offset, SEEK_SET);
       fputc((final >> 0) & 0xFF, file);
       fputc((final >> 8) & 0xFF, file);
       fputc((final >> 16) & 0xFF, file);
       fputc((final >> 24) & 0xFF, file);
      }
      break;
    case METHOD_J20:
      value = (rel->value - rel->offset - rvcdelta );
      if((value&1)) error("jal not half word aligned");
      fseek(file, rel->offset, SEEK_SET);
      fread(&final, sizeof(final), 1, file);
      value &= 0x1FFFFF;
      offset = signext32(21, value);
      dst = (final>>7) & 0x1F;
      if((rvc==0) || (insrange(12, offset)==0) || dst!=1)
        final |= ((value>>20)&0x1)<<31 | ((value>>1)&0x3FF)<<21 | ((value>>11)&0x1)<<20 | ((value>>12)&0xFF)<<12;
      else {
        final = 1<<13 | ((value>>11)&0x1)<<12 | ((value>>4)&0x1)<<11 |((value>>8)&0x3)<<9 | ((value>>10)&0x1)<<8 | ((value>>6)&0x1)<<7 | ((value>>7)&0x1)<<6 | ((value>>1)&0x7)<<3 | ((value>>5)&0x1)<<2 | 1;
        final &= 0xFFFF;
        final |= 0x10000;
        rvcmatch = 1;
      }
      if(scan==0) {
       fseek(file, rel->offset, SEEK_SET);
       fputc((final >> 0) & 0xFF, file);
       fputc((final >> 8) & 0xFF, file);
       fputc((final >> 16) & 0xFF, file);
       fputc((final >> 24) & 0xFF, file);
      }
      break;
    default:
      /* this should never happen */
      error("illegal method in doFixup()");
      break;
  }
  if(rvcmatch && scan==1) {
   NEW(rvcadr);   
   *rvcadr = (rel->offset)+2;
   Set_put(rvcset, rvcadr);		
  }
  /* output debugging info */
  if (debugFixup) {
    printf("DEBUG: fixup (s:%s, o:%08X, m:%s, v:%08X), %08X --> %08X,delta %i, dst %i, ofs %i\n",
           segName[rel->segment], rel->offset, methodName[rel->method],
           rel->value, value, final, rvcdelta, deltadst, deltaofs);
  }
}


void relocateSegments(void) {
  Reloc *rel;

  /* determine start of segments */
  if (!segStartDefined[SEGMENT_CODE]) {
    segStart[SEGMENT_CODE] = 0;
    segStartDefined[SEGMENT_CODE] = 1;
  }
  if (!segStartDefined[SEGMENT_DATA]) {
    segStart[SEGMENT_DATA] = segStart[SEGMENT_CODE]+segPtr[SEGMENT_CODE]; //+
                            // PAGE_ROUND(segPtr[SEGMENT_CODE]);
    segStartDefined[SEGMENT_DATA] = 1;
  }
  if (!segStartDefined[SEGMENT_BSS]) {
    segStart[SEGMENT_BSS] = segStart[SEGMENT_DATA] +
                            segPtr[SEGMENT_DATA];
    segStartDefined[SEGMENT_BSS] = 1;
  }
  /* fixup all references (which now are only relative to segments) */
  rel = relocs;
  while (rel != NULL) {
    fixupRef(rel, 1);
    rel = rel->next;
  }
  //rvclist = List_reverse(rvclist);
  //rvcarray = List_toArray(rvclist, NULL);  
    rvcarray = Set_toArray(rvcset, NULL);
  //printf("Len:%d\n",Set_length(rvcset));
   //for(i=0;i<Set_length(rvcset);i++) printf("BS: %d\r\n", *(unsigned int*)rvcarray[i]);
    qsort(rvcarray, Set_length(rvcset), sizeof (*rvcarray),  cmpint2); 
   //for(i=0;i<Set_length(rvcset);i++) printf("AS: %d\r\n", *(unsigned int*)rvcarray[i]);
  
  updatesymbols();
  while (relocs != NULL) {
    rel = relocs;
    relocs = rel->next;
    fixupRef(rel, 0);
    freeMemory(rel);
  }
}


/**************************************************************/


Symbol *symbolTable = NULL;
Symbol *libsymbolTable = NULL;


Symbol *newSymbol(char *name, char* filename, int debug) {
  Symbol *p;

  p = allocateMemory(sizeof(Symbol));
  p->name = allocateMemory(strlen(name) + 1);
  strcpy(p->name, name);
  if(filename) {
    p->filename = allocateMemory(strlen(filename) + 1);
    strcpy(p->filename, filename);
  } else p->filename = NULL;
  p->type = MSB;
  p->value = 0;
  p->left = NULL;
  p->right = NULL;
  p->debug = debug;
  p->rvcdelta = 0;
  return p;
}


Symbol *lookupEnter(Symbol **table, char *name, char* filename, int debug) {
  Symbol *p, *q, *r;
  int cmp;

  p = *table;
  if (p == NULL) {
    r = newSymbol(name, filename, debug);
    *table = r;
    return r;
  }
  while (1) {
    q = p;
    cmp = strcmp(name, q->name);
    if (cmp == 0) {
      return q;
    }
    if (cmp < 0) {
      p = q->left;
    } else {
      p = q->right;
    }
    if (p == NULL) {
      r = newSymbol(name, filename, debug);
      if (cmp < 0) {
        q->left = r;
      } else {
        q->right = r;
      }
      return r;
    }
  }
}

Symbol *lookup(Symbol *table, char *name) {
  Symbol *p, *q;
  int cmp;

  p = table;
  
  while (1) {
    q = p;
    cmp = strcmp(name, q->name);
    if (cmp == 0) {
      return q;
    }
    if (cmp < 0) {
      p = q->left;
    } else {
      p = q->right;
    }
    if (p == NULL) {      
      return(NULL);
    }
  }
}


void walkTree(Symbol *s, void (*fp)(Symbol *sp)) {
  if (s == NULL) {
    return;
  }
  walkTree(s->left, fp);
  (*fp)(s);
  walkTree(s->right, fp);
}


void walkSymbols(void (*fp)(Symbol *sym)) {
  walkTree(symbolTable, fp);
}


/**************************************************************/





//ExecHeader inHeader;
Symbol **symMap;


void readHeader(void) {
  if (fseek(inFile, 0, SEEK_SET) < 0) {
    error("cannot seek to exec header");
  }
  if (fread(&inHeader, sizeof(ExecHeader), 1, inFile) != 1) {
    error("cannot read exec header");
  }
  if (inHeader.magic != EXEC_MAGIC) {
    error("wrong magic number in exec header");
  }
}


void readCode(void) {
  unsigned char *buffer;

  if (fseek(inFile, CODE_START(inHeader), SEEK_SET) < 0) {
    error("cannot seek to code section");
  }
  buffer = allocateMemory(inHeader.csize);
  if (fread(buffer, 1, inHeader.csize, inFile) != inHeader.csize) {
    error("cannot read code segment");
  }
  if (fwrite(buffer, 1, inHeader.csize, codeFile) != inHeader.csize) {
    error("cannot write code segment");
  }
  freeMemory(buffer);
}


void readData(void) {
  unsigned char *buffer;

  if (fseek(inFile, DATA_START(inHeader), SEEK_SET) < 0) {
    error("cannot seek to data section");
  }
  buffer = allocateMemory(inHeader.dsize);
  if (fread(buffer, 1, inHeader.dsize, inFile) != inHeader.dsize) {
    error("cannot read data segment");
  }
  if (fwrite(buffer, 1, inHeader.dsize, dataFile) != inHeader.dsize) {
    error("cannot write data segment");
  }
  freeMemory(buffer);
}


void readCodeRelocs(void) {
  int n, i;
  RelocRecord relRec;

  if (fseek(inFile, CRELOC_START(inHeader), SEEK_SET) < 0) {
    error("cannot seek to code relocation section");
  }
  n = inHeader.crsize / sizeof(RelocRecord);
  for (i = 0; i < n; i++) {
    if (fread(&relRec, sizeof(RelocRecord), 1, inFile) != 1) {
      error("cannot read code relocation records");
    }
    addReloc(SEGMENT_CODE, &relRec, symMap);
  }
}


void readDataRelocs(void) {
  int n, i;
  RelocRecord relRec;

  if (fseek(inFile, DRELOC_START(inHeader), SEEK_SET) < 0) {
    error("cannot seek to data relocation section");
  }
  n = inHeader.drsize / sizeof(RelocRecord);
  for (i = 0; i < n; i++) {
    if (fread(&relRec, sizeof(RelocRecord), 1, inFile) != 1) {
      error("cannot read data relocation records");
    }
    addReloc(SEGMENT_DATA, &relRec, symMap);
  }
}


void readString(unsigned int offset, char *buffer, int size) {
  long pos;
  int c;

  pos = ftell(inFile);
  if (fseek(inFile, STRING_START(inHeader) + offset, SEEK_SET) < 0) {
    error("cannot seek to string");
  }
  do {
    c = fgetc(inFile);
    if (c == EOF) {
      error("unexpected end of file");
    }
    *buffer++ = c;
    if (--size == 0) {
      error("string buffer overflow");
    }
  } while (c != 0);
  fseek(inFile, pos, SEEK_SET);
}


void readSymbols(void) {
  int n, i;
  SymbolRecord symRec;
  char strBuf[MAX_STRLEN];
  Symbol *sym;

  if (fseek(inFile, SYMTBL_START(inHeader), SEEK_SET) < 0) {
    error("cannot seek to symbol table section");
  }
  n = inHeader.symsize / sizeof(SymbolRecord);
  symMap = allocateMemory(n * sizeof(Symbol *));
  for (i = 0; i < n; i++) {
    if (fread(&symRec, sizeof(SymbolRecord), 1, inFile) != 1) {
      error("cannot read symbol table");
    }
    readString(symRec.name, strBuf, MAX_STRLEN);
    sym = lookupEnter(&symbolTable, strBuf, NULL, symRec.debug);
    if ((symRec.type & MSB) == 0) {
      /* the symbol is defined in this symbol record */
      if ((sym->type & MSB) == 0) {
        /* the symbol was already defined in the table */
        printf("symbol '%s' multiply defined!\n\r", sym->name);
      } else {
        /* the symbol was not yet defined in the table, so define it now */
        /* the segment is copied directly from the file */
        /* the value is the sum of the value given in the file */
        /* and this module's segment start of the symbol's segment */
        sym->type = symRec.type;
        sym->value = symRec.value + segPtr[symRec.type];        
		Set_put(defset, Atom_string(strBuf));		
      }
    } else {            
	  Set_put(undefset, Atom_string(strBuf));	  
      /* the symbol is undefined in this symbol record */
      /* nothing to do here: lookupEnter already entered */
      /* the symbol into the symbol table, if necessary */
    }
    /* in any case remember the symbol table entry, so that */
    /* a symbol index in a relocation record can be resolved */
    symMap[i] = sym;
  }
}


void readModule(void) {
  /* read the file header to determine the sizes */
  readHeader();  
  /* read and transfer the code and data segments */
  readCode();
  readData();
  /* read and build the symbol table and a symbol map */
  readSymbols();
  /* read and build a list of relocation records */
  readCodeRelocs();
  readDataRelocs();
  /* free the symbol map, it is no longer needed */
  freeMemory(symMap);
  /* update accumulated segment sizes */
  segPtr[SEGMENT_CODE] += inHeader.csize;
  segPtr[SEGMENT_DATA] += inHeader.dsize;
  segPtr[SEGMENT_BSS] += inHeader.bsize;
}


/**************************************************************/


void printSymbol(Symbol *s) {
  fprintf(mapFile, "%-32s", s->name);
  if (s->type & MSB) {
    /* symbol is undefined */
    fprintf(mapFile, "%-15s", "UNDEFINED");
  } else {
    /* symbol is defined */
    switch (s->type) {
      case SEGMENT_ABS:
        fprintf(mapFile, "%-15s", "ABS");
        break;
      case SEGMENT_CODE:
        fprintf(mapFile, "%-15s", "CODE");
        break;
      case SEGMENT_DATA:
        fprintf(mapFile, "%-15s", "DATA");
        break;
      case SEGMENT_BSS:
        fprintf(mapFile, "%-15s", "BSS");
        break;
      default:
        error("illegal symbol segment in printToMap()");
    }
  }
  fprintf(mapFile, "0x%08X", s->value);
  fprintf(mapFile, "-0x%08X", s->rvcdelta);
  fprintf(mapFile, "\n");
}


void printToMapFile(void) {
  walkSymbols(printSymbol);
  fprintf(mapFile, "\n");
  fprintf(mapFile, "CODE     start 0x%08X     size 0x%08X\n",
          segStart[SEGMENT_CODE], segPtr[SEGMENT_CODE]);
  fprintf(mapFile, "DATA     start 0x%08X     size 0x%08X\n",
          segStart[SEGMENT_DATA], segPtr[SEGMENT_DATA]);
  fprintf(mapFile, "BSS      start 0x%08X     size 0x%08X\n",
          segStart[SEGMENT_BSS], segPtr[SEGMENT_BSS]);
}

void printliststring(Lelem* x) {
    printf("%s\r\n",(char *) x->valptr);
}

void printdebSymbol(Symbol *s) {
  Symbol *refsym;
  char *refname, *tmp;  
  switch (s->debug) {
    /* debug symbol */
    case DBG_LINE: fprintf(debFile, "line: %s @ 0x%08X\n", s->name, s->value+segStart[s->type]);
                   break;
    case DBG_FUNC: tmp = strdup(s->name);
                   refname = strtok(tmp, " ");
                   refsym = lookupEnter(&symbolTable, refname, NULL, 0);    
                   fprintf(debFile, "function: %s @ 0x%08X\n", refsym->name, refsym->value+segStart[refsym->type]);
                   break;
    case DBG_VARGLO: fprintf(debFile, "global: %s @ 0x%08X\n", s->name, s->value+segStart[s->type]);
                     break;
    case DBG_VARLOCSTACK: fprintf(debFile, "%s", s->name);      //local or locparam
                          break;
    case DBG_VARLOCREG: fprintf(debFile, "%s", s->name); // reglocal or regparam
                        break;
    case DBG_TYPEDEF: fprintf(debFile, "%s\n", s->name); // typedef
                      break;
    default: break;
  }
}

void printToDebFile(void) {
  walkSymbols(printdebSymbol);
}

void updatesymbol(Symbol *s) {
  s->rvcdelta=2*getmaxpos2(s->value);
}

void updatesymbols(void) {
  walkSymbols(updatesymbol);
}

/**************************************************************/


void writeHeader(void) {
  ExecHeader outHeader;

  if (withHeader) {
    outHeader.magic = EXEC_MAGIC;
    outHeader.csize = segPtr[SEGMENT_CODE];
    outHeader.dsize = segPtr[SEGMENT_DATA];
    outHeader.bsize = segPtr[SEGMENT_BSS];
    outHeader.crsize = 0;
    outHeader.drsize = 0;
    outHeader.symsize = 0;
    outHeader.strsize = 0;
    fwrite(&outHeader, sizeof(ExecHeader), 1, outFile);
  }
}


void writeCode(void) {
  int data;
  unsigned int pos=0;
  rewind(codeFile);

  while (1) {
    data = fgetc(codeFile);
    if (data == EOF) {
      break;
    }

    if(!Set_member(rvcset, &pos))		
     fputc(data, outFile);	 
    else { 
     data = fgetc(codeFile);
     pos++;
    }
    pos++;
  }
}


void writeData(void) {
  int i, data = 0;
  rewind(dataFile);
  for(i=0;i<(2*getmaxpos2(segStart[SEGMENT_DATA])&(SEGALIGN-1));i++) fputc(data, outFile);;
  while (1) {
    data = fgetc(dataFile);
    if (data == EOF) {
      break;
    }
     fputc(data, outFile);
  }
}


/**************************************************************/


int readNumber(char *str, unsigned int *np) {
  int base;
  int value;
  int digit;

  base = 10;
  value = 0;
  if (*str == '0') {
    str++;
    if (*str == 'x' || *str == 'X') {
      base = 16;
      str++;
    } else
    if (isdigit((int) *str)) {
      base = 8;
    } else
    if (*str == '\0') {
      *np = value;
      return 1;
    } else {
      return 0;
    }
  }
  while (isxdigit((int) *str)) {
    digit = *str++ - '0';
    if (digit >= 'A' - '0') {
      if (digit >= 'a' - '0') {
        digit += '0' - 'a' + 10;
      } else {
        digit += '0' - 'A' + 10;
      }
    }
    if (digit >= base) {
      return 0;
    }
    value *= base;
    value += digit;
  }
  if (*str == '\0') {
    *np = value;
    return 1;
  } else {
    return 0;
  }
}

int readlibsymbols(char * libname) {
  unsigned int arMagic; 
  FILE *libfile;
  int numSymbols;
  int i;
  long pos;
  
  unsigned int n = 40;
  char symname[40], filename[40];
  char* symnameptr = &symname[0];
  char* filenameptr = &filename[0];

  libfile = fopen(libname, "r");
  if (libfile == NULL) {
    printf("error: cannot opening library file\n");
    exit(1);
  }
  if (fread(&arMagic, sizeof(arMagic), 1, libfile) != 1 ||
      arMagic != AR_MAGIC) {
    printf("ar: not in archive format\n");
    fclose(libfile);
    return 1;
  } 
  if (fread(&arhdr, sizeof(arhdr), 1, libfile) != 1) {
    return 1;
  } 
  if (fread(&numSymbols, sizeof(int), 1, libfile) != 1) {
    printf("cannot read symdef file\n");
    exit(1);
  }
  pos = sizeof(arMagic)+ sizeof(arhdr) + sizeof(int) + numSymbols * sizeof(Entry);
  fseek(libfile, pos, SEEK_SET);
  for (i = 0; i < numSymbols; i++) {  
   getdelim(&symnameptr, &n, ':', libfile);
   symnameptr[strlen(symnameptr)-1] = '\0';
   getdelim(&filenameptr, &n, '\0', libfile);   
   lookupEnter(&libsymbolTable, symnameptr, filenameptr, 0);
  }
  fclose(libfile);
 return(0); 
}

int extractfilefromlib(char *libname, char *filename) {
  unsigned int arMagic; 
  FILE *in, *out;
  int i;
  int pad;    
  #define BUFSIZE	32000
  unsigned char buf[BUFSIZE];
 
  in = fopen(libname, "r");
  if (in == NULL) {
    printf("error: cannot open library file\n");
    exit(1);
  }
  if (fread(&arMagic, sizeof(arMagic), 1, in) != 1 ||
      arMagic != AR_MAGIC) {
    printf("ar: not in archive format\n");
    fclose(in);
    exit(1);
   } 
  arhdr.size = 0;
  i = 0;
  do {
	pad = -arhdr.size & 0x03;
    arhdr.size += pad; 
	fseek(in, arhdr.size, SEEK_CUR); 
	if (fread(&arhdr, sizeof(arhdr), 1, in) != 1) 
		return(1);	
	i++;
  } while (strcmp(filename, arhdr.name)!=0); 
  if (fread(buf, arhdr.size, 1, in) != 1) {
    printf("read error\n");
    exit(1);
  }
  strcpy(destfile, outpath);
  strcat(destfile, filename);
  out = fopen(destfile, "w");
  if (fwrite(buf, arhdr.size, 1, out) != 1) {
    printf("write error\n");
    exit(1);
  }
  fclose(out);
  fclose(in);  
  return(0);
} 

void usage(char *myself) {
  fprintf(stderr, "Usage: %s\n", myself);
  fprintf(stderr, "         [-h]             do not write object header\n");
  fprintf(stderr, "         [-o objfile]     set output file name\n");
  fprintf(stderr, "         [-m mapfile]     produce map file\n");
  fprintf(stderr, "         [-rc addr]       relocate code segment\n");
  fprintf(stderr, "         [-rd addr]       relocate data segment\n");
  fprintf(stderr, "         [-rb addr]       relocate bss segment\n");
  fprintf(stderr, "         file             object file name\n");
  fprintf(stderr, "         [files...]       additional object files\n");
  exit(1);
}


int main(int argc, char *argv[]) {
  int i;
  char *argp;
  char* dot;
  unsigned int *ssp;
  int *ssdp;  
  Symbol *Sym;
  char *tmp;  
  undefset = Set_new(0, NULL, NULL); 
  defset = Set_new(0, NULL, NULL);
  //rvcset =  Set_new(0, cmpint2, inthash); 
  rvcset =  Set_new(0, intcmp, inthash); 
  tmpnam(codeName);
  tmpnam(dataName);
  outName = "a.out";
  struct stat buf;

  for (i = 1; i < argc; i++) {
    argp = argv[i];
    if (*argp != '-') {
      break;
    }
    argp++;
    switch (*argp) {
      case 'h':
        withHeader = 0;
        break;
      case 'g':
        withDebug = 1;
        break;
      case 'o':
        if (i == argc - 1) {
          usage(argv[0]);
        }
        outName = argv[++i];
        break;
	  case 'l':
        if (i == argc - 1) {
          usage(argv[0]);
        }
        libName = argv[++i];
        break;
      case 'm':
        if (i == argc - 1) {
          usage(argv[0]);
        }
        mapName = argv[++i];
        break;
      case 'r':
        if (argp[1] == 'c') {
          ssp = &segStart[SEGMENT_CODE];
          ssdp = &segStartDefined[SEGMENT_CODE];
        } else
        if (argp[1] == 'd') {
          ssp = &segStart[SEGMENT_DATA];
          ssdp = &segStartDefined[SEGMENT_DATA];
        } else
        if (argp[1] == 'b') {
          ssp = &segStart[SEGMENT_BSS];
          ssdp = &segStartDefined[SEGMENT_BSS];
        } else {
          usage(argv[0]);
        }
        if (i == argc - 1) {
          usage(argv[0]);
        }
        if (!readNumber(argv[++i], ssp
        )) {
          error("cannot read number given with option '-%s'", argp);
        }
        *ssdp = 1;
        break;
       case 'c':
        if (i == argc - 1) {
          usage(argv[0]);
        }
        rvc = 1;
        break;
      default:
        usage(argv[0]);
    }
  }
  if (i == argc) {
    usage(argv[0]);
  }
  codeFile = fopen(codeName, "w+b");
  if (codeFile == NULL) {
    error("cannot create temporary code file '%s'", codeName);
  }
  dataFile = fopen(dataName, "w+b");
  if (dataFile == NULL) {
    error("cannot create temporary data file '%s'", dataName);
  }
  outFile = fopen(outName, "wb");
  if (outFile == NULL) {
    error("cannot open output file '%s'", outName);
  }
  tmp = dirname(outName);
  strcpy(outpath, tmp);
  strcat(outpath,"/");  
  if(withDebug) {
    strcpy(debName,outName);
    dot = strrchr(debName, '.');
    strcpy(dot, ".deb");
    debFile = fopen(debName, "wt");
    if (debFile == NULL) {
      error("cannot open debug file '%s'", debName);
    }
  }
  if (mapName != NULL) {
    mapFile = fopen(mapName, "wt");
    if (mapFile == NULL) {
      error("cannot open map file '%s'", mapName);
    }
  }
  do {
    inName = argv[i];
    if (*inName == '-') {
      usage(argv[0]);
    }
    inFile = fopen(inName, "rb");
    if (inFile == NULL) {
      error("cannot open input file '%s'", inName);
    }	
    fprintf(stderr, "Reading module '%s'...\n", inName);
    readModule();
    if (inFile != NULL) {
      fclose(inFile);
      inFile = NULL;
    }
  } while (++i < argc);

  void **lines = Set_toArray(defset, NULL);
  
  for (i = 0; lines[i]; i++)   
	  Set_remove(undefset, Atom_string(lines[i])); 	   
  free(lines);  
  lines = Set_toArray(undefset, NULL); 
  
  if(libName) {
      readlibsymbols(libName);      
      printf("Resolving:\r\n");      
	  for (i = 0; lines[i]; i++) {	  
		  Sym = lookup(libsymbolTable, lines[i]);
          if(Sym) {            
            strcpy(destfile, outpath);
            strcat(destfile, Sym->filename);            
          }
          if(Sym && stat(destfile, &buf)!=0) {
			  printf("Extracting %s for %s from archive %s\r\n", Sym->filename, Sym->name, libName);			  
			  Set_remove(undefset, Atom_string(Sym->name));
			  extractfilefromlib(libName, Sym->filename);              
			  inFile = fopen(destfile, "rb");
			  if (inFile == NULL) {
				error("Cannot open input file '%s'", destfile);
			  }	
              fprintf(stderr, "Reading module '%s'...\n", Sym->filename);
              readModule();
			  if (inFile != NULL) {
				fclose(inFile);                
				inFile = NULL;
              }			 
			 FREE(lines);
			 lines = Set_toArray(undefset, NULL);			 
			 i = -1;
          }
      }	  
	  FREE(lines);
  }
  fprintf(stderr, "Linking modules...\n");
  linkSymbols();
  fprintf(stderr, "Relocating segments...\n");
  relocateSegments();
  writeHeader();
  writeCode();
  writeData();
  if (mapFile != NULL) {
    printToMapFile();
  }
  if (debFile != NULL) {
    printToDebFile();
  }
  if (codeFile != NULL) {
    fclose(codeFile);
    codeFile = NULL;
  }
  if (dataFile != NULL) {
    fclose(dataFile);
    dataFile = NULL;
  }
  if (outFile != NULL) {
    fclose(outFile);
    outFile = NULL;
  }
  if (mapFile != NULL) {
    fclose(mapFile);
    mapFile = NULL;
  }
  if (codeName != NULL) {
    unlink(codeName);
  }
  if (dataName != NULL) {
    unlink(dataName);
  }
  return 0;
}
