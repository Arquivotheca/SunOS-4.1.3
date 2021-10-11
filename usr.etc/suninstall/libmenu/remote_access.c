#ifndef lint
#ifdef SunB1
#ident			"@(#)remote_access.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)remote_access.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		remote_access()
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
	char	buf[80 + MAXHOSTNAMELEN];


	(void) sprintf(buf, "rsh %s -n ls /dev > /dev/null 2>&1",
		       remote_host);

	if (system(buf) == 0)
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
			


