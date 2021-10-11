#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)mk_hostname.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)mk_hostname.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		mk_rc_hostname()
 *
 *	Description:	Make the RC_HOSTNAME file.  'hostname' is what to put
 *		in the file.  'prefix' is the prefix to the place to put the
 *		file. and the 'ether_name' is the ethernet type of system
 *		(i.e. le0, ie0, etc.) 
 */

#include <errno.h>
#include <stdio.h>
#include "install.h"
#include "menu.h"


/*
 *	External functions:
 */
extern	char *		sprintf();


void
mk_rc_hostname(hostname, prefix, ether_name)
	char *		hostname;
	char *		prefix;
	char *		ether_name;
{
	FILE *		fp;			/* ptr to RC_HOSTNAME file */
	char		pathname[MAXPATHLEN];	/* path to RC_HOSTNAME file */


	/*
	** if the ether_name is a NULL string, then we assume there is no
	** ethernet for this interface and append a xx0 to the hostname
	** file.
	*/
	if (strlen(ether_name) == 0)
		(void) sprintf(pathname, "%s%s.xx0", prefix, RC_HOSTNAME);
	else
		(void) sprintf(pathname, "%s%s.%s", prefix, RC_HOSTNAME,
			       ether_name);

	fp = fopen(pathname, "w");
	if (fp == NULL) {
		menu_log("%s: %s: %s.", progname, pathname, err_mesg(errno));
		menu_log("\tCannot open file for writing.");
		menu_abort(1);
	}

	(void) fprintf(fp, "%s\n", hostname);

	(void) fclose(fp);
} /* end mk_rc_hostname() */




