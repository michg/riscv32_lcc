/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "local.h"
#include "glue.h"

int	__sdidinit = 0;

#define NSTATIC	20	/* stdin + stdout + stderr + the usual */
#define	NDYNAMIC 10	/* add ten more whenever necessary */

#define	std(flags, file) \
	{0,0,0,flags,file,{0},0, __sF+file,__sread,__swrite,__sseek,__sclose}
/*	 p r w flags file _bf z     cookie    read    write    seek    close */

static FILE usual[NSTATIC - 3];	/* the usual */
static struct glue uglue = { 0, NSTATIC - 3, usual };

FILE __sF[3] = {
	std(__SRD, 0),		/* stdin */
	std(__SWR|__SLBF|__SRW, 1),		/* stdout */
	std(__SWR|__SNBF|__SRW, 2)	/* stderr */
};
struct glue __sglue = { &uglue, 3, __sF };	/* Root of glue chain */

struct glue *
__sfmoreglue( register int n )
{
	register struct glue *g;
	register FILE *p;
	static FILE empty;

	g = (struct glue *)malloc(sizeof(*g) + n * sizeof(FILE));
	if (g == NULL)
		return (NULL);
	p = (FILE *)(g + 1);
	g->next = NULL;
	g->niobs = n;
	g->iobs = p;
	while (--n >= 0)
		*p++ = empty;
	return (g);
}

/*
 * Find a free FILE for fopen et al.
 */
FILE *
__sfp(void)
{
	register FILE *fp;
	register int n;
	register struct glue *g;

	if (!__sdidinit)
		__sinit();
	for (g = &__sglue;; g = g->next) {
		for (fp = g->iobs, n = g->niobs; --n >= 0; fp++)
			if (fp->_flags == 0)
				goto found;
		if (g->next == NULL &&
		   (g->next = __sfmoreglue(NDYNAMIC)) == NULL)
			break;
	}
	errno = ENOMEM;
	return (NULL);
found:
	fp->_flags = 1;		/* reserve this slot; caller sets real flags */
	fp->_p = NULL;		/* no current pointer */
	fp->_w = 0;		/* nothing to read or write */
	fp->_r = 0;
	fp->_bf._base = NULL;	/* no buffer */
	fp->_bf._size = 0;
	fp->_lbfsize = 0;	/* not line buffered */
	fp->_file = -1;		/* no file */
/*	fp->_cookie = <any>; */	/* caller sets cookie, _read/_write etc */
	fp->_ub._base = NULL;	/* no ungetc buffer */
	fp->_ub._size = 0;
	fp->_lb._base = NULL;	/* no line buffer */
	fp->_lb._size = 0;
	return (fp);
}

static
int
funlock(FILE *fp)
{
	return 0;
}

/*
 * exit() calls _cleanup() through *__cleanup, set whenever we
 * open or buffer a file.  This chicanery is done so that programs
 * that do not use stdio need not link it all in.
 *
 * The name `_cleanup' is, alas, fairly well known outside stdio.
 */
void
_cleanup(void)
{

	/* (void) _fwalk(fclose); */
	(void) _fwalk(funlock);		/* force to be unlocked */
	(void) _fwalk(fflush);		/* `cheating' */
}

/*
 * __sinit() is called whenever stdio's internal variables must be set up.
 */
void
__sinit(void)
{

	/* make sure we clean up on exit */
	__cleanup = _cleanup;		/* conservative */
	__sdidinit = 1;
}


/*
 * Initialization.
 * This gets executed the first time any of the system calls
 * which reference a file descriptor get called.
 */

int 
_init_io(void)
{
	return 1;
}
