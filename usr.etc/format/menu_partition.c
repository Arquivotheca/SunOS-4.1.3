
#ifndef lint
static  char sccsid[] = "@(#)menu_partition.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This file contains functions to implement the partition menu commands.
 */
#include "global.h"

/*
 * This routine implements the 'a' command.  It changes the 'a' partition.
 */
int
p_apart()
{

	change_partition(0);
	return (0);
}

/*
 * This routine implements the 'b' command.  It changes the 'b' partition.
 */
int
p_bpart()
{

	change_partition(1);
	return (0);
}

/*
 * This routine implements the 'c' command.  It changes the 'c' partition.
 */
int
p_cpart()
{

	change_partition(2);
	return (0);
}

/*
 * This routine implements the 'd' command.  It changes the 'd' partition.
 */
int
p_dpart()
{

	change_partition(3);
	return (0);
}

/*
 * This routine implements the 'e' command.  It changes the 'e' partition.
 */
int
p_epart()
{

	change_partition(4);
	return (0);
}

/*
 * This routine implements the 'f' command.  It changes the 'f' partition.
 */
int
p_fpart()
{

	change_partition(5);
	return (0);
}

/*
 * This routine implements the 'g' command.  It changes the 'g' partition.
 */
int
p_gpart()
{

	change_partition(6);
	return (0);
}

/*
 * This routine implements the 'h' command.  It changes the 'h' partition.
 */
int
p_hpart()
{

	change_partition(7);
	return (0);
}

/*
 * This routine implements the 'load' command.  It allows the user
 * to make a pre-defined partition map the current map.
 */
int
p_load()
{
	register struct partition_info *pptr, *parts;
	struct	bounds bounds;
	int i, index, deflt, *defltptr = NULL;

	parts = cur_dtype->dtype_plist;
	/*
	 * If there are no pre-defined maps for this disk type, it's
	 * an error.
	 */
	if (parts == NULL) {
		eprint("No defined partition tables.\n");
		return (-1);
	}
	/*
	 * Loop through the pre-defined maps and list them by name.  If
	 * the current map is one of them, make it the default.  If any
	 * the maps are unnamed, label them as such.
	 */
	for (i = 0, pptr = parts; pptr != NULL; pptr = pptr->pinfo_next) {
		if (cur_parts == pptr) {
			deflt = i;
			defltptr = &deflt;
		}
		if (pptr->pinfo_name == NULL)
			printf("        %d. unnamed\n", i++);
		else
			printf("        %d. %s\n", i++, pptr->pinfo_name);
	}
	bounds.lower = 0;
	bounds.upper = i - 1;
	/*
	 * Ask which map should be made current.
	 */
	index = input(FIO_INT, "Specify table (enter its number)", ':',
	    (char *)&bounds, defltptr, DATA_INPUT);
	for (i = 0, pptr = parts; i < index; i++, pptr = pptr->pinfo_next)
		;
	/*
	 * Before we blow the current map away, do some limits checking. 
	 */
	for( i = 0; i < NDKMAP; i++)  {
		if( pptr->pinfo_map[i].dkl_cylno < 0 || 
			pptr->pinfo_map[i].dkl_cylno > ncyl-1) {
			eprint("partition %c: starting cylinder %d is out of range\n", 
				('a'+i), pptr->pinfo_map[i].dkl_cylno);
			return(0);
		}
		if( pptr->pinfo_map[i].dkl_nblk < 0 || pptr->pinfo_map[i].dkl_nblk > 
				(ncyl - pptr->pinfo_map[i].dkl_cylno) * spc()) {
			eprint("partition %c: specified # of blocks, %d, is out of range\n",
				('a'+i), pptr->pinfo_map[i].dkl_nblk );
			return(0);
		}
	}
	/*
	 * Lock out interrupts so the lists don't get mangled.
	 */
	enter_critical();
	/*
	 * If the old current map is unnamed, delete it.
	 */
	if (cur_parts != NULL && cur_parts != pptr &&
	    cur_parts->pinfo_name == NULL)
		delete_partition(cur_parts);
	/*
	 * Make the selected map current.
	 */
	cur_disk->disk_parts = cur_parts = pptr;
	exit_critical();
	printf("\n");
	return (0);
}

/*
 * This routine implements the 'name' command.  It allows the user
 * to name the current partition map.  If the map was already named,
 * the name is changed.  Once a map is named, the values of the partitions
 * cannot be changed.  Attempts to change them will cause another map
 * to be created.
 */
int
p_name()
{
	char	*name;

	/*
	 * Ask for the name.  Note that the input routine will malloc
	 * space for the name since we are using the OSTR input type.
	 */
	name = (char *)input(FIO_OSTR, "Enter table name (remember quotes)",
	    ':', (char *)NULL, (int *)NULL, DATA_INPUT);
	/*
	 * Lock out interrupts.
	 */
	enter_critical();
	/*
	 * If it was already named, destroy the old name.
	 */
	if (cur_parts->pinfo_name != NULL)
		destroy_data(cur_parts->pinfo_name);
	/*
	 * Set the name.
	 */
	cur_parts->pinfo_name = name;
	exit_critical();
	printf("\n");
	return (0);
}

/*
 * This routine implements the 'print' command.  It lists the values
 * for all the partitions in the current partition map.
 */
int
p_print()
{
	int i;

	/*
	 * Print the name of the current map.
	 */
	if (cur_parts->pinfo_name != NULL)
		printf("Current partition table (%s):\n",
		    cur_parts->pinfo_name);
	else
		printf("Current partition table (unnamed):\n");
	/*
	 * Loop through each partition.  List the values of the partition,
	 * displaying the block count in cylinder/head/sector format too.
	 */
	for (i = 0; i < NDKMAP; i++) {
		printf("        partition %c - ", 'a' + i);
		printf("starting cyl %6d, ", cur_parts->pinfo_map[i].dkl_cylno);
		printf("# blocks %8d (", cur_parts->pinfo_map[i].dkl_nblk);
		pr_dblock(printf, cur_parts->pinfo_map[i].dkl_nblk);
		printf(")\n");
	}
	printf("\n");
	return (0);
}
