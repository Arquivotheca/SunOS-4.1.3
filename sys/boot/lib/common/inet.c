#ifndef lint
static        char sccsid[] = "@(#)inet.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Standalone IP send and receive - specific to Ethernet
 * Includes ARP and Reverse ARP
 */
#include <stand/saio.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <stand/sainet.h>
#include <mon/sunromvec.h>
#include <mon/idprom.h>

#ifdef OPENPROMS
#define millitime() (*romp->op_milliseconds)
#else
#define millitime() (*romp->v_nmiclock)
#endif !OPENPROMS

struct ether_addr etherbroadcastaddr = { 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

struct	in_addr	my_in_addr;
struct	in_addr	null_in_addr;

struct arp_packet {
	struct ether_header	arp_eh;
	struct ether_arp	arp_ea;
#define	used_size (sizeof (struct ether_header)+sizeof(struct ether_arp))
	char	filler[ETHERMIN - sizeof(struct ether_arp)];
};

#define WAITCNT	2	/* 4 seconds before bitching about arp/revarp */

/*
 * Fetch our Ethernet address from the ID prom
 */
myetheraddr(ea)
	struct ether_addr *ea;
{
	struct idprom id;

	if (idprom(IDFORM_1, &id) != IDFORM_1) {
		printf("ERROR: missing or invalid ID prom\n");
		return;
	}
	*ea = *(struct ether_addr *)id.id_ether;
}

/*
 * Initialize IP state
 * Find out our Ethernet address and call Reverse ARP
 * to find out our Internet address
 * Set the ARP cache to the broadcast host
 */
inet_init(sip, sain, tmpbuf)
	register struct saioreq *sip;
	register struct sainet *sain;
	char *tmpbuf;
{
	/* get MAC address */
	(*sip->si_sif->sif_macaddr)(&sain->sain_myether);	
	bzero((caddr_t)&sain->sain_myaddr, sizeof(struct in_addr));
	bzero((caddr_t)&sain->sain_hisaddr, sizeof(struct in_addr));
	sain->sain_hisether = etherbroadcastaddr;
	revarp(sip, sain, tmpbuf);
}


/*
 * Output an IP packet
 * Cause ARP to be invoked if necessary
 */
ip_output(sip, buf, len, sain, tmpbuf)
	register struct saioreq *sip;
	caddr_t buf, tmpbuf;
	short len;
	register struct sainet *sain;
{
	register struct ether_header *eh;
	register struct ip *ip;
#ifdef	NFS_BOOT
	int	i,	error;
#endif	 /* NFS_BOOT */

	eh = (struct ether_header *)buf;
	ip = (struct ip *)(buf + sizeof(struct ether_header));
	if (bcmp((caddr_t)&ip->ip_dst,
		(caddr_t)&sain->sain_hisaddr,
		sizeof(struct in_addr)) != 0) {
			bcopy((caddr_t)&ip->ip_dst,
				(caddr_t)&sain->sain_hisaddr,
				sizeof (struct in_addr));
			arp(sip, sain, tmpbuf);
	}
	eh->ether_type = ETHERTYPE_IP;
	eh->ether_shost = sain->sain_myether;
	eh->ether_dhost = sain->sain_hisether;
	/* checksum the packet */
	ip->ip_sum = 0;
	ip->ip_sum = ipcksum((caddr_t)ip, sizeof (struct ip));
	if (len < ETHERMIN + sizeof(struct ether_header)) {
		len = ETHERMIN+sizeof(struct ether_header);
	}
#ifdef	 NFS_BOOT
	/*
	 * Since the ie chip hangs quite often on a Sun-2,
	 * we give it several chances.
	 */
	for(i = 0; i < 10; i++)		{
	        error = (*sip->si_sif->sif_xmit)(sip->si_devdata, buf, len);
		if (error == 0) {
			return(0);
		}
	}
	return(error);
#else
	return (*sip->si_sif->sif_xmit)(sip->si_devdata, buf, len);
#endif	 /* NFS_BOOT */
}

/*
 * Check incoming packets for IP packets
 * addressed to us. Also, respond to ARP packets
 * that wish to know about us.
 * Returns a length for any IP packet addressed to us, 0 otherwise.
 */
ip_input(sip, buf, sain)
	register struct saioreq *sip;
	caddr_t buf;
	register struct sainet *sain;
{
	register short len;
	register struct ether_header *eh;
	register struct ip *ip;
	register struct ether_arp *ea;


	len = (*sip->si_sif->sif_poll)(sip->si_devdata, buf);
	eh = (struct ether_header *)buf;
	if (eh->ether_type == ETHERTYPE_IP &&
	    len >= sizeof(struct ether_header)+sizeof(struct ip)) {
		ip = (struct ip *)(buf + sizeof(struct ether_header));
#ifdef NOREVARP
		if ((sain->sain_hisaddr.s_addr & 0xFF000000) == 0 &&
		    bcmp((caddr_t)&etherbroadcastaddr,
			(caddr_t)&eh->ether_dhost,
			sizeof(struct ether_addr)) != 0 &&
		    (in_broadaddr(sain->sain_hisaddr) ||
		    in_lnaof(ip->ip_src) == in_lnaof(sain->sain_hisaddr))) {
			sain->sain_myaddr = ip->ip_dst;
			sain->sain_hisaddr = ip->ip_src;
			sain->sain_hisether = eh->ether_shost;
		}
#endif
		if (bcmp((caddr_t)&ip->ip_dst,
		    (caddr_t)&sain->sain_myaddr,
		    sizeof(struct in_addr)) != 0)
			return (0);
		return (len);
	}
	if (eh->ether_type == ETHERTYPE_ARP &&
	    len >= sizeof(struct ether_header) + sizeof(struct ether_arp)) {
		ea = (struct ether_arp *)(buf + sizeof(struct ether_header));
		if (ea->arp_pro != ETHERTYPE_IP)
			return (0);
		if (bcmp((caddr_t)ea->arp_spa,
			(caddr_t)&sain->sain_hisaddr,
			sizeof(struct in_addr)) == 0)
			sain->sain_hisether = ea->arp_sha;
		if (ea->arp_op == ARPOP_REQUEST &&
		    (bcmp((caddr_t)ea->arp_tpa,
		      (caddr_t)&sain->sain_myaddr,
		      sizeof(struct in_addr)) == 0)) {
			ea->arp_op = ARPOP_REPLY;
			eh->ether_dhost = ea->arp_sha;
			eh->ether_shost = sain->sain_myether;
			ea->arp_tha = ea->arp_sha;
			bcopy((caddr_t)ea->arp_spa,
			    (caddr_t)ea->arp_tpa,
			    sizeof(ea->arp_tpa));
			ea->arp_sha = sain->sain_myether;
			bcopy((caddr_t)&sain->sain_myaddr,
			    (caddr_t)ea->arp_spa,
			    sizeof(ea->arp_spa));
			(*sip->si_sif->sif_xmit)(sip->si_devdata, buf, 
						sizeof(struct arp_packet));
		}
		return (0);
	}
	return (0);
}

/*
 * arp
 * Broadcasts to determine Ethernet address given IP address
 * See RFC 826
 */
arp(sip, sain, tmpbuf)
	register struct saioreq *sip;
	register struct sainet *sain;
	char *tmpbuf;
{
	struct arp_packet out;

	if (in_broadaddr(sain->sain_hisaddr)
#ifdef NOREVARP
	    || (sain->sain_hisaddr.s_addr & 0xFF000000) == 0
#endif
	    ) {
		sain->sain_hisether = etherbroadcastaddr;
		return;
	}
	out.arp_eh.ether_type = ETHERTYPE_ARP;
	out.arp_ea.arp_op = ARPOP_REQUEST;
	out.arp_ea.arp_tha = etherbroadcastaddr;	/* what we want */
	bcopy((caddr_t)&sain->sain_hisaddr,
	    (caddr_t)out.arp_ea.arp_tpa,
	    sizeof(out.arp_ea.arp_tpa));
	comarp(sip, sain, &out, tmpbuf);
}

/*
 * Reverse ARP client side
 * Determine our Internet address given our Ethernet address
 * See RFC 903
 */
revarp(sip, sain, tmpbuf)
	register struct saioreq *sip;
	register struct sainet *sain;
	char *tmpbuf;
{
	struct arp_packet out;

	bzero(&out, sizeof(struct arp_packet));

#ifdef NOREVARP
	bzero((caddr_t)&sain->sain_myaddr, sizeof(struct in_addr));
	bcopy((caddr_t)&sain->sain_myether.ether_addr_octet[3],
		(caddr_t)(&sain->sain_myaddr)+1, 3);
#else
	out.arp_eh.ether_type = ETHERTYPE_REVARP;
	out.arp_ea.arp_op = REVARP_REQUEST;
	out.arp_ea.arp_tha = sain->sain_myether;
	/* What we want to find out... */
	bzero(out.arp_ea.arp_tpa, sizeof(struct in_addr));
	comarp(sip, sain, &out, tmpbuf);
#endif
	bcopy(&sain->sain_myaddr, &my_in_addr, sizeof(struct in_addr));
}

/*
 * Common ARP code 
 * Broadcast the packet and wait for the right response.
 * Fills in *sain with the results
 */
comarp(sip, sain, out, tmpbuf)
	register struct saioreq *sip;
	register struct sainet *sain;
	register struct arp_packet *out;
	char *tmpbuf;
{
	register struct arp_packet *in = (struct arp_packet *)tmpbuf;
	register int e, count, time, feedback,len, delay = 2;
	char    *ind = "-\|/";
	struct in_addr tmp_ia;


	out->arp_eh.ether_dhost = etherbroadcastaddr;
	out->arp_eh.ether_shost = sain->sain_myether;
	out->arp_ea.arp_hrd =  ARPHRD_ETHER;
	out->arp_ea.arp_pro = ETHERTYPE_IP;
	out->arp_ea.arp_hln = sizeof(struct ether_addr);
	out->arp_ea.arp_pln = sizeof(struct in_addr);
	out->arp_ea.arp_sha = sain->sain_myether;
	bcopy((caddr_t)&sain->sain_myaddr,
	    (caddr_t)out->arp_ea.arp_spa, 
	    sizeof(out->arp_ea.arp_spa));
	feedback = 0;

	for (count=0; ; count++) {
		if (count == WAITCNT) {
			if (out->arp_ea.arp_op == ARPOP_REQUEST) {
				printf("\nRequesting Ethernet address for ");
				bcopy((caddr_t)out->arp_ea.arp_tpa,
				    (caddr_t)&tmp_ia, sizeof(tmp_ia));
				inet_print(tmp_ia);
			} else {
				printf("\nRequesting Internet address for ");
				ether_print(&out->arp_ea.arp_tha);
			}
		}
		e = (*sip->si_sif->sif_xmit)(sip->si_devdata, (caddr_t)out,
			sizeof *out);
		if (e)
			printf("X\b");
		else
			printf("%c\b", ind[feedback++ % 4]); /* Show activity */

		time = millitime() + (delay * 1000);	/* broadcast delay */
		while (millitime() <= time) {
			len = (*sip->si_sif->sif_poll)(sip->si_devdata, tmpbuf);
			if (len < used_size)
				continue;
			if (in->arp_ea.arp_pro != ETHERTYPE_IP)
				continue;
			if (out->arp_ea.arp_op == ARPOP_REQUEST) {
				if (in->arp_eh.ether_type != ETHERTYPE_ARP)
					continue;
				if (in->arp_ea.arp_op != ARPOP_REPLY)
					continue;
				if (bcmp((caddr_t)in->arp_ea.arp_spa,
				    (caddr_t)out->arp_ea.arp_tpa,
				     sizeof(struct in_addr)) != 0)
					continue;
				if (count >= WAITCNT) {
					printf("Found at ");
					ether_print(&in->arp_ea.arp_sha);
				}
				sain->sain_hisether = in->arp_ea.arp_sha;
				return;
			} else {		/* Reverse ARP */
				if (in->arp_eh.ether_type !=ETHERTYPE_REVARP)
					continue;
				if (in->arp_ea.arp_op != REVARP_REPLY)
					continue;
				if (bcmp((caddr_t)&in->arp_ea.arp_tha,
				    (caddr_t)&out->arp_ea.arp_tha, 
				    sizeof (struct ether_addr)) != 0)
					continue;

				if (count >= WAITCNT) {
					printf("Internet address is ");
					bcopy((caddr_t)in->arp_ea.arp_tpa,
					    (caddr_t)&tmp_ia, sizeof(tmp_ia));
					inet_print(tmp_ia);
				}
				bcopy((caddr_t)in->arp_ea.arp_tpa, 
				    (caddr_t)&sain->sain_myaddr,
				    sizeof(sain->sain_myaddr));
				/*
				 * short circuit first ARP
				 */
				bcopy((caddr_t)in->arp_ea.arp_spa, 
				    (caddr_t)&sain->sain_hisaddr,
				    sizeof(sain->sain_hisaddr));
				sain->sain_hisether = in->arp_ea.arp_sha;
				return;
			}
		}

		delay = delay * 2;	/* Double the request delay */
		if (delay > 64)		/* maximum delay is 64 seconds */
			delay = 64;

		(*sip->si_sif->sif_reset)(sip->si_devdata);
	}
	/*NOTREACHED*/
}

/*
 * Return the host portion of an internet address.
 */
u_long
in_lnaof(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);

