#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)find_part.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)find_part.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		find_part.c
 *
 *	Description:	This file contains the routines needed to find
 *		partition associated with a given pathname.
 */

#include <stdio.h>
#include <string.h>
#include "install.h"
#include "menu.h"


static	int		df_to_mount_list();


/*
 *	Name:		find_part()
 *
 *	Description:	Find the partition associated with 'path'.  If we
 *		are on the miniroot(), then we can use the mount_list.
 *		Otherwise we have to use df_to_mount_list() to make a
 *		mount_list.  Returns NULL if there was an error.
 */

char *
find_part(path)
	char *		path;
{
	int		mount_len;		/* length of mount point */
	mnt_ent		mount_list[NMOUNT];	/* mount list buffer */
	mnt_ent *	mp;			/* ptr to mount list entry */
	int		path_len;		/* length of path */
	int		ret_code;		/* return code */

	static	char	part_name[TINY_STR];	/* name of the partition */


	bzero((char *) mount_list, sizeof(mount_list));

	if (is_miniroot()) {
		ret_code = read_mount_list(MOUNT_LIST, mount_list);
		if (ret_code != 1) {
			/*
			 *	Only return code of 1 is okay here.  An error
			 *	message is not provided for ret_code == 0 so
			 *	we have to print it.
			 */
			if (ret_code == 0)
				menu_log("%s: %s: cannot read file.", progname,
					 MOUNT_LIST);

			return(NULL);
		}
	}
	else if ((ret_code = df_to_mount_list(mount_list)) != 1) {
		/*
		 *	Only return code of 1 is okay here.  An error
		 *	message is not provided for ret_code == 0 so
		 *	we have to print it.
		 */
		if (ret_code == 0)
			menu_log("%s: %s: cannot read file.", progname,
				 MOUNT_LIST);
		return(NULL);
	}

	/*
	 *	Find the last mount entry
	 */
	for (mp = mount_list; mp->partition[0]; mp++)
		/* NULL statement */ ;
	mp--;

	/*
	 *	Search backwards though the mount list to find the longest
	 *	matching mount point for 'path'.
	 */
	path_len = strlen(path);

	for (; mp >= mount_list; mp--) {
		mount_len = strlen(mp->mount_pt);

		/*
		 *	The mount point name is longer than the target path
		 *	so ignore this mount point.
		 */
		if (path_len < mount_len)
			continue;

		/*
		 *	If we have reached the catch-all, then catch all.
		 *	If the target path is the same length as the mount
		 *	point or if the target path has a path separator at
		 *	the same place as the mount point, then see if the
		 *	mount point is a prefix of the target path.
		 */
		if (mount_len == 1 ||
		    (path[mount_len] == NULL || path[mount_len] == '/') &&
		    strncmp(path, mp->mount_pt, mount_len) == 0) {
			(void) strcpy(part_name, mp->partition);
			return(part_name);
		}
	}

	menu_log("%s: %s: cannot find a mount point for this path.", progname,
		 MOUNT_LIST);
	return(NULL);
} /* end find_part() */




/*
 *	Name:		df_to_mount_list()
 *
 *	Description:	Convert output from df(1) into a mount_list.
 *		Returns 1 if everything is okay, and -1 if there was an error.
 */

static	int
df_to_mount_list(list)
	mnt_ent		list[];
{
	char		buf[BUFSIZ];		/* I/O buffer */
	int		count = 0;		/* entry counter */
	mnt_ent		ent;			/* scratch mount entry */
	FILE *		fp;			/* ptr to process */


#ifdef SunB1
	/*
	 *	df(1) on SunOS MLS is detained and is very slow so we
	 *	should advise the installer what we are doing.
	 */
	menu_flash_on("Using df to make mount list");
#endif /* SunB1 */

	macex_on();
	fp = popen("df 2> /dev/null | grep '^/dev'", "r");
	macex_off();
	if (fp == NULL)
		return(-1);

	while (fgets(buf, sizeof(buf), fp)) {
		bzero((char *) &ent, sizeof(ent));

		if (sscanf(buf, "/dev/%s %*d %*d %*d %*s %s", ent.partition,
			   ent.mount_pt) != 2) {
			(void) pclose(fp);
			return(0);
		}

		ent.count = elem_count(ent.mount_pt);

		list[count++] = ent;
	}

	(void) pclose(fp);

#ifdef SunB1
	menu_flash_off(REDISPLAY);
#endif /* SunB1 */

	sort_mount_list(list);

	return(1);
} /* end df_to_mount_list() */
