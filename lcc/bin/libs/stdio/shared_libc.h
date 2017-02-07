/*
 * shared_libc.h
 * This header contains macros to support exclusive use of stdio buffered i/o
 * by a shared version of the standard library.
 */

#if	defined(SHARED_LIBC)

#define	EXCL_START(semp) \
	sem_P(semp) 

#define	EXCL_END(semp) \
	sem_V(semp) 

#else	/* defined(SHARED_LIBC) */

#define	EXCL_START(semp)
#define	EXCL_END(semp)

#endif	/* defined(SHARED_LIBC) */

/* end of shared_libc.h */
