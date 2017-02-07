/*
 * lib-src/ansi/stdlib/bsearch.c
 * ANSI/ISO 9899-1990, Section 7.10.5.1.
 *
 * void *
 * bsearch(const void *key, const void *base, size_t nmemb, size_t size,
 *		int (*compar)(const void *, const void *))
 *
 * Perform a binary search for a given <key> within a sorted table.
 * The table contains <count> entries of size <width> and starts at <base>.
 * Entries are compared using compar(key, entry), where each argument
 * is a (void *) and the function returns an int < 0, = 0 or > 0
 * according to the order of the two arguments.
 * bsearch() returns a pointer to the matching entry, if found,
 * otherwise NULL is returned.
 *
 *  Author: Terrence Holm          Aug. 1988
 */

#include <stdlib.h>

void *
bsearch(const void *key, const void *base, size_t nmemb, size_t size,
  	int  (*compar)(const void *, const void *))
{
#ifdef __TCS_V2__ /* version V2.0 */
	const char	*mid_point;
#else /* __TCS_V2__ */	
	const void	*mid_point;
#endif /* __TCS_V2__ */
	int		cmp;

	while (nmemb > 0) {
#ifdef __TCS_V2__ /* version V2.0 */
		mid_point = (const char *)base + size * (nmemb >> 1);
#else /* __TCS_V2__ */	
		mid_point = base + size * (nmemb >> 1);
#endif /* __TCS_V2__ */
		cmp = compar(key, mid_point);
		if (cmp == 0)
			return (void *)mid_point;
		if (cmp < 0)
			nmemb >>= 1;
		else {
			base = mid_point + size;
			nmemb = (nmemb - 1) >> 1;
		}
	}
	return NULL;
}
