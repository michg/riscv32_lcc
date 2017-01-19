/*
 * ar.h
 * Archive file format.
 * Reference: iBCS2, p. 7-2.
 *
 * An archive typically contains:
 *	magic string ARMAG
 *	optional symbol table (__SYMDEF)
 *	optional archive string table (if any member name exceeds 15 bytes)
 *	archive members (each starting on even boundary):
 *		struct ar_hdr archive header
 *		unchanged contents of member
 *		optional '\n' to pad to even boundary
 * The archive format is generally portable across machines,
 * but there may be system-dependent features (e.g. long module names);
 * cf. e.g. "man 5 ar" on Sun.
 */

#if	!defined(__AR_H__)
#define	__AR_H__

#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */

#define	ARMAG	"!<arch>\n"
#define	SARMAG	8
#define	ARFMAG	"`\n"

/* Fields are ASCII; numbers are blank-padded, decimal except for ar_mode. */
struct	ar_hdr	{
	char	ar_name[16];		/* member name		*/
	char	ar_date[12];		/* decimal date		*/
	char	ar_uid[6];		/* decimal user id	*/
	char	ar_gid[6];		/* decimal group id	*/
	char	ar_mode[8];		/* octal mode		*/
	char	ar_size[10];		/* actual file size	*/
	char	ar_fmag[2];		/* contains ARFMAG	*/
};

#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

#endif	/* !defined(__AR_H__) */

/* end of ar.h */
