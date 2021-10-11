#ifndef lint
static char sccsid[] = "@(#)udp_boot.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include "boot/systm.h"
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <stand/saio.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <sys/dir.h>
#include "boot/vnode.h"
#include <ufs/fs.h>
#include "boot/inode.h"
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <stand/sainet.h>
#include <mon/sunromvec.h>
#include <mon/idprom.h>
#include "boot/iob.h"
#include "boot/inet.h"

extern struct no_nd no_nd;

static int dump_debug = 20;

struct {
	int	tries;		/* times entered. */
	int	packets;	/* packets received. */
	int	ip_frags;	/* IP fragments */
	int	ip_packets;	/* IP packets received */
	int	attempts;	/* loops to get packet */
	int	trytime;	/* time in this routine */
	int	successtime;	/* time to get a packet */
	int	looptime;	/* time around loop once */
	int	successloop;	/* time in loop to get packet */
	int	timedout;	/* returned with no ip_packet */
	int	handletime;	/* time to handle packet */
} njlstat;

char	*buff = (char *)0;
int	udp_free();
extern	int	ip_gotit;

#ifdef OPENPROMS
#define	millitime()	prom_gettime()
#else
#define	millitime()	(*romp->v_nmiclock)
#endif !OPENPROMS

/*
 * Continuously poll ether input, looking for udp packets, and
 * when a packet is found, call ipintr to deal with it further.
 *
 * TODO: deal with non-udp packets.
 */
get_udp()
{
	struct ifnet *ifp, *if_ifwithafup();
	struct saioreq *sip;
	struct sainet *sain;
	struct ether_header *eh;
	struct ip *ip;
	struct mbuf *m;
	int	i;
	int	entrytime;
	int	packettime;
	int	currenttime;
	int	intime,	outtime;

	ip_slowtimo();
	njlstat.tries++;
	entrytime = millitime();

	/*
	 * Assumption: there is only one ifnet hanging off 'ifnet'.
	 * If we ever open more than one interface, we need to revisit
	 * this.
	 */
	if ((ifp = if_ifwithafup(AF_INET)) == (struct ifnet *)0) {
	}
	sip = (struct saioreq *)(ifp->if_lower);
	sain = &no_nd.no_nd_inet;
	/*
	 * Get a buffer for the input.
	 */

	if ((buff = kmem_alloc(1600)) == (char *)0)	{
		dprint(dump_debug, 0,
			"udp_boot: unable to get buffer\n");
		return (-1);
	}

	for (i = 0; i < 1000; i++)	{
		packettime = millitime();
#ifdef sparc
		if (ip_input(sip, buff+2, sain) != 0)	{
			eh = (struct ether_header *)(buff+2);
#else
		if (ip_input(sip, buff, sain) != 0)	{
			eh = (struct ether_header *)buff;
#endif
			njlstat.packets++;
			ip = (struct ip *)(eh + 1);
			if (ip->ip_p != IPPROTO_UDP)	{
				continue;
			}
			njlstat.ip_frags++;
			/*
			 * Get the packet into an mbuf.   Just create a funny
			 * mbuf pointing to the buffer.
			 */
			if ((m = (struct mbuf *)mclgetx(udp_free, (int)buff,
			    buff, 1600, 0)) == (struct mbuf *)0) {
				continue;
			}
			/*
			 * Get rid of the ether header.
			 */
#ifdef sparc
			m_adj(m, sizeof (struct ether_header) + 2);
#else
			m_adj(m, sizeof (struct ether_header));
#endif
			currenttime = millitime();
			intime = currenttime;
			if (njlstat.successtime < (currenttime - entrytime))
				njlstat.successtime = currenttime - entrytime;
			if (njlstat.attempts < i)
				njlstat.attempts = i;
			if (njlstat.successloop < (currenttime - packettime))
				njlstat.successloop = currenttime - packettime;
			ipintr(m);
			if (ip_gotit == 1) {
				njlstat.ip_packets++;
				return (1);
			}
			if ((buff = kmem_alloc(1600)) == (char *)0)  {
				dprint(dump_debug, 0,
				    "udp_boot: unable to get buffer\n");
				return (-1);
			}
		}
		currenttime = millitime();
		outtime = currenttime;
		if (njlstat.looptime < (currenttime - packettime))
			njlstat.looptime = currenttime - packettime;
		if (njlstat.handletime < (outtime - intime))
			njlstat.handletime = outtime - intime;
	}

	kmem_free(buff, 1600);
	currenttime = millitime();
	if (njlstat.trytime < (currenttime - entrytime))
		njlstat.trytime = currenttime - entrytime;
	njlstat.timedout++;
	return (0);

}

udp_free (arg)
	char *arg;
{
	kmem_free(arg, 1600);
}
