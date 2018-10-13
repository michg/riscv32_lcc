/*
 * ranlib.c - archive index generator
 */


#ifndef _RANLIB_H_
#define _RANLIB_H_


int hasSymbols(char *archive);
int updateSymbols(char *archive, int verbose);

void rCmd(char *posName, char* arnam, char **namv, int namc, int aflag, int bflag, int uflag);
int notFound(int namc, char **namv);
char* getfileSymdefs(char *symdefs, char *symbol);
#define TEMP_NAME		"__.SYMDEF" 
 
#endif /* _RANLIB_H_ */
