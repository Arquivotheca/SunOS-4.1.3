#ifndef lint
static        char sccsid[] = "@(#)kudp_fastsend.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif


/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include "boot/systm.h"
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
#include <netinet/udp.h>

#include <mon/sunromvec.h>
#include <stand/saio.h>
#include <stand/sainet.h>
#include "boot/inet.h"

static int dump_debug = 10;

char	temp_buff[1600];
struct	ether_addr	nulletheraddr = { 0, 0, 0, 0, 0, 0};
struct	ether_addr	destetheraddr = { 0, 0, 0, 0, 0, 0};

static
buffree()
{
}

ku_fastsend(so, am, to)
	struct socket *so;		/* socket data is sent from */
	register struct mbuf *am;	/* data to be sent */
	struct sockaddr_in *to;		/* destination data is sent to */
{
	register int datalen;		/* length of all data in packet */
	register int maxlen;		/* max length of fragment */
	register int curlen;		/* data fragment length */
	register int fragoff;		/* data fragment offset */
	register int sum;		/* ip header checksum */
	register int grablen;		/* number of mbuf bytes to grab */
	register struct ip *ip;		/* ip header */
	register struct mbuf *m;	/* ip header mbuf */
	int error;			/* error number */
	struct ifnet *ifp;		/* interface */
	struct mbuf *lam;		/* last mbuf in chain to be sent */
	struct sockaddr	*dst;		/* packet destination */
	struct inpcb *inp;		/* inpcb for binding */
	struct ip *nextip;		/* ip header for next fragment */
	static struct route route;	/* route to send packet */
	static struct route zero_route;	/* to initialize route */

#ifdef	DUMP_DEBUG
	dprint(dump_debug, 5,
		"ku_fastsend(so 0x%x, am 0x%x, to 0x%x)\n",
		so, am, to);
#endif	/* DUMP_DEBUG */

	/*
	 * Determine length of data.
	 * This should be passed in as a parameter.
	 */
	datalen = 0;
	for (m = am; m; m = m->m_next) {
		datalen += m->m_len;
	}
	/*
	 * Routing.
	 * We worry about routing early so we get the right ifp.
	 */
	{
		register struct route *ro;

		ro = &route;

		if (ro->ro_rt == 0 || (ro->ro_rt->rt_flags & RTF_UP) == 0 ||
		    ((struct sockaddr_in *)&ro->ro_dst)->sin_addr.s_addr !=
		    to->sin_addr.s_addr) {
			if (ro->ro_rt)
				rtfree(ro->ro_rt);
			route = zero_route;
			ro->ro_dst.sa_family = AF_INET;
			((struct sockaddr_in *)&ro->ro_dst)->sin_addr =
			    to->sin_addr;
			rtalloc(ro);
			if (ro->ro_rt == 0 || ro->ro_rt->rt_ifp == 0) {
				dprint(dump_debug, 10,
					"ku_fastsend: ro_rt 0x%x ro_rt->rt_ifp 0x%x ENETUNREACH\n",
					ro->ro_rt, ro->ro_rt->rt_ifp);
				(void) m_free(am);
				return (ENETUNREACH);
			}
		}
		ifp = ro->ro_rt->rt_ifp;
		ro->ro_rt->rt_use++;
		if (ro->ro_rt->rt_flags & RTF_GATEWAY) {
			dst = &ro->ro_rt->rt_gateway;
		} else {
			dst = &ro->ro_dst;
		}
#ifdef  DUMP_DEBUG
	dprint(dump_debug, 5,
		"ku_fastsend: dst 0x%x\n", dst);
#endif  /* DUMP_DEBUG */
	}
	/*
	 * Get mbuf for ip, udp headers.
	 */
	MGET(m, M_WAIT, MT_HEADER);
	if (m == NULL) {
		(void) m_free(am);
		return (ENOBUFS);
	}
#ifdef sparc
	/*
	 * XXX on sparc we must pad to make sure that the packet after
	 * the ethernet header is int aligned.
	 */
	m->m_off = MMINOFF + sizeof (short) + sizeof (struct ether_header);
#else
	m->m_off = MMINOFF + sizeof (struct ether_header);
#endif
	m->m_len = sizeof (struct ip) + sizeof (struct udphdr);
	/*
	 * Create IP header.
	 */
	ip = mtod(m, struct ip *);
	ip->ip_hl = sizeof (struct ip) >> 2;
	ip->ip_v = IPVERSION;
	ip->ip_tos = 0;
	ip->ip_id = ip_id++;
	ip->ip_off = 0;
	ip->ip_ttl = MAXTTL;
	ip->ip_p = IPPROTO_UDP;
	ip->ip_sum = 0;			/* is this necessary? */
	ip->ip_src = ((struct sockaddr_in *)&(ifp->if_addrlist->ifa_addr))->sin_addr;
	ip->ip_dst = to->sin_addr;


