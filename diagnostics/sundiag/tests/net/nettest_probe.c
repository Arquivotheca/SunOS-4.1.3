#ifndef lint
static	char sccsid[] = "@(#)nettest_probe.c 1.1 7/30/92 Copyright Sun Microsystems";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/******************************************************************************
 
  net_probe.c

  The original version of the code was :

  Completely swiped from /usr/src/lib/libc/rpc/pmap_rmt.c:
  The following is kludged-up support for simple rpc broadcasts.

  Revision	 : SCHEUFELE, SUSAN	susans@freedonia
		    
		     Complete clean up and rewrite of original
		     code including 4.0 bug fixes.		  
		  
  2nd Revision     : MASSOUDI, ROBERT A.     bobm@thesius
		     2-10-88,
		      
		     Structural change into more readable blocks,
		     Complete documentation and comments, bug fix,
		     enhancements, and mods to accomodate other
		     network devices as well (i.e. fddi).

  3rd Revison	   : Anantha N. Srirama		anantha@ravi

		     Total rewrite, I ripped the guts out of this program.
		     The present version bears very little resemblence to
		     the previous version, tons of useless stuff has been
		     thrown away with vengence.

*******************************************************************************

		      INCLUDE FILES

******************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sdrtns.h>
#include "nettest.h"


/******************************************************************************

		     EXTERNAL VARIABLE DEFINITIONS

******************************************************************************/
extern	int	debug;

struct	in_addr	 inet_makeaddr ();
char*	inet_ntoa ();


/******************************************************************************

				GLOBALS

******************************************************************************/
struct	ifreq	ifreq;


/******************************************************************************

  getnetaddrs

  This function will return the address of the interface passed in.  It does
  so by calling the ioctl function.  It returns a 0 on a successful net address
  extraction or else it returns a -1.

******************************************************************************/
int	getnetaddrs (sock, flags, ifindx, valid_if_p)
int	sock, flags, ifindx;
struct	avail_inet_if_struct  *valid_if_p;  	/* valid IF devices */
{
    struct	sockaddr_in	*sin;
    struct	sockaddr_in	netmask;

    func_name = "getnetaddrs";
    TRACE_IN
    (void) strcpy (valid_if_p[ifindx].dev_name, ifreq.ifr_name);
    send_message (0, DEBUG, "%s: found: %s", func_name, ifreq.ifr_name);
    if (ioctl (sock, SIOCGIFADDR, (caddr_t) &ifreq) < 0) {
	send_message (0, DEBUG, "%s: ioctl (SIOCGIFADDR) %s",
		      func_name, errmsg (errno));
	return (-1);
    }
    sin = (struct sockaddr_in *) &ifreq.ifr_addr;
    valid_if_p[ifindx].my_addr = sin->sin_addr;
    if (debug) {
	send_message (0, DEBUG, "%s: inet %s ", func_name, inet_ntoa (sin->sin_addr));
	if (ioctl (sock, SIOCGIFNETMASK, (caddr_t) &ifreq) < 0) {
	    if (errno != EADDRNOTAVAIL) {
		send_message (0, DEBUG, "%s: ioctl (SIOCGIFNETMASK) %s",
			      func_name, errmsg (errno));
	    }
	    bzero ((char *) &ifreq.ifr_addr, sizeof (ifreq.ifr_addr));
	}
	else
	    netmask.sin_addr =
		((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr;
	send_message (0, DEBUG, "%s: netmask 0x%x",
		      func_name, ntohl (netmask.sin_addr.s_addr));
    }
    bzero ((char *) &ifreq.ifr_addr, sizeof (ifreq.ifr_addr));
    if (ioctl (sock, SIOCGIFBRDADDR, (caddr_t) &ifreq) < 0) {
	send_message (0, DEBUG, "%s: ioctl (SIOCGIFBRDADDR) %s",
		      func_name, errmsg (errno));
	return (-1);
    }
    sin = (struct sockaddr_in *) &ifreq.ifr_addr;
    send_message (0, DEBUG, "%s: IFF_BROADCAST: %s",
		  func_name, inet_ntoa (sin->sin_addr));
    if (sin->sin_addr.s_addr == 0) return (-1);	  
    valid_if_p[ifindx].broad_addrs = sin->sin_addr;
    TRACE_OUT
    return (0); 
}


/******************************************************************************
 
  getbroadcastnets

  Get all valid IF interfaces (except for "me0"), save the IF name, IP address
  and create a broadcast address for each valid entry.  The structure that this
  information is saved in is passed in as an argument, the definition for that
  structure is within nettest.h.

  The following is done to get the networks that have broadcast capability:

  	1. Open a socket for raw communication
	2. Get the network configuration by calling 'ioctl'.  This call will
	   return a pointer to an array which contains all the valid interfaces
	   in the system.
	3. Then looping through all the available network interfaces and
	   checking if they have broadcast capability and incrementing the
	   counter which keeps track of # of broadcast nets.  The networks
	   broadcast address is computed by calling the function 'getnetaddrs'.
	4. Close the socket

  The return value of this function is the number of braodcast networks in
  the system.  In case of fatal errors (unrecoverable), the function exits
  the program by calling send_message with a negative 'type' argument.

******************************************************************************/
int	getbroadcastnets (valid_if_p, buf)
struct	avail_inet_if_struct  *valid_if_p;
char	*buf;
{
    struct	ifconf   ifc;	/* place holder for IF configuration info */
    struct	ifreq    *ifr;
    int		sock, n, i, flags;

    func_name = "getbroadcastnets";
    TRACE_IN
    if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
	send_message (-NO_SOCKET, FATAL, "%s could not open a socket %s",
		      test_name, errmsg (errno));
    }
    ifc.ifc_len = UDPMSGSIZE;
    ifc.ifc_buf = buf;
    if (ioctl (sock, SIOCGIFCONF, (char *) &ifc) < 0) {
	send_message (0, DEBUG, "%s: ioctl (IF configuration) %s",
		      func_name, errmsg (errno));
	return (0);
    }
    send_message (0, DEBUG,
		  "%s: Probing for valid network IF boards", func_name);
    ifr = ifc.ifc_req;   			 
    for (i = 0, n = ifc.ifc_len/sizeof (struct ifreq); n > 0; n--, ifr++) {
	ifreq = *ifr;
	if (ioctl (sock, SIOCGIFFLAGS, (char *) &ifreq) < 0) {
	    send_message (0, DEBUG, "%s: ioctl (IF flags) %s",
			  func_name, errmsg (errno));
	    continue;
	}
	flags = ifreq.ifr_flags;
	if ((flags & (IFF_UP | IFF_BROADCAST)) &&
	    (ifr->ifr_addr.sa_family == AF_INET)) {
	    if (strcmp (ifreq.ifr_name, "me0") == 0) {
		continue;
	    }
	    if (strcmp (ifreq.ifr_name, "lo0") == 0) { /* skip local loopbk */
		continue;
	    }
	    if (getnetaddrs (sock, flags, i, valid_if_p) == 0) {
		i++;
	    }
	    else {
		send_message (0, ERROR, "Could not get broadcast address");
	    }
	}
    }
    (void) close (sock);
    TRACE_OUT
    return (i);
}
