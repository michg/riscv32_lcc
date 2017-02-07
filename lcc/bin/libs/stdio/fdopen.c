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

/* #include <sys/types.h> */
/* #include <fcntl.h> */
#include <stdio.h>
#include <errno.h>
#include "local.h"
#include "shared_libc.h"


FILE *
_fdopen( int fd, const char *mode)
{
	register FILE *fp;
	int flags, oflags, fdflags, fdmode;

	EXCL_START(&__sF_lock);
	if ((flags = __sflags(mode, &oflags)) == 0) {
		EXCL_END(&__sF_lock);
		return NULL;
	}

	/* make sure the mode the user wants is a subset of the actual mode */
#ifdef F_GETFL
	if ((fdflags = fcntl(fd, F_GETFL, 0)) < 0) {
		EXCL_END(&__sF_lock);
		return NULL;
	}
	fdmode = fdflags & O_ACCMODE;
	if (fdmode != O_RDWR && (fdmode != (oflags & O_ACCMODE))) {
		EXCL_END(&__sF_lock);
		errno = EBADF;
		return NULL;
	}
#endif

	if ((fp = __sfp()) == NULL) {
		EXCL_END(&__sF_lock);
		return NULL;
	}
	fp->_flags = flags;
	/*
	 * If opened for appending, but underlying descriptor
	 * does not have O_APPEND bit set, assert __SAPP so that
	 * __swrite() will lseek to end before each write.
	 */
#ifdef F_GETFL
	if ((oflags & O_APPEND) && !(fdflags & O_APPEND))
#endif
		fp->_flags |= __SAPP;
	fp->_file = fd;
	fp->_cookie = fp;
	fp->_read = __sread;
	fp->_write = __swrite;
	fp->_seek = __sseek;
	fp->_close = __sclose;
	EXCL_END(&__sF_lock);
	return fp;
}
