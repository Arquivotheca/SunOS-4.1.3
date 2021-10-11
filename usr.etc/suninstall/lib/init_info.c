#ifndef lint
static  char    sccsid[] = 	"@(#)init_info.c 1.1 92/07/30 SMI";
#endif lint

/*
 *	Copyright (c) 1991 Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../install.h"

/*
 *	Name:		init_disk_info()
 *
 *	Description:	Create the DISK_LIST file and DISK_INFO files
 *		for 4.2 filesystems (discovered using df).
 *		Only the strings in the disk_info file that are required
 *		to size a partition are updated in the disk info file.
 *		Since we get the disk names dynamically this also allows
 *		third party devices to be used.
 *
 *		To minimize code changes, the disk list can have multiple
 *		entries for the same disk.  This is ok, it just wastes cpu
 *		cycles.
 *
 *		This is really a workaround to provide for dynamic
 *		disk layout.  The previous code allowed dynamic disk
 *		sizing only for partitions that existed when the
 *		machine was installed.  This code merely creates
 *		disk_info files with enough data in them to be used by
 *		the rest of the code.
 *
 *		To disable this feature set static_sizing to yes in
 *		/etc/install/sys_info.  You might need to to add the line
 *		static_sizing=yes
 */

void
init_disk_info(sys_p)
	sys_info *	sys_p;
{
	FILE *		fp;			/* ptr to disk_list */
	FILE *		pp;			/* ptr to mount pts */
	char		pathname[MAXPATHLEN];	/* pathname to disk_list */
	char            device[SMALL_STR];      /* device name */
	int		unit;			/* unit counter */
	long            avail;                  /* available K bytes */
	char            buf[BUFSIZ];            /* scratch buffer */
	disk_info	disk;
	char            cmd[MAXPATHLEN * 2];    /* command buffer */


	/*
	 * Don't run in the miniroot
	 */
	if (is_miniroot())
		return;

	if (sys_p->static_sizing)
		return;

	fp = fopen(DISK_LIST, "w");

	if (fp == NULL) {
		menu_log("%s: unable to open file for writing.", DISK_LIST);
		menu_abort(1);
	}
	(void) sprintf(buf, "df -t %s", DEFAULT_FS);
	macex_on();
	pp = popen(buf, "r");
	macex_off();

	if (pp == NULL) {
		menu_log("%s: '%s': command failed.", progname, buf);
		return;
	}

	while (fgets(buf, sizeof(buf), pp)) {
		if (sscanf(buf, "/dev/%s %*s %*s %ld", device, &avail) == 2) {
			device[strlen(device) - 1] = 0;
			if (strncmp(device, "md", 2) == 0) {
				fprintf(fp, "%s\n", device);
				continue;
			}
			strcpy(buf, "/dev/");
			strcat(buf, device);
			strcat(buf, "c");
			if (access(buf, F_OK))
				continue;
			fprintf(fp, "%s\n", device);
			bzero(disk, sizeof(disk_info));
			strcpy(disk.disk_name, device);
			get_disk_config(&disk);
			get_existing_part(&disk);
			update_parts(&disk);
			if (is_small_size(&disk))
				disk.free_hog = 'g';
			else
				disk.free_hog = 'h';
			disk.label_source = DKL_EXISTING;
			disk.display_unit = DU_MBYTES;
			sprintf(pathname, "%s.%s", DISK_INFO, disk.disk_name);
			(void) save_disk_info(pathname, &disk);
			sprintf(cmd, "cp %s %s.original 2>> %s", pathname, 
				pathname, LOGFILE);
			x_system(cmd);
		}
	}


	(void) fclose(fp);
	(void) pclose(pp);
} /* end init_info() */
