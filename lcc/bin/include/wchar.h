/*
 * wchar.h
 * ANSI/ISO 9899-1990 Normative Addendum 1.
 */

#if	!defined(__WCHAR_H__)
#define	__WCHAR_H__

#include <common/_null.h>
#include <common/_size_t.h>

typedef	int	wchar_t;
typedef	int	wint_t;
typedef	int	mbstate_t;

#if	defined(__cplusplus)
extern	"C"	{
}
#endif	/* defined(__cplusplus) */

#endif	/* !defined(__WCHAR_H__) */

/* end of wchar.h */
