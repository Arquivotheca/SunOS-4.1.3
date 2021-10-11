/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)in_proto.c 1.1 92/07/30 SMI; from UCB 7.2 12/7/87
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/tcp_timer.h>

/*
 * TCP/IP protocol family: IP, ICMP, UDP, TCP.
 */
int	ip_output(),ip_ctloutput();
int	ip_init(),ip_slowtimo(),ip_drain();
int	icmp_input();
int	udp_input(),udp_ctlinput();
int	udp_usrreq();
int	udp_init();
int	tcp_input(),tcp_ctlinput();
int	tcp_usrreq(),tcp_ctloutput();
int	tcp_init(),tcp_fasttimo(),tcp_slowtimo(),tcp_drain();
int	rip_input(),rip_output(),rip_ctloutput();
extern	int raw_usrreq();

#ifdef NSIP
int	idpip_input(), nsip_ctlinput();
#endif

extern	struct domain inetdomain;

struct protosw inetsw[] = {
{ 0,		&inetdomain,	0,		0,
  0,		ip_output,	0,		0,
  0,
  ip_init,	0,		ip_slowtimo,	ip_drain,
},
{ SOCK_DGRAM,	&inetdomain,	IPPROTO_UDP,	PR_ATOMIC|PR_ADDR,
  udp_input,	0,		udp_ctlinput,	ip_ctloutput,
  udp_usrreq,
  udp_init,	0,		0,		0,
},
{ SOCK_STREAM,	&inetdomain,	IPPROTO_TCP,	PR_CONNREQUIRED|PR_WANTRCVD,
  tcp_input,	0,		tcp_ctlinput,	tcp_ctloutput,
  tcp_usrreq,
  tcp_init,	tcp_fasttimo,	tcp_slowtimo,	tcp_drain,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_RAW,	PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	0,		rip_ctloutput,
  raw_usrreq,
  0,		0,		0,		0,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_ICMP,	PR_ATOMIC|PR_ADDR,
  icmp_input,	rip_output,	0,		rip_ctloutput,
  raw_usrreq,
  0,		0,		0,		0,
},
#ifdef NSIP
{ SOCK_RAW,	&inetdomain,	IPPROTO_IDP,	PR_ATOMIC|PR_ADDR,
  idpip_input,	rip_output,	nsip_ctlinput,	0,
  raw_usrreq,
  0,		0,		0,		0,
},
#endif
	/* raw wildcard */
{ SOCK_RAW,	&inetdomain,	0,		PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	0,		rip_ctloutput,
  raw_usrreq,
  0,		0,		0,		0,
},
};

struct domain inetdomain =
    { AF_INET, "internet", 0, 0, 0, 
      inetsw, &inetsw[sizeof(inetsw)/sizeof(inetsw[0])] };

/*
 * ip_forwarding controls whether or not to forward packets:
 *	ip_forwarding == -1  -- never forward; never change this value.
 *	ip_forwarding ==  0  -- don't forward; set this value to 1 when two
 *				interfaces are up.
 *	ip_forwarding ==  1  -- always forward.
 */

#ifndef IPFORWARDING
#define IPFORWARDING 0
#endif
int	ip_forwarding = IPFORWARDING;

#ifndef SUBNETSARELOCAL
#define	SUBNETSARELOCAL	1
#endif
int ip_subnetslocal = SUBNETSARELOCAL;

#ifndef IPSENDREDIRECTS
#define IPSENDREDIRECTS 1
#endif
int	ip_sendredirects = IPSENDREDIRECTS;

#ifndef DIRECTED_BROADCAST
#define DIRECTED_BROADCAST 1
#endif
int	ip_dirbroadcast = DIRECTED_BROADCAST;	/* "letter-bombs" */

int	ip_printfs = 0;		/* enable debug printouts */

/*
 * Default TCP Maximum Segment Size - 512 to be conservative,
 * Higher for high-performance routers
 */
int	tcp_default_mss = 512;

/*
 * Default TCP buffer sizes (in bytes)
 */
int	tcp_sendspace = 1024*4;
int	tcp_recvspace = 1024*4;

/*
 * size of "keep alive" probes.
 */
int	tcp_keeplen = 1;	/* must be nonzero for 4.2 compat- XXX */
int	tcp_ttl = TCP_TTL;	/* default time to live for TCPs */
int	tcp_nodelack = 0;	/* turn off delayed acknowledgements */
int	tcp_keepidle = TCPTV_KEEP_IDLE;	/* for Keep-alives */
int	tcp_keepintvl = TCPTV_KEEPINTVL;

int	udp_cksum = 0;		/* turn on to check & generate udp checksums */
int	udp_ttl = 60;		/* default time to live for UDPs */

/*
 * Default UDP buffer sizes (in bytes)
 */
int	udp_sendspace = 9000;		/* really max datagram size */
int	udp_recvspace = 2*(9000+sizeof(struct sockaddr)); /* 2 8K dgrams */

