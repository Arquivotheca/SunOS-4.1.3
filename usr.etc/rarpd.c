#ifndef lint
static  char *sccsid = "@(#)rarpd.c 1.1 92/07/30 Copyright Sun Microsystems Inc.";
#endif
	
/*
 * rarpd.c  Reverse-ARP server.
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <netdb.h>
#include <syslog.h>

#include <sys/types.h>

#include <dirent.h>
#include <des_crypt.h>

#include <sys/errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stropts.h>
#include <sys/resource.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/nit_if.h>
#include <net/nit_pf.h>
#include <net/packetfilt.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <rpc/rpc.h>

#include <rpcsvc/ipalloc.h>


#define	BOOTDIR		"/tftpboot"
#define NIT_DEV		"/dev/nit"

#define BUFSIZE		10000

#define	PNP_ON		1
#define PNP_OFF		0
#define PNP_AUTO	-1

#define MAXIFS		16

static int
	received,	/* total packets read */
	bad,		/* packets not understood */
	unknown,	/* unknown ether -> ip address mapping */
	processed,	/* answer known and sent */
	delayed,	/* answer was delayed before sending (UNUSED) */
	/* allocated,	/* address was allocated (CHILD ONLY) */
	weird;		/* unexpected, yet valid */
	/* ignored = received - (bad + unknown + processed) */

static struct ether_addr my_etheraddr;

static int	if_fd;			/* NIT stream with filter */
static struct in_addr my_ipaddr;	/* in network order */
static char	*cmdname;		/* "rarpd" */
static int	debug;			/* enable diagnostics */
static char	*domain;		/* default NIS domain name */

static int	pnp_enable = PNP_AUTO;	/* verified externally */
static u_long	if_netmask;		/* host order */
static u_long	if_ipaddr;		/* host order */
static u_long	if_netnum;		/* host order, with subnet */

static void	get_etheraddr ();	/* get ethernet addr from IF */
static void	get_ifdata ();		/* get IP addr, subnet mask from IF */
static int	get_ipaddr ();		/* lookup IP addr */
static int	ipalloc ();		/* allocate IP address */

static void	rarp_request ();	/* RARP request handler */
static int	sigchld ();		/* child signal handler */
static int	d_fd;
static int	aflag;

extern int	errno;
extern char	*malloc(), *inet_ntoa(), *sprintf(), *strncpy();
extern struct dirent *readdir ();
extern u_long	inet_addr();
extern char	*getopt (), *optarg;
extern int	optind;


/*
 * When the rarp packet is a request (arp_op = REVARP_REQUEST = 3), then
 *	arp_xsha is the ethernet address of the sender of the request;
 *	arp_xspa is undefined;
 *	arp_xtha is the 'target' hardware address, i.e. the sender's address,
 *	arp_xtpa is undefined.
 * The rarp reply (arp_op = REVARP_REPLY = 4) looks like:
 *	arp_xsha is the responder's (our) ethernet address;
 *	arp_xspa is the responder's (our) IP address;
 *	arp_xtha is identical to the request packet;
 *	arp_xtpa is the request's desired IP address.
 *
 */
struct rarp_request {
	struct	ether_header rr_eheader;
	struct ether_arp rr_arpheader;
	u_short rr_len;  /* not part of the protocol; fill in at "read" time */
};

