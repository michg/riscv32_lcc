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
#include <unistd.h>
/* #include <sys/types.h> */
#include <sys/stat.h>
#include "local.h"

/*
 * Allocate a file buffer, or switch to unbuffered I/O.
 * Per the ANSI C standard, ALL tty devices default to line buffered.
 *
 * As a side effect, we set __SOPT or __SNPT (en/dis-able fseek
 * optimisation) right after the _fstat() that finds the buffer size.
 */
void
__smakebuf( register FILE *fp )
{
	register size_t size, couldbetty;
	register void *p;
	struct stat st;

#if 1
	if (fp->_flags & __SNBF) {
		fp->_bf._base = fp->_p = fp->_nbuf;
		fp->_bf._size = 1;
		return;
	}
	if (fp->_file < 0 || _fstat(fp->_file, &st) < 0) {
		couldbetty = 0;
		size = BUFSIZ;
		/* do not try to optimise fseek() */
		fp->_flags |= __SNPT;
	} else {
		couldbetty = (st.st_mode & S_IFMT) == S_IFCHR;
		size = st.st_blksize <= 0 ? BUFSIZ : st.st_blksize;
		/*
		 * Optimise fseek() only if it is a regular file.
		 * (The test for __sseek is mainly paranoia.)
		 */
		if ((st.st_mode & S_IFMT) == S_IFREG &&
		    fp->_seek == __sseek) {
			fp->_flags |= __SOPT;
			fp->_blksize = st.st_blksize;
		} else
			fp->_flags |= __SNPT;
	}
#else
        couldbetty = 0;
        size = BUFSIZ;
        fp->_flags |= __SNPT;
#endif

	if ((p = malloc(size)) == NULL) {
		fp->_flags |= __SNBF;
		fp->_bf._base = fp->_p = fp->_nbuf;
		fp->_bf._size = 1;
	} 
        else 
        {
		__cleanup = _cleanup;
		fp->_flags |= __SMBF;
		fp->_bf._base = fp->_p = p;
		fp->_bf._size = size;
		if (couldbetty && _isatty(fp->_file))
			fp->_flags |= __SLBF;
	}
}
