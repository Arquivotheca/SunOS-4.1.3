#ifndef lint
static        char sccsid[] = "@(#)in_pcb.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif


#include <sys/param.h>
#include "boot/systm.h"
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in_pcb.h>
#include "boot/protosw.h"

static int dump_debug = 10;

#undef	u
extern struct user u;

struct	in_addr zeroin_addr;

in_pcballoc(so, head)
	struct socket *so;
	struct inpcb *head;
{
	struct mbuf *m;
	register struct inpcb *inp;

	m = m_getclr(M_DONTWAIT, MT_PCB);
	if (m == NULL)
		return (ENOBUFS);
	inp = mtod(m, struct inpcb *);
	inp->inp_head = head;
	inp->inp_socket = so;
	insque(inp, head);
	so->so_pcb = (caddr_t)inp;
	return (0);
}
	
in_pcbbind(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct socket *so = inp->inp_socket;
	register struct inpcb *head = inp->inp_head;
	register struct sockaddr_in *sin;
	u_short lport = 0;

	if (ifnet == 0)
		return (EADDRNOTAVAIL);
	if (inp->inp_lport || inp->inp_laddr.s_addr != INADDR_ANY)
		return (EINVAL);
	if (nam == 0)
		goto noname;
	sin = mtod(nam, struct sockaddr_in *);
#ifdef	DUMP_DEBUG1
	dprint(dump_debug, 6,
		"in_pcbbind: sin_addr 0x%x sin_port 0x%x\n",
		sin->sin_addr, sin->sin_port);
#endif	/* DUMP_DEBUG */
	if (nam->m_len != sizeof (*sin))
		return (EINVAL);
	if (sin->sin_addr.s_addr != INADDR_ANY) {
		int tport = sin->sin_port;

		sin->sin_port = 0;		/* yech... */
		if (if_ifwithaddr((struct sockaddr *)sin) == 0)
			return (EADDRNOTAVAIL);
		sin->sin_port = tport;
	}
	lport = sin->sin_port;
	if (lport) {
		u_short aport = htons(lport);
		int wild = 0;

		/* GROSS */
		if (aport < IPPORT_RESERVED && u.u_uid != 0)
			return (EACCES);
		/* even GROSSER, but this is the Internet */
		if ((so->so_options & SO_REUSEADDR) == 0 &&
		    ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0 ||
		     (so->so_options & SO_ACCEPTCONN) == 0))
			wild = INPLOOKUP_WILDCARD;
		if (in_pcblookup(head,
		    zeroin_addr, 0, sin->sin_addr, lport, wild))
			return (EADDRINUSE);
	}
	inp->inp_laddr = sin->sin_addr;
noname:
	if (lport == 0)
		do {
			if (head->inp_lport++ < IPPORT_RESERVED)
				head->inp_lport = IPPORT_RESERVED;
			lport = htons(head->inp_lport);
		} while (in_pcblookup(head,
			    zeroin_addr, 0, inp->inp_laddr, lport,
				INPLOOKUP_WILDCARD));
	inp->inp_lport = lport;
	return (0);
}

#ifdef	NEVER
/*
 * Connect from a socket to a specified address.
 * Both address and port must be specified in argument sin.
 * If don't have a local address for this socket yet,
 * then pick one.
 */
in_pcbconnect(inp, nam)
	struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);

	if (nam->m_len != sizeof (*sin))
		return (EINVAL);
	return (in_pcbsetaddr(inp, sin));
}

