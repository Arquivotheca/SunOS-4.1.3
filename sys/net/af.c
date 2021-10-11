/*	@(#)af.c 1.1 92/07/30 SMI; from UCB 7.1 6/4/86	*/

/*
 * Copyright (c) 1983, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <net/af.h>

/*
 * Address family support routines
 */
int	null_hash(), null_netmatch();
#define	AFNULL \
	{ null_hash,	null_netmatch }

#ifdef INET
extern int inet_hash(), inet_netmatch();
#define	AFINET \
	{ inet_hash,	inet_netmatch }
#else
#define	AFINET	AFNULL
#endif

struct afswitch afswitch[AF_MAX] = {
	AFNULL,	AFNULL,	AFINET,	AFINET,	AFNULL,		/*  0 -  4 */
	AFNULL,	AFNULL,	AFNULL,	AFNULL,	AFNULL,		/*  5 -  9 */
	AFNULL,	AFNULL,	AFNULL,	AFNULL,	AFNULL,		/* 10 - 14 */
	AFNULL,	AFNULL,	AFNULL,	AFNULL			/* 15 - 19 */
};

null_init()
{
	register struct afswitch *af;

	for (af = afswitch; af < &afswitch[AF_MAX]; af++)
		if (af->af_hash == (int (*)())NULL) {
			af->af_hash = null_hash;
		}
}

/*ARGSUSED*/
null_hash(addr, hp)
	struct sockaddr *addr;
	struct afhash *hp;
{

	hp->afh_nethash = hp->afh_hosthash = 0;
}

/*ARGSUSED*/
null_netmatch(a1, a2)
	struct sockaddr *a1, *a2;
{

	return (0);
}


/*
 * Add a new protocol family to the socket system.
 */
protocol_family(dp, hash, netmatch)
	register struct domain *dp;
	int (*hash)();
	int (*netmatch)();
{
# ifdef lint
	if (dp != dp) protocol_family(dp, hash, netmatch);
# endif lint
	if (hash != (int (*)())NULL)
		afswitch[dp->dom_family].af_hash = hash;
	if (netmatch != (int (*)())NULL)
		afswitch[dp->dom_family].af_netmatch = netmatch;
	dp->dom_next = domains;
	domains = dp;
}
