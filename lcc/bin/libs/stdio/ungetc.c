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
#include <string.h>
#include "local.h"
#include "shared_libc.h"

extern	int	_ungetc(int, FILE *);

/*
 * Expand the ungetc buffer `in place'.  That is, adjust fp->_p when
 * the buffer moves, so that it points the same distance from the end,
 * and move the bytes in the buffer around as necessary so that they
 * are all at the end (stack-style).
 */
/*static*/
int
__submore(fp) register FILE *fp;
{
	register int i;
	register unsigned char *p;

	if (fp->_ub._base == fp->_ubuf) {
		/*
		 * Get a new buffer (rather than expanding the old one).
		 */
		if ((p = malloc((size_t)BUFSIZ)) == NULL)
			return (EOF);
		fp->_ub._base = p;
		fp->_ub._size = BUFSIZ;
		p += BUFSIZ - sizeof(fp->_ubuf);
		for (i = sizeof(fp->_ubuf); --i >= 0;)
			p[i] = fp->_ubuf[i];
		fp->_p = p;
		return (0);
	}
	i = fp->_ub._size;
	p = realloc(fp->_ub._base, i << 1);
	if (p == NULL)
		return (EOF);
	(void) memcpy((void *)(p + i), (void *)p, (size_t)i);
	fp->_p = p + i;
	fp->_ub._base = p;
	fp->_ub._size = i << 1;
	return (0);
}

/* ungetc() with "exclusive use" stream locking. */
int
ungetc(int c, FILE *stream)
{
	if (c == EOF)
		return EOF;

        EXCL_START(&stream->_file_lock);
	c = _ungetc(c, stream);
	EXCL_END(&stream->_file_lock);
	return c;
}

/*
 * ungetc without stream locking.
 * This is separate from ungetc() so that vfscanf() can call it
 * to push back characters on an already locked stream.
 */
int
_ungetc(int c, FILE *stream)
{
	if (c == EOF)
		return EOF;

	/* ### pete:
	 *
	 * The standard requires that ungetc function at the end
	 * of a file. The following is an attempt to make it
	 * do that.
	 *
	 * ### yen:  I'm not sure this is the correct thing to do.
	 *
	 */

	if ( stream->_flags & __SEOF )
		stream->_flags &= ~__SEOF;

	if (!__sdidinit)
		__sinit();
	if ((stream->_flags & __SRD) == 0) {
		/*
		 * Not already reading: no good unless reading-and-writing.
		 * Otherwise, flush any current write stuff.
		 */
		if ((stream->_flags & __SRW) == 0) {
			return EOF;
		}
		if (stream->_flags & __SWR) {
			if (_fflush(stream)) {
				return EOF;
			}
			stream->_flags &= ~__SWR;
			stream->_w = 0;
			stream->_lbfsize = 0;
		}
		stream->_flags |= __SRD;
	}
	c = (unsigned char)c;

	/*
	 * If we are in the middle of ungetc'ing, just continue.
	 * This may require expanding the current ungetc buffer.
	 */
	if (HASUB(stream)) {
		if (stream->_r >= stream->_ub._size && __submore(stream)) {
			return EOF;
		}
		*--stream->_p = c;
		stream->_r++;
		return c;
	}

	/*
	 * If we can handle this by simply backing up, do so,
	 * but never replace the original character.
	 * (This makes sscanf() work when scanning `const' data.)
	 */
	if (stream->_bf._base != NULL && stream->_p > stream->_bf._base &&
	    stream->_p[-1] == c) {
		stream->_p--;
		stream->_r++;
		return c;
	}

	/*
	 * Create an ungetc buffer.
	 * Initially, we will use the `reserve' buffer.
	 */
	stream->_ur = stream->_r;
	stream->_up = stream->_p;
	stream->_ub._base = stream->_ubuf;
	stream->_ub._size = sizeof(stream->_ubuf);
	stream->_ubuf[sizeof(stream->_ubuf) - 1] = c;
	stream->_p = &stream->_ubuf[sizeof(stream->_ubuf) - 1];
	stream->_r = 1;
	return c;
}
