#ifndef lint
#ifdef SunB1
#ident			"@(#)update_yp.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)update_yp.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 * Copyright (c) 1990 by Sun Microsystems,Inc
 */

/***************************************************************************
** Function : (void) update_yp()
**
** Return Value : none
**
** Description : This function updates the NIS's appropriately, or tells the
**	user to inform the NIS administrator.
**
****************************************************************************
*/

#include <stdio.h>
#include <sys/file.h>
#include "install.h"

void
update_yp(client_p)
	clnt_info	*client_p;
{
	char	cmd[MAXPATHLEN + 32];
	char	domain[64];
	short	noname_flag;		/* set if domainname is "noname" */
	short	bootparam_flag;		/* set if "/var/yp/bootparams.time"
					 * exists
					 */

	if (is_miniroot())
		return;

	/*
	**	If this call fails, assume no domain and fail
	*/
	if (getdomainname(domain, 64) != 0) {
		menu_log("get domainname failed\n");
		return;
	}

	noname_flag 	= (strcmp(domain, "noname") == 0); 
	bootparam_flag  = (access("/var/yp/bootparams.time", F_OK) == 0);

	/*
	 *	If we are not on the miniroot and this is a server,
	 *	then push the new NIS databases.
	 */
	if (!noname_flag  && bootparam_flag) {
		/*
		**  	We have a server.
		*/
		menu_log("Remaking NIS databases for this NIS server\n");
#ifndef	TEST_JIG
		(void) sprintf(cmd, "cd /var/yp; make >> %s 2>&1", LOGFILE);
		x_system(cmd);
#endif
	} else	if  (!noname_flag && !bootparam_flag) {
		if (access("/var/yp/bootparams.time-", F_OK) != 0)
			menu_log(
"\nYou must now ask the system administrator to update the NIS\n\
master's /etc/hosts, /etc/bootparams and /etc/ethers files,\n\
because of the change in status of the client '%s.'\n",  client_p->hostname);

	}
}

