#ifndef lint
static	char sccsid[] = "@(#)ndbootd.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * ndbootd.c  ND boot block server.
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * This program serves boot blocks to Sun-2 workstations
 * (i.e. machines with an "old" prom) using the ND protocol.
 *
 * If ether_addr.c is not present in the C library (this will
 * probably be the case on machines other than Suns), it must be
 * otherwise provided so that ether_ntohost() and ether_hostton()
 * will work.  Also, it is necessary to be able to open a raw IP
 * socket that delivers all ND packets (protocol number 77) that
 * have either the server's IP address or all zeroes in the IP
 * destination address field.
 *
 * This code assumes that the fields of a 'struct packet' will
 * be sequentially ordered in a contiguous block of memory
 * (i.e. no holes) so that it (or parts of it) can be sent
 * and received as-is with sendto and recvfrom.
 */

#include <stdio.h>
#ifdef sun
#include <malloc.h>
#endif
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "nddefs.h"

#ifndef IPPROTO_ND
#define IPPROTO_ND 77			/* ND protocol number */
#endif
#define IP_HDRLEN 5			/* length (in longs) of the ip
					   header portion of ND packets */
#define ND_HDRLEN 7			/* length (in longs) of an ND header */
#define NDBLOCKSIZ 512			/* size of an ND "block" (in bytes) */
#define NDBOOTBLKMAX 16			/* highest valid boot block number */
#define NDBOOT_DATALEN 1024		/* amount of data requested (in bytes)
					   per boot block request */
#define BBFILE "/tftpboot/sun2.bb"	/* boot block filename */
#define BOOTFN_FMT "/tftpboot/%08X%s"	/* boot program filename format */
#define BOOTFN_LEN 32			/* boot program filename length */
#define HOSTNAME_LEN 256		/* maximum hostname length */
#define HTBLSIZ 20			/* size of host address cache */
#define NULL_IP 0			/* null IP address */
#define ETHER_LEN 6			/* size of Ethernet address */

/*
 * On a Sun, struct ether_addr should already be defined in
 * <netinet/if_ether.h>.
 */
#ifndef sun
struct ether_addr {
	u_char ether_addr_octet[ETHER_LEN];
};
#endif

struct iphdr {
	u_long fill[IP_HDRLEN];
};

struct packet {
	struct iphdr ip;
	struct ndhdr nd;
	char databuf[NDBOOT_DATALEN];
};

struct hostentry {
	u_long eaddr, iaddr;
	struct arpreq ar;
	short ru;	/* ru: recently used */
} *htable[HTBLSIZ];	/* host address table */

int s;			/* socket */
int mru;		/* most recently used table entry */
int dflag = 0, vflag = 0;
char str[80];		/* string for printing */
int usecache;           /* use the address cache? */

extern long lseek();
u_long getip();
int display_recv(), display_ip(), display_nd();
int dpr();
int htable_init();
int detach();


