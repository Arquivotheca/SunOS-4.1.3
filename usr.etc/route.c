/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char *sccsid = "@(#)route.c 1.1 92/07/30 SMI"; /* from UCB 5.6 6/5/86 */
#endif not lint

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <signal.h>
#include <setjmp.h>

#include <net/route.h>
#include <netinet/in.h>
#ifdef XNS
#include <netns/ns.h>
#endif XNS

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>

struct	rtentry route;
int	s;
int	forcehost, forcenet, doflush, nflag;
struct	sockaddr_in sin = { AF_INET };
struct	in_addr inet_makeaddr();
char	*malloc();

main(argc, argv)
	int argc;
	char *argv[];
{

	if (argc < 2)
		printf("usage: route [ -n ] [ -f ] [ cmd [ net | host ] args ]\n"),
		exit(1);
	s = socket(AF_INET, SOCK_RAW, 0);
	if (s < 0) {
		perror("route: socket");
		exit(1);
	}
	argc--, argv++;
	for (; argc >  0 && argv[0][0] == '-'; argc--, argv++) {
		for (argv[0]++; *argv[0]; argv[0]++)
			switch (*argv[0]) {
			case 'f':
				doflush++;
				break;
			case 'n':
				nflag++;
				break;
			}
	}
	if (doflush)
		flushroutes();
	if (argc > 0) {
		if (strcmp(*argv, "add") == 0)
			newroute(argc, argv);
		else if (strcmp(*argv, "delete") == 0)
			newroute(argc, argv);
		else if (strcmp(*argv, "change") == 0)
			changeroute(argc-1, argv+1);
		else
			printf("%s: huh?\n", *argv);
	}
	exit(0);
	/* NOTREACHED */
}

/*
 * Purge all entries in the routing tables not
 * associated with network interfaces.
 */
#include <nlist.h>

struct nlist nl[] = {
#define	N_RTHOST	0
	{ "_rthost" },
#define	N_RTNET		1
	{ "_rtnet" },
#define N_RTHASHSIZE	2
	{ "_rthashsize" },
	"",
};

flushroutes()
{
	struct mbuf mb;
	register struct rtentry *rt;
	register struct mbuf *m;
	struct mbuf **routehash;
	int rthashsize, i, doinghost = 1, kmem;
	char *routename(), *netname();

	nlist("/vmunix", nl);
	if (nl[N_RTHOST].n_value == 0) {
		printf("route: \"rthost\", symbol not in namelist\n");
		exit(1);
	}
	if (nl[N_RTNET].n_value == 0) {
		printf("route: \"rtnet\", symbol not in namelist\n");
		exit(1);
	}
	if (nl[N_RTHASHSIZE].n_value == 0) {
		printf("route: \"rthashsize\", symbol not in namelist\n");
		exit(1);
	}
	kmem = open("/dev/kmem", 0);
	if (kmem < 0) {
		perror("route: /dev/kmem");
		exit(1);
	}
	lseek(kmem, nl[N_RTHASHSIZE].n_value, 0);
	read(kmem, &rthashsize, sizeof (rthashsize));
	routehash = (struct mbuf **)malloc(rthashsize*sizeof (struct mbuf *));

	lseek(kmem, nl[N_RTHOST].n_value, 0);
	read(kmem, routehash, rthashsize*sizeof (struct mbuf *));
	printf("Flushing routing tables:\n");
again:
	for (i = 0; i < rthashsize; i++) {
		if (routehash[i] == 0)
			continue;
		m = routehash[i];
		while (m) {
			lseek(kmem, m, 0);
			read(kmem, &mb, sizeof (mb));
			rt = mtod(&mb, struct rtentry *);
			if (rt->rt_flags & RTF_GATEWAY) {
				printf("%-20.20s ", doinghost ?
				    routename(&rt->rt_dst) :
				    netname(&rt->rt_dst));
				printf("%-20.20s ", routename(&rt->rt_gateway));
				if (ioctl(s, SIOCDELRT, (caddr_t)rt) < 0)
					error("delete");
				else
					printf("done\n");
			}
			m = mb.m_next;
		}
	}
	if (doinghost) {
		lseek(kmem, nl[N_RTNET].n_value, 0);
		read(kmem, routehash, rthashsize*sizeof (struct mbuf *));
		doinghost = 0;
		goto again;
	}
	close(kmem);
	free(routehash);
}

