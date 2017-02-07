/*
 * lib-src/ansi/stdlib/atol.c
 * ANSI/ISO 9899-1990, Section 7.10.1.3.
 *
 * long atol(const char *nptr)
 * Return a long containing the value represented by the string at nptr.
 */

#include <stdlib.h>
#include <ctype.h>

long
atol(const char *nptr)
{
	register long	result;
	register int	signflag;
	char		digit;

	signflag = 0;

	/* Skip leading white space. */
	while (isspace(*nptr))
		nptr++;

	/* Skip optional sign. */
	if (*nptr == '+')
		nptr++;
	else if (*nptr == '-') {
		nptr++;
		signflag++;
	}

	/* Process a nonempty sequence of digits in base 10. */
	for (result = 0; (digit = *nptr++) != 0; ) {
		if (isdigit(digit))
			result = result * 10 + digit - '0';
		else
			break;
	}

	/* Return result. */
	return (signflag) ? -result : result;
}

/* end of libc/stdlib/atol.c */
