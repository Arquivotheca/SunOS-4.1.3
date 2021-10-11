#ifndef lint
static  char sccsid[] = "@(#)probe_nets.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
#include <stdio.h> 
#include <errno.h> 
#ifdef SVR4
#include <string.h> 
#else
#include <strings.h> 
#endif SVR4
#include <sys/types.h> 
#ifndef SVR4
#include <sys/dir.h> 
#endif SVR4
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include "probe.h" 
#include "../../lib/include/probe_sundiag.h" 
#include "sdrtns.h"		/* sundiag standard header */ 

#ifdef SVR4
#include <sys/sockio.h>
#define UDPMSGSIZE 8800
#endif SVR4

extern char *malloc();

static char **getbroadcastnets();

/*
 * check_pc_dev(makedevs, dunit) - checks IPC device files
 */
check_pc_dev(makedevs, dunit)
    int makedevs;
    int dunit;
{
    int fmajor = 38;
    char *mode = "0666";
    char name[MAXNAMLEN];
    int fminor;

    func_name = "check_pc_dev";
    TRACE_IN
    (void) sprintf(name, "/dev/pc%d", dunit);
    fminor = dunit;
    if ((check_dev(makedevs, name, character, fmajor, fminor, mode, no)) != 1)
    {
	TRACE_OUT
        return(0);
    }
    TRACE_OUT
    return(1);
}

/*
 * check_net(makedevs, dunit) - checks net to make sure that it's up
 */
check_net(name, unit)
    char *name;
    int unit;
{
    int  s;
    char inbuf[UDPMSGSIZE];
    char **pn, dname[MAXNAMLEN], buf[BUFSIZ-1];
    static char **nets = NULL;

    func_name = "check_net";
    TRACE_IN
    if (strcmp(name, FDDI) == 0)
    {
	TRACE_OUT
	return(NETUP);		/* FDDI has loopback cable */
    }

    if (nets == NULL) {
#ifdef SVR4
	if ((s = open("/dev/ip", O_RDWR)) < 0) {
            perror("probe: open /dev/ip");
	    TRACE_OUT
            return(NETPROB);
        }
#else SVR4
        if ((s = socket(AF_INET, SOCK_RAW, 0)) < 0) {
            perror("probe: socket");
	    TRACE_OUT
            return(NETPROB);
        }
#endif SVR4
        if ((nets = getbroadcastnets(s, inbuf)) == NULL)
	{
	    TRACE_OUT
	    return(NETPROB);
	}
        (void) close(s);
    }

#ifdef SVR4
/* ifioctl pre-appends an "s" to the name so we do the same */
    (void) sprintf(dname, "s%s%d", name, unit);
#else SVR4
    (void) sprintf(dname, "%s%d", name, unit);
#endif SVR4

    for (pn = nets; *pn != NULL; pn++) {
	if (strcmp(*pn, dname) == 0) {
	    send_message(0, DEBUG, "*pn = %s", *pn);
	    TRACE_OUT
	    return(NETUP);
	}
    }
    (void) sprintf(buf, "%s net not up\n", dname);

    if (strcmp(name, ME) != 0)
        send_message(0, ERROR, buf);

    TRACE_OUT
    return(NETDOWN);
}

/*
 * Mostly swiped from /usr/src/lib/libc/rpc/pmap_rmt.c:
 * The following is kludged-up support for simple rpc broadcasts.
 * Someday a large, complicated system will replace these trivial
 * routines which only support udp/ip - get rid of sun4 ifdef when sun4 > 3.2
 */
static char **
getbroadcastnets(sock, buf)
    int             sock;                      /* any valid socket will do */
    char           *buf;                       /* why allocxate more when we
                                                * can use existing... */
{
    struct ifconf   ifc;
    struct ifreq    ifreq, *ifr;
    int             n, i;
    static char     *ps[BUFSIZ-1];
    char *perrmsg, errbuf[BUFSIZ-1];

    func_name = "getbroadcastnets";
    TRACE_IN
    ifc.ifc_len = UDPMSGSIZE;
    ifc.ifc_buf = buf;
#ifdef SVR4
    if (ifioctl(sock, SIOCGIFCONF, (char *) &ifc) < 0) {
#else SVR4
    if (ioctl(sock, SIOCGIFCONF, (char *) &ifc) < 0) {
#endif SVR4
        perror("ioctl: SIOCGIFCONF");
	TRACE_OUT
        return(NULL);
    }
    ifr = ifc.ifc_req;
    for (i = 0, n = ifc.ifc_len / sizeof(struct ifreq); n > 0; n--, ifr++) {
        ifreq = *ifr;
#ifdef SVR4
        if (ifioctl(sock, SIOCGIFFLAGS, (char *) &ifreq) < 0) {
#else SVR4
        if (ioctl(sock, SIOCGIFFLAGS, (char *) &ifreq) < 0) {
#endif SVR4
            perror("ioctl: SIOCGIFFLAGS");
            continue;
        }
        if ((ifreq.ifr_flags & IFF_BROADCAST) &&
            (ifreq.ifr_flags & IFF_UP) &&
            ifr->ifr_addr.sa_family == AF_INET) {
	    send_message (0, DEBUG, "ifreq.ifr_name = %s", ifreq.ifr_name);
	    if ((ps[i] = malloc((unsigned)strlen(ifreq.ifr_name+1))) == NULL) {
		send_message(1, FATAL, "malloc ps[%d]: %s", i,errmsg(errno)); 
            }
	    (void) strcpy(ps[i++], ifreq.ifr_name);
        }
    }    
    TRACE_OUT
    return (ps);                 /* return the # of boards found */
}

 
#ifdef SVR4
#include <sys/stropts.h>
 
ifioctl(s, cmd, arg)
        int s;
        int cmd;
        char *arg;
{
        struct strioctl ioc;
        int ret;

        bzero((char *) &ioc, sizeof(ioc));
        ioc.ic_cmd = cmd;
        ioc.ic_timout = 0;
        if (cmd == SIOCGIFCONF) {
                ioc.ic_len = ((struct ifconf *) arg)->ifc_len;
                ioc.ic_dp = ((struct ifconf *) arg)->ifc_buf;
        } else {
                ioc.ic_len = sizeof(struct ifreq);
                ioc.ic_dp = arg;
        }
        ret = ioctl(s, I_STR, (char *) &ioc);
        if (ret != -1 && cmd == SIOCGIFCONF)
                ((struct ifconf *) arg)->ifc_len = ioc.ic_len;
        return(ret);
}
#endif SVR4
