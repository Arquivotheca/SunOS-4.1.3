#ifndef lint
static	char		mls_sccsid[] = "@(#)add_net_lab.c 1.1 92/07/30 SMI; SunOS MLS";
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		add_net_label()
 *
 *	Description:	Add an entry to the NET_LABEL file.  Adds a keyed
 *		entry with a key of 'ip' and a value of 'minlab maxlab'
 *		to 'path_prefix'/NET_LABEL.  The path prefix is used to
 *		specify which NET_LABEL file to update (MINIROOT, or client).
 *		The NET_LABEL file on the MINIROOT is copied to the root
 *		mount point by 'installation'.
 */

#include <stdio.h>
#include <string.h>
#include "install.h"

/*
 *	External functions:
 */
extern	char *		sprintf();


void
add_net_label(ip, minlab, maxlab, path_prefix)
	char *		ip;
	int		minlab;
	int		maxlab;
	char *		path_prefix;
{
	char *		cp;			/* scratch char ptr */
	char		key[SMALL_STR];		/* buffer for key to add */
	char		pathname[MAXPATHLEN];	/* path to NET_LABEL */
	char *		prefix = "";		/* prefix for entry */
	char		net_str[SMALL_STR];	/* network string */
	char		value[MEDIUM_STR];	/* buffer for value to add */


	/*
	 *	Strip off the last numeric field in the IP address.  This is
	 *	the network number.
	 */
	(void) strcpy(net_str, ip);
	if (cp = strrchr(net_str, '.'))
		*cp = NULL;

	/*
	 *	If either the minimum label or maximum label is LAB_OTHER,
	 *	the the entry must be commented out.
	 */
	if (minlab == LAB_OTHER || maxlab == LAB_OTHER)
		prefix = "#";

	(void) sprintf(key, "%s%s", prefix, net_str);

	/*
	 *	Have to build the value in pieces since the buffer returned
	 *	by cv_lab_to_str() is static.
	 */
	(void) strcpy(value, cv_lab_to_str(&minlab));
	(void) strcat(value, " ");
	(void) strcat(value, cv_lab_to_str(&maxlab));

	(void) sprintf(pathname, "%s%s", path_prefix, NET_LABEL);
	add_key_entry(key, value, pathname, KEY_ONLY);
} /* end add_net_label() */
