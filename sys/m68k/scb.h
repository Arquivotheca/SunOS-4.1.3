/*	@(#)scb.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Sun System control block layout
 * [] denoted 68020/30 particulars
 * <> denotes 68030 particulars.
 */

#ifndef _m68k_scb_h
#define _m68k_scb_h

struct scb {
	int	scb_issp;		/* 00 - initial SSP */
	int	(*scb_ipc)();		/* 04 - initial PC */
	int	(*scb_buserr)();	/* 08 - bus error */
	int	(*scb_addrerr)();	/* 0c - address error */
	int	(*scb_illinst)();	/* 10 - illegal instruction */
	int	(*scb_zerodiv)();	/* 14 - zero divide */
	int	(*scb_chk)();		/* 18 - CHK [CHK2] instruction */
	int	(*scb_trapv)();		/* 1c - TRAPV [cpTRAPcc TRAPcc] instr */
	int	(*scb_priv)();		/* 20 - privilege violation */
	int	(*scb_trace)();		/* 24 - trace trap */
	int	(*scb_e1010)();		/* 28 - line 1010 emulator */
	int	(*scb_e1111)();		/* 2c - line 1111 emulator */
	int	(*scb_res30)();		/* 30 - reserved */
	int	(*scb_coprocerr)();	/* 34 - [coprocessor protocol error] */
	int	(*scb_fmterr)();	/* 38 - RTE format error */
	int	(*scb_uninit)();	/* 3c - uninitialized interrupt */
	int	(*scb_res1[8])();	/* 40-5c - reserved */
	int	(*scb_stray)();		/* 60 - spurious interrupt */
	int	(*scb_autovec[7])();	/* 64-7c - level 1-7 autovectors */
	int	(*scb_trap[16])();	/* 80-bc - trap instruction vectors */
	int	(*scb_flterr[7])();	/* c0-d8 - [floating cop errors] */
	int	(*scb_resdc)();		/* dc - reserved */
	int	(*scb_mmuerr[3])();	/* e0-e8 - <mmu errors> */
	int	(*scb_res2[5])();	/* ec-fc - reserved */
	int	(*scb_user[192])();	/* 100-3fc - user interrupt vectors */
};

#define	AUTOBASE (0x60 / sizeof (int))	/* autovector base vector number */
#define	VEC_MIN	(0x100 / sizeof (int))	/* minimum vectored interrupt number */
#define	VEC_MAX	(0x3fc / sizeof (int))	/* maximum vectored interrupt number */

#ifdef KERNEL
extern	struct scb scb, protoscb;
#endif

#endif /*!_m68k_scb_h*/
