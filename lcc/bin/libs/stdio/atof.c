/*
 * lib-src/ansi/stdlib/atof.c
 * ANSI/ISO 9899-1990, Section 7.10.1.1.
 *
 * double atof(const char *nptr)
 * Return a double containing the value represented by the string at nptr.
 *
 * This version just lets strtod() do the work,
 * disregarding the excess baggage of the endptr code in strtod().
 */

#include <stdlib.h>

double
atof(const char *nptr)
{
	return strtod(nptr, (char **)NULL);
}

