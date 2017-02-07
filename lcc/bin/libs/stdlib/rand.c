/*
 * libc/stdlib/rand.c
 * ANSI/ISO 9899-1990, Sections 7.10.2.1 and 7.10.2.2.
 *
 * int rand(void)
 * Return a random number in the range [0, RAND_MAX].
 *
 * void srand(unsigned int seed)
 * Seed the random number generator with the given value.
 *
 * This code uses the portable implementation from the
 * example given in Section 4.10.2.2 of the Standard.
 */

#include <stdlib.h>

static unsigned long int rand_flag, rand_next;

int 
rand(void)
{
	unsigned long int *nextp;

	/*
	 * ANSI 7.10.2.2:
	 * If rand is called before any calls to srand have been made,
	 * the same sequence shall be generated as when srand is first called
	 * with a seed value of 1.
	 */
	if (rand_flag == 0)
		srand(1);
	nextp = &rand_next;
	*nextp = (*nextp) * 1103515245 + 12345;
	return (unsigned int)((*nextp)/65536) % 32768;
}

void
srand(unsigned int seed)
{
	rand_flag = 1;
	rand_next = seed;
}

/* end of rand.c */
