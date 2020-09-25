/*
 * as.c -- RISCV32 assembler
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <libgen.h> 

#include "../include/a.out.h"
#include "../include/dbg.h"
#include "stab.h"


/**************************************************************/


#define NUM_REGS    32
#define AUX_REG     1

#define LINE_SIZE   600

#define TOK_EOL     0
#define TOK_LABEL   1
#define TOK_IDENT   2
#define TOK_STRING  3
#define TOK_NUMBER  4
#define TOK_IREGISTER   5

#define TOK_PLUS    6
#define TOK_MINUS   7
#define TOK_STAR    8
#define TOK_SLASH   9
#define TOK_PERCENT 10
#define TOK_LSHIFT  11
#define TOK_RSHIFT  12
#define TOK_LPAREN  13
#define TOK_RPAREN  14
#define TOK_COMMA   15
#define TOK_TILDE   16
#define TOK_AMPER   17
#define TOK_BAR     18
#define TOK_CARET   19
#define TOK_DOTRELADR   20
#define TOK_FREGISTER   21
#define TOK_ADD 22


#define STATUS_UNKNOWN  0   /* symbol is not yet defined */
#define STATUS_DEFINED  1   /* symbol is defined */
#define STATUS_GLOBREF  2   /* local entry refers to a global one */

#define GLOBAL_TABLE    0   /* global symbol table identifier */
#define LOCAL_TABLE 1   /* local symbol table identifier */

#define MSB ((unsigned int) 1 << (sizeof(unsigned int) * 8 - 1))


/**************************************************************/



#define OP_ADDF     0x007
#define OP_SUBF     0x047
#define OP_MULF     0x087
#define OP_DIVF     0x0C7
#define OP_SGNJF    0x100
#define OP_SGNJNF   0x101
#define OP_SGNJXF   0x102
#define OP_SQRTF    0x2C7
#define OP_MVSXF    0x780
#define OP_MVXSF    0x700
#define OP_CVTSWF    0x687
#define OP_CVTSWUF    0x68F
#define OP_CVTWSF    0x607
#define OP_CVTWUSF    0x60F
#define OP_EQF      0x502
#define OP_EQF      0x502
#define OP_LTF      0x501
#define OP_LEF      0x500

#define OP_MINF     0x140
#define OP_MAXF     0x141
#define OP_LWF      0x20
#define OP_SWF      0x2

#define OP_ADD      0x000
#define OP_SUB      0x100
#define OP_SLL      0x001
#define OP_SRL      0x005
#define OP_SRA      0x105
#define OP_AND      0x007
#define OP_OR       0x006
#define OP_XOR      0x004
#define OP_BEQ      0x0
#define OP_BNE      0x1
#define OP_SLT      0x2
#define OP_SLTU     0x3
#define OP_BLT      0x4
#define OP_BGT      0x14
#define OP_BGE      0x5
#define OP_BLE      0x15
#define OP_BLTU     0x6
#define OP_BGTU     0x16
#define OP_BGEU     0x7
#define OP_BLEU     0x17
#define OP_ADDI     0x0
#define OP_SLLI         0x1
#define OP_NOP      0x8
#define OP_ANDI     0x7
#define OP_ORI      0x6
#define OP_XORI     0x4
#define OP_SLTI     0x2
#define OP_SLTIU        0x3
//#define OP_SLLI         0x001
#define OP_SRLI         0x005
#define OP_SRAI         0x105
#define OP_CSRW     0x1
#define OP_CSRS     0x2
#define OP_CSRR     0x2
#define OP_CSRWI    0x5
#define OP_CSRSI    0x6
#define OP_CSRCI    0x7
#define OP_LB       0x00
#define OP_LH       0x10
#define OP_LW       0x20
#define OP_LBU      0x40
#define OP_LHU      0x50
#define OP_LA       0x02
#define OP_LUI      0x0
#define OP_SB       0x0
#define OP_SH       0x1
#define OP_SW       0x2
#define OP_JAL      0x0
#define OP_JALR     0x0
#define OP_BRK      0x1
#define OP_MRET     0x302
#define OP_MUL      0x008
#define OP_MULH     0x009
#define OP_MULHSU       0x00A
#define OP_MULHU        0x00B
#define OP_DIV          0x00C
#define OP_DIVU         0x00D
#define OP_REM          0x00E
#define OP_REMU         0x00F
#define OP_CNTCYC       0x00
#define OP_CNTCYCH      0x80
#define OP_CNTTIME      0x01
#define OP_CNTTIMEH     0x81
#define OP_CNTINST      0x02
#define OP_CNTINSTH     0x82

/**************************************************************/


int debugToken = 0;
int debugCode = 0;
int debugFixup = 0;

int debug = 0;
int rvc = 0;

char codeName[L_tmpnam];
char dataName[L_tmpnam];
char srcfileName[L_tmpnam];
char *outName = NULL;
char *inName = NULL;
char outpath[40];
char dbgfile[40];

FILE *codeFile = NULL;
FILE *dataFile = NULL;
FILE *outFile = NULL;
FILE *inFile = NULL;

char line[LINE_SIZE];
char debuglabel[LINE_SIZE];
char funcname[LINE_SIZE];
char tmpstring[LINE_SIZE];
char *lineptr;
int lineno;

int token;
int tokenvalNumber;
char tokenvalString[LINE_SIZE];

int allowSyn = 1;
int currSeg = SEGMENT_CODE;
unsigned int segPtr[4] = { 0, 0, 0, 0 };
char *segName[4] = { "ABS", "CODE", "DATA", "BSS" };
char *methodName[6] = { "W32" , "R12" , "RL12" , "RH20", "RS12","J20" };



typedef struct fixup {
  int segment;          /* in which segment */
  unsigned int offset;      /* at which offset */
  int method;           /* what kind of coding method is to be used */
  int value;            /* known part of value */
  int base;         /* segment which this ref is relative to */
                /* valid only when used for relocation */
  struct fixup *next;       /* next fixup */
} Fixup;

#define symbolmaxnamelen 256


typedef struct symbol {
  char *name;           /* name of symbol */
  int status;           /* status of symbol */
  int segment;          /* the symbol's segment */
  int value;            /* the symbol's value */
  Fixup *fixups;        /* list of locations to fix */
  struct symbol *globref;   /* set if this local refers to a global */
  struct symbol *left;      /* left son in binary search tree */
  struct symbol *right;     /* right son in binary search tree */
  int skip;                 /* this symbol is not defined here nor is */
                                /* it used here: don't write to object file */
  int debug;
  int debugtype;
  int debugvalue;
} Symbol;

loc_t loc;
func_t func;
string_t funckey;
global_t glob;
typdef_t typdef;
funcvar_t funcvar;
root_t root;
m_serial_read_t  in;
m_serial_write_t out;
m_serial_return_code_t ret;
FILE *f;

void mlibinit() {
    loc_init(loc);
    funcvar_init(funcvar);
    func_init(func);
	string_init(funckey);
    global_init(glob);
    typdef_init(typdef);
    root_init(root);
}

void mlibclear() {
    loc_clear(loc);
    funcvar_clear(funcvar);
    func_clear(func);
	string_clear(funckey);
    global_clear(glob);
    typdef_clear(typdef);
    root_clear(root);
} 

int str2int(char *p, unsigned int *num) {
    *num = 0;
    while (isdigit(*p)) {
        if (*p > '9' || *p < '0')
            return -1;
        *num = *num * 10 + *p - '0';
        p++;
    }
    return 0;
}

int insrange(int bits, int val) {
  int msb = 1<<(bits-1);
  int ll = -msb;
  return((val<=(msb-1) && val>=ll) ? 1 : 0);
}

int inurange(int bits, int val) {
  int msb = 1<<bits;
  int ll = 0;
  return((val<=(msb-1) && val>=ll) ? 1 : 0);
}

int rvcreg(int reg) {
 return((reg>=8 && reg<=15)? 1 : 0);
}

unsigned int countbits(int n)
{
    unsigned int count = 0;
    while (n)
    {
      n &= (n-1) ;
      count++;
    }
    return count;
}

typedef struct csrnames {
  char *name;
  unsigned int code;
} Csr;

Csr csrTable[] = {
  /* pseudo instructions */
  { "mstatus",  0x300},
  { "mie",  0x304},
  { "mtvec",  0x305},
  { "mepc",  0x341},
  { "mcause",  0x342},
  { NULL,  0},
};

void error(char *fmt, ...);
unsigned int getcsrcode(char *str) {
    unsigned int i;
    for(i=0;csrTable[i].name;i++) if (strcmp(csrTable[i].name, str)==0) return(csrTable[i].code);
    error("invalid csr name '%s'", str);
}

/**************************************************************/


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
  exit(1);
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


