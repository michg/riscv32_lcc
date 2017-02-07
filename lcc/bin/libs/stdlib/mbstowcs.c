/*
 * ansi/stdlib/mbstowcs.c
 * ANSI/ISO 9899-1990, Section 7.10.8.1.
 *
 * size_t mbstowcs(wchar_t *pwcs, const char *s, size_t n)
 * Convert multibyte character string to wide character string.
 * This uses mbtowc(), thus assuming the encoding used is not state-dependent.
 */

#include <stdlib.h>

size_t
mbstowcs(wchar_t *pwcs, const char *s, size_t n)
{
	register int count, len;

	if (s == NULL)
		return 0;		/* encoding is not state-dependent */
	count = 0;
	while (*s != 0 && count < n) {
		len = mbtowc(pwcs++, s, MB_CUR_MAX);
		if (len == -1)
			return (size_t) -1;	/* invalid */
		++count;
		s += len;
	}

	/* Null-terminate the wide string if there is space. */
	if (count < n)
		*pwcs = (wchar_t) 0;

	return count;
}

/* end of mbstowcs.c */
