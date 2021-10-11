#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)update_parts.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)update_parts.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		update_parts()
 *
 *	Description:	Update the partitions associated with 'disk_p'.
 *		The start cylinders and the static strings are updated.
 */

#include <string.h>
#include "install.h"

/*
 *	External functions:
 */
extern	char *		sprintf();


void
update_parts(disk_p)
	disk_info *	disk_p;
{
	int		i, j;			/* index variables */


	/*
	 *	Change start cylinders for everything
	 *
	 *	i indexes the partition that is changing
	 *	j indexes the partition that provides the start value
	 */
	for (i = 1; i < NDKMAP; i++) {
		/*
		 *	If this partition has disk space associated with it,
		 *	then change the start cylinder.
		 */
		if ('a' + i != 'c' && map_blk(i + 'a')) {
			/*
			 * If the partition is preserved then don't
			 * touch it
			 */
			if (disk_p->partitions[i].preserve_flag == 'y')
				continue;
			/*
			 *	Move backward through the partition table
			 *	to find the first previous partition with
			 *	disk space.  This partition provides the
			 *	starting cylinder.  We always use the 0th
			 *	partition if we reach it.
			 */
			for (j = i - 1; j > 0; j--) { 
				if (map_blk(j + 'a') && 'a' + j != 'c')
					break;
			}
			map_cyl(i + 'a') = map_cyl(j + 'a') +
			    blocks_to_cyls(map_blk(j + 'a'), disk_p->geom_buf);
		}
	}

	/*
	 *	Update the strings associated with the numbers.  This loop
	 *	explicitly does not update the field 'avail_bytes' since
	 *	that field is initialized in get_disk_info and is used
	 *	by get_software_info.
	 */
	for (i = 0; i < NDKMAP; i++) {
                (void) sprintf(disk_p->partitions[i].start_str, "%ld",
                               map_cyl(i + 'a'));
                (void) sprintf(disk_p->partitions[i].block_str, "%ld",
                               map_blk(i + 'a'));
                (void) strcpy(disk_p->partitions[i].size_str,
                              blocks_to_str(disk_p, map_blk(i + 'a')));
	}
} /* end update_parts() */