main(argc, argv)
	int argc;
	char **argv;
{
	char *fname = BBFILE;
	struct packet p;
	static struct sockaddr_in from, to = { AF_INET };
	int f, cc, cr, fromlen, tolen = sizeof(to), bufsiz = sizeof(p);
	int reqsiz = sizeof(p.ip) + sizeof(p.nd);
	int replysiz = sizeof(p.nd) + sizeof(p.databuf);
	u_long ipaddr, src, oldsrc = 0;
	long pos, blk, oldblk = -1;
	int i;

	while (argc > 1 && argv[1][0] == '-' && argv[1][2] == '\0') {
		switch (argv[1][1]) {
		case 'd':
			dflag++;	/* print debug messages */
			break;
		case 'v':
			vflag++;	/* verbose */
			break;
		case 'c':
			usecache++;
			break;
		default:
			usage();
		}
		argc--, argv++;
	}
	if (argc > 1)
		usage();

	if (!dflag && !vflag)
		detach();
	if (vflag)
		printf("(socket %d file %d)\n", s, f);
	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_ND)) < 0) {
		perror("ndbootd: socket");
		exit(1);
	}
	if ((f = open(fname, O_RDONLY)) < 0) {
		perror("ndbootd: open");
		exit(1);
	}
	htable_init();

	for (;;) {

		/*
		 * Wait for an ND packet.
		 */
		if (vflag)
			printf("\nWaiting for a request...\n");
		fromlen = sizeof(from);
		if ((cc = recvfrom(s, (char *)&p, bufsiz, 0,
		    (struct sockaddr *)&from, &fromlen)) < 0) {
		    	perror("ndbootd: recvfrom");
			continue;
		}
		if (vflag) {
			display_recv(cc, &from, fromlen);
			display_ip(&p.ip);
			display_nd(&p.nd);
		}

		/*
		 * Check to see if the received packet is from one of our
		 * clients by using getip(), and verify that it is a valid
		 * boot block request.
		 */
		src = ntohl(from.sin_addr.s_addr);
		blk = ntohl(p.nd.np_blkno);
		if ((ipaddr = getip(src)) == NULL_IP  ||  cc != reqsiz  ||
		    p.nd.np_op != (NDOPREAD | NDOPWAIT)  ||
		    blk > NDBOOTBLKMAX  ||  blk < 0  ||  p.nd.np_error) {
			sprintf(str, "ndbootd: ignoring packet %x", src);
			dpr(str);
			continue;
		}

		/*
		 * Reply to the request.
		 */
		if (vflag)
			printf("Replying:\n");
		p.nd.np_op |= NDOPDONE;
		p.nd.np_ccount = htonl(NDBOOT_DATALEN);

		/*
		 * Block 0 of the file is actually what the client expects
		 * as block 1, so we lseek one block less into the file.
		 * (The client expects a disk label to occupy block 0 of
		 * device ndp0, and it throws this away.)  When asked for
		 * block 0, we'll read the file's block 0 into our buffer
		 * starting one block past the beginning; the first block
		 * of the buffer will contain garbage.
		 */
		pos = blk ? lseek(f, (long)(NDBLOCKSIZ*(blk-1)), L_SET)
			  : lseek(f, 0L, L_SET);
		if (pos < 0) {
			perror("ndbootd: lseek");
			continue;
		}

		cr = blk ? read(f, p.databuf, NDBOOT_DATALEN)
			 : read(f, &p.databuf[NDBLOCKSIZ], NDBOOT_DATALEN -
			   NDBLOCKSIZ);
		if (cr < 0) {
			perror("ndbootd: read");
			continue;
		}
		if (vflag)
			printf("(read %d bytes at file pos %d)\n", cr, pos);

		/*
		 * Send the reply.
		 */
		to.sin_addr.s_addr = htonl(ipaddr);
		if ((cc = sendto(s, (char *)&p.nd, replysiz, 0,
		    (struct sockaddr *)&to, tolen)) < 0) {
			perror("ndbootd: sendto");
			continue;
		}

		/*
		 * If the last packet processed was from the same client
		 * asking for the same block, we'll report a packet
		 * retransmission.  This report will also appear if the
		 * first request of a boot session is for the same block
		 * as the last request of the previous boot session
		 * (which is usually the case, if no other clients have
		 * booted in the meantime).
		 *
		 * Retransmission may be caused by an out-of-date ARP
		 * table, so we should reload the ARP table entry.
		 */
		if (blk == oldblk && src == oldsrc) {
			sprintf(str, "ndbootd: re-xmit block %d to %x",
				blk, src);
			dpr(str);
			if ((i = lookup_by_iaddr(ipaddr)) >= 0 &&
			    ioctl(s, SIOCSARP,
				  (caddr_t)&htable[i]->ar) < 0) {
				perror("ndbootd: ioctl");
			}
		}
		else {
			oldblk = blk;
			oldsrc = src;
		}
		if (vflag)
			display_nd(&p.nd);
	}
}


usage()
{
	fprintf(stderr, "Usage: ndbootd [-d] [-v]\n");
	exit(1);
}


/*
 * Print debug message.
 */
dpr(str)
	char str[];
{
	if (dflag) {
		fprintf(stderr, str);
		fprintf(stderr, "\n");
	}
}


/*
 * Detach from tty.
 */
detach()
{
	int tt;

	switch (fork()) {
	case -1:
		perror("ndbootd: fork");
		break;
	case 0:
		break;
	default:
		exit(0);
	}
	tt = getdtablesize();
	while (-tt >= 0) {
		close(tt);
	}
	open("/dev/null", O_RDWR, 0);
	dup(0);
	dup(0);
	tt = open("/dev/tty", O_RDWR);
	if (tt > 0) {
		ioctl(tt, TIOCNOTTY, 0);
		close(tt);
	}
}
 

/*
 * Display results of recvfrom.
 */
display_recv(cc, fr, fromlen)
	int cc;
	struct sockaddr_in *fr;
	int fromlen;
{
	u_long z1 = *(u_long *)&fr->sin_zero[0];
	u_long z2 = *(u_long *)&fr->sin_zero[4];

	printf("len:%d fromlen:%d fam:%d port:%u addr:%x z:", cc, fromlen,
	    fr->sin_family, ntohs(fr->sin_port), ntohl(fr->sin_addr.s_addr));
	if (z1 || z2)
		printf("%08x%08x\n", z1, z2);
	else
		printf("0\n");
}


