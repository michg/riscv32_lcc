/*
 * dof.c -- dump object file
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "../include/a.out.h"


#define MSB	((unsigned int) 1 << (sizeof(unsigned int) * 8 - 1))


/**************************************************************/


FILE *inFile;
ExecHeader execHeader;
char *segmentName[4] = { "ABS", "CODE", "DATA", "BSS" };
char *methodName[6] = { "W32" , "R12" , "RL12" , "RH20", "RS12","J20" };



/**************************************************************/


void error(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  printf("Error: ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
  exit(1);
}


/**************************************************************/








/**************************************************************/


#define CODE_START	(sizeof(ExecHeader))
#define DATA_START	(CODE_START + execHeader.csize)
#define CRELOC_START	(DATA_START + execHeader.dsize)
#define DRELOC_START	(CRELOC_START + execHeader.crsize)
#define SYMTBL_START	(DRELOC_START + execHeader.drsize)
#define STRING_START	(SYMTBL_START + execHeader.symsize)


void dumpHeader(int opt) {
  if (fseek(inFile, 0, SEEK_SET) < 0) {
    error("cannot seek to exec header");
  }
  if (fread(&execHeader, sizeof(ExecHeader), 1, inFile) != 1) {
    error("cannot read exec header");
  }
  if (execHeader.magic != EXEC_MAGIC) {
    error("wrong magic number in exec header");
  }
  if(opt==0) {
  printf("Header\n");
  printf("    size of code         : %8u bytes\n", execHeader.csize);
  printf("    size of data         : %8u bytes\n", execHeader.dsize);
  printf("    size of bss          : %8u bytes\n", execHeader.bsize);
  printf("    size of code relocs  : %8u bytes\n", execHeader.crsize);
  printf("    size of data relocs  : %8u bytes\n", execHeader.drsize);
  printf("    size of symbol table : %8u bytes\n", execHeader.symsize);
  printf("    size of string space : %8u bytes\n", execHeader.strsize);
  }
}


void dumpBytes(unsigned int totalSize) {
  unsigned int currSize;
  unsigned char line[16];
  int n, i;
  unsigned char c;

  currSize = 0;
  while (currSize < totalSize) {
    if (totalSize - currSize >= 16) {
      n = 16;
    } else {
      n = totalSize - currSize;
    }
    for (i = 0; i < n; i++) {
      line[i] = fgetc(inFile);
    }
    printf("%08X:  ", currSize);
    for (i = 0; i < 16; i++) {
      if (i < n) {
        c = line[i];
        printf("%02X", c);
      } else {
        printf("  ");
      }
      printf(" ");
    }
    printf("  ");
    for (i = 0; i < 16; i++) {
      if (i < n) {
        c = line[i];
        if (c >= 32 && c <= 126) {
          printf("%c", c);
        } else {
          printf(".");
        }
      } else {
        printf(" ");
      }
    }
    printf("\n");
    currSize += n;
  }
}


void dumpCode(void) {
  if (fseek(inFile, CODE_START, SEEK_SET) < 0) {
    error("cannot seek to code section");
  }
  printf("\nCode Segment\n");
  dumpBytes(execHeader.csize);
}


void dumpData(void) {
  if (fseek(inFile, DATA_START, SEEK_SET) < 0) {
    error("cannot seek to data section");
  }
  printf("\nData Segment\n");
  dumpBytes(execHeader.dsize);
}


void dumpRelocs(unsigned int totalSize) {
  unsigned int currSize;
  int n;
  RelocRecord relRec;

  currSize = 0;
  n = 0;
  while (currSize < totalSize) {
    if (fread(&relRec, sizeof(RelocRecord), 1, inFile) != 1) {
      error("cannot read relocation record");
    }
    printf("    %d:\n", n);
    printf("        offset  = 0x%08X\n", relRec.offset);
    if (relRec.method < 0 || relRec.method > 5) {
      error("illegal relocation method");
    }
    printf("        method  = %s\n", methodName[relRec.method]);
    printf("        value   = 0x%08X\n", relRec.value);
    if (relRec.base & MSB) {
      printf("        base    = symbol # %d\n", relRec.base & ~MSB);
    } else {
      if (relRec.base < 0 || relRec.base > 3) {
        error("base contains an illegal segment number");
      }
      printf("        base    = %s\n", segmentName[relRec.base]);
    }
    currSize += sizeof(RelocRecord);
    n++;
  }
}


void dumpCodeRelocs(void) {
  if (fseek(inFile, CRELOC_START, SEEK_SET) < 0) {
    error("cannot seek to code relocation section");
  }
  printf("\nCode Relocation Records\n");
  dumpRelocs(execHeader.crsize);
}


void dumpDataRelocs(void) {
  if (fseek(inFile, DRELOC_START, SEEK_SET) < 0) {
    error("cannot seek to data relocation section");
  }
  printf("\nData Relocation Records\n");
  dumpRelocs(execHeader.drsize);
}


void dumpString(unsigned int offset) {
  long pos;
  int c;

  pos = ftell(inFile);
  if (fseek(inFile, STRING_START + offset, SEEK_SET) < 0) {
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


void dumpSymbolTable(void) {
  unsigned int currSize;
  int n;
  SymbolRecord symRec;

  if (fseek(inFile, SYMTBL_START, SEEK_SET) < 0) {
    error("cannot seek to symbol table section");
  }
  printf("\nSymbol Table Records\n");
  currSize = 0;
  n = 0;
  while (currSize < execHeader.symsize) {
    if (fread(&symRec, sizeof(SymbolRecord), 1, inFile) != 1) {
      error("cannot read symbol record");
    }
    printf("    %d:\n", n);
    printf("        name    = ");
    dumpString(symRec.name);
    printf("\n");
    if (symRec.type & MSB) {
      printf("        --- undefined ---\n");
    } else {
      if (symRec.type < 0 || symRec.type > 3) {
        error("type contains an illegal segment number");
      }
      printf("        segment = %s\n", segmentName[symRec.type]);
      printf("        value   = 0x%08X\n", symRec.value);
    }
    currSize += sizeof(SymbolRecord);
    n++;
  }
}

void dumpUndefined(void) {
  unsigned int currSize;
  int n;
  SymbolRecord symRec;

  if (fseek(inFile, SYMTBL_START, SEEK_SET) < 0) {
    error("cannot seek to symbol table section");
  }
  currSize = 0;
  n = 0;
  while (currSize < execHeader.symsize) {
    if (fread(&symRec, sizeof(SymbolRecord), 1, inFile) != 1) {
      error("cannot read symbol record");
    }
    if (symRec.type & MSB) {
      dumpString(symRec.name);
      printf("\n");
    } else {
      if (symRec.type < 0 || symRec.type > 3) {
        error("type contains an illegal segment number");
      }
    }
    currSize += sizeof(SymbolRecord);
    n++;
  }
}

/**************************************************************/


void usage(char *myself) {
  printf("Usage: %s\n", myself);
  printf("         [-c]             dump code\n");
  printf("         [-d]             dump data\n");
  printf("         [-x]             dump code relocations\n");
  printf("         [-y]             dump data relocations\n");
  printf("         [-s]             dump symbol table\n");
  printf("         [-u]             dump undefined symbols\n");
  printf("         [-a]             dump all\n");
  printf("         file             object file to be dumped\n");
  exit(1);
}


int main(int argc, char *argv[]) {
  int i;
  char *argp;
  int optionCode;
  int optionData;
  int optionCodeRelocs;
  int optionDataRelocs;
  int optionSymbolTable;
  int optionUndefined;
  char *inName;

  optionCode = 0;
  optionData = 0;
  optionCodeRelocs = 0;
  optionDataRelocs = 0;
  optionSymbolTable = 0;
  optionUndefined = 0;
  inName = NULL;
  for (i = 1; i < argc; i++) {
    argp = argv[i];
    if (*argp == '-') {
      argp++;
      switch (*argp) {
        case 'c':
          optionCode = 1;
          break;
        case 'd':
          optionData = 1;
          break;
        case 'x':
          optionCodeRelocs = 1;
          break;
        case 'y':
          optionDataRelocs = 1;
          break;
        case 's':
          optionSymbolTable = 1;
          break;
        case 'u':
          optionUndefined = 1;
          break;
        case 'a':
          optionCode = 1;
          optionData = 1;
          optionCodeRelocs = 1;
          optionDataRelocs = 1;
          optionSymbolTable = 1;
          break;
        default:
          usage(argv[0]);
      }
    } else {
      if (inName != NULL) {
        usage(argv[0]);
      }
      inName = argp;
    }
  }
  if (inName == NULL) {
    usage(argv[0]);
  }
  inFile = fopen(inName, "r");
  if (inFile == NULL) {
    error("cannot open input file '%s'", inName);
  }
  dumpHeader(optionUndefined);
  if (optionCode) {
    dumpCode();
  }
  if (optionData) {
    dumpData();
  }
  if (optionCodeRelocs) {
    dumpCodeRelocs();
  }
  if (optionDataRelocs) {
    dumpDataRelocs();
  }
  if (optionSymbolTable) {
    dumpSymbolTable();
  }
  if (optionUndefined) {
    dumpUndefined();
  }
  fclose(inFile);
  return 0;
}
