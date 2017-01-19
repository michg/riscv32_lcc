/*
 * unistd.h
 * Unix/POSIX-style system call header.
 */

#ifndef	__UNISTD_H__
#define	__UNISTD_H__

#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */

#include <common/_size_t.h>
#include <sys/types.h>

/* Symbolic constants for access(). */
#define	R_OK	4	/* Test for Read permission	*/
#define	W_OK	2	/* Test for Write permission	*/
#define	X_OK	1	/* Test for eXecute permission	*/
#define	F_OK	0	/* Test for File existence	*/

/*
 * Function prototypes for Unix/POSIX-style system calls.
 * _fstat() and stat() are declared in <sys/stat.h>.
 * open() is declared in <fcntl.h>.
 * Directory handling functions are declared in <dirent.h>.
 */
extern	int	 _access(const char *path, int amode);
extern	int	 _close(int fildes);
extern	void	 _exit(int status);
extern	int	 _fsync(int fildes);				/* non-POSIX */
extern	int	 _isatty(int fildes);
extern	int	 _link(const char *existing, const char *newfile);
extern	off_t	 _lseek(int fildes, off_t offset, int whence);
extern	int	 _mkdir(char *path, int mode);
extern	char	*_mktemp(char *templat);			/* non-POSIX */
extern	int	 _putenv(char *string);				/* non-POSIX */
extern	ssize_t	 _read(int fildes, void *buf, size_t nbyte);
extern	int	 _rmdir(char *path);
extern	void	*_sbrk(int incr);				/* non-POSIX */
extern	int	 _sync(void);					/* non-POSIX */
extern	int	 _unlink(const char *path);
extern	ssize_t	 _write(int fildes, const void *buf, size_t nbyte);

#if	!defined(__STRICT_ANSI__) || defined(_POSIX_SOURCE)

/*
 * Define functions from ISO/IEC IS 9945-1:1990 / IEEE Standard 1003.1-1990,
 * more commonly known as POSIX.1, same as system calls above.
 * Compile with -D__STRICT_ANSI__ if you want these definitions to go away,
 * compile with -D_POSIX_SOURCE if you want to keep them.
 * mktemp() and sbrk() are non-POSIX.
 */
extern	int		access(const char *path, int amode);
extern	int		close(int fildes);
extern	int		isatty(int fildes);
extern	int		link(const char *existing, const char *newfile);
extern	off_t		lseek(int fildes, off_t offset, int whence);
extern	int		mkdir(char *path, int mode);
extern	ssize_t		read(int fildes, void *buf, size_t nbyte);
extern	int		rmdir(char *path);
extern	int		unlink(const char *path);
extern	unsigned int	sleep(unsigned int seconds);
extern	ssize_t		write(int fildes, const void *buf, size_t nbyte);

#if	!defined(_POSIX_SOURCE)
extern	int		fsync(int fildes);			/* non-POSIX */
extern	unsigned int	microsleep(unsigned int microseconds);	/* non-POSIX */
extern	char		*mktemp(char *templat);			/* non-POSIX */
extern	int		putenv(char *string);			/* non-POSIX */
extern	void		*sbrk(int incr);			/* non-POSIX */
extern	int		sync(void);				/* non-POSIX */
#endif	/* !defined(_POSIX_SOURCE) */

#endif	/* !defined(__STRICT_ANSI__) || defined(_POSIX_SOURCE) */

#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

#endif	/* __UNISTD_H__ */

/* end of unistd.h */
