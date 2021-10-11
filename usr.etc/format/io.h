
/*      @(#)io.h 1.1 92/07/30 SMI   */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#ifndef	_IO_
#define	_IO_

/*
 * This file contains declarations and definitions for I/O.
 */

extern	printf(), fprint(), eprint(), lprint(), nolprint();

/*
 * This declaration is the structure of bounds for integer and disk
 * input.
 */
struct bounds {
	daddr_t	lower, upper;
};

/*
 * These declarations define the legal input types.
 */
#define	FIO_BN		0
#define	FIO_INT		1
#define	FIO_CSTR	2
#define	FIO_MSTR	3
#define	FIO_OSTR	4
#define	FIO_BLNK	5

/*
 * Miscellaneous definitions.
 */
#define	TOKEN_SIZE	36			/* max length of a token */
typedef	char TOKEN[TOKEN_SIZE+1];		/* token type */
#define	DATA_INPUT	0			/* 2 modes of input */
#define	CMD_INPUT	1
#define	WILD_STRING	"$"			/* wildcard character */
#define	COMMENT_CHAR	'#'			/* comment character */

#endif	!_IO_

