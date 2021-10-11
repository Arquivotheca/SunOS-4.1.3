#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)remote_access.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)remote_access.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint


/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */


/*
 *	Name:		menu_check_remote_access()
 *
 *	Description:	This function checks for access permissions on
 *			remote hosts. This function is used to produce its
 *			own error messages.  We use the function
 *			check_remote_access() to tell us if we really have
 *			access.
 *
 *			We are assuming the ifconfig has been done.
 *
 *	Return Value :  1 if access is allowed
 *			0 if access is denied
 */

#include <stdio.h>
#include <sys/param.h>
#include "install.h"

int
menu_check_remote_access(remote_host)
	char *		remote_host;	/* name of the remote host to check */
{

	if (check_remote_access(remote_host) == 1)
		return(1);
	else {
		/*
		** 	access was denied, so give an error message.
		*/
		menu_mesg("%s '%s'\n%s %s%s",
			  "Access was denied to remote host", remote_host,
	          "Make sure that your hostname exists in", remote_host,
			  "'s /.rhosts file");
		
		return(0);
	}
}
			


/*
 *	Name:		check_remote_access()
 *
 *	Description:	This function checks for access permissions on
 *			remote hosts.  It trys to to an rsh to ls the
 *			/dev directory.  This is very simple and should not
 *			fail on a correctly setup system.
 *
 *			We are assuming the ifconfig has been done.
 *
 *	Return Value :  1 if access is allowed
 *			0 if access is denied
 */

#include <stdio.h>
#include <sys/param.h>

int
check_remote_access(remote_host)
	char *		remote_host;	/* name of the remote host to check */
{
	char	buf[60 + MAXHOSTNAMELEN];
	
	
	(void) sprintf(buf, "rsh %s -n ls /dev > /dev/null 2>&1", remote_host);
	
	if (system(buf) == 0)
		return(1);
	else 
		return(0);
	
	
}


/*
 *	Name:		ck_remote_path()
 *
 *	Description:	Determine if a remote media path is valid.  Returns
 *		one if it is and zero if it is not.
 *
 */
int
ck_remote_path(soft_p, dev)
	soft_info *	soft_p;
	char *		dev;
{
	char		buf[MAXPATHLEN + BUFSIZ]; /* scratch buffer */
	FILE *		pp;			/* ptr to process */
	
	menu_flash_on("Checking remote media device");
	
	/*
	 *	If this fails, we are not in the /.rhosts file on the media
	 *	host.
	 */
	if (!menu_check_remote_access(soft_p->media_host)) 
		return(-1);

	(void) sprintf(buf,
"rsh -n %s \"exec sh -c 'if [ -r %s ]; then echo yes; else echo no; fi'\" 2> /dev/null",
		       soft_p->media_host, dev);

	pp = popen(buf, "r");
	if (pp == NULL) {
		menu_flash_off(REDISPLAY);
		menu_log("%s: cannot determine media path.", progname);
		return(-1);
	}

	bzero(buf, sizeof(buf));
	(void) fgets(buf, sizeof(buf), pp);
	buf[strlen(buf) - 1 ] = NULL;
	(void) pclose(pp);

	menu_flash_off(NOREDISPLAY);

	return(strcmp(buf, "yes") == 0 ? 1 : 0);
} /* end ck_remote_path() */

