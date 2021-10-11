#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)update_bytes.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)update_bytes.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		update_bytes()
 *
 *	Description:	Update the bytes available field associated with
 *		the partition 'disk_name'.  'size' is decremented from
 *		the available bytes field.  If there is insufficient space
 *		in that partition, then space is taken from the free hog.
 *		If there is no space left in the free hog or the requested
 *		partition is the free hog, then 0 is returned.  If space
 *		is available, then 1 is returned.  If 'size' is a negative
 *		number, then the space available is incremented.  Returns
 *		-1 if an error occurs.
 */

#include <string.h>
#include "install.h"
#include "menu.h"


extern	char *		sprintf();


int
update_bytes(disk_name, size)
	char *		disk_name;
	long		size;
{
	disk_info	disk;			/* disk information */
	disk_info *	disk_p = &disk;		/* needed for macros */
	part_info *	hog_p;			/* ptr to free hog partition */
	long		need_blocks;		/* number of blocks needed */
	long		need_bytes;		/* number of bytes needed */
	char		part;			/* target partition */
	part_info *	part_p;			/* ptr to target partition */
	char		pathname[MAXPATHLEN];	/* pathname buffer */
	int		ret_code;		/* return code */
	int		i,j;			/* indexes */
	char		preserved_part[TINY_STR]; /* preserved partition */


	bzero(disk_p->disk_name, sizeof(disk_p->disk_name));
	(void) strncpy(disk_p->disk_name, disk_name, strlen(disk_name) - 1);

	(void) sprintf(pathname, "%s.%s", DISK_INFO, disk_p->disk_name);
	ret_code = read_disk_info(pathname, disk_p);
	/*
	 *      Only a return of 1 is okay here.  This file is not optional.
	 *	read_disk_info() prints the error message for ret_code == -1
	 *	so we have to print one for ret_code == 0.
	 */
	if (ret_code != 1) {
		if (ret_code == 0)
			menu_log("%s: cannot read file.", pathname);
		return(-1);
	}

	if (get_disk_config(disk_p) != 1)
		return(-1);

	/*
	 *	Set up all the shorthand pointers.
	 */
	hog_p = &disk_p->partitions[disk_p->free_hog - 'a'];
	part = disk_name[strlen(disk_name) - 1];
	part_p = &disk_p->partitions[part - 'a'];


	/*
	 *	This is the FREE HOG partitition
	 */
	if (part == disk_p->free_hog) {
		if (size > hog_p->avail_bytes) {
			menu_mesg("Not enough space in free hog partition.");
			return(0);
		}

		part_p->avail_bytes -= size;
	}
	/*
	 *	Partition has enough space
	 */
	else if (size <= part_p->avail_bytes) {
		part_p->avail_bytes -= size;
	}
	/*
	 *	Partition needs space from the FREE HOG
	 */
	else {
		/*
		 *	If we are not on the miniroot or if we are supposed
		 *      to use the existing partition table or if the free
		 *	hog is preserved or if the partition is preserved,
		 *	then we cannot allow changes.
		 */
		if (!is_miniroot() || disk_p->label_source == DKL_EXISTING ) {
			menu_mesg("Not enough space in %s.", disk_name);
			return(0);
		}
		if ( hog_p->preserve_flag == 'y' ) {
			menu_mesg("Not enough space in %s.  Can't use a preserved free hog.", disk_name);
			return(0);
		}
		if (part_p->preserve_flag == 'y') {
			menu_mesg("Not enough space in %s.  Can't modify a preserved partition.", disk_name);
			return(0);
		}
		/*
		 * Can't use the free hog if there is a preserved partition
		 * between the partion in question and the free hog
		 */
		i = disk_name[strlen(disk_name) - 1] - 'a';
		j = disk_p->free_hog - 'a';
		if ( i > j) {
			i = j;
			j = disk_name[strlen(disk_name) - 1] - 'a';
		}
		for ( ; i < j ; i++) {
			if (disk_p->partitions[i].preserve_flag != 'y')
				continue;
			strcpy(preserved_part,disk_name);
			preserved_part[strlen(preserved_part) -1] = 'a' + i;
			menu_mesg("Not enough space in %s.  Can't use freehog because %s is preserved", disk_name, preserved_part);
			return(0);
		}

		/*
		 *	Calculate the number of bytes needed and round up to
		 *	the next block boundary.
		 */
		need_bytes = size - part_p->avail_bytes;
		need_blocks = bytes_to_blocks(need_bytes, disk_p->geom_buf);
		need_bytes = blocks_to_bytes(need_blocks);

		/*
		 *	Not enough space
		 */
		if (need_bytes > hog_p->avail_bytes) {
			menu_mesg("Not enough space in free hog partition.");
			return(0);
		}

		/*
		 *	Update the FREE HOG partition
		 */
		map_blk(disk_p->free_hog) -= need_blocks;
		hog_p->avail_bytes -= need_bytes;

		/*
		 *	If we have zeroed out the free hog,
		 *	then there is no need for the mount point.
		 */
		if (map_blk(disk_p->free_hog) == 0)
			hog_p->mount_pt[0] = NULL;

		/*
		 *	Update the target partition with the needed space
		 */
		map_blk(part) += need_blocks;
		part_p->avail_bytes += need_bytes;

		/*
		 *	Now grab the space
		 */
		part_p->avail_bytes -= size;

		update_parts(disk_p);
	}

	if (save_disk_info(pathname, disk_p) != 1)
		return(-1);

	(void) sprintf(pathname, "%s.%s", CMDFILE, disk_p->disk_name);
	if (save_cmdfile(pathname, disk_p) != 1)
		return(-1);

	return(1);
} /* end update_bytes() */
