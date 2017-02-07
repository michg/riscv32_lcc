/*
 * lib-src/ansi/ctype/toupper.c
 * ANSI/ISO 9899-1990, Section 7.3.2.2.
 *
 * int toupper(int c)
 */

#include <ctype.h>

int
(toupper)(int c)
{
	return (islower(c)) ? c + 'A' - 'a' : c;
}

