#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)find_arch.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)find_arch.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		find_arch()
 *
 *	Description:	Find the architecture matching 'arch_code' in
 *		the array of architecture information, 'arch_p'.  If
 *		a match is found, then a pointer to the matching
 *		structures is returned otherwise NULL is returned.
 */

#include "install.h"


arch_info *
find_arch(arch_code, arch_p)
	char *		arch_code;
	arch_info *	arch_p;
{
	arch_info *ap;

	for (ap = arch_p; ap ; ap = ap->next)
		if (strcmp(ap->arch_str, arch_code) == 0)
			return(ap);

	return(NULL);
} /* end find_arch() */
