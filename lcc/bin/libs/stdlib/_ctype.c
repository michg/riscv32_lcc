/*
 * lib-src/ansi/ctype/carray.c
 * ANSI/ISO 9899-1990, Section 7.3.
 *
 * char _ctype[]
 * Global array defining character types, referenced by ctype macros.
 * This is for the "C" locale only, using the ASCII character set.
 */

#include <ctype.h>

#define	_IS_CTL_SP	(_IS_CTL | _IS_SP )
#define	_IS_UPP_HEX	(_IS_UPP | _IS_HEX)
#define	_IS_LOW_HEX	(_IS_LOW | _IS_HEX)

char _ctype[257] = {
	_IS_EOF,							/* EOF (-1)        */
	_IS_CTL,	_IS_CTL,	_IS_CTL,	_IS_CTL,	/* NUL SOH STX ETX */
	_IS_CTL,	_IS_CTL,	_IS_CTL,	_IS_CTL,	/* EOT ENQ ACK BEL */
	_IS_CTL,	_IS_CTL_SP,	_IS_CTL_SP,	_IS_CTL_SP,	/* BS  HT  LF  VT  */
	_IS_CTL_SP,	_IS_CTL_SP,	_IS_CTL,	_IS_CTL,	/* FF  CR  SO  SI  */
	_IS_CTL,	_IS_CTL,	_IS_CTL,	_IS_CTL,	/* DLE DC1 DC2 DC3 */
	_IS_CTL,	_IS_CTL,	_IS_CTL,	_IS_CTL,	/* DC4 NAK SYN ETB */
	_IS_CTL,	_IS_CTL,	_IS_CTL,	_IS_CTL,	/* CAN EM  SUB ESC */
	_IS_CTL,	_IS_CTL,	_IS_CTL,	_IS_CTL,	/* FS  GS  RS  US  */
	_IS_SP,		_IS_PUN,	_IS_PUN,	_IS_PUN,	/* SP   !   "   #  */
	_IS_PUN,	_IS_PUN,	_IS_PUN,	_IS_PUN,	/*  $   %   &   '  */
	_IS_PUN,	_IS_PUN,	_IS_PUN,	_IS_PUN,	/*  (   )   *   +  */
	_IS_PUN,	_IS_PUN,	_IS_PUN,	_IS_PUN,	/*  ,   -   .   /  */
	_IS_DIG,	_IS_DIG,	_IS_DIG,	_IS_DIG,	/*  0   1   2   3  */
	_IS_DIG,	_IS_DIG,	_IS_DIG,	_IS_DIG,	/*  4   5   6   7  */
	_IS_DIG,	_IS_DIG,	_IS_PUN,	_IS_PUN,	/*  8   9   :   ;  */
	_IS_PUN,	_IS_PUN,	_IS_PUN,	_IS_PUN,	/*  <   =   >   ?  */
	_IS_PUN,	_IS_UPP_HEX,	_IS_UPP_HEX,	_IS_UPP_HEX,	/*  @   A   B   C  */
	_IS_UPP_HEX,	_IS_UPP_HEX,	_IS_UPP_HEX,	_IS_UPP,	/*  D   E   F   G  */
	_IS_UPP,	_IS_UPP,	_IS_UPP,	_IS_UPP,	/*  H   I   J   K  */
	_IS_UPP,	_IS_UPP,	_IS_UPP,	_IS_UPP,	/*  L   M   N   O  */
	_IS_UPP,	_IS_UPP,	_IS_UPP,	_IS_UPP,	/*  P   Q   R   S  */
	_IS_UPP,	_IS_UPP,	_IS_UPP,	_IS_UPP,	/*  T   U   V   W  */
	_IS_UPP,	_IS_UPP,	_IS_UPP,	_IS_PUN,	/*  X   Y   Z   [  */
	_IS_PUN,	_IS_PUN,	_IS_PUN,	_IS_PUN,	/*  \   ]   ^   _  */
	_IS_PUN,	_IS_LOW_HEX,	_IS_LOW_HEX,	_IS_LOW_HEX,	/*  `   a   b   c  */
	_IS_LOW_HEX,	_IS_LOW_HEX,	_IS_LOW_HEX,	_IS_LOW,	/*  d   e   f   g  */
	_IS_LOW,	_IS_LOW,	_IS_LOW,	_IS_LOW,	/*  h   i   j   k  */
	_IS_LOW,	_IS_LOW,	_IS_LOW,	_IS_LOW,	/*  l   m   n   o  */
	_IS_LOW,	_IS_LOW,	_IS_LOW,	_IS_LOW,	/*  p   q   r   s  */
	_IS_LOW,	_IS_LOW,	_IS_LOW,	_IS_LOW,	/*  t   u   v   w  */
	_IS_LOW,	_IS_LOW,	_IS_LOW,	_IS_PUN,	/*  x   y   z   {  */
	_IS_PUN,	_IS_PUN,	_IS_PUN,	_IS_CTL,	/*  |   }   ~  DEL */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0x80 to 0x87 */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0x88 to 0x8F */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0x90 to 0x97 */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0x98 to 0x9F */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0xA0 to 0xA7 */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0xA8 to 0xAF */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0xB0 to 0xB7 */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0xB8 to 0xBF */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0xC0 to 0xC7 */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0xC8 to 0xCF */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0xD0 to 0xD7 */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0xD8 to 0xDF */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0xE0 to 0xE7 */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0xE8 to 0xEF */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,	/* 0xF0 to 0xF7 */
 _IS_CTL, _IS_CTL,_IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL	/* 0xF8 to 0xFF */
};
