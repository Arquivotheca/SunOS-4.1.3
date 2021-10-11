#ident "@(#)ping.c 1.1 92/07/30 SMI from UCB 4.9 6/18/88"

/*
 *			P I N G . C
 *
 * Using the InterNet Control Message Protocol (ICMP) "ECHO" facility,
 * measure round-trip-delays and packet loss across network paths.
 *
 * Author -
 *	Mike Muuss
 *	U. S. Army Ballistic Research Laboratory
 *	December, 1983
 * Modified at Uc Berkeley
 * Modified at Sun Microsystems
 *
 * Status -
 *	Public Domain.  Distribution Unlimited.
 *
 * Bugs -
 *	More statistics could always be gathered.
 *	This program has to run SUID to ROOT to access the ICMP socket.
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>

#define	MAXWAIT		10	/* max time to wait for response, sec. */
#define	MAXPACKET	4096	/* max packet size */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	64
#endif

int	verbose;
int	stats;
u_char	packet[MAXPACKET];
int	options;
extern	int errno;

int s;			/* Socket file descriptor */
struct hostent *hp;	/* Pointer to host info */

struct sockaddr whereto;/* Who to ping */
int datalen;		/* How much data */

char usage[] =
"Usage:	ping host [timeout]\n\
	ping -s[drvRl] host [data size] [npackets]\n";

char *hostname;
char hnamebuf[MAXHOSTNAMELEN];

int npackets;
int ntransmitted = 0;		/* sequence # for outbound packets = #sent */
int ident;

int nreceived = 0;		/* # of packets we got back */
int timing = 0;
int tmin = 999999999;
int tmax = 0;
int tsum = 0;			/* sum of all times, for doing average */
int record = 0;			/* true if using record route */
int loose = 0;			/* true if using loose source route */

# define MAX_ROUTES 9		/* maximum number of source route space */
# define ROUTE_SIZE (IPOPT_OLEN + IPOPT_OFFSET + \
	   		MAX_ROUTES * sizeof(struct in_addr))
void finish(), catcher();

/*
 * 			M A I N
 */
main(argc, argv)
char *argv[];
{
	struct sigvec sv;
	struct sockaddr_in from;
	char **av = argv;
	struct sockaddr_in *to = (struct sockaddr_in *) &whereto;
	int on = 1;
	int timeout = 20;

	argc--, av++;
	while (argc > 0 && *av[0] == '-') {
		while (*++av[0]) switch (*av[0]) {
			case 'd':
				options |= SO_DEBUG;
				break;
			case 'r':
				options |= SO_DONTROUTE;
				break;
			case 'R':
				record = 1;
				break;
			case 'l':
				loose = 1;
				break;
			case 'v':
				verbose++;
				break;
			case 's':
				stats++;
				break;
		}
		argc--, av++;
	}
	if( argc < 1)  {
		fprintf(stderr, usage);
		exit(1);
	}

	bzero( (char *)&whereto, sizeof(struct sockaddr) );
	to->sin_family = AF_INET;
	to->sin_addr.s_addr = inet_addr(av[0]);
	if (to->sin_addr.s_addr != -1) {
		strcpy(hnamebuf, av[0]);
		hostname = hnamebuf;
	} else {
		hp = gethostbyname(av[0]);
		if (hp) {
			to->sin_family = hp->h_addrtype;
			bcopy(hp->h_addr, (caddr_t)&to->sin_addr, hp->h_length);
			hostname = hp->h_name;
		} else {
			fprintf(stderr, "%s: unknown host %s\n", argv[0], av[0]);
			exit(1);
		}
	}

	if( argc >= 2 )
		datalen = atoi( av[1] );
	else
		datalen = 64-8;
	if (argc > 2)
		npackets = atoi(av[2]);
	if (!stats && argc >= 2) {
		npackets = 0;
		timeout = atoi(av[1]);
		datalen = 64-8;
	}
	if (datalen > MAXPACKET) {
		fprintf(stderr, "ping: packet size too large\n");
		exit(1);
	}
	if (datalen >= sizeof(struct timeval))
		timing = 1;

	ident = getpid() & 0xFFFF;

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
		perror("ping: socket");
		exit(5);
	}
	if (options & SO_DEBUG)
		setsockopt(s, SOL_SOCKET, SO_DEBUG, &on, sizeof(on));
	if (options & SO_DONTROUTE)
		setsockopt(s, SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on));
	if (record || loose) {
	  /*
	   * Set the record route option
	   */
	    char srr[ROUTE_SIZE + 1];
	    int optsize = sizeof(srr);
	    
	    bzero(srr, sizeof(srr));
	    srr[0] = IPOPT_RR;
	    srr[1] = ROUTE_SIZE;
	    srr[2] = IPOPT_MINOFF;
	    if (loose) {
	        struct sockaddr_in mine; 

		srr[0] = IPOPT_LSRR;
		srr[1] = 11;
		get_myaddress( &mine );
	    	bcopy( (char *)&to->sin_addr, srr+3, sizeof(struct in_addr));
	    	bcopy( (char *)&mine.sin_addr, srr+7, sizeof(struct in_addr));
		if (record) {
		    srr[11] = IPOPT_RR;
		    srr[12] = ROUTE_SIZE - 11;
		    srr[13] = IPOPT_MINOFF;
		}
		else {
		    optsize = 12;
		}
	    }
	    if (setsockopt(s, IPPROTO_IP, IP_OPTIONS, srr, optsize) < 0)
		perror("IP Options");
	}

	if (stats)
		printf("PING %s: %d data bytes\n", hostname, datalen );

	setlinebuf( stdout );

	if (stats)
		signal( SIGINT, finish );
	/*
	 * Make sure that this signal actually interrupts (rather than
	 * restarts) the recvfrom call below.
	 */
	sv.sv_handler = catcher;
	sv.sv_mask = 0;
	sv.sv_flags = SV_INTERRUPT;
	(void) sigvec(SIGALRM, &sv, (struct sigvec *)NULL);

	catcher();	/* start things going */

	for (;;) {
		int len = sizeof (packet);
		int fromlen = sizeof (from);
		int cc;

		if (!stats && ntransmitted > timeout) {
			printf("no answer from %s\n", hostname);
			exit(1);
		}
		if ( (cc=recvfrom(s, packet, len, 0, &from, &fromlen)) < 0) {
			if( errno == EINTR )
				continue;
			perror("ping: recvfrom");
			continue;
		}
		pr_pack( packet, cc, &from );
		if (npackets && nreceived >= npackets)
			finish();
	}
	/*NOTREACHED*/
}

