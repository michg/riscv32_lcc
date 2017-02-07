/*
 * lib-src/ansi/stdio/vfprintf.c
 * int vfprintf(FILE *stream, const char *format, va_list arg)
 * ANSI/ISO 9899-1990, Section 7.9.6.7.
 *
 * Copyright (c) 1990 Regents of the University of California.
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

#include <stdio.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ieee.h>

#if	defined(__TCS__)
#include <ops/custom_ops.h>
#endif	/* defined(__TCS__) */

#include "local.h"
#include "fvwrite.h"
#include "shared_libc.h"

/* N.B. DBL_EXP_10_DIG should be (int)(log10(DBL_MAX_10_EXP) + 1) */
/* N.B. Also change fpcvt() exponent code if exponent can be > 999! */
#define	DBL_EXP_10_DIG	3			/* max decimal digits in double exponent */
#define	DEFPREC	6				/* default precision */
#define LOG2_10 3.3219280948873623		/* log2(10) */

/*
 * Maximum precision:
 * DBL_DIG is (int)floor((DBL_MANT_DIG - 1) * log10(FLT_RADIX)),
 * which gives the maximum precision in decimal digits.
 * Digits after DBL_DIG + 1 significant digits are noise.
 * The last of the DBL_DIG + 1 digits may be partially noise but
 * we convert it anyway rather than throw away some valid precision.
 */
#define	MAXPREC	(DBL_DIG + 1)

/*
 * Output conversion buffer size:
 * the buffer needs to hold the largest fp "%f" value, which contains
 * DBL_MAX_10_EXP + 1 digits, then decimal point, then MAXPREC digits.
 */
#define	NBUF	(DBL_MAX_10_EXP + 1 + 1 + MAXPREC)

/* Local function prototypes. */
static	int	fpcvt(double d, char *buf, int fmtch, int prec, int flags,
	       char *signp, char *expbuf, int *expsizep, int *fppadp);
static	double	frexp10(double d, int *expp);
static	int	significand(double d, int fmtch, int prec, char *buf, int *expp);

/* Flags used during conversion. */
#define	LONGINT		0x01		/* long integer			*/
#define	LONGDBL		0x02		/* long double			*/
#define	SHORTINT	0x04		/* short integer		*/
#define	ALT		0x08		/* alternate form		*/
#define	LADJUST		0x10		/* left adjustment		*/
#define	ZEROPAD		0x20		/* zero (as opposed to blank) pad */
#define	HEXPREFIX	0x40		/* add 0x or 0X prefix		*/

/* Macros for testing/setting/clearing conversion flags. */
#define	is_flag(flag)	((flags & flag) != 0)
#define	set_flag(flag)	flags |= flag
#define	clr_flag(flag)	flags &= ~flag

/*
 * Choose PADSIZE to trade efficiency vs size.  If larger
 * printf fields occur frequently, increase PADSIZE (and make
 * the initializers below longer).
 */
#define	PADSIZE	16			/* pad chunk size */
static	const	char	*blanks  = "                ";
static	const	char	*zeroes  = "0000000000000000";
static	const	char	*xdigits = "0123456789abcdef";
static	const	char	*Xdigits = "0123456789ABCDEF";

/*
 * BEWARE, these macros `goto error' on error, and PAD uses `n'.
 */
#define NIOV 8
#define	PRINT(ptr, len) { \
	iovp->iov_base = (ptr); \
	iovp->iov_len = (len); \
	uio.uio_resid += (len); \
	iovp++; \
	if (++uio.uio_iovcnt >= NIOV) { \
		if (__sprint(stream, &uio)) \
			goto error; \
		iovp = iov; \
	} \
}
#define	PAD(howmany, with) { \
	if ((n = (howmany)) > 0) { \
		while (n > PADSIZE) { \
			PRINT(((char *)with), PADSIZE); \
			n -= PADSIZE; \
		} \
		PRINT(((char *)with), n); \
	} \
}
#define	FLUSH() { \
	if (uio.uio_resid && __sprint(stream, &uio)) \
		goto error; \
	uio.uio_iovcnt = 0; \
	iovp = iov; \
}

/*
 * To extend shorts properly, we need both signed and unsigned
 * argument extraction methods.
 */
#define	SARG() \
	(is_flag(LONGINT) ? va_arg(arg, long) : \
	    is_flag(SHORTINT) ? (long)(short)va_arg(arg, int) : \
	    (long)va_arg(arg, int))