int getNextToken(void) {
  char *p;
  int base;
  long long int digit;

  while (*lineptr == ' ' || *lineptr == '\t') {
    lineptr++;
  }
  if (*lineptr == '\n' || *lineptr == '\0' || *lineptr == ';' || *lineptr == '#' || (*lineptr == '/' && *(lineptr + 1) == '*')) {
    return TOK_EOL;
  }
  if (*lineptr == 'x'){
    lineptr++;
     if (!isdigit((int) *lineptr)) {
       lineptr--;
     }
     else {
       tokenvalNumber = 0;
       while (isdigit((int) *lineptr)) {
         digit = *lineptr++ - '0';
         tokenvalNumber *= 10;
         tokenvalNumber += digit;
       }
       if (tokenvalNumber < 0 || tokenvalNumber >= NUM_REGS) {
         error("illegal register number %d in line %d", tokenvalNumber, lineno);
       }
       return TOK_IREGISTER;
     }
  }
  if (*lineptr == 'f'){
    lineptr++;
     if (!isdigit((int) *lineptr)) {
       lineptr--;
     }
     else {
       tokenvalNumber = 0;
       while (isdigit((int) *lineptr)) {
         digit = *lineptr++ - '0';
         tokenvalNumber *= 10;
         tokenvalNumber += digit;
       }
       if (tokenvalNumber < 0 || tokenvalNumber >= NUM_REGS) {
         error("illegal register number %d in line %d", tokenvalNumber, lineno);
       }
       return TOK_FREGISTER;
     }
  }
  if (*lineptr == '.' && *(lineptr + 1) == '+') {
    lineptr+=2;
    return TOK_DOTRELADR;
  }
  if (isalpha((int) *lineptr) || *lineptr == '_' || *lineptr == '.') {
    p = tokenvalString;
    while (isalnum((int) *lineptr) || *lineptr == '_' || *lineptr == '.') {
      *p++ = *lineptr++;
    }
    *p = '\0';
    if (*lineptr == ':') {
      lineptr++;
      return TOK_LABEL;
    } else {
      return TOK_IDENT;
    }
  }
  if (isdigit((int) *lineptr)) {
    base = 10;
    tokenvalNumber = 0;
    if (*lineptr == '0') {
      lineptr++;
      if (*lineptr == 'x' || *lineptr == 'X') {
        base = 16;
        lineptr++;
      } else
      if (isdigit((int) *lineptr)) {
        base = 8;
      } else {
        return TOK_NUMBER;
      }
    }
    while (isxdigit((int) *lineptr)) {
      digit = *lineptr++ - '0';
      if (digit >= 'A' - '0') {
        if (digit >= 'a' - '0') {
          digit += '0' - 'a' + 10;
        } else {
          digit += '0' - 'A' + 10;
        }
      }
      if (digit >= base) {
        error("illegal digit value %d in line %d", digit, lineno);
      }
      tokenvalNumber *= base;
      tokenvalNumber += digit;
    }
    return TOK_NUMBER;
  }
  if (*lineptr == '\'') {
    lineptr++;
    if (!isprint((int) *lineptr)) {
      error("cannot quote character 0x%02X in line %d", *lineptr, lineno);
    }
    tokenvalNumber = *lineptr;
    lineptr++;
    if (*lineptr != '\'') {
      error("unbalanced quote in line %d", lineno);
    }
    lineptr++;
    return TOK_NUMBER;
  }
  if (*lineptr == '\"') {
    lineptr++;
    p = tokenvalString;
    while (1) {
      if (*lineptr == '\n' || *lineptr == '\0') {
        error("unterminated string constant in line %d", lineno);
      }
      if (!isprint((int) *lineptr)) {
        error("string contains illegal character 0x%02X in line %d",
              *lineptr, lineno);
      }
      if (*lineptr == '\"') {
        break;
      }
      *p++ = *lineptr++;
    }
    lineptr++;
    *p = '\0';
    return TOK_STRING;
  }
  if (*lineptr == '+') {
    lineptr++;
    return TOK_PLUS;
  }
  if (*lineptr == '-') {
    lineptr++;
    return TOK_MINUS;
  }
  if (*lineptr == '*') {
    lineptr++;
    return TOK_STAR;
  }
  if (*lineptr == '/') {
    lineptr++;
    return TOK_SLASH;
  }
  if (*lineptr == '%') {
    lineptr++;
    return TOK_PERCENT;
  }
  if (*lineptr == '<' && *(lineptr + 1) == '<') {
    lineptr += 2;
    return TOK_LSHIFT;
  }
  if (*lineptr == '>' && *(lineptr + 1) == '>') {
    lineptr += 2;
    return TOK_RSHIFT;
  }
  if (*lineptr == '(') {
    lineptr++;
    return TOK_LPAREN;
  }
  if (*lineptr == ')') {
    lineptr++;
    return TOK_RPAREN;
  }
  if (*lineptr == ',') {
    lineptr++;
    return TOK_COMMA;
  }
  if (*lineptr == '~') {
    lineptr++;
    return TOK_TILDE;
  }
  if (*lineptr == '&') {
    lineptr++;
    return TOK_AMPER;
  }
  if (*lineptr == '|') {
    lineptr++;
    return TOK_BAR;
  }
  if (*lineptr == '^') {
    lineptr++;
    return TOK_CARET;
  }
  if (*lineptr == '@') {
    lineptr++;
    return TOK_ADD;
  }
  error("illegal character 0x%02X in line %d", *lineptr, lineno);
  return 0;
}


void showToken(void) {
  printf("DEBUG: ");
  switch (token) {
    case TOK_EOL:
      printf("token = TOK_EOL\n");
      break;
    case TOK_LABEL:
      printf("token = TOK_LABEL, value = %s\n", tokenvalString);
      break;
    case TOK_IDENT:
      printf("token = TOK_IDENT, value = %s\n", tokenvalString);
      break;
    case TOK_STRING:
      printf("token = TOK_STRING, value = %s\n", tokenvalString);
      break;
    case TOK_NUMBER:
      printf("token = TOK_NUMBER, value = 0x%x\n", tokenvalNumber);
      break;
    case TOK_IREGISTER:
      printf("token = TOK_IREGISTER, value = %d\n", tokenvalNumber);
      break;
     case TOK_FREGISTER:
      printf("token = TOK_FREGISTER, value = %d\n", tokenvalNumber);
      break;
    case TOK_PLUS:
      printf("token = TOK_PLUS\n");
      break;
    case TOK_MINUS:
      printf("token = TOK_MINUS\n");
      break;
    case TOK_STAR:
      printf("token = TOK_STAR\n");
      break;
    case TOK_SLASH:
      printf("token = TOK_SLASH\n");
      break;
    case TOK_PERCENT:
      printf("token = TOK_PERCENT\n");
      break;
    case TOK_LSHIFT:
      printf("token = TOK_LSHIFT\n");
      break;
    case TOK_RSHIFT:
      printf("token = TOK_RSHIFT\n");
      break;
    case TOK_LPAREN:
      printf("token = TOK_LPAREN\n");
      break;
    case TOK_RPAREN:
      printf("token = TOK_RPAREN\n");
      break;
    case TOK_COMMA:
      printf("token = TOK_COMMA\n");
      break;
    case TOK_TILDE:
      printf("token = TOK_TILDE\n");
      break;
    case TOK_AMPER:
      printf("token = TOK_AMPER\n");
      break;
    case TOK_BAR:
      printf("token = TOK_BAR\n");
      break;
    case TOK_CARET:
      printf("token = TOK_CARET\n");
      break;
    default:
      error("illegal token %d in showToken()", token);
  }
}


void getToken(void) {
  token = getNextToken();
  if (debugToken) {
    showToken();
  }
}


static char *tok2str[] = {
  "end-of-line",
  "label",
  "identifier",
  "string",
  "number",
  "register",
  "+",
  "-",
  "*",
  "/",
  "%",
  "<<",
  ">>",
  "(",
  ")",
  ",",
  "~",
  "&",
  "|",
  "^"
};


void expect(int expected) {
  if (token != expected) {
    error("'%s' expected, got '%s' in line %d",
          tok2str[expected], tok2str[token], lineno);
  }
}


/**************************************************************/


Fixup *fixupList = NULL;


Fixup *newFixup(int segment, unsigned int offset, int method, int value) {
  Fixup *f;

  f = allocateMemory(sizeof(Fixup));
  f->segment = segment;
  f->offset = offset;
  f->method = method;
  f->value = value;
  f->base = 0;
  f->next = NULL;
  return f;
}


void addFixup(Symbol *s,
              int segment, unsigned int offset, int method, int value) {
  Fixup *f;

  if (debugFixup) {
    printf("DEBUG: fixup (s:%s, o:%08X, m:%s, v:%08X) added to '%s'\n",
           segName[segment], offset, methodName[method], value, s->name);
  }
  f = newFixup(segment, offset, method, value);
  f->next = s->fixups;
  s->fixups = f;
}


/**************************************************************/


Symbol *globalTable = NULL;
Symbol *localTable = NULL;


Symbol *deref(Symbol *s) {
  if (s->status == STATUS_GLOBREF) {
    return s->globref;
  } else {
    return s;
  }
}


Symbol *newSymbol(char *name,int debug) {
  Symbol *p;

  p = allocateMemory(sizeof(Symbol));
  if(debug)
  p->name = allocateMemory(strlen(name) + symbolmaxnamelen);
  else
  p->name = allocateMemory(strlen(name) + 1);
  strcpy(p->name, name);
  p->status = STATUS_UNKNOWN;
  p->segment = 0;
  p->value = 0;
  p->fixups = NULL;
  p->globref = NULL;
  p->left = NULL;
  p->right = NULL;
  p->debug = debug;
  p->debugtype = 0;
  p->debugvalue = 0;
  return p;
}


Symbol *lookupEnter(char *name, int whichTable, int debug) {
  Symbol *p, *q, *r;
  int cmp;

  if (whichTable == GLOBAL_TABLE) {
    p = globalTable;
  } else {
    p = localTable;
  }
  if (p == NULL) {
    r = newSymbol(name, debug);
    if (whichTable == GLOBAL_TABLE) {
      globalTable = r;
    } else {
      localTable = r;
    }
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
      r = newSymbol(name, debug);
      if (cmp < 0) {
        q->left = r;
      } else {
        q->right = r;
      }
      return r;
    }
  }
}


