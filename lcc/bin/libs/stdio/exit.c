/*
 * libc/stdlib/exit.c
 * ANSI/ISO 9899-1990, Section 7.10.4.3.
 *
 * void exit(int status)
 * Exit, flushing stdio buffers if necessary.
 *
 * Copyright (c) 1990 Regents of the University of California.
 * All rights reserved.
 */

#include <stdlib.h>
#include <unistd.h>

#include "atexit.h"

void	(*__cleanup)();

struct atexit **__atexit;

void
exit(int status)
{
	register struct atexit *p;
	register int n;

	for (p = *__atexit; p; p = p->next)
		for (n = p->ind; --n >= 0;)
			(*p->fns[n])();

       /* 
        * ANSI C requires that streams be flushed when exit() is called.
        * __cleanup is set to _cleanup (in findfp.c) which calls _fwalk(fflush)
	* tmdbg (possibly other apps) relies on this behavior. 
	* Do not remove the call to __cleanup.
	*/

	if (__cleanup)
		(*__cleanup)();

	_exit(status);

	/* NOTREACHED */
}

/* end of libc/stdlib/exit.c */
