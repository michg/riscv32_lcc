/*
 * ansi/stdlib/mblen.c
 * ANSI/ISO 9899-1990, Section 7.10.7.1.
 *
 * int mblen(const char *s, size_t n)
 * Determine the number of bytes in a multibyte character.
 * This uses mbtowc(), thus assuming the encoding used is not state-dependent.
 */

#include <stdlib.h>

int
mblen(const char *s, size_t n)
{
	if (s == NULL)
		return 0;		/* encoding is not state-dependent */
	return mbtowc((wchar_t *)0, s, n);
}

/* end of mblen.c */