static void linkSymbol(Symbol *s) {
  Fixup *f;
  Symbol *global;

  if (s->status == STATUS_UNKNOWN) {
    global = lookupEnter(s->name, GLOBAL_TABLE, 0);
    while (s->fixups != NULL) {
      f = s->fixups;
      s->fixups = f->next;
      f->next = global->fixups;
      global->fixups = f;
    }
    s->status = STATUS_GLOBREF;
    s->globref = global;
  }
  if (s->status == STATUS_GLOBREF) {
    if (s->fixups != NULL) {
      error("local fixups detected with global symbol '%s'", s->name);
    }
  } else {
    if (debugFixup) {
      printf("DEBUG: link '%s' (s:%s, v:%08X)\n",
             s->name, segName[s->segment], s->value);
    }
    while (s->fixups != NULL) {
      /* get next fixup record */
      f = s->fixups;
      s->fixups = f->next;
      /* add the symbol's value to the value in the record */
      /* and remember the symbol's segment */
      if (debugFixup) {
        printf("       (s:%s, o:%08X, m:%s, v:%08X --> %08X, b:%s)\n",
               segName[f->segment], f->offset, methodName[f->method],
               f->value, f->value + s->value, segName[s->segment]);
      }
      f->value += s->value;
      f->base = s->segment;
      /* transfer the record to the fixup list */
      f->next = fixupList;
      fixupList = f;
    }
  }
}


static void linkTree(Symbol *s) {
  if (s == NULL) {
    return;
  }
  linkTree(s->left);
  linkSymbol(s);
  linkTree(s->right);
  freeMemory(s->name);
  freeMemory(s);
}


void linkLocals(void) {
  linkTree(localTable);
  localTable = NULL;
  fseek(codeFile, 0, SEEK_END);
  fseek(dataFile, 0, SEEK_END);
}


/**************************************************************/


void emitByte(unsigned int byte) {
  byte &= 0x000000FF;
  if (debugCode) {
    printf("DEBUG: byte @ segment = %s, offset = %08X",
           segName[currSeg], segPtr[currSeg]);
    printf(", value = %02X\n", byte);
  }
  switch (currSeg) {
    case SEGMENT_ABS:
      error("illegal segment in emitByte()");
      break;
    case SEGMENT_CODE:
      fputc(byte, codeFile);
      break;
    case SEGMENT_DATA:
      fputc(byte, dataFile);
      break;
    case SEGMENT_BSS:
      break;
  }
  segPtr[currSeg] += 1;
}


void emitHalf(unsigned int half) {
  half &= 0x0000FFFF;
  if (debugCode) {
    printf("DEBUG: half @ segment = %s, offset = %08X",
           segName[currSeg], segPtr[currSeg]);
    printf(", value = %02X%02X\n",
           (half >> 8) & 0xFF, half & 0xFF);
  }
  switch (currSeg) {
    case SEGMENT_ABS:
      error("illegal segment in emitHalf()");
      break;
    case SEGMENT_CODE:
      fputc(half & 0xFF, codeFile);
      fputc((half >> 8) & 0xFF, codeFile);
      break;
    case SEGMENT_DATA:
      fputc(half & 0xFF, dataFile);
      fputc((half >> 8) & 0xFF, dataFile);
      break;
    case SEGMENT_BSS:
      break;
  }
  segPtr[currSeg] += 2;
}


void emitWord(unsigned int word) {
  if (debugCode) {
    printf("DEBUG: word @ segment = %s, offset = %08X",
           segName[currSeg], segPtr[currSeg]);
    printf(", value = %02X%02X%02X%02X\n",
           (word >> 24) & 0xFF, (word >> 16) & 0xFF,
           (word >> 8) & 0xFF, word & 0xFF);
  }
  switch (currSeg) {
    case SEGMENT_ABS:
      error("illegal segment in emitWord()");
      break;
    case SEGMENT_CODE:
      fputc(word & 0xFF, codeFile);
      fputc((word >> 8) & 0xFF, codeFile);
      fputc((word >> 16) & 0xFF, codeFile);
      fputc((word >> 24) & 0xFF, codeFile);
      break;
    case SEGMENT_DATA:
      fputc(word & 0xFF, dataFile);
      fputc((word >> 8) & 0xFF, dataFile);
      fputc((word >> 16) & 0xFF, dataFile);
      fputc((word >> 24) & 0xFF, dataFile);
      break;
    case SEGMENT_BSS:
      break;
  }
  segPtr[currSeg] += 4;
}


/**************************************************************/


typedef struct {
  int con;
  long long int lcon;
  Symbol *sym;
} Value;


Value parseExpression(void);


Value parsePrimaryExpression(void) {
  Value v;
  Symbol *s;

  if (token == TOK_NUMBER) {
    v.con = tokenvalNumber & 0xFFFFFFFF;
    v.lcon = tokenvalNumber;
    v.sym = NULL;
    getToken();
  } else
  if (token == TOK_IDENT) {
    s = deref(lookupEnter(tokenvalString, LOCAL_TABLE, 0));
    if (s->status == STATUS_DEFINED && s->segment == SEGMENT_ABS) {
      v.con = s->value;
      v.sym = NULL;
    } else {
      v.con = 0;
      v.sym = s;
    }
    getToken();
  } else
  if (token == TOK_LPAREN) {
    getToken();
    v = parseExpression();
    expect(TOK_RPAREN);
    getToken();
  } else {
    error("illegal primary expression, line %d", lineno);
  }
  return v;
}


Value parseUnaryExpression(void) {
  Value v;

  if (token == TOK_PLUS) {
    getToken();
    v = parseUnaryExpression();
  } else
  if (token == TOK_MINUS) {
    getToken();
    v = parseUnaryExpression();
    if (v.sym != NULL) {
      error("cannot negate symbol '%s' in line %d", v.sym->name, lineno);
    }
    v.con = -v.con;
  } else
  if (token == TOK_TILDE) {
    getToken();
    v = parseUnaryExpression();
    if (v.sym != NULL) {
      error("cannot complement symbol '%s' in line %d", v.sym->name, lineno);
    }
    v.con = ~v.con;
  } else {
    v = parsePrimaryExpression();
  }
  return v;
}


Value parseMultiplicativeExpression(void) {
  Value v1, v2;

  v1 = parseUnaryExpression();
  while (token == TOK_STAR || token == TOK_SLASH || token == TOK_PERCENT) {
    if (token == TOK_STAR) {
      getToken();
      v2 = parseUnaryExpression();
      if (v1.sym != NULL || v2.sym != NULL) {
        error("multiplication of symbols not supported, line %d", lineno);
      }
      v1.con *= v2.con;
    } else
    if (token == TOK_SLASH) {
      getToken();
      v2 = parseUnaryExpression();
      if (v1.sym != NULL || v2.sym != NULL) {
        error("division of symbols not supported, line %d", lineno);
      }
      if (v2.con == 0) {
        error("division by zero, line %d", lineno);
      }
      v1.con /= v2.con;
    } else
    if (token == TOK_PERCENT) {
      getToken();
      v2 = parseUnaryExpression();
      if (v1.sym != NULL || v2.sym != NULL) {
        error("division of symbols not supported, line %d", lineno);
      }
      if (v2.con == 0) {
        error("division by zero, line %d", lineno);
      }
      v1.con %= v2.con;
    }
  }
  return v1;
}


Value parseAdditiveExpression(void) {
  Value v1, v2;

  v1 = parseMultiplicativeExpression();
  while (token == TOK_PLUS || token == TOK_MINUS) {
    if (token == TOK_PLUS) {
      getToken();
      v2 = parseMultiplicativeExpression();
      if (v1.sym != NULL && v2.sym != NULL) {
        error("addition of symbols not supported, line %d", lineno);
      }
      if (v2.sym != NULL) {
        v1.sym = v2.sym;
      }
      v1.con += v2.con;
    } else
    if (token == TOK_MINUS) {
      getToken();
      v2 = parseMultiplicativeExpression();
      if (v2.sym != NULL) {
        printf("subtraction of symbols not supported, line %d\r\n", lineno);
        v2.con = 0;
      }
      v1.con -= v2.con;
    }
  }
  return v1;
}


Value parseShiftExpression(void) {
  Value v1, v2;

  v1 = parseAdditiveExpression();
  while (token == TOK_LSHIFT || token == TOK_RSHIFT) {
    if (token == TOK_LSHIFT) {
      getToken();
      v2 = parseAdditiveExpression();
      if (v1.sym != NULL || v2.sym != NULL) {
        error("shifting of symbols not supported, line %d", lineno);
      }
      v1.con <<= v2.con;
    } else
    if (token == TOK_RSHIFT) {
      getToken();
      v2 = parseAdditiveExpression();
      if (v1.sym != NULL || v2.sym != NULL) {
        error("shifting of symbols not supported, line %d", lineno);
      }
      v1.con >>= v2.con;
    }
  }
  return v1;
}


Value parseAndExpression(void) {
  Value v1, v2;

  v1 = parseShiftExpression();
  while (token == TOK_AMPER) {
    getToken();
    v2 = parseShiftExpression();
    if (v2.sym != NULL) {
      error("bitwise 'and' of symbols not supported, line %d", lineno);
    }
    v1.con &= v2.con;
  }
  return v1;
}


Value parseExclusiveOrExpression(void) {
  Value v1, v2;

  v1 = parseAndExpression();
  while (token == TOK_CARET) {
    getToken();
    v2 = parseAndExpression();
    if (v2.sym != NULL) {
      error("bitwise 'xor' of symbols not supported, line %d", lineno);
    }
    v1.con ^= v2.con;
  }
  return v1;
}


Value parseInclusiveOrExpression(void) {
  Value v1, v2;

  v1 = parseExclusiveOrExpression();
  while (token == TOK_BAR) {
    getToken();
    v2 = parseExclusiveOrExpression();
    if (v2.sym != NULL) {
      error("bitwise 'or' of symbols not supported, line %d", lineno);
    }
    v1.con |= v2.con;
  }
  return v1;
}


