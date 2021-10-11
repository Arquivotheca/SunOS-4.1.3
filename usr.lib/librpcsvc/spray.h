/*	@(#)spray.h 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef _rpcsvc_spray_h
#define _rpcsvc_spray_h

#define SPRAYPROG 100012
#define SPRAYPROC_SPRAY 1
#define SPRAYPROC_GET 2
#define SPRAYPROC_CLEAR 3
#define SPRAYVERS_ORIG 1
#define SPRAYVERS 1

#define SPRAYOVERHEAD 86	/* size of rpc packet when size=0 */
#define SPRAYMAX 8845		/* related to max udp packet of 9000 */

int xdr_sprayarr();
int xdr_spraycumul();

struct spraycumul {
	unsigned counter;
	struct timeval clock;
};

struct sprayarr {
	int *data;
	int lnth;
};

#endif /*!_rpcsvc_spray_h*/
