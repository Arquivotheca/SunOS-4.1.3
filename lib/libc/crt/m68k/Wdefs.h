/*	@(#)Wdefs.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*	Definitions for Sun Floating Point Accelerator. */

#define	FPABASEADDRESS	0xe0000000
#define FPABASE a1

#define WNOP	0
#define WNEG	1
#define WABS	2
#define WFLT	3
#define WCVT	5
#define WSQR	6
#define WADD	7
#define WSUB	8
#define WMUL	9
#define WDIV	10
#define WRSUB	11
#define WRDIV	12
#define WTST	13
#define WCMP	14
#define WMCMP	15
#define WDINT	0x38	

/* Weitek Modes */

#define WIEEE	0
#define WFAST	1
#define WRINT	0
#define WINT	2
#define WRN	0
#define WRP	8
#define WRM	12
#define WRZ	4

#define LOADFPABASE	\
	lea	FPABASEADDRESS,FPABASE
#define READFPACC \
	fpmove	fpastatus,d0 ; \
	movew	d0,cc
#define SWCOMMAND(c) \
	movel	#0,FPABASEADDRESS+(8*c+0xa00)
#define SWRITER0	\
	fpmoves	d0,fpa0
#define SMONADOP(f)	\
	fp/**/f/**/s d0,fpa0
#define SDYADOP(f)	\
	fp/**/f/**/s d1,fpa0
#define SREADR0		\
	fpmoves	fpa0,d0
#define SMONADIC(e,f) \
RTENTRY(e) ; \
	SMONADOP(f) ; \
	SREADR0 ; \
	RET

#define SDYADIC(e,f) \
RTENTRY(e) ; \
	SWRITER0 ; \
	SDYADOP(f) ; \
	SREADR0 ; \
	RET

#define DWCOMMAND(c) \
	movel	#0,FPABASEADDRESS+(8*c+0xa04)
#define DWRITER0	\
	fpmoved	d0:d1,fpa0
#define DMONADOP(f)	\
	fp/**/f/**/d d0:d1,fpa0
#define DDYADOP(f)	\
	fp/**/f/**/d a0@,fpa0
#define DREADR0		\
	fpmoved	fpa0,d0:d1
#define DMONADIC(e,f) \
RTENTRY(e) ; \
	DMONADOP(f) ; \
	DREADR0 ; \
	RET

#define DDYADIC(e,f) \
RTENTRY(e) ; \
	DWRITER0 ; \
	DDYADOP(f) ; \
	DREADR0 ; \
	RET
