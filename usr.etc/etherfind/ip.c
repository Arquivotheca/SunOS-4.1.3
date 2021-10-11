/*
 * IP Utility module for the "Etherfind" program
 *
 * @(#)ip.c 1.1 92/07/30
 *
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <netdb.h>
#include <stdio.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_var.h>
#include <netinet/udp_var.h>
#include <netinet/ip_icmp.h>

extern unsigned long inet_network();
extern unsigned long inet_addr();
extern char *malloc(), *index();

#define ICMPTABLESIZE 256
char *icmptable[ICMPTABLESIZE];

extern int nflag;

#define HASHNAMESIZE 256

/*
 * XXX:	Need to revise to take multiple addresses per interface
 *	into account.  This change should be part of a revision
 *	that teaches etherfind about protocol families other than
 *	AF_INET.
 */
struct hnamemem {
	struct in_addr h_addr0;		/* first address in list (XXX) */
	char *h_name;
	struct hnamemem *h_nxt;
};

struct hnamemem *htable[HASHNAMESIZE];

#define TCP 0
#define UDP 1
#define NPROTOCOLS 2
#define MAXPORT 1024
char *names[NPROTOCOLS][MAXPORT];

#define PROTOTABLESIZE 256

char *prototable[PROTOTABLESIZE];


/*
 * return the name of the given IP address, using the internal hash
 * table if necessary.  The "nflag" bypasses all name lookup and just 
 * prints it out as a number.  The address is passed as a structure in
 * network order.
 */
char *
getname(addr)
	struct in_addr addr;
{
	int x;
	struct hnamemem *p;
	char *buf, *inet_ntoa();
	struct netent *np;
	struct hostent *hp;
	
	x = ntohl(addr.s_addr) & 0xff;
	for (p = htable[x]; p != NULL; p = p->h_nxt) {
		if (p->h_addr0.s_addr == addr.s_addr)
			return (p->h_name);
	}
	p = (struct hnamemem *)malloc(sizeof(struct hnamemem));
	p->h_addr0 = addr;
	p->h_nxt = htable[x];
	htable[x] = p;

# ifndef INADDR_BROADCAST
# define INADDR_BROADCAST (0xffffffff)
# endif 

	if (nflag) {
		hp = NULL;
	}
	else {
		if (addr.s_addr == htonl(INADDR_BROADCAST)) {
			p->h_name = "broadcast";
			return (p->h_name);
		}
		if (addr.s_addr == htonl(INADDR_ANY)) {
			p->h_name = "old-broadcast";
			return (p->h_name);
		}
		hp = gethostbyaddr((char *)&addr, sizeof(int), AF_INET);
		if (hp == NULL && inet_lnaof(addr) == 0) {
		    np = getnetbyaddr(inet_netof(addr), AF_INET);
		    if (np) {
			p->h_name = malloc((unsigned)strlen(np->n_name) + 1);
			(void) strcpy(p->h_name, np->n_name);
			return (p->h_name);
		    }
		}
	}
	if (hp) {
		p->h_name = malloc((unsigned)strlen(hp->h_name) + 1);
		(void) strcpy(p->h_name, hp->h_name);
	}
	else {
		buf = inet_ntoa(addr);
		p->h_name = malloc((unsigned)(strlen(buf)) + 1);
		(void) strcpy(p->h_name, buf);
	}
	return (p->h_name);
}

/*
 *  convert string (which may be host address or host name) to address
 */
stringtoaddr(name)
	char *name;
{
	struct hostent *hp;
	char	*p;
	unsigned short a, b;

	if (isdigit(name[0])) {
		p = index(name, '.');
		if (p && index(p+1, '.') == 0) { /* decnet address area.host */
			a = atoi(name);
			b = atoi(p+1);
			if ((a < 1) || (a > 63)) {
				fprintf(stderr, "%d invalid decnet area\n", a);
				exit(1);
			}
			if ((b < 1) || (b > 1023)) {
				fprintf(stderr, "%d invalid decnet host\n", b);
				exit(1);
			}
			a = (a << 10) + b;
			swab((char *)&a, (char *)&b, 2);
			return(b);
		} else
			return(inet_addr(name));
	} 

	hp = gethostbyname(name);
	if (hp == NULL) {
		fprintf(stderr, "%s unknown host name\n", name);
		exit(1);
	}
	else
		return(*(int *)hp->h_addr);
}