Value parseExpression(void) {
  Value v;

  v = parseInclusiveOrExpression();
  return v;
}


/**************************************************************/


void dotSyn(unsigned int code) {
  allowSyn = 1;
}


void dotNosyn(unsigned int code) {
  allowSyn = 0;
}


void dotCode(unsigned int code) {
  currSeg = SEGMENT_CODE;
}


void dotData(unsigned int code) {
  currSeg = SEGMENT_DATA;
  while(token != TOK_EOL) getToken();
}


void dotBss(unsigned int code) {
  currSeg = SEGMENT_BSS;
}

void Ignore(unsigned int code) {
     while(token != TOK_EOL) getToken();
}

void dotStabs(unsigned int code) {
  Value v;
  unsigned int i, typenum, semicol;
  char *ptr, *def, *name;
  char ch;
  Symbol *label, *local;
  expect(TOK_STRING);
  strcpy(tmpstring,tokenvalString);
  getToken();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  semicol = strcspn(tmpstring, ":");
  name = strndup(tmpstring, semicol);  
  def = strchr(tmpstring, '=');
  if(def) {
    str2int(tmpstring + semicol + 2, &typenum);    
    string_set_str(typdef->name, name);
    typdef->number = typenum;
    string_set_str(typdef->desc, def);
    typdefarr_push_back(root->typdefs, typdef); 
    
  } else {
  }
  expect(TOK_COMMA);
  getToken();
  parseExpression();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  expect(TOK_COMMA);
  getToken();
  if (token == TOK_IDENT) {
     getToken();
  } else {
    v = parseExpression();
  }
  if(!def) {
   ptr=strchr(tmpstring, ':');
   ch=*(ptr+1);
  switch(ch) {
  case 'G': sprintf(debuglabel, "%s ", name);
            label = deref(lookupEnter(debuglabel, GLOBAL_TABLE, 1));
            local = deref(lookupEnter(name, LOCAL_TABLE, 0));
            label->status = STATUS_DEFINED;
            label->segment = currSeg; 
            label->value = local->value;
            label->debug = DBG_VARGLO;
            ptr+=2;
            label->debugvalue = 0;
            str2int(ptr, &i);
            sprintf(label->name+strlen(label->name),"%d",i); 
            label->debugtype = i;
            break; 
  case 'f': return;
            break;
  case 'F': sprintf(debuglabel,"%s beg",name);
            strcpy(funcname,name);
            label = deref(lookupEnter(debuglabel, GLOBAL_TABLE, 1));
            label->status = STATUS_DEFINED;
            label->segment = currSeg;
            label->value = segPtr[currSeg];            
            label->debug = DBG_FUNCBEG;
            ptr+=2;            
            str2int(ptr, &i);
            label->debugtype = i;
            string_set_str(funckey, name);
            func->rettype = i;
            return;
            break;    
 case 'E':  funcdict_set_at(root->functions, funckey, func);
            return;
            break;  
 case 'P':  ptr+=2;
            str2int(ptr, &i);
            string_set_str(funcvar->name, name);
            funcvar->type = i;
            funcvar->pos = v.con;
            funcvararr_push_back(func->regargs, funcvar);
            break;
  case 'r': ptr+=2;
            str2int(ptr, &i);
            string_set_str(funcvar->name, name);
            funcvar->type = i;
            funcvar->pos = v.con;
            funcvararr_push_back(func->reglocals, funcvar);
            break;
  case 'p': ptr+=2;
            str2int(ptr, &i);
            string_set_str(funcvar->name, name);
            funcvar->type = i;
            funcvar->pos = v.con;
            funcvararr_push_back(func->stackargs, funcvar);
            break;
  default:  ptr++;
            str2int(ptr, &i);
            string_set_str(funcvar->name, name);
            funcvar->type = i;
            funcvar->pos = v.con;
            funcvararr_push_back(func->stacklocals, funcvar);
            break;
  }
 }
}

void dotStabn(unsigned int code) {
  parseExpression();
  expect(TOK_COMMA);
  getToken();
  parseExpression();
  expect(TOK_COMMA);
  getToken();
  parseExpression();
  expect(TOK_COMMA);
  getToken();
  if (token == TOK_IDENT) {
  getToken();
  } else {
  parseExpression();
  }
}

void dotFile(unsigned int code) {
  Value v;
  v = parseExpression();
  if (v.sym != NULL) {
    error("absolute expression expected in line %d", lineno);
  }
  expect(TOK_STRING);
  strcpy(srcfileName,tokenvalString);
  getToken();
}

void dotLoc(unsigned int code) {
  Value v, w;
  Symbol* label;
  v = parseExpression();
  if (v.sym != NULL) {
    error("absolute expression expected in line %d", lineno);
  }
  v = parseExpression();
  if (v.sym != NULL) {
    error("absolute expression expected in line %d", lineno);
  }
  if(token != TOK_EOL) {
      w = parseExpression();
    if (w.sym != NULL) {
        error("absolute expression expected in line %d", lineno);
    }
  }
  if(debug) {
    sprintf(debuglabel,"\"%s\":%d",srcfileName,v.con);
    label=deref(lookupEnter(debuglabel, GLOBAL_TABLE, 0));
    label->status = STATUS_DEFINED;
    label->segment = currSeg;
    label->value = segPtr[currSeg];
    label->debug = DBG_LINE;
  }
}

void dotExport(unsigned int code) {
  Symbol *global;
  Symbol *local;
  Fixup *f;

  while (1) {
    expect(TOK_IDENT);
    global = lookupEnter(tokenvalString, GLOBAL_TABLE, 0);
    if (global->status != STATUS_UNKNOWN) {
      error("exported symbol '%s' multiply defined in line %d",
            global->name, lineno);
    }
    local = lookupEnter(tokenvalString, LOCAL_TABLE, 0);
    if (local->status == STATUS_GLOBREF) {
      error("exported symbol '%s' multiply exported in line %d",
            local->name, lineno);
    }
    global->status = local->status;
    global->segment = local->segment;
    global->value = local->value;
    while (local->fixups != NULL) {
      f = local->fixups;
      local->fixups = f->next;
      f->next = global->fixups;
      global->fixups = f;
    }
    local->status = STATUS_GLOBREF;
    local->globref = global;
    getToken();
    if (token != TOK_COMMA) {
      break;
    }
    getToken();
  }
}

void dotSet(unsigned int code) {
  Symbol *global;
  Symbol *globalnew;

 
    expect(TOK_IDENT);
    globalnew = lookupEnter(tokenvalString, GLOBAL_TABLE, 0);
    if (globalnew->status == STATUS_DEFINED) {
      error("'%s' already not defined, line %d",
            globalnew->name, lineno);
    }
    getToken();
    expect(TOK_COMMA);
    getToken();
    expect(TOK_IDENT);
    global = lookupEnter(tokenvalString, GLOBAL_TABLE, 0);
    if (global->status != STATUS_DEFINED) {
      error(".set '%s' not defined, line %d",
            global->name, lineno);
    }
    globalnew->segment = global->segment;
    globalnew->value = global->value;
    getToken();
}

void dotImport(unsigned int code) {
  Symbol *global;
  Symbol *local;
  Fixup *f;

  while (1) {
    expect(TOK_IDENT);
    global = lookupEnter(tokenvalString, GLOBAL_TABLE, 0);
    local = lookupEnter(tokenvalString, LOCAL_TABLE, 0);
    if (local->status != STATUS_UNKNOWN) {
      error("imported symbol '%s' multiply defined in line %d",
            local->name, lineno);
    }
    while (local->fixups != NULL) {
      f = local->fixups;
      local->fixups = f->next;
      f->next = global->fixups;
      global->fixups = f;
    }
    local->status = STATUS_GLOBREF;
    local->globref = global;
    getToken();
    if (token != TOK_COMMA) {
      break;
    }
    getToken();
  }
}


int countBits(unsigned int x) {
  int n;

  n = 0;
  while (x != 0) {
    x &= x - 1;
    n++;
  }
  return n;
}


void dotAlign(unsigned int code) {
  Value v;
  unsigned int mask;

  v = parseExpression();
  if (v.sym != NULL) {
    error("absolute expression expected in line %d", lineno);
  }
  mask = (1<<v.con) - 1;
  while ((segPtr[currSeg] & mask) != 0) {
    if(mask&1) {
      emitByte(0x0);
    } else {
      if(mask&2){
        emitHalf(0x1);
      } else {
        emitWord(0x13);
      }
    }
  }
  while(token != TOK_EOL) getToken();
}


void dotSpace(unsigned int code) {
  Value v;
  int i;

  v = parseExpression();
  if (v.sym != NULL) {
    error("absolute expression expected in line %d", lineno);
  }
  for (i = 0; i < v.con; i++) {
    emitByte(0);
  }
}


void dotLocate(unsigned int code) {
  Value v;

  v = parseExpression();
  if (v.sym != NULL) {
    error("absolute expression expected in line %d", lineno);
  }
  while (segPtr[currSeg] != v.con) {
    emitByte(0);
  }
}


void dotByte(unsigned int code) {
  Value v;
  char *p;

  while (1) {
    if (token == TOK_STRING) {
      p = tokenvalString;
      while (*p != '\0') {
        emitByte(*p);
        p++;
      }
      getToken();
    } else {
      v = parseExpression();
      if (v.sym != NULL) {
        error("absolute expression expected in line %d", lineno);
      }
      emitByte(v.con);
    }
    if (token != TOK_COMMA) {
      break;
    }
    getToken();
  }
}