/*
 * Display an IP packet header.
 */
display_ip(ipp)
	struct iphdr *ipp;
{
	int i;

	printf("-ip-\t");
	for (i = 0; i < IP_HDRLEN; i++)
		printf("%08x  ", ntohl(ipp->fill[i]));
	printf("\n");
}


/*
 * Display an ND packet header.
 */
display_nd(ndp)
	struct ndhdr *ndp;
{
	u_long *ptr = (u_long *)ndp;
	long ct = ntohl(ndp->np_ccount);
	int i;

	printf("-nd-\t");
	for (i = 0; i < ND_HDRLEN; i++) {
		printf("%08x  ", ntohl(*ptr++));
		if (i == 4)
			printf("\n\t");
	}
	printf("\n");
	if (ct)
		printf("data\t%d bytes\n", ct);
}


/*
 * Table Maintenance Functions:  The following functions
 * (through cache_host()) maintain a table containing IP
 * and Ethernet addresses of clients.  The table is kept
 * so that it will not be necessary to do ether_ntohost(),
 * gethostbyname(), etc. every time a packet arrives.  If
 * the table is full when we need to make an entry, we
 * first delete an entry that has not been recently used.
 *
 * MARK_AS_RECENT() marks a table entry as recently used
 * (called when an entry is put in or referenced).
 *
 * MARK_AS_NOT_RECENT() marks a table entry as not recently
 * used (called by the lru() routine when searching for a
 * table position to use).
 *
 * RECENTLY_USED() returns a logical value indicating whether
 * an entry has been recently used.
 */
#define MARK_AS_RECENT(i)	(htable[i]->ru = 1, mru = i)
#define MARK_AS_NOT_RECENT(i)	(htable[i]->ru = 0)
#define RECENTLY_USED(i)	(htable[i]->ru)


/*
 * Zero the table.
 */
htable_init()
{
	int i;

	for (i = 0; i < HTBLSIZ; i++)
		htable[i] = (struct hostentry *)NULL;
}


/*
 * Look up an IP address in the table.  If found, return the table
 * index; otherwise, return -1.
 */
int
lookup_by_iaddr(isrc)
	u_long isrc;
{
	int i;

	if (usecache) {
		for (i = 0; i < HTBLSIZ; i++) {
			if (htable[i] != (struct hostentry *)NULL &&
			    htable[i]->iaddr == isrc) {
				return(i);
			}
		}
	}
	return(-1);
}


/*
 * Look up an Ethernet address in the table.  If found, return
 * the table index; otherwise, return -1.
 *
 * The format of the address is four bytes; the first byte is zero,
 * and the remaining bytes are the last three bytes of the actual
 * Ethernet address (this is what ND clients are expected to put
 * in the IP source address field of their initial broadcast packet).
 */
int
lookup_by_eaddr(esrc)
	u_long esrc;
{
	int i;

	if (usecache) {
		for (i = 0; i < HTBLSIZ; i++) {
			if (htable[i] != (struct hostentry *)NULL &&
			    htable[i]->eaddr == esrc) {
				return(i);
			}
		}
	}
	return(-1);
}


/*
 * Return the index of a never-used table position if possible;
 * otherwise, return the index of a not-recently-used entry.
 */
int
lru()
{
	int i;

	for (i = 0; i < HTBLSIZ; i++)
		if (htable[i] == (struct hostentry *)NULL)
			return(i);
	i = (mru + 1) % HTBLSIZ;
	while (RECENTLY_USED(i)) {
		MARK_AS_NOT_RECENT(i);
		i = (i + 1) % HTBLSIZ;
	}
	return(i);
}


/*
 * Construct a hostentry, set an ARP table entry, and record
 * the host's address information in htable.  Return 0 if successful,
 * otherwise -1.
 */
int
cache_host(esrc, isrc, ether)
	u_long esrc, isrc;
	u_char ether[];
{
	int i;
	struct hostentry *h;
	struct sockaddr_in *sin;

	h = (struct hostentry *)malloc(sizeof(struct hostentry));
	bzero((char *)h, sizeof(struct hostentry));
	h->eaddr = esrc;
	h->iaddr = isrc;
	h->ar.arp_pa.sa_family = AF_INET;
	sin = (struct sockaddr_in *)&h->ar.arp_pa;
	sin->sin_addr.s_addr = htonl(isrc);
	h->ar.arp_ha.sa_family = AF_UNSPEC;
	bcopy((char *)ether, h->ar.arp_ha.sa_data, ETHER_LEN);
	h->ar.arp_flags = ATF_COM;
	if (ioctl(s, SIOCSARP, (caddr_t)&h->ar) < 0) {
		perror("ndbootd: ioctl");
		free((char *)h);
		return(-1);
	}

	i = lru();
	if (htable[i] != (struct hostentry *)NULL)
		free((char *)htable[i]);
	htable[i] = h;
	MARK_AS_RECENT(i);
	sprintf(str, "(entry made at table[%d])", i);
	dpr(str);
	return(0);
}


