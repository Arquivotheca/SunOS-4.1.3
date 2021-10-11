#ifndef lint
static        char sccsid[] = "@(#)udp_usrreq.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif


#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/mbuf.h>
#include "boot/protosw.h"
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

static int dump_debug = 10;

/*
 * UDP protocol implementation.
 * Per RFC 768, August, 1980.
 */
udp_init()
{

	udb.inp_next = udb.inp_prev = &udb;
}

int	udpcksum = 0;
struct	sockaddr_in udp_in = { AF_INET };

udp_input(m0)
	struct mbuf *m0;
{
	register struct udpiphdr *ui;
	register struct inpcb *inp;
	register struct mbuf *m;
	int len;


	/*
	 * Get IP and UDP header together in first mbuf.
	 */
	m = m0;
	if ((m->m_off > MMAXOFF || m->m_len < sizeof (struct udpiphdr)) &&
	    (m = m_pullup(m, sizeof (struct udpiphdr))) == 0) {
		udpstat.udps_hdrops++;
		return;
	}
	ui = mtod(m, struct udpiphdr *);

#ifdef	NEVER
	if (((struct ip *)ui)->ip_hl > (sizeof (struct ip) >> 2))
		ip_stripoptions((struct ip *)ui, (struct mbuf *)0);
#endif	/* NEVER */

	/*
	 * Make mbuf data length reflect UDP length.
	 * If not enough data to reflect UDP length, drop.
	 */
	len = ntohs((u_short)ui->ui_ulen);
	if (((struct ip *)ui)->ip_len != len) {
		if (len > ((struct ip *)ui)->ip_len) {
			udpstat.udps_badlen++;
			goto bad;
		}
		m_adj(m, len - ((struct ip *)ui)->ip_len);
		/* (struct ip *)ui->ip_len = len; */
	}

	/*
	 * Checksum extended UDP header and data.
	 */
	if (udpcksum && ui->ui_sum) {
		ui->ui_next = ui->ui_prev = 0;
		ui->ui_x1 = 0;
		ui->ui_len = htons((u_short)len);
		if (ui->ui_sum = ipcksum((char *)&(ui->ui_sport),
		    (unsigned short)(len + sizeof (struct ip)))) {
			dprint(dump_debug, 6,
				"udp_input: bad checksum 0x%x\n",
				ui->ui_sum);
			udpstat.udps_badsum++;
			m_freem(m);
			return;
		}
	}

	/*
	 * Locate pcb for datagram.
	 */
	inp = in_pcblookup(&udb,
	    ui->ui_src, ui->ui_sport, ui->ui_dst, ui->ui_dport,
		INPLOOKUP_WILDCARD);
	if (inp == 0) {
		/* don't send ICMP response for broadcast packet */
		if (in_lnaof(ui->ui_dst) == INADDR_ANY)
			goto bad;
#ifdef	NEVER
		icmp_error((struct ip *)ui, ICMP_UNREACH, ICMP_UNREACH_PORT);
#endif	/* NEVER */
		return;
	}

	/*
	 * Construct sockaddr format source address.
	 * Stuff source address and datagram in user buffer.
	 */
	udp_in.sin_port = ui->ui_sport;
	udp_in.sin_addr = ui->ui_src;
	m->m_len -= sizeof (struct udpiphdr);
	m->m_off += sizeof (struct udpiphdr);

	if (sbappendaddr(&inp->inp_socket->so_rcv, (struct sockaddr *)&udp_in,
	    m, (struct mbuf *)0,
	    inp->inp_socket->so_proto->pr_flags & PR_RIGHTS) == 0) {
		udpstat.udps_fullsock++;
		goto bad;
	}
#ifdef	NEVER
	sorwakeup(inp->inp_socket);
#endif	/* NEVER */
	return;
bad:
	dprint(dump_debug, 6,
		"udp_input: bad\n");
	m_freem(m);
}

udp_abort(inp)
	struct inpcb *inp;
{
#ifdef	NEVER
	struct socket *so = inp->inp_socket;

	in_pcbdisconnect(inp);
	soisdisconnected(so);
#endif	/* NEVER */
}

udp_ctlinput(cmd, arg)
	int cmd;
	caddr_t arg;
{
#ifdef	NEVER
	struct in_addr *sin;
	extern u_char inetctlerrmap[];

	if (cmd < 0 || cmd > PRC_NCMDS)
		return;
	switch (cmd) {

	case PRC_ROUTEDEAD:
		break;

	case PRC_QUENCH:
		break;

	/* these are handled by ip */
	case PRC_IFDOWN:
	case PRC_HOSTDEAD:
	case PRC_HOSTUNREACH:
		break;

	default:
		sin = &((struct icmp *)arg)->icmp_ip.ip_dst;
		in_pcbnotify(&udb, sin, (int)inetctlerrmap[cmd], udp_abort);
	}
#endif	/* NEVER */
}

int udp_fastloop = 1;	/* udp fast loopback enabled */

udp_output(inp, m0)
	struct inpcb *inp;
	struct mbuf *m0;
{
#ifdef	NEVER
	register struct mbuf *m;
	register struct udpiphdr *ui;
	register struct socket *so;
	register int len = 0;
	int flags;

	/*
	 * Calculate data length and get a mbuf
	 * for UDP and IP headers.
	 */
	for (m = m0; m; m = m->m_next)
		len += m->m_len;
	m = m_get(M_DONTWAIT, MT_HEADER);
	if (m == 0) {
		m_freem(m0);
		return (ENOBUFS);
	}

