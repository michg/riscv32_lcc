/*
 * stdio.h
 * Input/output.
 * ANSI/ISO 9899-1990, Section 7.9.
 */

/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * NB: to fit things in six character monocase externals, the
 * stdio code uses the prefix `__s' for stdio objects, typically
 * followed by a three-character attempt at a mnemonic.
 */

#ifndef __STDIO_H__
#define	__STDIO_H__

#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */

#define	_FSTDIO			/* ``function stdio'' */

#include <common/_size_t.h>
#include <common/_va_list.h>

#include <common/_off_t.h>
#include <stdarg.h>
typedef	_OFF_T_	fpos_t;

/*
 * Stdio buffers.
 */
struct __sbuf {
	unsigned char *_base;
	int	_size;
};

/*
 * Stdio state variables.
 *
 * The following always hold:
 *
 *	if (_flags&(__SLBF|__SWR)) == (__SLBF|__SWR),
 *		_lbfsize is -_bf._size, else _lbfsize is 0
 *	if _flags&__SRD, _w is 0
 *	if _flags&__SWR, _r is 0
 *
 * This ensures that the getc and putc macros (or inline functions) never
 * try to write or read from a file that is in `read' or `write' mode.
 * (Moreover, they can, and do, automatically switch from read mode to
 * write mode, and back, on "r+" and "w+" files.)
 *
 * _lbfsize is used only to make the inline line-buffered output stream
 * code as compact as possible.
 *
 * _ub, _up, and _ur are used when ungetc() pushes back more characters
 * than fit in the current _bf, or when ungetc() pushes back a character
 * that does not match the previous one in _bf.  When this happens,
 * _ub._base becomes non-nil (i.e., a stream has ungetc() data iff
 * _ub._base!=NULL) and _up and _ur save the current values of _p and _r.
 */
typedef	struct __sFILE {
	unsigned char *_p;	/* current position in (some) buffer */
	int	_r;		/* read space left for getc() */
	int	_w;		/* write space left for putc() */
	short	_flags;		/* flags, below; this FILE is free if 0 */
	short	_file;		/* fileno, if Unix descriptor, else -1 */
	struct	__sbuf _bf;	/* the buffer (at least 1 byte, if !NULL) */
	int	_lbfsize;	/* 0 or -_bf._size, for inline putc */

	/* operations */
	void	*_cookie;	/* cookie passed to io functions */
	int	(*_read)(void *_cookie, char *_buf, int _n);
	int	(*_write)(void *_cookie, const char *_buf, int _n);
	fpos_t	(*_seek)(void *_cookie, fpos_t _offset, int _whence);
	int	(*_close)(void *_cookie);

	/* separate buffer for long sequences of ungetc() */
	struct	__sbuf _ub;	/* ungetc buffer */
	unsigned char *_up;	/* saved _p when _p is doing ungetc data */
	int	_ur;		/* saved _r when _r is counting ungetc data */

	/* tricks to meet minimum requirements even when malloc() fails */
	unsigned char _ubuf[3];	/* guarantee an ungetc() buffer */
	unsigned char _nbuf[1];	/* guarantee a getc() buffer */

	/* separate buffer for fgetline() when line crosses buffer boundary */
	struct	__sbuf _lb;	/* buffer for fgetline() */

	/* Unix stdio files get aligned to block boundaries on fseek() */
	int	_blksize;	/* stat.st_blksize (may be != _bf._size) */
	int	_offset;	/* current lseek offset */
} FILE;

extern FILE __sF[];

#define	__SLBF	0x0001		/* line buffered */
#define	__SNBF	0x0002		/* unbuffered */
#define	__SRD	0x0004		/* OK to read */
#define	__SWR	0x0008		/* OK to write */
	/* RD and WR are never simultaneously asserted */
#define	__SRW	0x0010		/* open for reading & writing */
#define	__SEOF	0x0020		/* found EOF */
#define	__SERR	0x0040		/* found error */
#define	__SMBF	0x0080		/* _buf is from malloc */
#define	__SAPP	0x0100		/* fdopen()ed in append mode */
#define	__SSTR	0x0200		/* this is an sprintf/snprintf string */
#define	__SOPT	0x0400		/* do fseek() optimisation */
#define	__SNPT	0x0800		/* do not do fseek() optimisation */
#define	__SOFF	0x1000		/* set iff _offset is in fact correct */
#define	__SMOD	0x2000		/* true => fgetline modified _p text */
#define	__SAPM	0x4000		/* fopen()'ed in append mode */
#define	__SBIN	0x8000		/* fopen()'ed in binary mode */

/*
 * The following three definitions are for ANSI C, which took them
 * from System V, which stupidly took internal interface macros and
 * made them official arguments to setvbuf(), without renaming them.
 * Hence, these ugly _IOxxx names are *supposed* to appear in user code.
 *
 * Although these happen to match their counterparts above, the
 * implementation does not rely on that (so these could be renumbered).
 */
#define	_IOFBF	0		/* setvbuf should set fully buffered */
#define	_IOLBF	1		/* setvbuf should set line buffered */
#define	_IONBF	2		/* setvbuf should set unbuffered */

#include <common/_null.h>

#define	BUFSIZ	10240
#define	EOF	(-1)

#define	FOPEN_MAX	20	/* must be <= OPEN_MAX <sys/syslimits.h> */
#define	FILENAME_MAX	1024	/* must be <= PATH_MAX <sys/syslimits.h> */
#define	L_tmpnam	1024	/* XXX must be == PATH_MAX */