/*
 * 			C A T C H E R
 * 
 * This routine causes another PING to be transmitted, and then
 * schedules another SIGALRM for 1 second from now.
 * 
 * Bug -
 * 	Our sense of time will slowly skew (ie, packets will not be launched
 * 	exactly at 1-second intervals).  This does not affect the quality
 *	of the delay and loss statistics.
 */
void
catcher()
{
	int waittime;

	pinger();
	if (npackets == 0 || ntransmitted < npackets)
		alarm(1);
	else {
		if (nreceived) {
			waittime = 2 * tmax / 1000;
			if (waittime == 0)
				waittime = 1;
		} else
			waittime = MAXWAIT;
		signal(SIGALRM, finish);
		alarm(waittime);
	}
}

/*
 * 			P I N G E R
 * 
 * Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
pinger()
{
	static u_char outpack[MAXPACKET];
	register struct icmp *icp = (struct icmp *) outpack;
	int i, cc;
	register struct timeval *tp = (struct timeval *) &outpack[8];
	register u_char *datap = &outpack[8+sizeof(struct timeval)];

	icp->icmp_type = loose ? ICMP_ECHOREPLY : ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = ntransmitted++;
	icp->icmp_id = ident;		/* ID */

	cc = datalen+8;			/* skips ICMP portion */

	if (timing)
		gettimeofday( tp, (struct timezone *) NULL );

	for( i=8; i<datalen; i++)	/* skip 8 for time */
		*datap++ = i;

	/* Compute ICMP checksum here */
	icp->icmp_cksum = in_cksum( icp, cc );

	/* cc = sendto(s, msg, len, flags, to, tolen) */
	i = sendto( s, outpack, cc, 0, &whereto, sizeof(struct sockaddr) );

	if( i < 0 || i != cc )  {
		if( i<0 ) {
		    perror("sendto");
		    if (!stats)
			exit(1);
		}
		printf("ping: wrote %s %d chars, ret=%d\n",
			hostname, cc, i );
		fflush(stdout);
	}
}

/*
 * 			P R _ T Y P E
 *
 * Convert an ICMP "type" field to a printable string.
 */
char *
pr_type( t )
register int t;
{
	static char *ttab[] = {
		"Echo Reply",
		"ICMP 1",
		"ICMP 2",
		"Dest Unreachable",
		"Source Quench",
		"Redirect",
		"ICMP 6",
		"ICMP 7",
		"Echo",
		"ICMP 9",
		"ICMP 10",
		"Time Exceeded",
		"Parameter Problem",
		"Timestamp",
		"Timestamp Reply",
		"Info Request",
		"Info Reply",
		"Netmask Request",
		"Netmask Reply"
	};

	if ( t < 0 || t > 16 )
		return("OUT-OF-RANGE");

	return(ttab[t]);
}

