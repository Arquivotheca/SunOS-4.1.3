#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)get_ethertypes.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)get_ethertypes.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 * 	Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include "install.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

/*
 *	Name:		get_ethertypes()
 *
 *	Description:	Get the types of the ethernet(s) configured in this
 *			system.
 */

void
get_ethertypes(sys_p)
	sys_info *	sys_p;
{
	char		buf[BUFSIZ];		/* buffer for socket info */
	int		count = 1;		/* counter for ethernets */
	int		ether;			/* ethernet interface */
	char		ether_name[TINY_STR];	/* name of the interface */
	int		i;			/* index variable */
        int		sd;			/* socket descriptor */
        struct ifconf	ifc;			/* interface config buffer */
	struct ifreq *	ifr;			/* ptr to interface request */


       	sd = socket(AF_INET, SOCK_DGRAM, 0);
       	if (sd < 0) {
       		perror("socket");
		exit(1);
       	}

	bzero(buf,sizeof(buf));
       	ifc.ifc_len = sizeof(buf);
       	ifc.ifc_buf = (caddr_t) buf;
       	if (ioctl(sd, SIOCGIFCONF, (char *) &ifc) < 0) {
		menu_log("%s: ioctl failed (get interface configuration).",
		         progname);
		menu_abort(1);
       	}

	/*
	 *	Now, go through the enthernet interfaces, and put them in
	 *	the sys_info structure.
	 */
       	for (ifr = ifc.ifc_req; ifr->ifr_name[0]; ifr++) {
		/*
		 *	Extract the first alpha-only string as the
		 *	interface name.
		 */
		for (i = 0; isalpha(ifr->ifr_name[i]); i++)
			ether_name[i] = ifr->ifr_name[i];
		ether_name[i] = NULL;

		/*
		 *	Not a known ethernet type.
		 */
		if (cv_str_to_ether(ether_name, &ether) != 1) {
			/*
			 *	Only complain if this is not the loopback
			 *	filesystem.
			 */
			if (strcmp(ether_name, "lo") != 0)
				menu_log("%s: %s: unknown ethernet interface.",
					 progname, ether_name);
			continue;
		}

		if (count >= MAX_ETHER_INTERFACES)
			menu_mesg("Ignoring '%s': too many interfaces.",
				  ifr->ifr_name);
		else {
			/*
			 *	If it's the system interface, put in in the
	 		 *	ether zero place. Since there is only 1 '0'
	 		 *	interface. 
	 		 */
			if (ifr->ifr_name[i] == '0') {
				(void)strcpy(sys_p->ether_namex(0),
					     ifr->ifr_name);
				continue;
			}
			(void)strcpy(sys_p->ether_namex(count),ifr->ifr_name);
			count++;
		}
       	}
	(void) close(sd);
} /* end get_ethertypes() */


