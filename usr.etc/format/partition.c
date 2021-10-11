
#ifndef lint
static	char sccsid[] = "@(#)partition.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This file contains functions that operate on partition tables.
 */
#include "global.h"

/*
 * This routine allows the user to change the boundaries of the given
 * partition in the current partition map.
 */
change_partition(num)
	int num;
{
	struct	bounds bounds;
	int i, j, deflt;

	/*
	 * Print out the given partiton so the user knows what he's
	 * getting into.
	 */
	printf("\n        partition %c - ", 'a' + num);
	printf("starting cyl %6d, ", cur_parts->pinfo_map[num].dkl_cylno);
	printf("# blocks %8d (", cur_parts->pinfo_map[num].dkl_nblk);
	pr_dblock(printf, cur_parts->pinfo_map[num].dkl_nblk);
	printf(")\n\n");
	/*
	 * Ask for the new values.  The old values are the defaults, and
	 * strict bounds checking is done on the values given.
	 */
	bounds.lower = 0;
	bounds.upper = ncyl - 1;
	deflt = cur_parts->pinfo_map[num].dkl_cylno;
	i = input(FIO_INT, "Enter new starting cyl", ':', (char *)&bounds,
	    &deflt, DATA_INPUT);
	bounds.upper = (ncyl - i) * spc();
	deflt = min(cur_parts->pinfo_map[num].dkl_nblk, bounds.upper);
	j = input(FIO_BN, "Enter new # blocks", ':', (char *)&bounds,
	    &deflt, DATA_INPUT);
	/*
	 * If the current partition is named, we can't change it.  We
	 * create a new current partition map instead.
	 */
	if (cur_parts->pinfo_name != NULL)
		make_partition();
	/*
	 * Change the values.
	 */
	cur_parts->pinfo_map[num].dkl_cylno = i;
	cur_parts->pinfo_map[num].dkl_nblk = j;
}

/*
 * This routine picks to closest partition table which matches the
 * selected disk type.  It is called each time the disk type is
 * changed.  If not match is found, it usese the first element
 * of the partition table.  If no table exists, a dummy is
 * created.
 */
get_partition()
{
	register struct partition_info *pptr, *parts;
	int i;

#ifdef	lint
	i = 0;
	i = i;
#endif	lint

	/*
	 * If there are no pre-defined maps for this disk type, it's
	 * an error.
	 */
	parts = cur_dtype->dtype_plist;
	if (parts == NULL) {
		eprint("No defined partition tables.\n");
		make_partition();
		return (-1);
	}
	/*
	 * Loop through the pre-defined maps searching for one which match
	 * disk type.  If found copy it into unmamed partition.
	 */
	enter_critical();
	for (i = 0, pptr = parts; pptr != NULL; pptr = pptr->pinfo_next) {
		if (pptr->pinfo_name != NULL &&
		    strcmp(pptr->pinfo_name, cur_dtype->dtype_asciilabel) == 0) {
			/*
			 * Set current partition and name it.
			 */
			cur_disk->disk_parts = cur_parts = pptr;
			cur_parts->pinfo_name = pptr->pinfo_name;
			exit_critical();
			return (0);
		}
	}
	/*
	 * If we couldn't find a match, take the first one.
	 * Set current partition and name it.
	 */
	cur_disk->disk_parts = cur_parts = cur_dtype->dtype_plist;
	cur_parts->pinfo_name = parts->pinfo_name;
	exit_critical();
	return (0);
}
/*
 * This routine creates a new partition map and sets it current.  If there
 * was a current map, the new map starts out identical to it.  Otherwise
 * the new map starts out all zeroes.
 */
make_partition()
{
	register struct partition_info *pptr, *parts;
	int	i;

	/*
	 * Lock out interrupts so the lists don't get mangled.
	 */
	enter_critical();
	/*
	 * Get space for for the new map and link it into the list
	 * of maps for the current disk type.
	 */
	pptr = (struct partition_info *)zalloc(sizeof (*pptr));
	parts = cur_dtype->dtype_plist;
	if (parts == NULL) {
		cur_dtype->dtype_plist = pptr;
	} else {
		while (parts->pinfo_next != NULL) {
			parts = parts->pinfo_next;
		}
		parts->pinfo_next = pptr;
		pptr->pinfo_next = NULL;
	}
	/*
	 * If there was a current map, copy its values.
	 */
	if (cur_parts != NULL)
		for (i = 0; i < NDKMAP; i++)
			pptr->pinfo_map[i] = cur_parts->pinfo_map[i];
	/*
	 * Make the new one current.
	 */
	cur_disk->disk_parts = cur_parts = pptr;
	exit_critical();
}

/*
 * This routine deletes a partition map from the list of maps for
 * the given disk type.
 */
delete_partition(parts)
	struct	partition_info *parts;
{
	struct	partition_info *pptr;

	/*
	 * If there isn't a current map, it's an error.
	 */
	if (cur_dtype->dtype_plist == NULL) {
		eprint("Error: unexpected null partition list.\n");
		fullabort();
	}
	/*
	 * Remove the map from the list.
	 */
	if (cur_dtype->dtype_plist == parts)
		cur_dtype->dtype_plist = parts->pinfo_next;
	else {
		for (pptr = cur_dtype->dtype_plist; pptr->pinfo_next != parts;
		    pptr = pptr->pinfo_next)
			;
		pptr->pinfo_next = parts->pinfo_next;
	}
	/*
	 * Free the space it was using.
	 */
	destroy_data((char *)parts);
}
