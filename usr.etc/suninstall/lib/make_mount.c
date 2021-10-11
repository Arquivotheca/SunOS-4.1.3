#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)make_mount.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)make_mount.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		make_mount_list()
 *
 *	Description:	Make a mount_list from the data in the DISK_INFO
 *		files and update the root and user partitions in 'sys_p'.
 *		This routine is used by its caller() to make sure that
 *		a root and user mount point exists in the configuration.
 *
 *		Returns 1 if everything is okay, and -1 if there was an error.
 */

#include <stdio.h>
#include <string.h>
#include "install.h"
#include "menu.h"


/*
 *	External functions:
 */
extern	char *		sprintf();




int
make_mount_list(sys_p)
	sys_info *	sys_p;
{
	char		buf[BUFSIZ];		/* input buffer */
	disk_info	disk;			/* disk info buffer */
	FILE *		fp;			/* ptr to disk_list */
	mnt_ent		mount_list[NMOUNT];	/* mount list */
	mnt_ent *	mp;			/* ptr to mount list entry */
	char		pathname[MAXPATHLEN];	/* pathname buffer */


	(void) unlink(MOUNT_LIST);

	fp = fopen(DISK_LIST, "r");
	if (fp == NULL) {
		menu_log("%s: %s: cannot open file.", progname, DISK_LIST);
		return(-1);
	}

	while (fgets(buf, sizeof(buf), fp)) {
		delete_blanks(buf);

		(void) sprintf(pathname, "%s.%s", DISK_INFO, buf);

		switch (read_disk_info(pathname, &disk)) {
		case 1:
			break;

		case 0:
			continue;

		case -1:
			(void) fclose(fp);
			return(-1);
		}

		if (disk_to_mount_list(&disk, MOUNT_LIST) != 1) {
			(void) fclose(fp);
			return(-1);
		}
	}

	(void) fclose(fp);

	switch (read_mount_list(MOUNT_LIST, mount_list)) {
	case 1:
		break;

	/*
	 *	Create a zero-length mount_list if one does not exist.
	 */
	case 0:
		if (fp = fopen(MOUNT_LIST, "w"))
			(void) fclose(fp);
		break;

	case -1:
		return(-1);
	} /* end switch */

	bzero(sys_p->root, sizeof(sys_p->root));
	bzero(sys_p->user, sizeof(sys_p->user));

	for (mp = mount_list; mp->partition[0]; mp++) {
		if (strcmp(mp->mount_pt, "/") == 0) {
			(void) strcpy(sys_p->root, mp->partition);
		}
		else if (strcmp(mp->mount_pt, "/usr") == 0) {
			(void) strcpy(sys_p->user, mp->partition);
		}
	}

	if (save_sys_info(SYS_INFO, sys_p) != 1)
		return(-1);

	return(1);
} /* end make_mount_list() */
