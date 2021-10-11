#ifndef lint
static        char sccsid[] = "@(#)in_proto.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

#include <sys/param.h>
#include <sys/socket.h>
#include "boot/protosw.h"
#include "boot/domain.h"
#include <sys/mbuf.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

/*
 * TCP/IP protocol family: IP, ICMP, UDP, TCP.
 */
int	udp_input(),udp_ctlinput();
int	udp_usrreq();
int	udp_init();
/*
 * IMP protocol family: raw interface.
 * Using the raw interface entry to get the timer routine
 * in is a kludge.
 */
#include "imp.h"
#if NIMP > 0
int	rimp_output(), hostslowtimo();
#endif

struct protosw inetsw[] = {
{ SOCK_DGRAM,	PF_INET,	IPPROTO_UDP,	PR_ATOMIC|PR_ADDR,
  udp_input,	0,		udp_ctlinput,	0,
  udp_usrreq,
  udp_init,	0,		0,		0,
},
/*
 * This is just a place-holder, since we do not support IPPROTO_RAW.
 */
{ SOCK_RAW,	PF_INET,	IPPROTO_RAW,	PR_ATOMIC|PR_ADDR,
  0,		0,		0,		0,
  0,
  0,		0,		0,		0,
},
};

struct domain inetdomain =
    { AF_INET, "internet", inetsw, &inetsw[sizeof(inetsw)/sizeof(inetsw[0])] };

#if NIMP > 0
struct protosw impsw[] = {
{ SOCK_RAW,	PF_IMPLINK,	0,		PR_ATOMIC|PR_ADDR,
  0,		rimp_output,	0,		0,
  raw_usrreq,
  0,		0,		hostslowtimo,	0,
},
};

struct domain impdomain =
    { AF_IMPLINK, "imp", impsw, &impsw[sizeof (impsw)/sizeof(impsw[0])] };
#endif
