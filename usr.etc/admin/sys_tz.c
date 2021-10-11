/* 
 * sys_tz TIMEZONE=timezone
 *
 * Administrative method to set the system's timezone 
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static  char sccsid[] = "@(#)sys_tz.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "admin_amcl.h"
#include "sys_param_names.h"
#include "admin_messages.h"

#define ZONEINFO 	"/usr/lib/zoneinfo"
#define LOCALTIME 	"/usr/lib/zoneinfo/localtime"
#define OLDLOCALTIME 	"/usr/lib/zoneinfo/localtime-"
#define BACKUPLOCALTIME "/usr/lib/zoneinfo/localtime%"

extern void admin_write_err();
extern int admin_get_param();

static int set_timezone();

extern int errno;

int
main(argc, argv)
        int argc;
        char *argv[];
{
        char zonepath[MAXPATHLEN];	/* Timezone name to place us in */
        int status;

        if (admin_get_param(argc, argv, TIMEZONE_PARAM, zonepath, 
	    sizeof(zonepath)) == -1) {
		admin_write_err(SYS_ERR_NO_TIMEZONE, SYS_TIMEZONE_METHOD);
                exit(FAIL_CLEAN);
        }

        if ((status = set_timezone(zonepath)) != SUCCESS)
		admin_write_err(SYS_ERR_SET_TZ, SYS_TIMEZONE_METHOD);
        exit(status);
}

/*
 * set_timezone(zonefile) sets the system's idea of it's timezone location.
 * Logic is borrowed from config_host script of suninstall.
 */
static int 
set_timezone(zonefile)
	char *zonefile;
{
	char tzone[MAXPATHLEN];		/* Full pathname of timezone file */
	struct stat statbuf;		/* buffer for stat() */

	/*
	 * If the pathname passed in is relative, prepend the standard location
	 * to it.  Then check whether it exists.
	 */
	if (*zonefile == '/')
		strcpy(tzone, zonefile);
	else
		sprintf(tzone, "%s/%s", ZONEINFO, zonefile);

	if (stat(tzone, &statbuf) != 0) {
		perror("stat");
		return (FAIL_CLEAN);
	}

	/*
	 * Now rename the system's timezone file for recovery purposes and then
	 * try to link it to the requested file and if successful save the old 
	 * timezone for later reference.  The explicit unlink() is done because
	 * rename() doesn't do it.  Otherwise link() fails.
	 */
	if ((rename(LOCALTIME, BACKUPLOCALTIME) != 0) &&
		(errno != ENOENT)) { 	/* ENOENT implies there file doesn't exist */
		perror("backup rename");
		return (FAIL_CLEAN);
	}
	if (unlink(LOCALTIME) != 0) {
/*		perror("unlink %s",LOCALTIME);
		return (FAIL_CLEAN);
*/
	}

	if (link(tzone, LOCALTIME) != 0) {
		perror("link");
		if (rename(BACKUPLOCALTIME, LOCALTIME) != 0) {
			perror("backout rename");
			return (FAIL_DIRTY);
		} else
			return (FAIL_CLEAN);
	}

	(void) rename(BACKUPLOCALTIME, OLDLOCALTIME);
	return (SUCCESS);
}
