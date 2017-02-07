/*
 * ansi/stdlib/wctomb.c
 * ANSI/ISO 9899-1990, Section 7.10.7.3.
 *
 * int wctomb(char *s, wchar_t wchar)
 * Convert wide character to multibyte character.
 * Assumes multibyte encoding as defined in mb_encode.h.
 */

#include <stdlib.h>
#include "mb_encode.h"

int
wctomb(char *s, wchar_t wchar)
{
	if (s == NULL)
		return 0;		/* encoding is not state-dependent */
	if (is1byte(wchar)) {		/* w encodes in one character */
		*s = (char) wchar;
		return 1;
	} else {			/* w encodes in three characters */
		*s++ = byte1(wchar);
		*s++ = byte2(wchar);
		*s   = byte3(wchar);
		return 3;
	}
}

/* end of wctomb.c */
