/*
 * Copyright (c) 1990 Regents of the University of California.
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
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* #include <sys/types.h> */
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

/*
 * Return the (stdio) flags for a given mode.  Store the flags
 * to be passed to an open() syscall through *optr.
 * Return 0 on error.
 */
int
__sflags(mode, optr)
	register char *mode;
	int *optr;
{
	register int ret, m, o, append, binary;

	append = 0;
	switch (*mode++) {

	case 'r':	/* open for reading */
		ret = __SRD;
		m = O_RDONLY;
		o = 0;
		break;

	case 'w':	/* open for writing */
		ret = __SWR;
		m = O_WRONLY;
		o = O_CREAT | O_TRUNC;
		break;

	case 'a':	/* open for appending */
		append = __SAPM;
		ret = __SWR;
		m = O_WRONLY;
		o = O_CREAT | O_APPEND;
		break;

	default:	/* illegal mode */
		errno = EINVAL;
		return 0;
	}

	binary = 0;
	if (*mode != '\0' && *mode == 'b') {
		mode++;
		binary = O_BINARY;
	}
	
	/* [rwa]\+ or [rwa]b\+ means read and write */
	if (*mode != '\0' && *mode == '+') {
		mode++;
		ret = __SRW;
		m = O_RDWR;
	}
	
	/*
	 * as well [rwa]+b as [rwa]b+ should be supported
	 */
	if (*mode != '\0' && *mode == 'b') {
		binary = O_BINARY;
	}

	*optr = m | o | binary;
	return ret | append;
}