#define	UARG() \
	(is_flag(LONGINT) ? va_arg(arg, unsigned long) : \
	    is_flag(SHORTINT) ? (unsigned long)(unsigned short)va_arg(arg, int) : \
	    (unsigned long)va_arg(arg, unsigned int))

/*
 * Flush out all the vectors defined by the given uio,
 * then reset it so that it can be reused.
 */
/* static */
int
__sprint(FILE *fp, register struct __suio *uio)
{
	register int err;

	if (uio->uio_resid == 0) {
		uio->uio_iovcnt = 0;
		return (0);
	}
	err = __sfvwrite(fp, uio);
	uio->uio_resid = 0;
	uio->uio_iovcnt = 0;
	return (err);
}

/*
 * Helper function for `fprintf to unbuffered unix file': creates a
 * temporary buffer.  We only work on write-only files; this avoids
 * worries about ungetc buffers and so forth.
 */
/* static */
int
__sbprintf(register FILE *fp, char const *fmt, va_list ap)
{
	int ret;
	FILE fake;
	unsigned char buf[BUFSIZ];

	/* copy the important variables */
	fake._flags = fp->_flags & ~__SNBF;
	fake._file = fp->_file;
	fake._cookie = fp->_cookie;
	fake._write = fp->_write;

	/* set up the buffer */
	fake._bf._base = fake._p = buf;
	fake._bf._size = fake._w = sizeof(buf);
	fake._lbfsize = 0;	/* not actually used, but Just In Case */

	/* do the work, then copy any error status */
	ret = vfprintf(&fake, fmt, ap);
	if (ret >= 0 && _fflush(&fake))
		ret = EOF;
	if (fake._flags & __SERR)
		fp->_flags |= __SERR;
	return (ret);
}

