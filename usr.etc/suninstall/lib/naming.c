#ifndef lint
#ifdef SunB1
#ident			"@(#)naming.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)naming.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		naming.c
 *
 *	Description:	This file contains all the routines necessary for
 *			consistent user interface prompts and the like.
 *
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"

/*
 *	External functions:
 */
extern	char *		sprintf();


/*
 *	Name:		please_check()
 *
 *	Description:	This function builds a string to check media devices.
 */

void
please_check(soft_p)
	soft_info	*soft_p;
{

	char	string[MEDIUM_STR + MAXHOSTNAMELEN + 80];
	
	switch(soft_p->media_loc) {
	case LOC_REMOTE	:
		/*
		 *	The device is remote
		 */
		(void) sprintf(string,
			"Please check remote media device %s on host '%s'",
			       soft_p->media_path,
			       soft_p->media_host);
		break;
	case LOC_LOCAL	:
	default		:
		/*
		 *	The device is local
		 */

		(void) sprintf(string, "Please check local media device %s",
			       soft_p->media_path);
		break;
	}
	menu_mesg(string);
}


/*
 *	Name:		os_name()
 *
 *	Description:	This function returns a char * to a static string
 *			holding the formated release name to use everywhere.
 */
char *
os_name(arch_str)
	char	*arch_str;
{
	Os_ident	os;
	static 	char	string[sizeof(Os_ident)];

	(void) fill_os_ident(&os, arch_str);

	(void) sprintf(string, "%s %s%s %s",
		       os.os_name,
		       os.release,
		       os.realization,
		       os.impl_arch);

	return(string);
}
		       


/*
 *	Name:		mount_string()
 *
 *	Description:	This function builds names for mounted tapes.
 */

char	*
mount_string(soft_p)
	soft_info	*soft_p;
{

	static	char	string[sizeof(Os_ident) + 64];
		
	(void) sprintf(string, "%s release media volume %d",
		       os_name(soft_p->arch_str),
		       soft_p->media_vol);

	return(string);
}

/*
 *	Name:		check_valid_release()
 *
 *	Description:	This function determines if we don't support the
 *			specific tape.  Currently we only say we don't support
 *			4.0 tapes.
 *
 */
check_valid_release(arch_str)
	char	*arch_str;
{
	Os_ident	os;
	static  char	string[sizeof(Os_ident)];

	(void) fill_os_ident(&os, arch_str);

	if (strncmp("4.0",os.release,3) == 0)
		return(0);
	return(1);
}

