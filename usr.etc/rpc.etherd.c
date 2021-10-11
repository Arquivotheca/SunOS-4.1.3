#ifndef lint
static  char sccsid[] = "@(#)rpc.etherd.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <ctype.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <netinet/udp.h>
#include <net/route.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/udp_var.h>
#include <netinet/ip_icmp.h>
#include <rpcsvc/ether.h>

#include <sys/stropts.h>

#include <net/nit_if.h>
#include <net/nit_buf.h>

#define	NIT_DEV		"/dev/nit"

union	netheader {
	struct  udpiphdr nh_udpipheader;
	struct ether_arp nh_arpheader;
#define nh_ipovlyheader nh_udpipheader.ui_i
#define nh_udpheader    nh_udpipheader.ui_u
} netheader;

struct	ether_header eheader;
int	wirelen;

#define IP		((struct ip *)&netheader.nh_ipovlyheader)
#define ARP		(&netheader.nh_arpheader)
#define TYPE		(ntohs(eheader.ether_type))

#define BUFSPACE        4*8192
#define CHUNKSIZE       2048
#define SNAPLEN         (sizeof(struct ether_header)+sizeof(union netheader))

int	chunksize = CHUNKSIZE;
char	buf[BUFSPACE];
struct	etherhmem *srchtable[HASHSIZE];
struct	etherhmem *dsthtable[HASHSIZE];

struct ether_addr baddr = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

extern	int errno;

int	senddata();

int if_fd = -1;
char *device;
int net;
struct etherstat etherstat;
struct etheraddrs etheraddrs;
struct addrmask srcmask, dstmask, protomask, lnthmask;

main(argc, argv)
	char **argv;
{
	SVCXPRT *transp;
	int readfds, s;
	struct timeval timeout;
	int readbit, rpcbit;
	struct sockaddr_in addr;

	if (argc < 2) {
		fprintf(stderr, "Usage: etherd interface\n");
		exit(1);
	}
	device = argv[1];
	initdevice();
	s = socket(AF_INET, SOCK_DGRAM, 0);
	transp = svcudp_create(s);
	if (transp == NULL) {
		fprintf(stderr, "couldn't create an RPC server\n");
		exit(1);
	}
	pmap_unset(ETHERSTATPROG, ETHERSTATVERS);
	if (!svc_register(transp, ETHERSTATPROG, ETHERSTATVERS,
	    senddata, IPPROTO_UDP)) {
		fprintf(stderr, "couldn't register ETHERSTAT service\n");
		exit(1);
	}
	/* 
	 * what if this machine is a gateway?
	 */
	get_myaddress(&addr);
	net = inet_netof(addr.sin_addr);

	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;
	readbit = 1 << if_fd;
	rpcbit = 1 << s;

	/* 
	 * should default be to init device?
	 */
	deinitdevice();
	for (;;) {
		if (if_fd < 0)
			readfds = rpcbit;
		else
			readfds = rpcbit | readbit;
		if (select(32, &readfds, 0, 0, 0) < 0) {
			if (errno == EINTR)
				continue;
			perror("etherd: select");
			deinitdevice();
			exit(1);
		}
		if (readfds & readbit)
			readdata();
		if (readfds & rpcbit)
			svc_getreq(1 << s);
	}
}

