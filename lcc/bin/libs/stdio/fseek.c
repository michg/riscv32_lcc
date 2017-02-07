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
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "local.h"
#include "shared_libc.h"

#define	POS_ERR	(-(fpos_t)1)

/*
 * Seek the given file to the given offset.
 * `Whence' must be one of the three SEEK_* macros.
 */
int
fseek(FILE *stream, long int offset, int whence)
{
#if __STDC__
	fpos_t (*seekfn)(void *, fpos_t, int);
#else
	fpos_t (*seekfn)();
#endif
	fpos_t target, curoff;
	size_t n;
	struct stat st;
	int havepos;

	EXCL_START(&stream->_file_lock);

	/* make sure stdio is set up */
	if (!__sdidinit)
		__sinit();

	/*
	 * Have to be able to seek.
	 */
	if ((seekfn = stream->_seek) == NULL) {
		EXCL_END(&stream->_file_lock);
		errno = ESPIPE;	/* ??? */
		return EOF;
	}

	/*
	 * Change any SEEK_CUR to SEEK_SET, and check `whence' argument.
	 * After this, whence is either SEEK_SET or SEEK_END.
	 */
	switch (whence) {

	case SEEK_CUR:
		/*
		 * In order to seek relative to the current stream offset,
		 * we have to first find the current stream offset a la
		 * ftell (see ftell for details).
		 */
		if (stream->_flags & __SOFF)
			curoff = stream->_offset;
		else {
			curoff = (*seekfn)(stream->_cookie, (fpos_t)0, SEEK_CUR);
			if (curoff == -1L) {
				EXCL_END(&stream->_file_lock);
				return EOF;
			}
		}
		if (stream->_flags & __SRD) {
			if (HASUB(stream))
				curoff -= stream->_ur;
			else
				curoff -= stream->_r;
		} else if (stream->_flags & __SWR && stream->_p != NULL)
			curoff += stream->_p - stream->_bf._base;

		offset += curoff;
		whence = SEEK_SET;
		havepos = 1;
		break;

	case SEEK_SET:
	case SEEK_END:
		havepos = 0;
		break;

	default:
		EXCL_END(&stream->_file_lock);
		errno = EINVAL;
		return EOF;
	}

	/*
	 * Can only optimise if:
	 *	reading (and not reading-and-writing);
	 *	not unbuffered; and
	 *	this is a `regular' Unix file (and hence seekfn==__sseek).
	 * We must check __NBF first, because it is possible to have __NBF
	 * and __SOPT both set.
	 */
	if (stream->_bf._base == NULL)
		__smakebuf(stream);
	if (stream->_flags & (__SWR | __SRW | __SNBF | __SNPT))
		goto dumb;
	if ((stream->_flags & __SOPT) == 0) {
		if (seekfn != __sseek ||
		    stream->_file < 0 || _fstat(stream->_file, &st) ||
		    (st.st_mode & S_IFMT) != S_IFREG) {
			stream->_flags |= __SNPT;
			goto dumb;
		}
		stream->_blksize = st.st_blksize;
		stream->_flags |= __SOPT;
	}

	/*
	 * We are reading; we can try to optimise.
	 * Figure out where we are going and where we are now.
	 */
	if (whence == SEEK_SET)
		target = offset;
	else {
		if (_fstat(stream->_file, &st))
			goto dumb;
		target = st.st_size + offset;
	}

	if (!havepos) {
		if (stream->_flags & __SOFF)
			curoff = stream->_offset;
		else {
			curoff = (*seekfn)(stream->_cookie, 0L, SEEK_CUR);
			if (curoff == POS_ERR)
				goto dumb;
		}
		if (HASUB(stream))
			curoff -= stream->_ur;
		else
			curoff -= stream->_r;
	}

	/*
	 * Compute the number of bytes in the input buffer (pretending
	 * that any ungetc() input has been discarded).  Adjust current
	 * offset backwards by this count so that it represents the
	 * file offset for the first byte in the current input buffer.
	 */
	if (HASUB(stream)) {
		n = stream->_up - stream->_bf._base;
		curoff -= n;
		n += stream->_ur;
	} else {
		n = stream->_p - stream->_bf._base;
		curoff -= n;
		n += stream->_r;
	}

	/*
	 * If the target offset is within the current buffer,
	 * simply adjust the pointers, clear EOF, undo ungetc(),
	 * and return.  (If the buffer was modified, we have to
	 * skip this; see fgetline.c.)
	 */
	if ((stream->_flags & __SMOD) == 0 &&
	    target >= curoff && target < curoff + n) {
		register int o = target - curoff;

		stream->_p = stream->_bf._base + o;
		stream->_r = n - o;
		if (HASUB(stream))
			FREEUB(stream);
		stream->_flags &= ~__SEOF;
		EXCL_END(&stream->_file_lock);
		return 0;
	}

	/*
	 * The place we want to get to is not within the current buffer,
	 * but we can still be kind to the kernel copyout mechanism.
	 * By aligning the file offset to a block boundary, we can let
	 * the kernel use the VM hardware to map pages instead of
	 * copying bytes laboriously.  Using a block boundary also
	 * ensures that we only read one block, rather than two.
	 */
	curoff = target & ~(stream->_blksize - 1);
	if ((*seekfn)(stream->_cookie, curoff, SEEK_SET) == POS_ERR)
		goto dumb;
	stream->_r = 0;
	if (HASUB(stream))
		FREEUB(stream);
	stream->_flags &= ~__SEOF;
	n = target - curoff;
	if (n) {
		if (__srefill(stream) || stream->_r < n)
			goto dumb;
		stream->_p += n;
		stream->_r -= n;
	}
	EXCL_END(&stream->_file_lock);
	return 0;

	/*
	 * We get here if we cannot optimise the seek ... just
	 * do it.  Allow the seek function to change stream->_bf._base.
	 */
dumb:
	if (_fflush(stream) ||
	    (*seekfn)(stream->_cookie, offset, whence) == POS_ERR) {
		EXCL_END(&stream->_file_lock);
		return EOF;
	}
	/* success: clear EOF indicator and discard ungetc() data */
	if (HASUB(stream))
		FREEUB(stream);
	stream->_p = stream->_bf._base;
	stream->_r = 0;
	/* stream->_w = 0; */	/* unnecessary (I think...) */
	stream->_flags &= ~__SEOF;
	EXCL_END(&stream->_file_lock);
	return 0;
}
