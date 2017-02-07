/*
 * lib-src/ansi/stdlib/abs.c
 * ANSI/ISO 9899-1990, Section 7.10.6.1.
 *
 * int abs(int j)
 * Return the absolute value of j.
 */

#include <stdlib.h>

#if	defined(__TCS__)
#include <ops/custom_ops.h>
#endif	/* defined(__TCS__) */

int
abs(int j)
{
#if	defined(__TCS__)
	return iabs(j);
#else	/* defined(__TCS__) */
	return (j < 0) ? -j : j;
#endif	/* defined(__TCS__) */
}