stringtonetaddr(name)
	char *name;
{
	struct netent *np;

	if (isdigit(name[0]))
		return(inet_network(name));

	np = getnetbyname(name);
	if (np == NULL) {
		fprintf(stderr, "%s unknown net name\n", name);
		exit(1);
	}
	else
		return (np->n_net);
}


/*
 * returns the port number in Network byte order
 */
unsigned short
stringtoport(str)
	char *str;
{
	struct servent *sp;
	
	if (isdigit(str[0]))
		return(htons((u_short) atoi(str)));
	else {
		sp = getservbyname(str, (char *)NULL);
		if (sp == NULL) {
			fprintf(stderr,
			 "etherfind: %s is not a valid port name or number\n",
			    str);
			exit(1);
		}
		return(sp->s_port);
	}
}


/*
 * Return the printable version of an IP port number
 */
char *
getportname(port, proto)
	u_short port;
{
	static char buf[10];
	int protonum;
	
	port = ntohs(port);
	
	if (proto == IPPROTO_TCP)
		protonum = TCP;
	else if (proto == IPPROTO_UDP)
		protonum = UDP;
	else {
		sprintf(buf, "%d", port);
		return (buf);
	}
	if (port < MAXPORT && names[protonum][port])
		return (names[protonum][port]);
	else {
		sprintf(buf, "%d", port);
		return (buf);
	}
}

char *icmptype(icp)
    struct icmp *icp;		/* points to icmp packet */
{
 return(icmptable[icp->icmp_type]);
}

initIP()
  /*
   * Initialize the Internet Protocol specific module
   */
{
	struct servent *sv;	
	int proto;
	char *p;
	
	icmptable[ICMP_ECHOREPLY] = "echo reply";
	icmptable[ICMP_UNREACH] = "dst unreachable";
	icmptable[ICMP_SOURCEQUENCH] = "src quench";
	icmptable[ICMP_REDIRECT] = "redirect";
	icmptable[ICMP_ECHO] = "echo";
	icmptable[ICMP_TIMXCEED] = "time exceeded";
	icmptable[ICMP_PARAMPROB] = "param problem";
	icmptable[ICMP_TSTAMP] = "timestamp";
	icmptable[ICMP_TSTAMPREPLY] = "timestamp reply";
	icmptable[ICMP_IREQ] = "info request";
	icmptable[ICMP_IREQREPLY] = "info reply";
	icmptable[ICMP_MASKREQ] = "mask request";
	icmptable[ICMP_MASKREPLY] = "mask reply";

	for (proto = 0; proto < ICMPTABLESIZE; proto++) {
		if (icmptable[proto] == NULL) {
			p = malloc(5);
			sprintf(p, "%d", proto);
			icmptable[proto] = p;
		}
	}

	prototable[IPPROTO_ICMP]  = "icmp";
	prototable[IPPROTO_IGMP]  = "igmp";
	prototable[IPPROTO_GGP]   = "ggp";
	prototable[5]		  = "st";
	prototable[IPPROTO_TCP]   = "tcp";
	prototable[IPPROTO_EGP]   = "egp";
	prototable[IPPROTO_UDP]   = "udp";
	prototable[27]		  = "rdp";
	prototable[30]		  = "netblt";
	prototable[IPPROTO_HELLO] = "hello";
	prototable[IPPROTO_ND]    = "nd";

	while (sv = getservent()) {
		if (strcmp(sv->s_proto, "tcp") == 0)
			proto = TCP;
		else if (strcmp(sv->s_proto, "udp") == 0)
			proto = UDP;
		else
			continue;
		sv->s_port = ntohs(sv->s_port);
		if (sv->s_port < 0 || sv->s_port >= MAXPORT)
			continue;
		names[proto][sv->s_port] = 
			malloc((unsigned)strlen(sv->s_name) + 1);
		(void) strcpy(names[proto][sv->s_port], sv->s_name);
	}
	endservent();
}

getproto(arg)
	char *arg;
{
	char **p;
	int protonum;
	
	if (isalpha(arg[0])) {
		protonum = -1;
		for (p = prototable; p  < prototable + PROTOTABLESIZE; p++) {
			if (*p && strcmp(arg, *p) == 0) {
				protonum = p - prototable;
				break;
			}
		}
		if (protonum == -1) {
			fprintf(stderr, "%s unknown protocol\n", arg);
			exit(1);
			/*NOTREACHED*/
		}
		else
			return (protonum);
	}
	else
		return (atoi(arg));
}

