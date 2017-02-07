/*
 * lib-src/ansi/stdlib/strtod.c
 * ANSI/ISO 9899-1990, Section 7.10.1.4.
 *
 * double strtod(const char *nptr, char **endptr)
 * Return a double containing the value represented by the string at nptr
 * and store a pointer to the first unrecognized character in *endptr.
 *
 * This source is only for the "C" locale at present.
 */

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>

/*
 * Number of decimal digits in max legal exponent, for overflow/underflow check.
 * This should really be in <float.h>, as it depends on the representation.
 * For the truly pedantic, this is something like
 *	(int)(max(log10(-DBL_MIN_10_EXP), log10(DBL_MAX_10_EXP))) + 1
 */
#if	defined(__IEEE_SP_FP__)
#define	DBL_EXP_10_DIG	2
#elif	defined(__IEEE_DP_FP__)
#define	DBL_EXP_10_DIG	3
#else
#error	!defined(__IEEE_SP_FP__) && !defined(__IEEE_DP_FP__)
#endif	/* defined(__IEEE_SP_FP__) */

/* Flag bits. */
#define	SIGN		0x01		/* if leading '-'		*/
#define	DIGITSEEN	0x02		/* if significand digit seen	*/
#define	DOTSEEN		0x04		/* if '.' seen			*/
#define	TOOBIG		0x08		/* if significand too big for ulong */
#define	EXPSIGN		0x10		/* if exponent sign '-'		*/
#define	OVERFLOW	0x20		/* if result overflows		*/
#define	UNDERFLOW	0x40		/* if result underflows		*/

double
strtod(const char *nptr, char **endptr)
{
	register double value;
	register unsigned int c;
	register const char *endp;
	register int flags, decexp, expexp, digits;
	unsigned long ul;

	/* Initialize flags, result value, end pointer, current character. */
	flags = 0;
	value = 0.0;
	endp = nptr;
	c = (unsigned int)*nptr++;

	/* Ignore initial whitespace. */
	while (isspace(c))
		c = (unsigned int)*nptr++;

	/* Look for leading sign. */
	if (c == '+' || c == '-') {
		if (c == '-')
			flags |= SIGN;		/* negate result */
		c = (unsigned int)*nptr++;
	}
	if (c != '.' && !isdigit(c))
		goto done2;			/* no valid subject string */

	/* Accept a nonempty sequence of digits optionally containing '.'. */
	for (ul = digits = decexp = 0;
			isdigit(c) || (c == '.' && (flags & DOTSEEN) == 0);
			c = (unsigned int)*nptr++) {

		/* Look for decimal point. */
		if (c == '.') {
			flags |= DOTSEEN;	/* note '.' seen */
			if ((flags & DIGITSEEN) != 0)
				endp = nptr;	/* input accepted to here */
			continue;
		}

		/*
		 * c must be a digit at this point.
		 * Compute the significand in an unsigned long ul
		 * until it overflows, counting digits.
		 * The actual current significand is:
		 * 	value * _pow10(digits) + ul
		 */
		flags |= DIGITSEEN;
		endp = nptr;			/* input accepted to here */
		c -= '0';
		if (ul > (ULONG_MAX - 9) / 10) {
			/* The significand in ul is about to overflow. */
			value = ((flags & TOOBIG) != 0) ? value * _pow10(digits) + ul : (double)ul;
			ul = (unsigned long)c;
			digits = 1;
			flags |= TOOBIG;
		} else {
			ul = ul * 10 + c;
			++digits;
		}
		if ((flags & DOTSEEN) != 0)
			--decexp;		/* adjust implicit exponent */
	}

	/* Store the significand in value. */
	value = ((flags & TOOBIG) != 0) ? value * _pow10(digits) + ul : (double)ul;

	/* Look for optional exponent: 'E' or 'e', optional sign, decimal digits. */
	if (c == 'E' || c == 'e') {
		c = (unsigned int)*nptr++;

		/* Exponent sign. */
		if (c == '+' || c == '-') {
			if (c == '-')
				flags |= EXPSIGN;
			c = (unsigned int)*nptr++;
		}
		if (!isdigit(c))
			goto done1;		/* exponent is absent */

		/* Process digits from explicit exponent. */
		for (digits = expexp = 0; isdigit(c); c = (unsigned int)*nptr++) {
			endp = nptr;		/* input accepted to here */
			c -= '0';
			if (digits != 0 || c != 0)
				++digits;	/* count exponent digits */
			expexp = expexp * 10 + c;
		}

		/*
		 * If the explicit exponent has too many significant digits,
		 * it must produce overflow or underflow.
		 */
		if (digits > DBL_EXP_10_DIG) {
			flags |= ((flags & EXPSIGN) != 0) ? UNDERFLOW : OVERFLOW;
			goto done2;
		}

		/* Reconcile the decimal exponent with the explicit exponent. */
		if ((flags & EXPSIGN) != 0)
			decexp -= expexp;
		else
			decexp += expexp;
	}

	/* Reconcile the result with the decimal exponent. */
done1:
	if (decexp < DBL_MIN_10_EXP)
		flags |= UNDERFLOW;		/* exponent underflow */
	else if (decexp > DBL_MAX_10_EXP)
		flags |= OVERFLOW;		/* exponent overflow */
	else if (decexp > 0)
		value *= _pow10(decexp);
	else if (decexp < 0)
		value /= _pow10(-decexp);	/* avoid precision loss on negative exponent */

	/* Done, store result and return. */
done2:
	if (endptr != NULL)
		*endptr = (char *)endp;
	if ((flags & OVERFLOW) != 0 || (flags & UNDERFLOW) != 0) {
		errno = ERANGE;
		value = ((flags & OVERFLOW) != 0) ? HUGE_VAL : 0.0;
	}
	return ((flags & SIGN) != 0) ? -value : value;
}
