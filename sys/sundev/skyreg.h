/*	@(#)skyreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Sky FFP
 */

#ifndef _sundev_skyreg_h
#define _sundev_skyreg_h

struct	skyreg {
	u_short	sky_command;
	u_short	sky_status;
	union {
		short	skyu_dword[2];
		long	skyu_dlong;
	} skyu;
#define	sky_data	skyu.skyu_dlong
#define	sky_d1reg	skyu.skyu_dword[0]
	long	sky_ucode;
	u_short	sky_vector;	/* VME: interrupt vector number */
};

/* commands */
#define	SKY_SAVE	0x1040
#define	SKY_RESTOR	0x1041
#define	SKY_NOP		0x1063
#define	SKY_START0	0x1000
#define	SKY_START1	0x1001

/* status bits */
#define	SKY_IHALT	0x0000
#define	SKY_INTRPT	0x0003
#define	SKY_INTENB	0x0010
#define	SKY_RUNENB	0x0040
#define	SKY_SNGRUN	0x0060
#define	SKY_RESET	0x0080
#define	SKY_IODIR	0x2000
#define	SKY_IDLE	0x4000
#define	SKY_IORDY	0x8000

#endif /*!_sundev_skyreg_h*/
