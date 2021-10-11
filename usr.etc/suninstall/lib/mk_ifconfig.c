#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)mk_ifconfig.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)mk_ifconfig.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		mk_rc_ifconfig()
 *
 *	Description:	Make the RC_IFCONFIG file.  'sys_p' points to
 *		the information about the interfaces.  'path_prefix'
 *		is the prefix to the place to put the file.
 */

#include <errno.h>
#include <stdio.h>
#include "install.h"
#include "menu.h"


/*
 *	Local constants:
 */
static	char		IFCONFIG_CMD[] =
#ifdef SunB1
				"%s/sbin/ifconfig %s %s -label -trailers up\n";
#else
				       "%s/sbin/ifconfig %s %s -trailers up\n";
#endif /* SunB1 */



/*
 *	External functions:
 */
extern	char *		sprintf();


void
mk_rc_ifconfig(sys_p, path_prefix)
	sys_info *	sys_p;
	char *		path_prefix;
{
	char **		cpp;			/* ptr to RC_HEADER text */
	FILE *		fp;			/* ptr to RC_IFCONFIG file */
	int		i;			/* index variable */
	char		interface[TINY_STR];	/* interface name */
	char		pathname[MAXPATHLEN];	/* path to RC_IFCONFIG file */
	char *		prefix = "";		/* entry prefix */


	(void) sprintf(pathname, "%s%s", path_prefix, RC_IFCONFIG);

	fp = fopen(pathname, "w");
	if (fp == NULL) {
		menu_log("%s: %s: %s.", progname, pathname, err_mesg(errno));
		menu_log("\tCannot open file for writing.");
		menu_abort(1);
	}

	for (cpp = RC_HEADER; *cpp; cpp++)
		(void) fprintf(fp, "%s", *cpp);

	if (sys_p == NULL) {
		for (i = 1; i <= ETHER_MAX; i++) {
			(void) sprintf(interface, "%s0", cv_ether_to_str(&i));
			(void) fprintf(fp, IFCONFIG_CMD, "", interface, "");
		}
	}
	else if (sys_p->ether_type != ETHER_NONE) {
#ifdef SunB1
		if (sys_p->ip0_minlab == LAB_OTHER ||
		    sys_p->ip0_maxlab == LAB_OTHER)
			prefix ="#";
#endif /* SunB1 */

		(void) fprintf(fp, IFCONFIG_CMD, prefix, sys_p->ether_name0,
			       sys_p->hostname0);

		if (strlen(sys_p->ether_name1)) {
#ifdef SunB1
			if (sys_p->ip1_minlab == LAB_OTHER ||
			    sys_p->ip1_maxlab == LAB_OTHER)
				prefix ="#";
			else
				prefix = "";
#endif /* SunB1 */

			(void) fprintf(fp, IFCONFIG_CMD, prefix,
				       sys_p->ether_name1, sys_p->hostname1);
		}
	}

	(void) fclose(fp);
} /* end mk_rc_ifconfig() */
