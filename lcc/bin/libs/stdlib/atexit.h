/*
 * lib-src/ansi/stdlib/atexit.h
 * Local header for atexit()-registered functions.
 */

#define	ATEXIT_SIZE 32	/* must be at least 32 to guarantee ANSI conformance */

struct atexit {
	struct	atexit *next;		/* next in list */
	int	ind;			/* next index in this table */
	void	(*fns[ATEXIT_SIZE])();	/* the table itself */
};