int
vfprintf(FILE *stream, const char *format, va_list arg)
{
	char *fmt;		/* format string */
	int ch;			/* character from fmt */
	int n;			/* handy integer (short term usage) */
	char *cp;		/* handy char pointer (short term usage) */
	struct __siov *iovp;	/* for PRINT macro */
	int flags;		/* flags as above */
	int ret;		/* return value accumulator */
	int width;		/* width from format (%8d), or 0 */
	int prec;		/* precision from format (%.3d), or -1 */
	int negflag;		/* iff negative precision */
	char sign;		/* sign prefix (' ', '+', '-', or 0) */
	double d;		/* double precision arguments %[eEfgG] */
	unsigned long _ulong;	/* integer arguments %[diouxX] */
	enum { OCT, DEC, HEX } base;/* base for [diouxX] conversion */
	int dprec;		/* a copy of prec if [diouxX], 0 otherwise */
	int fieldsz;		/* field size expanded by sign, etc */
	int realsz;		/* field size expanded by dprec */
	int size;		/* size of converted field or string */
	int expsize;		/* size of fp exponent */
	int fppad;		/* trailing 0-padding required for fp */
	const char *xdigs;	/* digits for [xX] conversion */
	struct __suio uio;	/* output information: summary */
	struct __siov iov[NIOV];/* ... and individual io vectors */
	char buf[NBUF];		/* buffer for %c, %[diouxX], %[eEfgG] */
	char expbuf[DBL_EXP_10_DIG + 3];	/* buffer for E + sign + digits + NUL */
	char ox[2];		/* buffer for 0x hex-prefix */
        int  result;

        EXCL_START(&stream->_file_lock);
	/* sorry, fprintf(read_only_file, "") returns EOF, not 0 */
	if (cantwrite(stream)) {
		EXCL_END(&stream->_file_lock);
		return EOF;
	}

	/* optimise fprintf(stderr) (and other unbuffered Unix files) */
	if ((stream->_flags & (__SNBF|__SWR|__SRW)) == (__SNBF|__SWR) &&
	    stream->_file >= 0) {
                result= (__sbprintf(stream, format, arg));
		EXCL_END(&stream->_file_lock);
		return result;
	}

	fmt = (char *)format;
	uio.uio_iov = iovp = iov;
	uio.uio_resid = 0;
	uio.uio_iovcnt = 0;
	ret = 0;

	/*
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
		/* UNDONE: multibyte characters in format string. */
		for (cp = fmt; (ch = *fmt) != '\0' && ch != '%'; fmt++)
			/* void */;
		if ((n = fmt - cp) != 0) {
			PRINT(cp, n);
			ret += n;
		}
		if (ch == '\0')
			goto done;
		fmt++;		/* skip over '%' */

		sign = 0;
		size = flags = dprec = width = fppad = expsize = 0;
		prec = -1;

rflag:		ch = *fmt++;
reswitch:	switch (ch) {
		case ' ':
			/*
			 * ``If the space and + flags both appear, the space
			 * flag will be ignored.''
			 *	-- ANSI/ISO 9899-1990.
			 */
			if (!sign)
				sign = ' ';
			goto rflag;
		case '#':
			set_flag(ALT);
			goto rflag;
		case '*':
			/*
			 * ``A negative field width argument is taken as a
			 * - flag followed by a positive field width.''
			 *	-- ANSI/ISO 9899-1990.
			 * They don't exclude field widths read from args.
			 */
			if ((width = va_arg(arg, int)) >= 0)
				goto rflag;
			width = -width;
			/* FALLTHROUGH */
		case '-':
			set_flag(LADJUST);
			goto rflag;
		case '+':
			sign = '+';
			goto rflag;
		case '.':
			if ((ch = *fmt++) == '*') {
				n = va_arg(arg, int);
				prec = n < 0 ? -1 : n;
				goto rflag;
			}
			negflag = (ch == '-');
			if (negflag)
				ch = *fmt++;
			for (n = 0; isdigit(ch); ch = *fmt++)
				n = 10 * n + ch - '0';
			if (negflag)
				n = -n;
			prec = n < 0 ? -1 : n;
			goto reswitch;
		case '0':
			/*
			 * ``Note that 0 is taken as a flag, not as the
			 * beginning of a field width.''
			 *	-- ANSI/ISO 9899-1990.
			 */
			set_flag(ZEROPAD);
			goto rflag;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = 0;
			do {
				n = 10 * n + ch - '0';
				ch = *fmt++;
			} while (isdigit(ch));
			width = n;
			goto reswitch;
		case 'L':
			set_flag(LONGDBL);
			goto rflag;
		case 'h':
			set_flag(SHORTINT);
			goto rflag;
		case 'l':
			set_flag(LONGINT);
			goto rflag;
		case 'c':
			*(cp = buf) = va_arg(arg, int);
			size = 1;
			sign = 0;
			break;
		case 'D':
			set_flag(LONGINT);
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
			_ulong = SARG();

			if ((long)_ulong < 0) {
				_ulong = -_ulong;
				sign = '-';
			}
			base = DEC;
			goto number;
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
			/*
			 * Floating point output, what fun.
			 * For now, we truncate long double to double.
			 * If long double were actually longer than double,
			 * to do long double output to full precision we would
			 * instead widen the double to long double here, pass
			 * long doubles down to fpcvt()/significand()/frexp10()/_pow10(),
			 * and pass the flags down to significand() so it can know
			 * whether to use DBL_DIG or LDBL_DIG.
			 */
			if (is_flag(LONGDBL))
				d = (double)va_arg(arg, long double);
			else
				d = va_arg(arg, double);
			cp = buf;
			size = fpcvt(d, cp, ch, prec, flags,
				     &sign, expbuf, &expsize, &fppad);
			break;
		case 'n':
			if (is_flag(LONGINT))
				*va_arg(arg, long *) = ret;
			else if (is_flag(SHORTINT))
				*va_arg(arg, short *) = ret;
			else
				*va_arg(arg, int *) = ret;
			continue;	/* no output */
		case 'O':
			set_flag(LONGINT);
			/*FALLTHROUGH*/
		case 'o':
			_ulong = UARG();
			base = OCT;
			goto nosign;
		case 'p':
			/*
			 * ``The argument shall be a pointer to void.  The
			 * value of the pointer is converted to a sequence
			 * of printable characters, in an implementation-
			 * defined manner.''
			 *	-- ANSI/ISO 9899-1990.
			 */
			/* NOSTRICT */
			_ulong = (unsigned long)va_arg(arg, void *);
			base = HEX;
			xdigs = xdigits;
			set_flag(HEXPREFIX);
			ch = 'x';
			goto nosign;
		case 's':
			if ((cp = va_arg(arg, char *)) == NULL)
				cp = "{NULL}";
			if (prec >= 0) {
				/*
				 * can't use strlen; can only look for the
				 * NUL in the first `prec' characters, and
				 * strlen() will go further.
				 */
				char *p = memchr(cp, 0, prec);

				if (p != NULL) {
					size = p - cp;
					if (size > prec)
						size = prec;
				} else
					size = prec;
			} else
				size = strlen(cp);
			sign = 0;
			break;
		case 'U':
			set_flag(LONGINT);
			/*FALLTHROUGH*/
		case 'u':
			_ulong = UARG();
			base = DEC;
			goto nosign;
		case 'X':
			xdigs = Xdigits;
			goto hex;
		case 'x':
			xdigs = xdigits;
hex:			_ulong = UARG();
			base = HEX;
			/* leading 0x/X only if non-zero */
			if (is_flag(ALT) && _ulong != 0)
				set_flag(HEXPREFIX);

			/* unsigned conversions */
nosign:			sign = 0;
			/*
			 * ``... diouXx conversions ... if a precision is
			 * specified, the 0 flag will be ignored.''
			 *	-- ANSI/ISO 9899-1990.
			 */
number:			if ((dprec = prec) >= 0)
				clr_flag(ZEROPAD);

			/*
			 * ``The result of converting a zero value with an
			 * explicit precision of zero is no characters.''
			 *	-- ANSI/ISO 9899-1990.
			 */
			cp = buf + NBUF;
			if (_ulong != 0 || prec != 0) {
				/*
				 * unsigned mod is hard, and unsigned mod
				 * by a constant is easier than that by
				 * a variable; hence this switch.
				 */
				switch (base) {
				case OCT:
					do {
						*--cp = (_ulong & 7) + '0';
						_ulong >>= 3;
					} while (_ulong);
					/* handle octal leading 0 */
					if (is_flag(ALT) && *cp != '0')
						*--cp = '0';
					break;

				case DEC:
#if	defined(__TCS__)
/*
 * Integer divide and remainder are very inefficient on TM-1,
 * so we try to avoid them by using custom ops.
 * Use umulm() to multiply by 2^32/10 (rounded up to 0x1999999A)
 * and take the high order word of the 64-bit result to get _ulong / 10;
 * similarly for remainder.
 * This does not work for very large _ulong because of the rounding,
 * so we just use normal / and % for those cases (once).
 */
					if (_ulong >= 0x40000000) {
						/* Use / and % at most once. */
						*--cp = _ulong % 10 + '0';
						_ulong /= 10;
					}
					/* Many numbers are 1 digit. */
					while (_ulong >= 10) {
						unsigned long udiv10;

						udiv10 = umulm(_ulong, 0x1999999A);
						*--cp = umulm(10, _ulong * 0x1999999A) + '0';
						_ulong = udiv10;
					}
					*--cp = _ulong + '0';
					break;
#else	/* defined(__TCS__) */
					/* Many numbers are 1 digit. */
					while (_ulong >= 10) {
						*--cp = _ulong % 10 + '0';
						_ulong /= 10;
					}
					*--cp = _ulong + '0';
					break;
#endif	/* defined(__TCS__) */

				case HEX:
					do {
						*--cp = xdigs[_ulong & 15];
						_ulong >>= 4;
					} while (_ulong);
					break;
				}
			}
			size = buf + NBUF - cp;
		skipsize:
			break;
		default:	/* "%?" prints ?, unless ? is NUL */
			if (ch == '\0')
				goto done;
			/* pretend it was %c with argument ch */
			cp = buf;
			*cp = ch;
			size = 1;
			sign = 0;
			break;
		}

		/*
		 * All reasonable formats wind up here.  At this point,
		 * `cp' points to a string of length size which (if !LADJUST)
		 * should be padded out to `width' places.  If ZEROPAD,
		 * it should first be prefixed by any
		 * sign or other prefix; otherwise, it should be blank
		 * padded before the prefix is emitted.  After any
		 * left-hand padding and prefixing, emit zeroes
		 * required by a decimal [diouxX] precision, then print
		 * the string proper, then emit zeroes required by any
		 * leftover floating precision and print the floating exponent;
		 * finally, if LADJUST, pad with blanks.
		 */

		/*
		 * compute actual size, so we know how much to pad.
		 * fieldsz excludes decimal prec; realsz includes it
		 */
		fieldsz = size + expsize + fppad;
		if (sign) {
			if (dprec)
				dprec++;
			fieldsz++;
		} else if (is_flag(HEXPREFIX))
			fieldsz += 2;
		realsz = dprec > fieldsz ? dprec : fieldsz;

		/* right-adjusting blank padding */
		if (!is_flag(LADJUST) && !is_flag(ZEROPAD))
			PAD(width - realsz, blanks);

		/* prefix */

		if (sign) {
			PRINT(&sign, 1);
		} else if (is_flag(HEXPREFIX)) {
			ox[0] = '0';
			ox[1] = ch;
			PRINT(ox, 2);
		}

		/* right-adjusting zero padding */
		if (is_flag(ZEROPAD) && !is_flag(LADJUST))
			PAD(width - realsz, zeroes);

		/* leading zeroes from decimal precision */
		PAD(dprec - fieldsz, zeroes);

		/* the string or number proper */
		PRINT(cp, size);

		/* trailing f.p. zeroes */
		PAD(fppad, zeroes);

		/* trailing f.p. exponent */
		PRINT(expbuf, expsize);

		/* left-adjusting padding (always blank) */
		if (is_flag(LADJUST))
			PAD(width - realsz, blanks);

		/* finally, adjust ret */
		ret += width > realsz ? width : realsz;

		FLUSH();	/* copy out the I/O vectors */
	}
