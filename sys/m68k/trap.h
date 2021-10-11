/*	@(#)trap.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef _m68k_trap_h
#define _m68k_trap_h

/*
 * Trap type values
 */

#define	T_M_ERRORVEC	0x00	/* magic number for any uninitialized vectors */
#define	T_BUSERR	0x08
#define	T_ADDRERR	0x0c
#define	T_ILLINST	0x10
#define	T_ZERODIV	0x14
#define	T_CHKINST	0x18
#define	T_TRAPV		0x1c
#define	T_PRIVVIO	0x20
#define	T_TRACE		0x24
#define	T_EMU1010	0x28
#define	T_EMU1111	0x2c
#define	T_COPROCERR	0x34
#define	T_FMTERR	0x38
#define	T_SPURIOUS	0x60
#define	T_LEVEL1	0x64
#define	T_LEVEL2	0x68
#define	T_LEVEL3	0x6c
#define	T_LEVEL4	0x70
#define	T_LEVEL5	0x74
#define	T_LEVEL6	0x78
#define	T_LEVEL7	0x7c
#define	T_SYSCALL	0x80
#define	T_M_BADTRAP	0x84	/* magic number for traps 1-14 */
#define	T_BRKPT		0xbc
#define	T_M_FLOATERR	0xc0	/* magic number for float traps */
#define	T_M_MMUERR	0xe0	/* magic number for mmu errors */

#endif /*!_m68k_trap_h*/
