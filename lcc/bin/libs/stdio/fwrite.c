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
#include "local.h"
#include "fvwrite.h"
#include "shared_libc.h"

/*
 * Write `count' objects (each size `size') from memory to the given file.
 * Return the number of whole objects written.
 */
size_t
fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t n;
	struct __suio uio;
	struct __siov iov;

        EXCL_START(&stream->_file_lock);
	iov.iov_base = (char *) ptr;
	uio.uio_resid = iov.iov_len = n = nmemb * size;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;

	/*
	 * The usual case is success (__sfvwrite returns 0);
	 * skip the divide if this happens, since divides are
	 * generally slow and since this occurs whenever size==0.
	 */
	if (__sfvwrite(stream, &uio) == 0) {
		EXCL_END(&stream->_file_lock);
		return nmemb;
	}
        EXCL_END(&stream->_file_lock);
	return (n - uio.uio_resid) / size;
}


