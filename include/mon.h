/*	@(#)mon.h 1.1 92/07/30 SMI; from S5R2 1.6	*/

#ifndef _mon_h
#define _mon_h

struct hdr {
	char	*lpc;
	char	*hpc;
	int	nfns;
};

struct cnt {
	char	*fnpc;
	long	mcnt;
};

typedef unsigned short WORD;

#define MON_OUT	"mon.out"
#define MPROGS0	(150 * sizeof(WORD))	/* 300 for pdp11, 600 for 32-bits */
#define MSCALE0	4
#ifndef NULL
#define NULL	0
#endif

#endif /*!_mon_h*/
