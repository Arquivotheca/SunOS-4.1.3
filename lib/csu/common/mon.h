/*	@(#)mon.h 1.1 92/07/30 SMI; from S5R2 1.6	*/

struct phdr {
    char	*lpc;
    char	*hpc;
    int		ncnt;
};

typedef unsigned short WORD;

    /*
     *	fraction of text space to allocate for histogram counters
     *	here, 1/2
     */
#define	HISTFRACTION	2

     /*
      *	percent of text space to allocate for counters
      *	with a minimum.
      */
#define ARCDENSITY	5
#define MINARCS		50

struct cnt {
    int		*pc;
    long	ncall;
};

#define MON_OUT	"mon.out"
#define MPROGS0	(150 * sizeof(WORD))	/* 300 for pdp11, 600 for 32-bits */
#define MSCALE0	4
#ifndef NULL
#define NULL	0
#endif
