/*
 * assert.h
 * Diagnostics.
 * ANSI/ISO 9899-1990, Section 7.2.
 */

#ifndef __ASSERT_H__
#define __ASSERT_H__

#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */

#ifdef	NDEBUG           /* required by ANSI standard */

#define assert(p)  	((void)0)

#else	/* NDEBUG */

#define assert(p)   ((p) ? (void)0 : __assertfail( \
			"Assertion failed: %s, file %s, line %d\n", \
			#p, __FILE__, __LINE__) )

void	__assertfail(char *__msg, char *__cond, char *__file, int __line);

#endif	/* NDEBUG */

#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

#endif	/* __ASSERT_H__ */
