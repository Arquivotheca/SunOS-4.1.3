#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)find_mf_part.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)find_mf_part.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		find_mf_part()
 *
 *	Description:	Find the partition associated with given media file.
 *		'soft_p' points to software information, 'mf_p' points
 *		to media file information and 'sys_p' points to system
 *		information.
 */

#include "install.h"



char *
find_mf_part(soft_p, mf_p, sys_p)
	soft_info *	soft_p;
	media_file *	mf_p;
	sys_info *	sys_p;
{
	char *		part_name;		/* partition name to return */


	if (strcmp(mf_p->mf_loadpt, "root") == 0)
	    if (is_miniroot() && sys_p->sys_type != SYS_SERVER)
		part_name = sys_p->root;
	    else
		part_name = find_part(soft_p->share_path);

	else if (strcmp(mf_p->mf_loadpt, "impl") == 0)
		part_name = find_part(soft_p->kvm_path);

	else if (strcmp(mf_p->mf_loadpt, "share") == 0)
		part_name = find_part(soft_p->share_path);

	else if (strcmp(mf_p->mf_loadpt, "appl") == 0)
		part_name = find_part(soft_p->exec_path);

	else
		part_name = find_part(soft_p->exec_path);

	return(part_name);
} /* end find_mf_part() */
