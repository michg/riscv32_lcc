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
#include <errno.h>
#include "local.h"
#include "shared_libc.h"

/*
 * ftell: return current offset.
 */
long
ftell(FILE *stream)
{
	fpos_t pos;

fflush(stream);
        EXCL_START(&stream->_file_lock);
	if (stream->_flags == 0 || stream->_seek == NULL) {
		EXCL_END(&stream->_file_lock);
		errno = ESPIPE;
		return -1L;
	}

	/*
	 * Find offset of underlying I/O object, then
	 * adjust for buffered bytes.
	 */
	if (stream->_flags & __SOFF)
		pos = stream->_offset;
	else {
		if ((stream->_flags & __SWR)
		 && stream->_p != NULL
		 && ((stream->_flags & __SBIN) != __SBIN)) {
			/*
			 * The code below returns the current seek position
			 * plus the buffer offset if writing.
			 * This is wrong for text mode on a system
			 * where text and binary modes differ,
			 * so the write buffer must be flushed on such systems.
			 * Always do the flush if writing to a buffered text stream,
			 * since the library is host-independent.
			 */
			_fflush(stream);
		}
		pos = (*stream->_seek)(stream->_cookie, (fpos_t)0, SEEK_CUR);
		if (pos == -1L) {
			EXCL_END(&stream->_file_lock);
			return pos;
		}
	}
	if (stream->_flags & __SRD) {
		/*
		 * Reading.  Any unread characters (including
		 * those from ungetc) cause the position to be
		 * smaller than that in the underlying object.
		 */
		pos -= stream->_r;
		if (HASUB(stream))
			pos -= stream->_ur;
	} else if ((stream->_flags & __SWR) && stream->_p != NULL) {
		/*
		 * Writing.  Any buffered characters cause the
		 * position to be greater than that in the
		 * underlying object.
		 */
		pos += stream->_p - stream->_bf._base;
	}
	EXCL_END(&stream->_file_lock);
	return pos;
}
