#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)disk_config.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)disk_config.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		get_disk_config()
 *
 *	Description:	Get the configuration for the disk pointed to by
 *		'disk_p'.  The configuration includes the disk geometry
 *		and the disk's hot status.  A disk is 'hot' if the system
 *		is currently booted on it.
 *
 *		Returns 1 if everything is okay, and -1 if there was an error.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include "install.h"
#include "menu.h"


extern	char *		sprintf();


int
get_disk_config(disk_p)
	disk_info *	disk_p;
{
	int		fd;			/* descriptor for disk */
	char		pathname[MAXPATHLEN];	/* path to disk_info */
	struct stat	stat_buf;		/* file info buffer */

	static	int	have_root = 0;		/* already stat'ed root? */
	static	dev_t	root_dev;		/* root device */


	if (have_root == 0) {			/* need root's device # */
		(void) stat("/", &stat_buf);
		root_dev = stat_buf.st_dev;
		have_root = 1;
	}

	/*
	 *	Determine if the disk we selected is the disk we are
	 *	booted on.  This is done by comparing the major device
	 *	number of "/" and the device, and comparing the minor
	 *	device number of "/" with the range for the disk we are
	 *	accessing.
	 */
	(void) sprintf(pathname, "/dev/%sc", disk_p->disk_name);
	if (stat(pathname, &stat_buf) != -1 &&
	    major(stat_buf.st_rdev) == major(root_dev) &&
	    minor(root_dev) >= minor(stat_buf.st_rdev) - 2 &&
	    minor(root_dev) < minor(stat_buf.st_rdev) - 2 + NDKMAP)
		disk_p->is_hot = 1;
	else
		disk_p->is_hot = 0;


	/*
	 *	Get this disk's geometry and store it away.
	 */
	(void) sprintf(pathname, "/dev/r%sc", disk_p->disk_name);
	fd = open(pathname, 0);
	if (fd < 0) {
		menu_mesg("%s: cannot open device.", pathname);
		return(-1);
	}
	if (ioctl(fd, DKIOCGGEOM, &disk_p->geom_buf) == -1) {
		(void) close(fd);
		menu_mesg("%s: unable to get disk geometry.", pathname);
		return(-1);
	}
	(void) close(fd);

	return(1);
} /* end get_disk_config() */



/*
 *	Name:		get_existing_part()
 *
 *	Description:	Get the existing partition table for a disk.  This
 *		routine also saves the size of the swap partition on the
 *		'hot' disk to keep a user from making it smaller than it
 *		originally was.
 */

int
get_existing_part(disk_p)
	disk_info *	disk_p;
{
	struct dk_allmap dk;			/* map of all the partitions */
	int		i;			/* index variable */
	int		fd;			/* descriptor for disk */
	char		pathname[MAXPATHLEN];	/* path to disk */
	int		ret_code;		/* return code */


	bzero((char *) disk_p->partitions, sizeof(disk_p->partitions));

	(void) sprintf(pathname, "/dev/r%sc", disk_p->disk_name);
	fd = open(pathname, 0);
	if (fd < 0) {
		menu_mesg("%s: cannot open device.", pathname);
		return(0);
	}

	if (ioctl(fd, DKIOCGAPART, &dk) == -1) {
		(void) close(fd);
		menu_mesg("%s: warning: no existing disk partitions.",
			  pathname);
		return(1);
	}
	(void) close(fd);

	/*
	 *	Initialize each partition's size.  If the size is non-zero,
	 *	then initialize the start cylinder number.
	 */
	for (i = 0; i < NDKMAP; i++) {
		map_blk(i + 'a') = dk.dka_map[i].dkl_nblk;
		if (map_blk(i + 'a'))
			map_cyl(i + 'a') = dk.dka_map[i].dkl_cylno;
#ifdef SunB1
		disk_p->partitions[i].fs_minlab = disk_p->disk_minlab;
		disk_p->partitions[i].fs_maxlab = disk_p->disk_maxlab;
#endif /* SunB1 */
	}

	/*
	 *	Save the size of the swap partition if the disk is hot.
	 */
	disk_p->swap_size = disk_p->is_hot ? map_blk('b') : 0;

	if (disk_p->label_source != DKL_EXISTING &&
	    (ret_code = check_partitions(disk_p,"cannot alter the existing disk partition.")) != 1) {
		return(ret_code);
	}

	return(1);
} /* end get_existing_part() */



/*
 *	Name:		check_partitions()
 *
 *	Description:	Check the partitions for invalid overlaps and
 *		partitions not in ascending order.  This alorithm allows
 *		gaps between partitions that are otherwise okay.
 */

int
check_partitions(disk_p, msg_p)
	disk_info *	disk_p;
	char *		msg_p;
{
	int		i, j;			/* index variables */
	char *		c;


	for (i = 0; i < NDKMAP - 1; i++) {
		if (map_blk(i + 'a') == 0)	/* no blocks so okay */
			continue;

		if (i + 'a' == 'c')		/* ignore partition 'c' */
			continue;

		for (j = i + 1; j < NDKMAP; j++) {
						/* no blocks so okay */
			if (map_blk(j + 'a') == 0)
				continue;

			if (j + 'a' == 'c')	/* ignore partition 'c' */
				continue;

			/*
			 *	At this point, both partitions are non-zero
			 *	and are not partition 'c', so this is an
			 *	invalid overlap or the partitions are not
			 *	in ascending order.
			 */
			if (map_cyl(i + 'a') +
			    blocks_to_cyls(map_blk(i + 'a'), disk_p->geom_buf)
			    > map_cyl(j + 'a')) {
				/*
				 * Lets make the best stab at it that we
				 * can.  If either partition a or b on the
				 * hot disk is in error then they need to
				 * use format under MUNIX
				 */
				if (disk_p->is_hot && i < 2)
					c = "Use the format command under MUNIX to alter partitions.";
				else
					c = "Use the format command to alter partitions.";
				menu_mesg("%s: %s\npartition %c starts before partition %c ends\n%s",disk_p->disk_name,msg_p,'a' + j,'a' + i,c);
				return(0);
			    }
		}
	}
	return(1);
} /* end check_partitions() */



