/*
 * dirent.h
 * Unix/POSIX-style system header.
 */

#if	!defined(__DIRENT_H__)
#define	__DIRENT_H__

#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */


struct	dirent {
	long	d_ino;
	long	d_namlen;
	char	d_name[1];
};

typedef	void *	DIR;

/*
 * Function prototypes.
 */
extern	int		 _closedir(DIR *);
extern	DIR		*_opendir(const char *);
extern	struct dirent	*_readdir(DIR *);
extern	void		 _rewinddir(DIR *);

#if	!defined(__STRICT_ANSI__) || defined(_POSIX_SOURCE)

/*
 * Define functions from ISO/IEC IS 9945-1:1990 / IEEE Standard 1003.1-1990,
 * more commonly known as POSIX.1, same as system calls above.
 * Compile with -D__STRICT_ANSI__ if you want these definitions to go away,
 * compile with -D_POSIX_SOURCE if you want to keep them.
 */
extern	int		 closedir(DIR *);
extern	DIR		*opendir(const char *);
extern	struct dirent	*readdir(DIR *);
extern	void		 rewinddir(DIR *);

#endif	/* !defined(__STRICT_ANSI__) || defined(_POSIX_SOURCE) */

#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

#endif	/* !defined(__DIRENT_H__) */

/* end of dirent.h */