static jmp_buf NameTimeout;
int nametime = 20;		/* seconds to wait for name server */

nametimeout()  {longjmp(NameTimeout, 1);}

char *
routename(sa)
	struct sockaddr *sa;
{
	register char *cp;
	static char line[50];
	struct hostent *hp;
	static char domain[MAXHOSTNAMELEN + 1];
	static int first = 1;
	char *index();
	char *ns_print();

	if (first) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = index(domain, '.')))
			(void) strcpy(domain, cp + 1);
		else
			domain[0] = 0;
	}
	switch (sa->sa_family) {

	case AF_INET:
	    {	struct in_addr in;
		in = ((struct sockaddr_in *)sa)->sin_addr;

		cp = 0;
		if (in.s_addr == INADDR_ANY)
			cp = "default";
		signal(SIGALRM, nametimeout);
		alarm(nametime);
		if (setjmp(NameTimeout) == 0 && cp == 0 && !nflag) {
			hp = gethostbyaddr(&in, sizeof (struct in_addr),
				AF_INET);
			if (hp) {
				if ((cp = index(hp->h_name, '.')) &&
				    !strcmp(cp + 1, domain))
					*cp = 0;
				cp = hp->h_name;
			}
		}
		alarm(0);
		if (cp)
			strcpy(line, cp);
		else {
			sprintf(line, "%s", inet_ntoa(in) );
		}
		break;
	    }

# ifdef XNS
	case AF_NS:
		return (ns_print((struct sockaddr_ns *)sa));
# endif XNS

	default:
	    {	u_short *s = (u_short *)sa->sa_data;

		sprintf(line, "af %d: %x %x %x %x %x %x %x", sa->sa_family,
			s[0], s[1], s[2], s[3], s[4], s[5], s[6]);
		break;
	    }
	}
	return (line);
}

/*
 * Return the name of the network whose address is given.
 * The address is assumed to be that of a net or subnet, not a host.
 */
char *
netname(sa)
	struct sockaddr *sa;
{
	char *cp = 0;
	static char line[50];
	struct netent *np = 0;
	u_long net, mask;
	register i;
	int subnetshift;

	switch (sa->sa_family) {

	case AF_INET:
	    {	struct in_addr in;
		in = ((struct sockaddr_in *)sa)->sin_addr;

		in.s_addr = ntohl(in.s_addr);
		if (in.s_addr == 0)
			cp = "default";
		else if (!nflag) {
			if (IN_CLASSA(i)) {
				mask = IN_CLASSA_NET;
				subnetshift = 8;
			} else if (IN_CLASSB(i)) {
				mask = IN_CLASSB_NET;
				subnetshift = 8;
			} else {
				mask = IN_CLASSC_NET;
				subnetshift = 4;
			}
			/*
			 * If there are more bits than the standard mask
			 * would suggest, subnets must be in use.
			 * Guess at the subnet mask, assuming reasonable
			 * width subnet fields.
			 */
			while (in.s_addr &~ mask)
				mask = (long)mask >> subnetshift;
			net = in.s_addr & mask;
			while ((mask & 1) == 0)
				mask >>= 1, net >>= 1;
			np = getnetbyaddr(net, AF_INET);
			if (np)
				cp = np->n_name;
		}
		if (cp)
			strcpy(line, cp);
#define C(x)	((x) & 0xff)
		else if ((in.s_addr & 0xffffff) == 0)
			sprintf(line, "%u", C(in.s_addr >> 24));
		else if ((in.s_addr & 0xffff) == 0)
			sprintf(line, "%u.%u", C(in.s_addr >> 24),
			    C(in.s_addr >> 16));
		else if ((in.s_addr & 0xff) == 0)
			sprintf(line, "%u.%u.%u", C(in.s_addr >> 24),
			    C(in.s_addr >> 16), C(in.s_addr >> 8));
		else
			sprintf(line, "%u.%u.%u.%u", C(in.s_addr >> 24),
			    C(in.s_addr >> 16), C(in.s_addr >> 8),
			    C(in.s_addr));
		break;
	    }

# ifdef XNS
	case AF_NS:
		return (ns_print((struct sockaddr_ns *)sa));
		break;
# endif XNS

	default:
	    {	u_short *s = (u_short *)sa->sa_data;

		sprintf(line, "af %d: %x %x %x %x %x %x %x", sa->sa_family,
			s[0], s[1], s[2], s[3], s[4], s[5], s[6]);
		break;
	    }
	}
	return (line);
}

