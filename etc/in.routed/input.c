/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)input.c 1.1 92/07/30 SMI"; /* from UCB 5.6 6/3/86 */
#endif

/*
 * Routing Table Management Daemon
 */
#include "defs.h"
#include <syslog.h>

/*
 * Process a newly received packet.
 */
rip_input(from, size)
	struct sockaddr *from;
	int size;
{
	register struct rt_entry *rt;
	register struct netinfo *n;
	register struct interface *ifp;
	int newsize;
	register struct afswitch *afp;
	int answer = supplier;
	struct entryinfo	*e;

	ifp = 0;
	TRACE_INPUT(ifp, from, size);
	if (from->sa_family >= af_max ||
	    (afp = &afswitch[from->sa_family])->af_hash == (int (*)())0) {
		return;
	}
	switch (msg->rip_cmd) {

	case RIPCMD_POLL:		/* request specifically for us */
		answer = 1;
		/* fall through */
	case RIPCMD_REQUEST:		/* broadcasted request */
		newsize = 0;
		size -= 4 * sizeof (char);
		n = msg->rip_nets;
		while (size > 0) {
			if (size < sizeof (struct netinfo))
				break;
			size -= sizeof (struct netinfo);

			if (msg->rip_vers > 0) {
				n->rip_dst.sa_family =
					ntohs(n->rip_dst.sa_family);
				n->rip_metric = ntohl(n->rip_metric);
			}
			/* 
			 * A single entry with sa_family == AF_UNSPEC and
			 * metric ``infinity'' means ``all routes''.
			 * We respond to routers only if we are acting
			 * as a supplier, or to anyone other than a router
			 * (eg, query).
			 */
			if (n->rip_dst.sa_family == AF_UNSPEC &&
			    n->rip_metric == HOPCNT_INFINITY && size == 0) {
				if (answer || (*afp->af_portmatch)(from) == 0)
					supply(from, 0, ifp);
				return;
			}
			if (n->rip_dst.sa_family < af_max &&
			    afswitch[n->rip_dst.sa_family].af_hash)
				rt = rtlookup(&n->rip_dst);
			else
				rt = 0;
			n->rip_metric = rt == 0 ? HOPCNT_INFINITY :
				min(rt->rt_metric+1, HOPCNT_INFINITY);
			if (msg->rip_vers > 0) {
				n->rip_dst.sa_family =
					htons(n->rip_dst.sa_family);
				n->rip_metric = htonl(n->rip_metric);
			}
			n++, newsize += sizeof (struct netinfo);
		}
		if (answer && newsize > 0) {
			msg->rip_cmd = RIPCMD_RESPONSE;
			newsize += sizeof (int);
			(*afp->af_output)(s, 0, from, newsize);
		}
		return;

	case RIPCMD_TRACEON:
	case RIPCMD_TRACEOFF:
		/* verify message came from a privileged port */
		if ((*afp->af_portcheck)(from) == 0)
			return;
		if (if_iflookup(from) == 0) {
			syslog(LOG_ERR, "trace command from unknown router, %s",
			   (*afswitch[from->sa_family].af_format)(from));
			return;
		}
		packet[size] = '\0';
		if (msg->rip_cmd == RIPCMD_TRACEON)
			traceon(msg->rip_tracefile);
		else
			traceoff();
		return;

	case RIPCMD_RESPONSE:
		/* verify message came from a router */
		if ((*afp->af_portmatch)(from) == 0)
			return;
		(*afp->af_canon)(from);
		/* are we talking to ourselves? */
		ifp = if_ifwithaddr(from);
		if (ifp) {
			rt = rtfind(from);
			if (rt == 0 || (rt->rt_state & RTS_INTERFACE) == 0)
				addrouteforif(ifp);
			else
				rt->rt_timer = 0;
			return;
		}
		/*
		 * Update timer for interface on which the packet arrived.
		 * If from other end of a point-to-point link that isn't
		 * in the routing tables, (re-)add the route.
		 */
		if ((rt = rtfind(from)) &&
		    (rt->rt_state & (RTS_INTERFACE | RTS_REMOTE)))
			rt->rt_timer = 0;
		else if (ifp = if_ifwithdstaddr(from))
			addrouteforif(ifp);
		else if (if_iflookup(from) == 0) {
			syslog(LOG_ERR, "packet from unknown router, %s",
			   (*afswitch[from->sa_family].af_format)(from));
			return;
		}
		size -= 4 * sizeof (char);
		n = msg->rip_nets;
		for (; size > 0; size -= sizeof (struct netinfo), n++) {
			if (size < sizeof (struct netinfo))
				break;
# ifndef sun
			if (msg->rip_vers > 0) {
				n->rip_dst.sa_family =
					ntohs(n->rip_dst.sa_family);
				n->rip_metric = ntohl(n->rip_metric);
			}
# endif
			if ((unsigned) n->rip_metric > HOPCNT_INFINITY)
				continue;
			if (n->rip_dst.sa_family >= af_max ||
			    (afp = &afswitch[n->rip_dst.sa_family])->af_hash ==
			    (int (*)())0) {
				if (ftrace)
fprintf(ftrace,"route in unsupported address family (%d), from %s (af %d)\n",
				   n->rip_dst.sa_family,
				   (*afswitch[from->sa_family].af_format)(from),
				   from->sa_family);
				continue;
			}
			if (((*afp->af_checkhost)(&n->rip_dst)) == 0) {
				if (ftrace) {
		fprintf(ftrace,"bad host %s ",
			 (*afswitch[from->sa_family].af_format)(&n->rip_dst));
		fprintf(ftrace,"in route from %s, metric %d\n",
			 (*afswitch[from->sa_family].af_format)(from),
			 n->rip_metric );
				   fflush(ftrace);
				}
				continue;
			}
			rt = rtlookup(&n->rip_dst);
			/*
			 * 4.3 BSD for some reason also special-cased
			 * INTERNAL INTERFACE routes here. This causes an
			 * extra non-subnet route to appear in the routing
			 * tables of routers at the outside edges of a 
			 * subnetted network.  It does not seem to cause
			 * problems, but why????
			 */
			if (rt == 0) {
				rt = rtfind(&n->rip_dst);
				if (rt && equal(from, &rt->rt_router) &&
				    rt->rt_metric == n->rip_metric)
					continue;
				if (n->rip_metric < HOPCNT_INFINITY)
				    rtadd(&n->rip_dst, from, n->rip_metric, 0);
				continue;
			}

			/*
			 * Update if from gateway and different,
			 * shorter, or getting stale and equivalent.
			 */
			if (equal(from, &rt->rt_router)) {
				if (n->rip_metric != rt->rt_metric)
					rtchange(rt, from, n->rip_metric);
				else if (n->rip_metric < HOPCNT_INFINITY) 
					rt->rt_timer = 0;
			} else if ((unsigned) (n->rip_metric) < rt->rt_metric ||
			    (rt->rt_timer > (EXPIRE_TIME/2) &&
			    rt->rt_metric == n->rip_metric)) {
				rtchange(rt, from, n->rip_metric);
			}
		}
		return;
	case RIPCMD_POLLENTRY:
		n = msg->rip_nets;
		if (n->rip_dst.sa_family < af_max &&
		    afswitch[n->rip_dst.sa_family].af_hash)
			rt = rtlookup(&n->rip_dst);
		else
			rt = 0;
		newsize = sizeof (struct entryinfo);
		if (rt) {	/* don't bother to check rip_vers */
			e = (struct entryinfo *) n;
			e->rtu_dst = rt->rt_dst;
			e->rtu_dst.sa_family =
				ntohs(e->rtu_dst.sa_family);
			e->rtu_router = rt->rt_router;
			e->rtu_router.sa_family =
				ntohs(e->rtu_router.sa_family);
			e->rtu_flags = ntohs(rt->rt_flags);
			e->rtu_state = ntohs(rt->rt_state);
			e->rtu_timer = ntohl(rt->rt_timer);
			e->rtu_metric = ntohl(rt->rt_metric);
			ifp = rt->rt_ifp;
			if (ifp) {
				e->int_flags = ntohl(ifp->int_flags);
				strncpy(e->int_name, rt->rt_ifp->int_name,
				    sizeof(e->int_name));
			} else {
				e->int_flags = 0;
				strcpy(e->int_name, "(none)");
			}
		}
		else
			bzero(n, newsize);
		(*afp->af_output)(s, 0, from, newsize);
		return;
	}
}
