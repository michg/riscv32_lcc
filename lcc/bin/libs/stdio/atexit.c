/*
 * lib-src/ansi/stdlib/atexit.c
 * ANSI/ISO 9899-1990, Section 7.10.4.2.
 *
 * int atexit(void (*func)(void))
 * Register function func to be performed on exit.
 *
 * Copyright (c) 1990 Regents of the University of California.
 * All rights reserved.
 */

#include <stdlib.h>
#include <stddef.h>
#include "atexit.h"

extern struct atexit **__atexit;

int
atexit(void (*func)(void))
{
	register struct atexit *p = *__atexit;

	if (p == NULL || p->ind >= ATEXIT_SIZE) {
		if ((p = malloc(sizeof *p)) == NULL)
			return -1;		/* failure */
		p->ind = 0;
		p->next = *__atexit;
		*__atexit = p;
	}
	p->fns[p->ind++] = func;
	return 0;				/* success */
}
