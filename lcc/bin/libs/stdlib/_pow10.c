/*
 * lib-src/ansi/stdlib/_pow10.c
 * Standard C library internal (non-ANSI) function.
 *
 * double _pow10(int n)
 * Return 10 to the nth power.
 *
 * There are many obvious ways to compute this function,
 * with interesting time/space tradeoffs.
 * Direct table lookup is very fast but large,
 * especially to cover the full range of IEEE format double precision;
 * computation by iterated multiplication is very slow but small.
 * This source takes the middle ground, using direct table lookup
 * for common values (in the range -16 < n < 16)
 * and performing at most one double multiply or divide
 * for less common values (in the range DBL_MIN_10_EXP < n < DBL_MAX_10_EXP).
 *
 * Exceptions:
 *	ERANGE	HUGE_VAL	n > DBL_MAX_10_EXP (exponent overflow)
 *	ERANGE	0.0		n < DBL_MIN_10_EXP (exponent underflow)
 *
 * Notes:
 * Table pow_16[] below knows the range of IEEE format single
 * and double precision values; it must change for other representations.
 * The n < 0 results are imprecise due to fp roundoff
 * (assuming FLT_RADIX is 2, not 10!).
 * If NO_NEGATIVE_ARGS is defined during compilation,
 * the tables and code for n < 0 are omitted.
 */

#include <stdlib.h>
#include <errno.h>
#include <float.h>
#include <math.h>

#define	NO_NEGATIVE_ARGS

/* Tables. */
static	const	double	pow_p[]	= {
	1e+0,	1e+1,	1e+2,	1e+3,	1e+4,	1e+5,	1e+6,	1e+7,
	1e+8,	1e+9,	1e+10,	1e+11,	1e+12,	1e+13,	1e+14,	1e+15
};

#if	!defined(NO_NEGATIVE_ARGS)
static	const	double	pow_m[]	= {
	1e-0,	1e-1,	1e-2,	1e-3,	1e-4,	1e-5,	1e-6,	1e-7,
	1e-8,	1e-9,	1e-10,	1e-11,	1e-12,	1e-13,	1e-14,	1e-15
};
#endif	/* !defined(NO_NEGATIVE_ARGS) */

static	const	double	pow_16[] = {

#if	defined(__IEEE_SP_FP__)

	1e+16,	1e+32

#elif	defined(__IEEE_DP_FP__)

	1e+16,	1e+32,	1e+48,	1e+64,	1e+80,	1e+96,	1e+112,	1e+128,
	1e+144,	1e+160,	1e+176,	1e+192,	1e+208,	1e+224,	1e+240,	1e+256,
	1e+272,	1e+288, 1e+304

#else
#error	!defined(__IEEE_SP_FP__) && !defined(__IEEE_SP_FP__)

#endif	/* defined(__IEEE_SP_FP__) */
};

double
_pow10(int n)
{

#if	!defined(NO_NEGATIVE_ARGS)
	if (n < 0) {
		n = -n;
		if (n < 16)
			return pow_m[n];
		else if (n <= -DBL_MIN_10_EXP)
			return pow_m[n & 0xF] / pow_16[(n >> 4) - 1];
		else {
			errno = ERANGE;		/* exponent underflow */
			return 0.0;
		}
	}
#endif	/* !defined(NO_NEGATIVE_ARGS) */

	if (n < 16)
		return pow_p[n];
	else if (n <= DBL_MAX_10_EXP)
		return pow_p[n & 0xF] * pow_16[(n >> 4) - 1];
	else {
		errno = ERANGE;			/* exponent overflow */
		return HUGE_VAL;
	}
}