/*
 *			P R _ N A M E
 *
 * Return a string name for the given IP address.
 */
char *pr_name(addr)
    struct in_addr addr;
{
	char *inet_ntoa();
	struct hostent *phe;
	static char buf[256];

	phe = gethostbyaddr(&addr.s_addr, 4, AF_INET);
	if (phe == NULL) 
		return( inet_ntoa(addr));
	(void) sprintf(buf, "%s (%s)", phe->h_name, inet_ntoa(addr));
	return(buf);
}

/*
 * Print the IP protocol
 */
char *
pr_protocol(p)
{
    static char buf[20];

    switch (p) {
    	case IPPROTO_ICMP:	return("icmp");
    	case IPPROTO_TCP:	return("tcp");
    	case IPPROTO_UDP:	return("udp");
    }
    sprintf(buf,"prot %d",p);
    return(buf);
}


/*
 *			P R _ P A C K
 *
 * Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
pr_pack( buf, cc, from )
char *buf;
int cc;
struct sockaddr_in *from;
{
	struct ip *ip;
	register struct icmp *icp;
	register long *lp = (long *) packet;
	register int i;
	struct timeval tv;
	struct timeval *tp;
	int hlen, triptime;
	struct sockaddr_in *to = (struct sockaddr_in *) &whereto;
	long w;
	static char *unreach[] = {
	    "Net Unreachable",
	    "Host Unreachable",
	    "Protocol Unreachable",
	    "Port Unreachable",
	    "Fragmentation needed and DF set",
	    "Source Route Failed"
	};
	static char *redirect[] = {
	    "Net",
	    "Host",
	    "TOS Net",
	    "TOS Host"
	};

	gettimeofday( &tv, (struct timezone *) NULL );

	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	if (cc < hlen + ICMP_MINLEN) {
		if (verbose)
			printf("packet too short (%d bytes) from %s\n", cc,
				pr_name(from->sin_addr));
		return;
	}
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	if (ip->ip_p == 0) {
		/*
		 * Assume that we are running on a pre-4.3BSD system
		 * such as SunOS before 4.0
		 */
		 icp = (struct icmp *)buf;
	}
	switch (icp->icmp_type) {
	  case ICMP_UNREACH:
	    ip = &icp->icmp_ip;
	    if (ip->ip_dst.s_addr == to->sin_addr.s_addr || verbose) {
		printf("ICMP %s from gateway %s\n", 
			unreach[icp->icmp_code], pr_name(from->sin_addr));
		printf(" for %s from %s", pr_protocol(ip->ip_p), 
					  pr_name(ip->ip_src) );
		printf(" to %s", pr_name(ip->ip_dst) );
		if (ip->ip_p == IPPROTO_TCP || ip->ip_p == IPPROTO_UDP)
		    printf(" port %d\n", 
		    	ntohs(((u_short *)ip)[sizeof(struct ip)/2 + 1]) );
		else
		    printf("\n");
	    }
	    break;

	  case ICMP_REDIRECT:
	    ip = &icp->icmp_ip;
	    if (ip->ip_dst.s_addr == to->sin_addr.s_addr || verbose) {
		printf("ICMP %s redirect from gateway %s\n", 
			redirect[icp->icmp_code], pr_name(from->sin_addr));
		printf(" to %s", pr_name(icp->icmp_gwaddr) );
		printf(" for %s\n", pr_name(ip->ip_dst) );
	    }
	    break;

	  case ICMP_ECHOREPLY:
	    if ( icp->icmp_id != ident )
		return;			/* 'Twas not our ECHO */

	    if (!stats) {
		printf("%s is alive\n", hostname);
		exit(0);
	    }
	    tp = (struct timeval *)&icp->icmp_data[0];
	    printf("%d bytes from %s: ", cc, pr_name(from->sin_addr));
	    printf("icmp_seq=%d. ", icp->icmp_seq );
	    if (timing) {
		tvsub( &tv, tp );
		triptime = tv.tv_sec*1000+(tv.tv_usec/1000);
		printf("time=%d. ms\n", triptime );
		tsum += triptime;
		if ( triptime < tmin )
			tmin = triptime;
		if ( triptime > tmax )
			tmax = triptime;
	    } else
		putchar('\n');
	    nreceived++;
	    break;

	  case ICMP_SOURCEQUENCH:
	  case ICMP_TIMXCEED:
	    ip = &icp->icmp_ip;
	    if (ip->ip_dst.s_addr == to->sin_addr.s_addr || verbose) {
		printf("ICMP %s from %s\n",pr_type(icp->icmp_type),
					   pr_name(from->sin_addr));
		printf(" for %s", pr_name(ip->ip_src) );
		printf(" to %s", pr_name(ip->ip_dst) );
		printf(" port %d\n", ((u_short *)ip)[sizeof(struct ip)/2 + 1] );
	    }
	    break;

	  default:
	    if (verbose) {
		printf("%d bytes from %s:\n", cc,
			pr_name(from->sin_addr));
		printf("icmp_type=%d (%s) ",
				icp->icmp_type, pr_type(icp->icmp_type) );
		printf("icmp_code=%d\n", icp->icmp_code );
		for( i=0; i<12; i++)
		    printf("x%2.2x: x%8.8x\n", i*sizeof(long), *lp++ );
	    }
	    break;
	}
	buf += sizeof(struct ip);
	hlen -= sizeof(struct ip);
	if (verbose && hlen > 0)
		pr_options(buf, hlen);
}

