/*
 * lib-src/ansi/stdlib/labs.c
 * ANSI/ISO 9899-1990, Section 7.10.6.4.
 *
 * long labs(long j)
 * Return the absolute value of j.
 */

#include <stdlib.h>

#if	defined(__TCS__)
#include <ops/custom_ops.h>
#endif	/* defined(__TCS__) */

long 
labs(long j)
{
#if	defined(__TCS__)
	return iabs((int)j);		/* N.B. assumes int==long */
#else	/* defined(__TCS__) */
	return (j < 0) ? -j : j;
#endif	/* defined(__TCS__) */
}
