#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <ctype.h>
#include <string.h>
#include <sys/ioctl.h>

/*
 *	Name:		get_ether0name()
 *
 *	Description:	Get the name of the first ethernet interface
 *
 *	Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint 
static  char sccsid[] = "@(#)get_ether0name.c 1.1 92/07/30 SMI"; 
#endif

#define DEFAULT_NAME	"xx"
#define LOOPBACK_NAME	"lo"
#define LOOPBACK_NAME_LEN 	2

int
get_ether0name(ether_name, namelen)
	char *	ether_name;
	int	namelen;
{
	char		buf[1024];		/* buffer for socket info */
        int		sd;			/* socket descriptor */
        struct ifconf	ifc;			/* interface config buffer */
	struct ifreq *	ifr;			/* ptr to interface request */
	int 		i;			/* loop index */

       	sd = socket(AF_INET, SOCK_DGRAM, 0);
       	if (sd < 0)
		return -1;

	bzero(buf,sizeof(buf));
       	ifc.ifc_len = sizeof(buf);
       	ifc.ifc_buf = (caddr_t) buf;
       	if (ioctl(sd, SIOCGIFCONF, (char *) &ifc) < 0) { 
		(void) close(sd);
		return -1;
		}

	(void) close(sd);


	/*
	 *	Now find the 0 interface
	 */
       	for (ifr = ifc.ifc_req; ifr->ifr_name[0]; ifr++) {
		/*
		 *	Extract the first alpha-only string as the
		 *	interface name.  Skip loopback interface
		 */
		if (!strncmp(ifr->ifr_name, LOOPBACK_NAME, LOOPBACK_NAME_LEN))
			continue;
		for (i = 0; isalpha(ifr->ifr_name[i]) && i < namelen; i++)
			ether_name[i] = ifr->ifr_name[i];
		ether_name[i] = NULL;
		/*
		 *	If it's the system interface return it.  This works 
 		 *	because there is only 1 '0' interface. 
 		 */
		if (ifr->ifr_name[i] == '0')
			return 0;
		}
/*
 * 	We're here because the only thing we found was loopback.  The only 
 *	thing to do is to fill in the default 
 */

	strcpy(ether_name, DEFAULT_NAME);
	return 0;
} /* end get_ether0name() */