	/*
	 * Fill in mbuf with extended UDP header
	 * and addresses and length put into network format.
	 */
	m->m_off = MMAXOFF - sizeof (struct udpiphdr);
	m->m_len = sizeof (struct udpiphdr);
	m->m_next = m0;
	ui = mtod(m, struct udpiphdr *);
	ui->ui_next = ui->ui_prev = 0;
	ui->ui_x1 = 0;
	ui->ui_pr = IPPROTO_UDP;
	ui->ui_len = htons((u_short)len + sizeof (struct udphdr));
	ui->ui_src = inp->inp_laddr;
	ui->ui_dst = inp->inp_faddr;
	ui->ui_sport = inp->inp_lport;
	ui->ui_dport = inp->inp_fport;
	ui->ui_ulen = (u_short)ui->ui_len;
	((struct ip *)ui)->ip_hl = sizeof (struct ip) >> 2;

	/*
	 * Stuff checksum and output datagram.
	 */
	ui->ui_sum = 0;
	if (udpcksum && 
	    (ui->ui_sum = in_cksum(m, sizeof (struct udpiphdr) + len)) == 0)
		ui->ui_sum = -1;
	((struct ip *)ui)->ip_len = sizeof (struct udpiphdr) + len;
	((struct ip *)ui)->ip_ttl = MAXTTL;
	so = inp->inp_socket;
	flags = (so->so_options & SO_DONTROUTE) | (so->so_state & SS_PRIV);
	if (udp_fastloop) {
		/*
		 * Check for loopback by checking whether we
		 * have an interface to the destination.
		 */
		if (ifnet && ifnet->if_addr.sa_family == AF_INET &&
		    ((struct sockaddr_in *) &ifnet->if_addr)->sin_addr.s_addr
		    == ui->ui_dst.s_addr) {
			if (ui->ui_src.s_addr == 0) {
				ui->ui_src.s_addr = ui->ui_dst.s_addr;
			}
			((struct ip *)ui)->ip_len -= sizeof (struct ipovly);
			udp_input(m);
			return (0);
		}
	}
	return (ip_output(m, (struct mbuf *)0, (struct route *)0, flags));
#endif	/* NEVER */
}

/*ARGSUSED*/
udp_usrreq(so, req, m, nam, rights)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *rights;
{
	struct inpcb *inp = sotoinpcb(so);
	int error = 0;
	/*
	 * XXX Protect udp_usrreq just like tcp.
	 * Avoids race condition with in_pcballoc;
	 * also keeps icmp error from aborting a udp
	 * socket that is not connected (sendto used).
	 */
	int s;

	s = splnet();
	if (rights && rights->m_len) {
		error = EINVAL;
		goto release;
	}
	if (inp == NULL && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}
	switch (req) {

	case PRU_ATTACH:
		if (inp != NULL) {
			error = EINVAL;
			break;
		}
		error = in_pcballoc(so, &udb);
		if (error)
			break;
		/* udp delivers data + sockaddr, must make space */
		error = soreserve(so, 9000, 9000 + sizeof(struct sockaddr));
		if (error)
			break;
		break;

#ifdef	NEVER
	case PRU_DETACH:
		if (inp == NULL) {
			error = ENOTCONN;
			break;
		}
		in_pcbdetach(inp);
		break;
#endif	/* NEVER */

	case PRU_BIND:
		error = in_pcbbind(inp, nam);
		break;

#ifdef	NEVER
	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			error = EISCONN;
			break;
		}
		error = in_pcbconnect(inp, nam);
		if (error == 0)
			soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	case PRU_ACCEPT:
		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		if (inp->inp_faddr.s_addr == INADDR_ANY) {
			error = ENOTCONN;
			break;
		}
		in_pcbdisconnect(inp);
		soisdisconnected(so);
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_SEND:
	    {
		struct in_addr laddr;

		if (nam) {
			laddr = inp->inp_laddr;
			if (inp->inp_faddr.s_addr != INADDR_ANY) {
				error = EISCONN;
				break;
			}
			error = in_pcbconnect(inp, nam);
			if (error)
				break;
		} else {
			if (inp->inp_faddr.s_addr == INADDR_ANY) {
				error = ENOTCONN;
				break;
			}
		}
		error = udp_output(inp, m);
		m = NULL;
		if (nam) {
			in_pcbdisconnect(inp);
			inp->inp_laddr = laddr;
		}
	    }
		break;

	case PRU_ABORT:
		in_pcbdetach(inp);
		sofree(so);
		soisdisconnected(so);
		break;

	case PRU_SOCKADDR:
		in_setsockaddr(inp, nam);
		break;

	case PRU_PEERADDR:
		in_setpeeraddr(inp, nam);
		break;

	case PRU_CONTROL:
		m = NULL;
		error = EOPNOTSUPP;
		break;

	case PRU_SENSE:
		m = NULL;
		/* fall thru... */

#endif	/* NEVER */
	case PRU_RCVD:
	case PRU_RCVOOB:
	case PRU_SENDOOB:
	case PRU_FASTTIMO:
	case PRU_SLOWTIMO:
	case PRU_PROTORCV:
	case PRU_PROTOSEND:
		error =  EOPNOTSUPP;
		break;

	default:
		panic("udp_usrreq");
	}
release:
	(void)splx(s);
	if (m != NULL)
		m_freem(m);
	return (error);
}
