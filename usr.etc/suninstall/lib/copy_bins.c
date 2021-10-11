#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)copy_bins.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)copy_bins.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		copy_install_bin()
 *
 *	Description:	Copy list of binaries from the source to the
 *		destination.  The list of binaries to copy is in
 *		INSTALL_BIN on the root mount point.
 */

#include <errno.h>
#include <stdio.h>
#include "install.h"
#include "menu.h"




/*
 *	External functions:
 */
extern	char *		sprintf();




void
copy_install_bin(exec_dir, kvm_dir, dest_dir)
	char *		exec_dir;
	char *		kvm_dir;
	char *		dest_dir;
{
	char		buf[MAXPATHLEN * 2];	/* command buffer */
	char		dest_path[MAXPATHLEN];	/* destination path */
	FILE *		fp;			/* ptr to INSTALL_BIN */
	char		pathname[MAXPATHLEN];	/* pathname to INSTALL_BIN */
	char		src_path[MAXPATHLEN];	/* source path */
	char		type[SMALL_STR];


	menu_mesg("Copying binaries\n");

	(void) sprintf(pathname, "%s%s%s", dir_prefix(), exec_dir,
		       INSTALL_BIN);

	fp = fopen(pathname, "r");
	if (fp == NULL) {
#ifdef TEST_JIG
		menu_log("Copying files from %s.", pathname);
		return;
#else
		menu_log("%s: %s: %s.", progname, pathname, err_mesg(errno));
		menu_abort(1);
#endif TEST_JIG
	}

	while (fgets(buf, sizeof(buf), fp)) {
		if (*buf == '#')
			continue;

		if (sscanf(buf, "%s %s %s", type, src_path, dest_path) != 3){
			menu_log("%s: %s: line format error.", progname,
				 pathname);
			menu_abort(1);
		}

		if (!strcmp(type,"appl")) {
			(void) sprintf(buf, "cp %s%s/%s %s%s/%s 2>> %s",
				dir_prefix(), exec_dir, src_path,
				dir_prefix(), dest_dir, dest_path, LOGFILE);
		} else {
			(void) sprintf(buf, "cp %s%s/%s %s%s/%s 2>> %s",
				dir_prefix(), kvm_dir, src_path,
				dir_prefix(), dest_dir, dest_path, LOGFILE);
		}

		x_system(buf);
	}

	(void) fclose(fp);

} /* end copy_install_bin() */
