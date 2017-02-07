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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "local.h"
#include "shared_libc.h"

FILE *
fopen(const char *filename, const char *mode )
{
	register FILE *fp;
	register int f;
	int flags, oflags;

	EXCL_START(&__sF_lock);
	if (filename == NULL
	    || filename[0] == '\0'
	    || (flags = __sflags(mode, &oflags)) == 0
	    || (fp = __sfp()) == NULL) {
		EXCL_END(&__sF_lock);
		return NULL;
	}
	if ((f = _open(filename, oflags, 0666)) < 0) {
		fp->_flags = 0;	/* release */
		EXCL_END(&__sF_lock);
		return NULL;
	}
	fp->_file = f;
	fp->_flags = flags;
	fp->_cookie = fp;
	fp->_read = __sread;
	fp->_write = __swrite;
	fp->_seek = __sseek;
	fp->_close = __sclose;
	EXCL_END(&__sF_lock);
	return fp;
}


