/*
 * lib-src/ansi/ctype/tolower.c
 * ANSI/ISO 9899-1990, Section 7.3.2.1.
 *
 * int tolower(int c)
 */

#include <ctype.h>

int
(tolower)(int c)
{
	return (isupper(c)) ? c + 'a' - 'A' : c;
}
