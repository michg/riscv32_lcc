/*
 * lib-src/ansi/stdlib/atoi.c
 * ANSI/ISO 9899-1990, Section 7.10.1.2.
 *
 * int atoi(const char *nptr)
 * Return an int containing the value represented by the string at nptr.
 */

#include <stdlib.h>
#include <ctype.h>

int
atoi(const char *nptr)
{
	register int	result;
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

/* end of libc/stdlib/atoi.c */
