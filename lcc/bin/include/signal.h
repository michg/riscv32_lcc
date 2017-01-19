/*
 * signal.h
 * Signal handling.
 * ANSI/ISO 9899-1990, Section 7.7.
 */

#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include <sys/types.h>

#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */

/* Type. */
typedef int	sig_atomic_t; 	/* Atomic entity type		*/

/* Macros. */
/* Actions. */
#define SIG_DFL ((void (*)(int))0)	/* Default action	*/
#define SIG_IGN ((void (*)(int))1)	/* Ignore action	*/
#define SIG_ERR ((void (*)(int))-1)	/* Error return		*/

/* Signal numbers. */
#define	SIGHUP	1		/* Hangup				*/
#define SIGINT	2		/* Interactive Interrupt received	*/
#define	SIGQUIT	3		/* Quit					*/
#define SIGILL	4		/* Illegal instruction			*/
#define SIGABRT	6		/* Abnormal termination for abort function */
#define SIGFPE	8		/* Floating point trap			*/
#define	SIGBUS	10		/* Bus error				*/
#define SIGSEGV	11		/* Memory access violation		*/
#define	SIGPIPE	13		/* Write to pipe to nowhere		*/
#define	SIGALRM	14		/* Alarm clock				*/
#define SIGTERM	15		/* Termination request sent to program	*/

/* Standard functions. */
int	raise(int __sig);
void  (*signal (int __sig, void (*__func)(int)))(int);

/* Signal handlers. */
#define	_NSIG	32
extern	void	(*_sig_handler[_NSIG + 1])(int __sig);

#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

#endif	/* __SIGNAL_H__ */