	if (IN_CLASSA(i))
		return ((i)&IN_CLASSA_HOST);
	else if (IN_CLASSB(i))
		return ((i)&IN_CLASSB_HOST);
	else
		return ((i)&IN_CLASSC_HOST);
}

/*
 * Test for broadcast IP address
 */
in_broadaddr(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);

	if (IN_CLASSA(i)) {
		i &= IN_CLASSA_HOST;
		return (i == 0 || i == 0xFFFFFF);
	} else if (IN_CLASSB(i)) {
		i &= IN_CLASSB_HOST;
		return (i == 0 || i == 0xFFFF);
	} else if (IN_CLASSC(i)) {
		i &= IN_CLASSC_HOST;
		return (i == 0 || i == 0xFF);
	} else
		return (0);
	/*NOTREACHED*/
}

/*
 * Compute one's complement checksum
 * for IP packet headers 
 */
ipcksum(cp, count)
	caddr_t	cp;
	register unsigned short	count;
{
	register unsigned short	*sp = (unsigned short *)cp;
	register unsigned long	sum = 0;
	register unsigned long	oneword = 0x00010000;

	count >>= 1;
	while (count--) {
		sum += *sp++;
		if (sum >= oneword) {		/* Wrap carries into low bit */
			sum -= oneword;
			sum++;
		}
	}
	return (~sum);
}

inet_print(s)
	struct in_addr s;
{
	printf("%d.%d.%d.%d\n",
		(s.s_addr >> 24) & 0xff,
		(s.s_addr >> 16) & 0xff,
		(s.s_addr >>  8) & 0xff,
		s.s_addr & 0xff);
}

ether_print(ea)
	struct ether_addr *ea;
{
	register u_char *p = &ea->ether_addr_octet[0];

	printf("%x:%x:%x:%x:%x:%x\n", p[0], p[1], p[2], p[3], p[4], p[5]);
}
