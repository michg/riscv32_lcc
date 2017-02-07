/*
 * ansi/stdlib/mbtowc.c
 * ANSI/ISO 9899-1990, Section 7.10.7.2.
 *
 * int mbtowc(wchar_t *pwc, const char *s, size_t n)
 * Convert multibyte character to wide character.
 * Assumes multibyte encoding as defined in mb_encode.h.
 */

#include <stdlib.h>
#include "mb_encode.h"

int
mbtowc(wchar_t *pwc, const char *s, size_t n)
{
	register wchar_t w;
	register int count;
	register unsigned int b1, b2, b3;

	if (s == NULL)
		return 0;		/* encoding is not state-dependent */
	b1 = *(unsigned char *)s;
	if (b1 == 0) {
		if (pwc != (wchar_t *)NULL)
			*pwc = 0;
		return 0;		/* *s is null character */
	} else if (n == 0)
		return -1;		/* 0 chars are not a valid encoding */
	else if (is1byte(b1)) {		/* *s encodes in one byte */
		count = 1;
		w = (wchar_t) b1;
	} else if (n < 3)
		return -1;		/* *s encodes in 3 but n < 3 */
	else {
		count = 3;
		b2 = * (unsigned char *) ++s;
		b3 = * (unsigned char *) ++s;
		if (!is3byte(b1, b2, b3))
			return -1;	/* s is not a valid encoding string */
		w = (wchar_t) decode(b1, b2, b3);
	}
	if (pwc != (wchar_t *)NULL)
		*pwc = w;
	return count;
}

/* end of mbtowc.c */