in_pcbsetaddr(inp, sin)
	struct inpcb *inp;
	register struct sockaddr_in *sin;
{
	struct ifnet *ifp, *if_ifwithafup();
	struct sockaddr_in *ifaddr;

	if (sin->sin_family != AF_INET)
		return (EAFNOSUPPORT);
	if (sin->sin_addr.s_addr == INADDR_ANY || sin->sin_port == 0)
		return (EADDRNOTAVAIL);
	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		ifp = if_ifonnetof(in_netof(sin->sin_addr));
		if (ifp == 0) {
			register struct route *ro;

			/* 
			 * If route is known or can be allocated now,
			 * our src addr is taken from the i/f, else punt.
			 */
			ro = &inp->inp_route;
			if (ro->ro_rt &&
		((struct sockaddr_in *) &ro->ro_dst)->sin_addr.s_addr !=
			     sin->sin_addr.s_addr) {
				RTFREE(ro->ro_rt);
				ro->ro_rt = (struct rtentry *)0;
			}
			if ((ro->ro_rt == (struct rtentry *)0) ||
			    (ifp = ro->ro_rt->rt_ifp) == (struct ifnet *)0) {
				/* No route yet, so try to acquire one */
				ro->ro_dst.sa_family = AF_INET;
				((struct sockaddr_in *) &ro->ro_dst)->sin_addr =
					sin->sin_addr;
				rtalloc(ro);
				if (ro->ro_rt == (struct rtentry *)0)
					ifp = if_ifwithafup(AF_INET);
				else
					ifp = ro->ro_rt->rt_ifp;
			}

			if (ifp == 0)
				return (EADDRNOTAVAIL);
		}
		ifaddr = (struct sockaddr_in *)&ifp->if_addr;
	}
	if (in_pcblookup(inp->inp_head,
	    sin->sin_addr,
	    sin->sin_port,
	    inp->inp_laddr.s_addr ? inp->inp_laddr : ifaddr->sin_addr,
	    inp->inp_lport,
	    0))
		return (EADDRINUSE);
	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		if (inp->inp_lport == 0)
			(void) in_pcbbind(inp, (struct mbuf *)0);
		inp->inp_laddr = ifaddr->sin_addr;
	}
	inp->inp_faddr = sin->sin_addr;
	inp->inp_fport = sin->sin_port;
	return (0);
}

in_pcbdisconnect(inp)
	struct inpcb *inp;
{

	inp->inp_faddr.s_addr = INADDR_ANY;
	inp->inp_fport = 0;
	if (inp->inp_socket->so_state & SS_NOFDREF)
		in_pcbdetach(inp);
}

in_pcbdetach(inp)
	struct inpcb *inp;
{
	struct socket *so = inp->inp_socket;

	so->so_pcb = 0;
	sofree(so);
	if (inp->inp_route.ro_rt)
		rtfree(inp->inp_route.ro_rt);
	remque(inp);
	(void) m_free(dtom(inp));
}

in_setsockaddr(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);
	
	nam->m_len = sizeof (*sin);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_port = inp->inp_lport;
	sin->sin_addr = inp->inp_laddr;
}

in_setpeeraddr(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);
	
	nam->m_len = sizeof (*sin);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_port = inp->inp_fport;
	sin->sin_addr = inp->inp_faddr;
}

/*
 * Pass an error to all internet connections
 * associated with address sin.  Call the
 * protocol specific routine to clean up the
 * mess afterwards.
 */
in_pcbnotify(head, dst, errno, abort)
	struct inpcb *head;
	register struct in_addr *dst;
	int errno, (*abort)();
{
	register struct inpcb *inp, *oinp;
	int s = splimp();

	for (inp = head->inp_next; inp != head;) {
		if (inp->inp_faddr.s_addr != dst->s_addr) {
	next:
			inp = inp->inp_next;
			continue;
		}
		if (inp->inp_socket == 0)
			goto next;
		inp->inp_socket->so_error = errno;
		oinp = inp;
		inp = inp->inp_next;
		(*abort)(oinp);
	}
	(void) splx(s);
}
#endif	/* NEVER */

struct inpcb *
in_pcblookup(head, faddr, fport, laddr, lport, flags)
	struct inpcb *head;
	struct in_addr faddr, laddr;
	u_short fport, lport;
	int flags;
{
	register struct inpcb *inp, *match = 0;
	int matchwild = 3, wildcard;

#ifdef	DUMP_DEBUG1
	dprint(dump_debug, 6,
		"in_pcblookup(head 0x%x faddr 0x%x fport 0x%x laddr 0x%x lport 0x%x flags 0x%x)\n",
		head, faddr, fport, laddr, lport, flags);
#endif	/* DUMP_DEBUG */

	for (inp = head->inp_next; inp != head; inp = inp->inp_next) {
		if (inp->inp_lport != lport)
			continue;
		wildcard = 0;
		if (inp->inp_laddr.s_addr != INADDR_ANY) {
			if (laddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_laddr.s_addr != laddr.s_addr)
				continue;
		} else {
			if (laddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			if (faddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_faddr.s_addr != faddr.s_addr ||
			    inp->inp_fport != fport)
				continue;
		} else {
			if (faddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (wildcard && (flags & INPLOOKUP_WILDCARD) == 0)
			continue;
		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
			if (matchwild == 0)
				break;
		}
	}
#ifdef  DUMP_DEBUG1
        dprint(dump_debug, 6,
		"in_pcblookup: match 0x%x lport 0x%x\n", match, match->inp_lport);
#endif	/* DUMP_DEBUG */
	return (match);
}
