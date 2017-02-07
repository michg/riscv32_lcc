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
 * Close a file.
 */
int
fclose(register FILE *stream)
{
	int r;

	if (stream == NULL)
		return 0;
	if (stream->_flags == 0)	/* not open! */
		return 0;

        EXCL_START(&stream->_file_lock);
	r = stream->_flags & __SWR ? _fflush(stream) : 0;

	if (stream->_close != NULL && (*stream->_close)(stream->_cookie) < 0)
		r = EOF;

	if (stream->_flags & __SMBF)
		free((char *)stream->_bf._base);
	if (HASUB(stream))
		FREEUB(stream);
	if (HASLB(stream))
		FREELB(stream);
	stream->_flags = 0;		/* release this FILE for reuse */
        EXCL_END(&stream->_file_lock);

	return r;
}
