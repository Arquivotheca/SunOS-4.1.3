#ifndef lint
#ifdef SunB1
#ident			"@(#)ifconfig.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)ifconfig.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint


/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		ifconfig()
 *
 *	Description:	Configure the ethernet interfaces that are
 *			appropriate for the operation we doing, specified by
 *			ifconfig_type.
 *
 *              Note for SunOS MLS:
 *              Do not use the '-label' option to ifconfig here. The '-label'
 *              option will restrict the ethernet interface to a label range
 *              of system_low to system_low which is not acceptable for all
 *              uses by suninstall, dataless clients in particular.
 *  		A dataless  client mounts its /usr from the server and needs
 *		system_low to system_high access.
 *
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"

extern	char *		sprintf();

static	int		ifconfig_test();


void
ifconfig(sys_p, soft_p, ifconfig_type)
	sys_info *	sys_p;
	soft_info *	soft_p;
	int		ifconfig_type;
{
	char		cmd[BUFSIZ];		/* command buffer */
	int		n;

	/*
	 *	Let's not configure interfaces in multiuser mode, because it
	 *	might screw things up
	 */
	if (!is_miniroot())
		return;
	
	if (sys_p->ether_type == ETHER_NONE)
		return;
	
	menu_flash_on("Configuring appropriate interface");

	
	for (n = 0; n < MAX_ETHER_INTERFACES; n++) {
#ifdef SunB1
		if (*sys_p->ipx(n) &&  sys_p->ethers[n].ip_minlab != LAB_OTHER
		    && sys_p->ethers[n].ip_maxlab != LAB_OTHER) {
#else
		if (*sys_p->ipx(n) != 0) {
#endif /* SunB1 */
			(void) sprintf(cmd,
			       "/etc/ifconfig %s %s -trailers up 2>> %s",
				       sys_p->ether_namex(n),
				       sys_p->hostnamex(n),
				       LOGFILE);
				
			x_system(cmd);
		}

		/*
		 *	if the ifconfig worked, then stop ifconfig'ing
		 */
		if (ifconfig_test(sys_p, soft_p, ifconfig_type))
			return;
	}

	menu_flash_on("");
		
} /* end ifconfig() */
	


/*
 *	Name:		(static void) ifconfig_test()
 *
 *	Description:	test if the interface is the one that we want
 *
 *	Return value:  1 : if the interface is good
 *		       0 : if the interface is not the one we want.
 *
 */

static int
ifconfig_test(sys_p, soft_p, ifconfig_type)
	sys_info *	sys_p;
	soft_info *	soft_p;
	int		ifconfig_type;
{
	char	cmd[2 * MAXPATHLEN + MAXHOSTNAMELEN + 25];
	
	switch(ifconfig_type) {
	case IFCONFIG_RSH :
		if (check_remote_access(soft_p->media_host))
			return(1);
		else
			return(0);
		break;
	case IFCONFIG_MOUNT:
		(void) sprintf(cmd, "mount -r -o soft %s:%s %s/usr", 
			       sys_p->server, sys_p->exec_path, 
			       dir_prefix());
		
		if (system(cmd) == 0) {
			(void) sprintf(cmd, "umount %s/usr", dir_prefix());
			(void) system(cmd);
			return(1);
		} else {
			/*
			 *	Mount failed
			 */
			return(0);
		}
	}
	/* NOTREACHED */
}
