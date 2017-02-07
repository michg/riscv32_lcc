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
#include "local.h"
#include "shared_libc.h"

/*
 * Flush a single file, or (if stream is NULL) all files.
 */
int
fflush( FILE *stream )
{
        int result;

	if (stream == NULL)
		return _fwalk(fflush);

        EXCL_START(&stream->_file_lock);
        result= _fflush(stream);
        EXCL_END(&stream->_file_lock);

        return result;
}


int
_fflush( FILE *stream )
{
	unsigned char *p;
	int n, t;

	t = stream->_flags;
	if ((t & __SWR) == 0 || (p = stream->_bf._base) == NULL) {
		return 0;
	}
	n = stream->_p - p;		/* write this much */

	/*
	 * Set these immediately to avoid problems with longjmp
	 * and to allow exchange buffering (via setvbuf) in user
	 * write function.
	 */
	stream->_p = p;
	stream->_w = t & (__SLBF|__SNBF) ? 0 : stream->_bf._size;

	while (n > 0) {
		t = (*stream->_write)(stream->_cookie, (char *)p, n);
		if (t <= 0) {
			stream->_flags |= __SERR;
			return EOF;
		}
		p += t;
		n -= t;
	}
	return 0;
}