senddata(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	switch (rqstp->rq_proc) {
		case NULLPROC:
			if (!svc_sendreply(transp, xdr_void, 0)) {
				fprintf(stderr,"rpc.etherd:%s",
				    " couldn't reply to RPC call\n");
				deinitdevice();
				exit(1);
			}
			return;
		case ETHERSTATPROC_GETDATA:
			/* if (!svc_getargs(transp, xdr_void, NULL)) {
				svcerr_decode(transp);
				return;
			} */
			gettimeofday(&etherstat.e_time, 0);
			if (!svc_sendreply(transp, xdr_etherstat, &etherstat)) {
				fprintf(stderr,"rpc.etherd:%s",
				    " couldn't reply to RPC call\n");
				deinitdevice();
				exit(1);
			}
			return;
		case ETHERSTATPROC_GETSRCDATA:
			/* if (!svc_getargs(transp, xdr_void, NULL)) {
				svcerr_decode(transp);
				return;
			} */
			gettimeofday(&etheraddrs.e_time, 0);
			etheraddrs.e_addrs = srchtable;
			if (!svc_sendreply(transp, xdr_etheraddrs,
			    &etheraddrs)) {
				fprintf(stderr,"rpc.etherd:%s",
				    " couldn't reply to RPC call\n");
				deinitdevice();
				exit(1);
			}
			return;
		case ETHERSTATPROC_GETDSTDATA:
			/* if (!svc_getargs(transp, xdr_void, NULL)) {
				svcerr_decode(transp);
				return;
			} */
			gettimeofday(&etheraddrs.e_time, 0);
			etheraddrs.e_addrs = dsthtable;
			if (!svc_sendreply(transp, xdr_etheraddrs,
			    &etheraddrs)) {
				fprintf(stderr,"rpc.etherd:%s",
				    " couldn't reply to RPC call\n");
				deinitdevice();
				exit(1);
			}
			return;
		case ETHERSTATPROC_ON:
			/* if (!svc_getargs(transp, xdr_void, NULL)) {
				svcerr_decode(transp);
				return;
			} */
			initdevice();
			if (!svc_sendreply(transp, xdr_void, NULL)) {
				fprintf(stderr,"rpc.etherd:%s",
				    " couldn't reply to RPC call\n");
				deinitdevice();
				exit(1);
			}
			return;
		case ETHERSTATPROC_OFF:
			/* if (!svc_getargs(transp, xdr_void, NULL)) {
				svcerr_decode(transp);
				return;
			} */
			deinitdevice();
			if (!svc_sendreply(transp, xdr_void, NULL)) {
				fprintf(stderr,"rpc.etherd:%s",
				    " couldn't reply to RPC call\n");
				exit(1);
			}
			return;
		case ETHERSTATPROC_SELECTSRC:
			if (!svc_getargs(transp, xdr_addrmask, &srcmask)) {
				svcerr_decode(transp);
				return;
			}
			if (!svc_sendreply(transp, xdr_void, NULL)) {
				fprintf(stderr,"rpc.etherd:%s",
				    " couldn't reply to RPC call\n");
				deinitdevice();
				exit(1);
			}
			return;
		case ETHERSTATPROC_SELECTDST:
			if (!svc_getargs(transp, xdr_addrmask, &dstmask)) {
				svcerr_decode(transp);
				return;
			}
			if (!svc_sendreply(transp, xdr_void, NULL)) {
				fprintf(stderr,"rpc.etherd:%s",
				    " couldn't reply to RPC call\n");
				deinitdevice();
				exit(1);
			}
			return;
		case ETHERSTATPROC_SELECTPROTO:
			if (!svc_getargs(transp, xdr_addrmask, &protomask)) {
				svcerr_decode(transp);
				return;
			}
			if (!svc_sendreply(transp, xdr_void, NULL)) {
				fprintf(stderr,"rpc.etherd:%s",
				    " couldn't reply to RPC call\n");
				deinitdevice();
				exit(1);
			}
			return;
		case ETHERSTATPROC_SELECTLNTH:
			if (!svc_getargs(transp, xdr_addrmask, &lnthmask)) {
				svcerr_decode(transp);
				return;
			}
			if (!svc_sendreply(transp, xdr_void, NULL)) {
				fprintf(stderr,"rpc.etherd:%s",
				    " couldn't reply to RPC call\n");
				deinitdevice();
				exit(1);
			}
			return;
		default:
			svcerr_noproc(transp);
			return;
	}
}

