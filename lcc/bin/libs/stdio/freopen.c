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

/* #include <sys/types.h> */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "local.h"
#include "shared_libc.h"

/*
 * Re-direct an existing, open (probably) file to some other file.
 */

FILE *freopen( const char *filename, const char *mode, FILE *stream )
{
	register int f;
	int wantfd, isopen, flags, oflags, e;

	if (filename == NULL || filename[0] == '\0')
		return NULL;
        EXCL_START(&stream->_file_lock);
	if ((flags = __sflags(mode, &oflags)) == 0) {
		EXCL_END(&stream->_file_lock);
		(void) fclose(stream);
		return NULL;
	}
	if (!__sdidinit)
		__sinit();

	/*
	 * Remember whether the stream was open to begin with, and
	 * which file descriptor (if any) was associated with it.
	 * If it was attached to a descriptor, defer closing it,
	 * so that, e.g., freopen("/dev/stdin", "r", stdin) works.
	 * This is unnecessary if it was not a Unix file.
	 */
	if (stream->_flags == 0) {
		stream->_flags = __SEOF;	/* hold on to it */
		isopen = 0;
		wantfd = -1;
	} else {
		if (stream->_flags & __SWR)
			(void) _fflush(stream);
		/* if close is NULL, closing is a no-op, hence pointless */
		isopen = stream->_close != NULL;
		if ((wantfd = stream->_file) < 0 && isopen) {
			(void) (*stream->_close)(stream->_cookie);
			isopen = 0;
		}
	}

	/*
	 * Now get a new descriptor to refer to the new file.
	 */
	f = _open(filename, oflags, 0666);
	if (f < 0 && isopen) {
		/*
		 * May have used up all descriptors, so close the old
		 * and try again.
		 */
		(void) (*stream->_close)(stream->_cookie);
		isopen = 0;
		f = _open(filename, oflags, 0666);
	}
	e = errno;

	/*
	 * Finish closing stream.  Even if the open succeeded above,
	 * we cannot keep stream->_base: it may be the wrong size.
	 * This loses the effect of any setbuffer calls,
	 * but stdio has always done this before.
	 */
	if (isopen)
		(void) (*stream->_close)(stream->_cookie);
	if (stream->_flags & __SMBF)
		free((char *)stream->_bf._base);
	stream->_w = 0;
	stream->_r = 0;
	stream->_p = NULL;
	stream->_bf._base = NULL;
	stream->_bf._size = 0;
	stream->_lbfsize = 0;
	if (HASUB(stream))
		FREEUB(stream);
	stream->_ub._size = 0;
	if (HASLB(stream))
		FREELB(stream);
	stream->_lb._size = 0;

	if (f < 0) {			/* did not get it after all */
		stream->_flags = 0;		/* set it free */
		errno = e;		/* restore in case _close clobbered */
		EXCL_END(&stream->_file_lock);
		return NULL;
	}

#ifdef NOTDEF
	/*
	 * If reopening something that was open before on a real file,
	 * try to maintain the descriptor.  Various routines (e.g.,
	 * perror) assume that after `freopen(name, mode, stderr)',
	 * fileno(stderr)==2.
	 */
	if (wantfd >= 0 && f != wantfd) {
		if (_dup2(f, wantfd) >= 0) {
			(void) _close(f);
			f = wantfd;
		}
	}
#endif /* NOTDEF */

	stream->_flags = flags;
	stream->_file = f;
	stream->_cookie = stream;
	stream->_read = __sread;
	stream->_write = __swrite;
	stream->_seek = __sseek;
	stream->_close = __sclose;
	EXCL_END(&stream->_file_lock);
	return stream;
}


