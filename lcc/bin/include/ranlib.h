/*
 * ranlib.h
 */

#if	!defined(__RANLIB_H__)
#define __RANLIB_H__

#include <sys/types.h>			/* to get off_t */

/*
 * The __.SYMDEF member of an archive defining nsyms symbols contains:
 *	word containing nsyms * sizeof(struct ranlib)
 *	nsyms struct ranlib entries
 *	word containing length of string table
 *	string table (0-padded to even boundary)
 */

#define	SYMDEF	"__.SYMDEF"		/* member name			*/

struct	ranlib {
	off_t	ran_strx;		/* string table index of symbol	*/
	off_t	ran_off;		/* offset of archive member	*/
};

#endif	/* !defined(__RANLIB_H__) */

/* end of ranlib.h */

