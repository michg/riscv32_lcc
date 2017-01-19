/*
 * sys/types.h
 * Basic system types.
 */

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef	__SYS_TYPES_H__
#define	__SYS_TYPES_H__

#include <common/_off_t.h>
#include <common/_size_t.h>
#include <common/_wchar_t.h>

typedef	char *		caddr_t;
typedef	short		dev_t;
typedef	unsigned short	gid_t;
typedef	unsigned long	ino_t;
typedef	unsigned short	mode_t;
typedef	_OFF_T_		off_t;
typedef	int		pid_t;
typedef	int		ssize_t;	/* signed size_t */	
typedef	unsigned short	uid_t;

#if	!defined(_POSIX_SOURCE)

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef	unsigned short	ushort;		/* System V compatibility */
typedef	unsigned int	uint;		/* System V compatibility */

#endif	/* !defined(_POSIX_SOURCE) */

#endif	/* __SYS_TYPES_H__ */

/* end of sys/types.h */
