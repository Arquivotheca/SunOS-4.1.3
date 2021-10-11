/*	@(#)ex_vars.h 1.1 92/07/30 SMI; from S5R3.1 1.7	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#define	vi_AUTOINDENT		0
#define	vi_AUTOPRINT		1
#define	vi_AUTOWRITE		2
#define	vi_BEAUTIFY		3
#define	vi_DIRECTORY		4
#define	vi_EDCOMPATIBLE		5
#define	vi_ERRORBELLS		6
#define	vi_FLASH		7
#define	vi_HARDTABS		8
#define	vi_IGNORECASE		9
#define	vi_LISP			10
#define	vi_LIST			11
#define	vi_MAGIC		12
#define	vi_MESG			13
#define	vi_MODELINE		14
#define	vi_NUMBER		15
#define	vi_NOVICE		16
#define	vi_OPTIMIZE		17
#define	vi_PARAGRAPHS		18
#define	vi_PROMPT		19
#define	vi_READONLY		20
#define	vi_REDRAW		21
#define	vi_REMAP		22
#define	vi_REPORT		23
#define	vi_SCROLL		24
#define	vi_SECTIONS		25
#define	vi_SHELL		26
#define	vi_SHIFTWIDTH		27
#define	vi_SHOWMATCH		28
#define	vi_SHOWMODE		29
#define	vi_SLOWOPEN		30
#define vi_SOURCEANY		31
#define	vi_TABSTOP		32
#define	vi_TAGLENGTH		33
#define	vi_TAGS			34
#ifdef TAG_STACK
#define vi_TAGSTACK		35
#define	vi_TERM			36
#define	vi_TERSE		37
#define	vi_TIMEOUT		38
#define	vi_TTYTYPE		39
#define	vi_WARN			40
#define	vi_WINDOW		41
#define	vi_WRAPSCAN		42
#define	vi_WRAPMARGIN		43
#define	vi_WRITEANY		44

#define	vi_NOPTS	45
#else
#define	vi_TERM			35
#define	vi_TERSE		36
#define	vi_TIMEOUT		37
#define	vi_TTYTYPE		38
#define	vi_WARN			39
#define	vi_WINDOW		40
#define	vi_WRAPSCAN		41
#define	vi_WRAPMARGIN		42
#define	vi_WRITEANY		43

#define	vi_NOPTS	44
#endif
