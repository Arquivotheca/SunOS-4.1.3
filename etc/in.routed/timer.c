/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)timer.c 1.1 92/07/30 SMI"; /* from UCB 5.3 11/20/85 */
#endif

/*
 * Routing Table Management Daemon
 */
#include "defs.h"

int	timeval = -TIMER_RATE;

/*
 * Timer routine.  Performs routing information supply
 * duties and manages timers on routing table entries.
 */
timer()
{
	register struct rthash *rh;
	register struct rt_entry *rt;
	struct rthash *base = hosthash;
	int doinghost = 1, timetobroadcast;

	timeval += TIMER_RATE;
	if (lookforinterfaces && (timeval % CHECK_INTERVAL) == 0)
		ifinit();
	timetobroadcast = supplier && (timeval % SUPPLY_INTERVAL) == 0;
again:
	for (rh = base; rh < &base[ROUTEHASHSIZ]; rh++) {
		rt = rh->rt_forw;
		for (; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
			/*
			 * We don't advance time on a routing entry for
			 * a passive gateway or that for a interface. 
			 * The latter is excused because we catch interfaces
			 * going up and down in ifinit.
			 * If the timer is already expired, it may be a dead
			 * interface so we advance it until garbage time.
			 */
			if (rt->rt_timer >= EXPIRE_TIME ||
			   (rt->rt_state & (RTS_PASSIVE|RTS_INTERFACE)) == 0)
				rt->rt_timer += TIMER_RATE;
			if (rt->rt_timer >= EXPIRE_TIME)
				rtdown(rt);
			if (rt->rt_timer >= GARBAGE_TIME) {
				rt = rt->rt_back;
				rtdelete(rt->rt_forw);
				continue;
			}
			if (rt->rt_state & RTS_CHANGED) {
				rt->rt_state &= ~RTS_CHANGED;
				/* don't send extraneous packets */
				if (!supplier || timetobroadcast)
					continue;
				msg->rip_cmd = RIPCMD_RESPONSE;
				msg->rip_vers = RIPVERSION;
				msg->rip_nets[0].rip_dst = rt->rt_dst;
				msg->rip_nets[0].rip_dst.sa_family =
				   htons(msg->rip_nets[0].rip_dst.sa_family);
				msg->rip_nets[0].rip_metric =
				   htonl(min(rt->rt_metric+1, HOPCNT_INFINITY));
				toall(sendpacket);
			}
		}
	}
	if (doinghost) {
		doinghost = 0;
		base = nethash;
		goto again;
	}
	if (timetobroadcast)
		toall(supply);
	alarm(TIMER_RATE);
}

/*
 * On hangup, let everyone know we're going away.
 */
hup()
{
	register struct rthash *rh;
	register struct rt_entry *rt;
	struct rthash *base = hosthash;
	int doinghost = 1;

	if (supplier) {
again:
		for (rh = base; rh < &base[ROUTEHASHSIZ]; rh++) {
			rt = rh->rt_forw;
			for (; rt != (struct rt_entry *)rh; rt = rt->rt_forw)
				rt->rt_metric = HOPCNT_INFINITY;
		}
		if (doinghost) {
			doinghost = 0;
			base = nethash;
			goto again;
		}
		toall(supply);
	}
	exit(1);
}
