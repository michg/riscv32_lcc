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
#include "local.h"
#include "shared_libc.h"

/*
 * Set one of the three kinds of buffering, optionally including
 * a buffer.
 */
int
setvbuf( FILE *stream, char *buf, int mode, size_t size )
{
	/*
	 * Verify arguments.  The `int' limit on `size' is due to this
	 * particular implementation.
	 */
	if ((mode != _IOFBF && mode != _IOLBF && mode != _IONBF) ||
	    (int)size < 0)
		return EOF;

	/*
	 * Write current buffer, if any; drop read count, if any.
	 * Make sure putc() will not think stream is line buffered.
	 * Free old buffer if it was from malloc().  Clear line and
	 * non buffer flags, and clear malloc flag.
	 */
        EXCL_START(&stream->_file_lock);
	(void) _fflush(stream);
	stream->_r = 0;
	stream->_lbfsize = 0;
	if (stream->_flags & __SMBF)
		free((void *)stream->_bf._base);
	stream->_flags &= ~(__SLBF|__SNBF|__SMBF);

	/*
	 * Now put back whichever flag is needed, and fix _lbfsize
	 * if line buffered.  Ensure output flush on exit if the
	 * stream will be buffered at all.
	 */
	switch (mode) {

	case _IONBF:
		stream->_flags |= __SNBF;
		stream->_bf._base = stream->_p = stream->_nbuf;
		stream->_bf._size = 1;
		break;

	case _IOLBF:
		stream->_flags |= __SLBF;
		stream->_lbfsize = -size;
		/* FALLTHROUGH */

	case _IOFBF:
		/* no flag */
		__cleanup = _cleanup;
		stream->_bf._base = stream->_p = (unsigned char *)buf;
		stream->_bf._size = size;
		break;
	}

	/*
	 * Patch up write count if necessary.
	 */
	if (stream->_flags & __SWR)
		stream->_w = stream->_flags & (__SLBF|__SNBF) ? 0 : size;

        EXCL_END(&stream->_file_lock);
	return 0;
}