readdata()
{
	register struct	nit_bufhdr *hdrp;
	register struct etherstat *p;
	register struct etheraddrs *q;
	caddr_t bp, bufstop;
	char *packet;
	char *edst, *esrc;
	int cc;

	p = &etherstat;
	q = &etheraddrs;

	if ((cc = read(if_fd, buf, sizeof buf)) < 0) {
		perror("etherd: read");
		deinitdevice();
		exit(1);
	}

	/*
	 * Process each packet in the chunk.
	 */
	for (bp = buf, bufstop = buf + cc; bp < bufstop;
	    bp += hdrp->nhb_totlen) {
		/*
		 * Set up pointers to successive objects
		 * embedded in the current message.
		 */
		hdrp = (struct nit_bufhdr *)bp;
		packet = (char *)(hdrp + 1);
		bcopy(packet, (char *)&eheader, sizeof (eheader));
		bcopy(packet + sizeof (eheader),
		    (char *)&netheader, sizeof (netheader));
		wirelen = hdrp->nhb_msglen;

		if (wirelen < MINPACKETLEN) {
			edst = (char *)&eheader.ether_dhost;
			esrc = (char *)&eheader.ether_shost;
			fprintf(stderr,
			    "%s %d dst %.2x%.2x%.2x%.2x%.2x%.2x",
			    "rpc.etherd: bad lnth",
			    wirelen,
			    0xff & edst[0], 0xff & edst[1], 0xff & edst[2],
			    0xff & edst[3], 0xff & edst[4], 0xff & edst[5]);
			fprintf(stderr,
			    "src %.2x%.2x%.2x%.2x%.2x%.2x\n",
			    0xff & esrc[0], 0xff & esrc[1], 0xff & esrc[2],
			    0xff & esrc[3], 0xff & esrc[4], 0xff & esrc[5]);
			continue;
		}
		if (srcmask.a_mask && !srcmatch())
			continue;
		if (dstmask.a_mask && !dstmatch())
			continue;
		if (protomask.a_mask && !protomatch())
			continue;
		if (lnthmask.a_mask && !lnthmatch())
			continue;
		p->e_packets++;
		q->e_packets++;
		p->e_bytes += wirelen;
		q->e_bytes += wirelen;
		p->e_size[(wirelen - MINPACKETLEN)/BUCKETLNTH]++;

		/* 
		 * first compute dst, checking for broadcast specially
		 */
		if (bcmp((char *)&eheader.ether_dhost,
		    (char *) &baddr, sizeof (struct ether_addr)) == 0) {
			p->e_bcast++;
			q->e_bcast++;
		}
		else if (TYPE == ETHERTYPE_ARP) {
			struct in_addr a;

			bcopy((char *)ARP->arp_tpa,
			    (char *)&a, sizeof (struct in_addr));
			adddst(a);
		}
		else if (TYPE == ETHERTYPE_IP)
			adddst(IP->ip_dst);

		if (TYPE == ETHERTYPE_ARP) {
			struct in_addr a;

			bcopy((char *)ARP->arp_spa,
			    (char *)&a, sizeof (struct in_addr));
			addsrc(a);
			p->e_proto[ARPPROTO]++;
		}
		else if (TYPE == ETHERTYPE_IP) {
			addsrc(IP->ip_src);
			switch(IP->ip_p) {
				case IPPROTO_ND:
					p->e_proto[NDPROTO]++;
					break;
				case IPPROTO_UDP:
					p->e_proto[UDPPROTO]++;
					break;
				case IPPROTO_TCP:
					p->e_proto[TCPPROTO]++;
					break;
				case IPPROTO_ICMP:
					p->e_proto[ICMPPROTO]++;
					break;
				default:
					p->e_proto[OTHERPROTO]++;
					break;
			}
		}
		else 
			p->e_proto[OTHERPROTO]++;
	}
}

initdevice()
{
	struct strioctl	si;
	struct ifreq	ifr;
	struct timeval	timeout;
	u_long		flags;

	if (if_fd >= 0)
		return;		/* already open */

	if ((if_fd = open(NIT_DEV, O_RDONLY)) < 0) {
		perror("nit open");
		exit (1);
	}

	/*
	 * Arrange to get discrete messages from the stream.
	 */
	if (ioctl(if_fd, I_SRDOPT, (char *)RMSGD) < 0) {
		perror("ioctl (I_SRDOPT)");
		exit (1);
	}

	si.ic_timout = INFTIM;

	/*
	 * Push and configure the buffering module.
	 */
	if (ioctl(if_fd, I_PUSH, "nbuf") < 0) {
		perror("ioctl (I_PUSH \"nbuf\")");
		exit (1);
	}

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	si.ic_cmd = NIOCSTIME;
	si.ic_len = sizeof timeout;
	si.ic_dp = (char *)&timeout;
	if (ioctl(if_fd, I_STR, (char *)&si) < 0) {
		perror("ioctl (I_STR: NIOCSTIME)");
		exit (1);
	}

	si.ic_cmd = NIOCSCHUNK;
	si.ic_len = sizeof chunksize;
	si.ic_dp = (char *)&chunksize;
	if (ioctl(if_fd, I_STR, (char *)&si) < 0) {
		perror("ioctl (I_STR: NIOCSCHUNK)");
		exit (1);
	}

	/*
	 * Configure the nit device, binding it to the proper
	 * underlying interface and setting the interface into
	 * promiscuous mode if necessary.
	 */
	(void) strncpy(ifr.ifr_name, device, sizeof ifr.ifr_name);
	ifr.ifr_name[sizeof ifr.ifr_name - 1] = '\0';
	si.ic_cmd = NIOCBIND;
	si.ic_len = sizeof ifr;
	si.ic_dp = (char *)&ifr;
	if (ioctl(if_fd, I_STR, (char *)&si) < 0) {
		perror("ioctl (I_STR: NIOCBIND)");
		exit(1);
	}

	flags = NI_PROMISC;
	si.ic_cmd = NIOCSFLAGS;
	si.ic_len = sizeof flags;
	si.ic_dp = (char *)&flags;
	if (ioctl(if_fd, I_STR, (char *)&si) < 0) {
		perror("ioctl (I_STR: NIOCSFLAGS)");
		exit (1);
	}

	/*
	 * Flush the read queue, to get rid of anything that
	 * accumulated before the device reached its final configuration.
	 */
	if (ioctl(if_fd, I_FLUSH, (char *)FLUSHR) < 0) {
		perror("ioctl (I_FLUSH)");
		exit(1);
	}
}

