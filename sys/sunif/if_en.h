/*	@(#)if_en.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Structure of a Ethernet header.
 */

#ifndef _sunif_if_en_h
#define _sunif_if_en_h

struct	en_header {
	u_char	en_dhost;
	u_char	en_shost;
	u_short	en_type;
};

#define	ENPUP_PUPTYPE	0x0200		/* PUP protocol */
#define	ENPUP_IPTYPE	0x0201		/* IP protocol */

/*
 * The ENPUP_NTRAILER packet types starting at ENPUP_TRAIL have
 * (type-ENPUP_TRAIL)*512 bytes of data followed
 * by a PUP type (as given above) and then the (variable-length) header.
 */
#define	ENPUP_TRAIL	0x1000		/* Trailer PUP */
#define	ENPUP_NTRAILER	16

#endif /*!_sunif_if_en_h*/
