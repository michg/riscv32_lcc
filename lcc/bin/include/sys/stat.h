/* sys/stat.h */

#if	!defined(__SYS_STAT_H__)
#define	__SYS_STAT_H__

#include <time.h>		/* to define time_t */
#include <sys/types.h>

#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */

struct	stat {
	dev_t	st_dev;
	ino_t	st_ino;
	mode_t	st_mode;
	short	st_nlink;
	uid_t	st_uid;
	gid_t	st_gid;
	dev_t	st_rdev;
	off_t	st_size;
	time_t	st_atime;
	int	st_spare1;
	time_t	st_mtime;
	int	st_spare2;
	time_t	st_ctime;
	int	st_spare3;
	long	st_blksize;
	long	st_blocks;
	long	st_spare4[2];
};

#define	_IFMT		0170000	/* type of file				*/
#define		_IFDIR	0040000	/* directory				*/
#define		_IFCHR	0020000	/* character special			*/
#define		_IFBLK	0060000	/* block special			*/
#define		_IFREG	0100000	/* regular				*/
#define		_IFLNK	0120000	/* symbolic link			*/
#define		_IFSOCK	0140000	/* socket				*/
#define		_IFIFO	0010000	/* fifo					*/

#define	S_ISUID		0004000	/* set user id on execution		*/
#define	S_ISGID		0002000	/* set group id on execution		*/
#if	!defined(_POSIX_SOUCE)
#define	S_ISVT		0001000	/* save swapped text even after use	*/
#define	S_IREAD		0000400	/* read permission, owner		*/
#define	S_IWRITE 	0000200	/* write permission, owner		*/
#define	S_IEXEC		0000100	/* execute/search permission, owner	*/

#define	S_ENFMT 	0002000	/* enforcement-mode locking		*/

#define	S_IFMT		_IFMT
#define	S_IFDIR		_IFDIR
#define	S_IFCHR		_IFCHR
#define	S_IFBLK		_IFBLK
#define	S_IFREG		_IFREG
#define	S_IFLNK		_IFLNK
#define	S_IFSOCK	_IFSOCK
#define	S_IFIFO		_IFIFO
#endif	/* !defined(_POSIX_SOUCE) */

#define	S_IRWXU 	0000700	/* rwx, owner				*/
#define		S_IRUSR	0000400	/* read permission, owner		*/
#define		S_IWUSR	0000200	/* write permission, owner		*/
#define		S_IXUSR	0000100	/* execute/search permission, owner	*/
#define	S_IRWXG		0000070	/* rwx, group				*/
#define		S_IRGRP	0000040	/* read permission, group		*/
#define		S_IWGRP	0000020	/* write permission, grougroup		*/
#define		S_IXGRP	0000010	/* execute/search permission, group	*/
#define	S_IRWXO		0000007	/* rwx, other				*/
#define		S_IROTH	0000004	/* read permission, other		*/
#define		S_IWOTH	0000002	/* write permission, other		*/
#define		S_IXOTH	0000001	/* execute/search permission, other	*/

#define	S_ISBLK(m)	(((m)&_IFMT) == _IFBLK)
#define	S_ISCHR(m)	(((m)&_IFMT) == _IFCHR)
#define	S_ISDIR(m)	(((m)&_IFMT) == _IFDIR)
#define	S_ISFIFO(m)	(((m)&_IFMT) == _IFIFO)
#define	S_ISREG(m)	(((m)&_IFMT) == _IFREG)
#if	!defined(_POSIX_SOURCE)
#define	S_ISLNK(m)	(((m)&_IFMT) == _IFLNK)
#define	S_ISSOCK(m)	(((m)&_IFMT) == _IFSOCK)
#endif	/* !defined(_POSIX_SOURCE) */

/* The fstat() and stat() system calls. */
extern	int	_fstat(int fd, struct stat *sbuf);
extern	int	_stat(const char *path, struct stat *sbuf);

#if	!defined(__STRICT_ANSI__) || defined(_POSIX_SOURCE)

extern	int	fstat(int fd, struct stat *sbuf);
extern	int	stat(const char *path, struct stat *sbuf);

#endif	/* !defined(__STRICT_ANSI__) || defined(_POSIX_SOURCE) */

#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

#endif	/* !defined(__SYS_STAT_H__) */

/* end of sys/stat.h */
