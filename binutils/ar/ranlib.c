/*
 * ranlib.c -- archive index generator
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include "../include/ar.h"
#include "../include/a.out.h"
#include "../include/ranlib.h"


#define MAX_SYM_ENTRIES		1000
#define MSB	((unsigned int) 1 << (sizeof(unsigned int) * 8 - 1))


typedef struct {
  unsigned int name;	/* name of symbol (as offset into string space) */
  long position;	/* position of member which defines the symbol */
			/* (as file offset to the member's ArHeader) */
} Entry;


FILE *fi;
FILE *fo;

long nxtOff;		/* file offset to next member */
long curOff;

Entry table[MAX_SYM_ENTRIES];
int numEntries;
char firstName[MAX_NAME];
char *stringArea = NULL;
unsigned int arMagic;
ArHeader arhdr;
ExecHeader exhdr;

/**************************************************************/

int nextMember(void) {
  int pad;

  curOff = nxtOff;
  fseek(fi, nxtOff, SEEK_SET);
  if (fread(&arhdr, sizeof(arhdr), 1, fi) != 1) {
    return 0;
  }
  pad = -arhdr.size & 0x03;
  arhdr.size += pad;
  nxtOff = ftell(fi) + arhdr.size;
  return 1;
}


void addSymbol(char* symbolname,char* filename) {
  int len;
  static unsigned int stringsize=0;

  if (numEntries >= MAX_SYM_ENTRIES) {
    fprintf(stderr, "ar: symbol table overflow\n");
    exit(1);
  }
  table[numEntries].name = stringsize;
  table[numEntries].position = curOff;
  numEntries++;
  len = strlen(symbolname) + strlen(filename) + 2;
  stringArea = realloc(stringArea, stringsize + len + 1);  
  if(!stringArea) {
      printf("Realloc failed!\n");
      exit(1);
  }
  stringsize += sprintf(stringArea + stringsize, "%s:%s,", symbolname, filename);  
}



/**************************************************************/


void showSymdefs(char *symdefs) {
  FILE *in;
  int numSymbols;
  int i, pos;
  unsigned int n = 40;
  char symname[40], filename[40];
  char* symnameptr = &symname[0];
  char* filenameptr = &filename[0];  
  

  in = fopen(symdefs, "r");
  if (in == NULL) {
    printf("error: cannot open symdef file '%s'\n", symdefs);
    exit(1);
  }
  if (fread(&numSymbols, sizeof(int), 1, in) != 1) {
    printf("cannot read symdef file\n");
    exit(1);
  }
  printf("%d symbols\n", numSymbols);
  pos = sizeof(int) + numSymbols * sizeof(Entry);
  fseek(in, pos, SEEK_SET);
  for (i = 0; i < numSymbols; i++) {  
   getdelim(&symnameptr, &n, ':', in);
   symnameptr[strlen(symnameptr)-1] = '\0';
   getdelim(&filenameptr, &n, ',', in);   
   filenameptr[strlen(filenameptr)-1] = '\0';
   printf("Symbol:%s,%s\n",symnameptr,filenameptr);
  } 
  fclose(in);
}


/**************************************************************/


int hasSymbols(char *archive) {
  int res;

  fi = fopen(archive, "r");
  if (fi == NULL) {
    return 0;
  }
  nxtOff = sizeof(unsigned int);
  if (fread(&arMagic, sizeof(arMagic), 1, fi) != 1 ||
      arMagic != AR_MAGIC) {
    fclose(fi);
    return 0;
  }
  fseek(fi, 0, SEEK_SET);
  if (nextMember() == 0) {
    fclose(fi);
    return 0;
  }
  fclose(fi);
  res = (strncmp(arhdr.name, TEMP_NAME, MAX_NAME) == 0);
  return res;
}


int updateSymbols(char *archive, int verbose) {  
  unsigned int skip;
  int numSymbols;
  unsigned int stringStart;
  SymbolRecord symbol;
  unsigned int n = 40;
  char symbolname[40];
  char* symbolnameptr = &symbolname[0];  
  char *nameptr;  
  int res;
  long curPos;

  if (verbose) {
    printf("ar: updating symbols in %s\n", archive);
  }
  fi = fopen(archive, "r");
  if (fi == NULL) {
    fprintf(stderr, "ar: cannot re-open %s\n", archive);
    return 1;
  }
  nxtOff = sizeof(unsigned int);
  if (fread(&arMagic, sizeof(arMagic), 1, fi) != 1 ||
      arMagic != AR_MAGIC) {
    fprintf(stderr, "ar: %s not in archive format\n", archive);
    fclose(fi);
    return 1;
  }
  fseek(fi, 0, SEEK_SET);
  numEntries = 0;
  if (nextMember() == 0) {
    fclose(fi);
    return 0;
  }
  /* iterate over archive members */
  do {
    if (fread(&exhdr, sizeof(exhdr), 1, fi) != 1 ||
        exhdr.magic != EXEC_MAGIC) {
      /* archive member not in proper format - skip */
      continue;
    }
    skip = exhdr.csize + exhdr.dsize + exhdr.crsize + exhdr.drsize;
    fseek(fi, skip, SEEK_CUR);
    numSymbols = exhdr.symsize / sizeof(SymbolRecord);
    if (numSymbols == 0) {
      fprintf(stderr,
              "ar: symbol table of %s is empty\n",
              arhdr.name);
      continue;
    }
    stringStart = sizeof(exhdr) + skip + exhdr.symsize;
    /* iterate over symbols */
    while (--numSymbols >= 0) {
      if (fread(&symbol, sizeof(symbol), 1, fi) != 1) {
        fprintf(stderr, "ar: cannot read archive\n");
        break;
      }
      if ((symbol.type & MSB) == 0) {
        /* this is an exported symbol */
        curPos = ftell(fi);
        fseek(fi, curOff + sizeof(arhdr) + stringStart + symbol.name, SEEK_SET);
        getdelim(&symbolnameptr, &n, '\0', fi);
        fseek(fi, curPos, SEEK_SET);
        addSymbol(symbolname, arhdr.name);         
      }
    }
  } while (nextMember() != 0) ;  
  nxtOff = sizeof(unsigned int);
  nextMember();  
  strncpy(firstName, arhdr.name, MAX_NAME);
  fclose(fi); 
  fo = fopen(TEMP_NAME, "w");
  if (fo == NULL) {
    fprintf(stderr, "ar: can't create temporary file\n");
    return 1;
  }
  if (fwrite(&numEntries, sizeof(numEntries), 1, fo) != 1) {
    fprintf(stderr, "ar: can't write temporary file\n");
    fclose(fo);
    unlink(TEMP_NAME);
    return 1;
  }
  if (fwrite(table, sizeof(Entry), numEntries, fo) != numEntries) {
    fprintf(stderr, "ar: can't write temporary file\n");
    fclose(fo);
    unlink(TEMP_NAME);
    return 1;
  }
  if (fwrite(stringArea, strlen(stringArea), 1, fo) != 1) {
    fprintf(stderr, "ar: can't write temporary file\n");
    fclose(fo);
    unlink(TEMP_NAME);
    return 1;
  }
  fclose(fo);
  
  if (verbose) {
    showSymdefs(TEMP_NAME);
  }
  /* ar -rlb firstName archive TEMP_NAME */      
    nameptr = TEMP_NAME;    
    rCmd(firstName, archive, &nameptr, 1, 0, 1, 0);    
    res = notFound(1, &nameptr);
  return res;
}
