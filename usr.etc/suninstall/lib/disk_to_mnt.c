#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)disk_to_mnt.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)disk_to_mnt.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		disk_to_mount_list()
 *
 *	Description:	Convert a disk information structure, 'disk_p',
 *		into a mount list file, 'name'.  This code checks for
 *		duplicate mount points and sorts the mount list via
 *		sort_mount_list().
 *
 *		Returns 1 if everything is okay, 0 if there were multiple
 *		mount points and -1 if there was an error.
 */

#include <string.h>
#include "install.h"
#include "menu.h"


/*
 *	External references:
 */
extern	char *		sprintf();




int
disk_to_mount_list(disk_p, name)
	disk_info *	disk_p;
	char *		name;
{
	int		i;			/* index variables */
	mnt_ent		mount_list[NMOUNT];	/* mount list buffer */
	mnt_ent *	mp;			/* mount entry pointer */
	char		partition[TINY_STR];	/* partition name */
	part_info *	pp;			/* partition pointer */


	/*
	 *	Read 'mount_list'.  Return code of 0 or 1 is okay.
	 *	Return of 0 means that the file does not exist.
	 */
	if (read_mount_list(name, mount_list) == -1)
		return(-1);

	for (i = 0; i < NDKMAP; i++) {
		pp = &disk_p->partitions[i];
		(void) sprintf(partition, "%s%c", disk_p->disk_name, i + 'a');

		/*
		 *	Check the mount list for a duplicate mount
		 *	point name.
		 */
		for (mp = mount_list; mp->partition[0]; mp++) {
			if (strcmp(mp->mount_pt, pp->mount_pt) == 0 &&
			    strcmp(partition, mp->partition) != 0) {
				menu_mesg("Multiple mount points for '%s'.",
					  pp->mount_pt);
				return(0);
			}
		}
					  
		/*
		 *	Find the right mount point entry, if any.
		 */
		for (mp = mount_list; mp < &mount_list[NMOUNT]; mp++)
			if (strcmp(partition, mp->partition) == 0)
				break;


		/*
		 *	Found the right mount point
		 */
		if (mp < &mount_list[NMOUNT]) {
			bzero((char *) mp, sizeof(*mp));

			if (pp->mount_pt[0]) {
				(void) strcpy(mp->mount_pt, pp->mount_pt);
				mp->preserve = pp->preserve_flag;
				mp->count = elem_count(pp->mount_pt);
			}

			continue;
		}

		/*
		 *	If there is no mount point or if there are no
		 *	blocks for this partition, then do not make
		 *	a mount entry.
		 */
		if (pp->mount_pt[0] == NULL || map_blk(i + 'a') == 0)
			continue;

		/*
		 *	At this point, the partition is not in the mount
		 *	list so we have to add it.  Find the right spot.
		 */
		for (mp = mount_list; mp->partition[0]; mp++)
			/* NULL statement */ ;

		if (mp >= &mount_list[NMOUNT]) {
			menu_log("%s: %s: too many mount points.", progname,
				 name);
			return(-1);
		}

		(void) strcpy(mp->partition, partition);
		(void) strcpy(mp->mount_pt, pp->mount_pt);
		mp->preserve = pp->preserve_flag;
#ifdef SunB1
		mp->fs_minlab = pp->fs_minlab;
		mp->fs_maxlab = pp->fs_maxlab;
#endif /* SunB1 */
		mp->count = elem_count(pp->mount_pt);
	}

	sort_mount_list(mount_list);

	/*
	 *	Save the new mount list
	 */
	return(save_mount_list(name, mount_list));
} /* end disk_to_mount_list() */