#define	SEEK_SET	0	/* set file offset to offset */
#define	SEEK_CUR	1	/* set file offset to current plus offset */
#define	SEEK_END	2	/* set file offset to EOF plus offset */

#define	TMP_MAX		26

#define	stdin	(&__sF[0])
#define	stdout	(&__sF[1])
#define	stderr	(&__sF[2])

/*
 * Functions reqired by the C standard ANSI/ISO 9899-1990.
 */
extern	void	 clearerr(FILE *__stream);
extern	int	 fclose(FILE *__stream);
extern	int	 feof(FILE *__stream);
extern	int	 ferror(FILE *__stream);
extern	int	 fflush(FILE *__stream);
extern	int	 fgetc(FILE *__stream);
extern	int	 fgetpos(FILE *__stream, fpos_t *__pos);
extern	char	*fgets(char *__s, int __n, FILE *__stream);
extern	FILE	*fopen(const char *__filename, const char *__mode);
extern	int	 fprintf(FILE *__stream, const char *__format, ...);
extern	int	 fputc(int __c, FILE *__stream);
extern	int	 fputs(const char *__s, FILE *__stream);
extern	size_t	 fread(void *__ptr, size_t __size, size_t __nmemb, FILE *__stream);
extern	FILE	*freopen(const char *__filename, const char *__mode, FILE *__stream);
extern	int	 fscanf(FILE *__stream, const char *__format, ...);
extern	int	 fseek(FILE *__stream, long int __offset, int __whence);
extern	int	 fsetpos(FILE *__stream, const fpos_t *__pos);
extern	long	 ftell(FILE *__stream);
extern	size_t	 fwrite(const void *__ptr, size_t __size, size_t __nmemb, FILE *__stream);
extern	int	 getc(FILE *__stream);
extern	int	 getchar(void);
extern	char	*gets(char *__s);
extern	void	 perror(const char *__s);
extern	int	 printf(const char *__format, ...);
extern	int	 putc(int c, FILE *__stream);
extern	int	 putchar(int __c);
extern	int	 puts(const char *__s);
extern	int	 remove(const char *__filename);
extern	int	 rename(const char *__old, const char *__newfile);
extern	void	 rewind(FILE *__stream);
extern	int	 scanf(const char *__format, ...);
extern	void	 setbuf(FILE *__stream, char *__buf);
extern	int	 setvbuf(FILE *__stream, char *__buf, int __mode, size_t __size);
extern	int	 sprintf(char *__s, const char *__format, ...);
extern	int	 sscanf(const char *__s, const char *__format, ...);
extern	FILE	*tmpfile(void);
extern	char	*tmpnam(char *__s);
extern	int	 ungetc(int __c, FILE *__stream);
extern	int	 vfprintf(FILE *__stream, const char *__format, va_list __arg);
extern	int	 vprintf(const char *__format, va_list __arg);
extern	int	 vsprintf(char *__s, const char *__format, va_list __arg);

/*
 * Functions internal to the implementation.
 */
extern	FILE	*_fdopen(int, const char *);
extern	int	_fflush(FILE *__stream);
extern	int	__srget(FILE *);
extern	int	__swbuf(int, FILE *);

/*
 * Functions defined in ISO/IEC IS 9945:1/1990, commonly called POSIX.1.
 * Compile with -D__STRICT_ANSI__ if you want these to go away.
 */
#if	!defined(__STRICT_ANSI__) || defined(_POSIX_SOURCE)

#define	fdopen(fd, type)	_fdopen((fd), (type))
#define	fileno(fp)		__sfileno(fp)

#endif	/* !defined(__STRICT_ANSI__) || defined(_POSIX_SOURCE) */

/*
 * The __sfoo macros are here so that we can 
 * define function versions in the C library.
 */
#define	__sgetc(p) (--(p)->_r < 0 ? __srget(p) : (int)(*(p)->_p++))
#define	__sputc(c, p) \
	(--(p)->_w < 0 ? \
		(p)->_w >= (p)->_lbfsize ? \
			(*(p)->_p = (c)), *(p)->_p != '\n' ? \
				(int)*(p)->_p++ : \
				__swbuf('\n', p) : \
			__swbuf((int)(c), p) : \
		(*(p)->_p = (c), (int)*(p)->_p++))

#define	__sfeof(p)	(((p)->_flags & __SEOF) != 0)
#define	__sferror(p)	(((p)->_flags & __SERR) != 0)
#define	__sclearerr(p)	((void)((p)->_flags &= ~(__SERR|__SEOF)))
#define	__sfileno(p)	((p)->_file)

#define	feof(p)		__sfeof(p)
#define	ferror(p)	__sferror(p)

#if	!defined(__TCS__)
/* These cannot be macros in the current TCS libc implementation. */
#define	clearerr(p)	__sclearerr(p)
#define	getc(fp)	__sgetc(fp)
#define putc(x, fp)	__sputc(x, fp)
#endif	/* !defined(__TCS__) */

#define	getchar()	getc(stdin)
#define	putchar(x)	putc(x, stdout)

#if	!defined(__STRICT_ANSI__)

#define	L_cuserid	9		/* posix says it goes in stdio.h :( */

#endif	/* !defined(__STRICT_ANSI__) */

#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

#endif	/* __STDIO_H__ */

/* end of stdio.h */