deinitdevice()
{
	/* Already de-initialized? */
	if (if_fd < 0)
		return;

	(void) close(if_fd);
	if_fd = -1;
}

addsrc(addr)
	struct in_addr addr;
{	
	int x;
	struct etherhmem *p;

	x = addr.s_addr & 0xff;
	for (p = srchtable[x]; p != NULL; p = p->ht_nxt) {
		if (p->ht_addr == addr.s_addr) {
			p->ht_cnt++;
			return;
		}
	}
	p = (struct etherhmem *)malloc(sizeof(struct etherhmem));
	p->ht_addr = addr.s_addr;
	p->ht_cnt = 1;
	p->ht_nxt = srchtable[x];
	srchtable[x] = p;
}

adddst(addr)
	struct in_addr addr;
{	
	int x;
	struct etherhmem *p;

	x = addr.s_addr & 0xff;
	for (p = dsthtable[x]; p != NULL; p = p->ht_nxt) {
		if (p->ht_addr == addr.s_addr) {
			p->ht_cnt++;
			return;
		}
	}
	p = (struct etherhmem *)malloc(sizeof(struct etherhmem));
	p->ht_addr = addr.s_addr;
	p->ht_cnt = 1;
	p->ht_nxt = dsthtable[x];
	dsthtable[x] = p;
}

dstmatch()
{
	int addr;

	switch (TYPE) {
		case ETHERTYPE_IP:
			addr = IP->ip_dst.s_addr;
			break;
		case ETHERTYPE_ARP:
			bcopy(ARP->arp_tpa, (caddr_t)&addr, sizeof (addr));
			break;
		default:
			return (0);
	}
	return ((addr & dstmask.a_mask)==(dstmask.a_addr & dstmask.a_mask));
}

srcmatch()
{
	int addr;

	switch (TYPE) {
		case ETHERTYPE_IP:
			addr = IP->ip_src.s_addr;
			break;
		case ETHERTYPE_ARP:
			bcopy(ARP->arp_spa, (caddr_t)&addr, sizeof (addr));
			break;
		default:
			return (0);
	}
	return ((addr & srcmask.a_mask) == (srcmask.a_addr & srcmask.a_mask));
}

protomatch()
{
	int proto;

	switch (TYPE) {
		case ETHERTYPE_IP:
			proto = IP->ip_p;
			break;
		default:
			return (0);
	}
	return ((proto & protomask.a_mask) ==
	    (protomask.a_addr & protomask.a_mask));
}

/*
 * for size, a_addr is lowvalue, a_mask is high value
 */
lnthmatch()
{
	return (wirelen >= lnthmask.a_addr && wirelen <= lnthmask.a_mask);
}

/* 
 * for debugging
 */
#if 0
printdata()
{
	int i;
	struct etherhmem *p;

	for (i = 0; i < HASHSIZE; i++) {
		for (p = srchtable[i]; p != NULL; p = p->ht_nxt)
			printf("%x %d\n", p->ht_addr, p->ht_cnt);
	}
}
#endif
