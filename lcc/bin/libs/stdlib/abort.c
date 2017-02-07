/*
 * lib-src/ansi/stdlib/abort.c
 * ANSI/ISO 9899-1990, Section 7.10.4.1.
 *
 * void abort(void)
 * Abort with SIGABRT.
 */

#include <stdlib.h>
#include <signal.h>

void
abort(void)
{
	_exit(1);
}
