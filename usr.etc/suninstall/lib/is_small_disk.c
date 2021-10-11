#ifndef lint
#ifdef SunB1
#ident			"@(#)is_small_disk.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)is_small_disk.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		is_small_disk.c
 *
 *	Description: This file contains routines to help decide if the
 *		disk should be partitioned in 2 or 3 partitions by default.
 */

#include "install.h"
#include "disk_impl.h"

/****************************************************************************
**  Function : (int) is_small_size()
**
** Return Value :  1  : if the hot_disk is smaller that SMALL_DSK_BYTES
**		   0  : if this is not a hot disk or a hot disk > ^
**
** Description : Function to tell whether the disk is smaller than
**		SMALL_DSK_BYTES
**
****************************************************************************
*/

int
is_small_size(disk_p)
	disk_info	*disk_p;
{
	if (!disk_p->is_hot)
		return(0); 	/* not to partition other disks as small */


	if (blocks_to_bytes(map_blk('c')) <= SMALL_DSK_BYTES)
		return(1);

	return(0);
}



/****************************************************************************
**  Function : (int) partitions()
**
** Return Value :  TWO_PARTS 	: If the disk should use 2 partitions
**		   THREE_PARTS	: If the disk should use 3 partitions
**
** Description : This function reads the mountlist and sees if only "/usr"
**		  and "/" are partions.  If they are, we return TWO_PARTS,
**		  else  we return THREE_PARTS.
**
****************************************************************************
*/

int
partitions()
{
	mnt_ent		mount_list[NMOUNT];
	int		i;
	int		root_usr = 0;
	static short	checked = 0;		
	static int	num_mounts = 0;
	
	/*
	 *	only read the mount list once
	 */
	if (checked)
		return(num_mounts);

	/*
	 *	Read the mount_list
	 */
	if (read_mount_list(MOUNT_LIST, mount_list) != 1) {
		menu_log("could not read mount list\n");
		menu_abort(1);
	}

	/*
	 *	count up all mount points and see if only "/usr" and "/" exist
	 */

	for (i = 0; i < NMOUNT; i++)  {
		if (strcmp(mount_list[i].mount_pt, "/") == 0)
			root_usr++;
		else if (strcmp(mount_list[i].mount_pt, "/usr") == 0)
			root_usr++;
		else if (strcmp(mount_list[i].mount_pt, "/home") == 0) {
			root_usr = num_mounts + 1;
			break;
		} else if (strcmp(mount_list[i].mount_pt, "") == 0)
			continue;

		num_mounts++;
	}

	checked = 1;
	
	if (num_mounts == root_usr)
		return(num_mounts = TWO_PARTS);
	else
		return(num_mounts = THREE_PARTS);
		
}


