/*	@(#)ctype.h 1.1 92/07/30 SMI; from S5R3.1 1.9	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/
/*	Portions Copyright (c) 1989 Sun Microsystems, Inc.	*/
/*	All Rights Reserved	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	__ctype_h
#define	__ctype_h

#define	_U	01	/* Upper case */
#define	_L	02	/* Lower case */
#define	_N	04	/* Numeral (digit) */
#define	_S	010	/* Spacing character */
#define	_P	020	/* Punctuation */
#define	_C	040	/* Control character */
#define	_X	0100	/* heXadecimal digit */
#define	_B	0200	/* Blank */

extern int	isalnum(/* int c */);
extern int	isalpha(/* int c */);
#ifndef	_POSIX_SOURCE
extern int	isascii(/* int c */);
#endif
extern int	iscntrl(/* int c */);
extern int	isdigit(/* int c */);
extern int	isgraph(/* int c */);
extern int	islower(/* int c */);
extern int	isprint(/* int c */);
extern int	ispunct(/* int c */);
extern int	isspace(/* int c */);
extern int	isupper(/* int c */);
extern int	isxdigit(/* int c */);
#ifndef	_POSIX_SOURCE
extern int	toascii(/* int c */);
#endif
extern int	tolower(/* int c */);
extern int	toupper(/* int c */);

#ifndef	lint

#define	isalnum(c)	((_ctype_ + 1)[c] & (_U | _L | _N))
#define	isalpha(c)	((_ctype_ + 1)[c] & (_U | _L))
#ifndef	_POSIX_SOURCE
#define	isascii(c)	(!((c) & ~0177))
#endif
#define	iscntrl(c)	((_ctype_ + 1)[c] & _C)
#define	isdigit(c)	((_ctype_ + 1)[c] & _N)
#define	isgraph(c)	((_ctype_ + 1)[c] & (_P | _U | _L | _N))
#define	islower(c)	((_ctype_ + 1)[c] & _L)
#define	isprint(c)	((_ctype_ + 1)[c] & (_P | _U | _L | _N | _B))
#define	ispunct(c)	((_ctype_ + 1)[c] & _P)
#define	isspace(c)	((_ctype_ + 1)[c] & _S)
#define	isupper(c)	((_ctype_ + 1)[c] & _U)
#define	isxdigit(c)	((_ctype_ + 1)[c] & _X)
#ifndef	_POSIX_SOURCE
#define	toascii(c)	((c) & 0177)
/*
 * These upper/lower macros are not codeset independent
 */

#define _toupper(c)      ((c) - 'a' + 'A')
#define _tolower(c)      ((c) - 'A' + 'a')
#endif

extern char	_ctype_[];

#endif	/* lint */

#endif	/* !__ctype_h */
