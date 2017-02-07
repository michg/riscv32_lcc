/*
 * lib-src/ansi/stdlib/ldiv.c
 * ANSI/ISO 9899-1990, Section 7.10.6.4.
 *
 * ldiv_t ldiv(long int numer, long int denom)
 * Compute the quotient and remainder of numer / denom.
 *
 * This version is no more efficient than doing / and % separately.
 */

#include <stdlib.h>

ldiv_t
ldiv(long int numer, long int denom)
{
	ldiv_t r;

	r.quot = numer / denom;
	r.rem = numer % denom;

	return r;
}
