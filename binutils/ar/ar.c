/*
 * ar.c -- archiver
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <libgen.h>

#include "../include/ranlib.h"
#include "../include/ar.h"


/**************************************************************/


#define BUFSIZE	512

#define SKIP	0x01
#define IODD	0x02
#define OODD	0x04
#define HEAD	0x08


char *com = "drqtpmx";
char *opt = "vuabs";
char *symfilenam =  TEMP_NAME;


void (*comfun)(void);
int flg[26];

char *arnam;
int af;

char **namv;
int namc;

int baState;
char *posName;

char tmp0nam[20];
char *tf0nam;
char tmp1nam[20];
char *tf1nam;

int tf0 = 0;
int tf1 = 0;

int qf;

char *file;
char name[MAX_NAME];

struct stat stbuf;
ArHeader arhead;
unsigned char buf[BUFSIZE];


/**************************************************************/


void mesg(int c, char *filename) {
  if (flg['v' - 'a']) {
    printf("%c - %s\n", c, filename);
  }
}

int notFound(int namc, char **namv) {
  int n;
  int i;

  n = 0;
  for (i = 0; i < namc; i++) {
    if (namv[i] != NULL) {
      fprintf(stderr, "ar: %s not found\n", namv[i]);
      n++;
    }
  }
  return n;
}


int moreFiles(int namc, char **namv) {
  int n;
  int i;

  n = 0;
  for (i = 0; i < namc; i++) {
    if (namv[i] != NULL) {
      n++;
    }
  }
  return n;
}


void unlinkTempFiles(void) {
int errcode;
  if (strcmp(tmp0nam,"v0XXXXXX")) {
    close(tf0);   
    errcode = unlink(tmp0nam);     
  }
  if (strcmp(tmp1nam,"v1XXXXXX")) {
    close(tf1);    
    errcode = unlink(tmp1nam);    
  } 
  
}


void done(int c) {
  unlinkTempFiles();
  exit(c);
}



void noArchive(void) {
  fprintf(stderr, "ar: %s does not exist\n", arnam);
  done(1);
}


void writeError(void) {
  perror("ar write error");
  done(1);
}




int openstats(char *filename, struct stat* stbuf) {
  int f;

  f = open(filename, O_RDONLY);
  if (f < 0) {
    return f;
  }
  if (fstat(f, stbuf) < 0) {
    close(f);
    return -1;
  }
  return f;
}


int matchargs(char** ptr, int namc, char **namv) {
  int i;

  for (i = 0; i < namc; i++) {
    if (namv[i] == NULL) {
      continue;
    }    
    if (strcmp(basename(strdup(namv[i])), *ptr) == 0) {
      *ptr = namv[i];      
      namv[i] = NULL;      
      return 1;
    }
  }
  return 0;
}


void baMatch(int* tf0, int* tf1, char* filename, char* posname, int aflag) {
  int f;
  static int baState = 0;  
  if (strcmp(filename, posname) != 0) {
      return;
  }
  baState = 2;
  if (aflag) {
     return;
  }
  
  if (baState == 2) {
    baState = 0;    
    f = mkstemp(tmp1nam);
    if (f < 0) {
      fprintf(stderr, "ar: cannot create second temp file\n");
      return;
    }
    *tf1 = *tf0;
    *tf0 = f;
  }
}


/**************************************************************/


void init(int* tf0) {
  unsigned int mbuf;

  mbuf = AR_MAGIC;  
  *tf0 = mkstemp(tmp0nam);
  if (*tf0 < 0) {
    fprintf(stderr, "ar: cannot create temp file\n");
    done(1);
  }
  if (write(*tf0, &mbuf, sizeof(mbuf)) != sizeof(mbuf)) {
    writeError();
  }
}


int getArchive(int *af, char *arnam) {
  unsigned int mbuf;

  *af = open(arnam, O_RDONLY);
  if (*af < 0) {
    return 1;
  }
  if (read(*af, &mbuf, sizeof(mbuf)) != sizeof(mbuf) ||
      mbuf != AR_MAGIC) {
    fprintf(stderr, "ar: %s not in archive format\n", arnam);
    done(1);
  }
  return 0;
}