newroute(argc, argv)
	int argc;
	char *argv[];
{
	struct sockaddr_in *sin;
	char *cmd, *dest, *gateway;
	int ishost, metric = 0, ret, attempts, oerrno;
	struct hostent *hp;
	extern int errno;

	cmd = argv[0];
	if (argc < 3) {
                printf("usage: %s destination gateway ...\n", cmd);
                return;
        }
	if ((strcmp(argv[1], "host")) == 0) {
		forcehost++;
		argc--, argv++;
	} else if ((strcmp(argv[1], "net")) == 0) {
		forcenet++;
		argc--, argv++;
	}
	if (*cmd == 'a') {
		if (argc != 4) {
			printf("usage: %s destination gateway metric\n", cmd);
			printf("(metric of 0 if gateway is this host)\n");
			return;
		}
		metric = atoi(argv[3]);
	} else {
		if (argc < 3) {
			printf("usage: %s destination gateway\n", cmd);
			return;
		}
	}
	sin = (struct sockaddr_in *)&route.rt_dst;
	ishost = getaddr(argv[1], &route.rt_dst, &hp, &dest, forcenet);
	if (forcehost)
		ishost = 1;
	if (forcenet)
		ishost = 0;
	sin = (struct sockaddr_in *)&route.rt_gateway;
	(void) getaddr(argv[2], &route.rt_gateway, &hp, &gateway, 0);
	route.rt_flags = RTF_UP;
	if (ishost)
		route.rt_flags |= RTF_HOST;
	if (metric > 0)
		route.rt_flags |= RTF_GATEWAY;
	for (attempts = 1; ; attempts++) {
		errno = 0;
		if ((ret = ioctl(s, *cmd == 'a' ? SIOCADDRT : SIOCDELRT,
		     (caddr_t)&route)) == 0)
			break;
		if (errno != ENETUNREACH && errno != ESRCH)
			break;
		if (hp && hp->h_addr_list[1]) {
			hp->h_addr_list++;
			bcopy(hp->h_addr_list[0], (caddr_t)&sin->sin_addr,
			    hp->h_length);
		} else
			break;
	}
	oerrno = errno;
	printf("%s %s %s: gateway %s", cmd, ishost? "host" : "net",
		dest, gateway);
	if (attempts > 1 && ret == 0)
	    printf(" (%s)",
		inet_ntoa(((struct sockaddr_in *)&route.rt_gateway)->sin_addr));
	if (ret == 0)
		printf("\n");
	else {
		printf(": ");
		fflush(stdout);
		errno = oerrno;
		error(0);
	}
}

changeroute(argc, argv)
	int argc;
	char *argv[];
{
	printf("not supported\n");
}

error(cmd)
	char *cmd;
{

	if (errno == ESRCH)
		fprintf(stderr, "not in table\n");
	else if (errno == EBUSY)
		fprintf(stderr, "entry in use\n");
	else if (errno == ENOBUFS)
		fprintf(stderr, "routing table overflow\n");
	else
		perror(cmd);
}

