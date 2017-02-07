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


size_t
_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	register size_t	resid;
	char		*p;
	int		r;
	size_t		total;

	if ((resid = nmemb * size) == 0)
		return resid;
	if (stream->_r < 0)
		stream->_r = 0;
	total = resid;
	p = ptr;
	while (resid > (r = stream->_r)) {
		(void) memcpy((void *)p, (void *)stream->_p, (size_t)r);
		stream->_p += r;
		/* stream->_r = 0 ... done in __srefill */
		p += r;
		resid -= r;
		if (__srefill(stream)) {
			/* no more input: return partial result */
			return ((total - resid) / size);
		}
	}
	(void) memcpy((void *)p, (void *)stream->_p, resid);
	stream->_r -= resid;
	stream->_p += resid;

	return nmemb;
}


size_t
fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
        size_t result;
        EXCL_START(&stream->_file_lock);
        result= _fread(ptr,size,nmemb,stream);
        EXCL_END(&stream->_file_lock);
        return result;
}


