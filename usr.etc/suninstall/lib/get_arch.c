#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)get_arch.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)get_arch.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		get_arch()
 *
 *	Description:	Return the architecture string for this system.
 *		Returns NULL if there is an error.
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"


/*
 *	External functions:
 */
extern	char *		sprintf();


char *
get_arch()
{
	FILE *		pp;			/* ptr to arch process */

	static	char		buf[MEDIUM_STR]; /* architecture string */


	bzero(buf, sizeof(buf));

	pp = popen("/bin/arch -k","r");
	if (pp) {
		(void) fscanf(pp,"%s",buf);
		(void) pclose(pp);
	}

	if (strlen(buf) == 0)
		menu_log("%s: invalid system architecture.", progname);

	return(buf);
} /* end get_arch() */