int getMember(char **filename, int af, ArHeader *arhead) {
  int i;

  i = read(af, arhead, sizeof(ArHeader));
  if (i != sizeof(ArHeader)) {
    if (strcmp(tmp1nam,"v1XXXXXX")) {
      i = tf0;
      tf0 = tf1;
      tf1 = i;
    }
    return 1;
  }
  for (i = 0; i < MAX_NAME; i++) {
    name[i] = arhead->name[i];
  }
  *filename = name;
  return 0;
}


void copyFile(int fi, int fo, int flags, ArHeader arhead) {
  int pe;
  int icount, ocount;
  int pad;

  if (flags & HEAD) {
    if (write(fo, &arhead, sizeof(arhead)) != sizeof(arhead)) {
      writeError();
    }
  }
  pe = 0;
  while (arhead.size > 0) {
    icount = ocount = BUFSIZE;
    if (arhead.size < icount) {
      icount = ocount = arhead.size;
      pad = -icount & 0x03;
      if (flags & IODD) {
        icount += pad;
      }
      if (flags & OODD) {
        ocount += pad;
      }
    }
    if (read(fi, buf, icount) != icount) {
      pe++;
    }
    if ((flags & SKIP) == 0) {
      if (write(fo, buf, ocount) != ocount) {
        writeError();
      }
    }
    arhead.size -= BUFSIZE;
  }
  if (pe != 0) fprintf(stderr, "ar: phase error on %s\n", file);  
}


void moveFile(int f, int tf0, char* file, struct stat *stbuf) {
  char *cp;
  int i;
  ArHeader arhead;
  
  cp = basename(strdup(file));
  for (i = 0; i < MAX_NAME; i++) {
    if ((arhead.name[i] = *cp) != '\0') {
      cp++;
    }
  }
  arhead.size = stbuf->st_size;
  arhead.date = stbuf->st_mtime;
  arhead.uid = stbuf->st_uid;
  arhead.gid = stbuf->st_gid;
  arhead.mode = stbuf->st_mode;
  copyFile(f, tf0, OODD | HEAD, arhead);
  close(f);
}


void install(int tf0, int tf1, char *arnam, int* af) {
  int i;    
  close(*af);
  *af = creat(arnam, 0666);
  if (*af < 0) {
    fprintf(stderr, "ar: cannot create %s\n", arnam);
    done(1);
  }
  if (strcmp(tmp0nam,"v0XXXXXX")) {
    lseek(tf0, 0, SEEK_SET);
    while ((i = read(tf0, buf, BUFSIZE)) > 0) {
      if (write(*af, buf, i) != i) {
        writeError();
      }
    }
  }
  
  if (strcmp(tmp1nam,"v1XXXXXX")) {
    lseek(tf1, 0, SEEK_SET);
    while ((i = read(tf1, buf, BUFSIZE)) > 0) {
      if (write(*af, buf, i) != i) {
        writeError();
      }
    }
  }
}


void moveargfiles(int tf0,  int namc, char **namv) {
  int i;
  int f;

  for (i = 0; i < namc; i++) {
    file = namv[i];
    if (file == NULL) {
      continue;
    }
    namv[i] = NULL;
    mesg('a', file);
    f = openstats(file, &stbuf);
    if (f < 0) {
      fprintf(stderr, "ar: cannot open %s\n", file);
      continue;
    }
    moveFile(f, tf0, file, &stbuf);
  }
}


/**************************************************************/


void dCmd(void) {
  init(&tf0);
  if (getArchive(&af, arnam)) {
    noArchive();
  }
  while (!getMember(&file, af, &arhead)) {
    if (matchargs(&file, namc, namv)) {
      mesg('d', file);
      copyFile(af, -1, IODD | SKIP, arhead);
      continue;
    }
    mesg('c', file);
    copyFile(af, tf0, IODD | OODD | HEAD, arhead);
  }
  install(tf0, tf1, arnam, &af);
}


