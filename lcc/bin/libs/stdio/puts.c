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
 * Write the given string to stdout, appending a newline.
 */

int
puts(const char *s)
{
	register int r;
	size_t c;
	struct __suio uio;
	struct __siov iov[2];

	c = strlen(s);
	iov[0].iov_base = (char *) s;
	iov[0].iov_len = c;
	iov[1].iov_base = "\n";
	iov[1].iov_len = 1;
	uio.uio_resid = c + 1;
	uio.uio_iov = &iov[0];
	uio.uio_iovcnt = 2;
        EXCL_START(&stdout->_file_lock);
	r = __sfvwrite(stdout, &uio) ? EOF : '\n';
        EXCL_END(&stdout->_file_lock);
	return r;
}
