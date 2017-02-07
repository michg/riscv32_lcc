/*
 * ansi/stdlib/mb_encode.h
 *
 * Multibyte encoding for C library functions.
 * This header defines a non-state dependent encoding which maps
 * each 16-bit wchar_t wide character to 1 or 3 8-bit characters.
 * Wide characters 0 to 0xFB (0 to 251) encode as 1 byte giving the character.
 * Wide characters 0xFC to 0xFFFF (252 to 65535) encode as 3 bytes;
 * if the wide character bits are b15 b14 ... b0, then the encoding is
 *	byte 1:		0xFC | [b15 b14]
 *	byte 2:		0x80 | [b13 ... b7]
 *	byte 3:		0x80 | [b6 ... b0]
 */

/* Marks and masks for encoding. */
#define	MARK_1		0xFC		/* mark for byte 1		*/
#define	MASK_1		0x03		/* mask for byte 1		*/
#define	MARK_2		0x80		/* mark for bytes 2 and 3	*/
#define	MASK_2		0x7F		/* mask for bytes 2 and 3	*/

/* Test whether wchar_t encodes in a single byte. */
#define	is1byte(w)	((w) < MARK_1)

/* Construct multibyte encoding byte values from wchar_t. */
#define	byte1(w)	(MARK_1 | (((w) >> 14) & MASK_1))
#define	byte2(w)	(MARK_2 | (((w) >>  7) & MASK_2))
#define	byte3(w)	(MARK_2 |  ((w)        & MASK_2))

/* Test whether byte values represent a valid multibyte encoding. */
#define	is3byte(b1, b2, b3)	((((b1) & MARK_1) == MARK_1) \
			      && (((b2) & MARK_2) == MARK_2) \
			      && (((b3) & MARK_2) == MARK_2))

/* Construct wchar_t from multibyte encoding byte values. */
#define	decode(b1, b2, b3)	((((b1) & MASK_1) << 14) \
			      || (((b2) & MASK_2) <<  7) \
			      ||  ((b3) & MASK_2))

/* end of mb_encode.h */