void rCmd(char *posName, char* arnam, char **namv, int namc, int aflag, int bflag, int uflag) {
  int f;

  init(&tf0);
  getArchive(&af, arnam);
  while (!getMember(&file, af, &arhead)) {
    if (aflag || bflag) baMatch(&tf0, &tf1, file, posName, aflag);
    if (namc == 0 || matchargs(&file, namc, namv)) {
      f = openstats(file, &stbuf);
      if (f < 0) {
        if (namc != 0) {
          fprintf(stderr, "ar: cannot open %s\n", file);
        }
        goto cp;
      }
      if (uflag) {
        if (stbuf.st_mtime <= arhead.date) {
          close(f);
          goto cp;
        }
      }
      mesg('r', file);
      copyFile(af, -1, IODD | SKIP, arhead);
      moveFile(f, tf0, file, &stbuf);
      continue;
    }
cp:
    mesg('c', file);
    copyFile(af, tf0, IODD | OODD | HEAD, arhead);
  }
  moveargfiles(tf0, namc, namv);
  install(tf0, tf1, arnam, &af);
}



void tCmd(void) {
  if (getArchive(&af, arnam)) {
    noArchive();
  }
  while (!getMember(&file, af, &arhead)) {
    if (namc == 0 || matchargs(&file, namc, namv)) {      
      printf("%s\n", basename(strdup(file)));
    }
    copyFile(af, -1, IODD | SKIP, arhead);
  }
}




void xCmd(void) {
  int f;

  if (getArchive(&af, arnam)) {
    noArchive();
  }
  while (!getMember(&file, af, &arhead)) {
    if (namc == 0 || matchargs(&file, namc, namv)) {
      f = creat(file, arhead.mode & 0777);
      if (f < 0) {
        fprintf(stderr, "ar: cannot create %s\n", file);
        goto sk;
      }
      mesg('x', file);
      copyFile(af, f, IODD, arhead);
      close(f);
      continue;
    }
sk:
    mesg('c', file);
    copyFile(af, -1, IODD | SKIP, arhead);
    if (namc > 0 && !moreFiles(namc, namv)) {
      unlinkTempFiles();
      return;      
    }
  }
}

/**************************************************************/

void usage(void) {
  printf("usage: ar -[%s][%s] archive files ...\n", com, opt);
  done(1);
}

int main(int argc, char *argv[]) {  
  char *cp;  
  int res;
  char cmd;

  
  strcpy(tmp0nam, "v0XXXXXX");
  strcpy(tmp1nam, "v1XXXXXX");  
  if (argc < 3 || *argv[1] != '-') {
    usage();
  }
  cmd = '0';
  for (cp = argv[1] + 1; *cp != '\0'; cp++) {
    switch (*cp) {
      case 'd':
        cmd = 'd';  
        break;
      case 'r':
        cmd = 'r';        
        break;      
      case 't':
        cmd = 't';        
        break;       
      case 'x':
        cmd = 'x';        
        break;      
      case 'v':
      case 'u':
      case 'a':
      case 'b':      
      case 's':
        flg[*cp - 'a'] = 1;
        break;
      default:
        fprintf(stderr, "ar: bad option '%c'\n", *cp);
        done(1);
    }
  }  
  if (flg['a' - 'a'] || flg['b' - 'a']) {
    baState = 1;
    posName = basename(strdup(argv[2]));
    arnam = argv[3];    
    namv = &argv[4];
    namc = argc - 4;
    if (argc < 4) {
      usage();
    }
  } else {
    arnam = argv[2];
    namv = &argv[3];
    namc = argc - 3;
  }  
  
  if (cmd == '0' && !flg['s' - 'a']) {
    fprintf(stderr, "ar: one of [%ss] must be specified\n", com);
    done(1);
  }
  res = 0;  
  switch(cmd) {
      case 'd': dCmd();
        break;
      case 'r': rCmd(posName, arnam, namv, namc, flg['a'-'a'], flg['b'-'a'], flg['u'-'a']);
        break;
      case 't': tCmd();
        break;
      case 'x': xCmd();
        break;      
    }
  res = notFound(namc, namv);
  unlinkTempFiles();
  if (res != 0) {
   return res;
  }
  
  if (flg['s' - 'a'] ||
      ((cmd == 'd' || cmd =='r') && hasSymbols(arnam))) {
    strcpy(tmp0nam, "v0XXXXXX");
    strcpy(tmp1nam, "v1XXXXXX");  
    res = updateSymbols(arnam, flg['v' - 'a']);
    unlinkTempFiles();
  }
  return res;
}
