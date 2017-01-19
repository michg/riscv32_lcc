/* alloca.h */

#ifndef __ALLOCA_H__
#define __ALLOCA_H__

#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */

#define alloca(x) _rt_alloca(x)

extern void *alloca(long);

#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

#endif /* __ALLOCA_H__ */

/* end of alloca.h */
