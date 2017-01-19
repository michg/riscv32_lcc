/*
 * time.h
 * Date and time.
 * ANSI/ISO 9899-1990, Section 7.12.
 */

#ifndef __TIME_H__
#define __TIME_H__

#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */

/* Macros. */
#include <common/_null.h>
#define CLOCKS_PER_SEC 1000000		/* machine dependent */

/* Types. */
typedef	long	clock_t;
typedef	long	time_t;
#include <common/_size_t.h>
struct tm
{
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
};

/* Standard functions. */
extern	char	  *asctime   (const struct tm *__timeptr);
extern	clock_t	   clock     (void);
extern	char	  *ctime     (const time_t *__timer);
extern	double	   difftime  (time_t __time1, time_t __time0);
extern	struct tm *gmtime    (const time_t *__timer);
extern	struct tm *localtime (const time_t *__timer);
extern	time_t	   mktime    (struct tm *__timeptr);
extern	size_t	   strftime  (char *__s, size_t __maxsize, const char *__format, const struct tm *__timeptr);
extern	time_t	   time      (time_t *__timer);

#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

#endif	/* __TIME_H__ */

/* end of time.h */
