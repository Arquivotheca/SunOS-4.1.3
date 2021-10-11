#ifdef	KERNEL
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 *
 * @(#)kudp_fastsend.c 1.1 92/07/30
 *
 * Functions to speed up Kernel UDP RPC sending
 */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_pcb.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

struct in_ifaddr *ifptoia();

/*
 * struct used to reference count a fragmented mbuf.
 */
struct mhold {
	int mh_ref;		/* holders of pointers to mbuf data */
	struct mbuf *mh_mbuf;	/* mbuf that owns the data */
};

static
fragbuf_free(mh)
	struct mhold *mh;
{
	int s;

	s = splimp();
	if (--mh->mh_ref) {
		(void) splx(s);
		return;
	}
	m_freem(dtom(mh));
	(void) splx(s);
}

static struct mhold *
fragbuf_alloc(m)
	struct mbuf *m;
{
	struct mbuf *newm;
	struct mhold *mh;

	MGET(newm, M_WAIT, MT_DATA);
	newm->m_next = m;
	newm->m_off = MMINOFF;
	newm->m_len = 0;
	m->m_next = NULL;
	mh = mtod(newm, struct mhold *);
	mh->mh_ref = 1;
	mh->mh_mbuf = m;
	return (mh);
}

extern int Sendtries;
extern int Sendok;
int route_hit, route_missed;

/*
 * Enough bytes so that the network drivers won't need
 * to allocate another mbuf near the front of the chain.
 * Note:  This must be a multiple of sizeof (int)!
 */
#define	KU_PRE_PAD	24

ku_sendto_mbuf(so, am, to)
	struct socket *so;		/* socket data is sent from */
	register struct mbuf *am;	/* data to be sent */
	struct sockaddr_in *to;		/* destination data is sent to */
{
	register int maxlen;		/* max length of fragment */
	register int curlen;		/* data fragment length */
	register int fragoff;		/* data fragment offset */
	register int grablen;		/* number of mbuf bytes to grab */
	register struct ip *ip;		/* ip header */
	register struct mbuf *m;	/* ip header mbuf */
	int error;			/* error number */
	int s;
	struct ifnet *ifp;		/* interface */
	struct sockaddr_in *dst;	/* packet destination */
	struct inpcb *inp;		/* inpcb for binding */
	struct ip *nextip;		/* ip header for next fragment */

	Sendtries++;
	/*
	 * Determine length of data.
	 * This should be passed in as a parameter.
	 */
	curlen = 0;
	for (m = am; m; m = m->m_next) {
		curlen += m->m_len;
	}
	/*
	 * Bind local port if necessary.  This will usually put a route in the
	 * pcb, so we just use that one if we have not changed destinations.
	 */
	inp = sotoinpcb(so);
	if ((inp->inp_lport == 0) &&
	    (inp->inp_laddr.s_addr == INADDR_ANY))
		(void) in_pcbbind(inp, (struct mbuf *) 0);

	{
		register struct route *ro;

		ro = &inp->inp_route;
		dst = (struct sockaddr_in *) &ro->ro_dst;

		if (ro->ro_rt == 0 || (ro->ro_rt->rt_flags & RTF_UP) == 0 ||
		    dst->sin_addr.s_addr != to->sin_addr.s_addr) {
			if (ro->ro_rt)
				rtfree(ro->ro_rt);
			bzero((caddr_t)ro, sizeof (*ro));
			ro->ro_dst.sa_family = AF_INET;
			dst->sin_addr = to->sin_addr;
			rtalloc(ro);
			route_missed++;
			if (ro->ro_rt == 0 || ro->ro_rt->rt_ifp == 0) {
				(void) m_freem(am);
				return (ENETUNREACH);
			}
		} else route_hit++;
		ifp = ro->ro_rt->rt_ifp;
		ro->ro_rt->rt_use++;
		if (ro->ro_rt->rt_flags & RTF_GATEWAY) {
			dst = (struct sockaddr_in *)&ro->ro_rt->rt_gateway;
		}
	}
	/*
	 * Get mbuf for ip, udp headers.
	 */
	MGET(m, M_WAIT, MT_HEADER);
	if (m == NULL) {
		(void) m_freem(am);
		return (ENOBUFS);
	}
	/*
	 * XXX on sparc we must pad to make sure that the packet after
	 * the ethernet header is int aligned.  Also does not hurt to
	 * long-word align on other architectures, and might actually help.
	 */
	m->m_off = MMINOFF + KU_PRE_PAD;
	m->m_len = sizeof (struct ip) + sizeof (struct udphdr);
	ip = mtod(m, struct ip *);

	/*
	 * Create pseudo-header and UDP header, and calculate
	 * the UDP level checksum only if desired.
	 */
	{
		extern int	udp_cksum;
#define	ui ((struct udpiphdr *)ip)

		ui->ui_pr = IPPROTO_UDP;
		ui->ui_len = htons((u_short) curlen + sizeof (struct udphdr));
		ui->ui_src = ((struct sockaddr_in *)ifptoia(ifp))->sin_addr;
		ui->ui_dst = to->sin_addr;
		ui->ui_sport = inp->inp_lport;
		ui->ui_dport = to->sin_port;
		ui->ui_ulen = ui->ui_len;
		ui->ui_sum = 0;
		if (udp_cksum) {
			ui->ui_next = ui->ui_prev = 0;
			ui->ui_x1 = 0;
			m->m_next = am;
			ui->ui_sum = in_cksum(m,
				sizeof (struct udpiphdr) + curlen);
			if (ui->ui_sum == 0)
				ui->ui_sum = 0xFFFF;
		}
# undef ui
	}
	/*
	 * Now that the pseudo-header has been checksummed, we can
	 * fill in the rest of the IP header except for the IP header
	 * checksum, done for each fragment.
	 */
	ip->ip_hl = sizeof (struct ip) >> 2;
	ip->ip_v = IPVERSION;
	ip->ip_tos = 0;
	ip->ip_id = ip_id++;
	ip->ip_off = 0;
	ip->ip_ttl = MAXTTL;

	/*
	 * Fragment the data into packets big enough for the
	 * interface, prepend the header, and send them off.
	 */
	maxlen = (ifp->if_mtu - sizeof (struct ip)) & ~7;
	curlen = sizeof (struct udphdr);
	fragoff = 0;
	for (;;) {
		register struct mbuf *mm;
		register struct mbuf *lam;	/* last mbuf in chain */

		/*
		 * Assertion: m points to an mbuf containing a mostly filled
		 * in ip header, while am points to a chain which contains
		 * all the data.
		 * The problem here is that there may be too much data.
		 * If there is, we have to fragment the data (and maybe the
		 * mbuf chain).
		 */
		m->m_next = am;
		lam = m;
		while (am->m_len + curlen <= maxlen) {
			curlen += am->m_len;
			lam = am;
			am = am->m_next;
			if (am == 0) {
				ip->ip_off = htons((u_short) (fragoff >> 3));
				goto send;
			}
		}
		if (curlen == maxlen) {
			/*
			 * Incredible luck: last mbuf exactly
			 * filled out the packet.
			 */
			lam->m_next = 0;
		} else {
			/*
			 * Have to fragment the mbuf chain.  am points
			 * to the mbuf that has too much, so we take part
			 * of its data, point mm to it, and attach mm to
			 * the current chain.  lam conveniently points to
			 * the last mbuf of the current chain.
			 *
			 * We will end up with many type 2 mbufs pointing
			 * into the data for am. We insure that the data
			 * doesn't get freed until all of these mbufs are
			 * freed by replacing am with a new type2 mbuf
			 * which keeps a ref count and frees am when the
			 * count goes to zero.
			 */
			MGET(mm, M_WAIT, MT_DATA);
			if ((int)am->m_clfun != (int)fragbuf_free) {
				/*
				 * clone the mbuf and save a pointer
				 * to it and a ref count.
				 */
				mm->m_off = mtod(am, int) - (int) mm;
				mm->m_len = am->m_len;
				mm->m_next = am->m_next;
				mm->m_cltype = MCL_LOANED;
				mm->m_clswp = NULL;
				mm->m_clfun = fragbuf_free;
				mm->m_clarg = (int)fragbuf_alloc(am);
				am = mm;
				MGET(mm, M_WAIT, MT_DATA);
			}
			grablen = maxlen - curlen;
			mm->m_next = NULL;
			mm->m_off = mtod(am, int) - (int) mm;
			mm->m_len = grablen;
			mm->m_cltype = MCL_LOANED;
			mm->m_clswp = NULL;
			mm->m_clfun = fragbuf_free;
			mm->m_clarg = am->m_clarg;
			s = splimp();
			((struct mhold *)mm->m_clarg)->mh_ref++;
			(void) splx(s);

			lam->m_next = mm;
			am->m_len -= grablen;
			am->m_off += grablen;
			curlen = maxlen;
		}
		/*
		 * Assertion: m points to an mbuf chain of data which
		 * can be sent, while am points to a chain containing
		 * all the data that is to be sent in later fragments.
		 */
		ip->ip_off = htons((u_short) ((fragoff >> 3) | IP_MF));
		/*
		 * There are more frags, so we save
		 * a copy of the ip hdr for the next
		 * frag.
		 */
		MGET(mm, M_WAIT, MT_HEADER);
		if (mm == 0) {
			(void) m_free(m);	/* this frag */
			(void) m_freem(am);	/* rest of data */
			return (ENOBUFS);
		}
#ifdef sparc
		/*
		 * XXX on sparc we must pad to make sure that the packet after
		 * the ethernet header is int aligned.
		 */
		mm->m_off =
		    MMINOFF + sizeof (short) + sizeof (struct ether_header);
#else
		mm->m_off = MMINOFF + sizeof (struct ether_header);
#endif
		mm->m_len = sizeof (struct ip);
		nextip = mtod(mm, struct ip *);
		bcopy((caddr_t) ip, (caddr_t) nextip, sizeof (struct ip));
send:
		{
			register int sum;
			/*
			 * Set ip_len and calculate the ip header checksum.
			 * Note ips[5] is skipped below since it IS where the
			 * checksum will eventually go.
			 */
			ip->ip_len = (short)
			    htons((u_short) (sizeof (struct ip) + curlen));
#define	ips ((u_short *) ip)
			sum = ips[0] + ips[1] + ips[2] + ips[3] + ips[4]
			    + ips[6] + ips[7] + ips[8] + ips[9];
			sum = (sum & 0xFFFF) + (sum >> 16);
			ip->ip_sum = ~((sum & 0xFFFF) + (sum >> 16));
		}
#undef ips
		/*
		 * Send it off to the newtork.
		 */
		if (error = (*ifp->if_output)(ifp, m, dst)) {
			if (am) {
				(void) m_freem(am);	/* rest of data */
				(void) m_free(mm);	/* hdr for next frag */
			}
			return (error);
		}
		if (am == 0) {			/* All fragments sent OK */
			Sendok++;
			return (0);
		}
		ip = nextip;
		m = mm;
		fragoff += curlen;
		curlen = 0;
	}
	/*NOTREACHED*/
}

#ifdef DEBUG
pr_mbuf(p, m)
	char *p;
	struct mbuf *m;
{
	register char *cp, *cp2;
	register struct ip *ip;
	register int len;

	len = 28;
	printf("%s: ", p);
	if (m && m->m_len >= 20) {
		ip = mtod(m, struct ip *);
		printf("hl %d v %d tos %d len %d id %d mf %d off ",
			ip->ip_hl, ip->ip_v, ip->ip_tos, ip->ip_len,
			ip->ip_id, ip->ip_off >> 13, ip->ip_off & 0x1fff);
		printf("%d ttl %d p %d sum %d src %x dst %x\n",
			ip->ip_ttl, ip->ip_p, ip->ip_sum, ip->ip_src.s_addr,
			ip->ip_dst.s_addr);
		len = 0;
		printf("m %x addr %x len %d\n", m, mtod(m, caddr_t), m->m_len);
		m = m->m_next;
	} else if (m) {
		printf("pr_mbuf: m_len %d\n", m->m_len);
	} else {
		printf("pr_mbuf: zero m\n");
	}
	while (m) {
		printf("m %x addr %x len %d\n", m, mtod(m, caddr_t), m->m_len);
		cp = mtod(m, caddr_t);
		cp2 = cp + m->m_len;
		while (cp < cp2) {
			if (len-- < 0) {
				break;
			}
			printf("%x ", *cp & 0xFF);
			cp++;
		}
		m = m->m_next;
		printf("\n");
	}
}
#endif DEBUG
#endif	/* KERNEL */
