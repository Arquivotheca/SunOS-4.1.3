#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)update_arch.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)update_arch.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		update_arch()
 *
 *	Description:	Update the 'exec_path' and 'kvm_path' fields
 *		associated with 'arch_code' in the ARCH_INFO file.
 *
 *		Returns 1 if everything is okay, and -1 if there was an error.
 */

#include <string.h>
#include "install.h"
#include "menu.h"


int
update_arch(arch_str, filename)
	char *		arch_str;
	char *		filename;
{
	arch_info *	arch_list;		/* architecture info */
	arch_info	*ap, *endp;		/* scratch pointer */


	if (read_arch_info(filename, &arch_list) == -1)
		/*
		 *	Only fail if arch_info exists and could not be read.
		 */
		return(-1);

	for (endp = ap = arch_list; ap ; endp = ap, ap = ap->next)
		if (strcmp(ap->arch_str,arch_str) == 0) {
			free_arch_info(arch_list);
			return(1);
		}

	if ((ap = (arch_info *) malloc(sizeof(arch_info))) == NULL) {
		menu_log("update_arch: memory allocation error");
		return(-1);
	}
	(void) strcpy(ap->arch_str, arch_str);
	ap->next = NULL;
	if (endp == NULL)    
		arch_list = ap;
	else
		endp->next = ap;

	if (save_arch_info(filename, arch_list) == -1) {
		free_arch_info(arch_list);
		return(-1);
	}
	free_arch_info(arch_list);
	return(1);
} /* end update_arch() */
