/*
 * lib-src/ansi/stdlib/strtoul.c
 * ANSI/ISO 9899-1990, Sections 7.10.1.5 and 7.10.1.6.
 *
 * long strtol(const char *nptr, char **endptr, int base)
 * unsigned long strtoul(const char *nptr, char **endptr, int base)
 * Return [unsigned] long value represented by string at nptr in given base
 * and store a pointer past the recognized string through endptr.
 *
 * This source is only for the "C" locale at present.
 */

#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

/* Flag bits. */
#define	SIGNED		0x00		/* if strtol()		*/
#define	UNSIGNED	0x01		/* if strtoul()		*/
#define	SIGN		0x02		/* if '-' given		*/
#define	SUCCESS		0x04		/* if sequence accepted	*/
#define	OVERFLOW	0x08		/* if overflow occurs	*/

/*
 * _strtoul() does the work both for strtoul() (with flag == UNSIGNED)
 * and for strtol() (with flag == SIGNED).
 */
static
unsigned long
_strtoul(const char *nptr, char **endptr, int base, int flag)
{
	unsigned long result, max_div_base, max_mod_base;
	char digit, c1, c2;
	int maxdigit, maxlower, maxupper;
	const char *start;

	start = nptr;

#if	0
	/* This case is not convered by the Standard. */
	if (nptr == NULL) {
		errno = ERANGE;
		if (endptr != NULL)
			*endptr = (char *)start;
		return 0L;
	}
#endif

	/* Skip leading white space. */
	while (isspace(*nptr))
		nptr++;

	/* Skip optional sign. */
	if (*nptr == '+')
		nptr++;
	else if (*nptr == '-'){
		nptr++;
		flag |= SIGN;
	}

	/* Determine the base if not given explicitly. */
	if (base == 0) {
		if (*nptr == '0') {
			c1 = *(nptr + 1);
			c2 = (c1 == 0) ? 0 : *(nptr + 2);
			if ('0' <= c1 && c1 <= '7')
				base = 8;	/* octal */
			else if ((c1 == 'x' || c1 == 'X')
				&& (isdigit(c2)
				 || ('a' <= c2 && c2 <= 'f')
				 || ('A' <= c2 && c2 <= 'F')))
				base = 16;	/* hexadecimal */
		}
		if (base == 0)
			base = 10;		/* default base: decimal */
	}


	/* Ignore optional "0x" or "0X" on hex input. */
	if (base == 16 && *nptr == '0') {
		c1 = *(nptr + 1);
		c2 = (c1 == 0) ? 0 : *(nptr + 2);
		if ((c1 == 'x' || c1 == 'X')
		  && (isdigit(c2)
		   || ('a' <= c2 && c2 <= 'f')
		   || ('A' <= c2 && c2 <= 'F')))
			nptr += 2;
	}

	/* Set up limits for accepted input character values. */
	if (base <= 10) {
		maxdigit = '0' + base - 1;
		maxlower = maxupper = 0;
	} else {
		maxdigit = '9';
		maxlower = 'a' + base - 11;
		maxupper = 'A' + base - 11;
	}

	/* Set up overflow limits. */
	if (flag & UNSIGNED) {
		max_div_base = ULONG_MAX / (unsigned long)base;
		max_mod_base = ULONG_MAX % (unsigned long)base;
	} else {
		max_div_base = (unsigned long)LONG_MAX / (unsigned long)base;
		max_mod_base = (unsigned long)LONG_MAX % (unsigned long)base;
	}

	/* Process a nonempty sequence of digits in the given base. */
	for (result = 0L; (digit = *nptr) != 0; ++nptr) {
		if (isdigit(digit) && digit <= maxdigit)
			digit -= '0';		/* numeric digit */
		else if (islower(digit) && digit <= maxlower)
			digit -= 'a' - 10;	/* lower case digit */
		else if (isupper(digit) && digit <= maxupper)
			digit -= 'A' - 10;	/* upper case digit */
		else
			break;
		flag |= SUCCESS;		/* digit accepted */
		if (result < max_div_base
		 || (result == max_div_base && digit <= max_mod_base))
			result = result * base + digit;
		else
			flag |= OVERFLOW;
	}

	/* Store end pointer and return result. */
	if (endptr != NULL)
		*endptr = (char *)((flag & SUCCESS) ? nptr : start);
	if (flag & OVERFLOW) {
		errno = ERANGE;
		if (flag & UNSIGNED)
			return ULONG_MAX;
		return (unsigned long)((flag & SIGN) ? LONG_MIN : LONG_MAX);
	}
	return (flag & SIGN) ? -result : result;
}

long
strtol(const char *nptr, char **endptr, int base)
{
	return (long)_strtoul(nptr, endptr, base, SIGNED);
} 

unsigned long
strtoul(const char *nptr, char **endptr, int base)
{
	return _strtoul(nptr, endptr, base, UNSIGNED);
}