done:
	FLUSH();
error:
        result= (__sferror(stream)) ? EOF : ret;
	EXCL_END(&stream->_file_lock);
	return result;
}

/*
 * This routine does all the significant formatting for floating point output,
 * leaving it up to the caller to provide padding and output the result.
 * Convert double d with given format, precision and flags into buf.
 * Store the sign through signp, the exponent (if any) in expbuf,
 * the exponent size (if any) through expsizep,
 * and the floating point 0-padding required through fppadp.
 * The 0-padding is extra precision required by the precision field; it is
 * not part of the returned buffer because prec might be arbitrarily large.
 * The exponent is not part of the returned buffer because the extra 0-padding
 * must precede the exponent in the actual output.
 * Return the number of characters written to buf, not including expsize.
 *
 * Exceptions:
 *	"NaN"		for NaNs
 *	"+Inf"		for +Infinity
 *	"-Inf"		for -Infinity
 *	"-0"		for minus zero
 *
 * Note: this currently prints IEEE minus zero
 * and values which round to -0 with '-' sign,
 * e.g. ``printf("%.3f", -.0001);'' prints "-0.000".
 * This may or may not be desirable.
 * To change this behavior, pass a different signp (not &sign)
 * in the fpcvt() call from vfprintf(), pass signp down to significand()
 * so significand() can zap the minus sign if the result rounds to 0,
 * and resolve the new *signp with sign in vfprintf().
 * The separate signp would be needed to avoid zapping
 * a specified '+' or ' ' flag in vfprintf().
 */
