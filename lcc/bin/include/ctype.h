/*	
 * ctype.h
 * Character handling.
 * ANSI/ISO 9899-1990, Section 7.3.
 */

#ifndef __CTYPE_H__
#define __CTYPE_H__

#if	defined(__cplusplus)
extern	"C"	{
#endif	/* defined(__cplusplus) */

/* Type classification macros. */
#define _IS_SP	0x01			/* is space			*/
#define _IS_DIG	0x02			/* is digit indicator		*/
#define _IS_UPP	0x04			/* is upper case		*/
#define _IS_LOW	0x08			/* is lower case		*/
#define _IS_HEX	0x10			/* is [0..9] or [A-F] or [a-f]	*/
#define _IS_CTL	0x20			/* is control (not printable)	*/
#define _IS_PUN	0x40			/* is punctuation		*/
#define _IS_EOF	0x80			/* is EOF			*/

extern	char  _ctype[];			/* Bits about each character	*/

/* Standard functions. */
int isalnum (int);
int isalpha (int);
int iscntrl (int);
int isdigit (int);
int isgraph (int);
int islower (int);
int isprint (int);
int ispunct (int);
int isspace (int);
int isupper (int);
int isxdigit(int);
int tolower (int);
int toupper (int);

/* Macros covering standard functions. */
#define isalnum(c)	(_ctype[(c) + 1] & (_IS_DIG | _IS_UPP | _IS_LOW))
#define isalpha(c)	(_ctype[(c) + 1] & (_IS_UPP | _IS_LOW))
#define iscntrl(c)	(_ctype[(c) + 1] & _IS_CTL)
#define isdigit(c)	(_ctype[(c) + 1] & _IS_DIG)
#define isgraph(c)    (!(_ctype[(c) + 1] & (_IS_EOF | _IS_CTL | _IS_SP)))
#define islower(c)	(_ctype[(c) + 1] & _IS_LOW)
#define isprint(c)    (!(_ctype[(c) + 1] & (_IS_EOF | _IS_CTL)))
#define ispunct(c)	(_ctype[(c) + 1] & _IS_PUN)
#define isspace(c)	(_ctype[(c) + 1] & _IS_SP)
#define isupper(c)	(_ctype[(c) + 1] & _IS_UPP)
#define isxdigit(c)	(_ctype[(c) + 1] & (_IS_DIG | _IS_HEX))

#if	!defined(__STRICT_ANSI__)

/* isascii() is not required by ANSI/ISO 9899-1990, but here for convenience. */
int isascii (int);
#define isascii(c)	((unsigned)(c) < 128)

#endif	/* !defined(__STRICT_ANSI__) */

#if	defined(__cplusplus)
}
#endif	/* defined(__cplusplus) */

#endif  /* __CTYPE_H__ */