void dotHalf(unsigned int code) {
  Value v;

  while (1) {
    v = parseExpression();
    if (v.sym != NULL) {
      error("absolute expression expected in line %d", lineno);
    }
    emitHalf(v.con);
    if (token != TOK_COMMA) {
      break;
    }
    getToken();
  }
}


void dotWord(unsigned int code) {
  Value v;

  while (1) {
    v = parseExpression();
    if (v.sym == NULL) {
      emitWord(v.con);
    } else {
      addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_W32, v.con);
      emitWord(0);
    }
    if (token != TOK_COMMA) {
      break;
    }
    getToken();
  }
}


void dotDword(unsigned int code) {
  Value v;

  while (1) {
    v = parseExpression();
    emitWord(v.lcon&0xFFFFFFFF);
    emitWord((v.lcon>>32)&0xFFFFFFFF);   
    if (token != TOK_COMMA) {
      break;
    }
    getToken();
  }
}

void dotSet2(unsigned int code) {
  Value v;
  Symbol *symbol;

  expect(TOK_IDENT);
  symbol = deref(lookupEnter(tokenvalString, LOCAL_TABLE, 0));
  if (symbol->status != STATUS_UNKNOWN) {
    error("symbol '%s' multiply defined in line %d",
          symbol->name, lineno);
  }
  getToken();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  if (v.sym == NULL) {
    symbol->status = STATUS_DEFINED;
    symbol->segment = SEGMENT_ABS;
    symbol->value = v.con;
  } else {
    error("illegal type of symbol '%s' in expression, line %d",
          v.sym->name, lineno);
  }
}





void formatR(unsigned int code) {
  int dst, src1, src2;

  /* opcode with three register operands */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_IREGISTER);
  src2 = tokenvalNumber;
  getToken();
  emitWord(((code>>3)&0x7F) << 25 | src2 << 20 | src1 << 15 | (code&0x7)<<12 | dst<<7 | 0x33);
}

void formatRF(unsigned int code) {
  int dst, src1, src2;

  /* opcode with three register operands */
  expect(TOK_FREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_FREGISTER);
  src1 = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_FREGISTER);
  src2 = tokenvalNumber;
  getToken();
  emitWord(((code>>4)&0x7F) << 25 | src2 << 20 | src1 << 15 | (code&7)<<12 | dst<<7 | 0x53);
}

void formatNF(unsigned int code) {
  int dst, src1;

  /* opcode with two register operands */
  expect(TOK_FREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_FREGISTER);
  src1 = tokenvalNumber;
  getToken();
  emitWord(((code>>4)&0x7F) << 25 | src1 << 20 | src1 << 15 | (code&7)<<12 | dst<<7 | 0x53);
}

void formatMF(unsigned int code) {
  int dst, src1;

  /* opcode with three register operands */
  expect(TOK_FREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_FREGISTER);
  src1 = tokenvalNumber;
  getToken();
  emitWord(((code>>4)&0x7F) << 25 | src1 << 20 | src1 << 15 | (code&7)<<12 | dst<<7 | 0x53);
}

void formatRF2(unsigned int code) {
  int dst, src1;

  /* opcode with three register operands */
  expect(TOK_FREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_FREGISTER);
  src1 = tokenvalNumber;
  getToken();
  emitWord(((code>>4)&0x7F) << 25 | src1 << 15 | (code&7)<<12 | dst<<7 | 0x53);
}

void formatRXF(unsigned int code) {
  int dst, src1;

  /* opcode with three register operands */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_FREGISTER);
  src1 = tokenvalNumber;
  getToken();
  emitWord(((code>>4)&0x7F) << 25 | ((code>>3)&1)<<20 | src1 << 15 | (code&7)<<12 | dst<<7 | 0x53);
}

void formatRFX(unsigned int code) {
  int dst, src1;

  /* opcode with three register operands */
  expect(TOK_FREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken();
  emitWord(((code>>4)&0x7F) << 25 | ((code>>3)&1)<<20 | src1 << 15 | (code&7)<<12 | dst<<7 | 0x53);
}

void formatRCMPF(unsigned int code) {
  int dst, src1, src2;

  /* opcode with three register operands */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_FREGISTER);
  src1 = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_FREGISTER);
  src2 = tokenvalNumber;
  getToken();
  emitWord(((code>>4)&0x7F) << 25 | src2 << 20 | src1 << 15 | (code&0x7)<<12 | dst<<7 | 0x53);
}

void formatR2(unsigned int code) {
  int dst, src1, src2;

  /* opcode with three register operands */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_IREGISTER);
  src2 = tokenvalNumber;
  getToken();
  if(rvc==0 || (rvcreg(dst)==0) || (src2>15) || (src2<8) || src1!=dst)
  emitWord(((code>>3)&0x7F) << 25 | src2 << 20 | src1 << 15 | (code&0x7)<<12 | dst<<7 | 0x33);
  else
  emitHalf(4<<13 | 3<<10 | (dst-8)<<7 | countbits(code&0x7)<<5 | (src2-8)<<2 | 0x1);
}

void formatRC2(unsigned int code) {
  int dst, src2;

  /* opcode with three register operands */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_IREGISTER);
  src2 = tokenvalNumber;
  getToken();
  emitHalf(4<<13 | 3<<10 | (dst-8)<<7 | countbits(code&0x7)<<5 | (src2-8)<<2 | 0x1);
}

void formatSB(unsigned int code) {
  int src1, src2, tmp;
  Value v;
  unsigned int immed;

  /* opcode with two registers and a 12 bit signed offset operand */
  expect(TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_IREGISTER);
  src2 = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  if(token == TOK_DOTRELADR) getToken();
  v = parseExpression();
  if (v.sym == NULL) {
    immed = v.con / 2;
  } else {
    addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_R12, v.con);
    immed = 0;
  }
  if(code&0x10) {
    tmp=src2;
    src2=src1;
    src1=tmp;
    code&=0xF;
  }
  emitWord(((immed>>11)&1)<<31 |((immed>>4)&0x3F)<<25 | src2<<20 | src1<<15 |(code&0x7)<<12| (immed&0xF)<<8 | ((immed>>10)&1)<<7 | 0x63); 
}

void formatS(unsigned int code) {
  int src1, src2, vcon;
  Value v;
  unsigned int immed;
  int rvccond;

  /* opcode with two registers and a 12 bit signed offset operand */
  expect(TOK_IREGISTER);
  src2 = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  if (v.sym == NULL) {
    immed = v.con;
    vcon = v.con;
    expect(TOK_LPAREN);
    getToken();
    expect(TOK_IREGISTER);
    src1 = tokenvalNumber;
    getToken();
    expect(TOK_RPAREN);
    getToken();
  } else {
    expect(TOK_COMMA);
    getToken();
    expect(TOK_IREGISTER);
    src1 = tokenvalNumber;
    getToken();
    addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RH20, v.con);
    immed = 0;
    emitWord( immed | src1<<7 | 0x17);
    immed = v.con;
    addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RS12, v.con);
    immed = 0;
  }
  rvccond = (rvc!=0) && (v.sym==NULL) && (code==OP_SW);
  if (rvccond && (rvcreg(src2)==1) && (rvcreg(src1)==1) && (inurange(7, vcon)==1) )
    emitHalf(6<<13 | ((immed>>3)&0x7)<<10 | (src1-8)<<7 | ((immed>>2)&0x1)<<6 | ((immed>>6)&0x1)<<5 | (src2-8)<<2 | 0x0);
  else if(rvccond && src1==2 && (inurange(8, vcon)==1))
    emitHalf(6<<13 | ((immed>>2)&0xF)<<9 | ((immed>>6)&0x3)<<7 | src2<<2 | 0x2);
  else
   emitWord(((immed>>5)&0x7F)<<25 | src2<<20 | src1<<15 |(code&0x7)<<12| (immed&0x1F)<<7 | 0x23);
}

void formatSF(unsigned int code) {
  int src1, src2;
  Value v;
  unsigned int immed;
  

  /* opcode with two registers and a 12 bit signed offset operand */
  expect(TOK_FREGISTER);
  src2 = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  if (v.sym == NULL) {
    immed = v.con;    
    expect(TOK_LPAREN);
    getToken();
    expect(TOK_IREGISTER);
    src1 = tokenvalNumber;
    getToken();
    expect(TOK_RPAREN);
    getToken();
  } else {
    expect(TOK_COMMA);
    getToken();
    expect(TOK_IREGISTER);
    src1 = tokenvalNumber;
    getToken();
    addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RH20, v.con);
    immed = 0;
    emitWord( immed | src1<<7 | 0x17);
    immed = v.con;
    addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RS12, v.con);
    immed = 0;
  }
   emitWord(((immed>>5)&0x7F)<<25 | src2<<20 | src1<<15 |(code&0x7)<<12| (immed&0x1F)<<7 | 0x27);
}


void formatIm(unsigned int code) {
  int dst, src1;
  Value v;
  unsigned int immed;

  /* opcode with two register operands and immediate */
  if((code&0x8)==0) {
    expect(TOK_IREGISTER);
    dst = tokenvalNumber;
    getToken();
    expect(TOK_COMMA);
    getToken();
    expect(TOK_IREGISTER);
    src1 = tokenvalNumber;
    getToken();
    expect(TOK_COMMA);
    getToken();
    v = parseExpression();
    immed = v.con;
    immed &= 0xFFF;
  } else {
    immed = 0;
    src1 = 0;
    dst = 0;
    v.con = 0;
  }  
  if(rvc==1 && (insrange(6,v.con)!=0) && src1==dst)
    emitHalf(((immed>>5)&0x1)<<12 | dst<<7 | (immed&0x1f)<<2 | (0x1+(code&0x7)));
  else if(rvc==1 && (rvcreg(dst)==1) && (v.con>0 && v.con<=((1<<10)-1))  && ((immed & 0x3) ==0)&& src1==2 && code==0) {   
   emitHalf( ((immed>>4)&0x3)<<11  | ((immed>>6)&0xf)<<7  | ((immed>>2)&0x1)<<6 | ((immed>>3)&0x1)<<5 | (dst-8)<<2 | 0);}
  else if(rvc==1 && dst==2 && src1==2 && (insrange(10, v.con)==1) && ((immed&0xf)==0) && code==0)
    emitHalf(3<<13 | ((immed>>9)&0x1)<<12 | 2<<7 | ((immed>>4)&0x1)<<6   | ((immed>>6)&0x1)<<5  | ((immed>>7)&0x3)<<3 | ((immed>>5)&0x1)<<2 | 1);
  else
    emitWord( immed<<20 | src1 << 15 | (code&0x7)<<12 | dst<<7 | 0x13);
}