main(argc, argv)
	int argc;
	char **argv;
{
	struct ifreq reqbuf [MAXIFS];
	struct ifreq *ifr;
	struct ifconf ifc;
	struct in_addr addr;
	struct hostent *hp;
	char *hostname;
	int s, n;

	cmdname = argv[0];
	while (argc > 1 && argv[1][0] == '-' && argv[1][2] == '\0') {
		switch (argv[1][1]) {
		case 'a':
			aflag++;
			break;
		case 'd':
			debug++;
			break;
		case 'p':
			if (argc > 2) {
				if (!strcmp (argv [2], "on"))
					pnp_enable = PNP_ON;
				if (!strcmp (argv [2], "off"))
					pnp_enable = PNP_OFF;
				if (!strcmp (argv [2], "auto"))
					pnp_enable = PNP_AUTO;
				argc--, argv++;
			} else
				pnp_enable = PNP_OFF;
			break;
		default:
			usage();
		}
		argc--, argv++;
	}

	if (debug)
		(void) fprintf (stderr, "%s: in debug mode\n", cmdname);

	if (aflag) {
		if (argc > 1) {
			usage ();
			exit (1);
		}

		if ((s = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror ("socket");
			exit (1);
		}
		ifc.ifc_buf = (caddr_t) &reqbuf [0];
		ifc.ifc_len = sizeof (reqbuf);
		if (ioctl (s, SIOCGIFCONF, (char *) &ifc) < 0) {
			perror ("ioctl SIOCGIFCONF");
			exit (1);
		}
		ifr = ifc.ifc_req;
		n = ifc.ifc_len / sizeof (struct ifreq);
		for (; n > 0 ; n--, ifr++) {
			if (ioctl (s, SIOCGIFFLAGS, (char *) ifr) < 0) {
				perror ("ioctl SIOCGIFFLAGS");
				exit (1);
			}
			if ((ifr->ifr_flags & IFF_LOOPBACK) ||
			    !(ifr->ifr_flags & IFF_BROADCAST) ||
			    !(ifr->ifr_flags & IFF_UP) ||
			    (ifr->ifr_flags & IFF_NOARP) ||
			    (ifr->ifr_flags & IFF_POINTOPOINT))
				continue;
			addr.s_addr = 0;
			do_rarp(ifr->ifr_name,  addr);
		}

	} else 
		switch (argc) {
		case 2:
			/* interface name only specified */
			addr.s_addr = 0;
			do_rarp(argv [1], addr);
			break;

		case 3:
			/* Interface name and hostname or IP address given */
			hostname = argv[2];

			if ((hp = gethostbyname(hostname)) == 
			    (struct hostent *) NULL) {
				if ((addr.s_addr = inet_addr (hostname)) 
				    == (u_long) -1) {
					(void) fprintf(stderr, 
						"%s: cannot get IP address for %s\n",
						cmdname, hostname);
					exit(2);
				}
			} else {
				if (hp->h_length != sizeof (addr)) {
					(void) fprintf(stderr, 
						"%s: cannot find host entry for %s\n",
						cmdname, hostname);
					exit(2);
				} else
					bcopy((char *) hp->h_addr, 
					      (char *) &addr.s_addr,
					      sizeof (addr));
			}
			do_rarp(argv [1], addr);
			break;
			
		default:
			usage();
		}
	exit(0);
	/* NOTREACHED */
}

do_rarp(device, addr)
char *device;
struct in_addr addr;
{
	register int i;
	int delay_fd[2];
	char *buf, *cause;
	struct rarp_request ans;
	char host[256];
	struct timeval now, then;

	/*
	 * Open interface and pull out goodies:
	 *   ieee802 (48 bit) address, IP address, IP subnet mask
	 */
	if_fd = rarp_open (device, htons(ETHERTYPE_REVARP));
	if (if_fd < 0)
		exit(5);

	get_etheraddr(if_fd, device, &my_etheraddr);
	get_ifdata (device, &if_ipaddr, &if_netmask);

	/* 
	 * Use IP address from hostname on the commandline, if the
	 * user supplied it.  Otherwise, use IP address from the
	 * interface.
	 */
	if (addr.s_addr == 0) {
		if_netnum = if_ipaddr & if_netmask;
		if_ipaddr = htonl (if_ipaddr);
		bcopy ((char *)&if_ipaddr, (char *) &my_ipaddr, 
		       sizeof (my_ipaddr));
		if_ipaddr = ntohl (if_ipaddr);
	} else {
		bcopy ((char *) &addr.s_addr, (char *) &my_ipaddr, 
		       sizeof (my_ipaddr));
		bcopy ((char *) &addr.s_addr, (char *) &if_netnum, 
		       sizeof (if_netnum));
		if_netnum &= if_netmask;
	}

	if (debug)
		(void) fprintf (stderr, 
			 "%s: starting rarp service on interface %s, address %s\n",
			 cmdname, device, inet_ntoa (my_ipaddr));
		
	if (!debug) {
		/*
		 * Background
		 */
		while (if_fd < 3) 
			if_fd = dup(if_fd);
		switch (fork ()) {
		case -1:			/* error */
			perror ("rarpd:  fork");
			exit (6);
		case 0:				/* child */
			break;
		default:			/* parent */
			return;
		}
		for (i = 0; i < 3; i++) {
			(void) close(i);
		}
		(void) open("/", O_RDONLY, 0);
		(void) dup2(0, 1);
		(void) dup2(0, 2);
		/*
		 * Detach terminal
		 */
		if ((i = open("/dev/tty", O_RDWR, 0)) >= 0) {
			(void) ioctl(i, TIOCNOTTY, 0);
			(void) close(i);
		}
	}

	(void) yp_get_default_domain (&domain);
	(void) signal (SIGCHLD, sigchld);
	(void) openlog (cmdname, LOG_PID, LOG_DAEMON);

	/*
	 * Fork off a delayed responder which uses a pipe.
	 */
	 if (pipe(delay_fd)) {
		perror("rarpd: pipe");
		exit(7);
	}

	 switch (fork()) {

	 case -1:
		perror("rarpd: delayed fork");
		exit(8);
		break;

	 case 0:
		/* child reads the pipe and sends responses */
		(void) close(delay_fd[1]);  /* no writing */
		d_fd = delay_fd[0];
		buf = host;
		i = sizeof (ans) - sizeof (ans.rr_len);
		for (;;) {
			if ((read(d_fd, (char *) &then, sizeof (then)) != 
			     sizeof (then)) || 
			    (read(d_fd, buf, i) != i)) {
				perror("delayed rarpd: read");
				exit(9);
			}
			received++;
			/*
			 * It is our job to never send a (delayed) reply
			 * in the same second of time that it was created;
			 * rather wait up to three seconds before responding.
			 */
			(void) gettimeofday(&now, (struct timezone *) NULL);
			if (now.tv_sec == then.tv_sec) {
				sleep(3);
				delayed++;
			}
			if (rarp_write(if_fd, buf, i) != i) {
				perror("delayed rarpd: rarp_write");
				exit(10);
				break;  /* eliminate warning msgs */
			};
		}
		/*NOTREACHED*/
		exit(0);
		break;

	 default:
		/* parent does most processing and maybe writes to the child */
		(void) close(delay_fd[0]);  /* no reading */
		d_fd = delay_fd[1];
		break;
	 }

	/*
	 * read RARP packets and respond to them.
	 *
	 * This server adapts to heavy loads by ignoring requests;
	 * this is accomplished by processing only the last packet
	 * of a multi-packet read call.
	 *
	 * XXX:	At least for the moment, the incoming stream does
	 *	no buffering, so that incoming packets are received
	 *	one at a time.  Thus, the above-mentioned load handling
	 *	strategy is currently inapplicable.
	 */
	buf = malloc(BUFSIZE);
	for (;;) {
		if ((i = read(if_fd, buf, BUFSIZE)) < 0) {
			perror("rarpd: read");
			exit(11);
		}
		if (debug > 1)
			(void) fprintf(stderr, "%s: read read %d\n", cmdname,
				       i);
		bcopy (buf, (char *) &ans, sizeof (ans));
		ans.rr_len = i;
		/*
		 * Sanity checks ... set i to sizeof an rarp packet
		 */
		i = sizeof (ans) - sizeof (ans.rr_len);
		cause = 0;

		if (ans.rr_len < i)
			cause="rr_len";
		else if (ans.rr_eheader.ether_type != ntohs(ETHERTYPE_REVARP))
			cause="type";
		else if (ans.rr_arpheader.arp_hrd != htons(ARPHRD_ETHER))
			cause="hrd";
		else if (ans.rr_arpheader.arp_pro != htons(ETHERTYPE_IP))
			cause="pro";
		else if (ans.rr_arpheader.arp_hln != 6)
			cause="hln";
		else if (ans.rr_arpheader.arp_pln != 4)
			cause="pln";
		if (cause) {
			if (debug)
				(void) fprintf(stderr,
				    "%s: sanity check failed; cause: %s\n",
				    cmdname, cause);
			continue;
		}

		received++;
		switch (ntohs(ans.rr_arpheader.arp_op)) {
		case REVARP_REQUEST:
			rarp_request (&ans, "RARP_REQUEST");
			break;
		case ARPOP_REQUEST:
			if (debug > 1)
				(void) fprintf(stderr, "%s: ARPOP_REQUEST\n",
					       cmdname);
			arp_request(&ans);
			break;
		case REVARP_REPLY:
		case ARPOP_REPLY:
			break;
		default:
			if (debug)
				(void) fprintf (stderr,
						"%s: unknown opcode 0x%xd\n", 
						cmdname,
						ans.rr_arpheader.arp_op);
			bad++;
			break;
		}
	}
}


/* 
 * Boolean:  do we allocate addresses at all?  Can be disabled by
 * commandline flag or network policy.
 */

static int
will_pnp ()
{
	char *val;
	int vallen;
	static char pnpval [] = "pnp";

	switch (pnp_enable) {
		
	case PNP_ON:
		return (1);
		break;

	case PNP_OFF:
		return (0);
		break;

	case PNP_AUTO:
		if (!domain || yp_match (domain, "policies",
					 pnpval, sizeof (pnpval) -1,
					 &val, &vallen) != 0)
			return (0);
		(void) free (val);
		val [vallen] = '\0';
		return (strcmp (val, "restricted") != 0);
		break;

	default:
		break;
	}
	return (0);
}


/* 
 * Reverse address determination and allocation code.
 */
static void
rarp_request (r, what)
	struct rarp_request *r;
	char *what;
{
	u_long tpa;
	struct timeval now;
	int i;

	if (debug)
		(void) fprintf (stderr, "%s: %s for %s\n",
			 cmdname, what, ether_ntoa (&r->rr_arpheader.arp_tha));
    
	/* 
	 * third party lookups are rare and wonderful
	 */
	if (bcmp ((char *)&r->rr_arpheader.arp_sha, 
		  (char *)&r->rr_arpheader.arp_tha, 6) || 
	    bcmp((char *)&r->rr_arpheader.arp_sha, 
		 (char *)&r->rr_eheader.ether_shost, 6)) {
		if (debug)
			(void) fprintf(stderr, 
				       "%s: weird (3rd party lookup)\n", 
				       cmdname);
		weird++;
	}

	/* fill in given parts of reply packet */
	r->rr_eheader.ether_dhost = r->rr_arpheader.arp_sha;
	bcopy ((char *) &my_etheraddr, (char *)&r->rr_arpheader.arp_sha,
	       sizeof (r->rr_arpheader.arp_sha));
	bcopy ((char *) &my_ipaddr, (char *)r->rr_arpheader.arp_spa,
	       sizeof (r->rr_arpheader.arp_spa));

	/*
	 * If a good address is stored in our lookup tables, return it
	 * immediately or after a delay.  Store it our kernel's ARP cache.
	 */
	if (get_ipaddr ((struct ether_addr *)&r->rr_arpheader.arp_tha,
			r->rr_arpheader.arp_tpa) == 0) {
		add_arp ((char *)r->rr_arpheader.arp_tpa, 
			&r->rr_arpheader.arp_tha);
		r->rr_arpheader.arp_op = htons (REVARP_REPLY);

		if (debug) {
			struct in_addr addr;

			bcopy (r->rr_arpheader.arp_tpa, (char *) &addr,
			       sizeof (addr));
			(void) fprintf (stderr, "%s: good lookup, maps to %s\n",
					cmdname, 
					inet_ntoa (addr));
		}

		/*
		 * If this is diskless and we're not its bootserver, let the
		 * bootserver reply first.  Load dependent performance hack.
		 */
		(void) bcopy ((char *) r->rr_arpheader.arp_tpa, (char *)&tpa, 
			      sizeof (u_long));
		i = sizeof (*r) - sizeof (r->rr_len);
		if (!mightboot (ntohl (tpa))) {
			(void) gettimeofday(&now, (struct timezone *) NULL);
			if ((write(d_fd, (char *) &now, sizeof (now)) != 
			     sizeof (now)) ||
			    (write(d_fd, (char *) r, i)) != i) {
				perror("rarpd: delayed write");
				exit(13);
			}
			if (debug)
				(void) fprintf (stderr, 
						"%s: delayed reply sent.\n",
						cmdname);
			delayed++;
		} else {
			if (rarp_write(if_fd, (char *) r, i) != i)
				syslog (LOG_ERR, "Bad lookup write:  %m");
			if (debug)
				(void) fprintf (stderr, 
						"%s: immediate reply sent.\n",
						cmdname);
		}
		processed++;
		return;
	} else
		unknown++;

	if (!will_pnp ()) {
		return;
	}

	/* 
	 * Address allocation may take some time ...
	 */
	if (!debug)
		switch (fork ()) {
		case -1:			/* error */
			syslog (LOG_ERR, 
				"Fork failed, no address allocation: %m");
		default:			/* parent */
			return;
		case 0:				/* child */
			break;
		}

	if (ipalloc ((struct ether_addr *)&r->rr_arpheader.arp_tha,
		     r->rr_arpheader.arp_tpa) == 0) {
		r->rr_arpheader.arp_op = htons (REVARP_REPLY);
	} else {
		r = 0;
	}
	
	/* 
	 * Write out any reply packet, save the new address, and (child) exit.
	 */
	i = sizeof (*r) - sizeof (r->rr_len);
	if (r && rarp_write(if_fd, (char *) r, i) != i)
		syslog (LOG_ERR, "Bad write:  %m");
	if (r)
		add_arp ((char *)r->rr_arpheader.arp_tpa, 
			&r->rr_arpheader.arp_tha);
	exit (0);
}

/*
 * Catch children that are handling pnp.
 */
static int
sigchld ()
{
	union wait w;

	while (wait3 (&w, WNOHANG, (struct rusage *)0)  > 0)
		continue;
	(void) signal (SIGCHLD, sigchld);
}

/*
 * down loads regular ARP entries to the kernel.
 * NB: Load even if it already exists
 */
static
add_arp(ipap, eap)
	char *ipap;  /* IP address pointer */
	struct ether_addr *eap;
{
	struct arpreq ar;
	struct sockaddr_in *sin;
	int s;

	/*
	 * Common part of query or set
	 */
	bzero((caddr_t)&ar, sizeof (ar));
	ar.arp_pa.sa_family = AF_INET;
	sin = (struct sockaddr_in *)&ar.arp_pa;
	bcopy(ipap, (char *)&sin->sin_addr, sizeof (struct in_addr));
	s = socket(AF_INET, SOCK_DGRAM, 0);

	/*
	 * Set the entry
	 */
	bcopy((char *) eap, ar.arp_ha.sa_data, sizeof (*eap));
	ar.arp_flags = 0;
        (void) ioctl(s, SIOCDARP, (caddr_t)&ar);
	(void) ioctl(s, SIOCSARP, (caddr_t)&ar);
	(void) close(s);
}

/*
 * The RARP spec says we must be able to process ARP requests, even through
 * the packet type is RARP.  Let's hope this feature is not heavily used.
 */
static
arp_request(a)
	struct rarp_request *a;
{
	if (bcmp((char *) &my_ipaddr, (char *)a->rr_arpheader.arp_tpa,
		  sizeof (my_ipaddr))) {
		return;
	}
	a->rr_eheader.ether_dhost = a->rr_eheader.ether_shost;
	a->rr_arpheader.arp_op = ARPOP_REPLY;
	bcopy((char *) &my_etheraddr, (char *) &a->rr_arpheader.arp_sha,
	      sizeof (a->rr_arpheader.arp_sha));
	bcopy((char *) &my_ipaddr, (char *) a->rr_arpheader.arp_spa,
	      sizeof (a->rr_arpheader.arp_spa));
	bcopy((char *) &my_etheraddr, (char *) &a->rr_arpheader.arp_tha,
	      sizeof (a->rr_arpheader.arp_tha));
	add_arp ((char *)a->rr_arpheader.arp_tpa, &a->rr_eheader.ether_dhost);
	(void) rarp_write(if_fd, (char *) a, sizeof (*a) - sizeof(a->rr_len));
}

usage()
{
	(void) fprintf(stderr,
		       "Usage: %s [-d] [-p] device [hostname]\n", cmdname);
	exit(1);
}

/*
 * Open the NIT device, and establish the following characteristics.
 * 1)	Bind it to device.
 * 2)	Push an instance of the packet-filtering module on it that passes
 *	through only packets whose Ethernet type field is type.
 * 3)	Set it into message-discard mode.
 *
 * Return the resulting descriptor.
 */
static int
rarp_open(device, type)
	char *device;
	u_short type;
{
	struct strioctl si;
	struct ifreq ifr;
	struct ether_header eh;		/* used only for offset values */
	struct packetfilt pf;
	register u_short *fwp;
	register int fd;

	if ((fd = open(NIT_DEV, O_RDWR)) < 0) {
		perror("nit open");
		return (-1);
	}

	if (ioctl(fd, I_SRDOPT, (char *)RMSGD) < 0) {
		perror("ioctl (I_SRDOPT)");
		return (-2);
	}

	/*
	 * Push the packet filtering module.
	 */
	if (ioctl(fd, I_PUSH, "pf") < 0) {
		perror("ioctl (I_PUSH \"pf\")");
		return (-3);
	}

	/*
	 * Set up filter.
	 */
	fwp = &pf.Pf_Filter[0];
	*fwp++ = ENF_PUSHWORD +	
		((u_int) &eh.ether_type - (u_int) &eh.ether_dhost) / 
			sizeof (u_short);
	*fwp++ = ENF_PUSHLIT | ENF_EQ;
	*fwp++ = type;			/* arg is in network byte order */
	pf.Pf_FilterLen = fwp - &pf.Pf_Filter[0];
	pf.Pf_Priority = 5;		/* unimportant, so long as > 2 */

	/*
	 * Package up the NIOCSETF ioctl and issue it.
	 */
	si.ic_cmd = NIOCSETF;
	si.ic_timout = INFTIM;
	si.ic_len = sizeof pf;
	si.ic_dp = (char *)&pf;

	if (ioctl(fd, I_STR, (char *)&si) < 0) {
		perror("ioctl (I_STR: NIOCSETF)");
		return (-4);
	}

	/*
	 * We defer the bind until after we've pushed the filter
	 * to prevent our being flooded with extraneous packets.
	 */
	(void) strncpy(ifr.ifr_name, device, sizeof ifr.ifr_name - 1);
	ifr.ifr_name[sizeof ifr.ifr_name - 1] = '\0';
	/*
	 * Package so the stream head can cope with it.
	 */
	si.ic_cmd = NIOCBIND;
	si.ic_len = sizeof ifr;
	si.ic_dp = (char *)&ifr;

	if (ioctl(fd, I_STR, (char *)&si) < 0) {
		perror("ioctl (I_STR: NIOCBIND)");
		return (-5);
	}

	return (fd);
}

static int
rarp_write(fd, buf, len)
	int fd, len;
	char *buf;
{
	struct sockaddr sa;
	struct strbuf cbuf, dbuf;
	/*
	 * Sleaze!  Offset used ambiguously, taking advantage of fact that
	 * sizeof sa.sa_data == sizeof (struct ether_header).
	 */
	register int offset = sizeof(sa.sa_data);

	sa.sa_family = AF_UNSPEC;
	bcopy(buf, sa.sa_data, offset);

	/*
	 * Set up NIT (write-side) protocol info.
	 */
	cbuf.maxlen = cbuf.len = sizeof sa;
	cbuf.buf = (char *)&sa;

	/*
	 * The interface output routines will paste the
	 * ether header back onto the front of the message.
	 */
	dbuf.maxlen = dbuf.len = len - offset;
	dbuf.buf = buf + offset;

	if (putmsg(fd, &cbuf, &dbuf, 0) < 0) {
		perror("rarp_write: putmsg");
		return (-1);
	}

	return (len);
}

/*
 * Obtain the Ethernet address of the interface named
 * by dev (and bound to fd), assigning it to *ap.
 */
static void
get_etheraddr(fd, dev, ap)
	int			fd;
	char			*dev;
	struct ether_addr	*ap;
{
	struct strioctl	si;
	struct ifreq	ifr;

	(void) strncpy(ifr.ifr_name, dev, sizeof ifr.ifr_name);

	si.ic_timout = INFTIM;
	si.ic_cmd = SIOCGIFADDR;
	si.ic_len = sizeof ifr;
	si.ic_dp = (char *)&ifr;

	if (ioctl(fd, I_STR, (char *)&si) < 0) {
		(void) fprintf(stderr, "%s: ", cmdname);
		perror("ioctl (I_STR: SIOCGIFADDR)");
		exit(15);
	}

	*ap = *(struct ether_addr *) &ifr.ifr_addr.sa_data[0];
}

 
/*
 * See if we have a TFTP boot file for this guy. Filenames in TFTP 
 * boot requests are of the form <ipaddr> for Sun-3's and of the form
 * <ipaddr>.<arch> for all other architectures.  Since we don't know
 * the client's architecture, either format will do.
 */
int
mightboot (ipa)
	u_long ipa;
{
	char buf [10];
	DIR *dirp;
	struct dirent *dp;

	(void) sprintf (buf, "%08X", ipa);
	if (!(dirp = opendir (BOOTDIR)))
		return 0;

	while ((dp = readdir (dirp)) != (struct dirent *) 0) {
		if (dp->d_namlen < 8 || strncmp (dp->d_name, buf, 8) != 0)
			continue;
		if (dp->d_namlen != 8 && dp->d_name [8] != '.')
			continue;
		break;
	}
	
	(void) closedir (dirp);
	return (int) dp;
}

/* 
 * this and get_etheraddr() should have more in common!
 */

static void
get_ifdata (dev, ipp, maskp)
	char *dev;
	u_long *ipp, *maskp;
{
	struct ifreq ifr;
	struct sockaddr_in *sin = (struct sockaddr_in *) &ifr.ifr_addr;
	int s;

	if ((s = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror ("get dgram socket");
		exit (16);
	}

	(void) strncpy (ifr.ifr_name, dev, sizeof ifr.ifr_name);

	if (ioctl (s, SIOCGIFADDR, (caddr_t) &ifr) < 0) {
		perror ("SIOCGIFADDR");
		exit (17);
	}
	*ipp = ntohl (sin->sin_addr.s_addr);
	
	if (debug > 1)
		(void) fprintf (stderr, "Interface '%s' address %s\n",
				dev, inet_ntoa (sin->sin_addr));

	if (ioctl (s, SIOCGIFNETMASK, (caddr_t) &ifr) < 0) {
		perror ("SIOCGIFNETMASK");
		exit (18);
	}

	*maskp = ntohl (sin->sin_addr.s_addr);

	if (debug > 1)
		(void) fprintf (stderr, "Interface '%s' subnet mask %s\n",
				dev, inet_ntoa (sin->sin_addr));
	(void) close (s);
}

static
have_encryption()
{
	static int status = -1;
	char key[8];
	char data[8];

	if (status < 0) {
		bzero(key, sizeof(key));
		status = ecb_crypt(key, data, sizeof(data), 
				   DES_ENCRYPT|DES_SW);
	}
	return (status == 0);
}

/*
 * Returns a TCP client handle to the ipalloc server.
 * This is always the master of the "hosts.byaddr"
 * map, and may change over time.  The handle returned
 * should be destroyed after use.
 */
#define	WAIT_MINUTES	5		/* max time to alloc addr */

static CLIENT *
ipalloc_clnt ()
{
	char 		*master = (char *) 0;
	CLIENT		*client = (CLIENT *) 0;
	struct sockaddr_in	sin;
	char		servername [MAXNETNAMELEN + 1];
	static struct timeval	timeout = { WAIT_MINUTES * 60, 0};

	if (!domain)
	return 0;
	else if (yp_master (domain, "hosts.byaddr", &master) != 0) {
		if (debug)
			(void) fprintf (stderr,
					"Can't find NIS master of hosts.byaddr in domain %s\n",
					domain);
		goto cleanup;
	}

	client = clnt_create (master, IPALLOC_PROG, IPALLOC_VERS, "tcp");
	
	if (!client) {
		if (debug)
			(void) fprintf (stderr, "clnt_create failed\n");
		goto cleanup;
	}

	if (have_encryption ()) {
		(void) host2netname (servername, master, domain);
		clnt_control (client, CLGET_SERVER_ADDR, &sin);
		client->cl_auth = authdes_create (servername, 
						  WAIT_MINUTES * 60,
						  (struct sockaddr *) &sin, 
						  (des_block *) 0);
	}
	clnt_control (client, CLSET_TIMEOUT, &timeout);

 cleanup:
	(void) free (master);
	
	return client;
}

/* 
 * allocate an IP address if possible.
 */
static
ipalloc (enet, dest)
	struct ether_addr	*enet;
	u_char *dest;		/* return, in network order */
{
	CLIENT		*client = ipalloc_clnt ();
	ip_alloc_arg	arg;
	ip_alloc_res	*res;
	int		retval = -1;
	struct in_addr	addr;
	int		fd;
	
	if (!client)
		return retval;
	else
		retval = 1;

	(void) bcopy ((char *) enet, arg.etheraddr, sizeof (arg.etheraddr));
	arg.netnum = if_netnum;
	arg.subnetmask = if_netmask;
	
	if (!(res = ip_alloc_2 (&arg, client)) && debug)
		clnt_perror (client, "ipalloc");
	else if (res) {
		if (res->status) {
			if (debug)
				(void) fprintf (stderr, 
						"%s: ipalloc failed:  %d\n", 
						cmdname, res->status);
		} else {
			addr.s_addr = htonl (res->ip_alloc_res_u.ipaddr);
			if (debug)
				(void) fprintf (stderr, "%s: ipalloc -> %s\n",
						cmdname, inet_ntoa (addr));
			bcopy ((char *) &addr, (char *) dest, sizeof (addr));
			retval = 0;
		}
	}

	bcopy ((char *) client->cl_private, (char *) &fd, sizeof (fd));
	(void) close (fd);	/* yeech */
	clnt_destroy (client);
	return retval;
}

static int
get_ipaddr (enetp, ipp)
	struct ether_addr *enetp;
	u_char *ipp;
{
	char host [256];
	struct hostent *hp;
	struct in_addr addr;

	/* if no data stored, immediate failure */
	if (ether_ntohost(host, enetp) != 0
	    || !(hp = gethostbyname (host))
	    || hp->h_addrtype != AF_INET
	    || hp->h_length != sizeof (struct in_addr)) {
		if (debug) 
			(void) fprintf (stderr, 
					"%s: could not map hardware address to IP address.\n",
					cmdname);
		return 1;
	}

	/* if stored addr is on the wrong net, ditto */
	bcopy (hp->h_addr, (char *) &addr, sizeof (addr));
	if ((ntohl (addr.s_addr) & if_netmask) != if_netnum) {
		if (debug) 
			(void) fprintf (stderr, 
				 "%s: IP address is not on this net.\n",
				 cmdname);
		return 1;
	}

	/* else return the stored address */
	bcopy (hp->h_addr, (char *) ipp, sizeof (struct in_addr));
	return 0;
}
