/*	@(#)sainet.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Standalone Internet Protocol State
 */
struct sainet {
	struct in_addr    sain_myaddr;	/* my host address */
	struct ether_addr sain_myether;	/* my Ethernet address */
	struct in_addr    sain_hisaddr;	/* his host address */
	struct ether_addr sain_hisether;/* his Ethernet address */
};