void formatCsrr(unsigned int code) {
  int dst;
  char *p;
  unsigned int immed;

  /* opcode with register operands and csr */
    expect(TOK_IREGISTER);
    dst = tokenvalNumber;
    getToken();
    expect(TOK_COMMA);
    getToken();
    expect(TOK_IDENT);
    p = tokenvalString;
    getToken(); 
    immed = getcsrcode(p);
    emitWord( immed<<20 | 0 << 15 | 2<<12 | dst<<7 | 0x73);
}

void formatCsrw(unsigned int code) {
  int src1;
  char *p;
  unsigned int immed;

  /* opcode with register operands and csr */
    expect(TOK_IDENT);
    p = tokenvalString;
    getToken(); 
    immed = getcsrcode(p);
    expect(TOK_COMMA);
    getToken();
    expect(TOK_IREGISTER);
    src1 = tokenvalNumber;
    getToken();
    emitWord( immed<<20 | src1 << 15 | code<<12 | 0<<7 | 0x73);
}

void formatCsrwi(unsigned int code) {
  Value v;
  char *p;
  unsigned int immed;

  /* opcode with register operands and csr */
    expect(TOK_IDENT);
    p = tokenvalString;
    getToken(); 
    immed = getcsrcode(p);
    expect(TOK_COMMA);
    getToken();
    v = parseExpression();
    emitWord( immed<<20 | v.con << 15 | code<<12 | 0<<7 | 0x73);
}

void formatMv(unsigned int code) {
  int dst, src1;
  
  /* opcode with two registers */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken();
  
  emitWord( src1 << 15 | (code&0x7)<<12 | dst<<7 | 0x13);
}


void formatCIm(unsigned int code) {
  int dst;
  Value v;
  unsigned int immed;

  /* opcode with two register operands and immediate */
  if((code&0x8)==0) {
    expect(TOK_IREGISTER);
    dst = tokenvalNumber;
    getToken();
    expect(TOK_COMMA);
    getToken();
    v = parseExpression();
    immed = v.con;
    immed &= 0x3F;
  } else {
    immed = 0;
    dst = 0;
  }
    emitHalf(((immed>>5)&0x1)<<12 | dst<<7 | (immed&0x1f)<<2 | (0x1+(code&1)));
}

void formatIm2(unsigned int code) {
  int dst, src1;
  Value v;
  unsigned int immed;

  /* opcode with two register operands and immediate */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  immed = v.con;
  immed &= 0xFFF;

  if(rvc==0 || !insrange(6, v.con) || src1!=dst || !rvcreg(src1))
    emitWord( immed<<20 | src1 << 15 | (code&0x7)<<12 | dst<<7 | 0x13);
  else    
    emitHalf(4<<13 | ((immed>>5)&0x1)<<12 |  2<<10 | (dst-8)<<7 | (immed&0x1f)<<2 | 0x1);
}

void formatCIm2(unsigned int code) {
  int dst;
  Value v;
  unsigned int immed;

  /* opcode with two register operands and immediate */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  immed = v.con;
  immed &= 0xFFF;
  emitHalf(4<<13 | ((immed>>5)&0x1)<<12 |  2<<10 | (dst-8)<<7 | (immed&0x1f)<<2 | 0x1);
}

void formatIm3(unsigned int code) {
  int dst, src1;
  Value v;
  unsigned int immed;

  /* opcode with two register operands and immediate */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  immed = v.con;
  immed &= 0xFFF;
  emitWord( immed<<20 | src1 << 15 | (code&0x7)<<12 | dst<<7 | 0x13);
}

void formatLIm(unsigned int code) {
  int dst, src1, vcon, rvccond;
  Value v;
  unsigned int immed;

  /* load opcode with two register operands and immediate */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  if (v.sym == NULL) {
    immed = v.con;
    vcon = v.con;
    expect(TOK_LPAREN);
    getToken();
    expect(TOK_IREGISTER);
    src1 = tokenvalNumber;
    getToken();
    expect(TOK_RPAREN);
    getToken();
  } else {
    addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RH20, v.con);
    immed = 0;
    emitWord( immed | dst<<7 | 0x17);
    immed = v.con;
    addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RL12, v.con);
    immed = 0;
    src1 = dst;
  }
  rvccond = (rvc!=0) && (v.sym==NULL) && (code==OP_LW);
   if( rvccond && (rvcreg(dst)==1) && (rvcreg(src1)==1) && (inurange(7, vcon)==1))
    emitHalf(2<<13 | ((immed>>3)&0x7)<<10 | (src1-8)<<7 | ((immed>>2)&0x1)<<6 | ((immed>>6)&0x1)<<5 | (dst-8)<<2 | 0x0);
   else if( rvccond && (src1==2) && (inurange(8, vcon)==1))
    emitHalf(2<<13 | ((immed>>5)&0x1)<<12 | dst<<7 | ((immed>>2)&0x7)<<4 | ((immed>>6)&0x3)<<2 | 0x2);
   else
    emitWord( immed<<20 | src1 << 15 | ((code>>4)&0x7)<<12 | dst<<7 | (code&0xF)<<3 | 0x3);
}

void formatLFIm(unsigned int code) {
  int dst, src1;
  Value v;
  unsigned int immed;

  /* load opcode with two register operands and immediate */
  expect(TOK_FREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  if (v.sym == NULL) {
    immed = v.con;    
    expect(TOK_LPAREN);
    getToken();
    expect(TOK_IREGISTER);
    src1 = tokenvalNumber;
    getToken();
    expect(TOK_RPAREN);
    getToken();
  } else {
    addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RH20, v.con);
    immed = 0;
    emitWord( immed | dst<<7 | 0x17);
    immed = v.con;
    addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RL12, v.con);
    immed = 0;
    src1 = dst;
  }
    emitWord( immed<<20 | src1 << 15 | ((code>>4)&0x7)<<12 | dst<<7 | (code&0xF)<<3 | 0x7);
}


void formatSIm(unsigned int code) {
  int dst, src1;
  Value v;
  unsigned int immed;

  /* shift opcode with two register operands and immediate */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  immed = v.con;
  immed &= 0x1F;
  if((rvc==0) || (rvcreg(dst)==0) || (src1!=dst))
  emitWord(((code>>3)&0x7F) << 25 | immed<<20 | src1 << 15 | (code&0x7)<<12 | dst<<7 | 0x13);
  else
  emitHalf(4<<13 | ((immed>>5)&0x1)<<12 | ((code>>8)&0x1)<<10 |  (dst-8)<<7 | (immed&0x1f)<<2 | 0x1);
}

void formatSCIm(unsigned int code) {
  int dst;
  Value v;
  unsigned int immed;

  /* shift opcode with two register operands and immediate */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  immed = v.con;
  immed &= 0x1F;
  emitHalf(4<<13 | ((immed>>5)&0x1)<<12 | ((code>>8)&0x1)<<10 |  (dst-8)<<7 | (immed&0x1f)<<2 | 0x1);
}

void formatUJ(unsigned int code) {
  int dst;
  Value v;

  /* jal opcode with one register operands and label */
  //expect(TOK_IREGISTER);
  if(token == TOK_IREGISTER) {
    dst = tokenvalNumber;    
    getToken();
    expect(TOK_COMMA);
    getToken();
  }  else
    dst = 1;
  v = parseExpression();
  addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_J20, v.con);
  emitWord(dst<<7 | 0x6F);
}

void formatJ(unsigned int code) {
  int dst;
  Value v;

  /* jal opcode with label */  
  dst = 0;  
  v = parseExpression();
  addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_J20, v.con);
  emitWord(dst<<7 | 0x6F);
}

