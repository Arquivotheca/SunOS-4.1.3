#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)arch_info.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)arch_info.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		arch_info.c
 *
 *	Description:	This file contains the routines necessary for
 *		interfacing with the 'arch_info' structure file.  The
 *		layout of a 'arch_info' file is as follows:
 *
 *		aprid, such as sun3.sun3x.sunos.4.1.0.r
 */

#include <stdio.h>
#include "install.h"


/*
 *	Name:		read_arch_info()
 *
 *	Description:	Read architecture information from 'name' into
 *		the linked list pointed to by 'arch_p'.  The calling 
 *		program must do a free operation when they are done.
 *
 *		Returns 1 if the file was read successfully, 0 if the file
 *		did not exist and -1 if there was an error.
 */

int
read_arch_info(name, arch_p)
	char *		name;
	arch_info **	arch_p;
{
	char		buf[BUFSIZ];		/* buffer for I/O */
	arch_info	*root, *ap;		/* pointer to arch_info */
	Os_ident	os;
	FILE *		fp;			/* file pointer for name */
	*arch_p = root = NULL;
	fp = fopen(name, "r");
	if (fp == NULL)	
		return (0);
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (fill_os_ident(&os,buf) > 0)	{
			if ((ap = (arch_info *) 
				malloc(sizeof(arch_info))) == NULL) {
				menu_log("%s: %s: memory allocation fail.",
					 progname, name);
				return(-1);
			}
			(void)	os_aprid(&os, ap->arch_str);
			ap->os = os;
			ap->next = root;
			root = ap;
		}
		else	{
			menu_log("%s: %s: unvalid format found.",
				progname, name);
			return(-1);
		}
	}
	(void) fclose(fp);
	*arch_p = root;
	return(1);
} /* end read_arch_info() */



/*
 *	Name:		save_arch_info()
 *
 *	Description:	Save the architecture information in the buffer
 *		pointed to by 'arch_p' into 'name'. 
 *
 *		Returns 1 if the file was saved successfully, and -1 if
 *		there was an error.
 */

int
save_arch_info(name, arch_p)
	char *		name;
	arch_info *	arch_p;
{
	FILE *		fp;			/* file pointer for name */
	arch_info	*ap;

	fp = fopen(name, "w");
	if (fp == NULL) {
		menu_log("%s: %s: cannot open file for writing.", progname,
			 name);
		return(-1);
	}

	for (ap = arch_p; ap; ap = ap->next) {
		(void) fprintf(fp, "%s\n", ap->arch_str);
	}

	(void) fclose(fp);

	return(1);
} /* end save_arch_info() */


/*
 *	Name:		free_arch_info()
 *
 *	Description:	Save the architecture information in the buffer
 *		pointed to by 'arch_p' into 'name'. 
 *
 *		Returns 1 if the file was saved successfully, and -1 if
 *		there was an error.
 */

void
free_arch_info(arch_p)
	arch_info *	arch_p;
{
	arch_info	*ap,*tmp;

	for (ap = arch_p; ap; ap = tmp) {
		tmp = ap->next;
		free((char *)ap);
	}
} /* end free_arch_info() */
