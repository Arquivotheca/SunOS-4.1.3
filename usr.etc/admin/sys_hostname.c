#ifndef lint
static	char sccsid[] = "@(#)sys_hostname.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 * sys_hostname HOSTNAME=hostname
 *
 * Administrative method to set the hostname for the local host.  Doesn't really
 * do the whole job, as there are a lot of places where the hostname shows up, 
 * e.g. /etc/hosts, /etc/ethers.  Have to figure that out for a more general 
 * solution.
 */

#include <sys/param.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <ctype.h>
#include <string.h>
#include <sys/ioctl.h>
#include "admin_amcl.h"
#include "sys_param_names.h"
#include "admin_messages.h"

#define HOSTNAME_FILE	"/etc/hostname"
#define	TMP_SUFFIX	"tmp"
#define HOME_ROOT	"/home"
#define HOME_MODE	0755

extern void admin_write_err();
extern int admin_get_param();
extern int get_ether0name();
extern int mkdir();

int
main(argc, argv)
        int argc;
        char *argv[];
{
        char name[MAXHOSTNAMELEN];    /* Server to be synced with */
        int status;

        if (admin_get_param(argc, argv, HOSTNAME_PARAM, name, sizeof(name)) == 
	    -1) {
		admin_write_err(SYS_ERR_NO_HOSTNAME, SYS_HOSTNAME_METHOD);
                exit(FAIL_CLEAN);
        }

        if ((status = set_hostname(name)) != SUCCESS)
		admin_write_err(SYS_ERR_SET_HOSTNAME, SYS_HOSTNAME_METHOD);
        exit(status);
}

/*
 * set_hostname(new_name) sets the local host's name to the new name.
 */
static int
set_hostname(new_name)
	char 	*new_name;
{
	char	ether_name[6];		/* Name of system ethernet interface */
	char	fname[MAXPATHLEN];	/* name of hostname file */
	char	tmp_fname[MAXPATHLEN];	/* temporary new hostname file */
	FILE 	*fp;			/* output file pointer */
	char	home_hostname[MAXPATHLEN]; /* pathname for /home/hostname */

	/*
	 * Get the name of the ethernet interface from the system, build the
	 * name of the hostname data file
	 */
	if (get_ether0name(ether_name, sizeof(ether_name))) {
		return (FAIL_CLEAN);
	}
	/*
	 * Generate the filenames to deal with.
	 */
	sprintf(fname, "%s.%s0", HOSTNAME_FILE, ether_name);
	sprintf(tmp_fname, "%s.%s", HOSTNAME_FILE, TMP_SUFFIX);

	if ((fp = fopen(tmp_fname, "w")) == NULL) {
		perror("fopen");
		return (FAIL_CLEAN);
	}
	if (fprintf(fp, "%s\n", new_name) == EOF) {
		perror("fprintf");
		fclose(fp);
		unlink(tmp_fname);
		return (FAIL_CLEAN);
	}
	fclose(fp);

	/*
	 * Move temporary file to permanent
	 */
	if (rename(tmp_fname, fname) != 0) {
		perror("rename");
		unlink(tmp_fname);
		return (FAIL_CLEAN);
	}

	/* 
	 * Make /home/hostname for consistency with suninstall behavior.
	 */
	sprintf(home_hostname, "%s/%s", HOME_ROOT, new_name);
	(void) mkdir(home_hostname, HOME_MODE);

	return (SUCCESS);
}