/*
 * getip() will return the IP address corresponding to address 'src'
 * if it is one of our clients; otherwise, return a null IP address.
 * 'src' is either an IP address or a partial Ethernet address.
 * This function is called by main() and uses the above Table
 * Maintenance Functions.
 *
 * When an ND client boots, it first broadcasts an ND boot block
 * request using its Ethernet address (in the format expected by
 * lookup_by_eaddr()) as its IP address, while subsequent packets
 * have the appropriate IP source address.  When a packet arrives,
 * if the sender is not in the host address table, we determine
 * whether to serve him by seeing if his boot program exists.  If
 * so, we add him to the host address table and set an ARP table
 * entry.
 */
u_long
getip(src)
	u_long src;
{
	/*
	 * Since this server is only meant to serve Sun-2's, we
	 * will assume that 8:0:20 is to be prepended to a partial
	 * Ethernet address given by 'src'.  All Sun-2's have this
	 * Ethernet address format.
	 */
	static u_char ether[ETHER_LEN] = { 8, 0, 0x20 };
	u_char eth[ETHER_LEN];
	struct hostent *hp;
	char hostname[HOSTNAME_LEN];
	u_long ipaddr, n_src, esrc, n_esrc;
	int i;
	
	/*
	 * A valid IP address should have a number greater than zero
	 * in the first byte of the address, while a partial Ethernet
	 * address should have a zero.
	 */
	if (src & 0xff000000) {
		if ((i = lookup_by_iaddr(src)) >= 0) {
			MARK_AS_RECENT(i);
			dpr("(ip in table)");
			return(src);
		}
		if (!ourclient(src)) {
			dpr("client not ours");
			return(NULL_IP);
		}
		n_src = htonl(src);
		if ((hp = gethostbyaddr((char *)&n_src, sizeof(n_src),
		    AF_INET)) == (struct hostent *)NULL) {
			dpr("ndbootd: gethostbyaddr failed");
			return(NULL_IP);
		}
		if (ether_hostton(hp->h_name, (struct ether_addr *)eth)) {
			dpr("ndbootd: ether_hostton failed");
			return(NULL_IP);
		}
		n_esrc = eth[3];
		n_esrc = n_esrc << 8 | eth[4];
		n_esrc = n_esrc << 8 | eth[5];
		esrc = ntohl(n_esrc);
		if (cache_host(esrc, src, eth) < 0)
			return(NULL_IP);
		return(src);
	}

	/*
	 * It wasn't an IP address, so it must be an Ether.  
	 */
	if ((i = lookup_by_eaddr(src)) >= 0) {
		if (ioctl(s, SIOCSARP, (caddr_t)&htable[i]->ar) < 0) {
			perror("ndbootd: ioctl");
			return(NULL_IP);
		}
		MARK_AS_RECENT(i);
		dpr("(ether in table)");
		return(htable[i]->iaddr);
	}

	/*
	 * Ether not in table.  Determine whether he's our client.
	 */
	ether[3] = src >> 16 & 0xff;
	ether[4] = src >>  8 & 0xff;
	ether[5] = src       & 0xff;
	if (ether_ntohost(hostname, (struct ether_addr *)ether)) {
		dpr("ndbootd: ether_ntohost failed");
		return(NULL_IP);
	}
	if ((hp = gethostbyname(hostname)) == (struct hostent *)NULL) {
		dpr("ndbootd: gethostbyname failed");
		return(NULL_IP);
	}
	ipaddr = ntohl(*(u_long *)hp->h_addr);
	if (!ourclient(ipaddr)) {
		dpr("client not ours");
		return(NULL_IP);
	}

	/*
	 * He's our client.  Put him in the table.
	 */
	if (cache_host(src, ipaddr, ether) < 0)
		return(NULL_IP);
	return(ipaddr);
}

ourclient(addr)
	u_long addr;
{
	char  bootfn[BOOTFN_LEN];
	struct stat statbuf;

	sprintf(bootfn, BOOTFN_FMT, addr, ".SUN2");
	if (stat(bootfn, &statbuf) < 0) {
		/* 
	 	 * If IPADDR.SUN2 is not found, try the
	 	 * old style which is just IPADDR.
	 	 */
		sprintf(bootfn, BOOTFN_FMT, addr, "");
		if (stat(bootfn, &statbuf) < 0) {
			return (0);
		}
	}
	return (1);
}
