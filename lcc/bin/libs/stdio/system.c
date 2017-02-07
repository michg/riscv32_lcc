/*
 * lib-src/ansi/stdlib/system.c
 * ANSI/ISO 9899-1990, Section 7.10.4.5.
 *
 * int system(const char *string)
 * Pass string to a command processor.
 *
 * Implementation-defined behavior:
 * system(string) does nothing at present.
 * If string is NULL, it returns 0 to indicate no command processor exists.
 * If string is not NULL, it returns EXIT_FAILURE.
 */

#include <stdlib.h>

int
system(const char *string)
{
	if (string == NULL)
		return 0;		/* no command processor exists */
	return EXIT_FAILURE;
}
