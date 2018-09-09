/* $Id$ */
#ifndef MEM_INCLUDED
#define MEM_INCLUDED
#include <stdio.h>
#include <stdlib.h>
#define ALLOC(nbytes) malloc(nbytes)
#define CALLOC(count, nbytes) calloc(count, nbytes) 
#define  NEW(p) ((p) = ALLOC((long)sizeof *(p)))
#define NEW0(p) ((p) = CALLOC(1, (long)sizeof *(p))) 
#define RESIZE(ptr, nbytes) 	((ptr) = realloc((ptr), (nbytes))) 
#define FREE(ptr) free(ptr)
#endif
