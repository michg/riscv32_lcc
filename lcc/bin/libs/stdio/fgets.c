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
#include "shared_libc.h"

/*
 * Read at most n-1 characters from the given file.
 * Stop when a newline has been read, or the count runs out.
 * Return first argument, or NULL if no characters were read.
 */
char *
fgets(char *s, int n, FILE *stream)
{
	size_t len;
	char *sp;
	unsigned char *p, *t;

	if (n < 2)		/* sanity check */
		return NULL;

        EXCL_START(&stream->_file_lock);
	sp = s;
	n--;			/* leave space for NUL */
	do {
		/*
		 * If the buffer is empty, refill it.
		 */
		if ((len = stream->_r) <= 0) {
			if (__srefill(stream)) {
				/* EOF: stop with partial or no line */
				if (sp == s) {
					EXCL_END(&stream->_file_lock);
					return NULL;
				}
				break;
			}
			len = stream->_r;
		}
		p = stream->_p;

		/*
		 * Scan through at most n bytes of the current buffer,
		 * looking for '\n'.  If found, copy up to and including
		 * newline, and stop.  Otherwise, copy entire chunk
		 * and loop.
		 */
		if (len > n)
			len = n;
		t = memchr((void *)p, '\n', len);
		if (t != NULL) {
			len = ++t - p;
			stream->_r -= len;
			stream->_p = t;
			(void) memcpy((void *)sp, (void *)p, len);
			sp[len] = 0;
			EXCL_END(&stream->_file_lock);
			return s;
		}
		stream->_r -= len;
		stream->_p += len;
		(void) memcpy((void *)sp, (void *)p, len);
		sp += len;
	} while ((n -= len) != 0);
	*sp = 0;
        EXCL_END(&stream->_file_lock);
	return s;
}