static
int
fpcvt(double d, char *buf, int fmtch, int prec, int flags,
      char *signp, char *expbuf, int *expsizep, int *fppadp)
{
	register int digits;
	register char *cp, *s, *expp;
	int decexp, gformat;
	char digbuf[MAXPREC];

#if	defined(__IEEE_FP__)
	/* Deal with NaN and infinities. */
	if (_isNaN(d)) {
		strcpy(buf, "NaN");
		return 3;
	} else if (_isInfinity(d)) {
		strcpy(buf, (d > 0) ? "+Inf" : "-Inf");
		return 4;
	} else if (_ismZero(d)) {		/* print minus zero with '-' */
		d = -d;
		*signp = '-';
	}
#endif	/* defined(__IEEE_FP__) */

	/* Force d nonnegative. */
	if (d < 0) {
		*signp = '-';
		d = -d;
	}

	s = buf;				/* destination */
	cp = digbuf;				/* source */
	gformat = (fmtch == 'g' || fmtch == 'G');

	/* Check for default precision. */
	if (prec == -1)
		prec = DEFPREC;			/* -1 means use default */
	else if (prec == 0 && gformat)
		prec = 1;			/* because ISO 9899-1990 says so */

	/* Find the significant digits in d. */
	digits = significand(d, fmtch, prec, digbuf, &decexp);

	/* Convert the number to the desired format. */
	switch(fmtch) {

	case 'e':
	case 'E':
e_or_g:
		/*
		 * Store the exponent into expbuf and its length to *expsizep.
		 * N.B. the code below assumes decexp < 1000.
		 */
		expp = expbuf;
		*expp++ = fmtch;		/* 'E' or 'e' */
		if (decexp < 0) {		/* '+' or '-' */
			decexp = -decexp;
			*expp++ = '-';
		} else
			*expp++ = '+';
		if (decexp >= 100) {
			*expp++ = (decexp / 100) + '0';
			decexp %= 100;
		}
		*expp++ = (decexp / 10) + '0';	/* at least two digits mandated */
		decexp %= 10;
		*expp++ = decexp + '0';
		*expsizep = expp - expbuf;

		/* Store the first significand digit and decimal point. */
		*s++ = *cp++;			/* digit preceding '.' */
		--digits;
		if (is_flag(ALT) || (prec > 0 && (!gformat || digits > 0)))
			*s++ = '.';
		break;

	case 'g':
	case 'G':
		if (decexp < -4 || decexp >= prec) {
			/*
			 * Use 'e' or 'E' format.
			 * 'e' precision means digits after the '.', but
			 * 'g' precision means significant digits;
			 * hence the "--prec;" below, since there is one
			 * significant digit before the '.'.
			 */
			fmtch -= 2;		/* 'G'->'E', 'g'->'e' */
			--prec;			/* adjust precision */
			goto e_or_g;
		}
		/* FALLTHROUGH */		/* use 'f' format. */

	case 'f':
		if (decexp >= 0) {
			for ( ; digits > 0 && decexp >= 0; --digits, --decexp) {
				*s++ = *cp++;	/* store digit before '.' */
				if (gformat)
					--prec;
			}
			for ( ; decexp >= 0; --decexp) {
				*s++ = '0';	/* out of digits, store a significant  0 */
				if (gformat)
					--prec;
			}
		} else
			*s++ = '0';		/* store insignificant '0' before '.' */

		/* Need decimal point if precision remains or alt flag. */
		if (is_flag(ALT) || (prec > 0 && (!gformat || digits > 0)))
			*s++ = '.';		/* store decimal point */
		for ( ; decexp < -1 && prec > 0; ++decexp) {
			*s++ = '0';		/* store insignificant '0' after '.' */
			if (!gformat)
				--prec;
		}
		break;

	}

	/* Store the remainder of the significand and pad later as needed. */
	for ( ; digits > 0 && prec > 0; --digits, --prec)
		*s++ = *cp++;			/* store digits after '.' */
	if (prec > 0 && (!gformat || is_flag(ALT)))
		*fppadp += prec;		/* pad with more 0s later */

	return s - buf;
}

