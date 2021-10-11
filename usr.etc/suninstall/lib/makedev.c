#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)makedev.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)makedev.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		MAKEDEV()
 *
 *	Description:	Call the MAKEDEV program to make the named devices
 *		in the dev directory of 'path'.  This routine handles the
 *		name space overlap between 'mt' and 'xt'.
 *              This routine also handles the fact that the sunfed box needs
 *              the device st8 to read QIC-24 but this device is made when
 *              a MAKEDEV is done for st0
 *		We also have support for remaking the mt or xt device if
 *		a scsi tape device has been made since the mt entry points
 *		to the SCSI tape.
 */

#include <stdio.h>
#include <string.h>
#include "install.h"
#include "menu.h"
#include "media.h"

extern	char *		getwd();
extern	char *		sprintf();

struct {char *path,*dev} hold_dev;

void
MAKEDEV(path, dev)
	char *		path;
	char *		dev;
{
	char		cmd[BUFSIZ];		/* command buffer */
	char		cwd[MAXPATHLEN];	/* current working directory */
	char		dir[MAXPATHLEN];	/* path to dev directory */

	/*
	 * For the CD_ROM Device, we don't need to make devices yet.
	 */
	if (is_miniroot()) {
		if (strcmp(dev, "sr0") == 0)
			return;
	}
	/* end CD_ROM */

	menu_flash_on("Making device nodes");

	(void) getwd(cwd);			/* save where we are */

	if (path && path[0]) {			/* different root file system */
		(void) sprintf(dir, "%s/dev", path);
	}
	else {
		(void) strcpy(dir, "/dev");
	}

	x_chdir(dir);				/* goto to the right dir */

	/*
	 *	'mt' and 'xt' use the same special device names, and
	 *	we don't care about the return status from system()
	 */
	if (strncmp(dev, "mt", 2) == 0 || strncmp(dev, "xt", 2) == 0) {
		(void) system("rm -f *mt*");
		if (path == NULL)
			hold_dev.path = path;
		else
			hold_dev.path = strdup(path);
		hold_dev.dev  = strdup(dev);
	}

	/*
	 *      If the device is st8 we need to do a MAKEDEV on st0
	 *      to create st8
	 */
	if (strncmp(dev,"st8",3) == 0)
		dev = "st0";


	/*
	 *	Make the devices
	 */
#ifdef SunB1
	if (path && path[0])
		(void) sprintf(cmd, "./MAKEDEV -r %s %s >> %s 2>&1", path, dev,
			       LOGFILE);
	else
		(void) sprintf(cmd, "./MAKEDEV %s >> %s 2>&1", dev, LOGFILE);
#else
	(void) sprintf(cmd, "./MAKEDEV %s >> %s 2>&1", dev, LOGFILE);
#endif /* SunB1 */
	x_system(cmd);

	x_chdir(cwd);

	menu_flash_off(REDISPLAY);
	/*
	 * if this devices was st0 and if there was a mt or xt device
	 * previously made we have to do it again
	 */
	if (strncmp(dev,"st0",3) == 0 && hold_dev.dev != NULL)
		MAKEDEV(hold_dev.path,hold_dev.dev);

} /* end MAKEDEV() */
