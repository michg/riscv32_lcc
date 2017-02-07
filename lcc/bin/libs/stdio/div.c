/*
 * lib-src/ansi/stdlib/div.c
 * ANSI/ISO 9899-1990, Section 7.10.6.2.
 *
 * div_t div(int numer, int denom)
 * Compute the quotient and remainder of numer / denom.
 *
 * This version is no more efficient than doing / and % separately.
 */

#include <stdlib.h>

div_t
div(int numer, int denom)
{
	div_t r;

	r.quot = numer / denom;
	r.rem = numer % denom;

	return r;
}