	/*
	 * Bind port, if necessary.
	 */
	inp = sotoinpcb(so);
	if (inp->inp_laddr.s_addr == INADDR_ANY && inp->inp_lport==0) {
		(void) in_pcbbind(inp, (struct mbuf *)0);
	}
	/*
	 * Create UDP header.
	 */
	{
		register struct udphdr *udp;

		udp = (struct udphdr *)(ip + 1);
		udp->uh_sport = inp->inp_lport;
		udp->uh_dport = to->sin_port;
		udp->uh_ulen = htons(sizeof (struct udphdr) + datalen);
		udp->uh_sum = 0;
	}
	/*
	 * Fragment the data into packets big enough for the
	 * interface, prepend the header, and send them off.
	 */
	maxlen = (ifp->if_mtu - sizeof (struct ip)) & ~7;
	curlen = sizeof (struct udphdr);
	fragoff = 0;
	for (;;) {
		register struct mbuf *mm;

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
			 */
			MGET(mm, M_WAIT, MT_DATA);
			if (mm == NULL) {
				(void) m_free(m);	/* includes am */
				return (ENOBUFS);
			}
			grablen = maxlen - curlen;
			mm->m_off = mtod(am, int) - (int) mm;
			mm->m_len = grablen;
			mm->m_cltype = 2;
			mm->m_clfun = buffree;
			mm->m_clswp = NULL;
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
			(void) m_free(am);	/* rest of data */
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
		*nextip = *ip;
send:
		/*
		 * Set ip_len and calculate the ip header checksum.
		 */
		ip->ip_len = htons(sizeof (struct ip) + curlen);
#define	ips ((u_short *) ip)
		sum = ips[0] + ips[1] + ips[2] + ips[3] + ips[4] + ips[6] +
			ips[7] + ips[8] + ips[9];
		ip->ip_sum = ~(sum + (sum >> 16));
#undef ips
		/*
		 * Send it off to the network.
		 */
		/*
		 * In this standalone system, the if_output() code
		 * does not know about mbufs, and also demands a
		 * pointer to the saioreq struct, which we have
		 * secreted in ifp->lower.    We must therefore
		 * copy it to a contiguous safe location.
		 */
		{
		int len;
		struct ether_header *eh;
		extern struct ether_addr etherbroadcastaddr;
		register struct saioreq *tmp_sip;

		    if((len = m_cpytoc(m, 0, curlen + sizeof(struct ip),
			temp_buff + sizeof(struct ether_header))) != 0)    {
			    dprint(dump_debug, 10,
				"ku_fastsend: bytes not copied 0x%x\n", len);
		    }
		    eh = (struct ether_header *)temp_buff;

		    /* get MAC address */
		    tmp_sip = (struct saioreq *)ifp->if_lower;
#ifdef OPENPROMS
		    (*tmp_sip->si_sif->sif_macaddr)(tmp_sip->si_devdata, 
							&(eh->ether_shost));
#else
		    (*tmp_sip->si_sif->sif_macaddr)(&(eh->ether_shost));
#endif

		    if(bcmp(&destetheraddr, &nulletheraddr, 6) == 0)	{
		            bcopy(&etherbroadcastaddr, &destetheraddr, 6);
		    }
		    bcopy(&destetheraddr, &(eh->ether_dhost), 6);
		    eh->ether_type = ETHERTYPE_IP;
		}

		if (error = (*ifp->if_output)(((struct saioreq *)(ifp->if_lower))->si_devdata,
			temp_buff, curlen + sizeof(struct ip) + sizeof(struct ether_header)))	{
			if (am) {
				(void) m_free(am);	/* rest of data */
				(void) m_free(mm);	/* hdr for next frag */
			}
			return (error);
		}
		/*
		 * Now get rid of the mbufs, since the lower level
		 * will no longer do it in this stand-alone version.
		 */
		(void) m_freem(m);
		if (am == 0) {
			return (0);
		}
		ip = nextip;
		m = mm;
		fragoff += curlen;
		curlen = 0;
	}
	/*NOTREACHED*/
}

#ifdef notdef
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
		printf("m %x addr %x len %d\n", m, mtod(m, caddr_t), m->m_len);
		ip = mtod(m, struct ip *);
		printf("hl %d v %d tos %d len %d id %d mf %d off %d ttl %d p %d sum %d src %s dst %s\n",
			ip->ip_hl, ip->ip_v, ip->ip_tos, ip->ip_len,
			ip->ip_id, ip->ip_off >> 13, ip->ip_off & 0x1fff,
			ip->ip_ttl, ip->ip_p, ip->ip_sum,
			inet_ntoa(ip->ip_src.s_addr), inet_ntoa(ip->ip_dst.s_addr));
		len = 0;
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
#endif notdef
