/*      @(#)ppreg.h 1.1 92/07/30 SMI          */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * (ppreg.h) Sun-3x Parallel Port Registers
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ident "TBD"

struct pp_reg {
	unsigned char filler1[0x30];
	unsigned char pp_intr;
	unsigned char filler2[0xb];
	unsigned char pp_data;	/* 00:Data Register */
	unsigned char pp_stat;	/* 01:Status Register */
	unsigned char pp_cntrl;	/* 10:Control Register */
	unsigned char pp_inval;	/* 11:Invalid Register */
};

/* Printer Control Reg bits */
/* #define	PC_INTENABLE	0x10	*/
#define	PC_INTENABLE	0x10	/* +IRQ ENABLE: enable ACK interrupts */
#define	PC_SELECT	0x08	/* +SLCT IN: select printer */
#define	PC_INIT		0x04	/* -INIT: init printer */
#define	PC_LINEFEED	0x02	/* +AUTO FD XT: set auto linefeed */
#define	PC_STROBE	0x01	/* +STROBE: strobe data */

#define	PC_NORM		(PC_INTENABLE|PC_SELECT|PC_INIT)
#define	PC_OFF		(PC_SELECT|PC_INIT)
#define	PC_RESET	(PC_SELECT)

/* Printer Status Reg bits */
#define	PS_READY	0x80	/* -BUSY: printer not busy */
#define	PS_NOTACK	0x40	/* -ACK: ACK state */
#define	PS_NOPAPER	0x20	/* +PE: printer out of paper */
#define	PS_SELECT	0x10	/* +SLCT: printer is selected */
#define	PS_NOERROR	0x08	/* -ERROR: printer error condition */

#define	PSREADY(s)	((s)&PS_READY)
#define	PSSELECT(s)	((s)&PS_SELECT)
#define	PSNOPAPER(s)	((s)&PS_NOPAPER)
#define	PSERROR(s)	(((s)&PS_NOERROR) == 0)

#define PPIOCGETS	_IOR(p, 0, char)
#define PPIOCGETC	_IOR(p, 1, char)
#define PPIOCSETC	_IOW(p, 2, char)