/*
 * This routine does the serious work of fp output conversion:
 * it produces the sequence of significant digits of the result,
 * not yet formatted.
 * Given a nonnegative double d and a desired format and precision,
 * convert d into a nonempty string of decimal digits in the
 * supplied buffer and store the corresponding decimal exponent in *expp.
 * Round the result appropriately and return the length of the result string.
 * The returned string has neither leading nor trailing zeros (or is "0")
 * and is at most MAXPREC characters long.
 * It has an implicit decimal point after the first digit.
 */
static
int
significand(double d, int fmtch, int prec, char *buf, int *expp)
{
	register char *cp;
	register int i, digits;

	cp = buf;
	if (d == 0.0) {			/* just return "0" */
zero:		*cp = '0';
		*expp = 0;
		return 1;
	}

	/*
	 * Find the decimal exponent and reduce d to [1., 10).
	 * N.B. *expp may be adjusted below if the result rounds up.
	 * We need to know the decimal exponent in order to decide
	 * how many significant digits will be required for 'f' format.
	 */
	d = frexp10(d, expp);

	/*
	 * Compute the number of significant digits needed, depending on the format.
	 * Note that the number of significant digits needed must be positive
	 * for 'e' (because prec >= 0 and digits = 1 + prec)
	 * and for 'g' (because prec > 0 and digits = prec),
	 * but may be zero or negative for 'f' when converting
	 * a value with a negative decimal exponent.
	 * Digits after MAXPREC significant digits are noise, do not convert them.
	 * The last of the MAXPREC digits may be partially noise but
	 * we convert it anyway rather than throw away some valid precision.
	 */
	if (fmtch == 'e' || fmtch == 'E')
		digits = 1 + prec;		/* e.g. 1.234 */
	else if (fmtch == 'f')
		digits = *expp + 1 + prec;	/* e.g. 123.456 */
	else
		digits = prec;			/* 'g' or 'G' format */
	if (digits > MAXPREC)
		digits = MAXPREC;		/* convert at most MAXPREC digits */

	/* Check for zero digits or negative digits ('f' format). */
	if (digits <= 0) {
		if (digits == 0 && d >= 5.0) {	/* round up to return "1" */
			*cp = '1';
			++*expp;		/* adjust decimal exponent */
			return 1;
		} else
			goto zero;		/* return "0" */
	}

	/* Store result digits. */
	while (cp < &buf[digits] && d != 0) {
		i = (int)d;
		*cp++ = i + '0';
		#if 0
		_write(1, "[", 1);
		_write(1, cp - 1, 1);
		_write(1, "]", 1);
		#endif
		d = 10.0 * (d - (double)i);	/* reduce d for next digit */
	}

	/* Round the result and strip trailing zeros. */
	if (d < 5.0) {				/* do not round up */
		while (cp > &buf[1] && *(cp - 1) == '0')
			--cp;			/* skip a trailing zero */
		return cp - buf;		/* return result length */
	} else {				/* round up */
		while (--cp >= buf)		/* look at previous digit */
			if (++*cp <= '9')	/* bump it */
				return cp + 1 - buf;	/* done */

		/*
		 * cp == &buf[-1], so we rounded e.g. "99" to "00".
		 * Adjust the decimal exponent and return "1".
		 */
		++*expp;			/* bump decimal exponent */
		*++cp = '1';			/* return "1" */
		return 1;
	}
	/* NOTREACHED */
}

