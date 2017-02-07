/*
 * ansi/stdlib/wcstombs.c
 * ANSI/ISO 9899-1990, Section 7.10.8.2.
 *
 * size_t wcstombs(char *s, const wchar_t *pwcs, size_t n)
 * Convert wide character string to multibyte character string.
 * This uses wctomb(), thus assuming the encoding used is not state-dependent.
 */

#include <stdlib.h>

size_t
wcstombs(char *s, const wchar_t *pwcs, size_t n)
{
	register int count, len;
	register char *cp;
	char buf[MB_CUR_MAX];

	if (s == NULL)
		return 0;		/* encoding is not state-dependent */
	count = 0;
	while (*pwcs != (wchar_t) 0 && count < n) {
		len = wctomb(buf, *pwcs++);
		if (len == -1)
			return (size_t) -1;	/* invalid */
		if (count + len > n)
			break;			/* out of space, quit */
		count += len;
		for (cp = buf; len-- > 0; )
			*s++ = *cp++;		/* copy converted value */
	}

	/* Null-terminate the string if there is space. */
	if (count < n)
		*s = 0;

	return count;
}

/* end of wcstombs.c */
