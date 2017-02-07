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
#include <string.h>
#include "fvwrite.h"
#include "shared_libc.h"

/*
 * Write the given string to the given file.
 */
int
fputs(const char *s, FILE *stream)
{
	struct __suio uio;
	struct __siov iov;
	register int r;

        EXCL_START(&stream->_file_lock);
	iov.iov_base = (char *) s;
	iov.iov_len = uio.uio_resid = strlen(s);
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	r = __sfvwrite(stream, &uio);
        EXCL_END(&stream->_file_lock);
	return r;
}