char *
savestr(s)
	char *s;
{
	char *sav;

	sav = malloc(strlen(s) + 1);
	if (sav == NULL) {
		fprintf("route: out of memory\n");
		exit(1);
	}
	strcpy(sav, s);
	return (sav);
}

/*
 * Interpret an argument as a network address of some kind,
 * returning 1 if a host address, 0 if a network address.
 */
getaddr(s, sin, hpp, name, isnet)
	char *s;
	struct sockaddr_in *sin;
	struct hostent **hpp;
	char **name;
	int isnet;
{
	struct hostent *hp;
	struct netent *np;
	u_long val;

	*hpp = 0;
	if (strcmp(s, "default") == 0) {
		sin->sin_family = AF_INET;
		sin->sin_addr = inet_makeaddr(0, INADDR_ANY);
		*name = "default";
		return(0);
	}
	sin->sin_family = AF_INET;
	val = inet_addr(s);
	if (val != -1) {
		sin->sin_addr.s_addr = val;
		*name = s;
		return(inet_lnaof(sin->sin_addr) != INADDR_ANY);
	}
	if (isnet == 0) {
		val = inet_addr(s);
		if (val != -1) {
			sin->sin_addr.s_addr = val;
			*name = s;
			return(inet_lnaof(sin->sin_addr) != INADDR_ANY);
		}
	}
	val = inet_network(s);
	if (val != -1) {
		sin->sin_addr = inet_makeaddr(val, INADDR_ANY);
		*name = s;
		return(0);
	}
	np = getnetbyname(s);
	if (np) {
		sin->sin_family = np->n_addrtype;
		sin->sin_addr = inet_makeaddr(np->n_net, INADDR_ANY);
		*name = savestr(np->n_name);
		return(0);
	}
	hp = gethostbyname(s);
	if (hp) {
		*hpp = hp;
		sin->sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, &sin->sin_addr, hp->h_length);
		*name = savestr(hp->h_name);
		return(1);
	}
	fprintf(stderr, "%s: bad value\n", s);
	exit(1);
}

short ns_nullh[] = {0,0,0};
short ns_bh[] = {-1,-1,-1};

# ifdef XNS
char *
ns_print(sns)
struct sockaddr_ns *sns;
{
	struct ns_addr work;
	union { union ns_net net_e; u_long long_e; } net;
	u_short port;
	static char mybuf[50], cport[10], chost[25];
	char *host = "";
	register char *p; register u_char *q; u_char *q_lim;

	work = sns->sns_addr;
	port = ntohs(work.x_port);
	work.x_port = 0;
	net.net_e  = work.x_net;
	if (ns_nullhost(work) && net.long_e == 0) {
		if (port ) {
			sprintf(mybuf, "*.%xH", port);
			upHex(mybuf);
		} else
			sprintf(mybuf, "*.*");
		return (mybuf);
	}

	if (bcmp(ns_bh, work.x_host.c_host, 6) == 0) { 
		host = "any";
	} else if (bcmp(ns_nullh, work.x_host.c_host, 6) == 0) {
		host = "*";
	} else {
		q = work.x_host.c_host;
		sprintf(chost, "%02x%02x%02x%02x%02x%02xH",
			q[0], q[1], q[2], q[3], q[4], q[5]);
		for (p = chost; *p == '0' && p < chost + 12; p++);
		host = p;
	}
	if (port)
		sprintf(cport, ".%xH", htons(port));
	else
		*cport = 0;

	sprintf(mybuf,"%xH.%s%s", ntohl(net.long_e), host, cport);
	upHex(mybuf);
	return(mybuf);
}
# endif XNS

upHex(p0)
char *p0;
{
	register char *p = p0;
	for (; *p; p++) switch (*p) {

	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		*p += ('A' - 'a');
	}
}