void formatJR(unsigned int code) {
  int dst, src1;
  Value v;
  unsigned int immed;

  /* opcode with two register operands and immediate */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  expect(TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  immed = v.con;
  immed &= 0xFFF;

  emitWord( immed<<20 | src1 << 15 | (code&0x7)<<12 | dst<<7 | 0x67);
}

void formatJ0R(unsigned int code) {
  int dst, src1;  
  unsigned int immed;

  /* opcode with one register operands and immediate */  
  dst = 0;  
  expect(TOK_IREGISTER);
  src1 = tokenvalNumber;
  immed = 0;
  getToken();  
  emitWord( immed<<20 | src1 << 15 | (code&0x7)<<12 | dst<<7 | 0x67);
}


void formatRET(unsigned int code) {
  int dst, src1;
  unsigned int immed;

  /* opcode with two register operands and immediate */  
  dst = 0;
  getToken();  
  src1 = 1;  
  immed = 0;  

  emitWord( immed<<20 | src1 << 15 | (code&0x7)<<12 | dst<<7 | 0x67);
}

void formatLI(unsigned int code) {
  int dst, src;
  Value v;
  unsigned int immed;

  /* opcode with one register operands and immediate */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  src = 0;
  getToken();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  immed = v.con;
  if(insrange(12, v.con)==0) {
    if((immed & 0x800)!=0) immed+=0x1000;
    if(rvc==0 ||
    insrange(18, v.con)==0) {
    emitWord( (immed&0xFFFFF000) | dst<<7 | 0x37);
    } else
    emitHalf( 0x3<<13| ((immed>>17)&0x1)<<12 | dst<<7 | ((immed>>12)&0x1F)<<2 | 0x1);
    src = dst;
  }
  if(dst!=0) {
    if(rvc==0 || insrange(6, v.con)==0) {
        emitWord( (immed&0xFFF)<<20 | src << 15 | (code&0x7)<<12 | dst<<7 | 0x13);
    } else
    emitHalf( 0x2<<13| ((immed>>5)&0x1)<<12 | dst<<7 | (immed&0x1F)<<2 | 0x1);
  }
}

void formatLUI(unsigned int code) {
  int dst;
  Value v;
  unsigned int immed;

  /* opcode with one register operands and immediate */
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  expect(TOK_COMMA);
  getToken();
  v = parseExpression();
  immed = v.con;
  emitWord( (immed&0xFFFFF)<<12 | dst<<7 | 0x37);
}

void formatBRK(unsigned int code) {

   /* BREAK */
  if((rvc==0) || (code==OP_MRET)) emitWord( code<<20 | 0x73);
  else emitHalf( 9<<12 | 0x2);
}

void formatC(unsigned int code) {
  int dst;
  expect(TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken();
  emitWord( 0xC0<<24 |code<<20 | 2<<12 | dst<<7 | 0x73);
}

typedef struct instr {
  char *name;
  void (*func)(unsigned int code);
  unsigned int code;
} Instr;


Instr instrTable[] = {

  /* pseudo instructions */
  { ".syn",    dotSyn,    0 },
  { ".nosyn",  dotNosyn,  0 },
  { ".text",   dotCode,   0 },
  { ".data",   dotData,   0 },
  { ".section", dotData,   0 },
  { ".bss",    dotBss,    0 },
  { ".globl", dotExport, 0 },
  { ".import", dotImport, 0 },
  { ".align",  dotAlign,  0 },
  { ".p2align",  dotAlign,  0 },
  { ".space",  dotSpace,  0 },
  { ".locate", dotLocate, 0 },
  { ".byte",   dotByte,   0 },
  { ".asciz",   dotByte,   0 },
  { ".half",   dotHalf,   0 },
  { ".short",   dotHalf,   0 },
  { ".word",   dotWord,   0 },
  { ".long",   dotWord,   0 },
  { ".quad",   dotDword,   0 },
  { ".set",    dotSet,    0 },
  { ".file",   dotFile,   0 },
  { ".loc",    dotLoc,    0 },
  { ".stabs",  dotStabs,  0 },
  { ".stabn",  dotStabn,  0 },
  { ".type",  Ignore,  0 },
  { ".size",  Ignore,  0 },
  { ".ident",  Ignore,  0 },
  { ".equ",  Ignore,  0 },
  { ".weak",  Ignore,  0 },
  { ".local",  Ignore,  0 },
  { ".comm",  Ignore,  0 },
  { ".uleb128",  Ignore,  0 },
  { ".cfi_sections",  Ignore,  0 },
  { ".cfi_sections",  Ignore,  0 },
  { ".cfi_startproc",  Ignore,  0 },
  { ".cfi_endproc",  Ignore,  0 },
    /* single precision floating point instructions */
  { "fadd.s",  formatRF, OP_ADDF  },
  { "fsub.s",  formatRF, OP_SUBF  },
  { "fmul.s",  formatRF, OP_MULF  },
  { "fdiv.s",  formatRF, OP_DIVF  },
  { "fsgnj.s",  formatRF, OP_SGNJF },
  { "fmv.s",  formatMF, OP_SGNJF },
  { "fsgnjn.s",  formatRF, OP_SGNJNF },
  { "fneg.s",  formatNF, OP_SGNJNF },
  { "fsgnjx.s",  formatRF, OP_SGNJXF },
  { "fmin.s",  formatRF, OP_MINF  },
  { "fmax.s",  formatRF, OP_MAXF  },
  { "flw",    formatLFIm, OP_LWF },
  { "fsw",    formatSF, OP_SWF }, 
  { "fsqrt.s",  formatRF2, OP_SQRTF  },
  { "fmv.s.x",  formatRFX, OP_MVSXF  },
  { "fmv.x.s",  formatRXF, OP_MVXSF  },
  { "fcvt.s.w",  formatRFX, OP_CVTSWF  },
  { "fcvt.s.wu",  formatRFX, OP_CVTSWUF  },
  { "fcvt.w.s",  formatRXF, OP_CVTWSF  },
  { "fcvt.wu.s",  formatRXF, OP_CVTWUSF  },
  { "feq.s",  formatRCMPF, OP_EQF  },
  { "flt.s",  formatRCMPF, OP_LTF  },
  { "fle.s",  formatRCMPF, OP_LEF  },
   /* arithmetical instructions */
  { "add",     formatR, OP_ADD  },
  { "sub",     formatR2, OP_SUB  },
  { "c.sub",   formatRC2, OP_SUB  },
    /* shift instructions */
  { "sll",     formatR, OP_SLL  },
  { "srl",     formatR, OP_SRL  },
  { "sra",     formatR, OP_SRA  },
    /* logical instructions */
  { "and",     formatR2, OP_AND  },
  { "c.and",   formatRC2, OP_AND  },
  { "or",      formatR2, OP_OR   },
  { "c.or",    formatRC2, OP_OR   },
  { "xor",     formatR2, OP_XOR  },
  { "c.xor",   formatRC2, OP_XOR  },
  { "slt",     formatR, OP_SLT  },
  { "sltu",    formatR, OP_SLTU },
  /* branch instructions */
  { "beq",     formatSB, OP_BEQ  },
  { "bne",     formatSB, OP_BNE  },
  { "ble",     formatSB, OP_BLE  },
  { "bleu",    formatSB, OP_BLEU },
  { "blt",     formatSB, OP_BLT  },
  { "bltu",    formatSB, OP_BLTU },
  { "bge",     formatSB, OP_BGE  },
  { "bgeu",    formatSB, OP_BGEU },
  { "bgt",     formatSB, OP_BGT  },
  { "bgtu",    formatSB, OP_BGTU },
  { "jal",     formatUJ, OP_JAL  },
  { "j",       formatJ, OP_JAL  },
  { "jalr",    formatJR, OP_JALR  },
  { "ret",    formatRET, OP_JALR  },
  { "jr",    formatJ0R, OP_JALR  },
  /* immediate instructions */
  { "addi",    formatIm, OP_ADDI  },
  { "mv",    formatMv, OP_ADDI  },
  { "c.addi",  formatCIm, OP_ADDI  },
  { "nop",     formatIm, OP_NOP   },
  { "c.nop",   formatCIm, OP_NOP   },
  { "slli",    formatIm, OP_SLLI },
  { "c.slli",  formatCIm, OP_SLLI },
  { "andi",    formatIm2, OP_ANDI  },
  { "c.andi",  formatCIm2, OP_ANDI  },
  { "ori",     formatIm3, OP_ORI   },
  { "xori",    formatIm3, OP_XORI  },
  { "slti",    formatIm, OP_SLTI  },
  { "sltiu",   formatIm, OP_SLTIU },
  { "sltiu",   formatIm, OP_SLTIU },
  { "csrw",   formatCsrw, OP_CSRW  },
  { "csrs",   formatCsrw, OP_CSRS  },
  { "csrr",   formatCsrr, OP_CSRR  },
  { "csrwi",   formatCsrwi, OP_CSRWI },
  { "csrsi",   formatCsrwi, OP_CSRSI },
  { "csrci",   formatCsrwi, OP_CSRCI },
  { "lb",      formatLIm, OP_LB    },
  { "lh",      formatLIm, OP_LH    },
  { "lw",      formatLIm, OP_LW    },
  { "lbu",     formatLIm, OP_LBU  },
  { "lhu",     formatLIm, OP_LHU  },
  { "la",      formatLIm, OP_LA    },
  { "li",      formatLI, OP_LUI   },
  { "lui",     formatLUI, OP_LUI   },
 // { "slli",    formatSIm, OP_SLLI },
  { "srli",    formatSIm, OP_SRLI },
  { "c.srli",  formatSCIm, OP_SRLI },
  { "srai",    formatSIm, OP_SRAI },
  { "c.srai",    formatSCIm, OP_SRAI },
  { "sb",      formatS, OP_SB    },
  { "sh",      formatS, OP_SH    },
  { "sw",      formatS, OP_SW    },
  { "ebreak",  formatBRK, OP_BRK },
  { "mret",  formatBRK, OP_MRET },
  { "c.ebreak",  formatBRK, OP_BRK },
  { "mul",     formatR, OP_MUL  },
  { "mulh",    formatR, OP_MULH  },
  { "mulhsu",  formatR, OP_MULHSU  },
  { "mulhu",   formatR, OP_MULHU  },
  { "div",     formatR, OP_DIV  },
  { "divu",    formatR, OP_DIVU  },
  { "rem",     formatR, OP_REM  },
  { "remu",    formatR, OP_REMU  },
  { "rdcycle", formatC, OP_CNTCYC},
  { "rdcycleh",formatC, OP_CNTCYCH},
  { "rdtime",  formatC, OP_CNTTIME},
  { "rdtimeh", formatC, OP_CNTTIMEH},
  { "rdinstret",formatC, OP_CNTINST},
  { "rdinstreth",formatC, OP_CNTINSTH}
};


static int cmpInstr(const void *instr1, const void *instr2) {
  return strcmp(((Instr *) instr1)->name, ((Instr *) instr2)->name);
}


void sortInstrTable(void) {
  qsort(instrTable, sizeof(instrTable)/sizeof(instrTable[0]),
        sizeof(instrTable[0]), cmpInstr);
}


Instr *lookupInstr(char *name) {
  int lo, hi, tst;
  int res;

  lo = 0;
  hi = sizeof(instrTable) / sizeof(instrTable[0]) - 1;
  while (lo <= hi) {
    tst = (lo + hi) / 2;
    res = strcmp(instrTable[tst].name, name);
    if (res == 0) {
      return &instrTable[tst];
    }
    if (res < 0) {
      lo = tst + 1;
    } else {
      hi = tst - 1;
    }
  }
  return NULL;
}


/**************************************************************/


void roundupSegments(void) {
int rounding = (rvc)? 1 : 3;
  while (segPtr[SEGMENT_CODE] & rounding) {
    fputc(0, codeFile);
    segPtr[SEGMENT_CODE] += 1;
  }
  while (segPtr[SEGMENT_DATA] & rounding) {
    fputc(0, dataFile);
    segPtr[SEGMENT_DATA] += 1;
  }
  while (segPtr[SEGMENT_BSS] & rounding) {
    segPtr[SEGMENT_BSS] += 1;
  }
}


void asmModule(void) {
  Symbol *label;
  Instr *instr;

  allowSyn = 1;
  currSeg = SEGMENT_CODE;
  lineno = 0;
  while (fgets(line, LINE_SIZE, inFile) != NULL) {
    lineno++;
    lineptr = line;
    getToken();
    while (token == TOK_LABEL) {
      label = deref(lookupEnter(tokenvalString, LOCAL_TABLE, 0));
      if (label->status != STATUS_UNKNOWN) {
        error("label '%s' multiply defined in line %d",
              label->name, lineno);
      }
      label->status = STATUS_DEFINED;
      label->segment = currSeg;
      label->value = segPtr[currSeg];
      getToken();
    }
    if (token == TOK_IDENT) {
      instr = lookupInstr(tokenvalString);
      if (instr == NULL) {
        error("unknown instruction '%s' in line %d",
              tokenvalString, lineno);
      }
      getToken();
      (*instr->func)(instr->code);
    }
    if (token != TOK_EOL) {
      error("garbage in line %d", lineno);
    }
  }
  roundupSegments();
}


/**************************************************************/



static ExecHeader execHeader;
static int numSymbols;
static int crelSize;
static int drelSize;
static int symtblSize;
static int stringSize;


static void walkTree(Symbol *s, void (*fp)(Symbol *sp)) {
  if (s == NULL) {
    return;
  }
  walkTree(s->left, fp);
  (*fp)(s);
  walkTree(s->right, fp);
}


void writeDummyHeader(void) {
  fwrite(&execHeader, sizeof(ExecHeader), 1, outFile);
}


void writeRealHeader(void) {
  rewind(outFile);
  execHeader.magic = EXEC_MAGIC;
  execHeader.csize = segPtr[SEGMENT_CODE];
  execHeader.dsize = segPtr[SEGMENT_DATA];
  execHeader.bsize = segPtr[SEGMENT_BSS];
  execHeader.crsize = crelSize;
  execHeader.drsize = drelSize;
  execHeader.symsize = symtblSize;
  execHeader.strsize = stringSize;
  fwrite(&execHeader, sizeof(ExecHeader), 1, outFile);
}


void writeCode(void) {
  int data;

  rewind(codeFile);
  while (1) {
    data = fgetc(codeFile);
    if (data == EOF) {
      break;
    }
    fputc(data, outFile);
  }
}


void writeData(void) {
  int data;

  rewind(dataFile);
  while (1) {
    data = fgetc(dataFile);
    if (data == EOF) {
      break;
    }
    fputc(data, outFile);
  }
}


void transferFixupsForSymbol(Symbol *s) {
  Fixup *f;

  if (s->status != STATUS_UNKNOWN && s->status != STATUS_DEFINED) {
    /* this should never happen */
    error("global symbol is neither unknown nor defined");
  }
  if (s->status == STATUS_UNKNOWN && s->fixups == NULL) {
    /* this symbol is neither defined here nor referenced here: skip */
    s->skip = 1;
    return;
  }
  s->skip = 0;
  while (s->fixups != NULL) {
    /* get next fixup record */
    f = s->fixups;
    s->fixups = f->next;
    /* use the 'base' component to store the current symbol number */
    f->base = MSB | numSymbols;
    /* transfer the record to the fixup list */
    f->next = fixupList;
    fixupList = f;
  }
  numSymbols++;
}


void transferFixups(void) {
  numSymbols = 0;
  walkTree(globalTable, transferFixupsForSymbol);
}


void writeCodeRelocs(void) {
  Fixup *f;
  RelocRecord relRec;

  crelSize = 0;
  f = fixupList;
  while (f != NULL) {
    if (f->segment != SEGMENT_CODE && f->segment != SEGMENT_DATA) {
      /* this should never happan */
      error("fixup found in a segment other than code or data");
    }
    if (f->segment == SEGMENT_CODE) {
      relRec.offset = f->offset;
      relRec.method = f->method;
      relRec.value = f->value;
      relRec.base = f->base;
      fwrite(&relRec, sizeof(RelocRecord), 1, outFile);
      crelSize += sizeof(RelocRecord);
    }
    f = f->next;
  }
}


void writeDataRelocs(void) {
  Fixup *f;
  RelocRecord relRec;

  drelSize = 0;
  f = fixupList;
  while (f != NULL) {
    if (f->segment != SEGMENT_CODE && f->segment != SEGMENT_DATA) {
      /* this should never happan */
      error("fixup found in a segment other than code or data");
    }
    if (f->segment == SEGMENT_DATA) {
      relRec.offset = f->offset;
      relRec.method = f->method;
      relRec.value = f->value;
      relRec.base = f->base;
      fwrite(&relRec, sizeof(RelocRecord), 1, outFile);
      drelSize += sizeof(RelocRecord);
    }
    f = f->next;
  }
}


void writeSymbol(Symbol *s) {
  SymbolRecord symRec;  
  if (s->skip) {
    /* this symbol is neither defined here nor referenced here: skip */
    return;
  }
  symRec.name = stringSize;
  if (s->status == STATUS_UNKNOWN) {
    symRec.type = MSB;
    symRec.value = 0;
    symRec.debug = 0;
    symRec.debugtype = 0;
    symRec.debugvalue = 0;
  } else {
    symRec.type = s->segment;
    symRec.value = s->value;
    symRec.debug = s->debug;
    symRec.debugtype = s->debugtype;
    symRec.debugvalue = s->debugvalue;
  }
  fwrite(&symRec, sizeof(SymbolRecord), 1, outFile);
  symtblSize += sizeof(SymbolRecord);  
  stringSize += strlen(s->name) + 1;
}


void writeSymbols(void) {
  symtblSize = 0;
  stringSize = 0;
  walkTree(globalTable, writeSymbol);
}


void writeString(Symbol *s) {
  if (s->skip) {
    /* this symbol is neither defined here nor referenced here: skip */
    return;
  }
  fputs(s->name, outFile);
  fputc('\0', outFile);
}


void writeStrings(void) {
  walkTree(globalTable, writeString);
}


/**************************************************************/


void usage(char *myself) {
  fprintf(stderr, "Usage: %s\n", myself);
  fprintf(stderr, "         [-o objfile]     set object file name\n");
  fprintf(stderr, "         file             source file name\n");
  fprintf(stderr, "         [files...]       additional source files\n");
  exit(1);
}


int main(int argc, char *argv[]) {
  int i;
  char *argp;
  char *tmp;
  sortInstrTable();
  tmpnam(codeName);
  tmpnam(dataName);
  outName = "a.out";
  for (i = 1; i < argc; i++) {
    argp = argv[i];
    if (*argp != '-') {
      break;
    }
    argp++;
    switch (*argp) {
      case 'o':
        if (i == argc - 1) {
          usage(argv[0]);
        }
        outName = argv[++i];
        break;
      case 'g':
        if (i == argc - 1) {
          usage(argv[0]);
        }
        debug = 1;
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
  tmp = dirname(strdup(outName));  
  strcpy(outpath, tmp);
  strcat(outpath,"/");
  if(debug) {
      strcpy(dbgfile,outpath);
      strcat(dbgfile,"as.dbg");
      mlibinit();
      f = m_core_fopen (dbgfile, "wt");
      if (!f) abort();
      m_serial_json_write_init(out, f);  
  }
  do {
    inName = argv[i];
    if (*inName == '-') {
      usage(argv[0]);
    }
    inFile = fopen(inName, "rt");
    if (inFile == NULL) {
      error("cannot open input file '%s'", inName);
    }
    fprintf(stderr, "Assembling module '%s'...\n", inName);
    asmModule();
    if (inFile != NULL) {
      fclose(inFile);
      inFile = NULL;
    }
    linkLocals();
  } while (++i < argc);
  writeDummyHeader();
  writeCode();
  writeData();
  transferFixups();
  writeCodeRelocs();
  writeDataRelocs();
  writeSymbols();
  writeStrings();
  writeRealHeader();
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
  if (codeName != NULL) {
    unlink(codeName);
  }
  if (dataName != NULL) {
    unlink(dataName);
  }
  if(debug) {
    ret = root_out_serial(out, root);
    assert (ret == M_SERIAL_OK_DONE);
    m_serial_json_write_clear(out); 
    mlibclear();
    fclose(f);
  }
  return 0;
}
