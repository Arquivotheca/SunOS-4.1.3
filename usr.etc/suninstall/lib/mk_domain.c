#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)mk_domain.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)mk_domain.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		mk_rc_domainname()
 *
 *	Description:	Make the RC_DOMAINNAME file.  'domainname' is what to
 *		put in the file.  'prefix' is the prefix to the place to put
 *		the file.
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
mk_rc_domainname(domainname, prefix)
	char *		domainname;
	char *		prefix;
{
	FILE *		fp;			/* ptr to RC_DOMAINNAME file */
	char		pathname[MAXPATHLEN];	/* path to RC_DOMAINNAME */


	(void) sprintf(pathname, "%s%s", prefix, RC_DOMAINNAME);

	fp = fopen(pathname, "w");
	if (fp == NULL) {
		menu_log("%s: %s: %s.", progname, pathname, err_mesg(errno));
		menu_log("\tCannot open file for writing.");
		menu_abort(1);
	}

	if (domainname == NULL || *domainname == NULL)
		(void) fprintf(fp, "noname\n");
	else
		(void) fprintf(fp, "%s\n", domainname);

	(void) fclose(fp);
} /* end mk_rc_domainname() */