/*
 *		P R _ O P T I O N S
 * Print out the ip options.
 */
pr_options(opt, optlength)
	unsigned char *opt;
	int optlength;
{
    int curlength;

    printf("  IP options: ");
    while (optlength > 0) {
       curlength = opt[1];
       switch (*opt) {
         case IPOPT_EOL:
	    optlength = 0;
	    break;
	 
	 case IPOPT_NOP:
	    opt++;
	    optlength--;
	    continue;
	 
	 case IPOPT_RR:
	    printf(" <record route> ");
	    ip_rrprint(opt, curlength);
	    break;
	 
	 case IPOPT_TS:
	    printf(" <time stamp>");
	    break;
	 
	 case IPOPT_SECURITY:
	    printf(" <security>");
	    break;
	 
	 case IPOPT_LSRR:
	    printf(" <loose source route> ");
	    ip_rrprint(opt, curlength);
	    break;
	 
	 case IPOPT_SATID:
	    printf(" <stream id>");
	    break;
	 
	 case IPOPT_SSRR:
	    printf(" <strict source route> ");
	    ip_rrprint(opt, curlength);
	    break;
	 
	 default:
	    printf(" <option %d, len %d>", *opt, curlength);
	    break;
       }
       /*
        * Following most options comes a length field
	*/
	opt += curlength;
	optlength -= curlength;
    }
    printf("\n");
  }


/*
 * Print out a recorded route option.
 */
ip_rrprint(opt, length)
    unsigned char *opt;
    int length;
{
    int pointer;
    struct in_addr addr;

    opt += IPOPT_OFFSET;
    length -= IPOPT_OFFSET;

    pointer = *opt++;
    pointer -= IPOPT_MINOFF;
    length--;
    while (length > 0) {
	bcopy((char *)opt, (char *)&addr, sizeof(addr));
	printf( "%s", pr_name(addr) );
	if (pointer == 0)
	    printf( "(Current)");
	opt += sizeof(addr);
	length -= sizeof(addr);
	pointer -= sizeof(addr);
	if (length >0)
	    printf( ", ");
    }
}


/*
 *			I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
in_cksum(addr, len)
u_short *addr;
int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
	u_short odd_byte = 0;
	register int sum = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while( nleft > 1 )  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if( nleft == 1 ) {
		*(u_char *)(&odd_byte) = *(u_char *)w;
		sum += odd_byte;
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}

/*
 * 			T V S U B
 * 
 * Subtract 2 timeval structs:  out = out - in.
 * 
 * Out is assumed to be >= in.
 */
tvsub( out, in )
register struct timeval *out, *in;
{
	if( (out->tv_usec -= in->tv_usec) < 0 )   {
		out->tv_sec--;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

/*
 *			F I N I S H
 *
 * Print out statistics, and give up.
 * Heavily buffered STDIO is used here, so that all the statistics
 * will be written with 1 sys-write call.  This is nice when more
 * than one copy of the program is running on a terminal;  it prevents
 * the statistics output from becomming intermingled.
 */
void
finish()
{
	printf("\n----%s PING Statistics----\n", hostname );
	printf("%d packets transmitted, ", ntransmitted );
	printf("%d packets received, ", nreceived );
	if (ntransmitted)
	    printf("%d%% packet loss",
		(int) (((ntransmitted-nreceived)*100) / ntransmitted ) );
	printf("\n");
	if (nreceived && timing)
	    printf("round-trip (ms)  min/avg/max = %d/%d/%d\n",
		tmin,
		tsum / nreceived,
		tmax );
	fflush(stdout);
	exit(0);
}
