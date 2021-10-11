#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)authdes_subr.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Miscellaneous support routines for kernel implentation of AUTH_DES
 * Copyright (C) 1986 by Sun Microsystems, Inc.
 */

/*
 *  rtime - get time from remote machine
 *
 *  sets time, obtaining value from host
 *  on the udp/time socket.  Since timeserver returns
 *  with time of day in seconds since Jan 1, 1900,  must
 *  subtract 86400(365*70 + 17) to get time
 *  since Jan 1, 1970, which is what get/settimeofday
 *  uses.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mbuf.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define	debug(msg)		/* turn off debugging */

#define	USEC_PER_SEC 1000000
#define	TOFFSET (86400*(365*70 + (70/4)))
#define	WRITTEN (86400*(365*86 + (86/4)))

rtime_wakeup(so)
	struct socket *so;
{
	so->so_error = ETIMEDOUT;
	sbwakeup(so, &so->so_rcv);
}


rtime(addrp, timep, wait)
	struct sockaddr_in *addrp;
	struct timeval *timep;
	struct timeval *wait;
{
	struct socket *so;
	struct mbuf *m;
	int error;
	u_long thetime;
	struct sockaddr_in from;
	int s;
	struct mbuf *ku_recvfrom();

	error = socreate(AF_INET, &so, SOCK_DGRAM, IPPROTO_UDP);
	if (error) {
		return (-1);
	}
	addrp->sin_family = AF_INET;
	addrp->sin_port = IPPORT_TIMESERVER;

	m = m_get(M_WAIT, MT_DATA);
	if (m == NULL) {
		(void) soclose(so);
		return (-1);
	}
	m->m_len = 0;

	error = ku_sendto_mbuf(so, m, addrp);
	/* m is now free */
	if (error) {
		debug("ku_sendto_mbuf");
		(void) soclose(so);
		return (-1);
	}

	so->so_error = 0;
	s = splnet();
	timeout(rtime_wakeup, (caddr_t)so,
		(int)(wait->tv_sec * hz + (wait->tv_usec * hz) / USEC_PER_SEC));

	so->so_rcv.sb_flags |= SB_WAIT;
	(void) sleep((caddr_t)&so->so_rcv.sb_cc, PRIBIO);

	untimeout(rtime_wakeup, (caddr_t)so);

	if (so->so_error) {
		(void) splx(s);
		debug("rtime timed out");
		(void) soclose(so);
		return (-1);
	}
	m = ku_recvfrom(so, &from);
	(void) splx(s);

	(void) soclose(so);
	if (m == NULL) {
		debug("recvfrom");
		return (-1);
	}
	if (m->m_len != sizeof (u_long)) {
		debug("invalid receipt time");
		m_freem(m);
		return (-1);
	}
	thetime = ntohl(*mtod(m, u_long *));
	m_freem(m);
	if (thetime < WRITTEN) {
		debug("time returned is too far in past");
		return (-1);
	}
	timep->tv_sec = thetime - TOFFSET;
	timep->tv_usec = 0;
	return (0);
}


/*
 * Short to ascii conversion
 */
static char *
sitoa(s, i)
	char *s;
	short i;
{
	char *p;
	char *end;
	char c;

	if (i < 0) {
		*s++ = '-';
		i = -i;
	} else if (i == 0) {
		*s++ = '0';
	}

	/*
	 * format in reverse order
	 */
	for (p = s; i > 0; i /= 10) {
		*p++ = (i % 10) + '0';
	}
	*(end = p) = 0;

	/*
	 * reverse
	 */
	while (p > s) {
		c = *--p;
		*p = *s;
		*s++ = c;
	}
	return (end);
}

static char *
atoa(dst, src)
	char *dst;
	char *src;
{
	while (*dst++ = *src++)
		;
	return (dst-1);
}

/*
 * What is my network name?
 * WARNING: this gets the network name in sun unix format.
 * Other operating systems (non-unix) are free to put something else
 * here.
 */
getnetname(netname)
	char *netname;
{
	char *p;
	extern char hostname[];
	extern char domainname[];

	p = atoa(netname, "unix.");
	if (u.u_uid == 0) {
		p = atoa(p, hostname);
	} else {
		p = sitoa(p, (short)u.u_uid);
	}
	*p++ = '@';
	p = atoa(p, domainname);
}