/*
 * frexp10() is like frexp() with a decimal rather than a binary base.
 * For nonzero d, it returns a double decfrac in the range [+-][1., 10) and
 * stores a decimal exponent through *expp, so d = decfrac * 10 ^ (*expp).
 * It extracts the decimal exponent fairly efficiently by using frexp()
 * and scales d with one multiplication and possibly one division.
 * Useful equations:
 * log10(d) = log10(decfrac) + decexp, where log10(decfrac) is in [0., 1.);
 * log10(d) = log2(d) / log2(10);
 * d = binfrac * 2 ^ binexp, so log2(d) = log2(binfrac) + binexp.
 * If binfrac and binexp are the results of frexp(),
 * then binfrac is in [.5, 1.) and log2(binfrac) is in [-1., 0).
 * Together these imply that (binexp - 1 / log2(10)) is a good lower bound
 * for decexp, off by at most 1.
 *
 * Exceptions:
 *	Error	Return	Store	Condition
 *	none	0.0	0.0	d is 0.0
 *	EDOM	NaN	NaN	d is NaN
 *	ERANGE	[+-]1.0	INT_MAX	d is [+-]Infinity
 */
static
double
frexp10(double d, int *expp)
{
	register decexp;
	int binexp;
	double dexp;

#if	0
	/*
	 * The code here to do NaNs, infinities and 0.0 in not needed,
	 * because fpcvt() checks for NaNs and infinities
	 * and significand() checks for 0.0 before frexp10() gets called.
	 */
#if	defined(__IEEE_FP__)
	if (_isNaN(d)) {
		errno = EDOM;
		*expp = 0.0;
		return d;
	} else if (_isInfinity(d)) {
		errno = ERANGE;
		*expp = INT_MAX;
		return (d < 0.0) ? -1.0 : 1.0;
	}
#endif	/* defined(__IEEE_FP__) */

	if (d == 0.0) {
		*expp = d;
		return d;
	}
#endif	/* 0 */

	(void)frexp(d, &binexp);		/* compute binary exponent */
	if (modf((--binexp) / LOG2_10, &dexp) < 0.0)	/* get decimal exponent */
		dexp -= 1.0;			/* adjust if negative */
	decexp = (int)dexp;			/* convert to int */

	/*
	 * Scale d to desired range.
	 * Avoid using _pow10(decexp) for decexp < 0 to avoid imprecision.
	 */
	if (decexp > 0)
		d /= _pow10(decexp);
	else if (decexp < 0)
		d *= _pow10(-decexp);
	if (d >= 10.) {				/* missed by 1, adjust */
		++decexp;
		d /= 10.;			/* not "d *= .1;" for precision */
	}

	/* Store the decimal exponent and return. */
	*expp = decexp;
	return d;
}
