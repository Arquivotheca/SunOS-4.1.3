/* 
 * sys_domain DOMAIN=domainname
 *
 * Administrative method to configure the system's domain name.  The underlying 
 * code is also capable of deconfiguring NIS, but not enabled for now.  Needs
 * to understand additional steps for master & slave roles to be fully
 * functional.
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint 
static  char sccsid[] = "@(#)sys_domain.c 1.1 92/07/30 SMI"; 
#endif

#include <sys/errno.h>
#include <stdio.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include "admin_amcl.h"
#include "sys_param_names.h"
#include "admin_messages.h"

#define NIS_DIR "/var/yp"
#define NIS_BACKUP_DIR "/var/yp-"
#define NIS_BINDING_DIR "/var/yp/binding"
#define NIS_DIR_OWNER "bin"
#define NIS_DIR_GROUP "staff"
#define NIS_DIR_MODE 02755

#define DOMAINNAME_FILE "/etc/defaultdomain"
#define DOMAINNAME_TEMP_FILE "/etc/defaultdomain.tmp"

extern void admin_write_err();
extern int admin_get_param();

extern int errno;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	static char type[]="client";	/* Role for machine */
	char domain[MAXDOMAINLEN];	/* domain to join */
	int status;			/* Result of setup attempt */

	/*
	 * Use lib routine to retrieve parameters from command line.  All
	 * parameters are passed name=value style.
	 */
	if (admin_get_param(argc, argv, DOMAIN_PARAM, domain, sizeof(domain))
	    == -1) {
		admin_write_err(SYS_ERR_NO_DOMAIN, SYS_DOMAIN_METHOD);
		exit(FAIL_CLEAN);
	}

	if ((status = set_nis(type, domain)) != SUCCESS)
		admin_write_err(SYS_ERR_SET_DOMAIN, SYS_DOMAIN_METHOD);

	exit(status);
}

/*
 * set_nis(type, domain) - Set up NIS on local machine for specific role (none,
 * 	client, slave, master) in the specified domain.
 */	
		
int
set_nis(type, domain)
	char *type, *domain;
{
	struct group *gp;	/* Group info for chown */
	struct passwd *pp;	/* uid info for chown */
	FILE *fp;
	int status;

	/*
	 * Put the new domain name in a temp file so we can set it at the
	 * end of everything with minimal hassle.
	 */
	if ((fp = fopen(DOMAINNAME_TEMP_FILE, "w")) == NULL) {
		perror("fopen");
		return (FAIL_CLEAN);
	} 
	if (fprintf(fp, "%s\n", domain) == EOF) {
		perror("fprintf");
		(void) fclose(fp);
		return (FAIL_CLEAN);
	}

	(void) fclose(fp);

	/*
	 * If no NIS (not currently allowed, but for future) rename
	 * directory and set domain to whatever was requested (should be noname)
	 */
	if (strcmp(type, "none") == 0) {

		/* Remove the backup NIS directory because we're going
		 * to re-use it.
		 */
		if ((unlink(NIS_BACKUP_DIR) != 0) && 
		    (errno != ENOENT)) { /* ENOENT implies doesn't exist */
			perror("deconfig unlink");
			return (FAIL_CLEAN);
		}

		/* Move the NIS directory to the backup directory
		 */
		if ((rename(NIS_DIR, NIS_BACKUP_DIR) != 0) && 
		    (errno != ENOENT)) { /* ENOENT implies already disabled */
			perror("deconfig rename");
			return (FAIL_CLEAN);

		/* Now setup the file used on reboots to set the domainname
		 */
		} else if (rename(DOMAINNAME_TEMP_FILE, DOMAINNAME_FILE) != 0) {
			perror("defaultdomain rename");
			if (rename(NIS_BACKUP_DIR, NIS_DIR) != 0) {
				perror("yp rename");
				return (FAIL_DIRTY);
			} else
				return (FAIL_CLEAN);
		} else
			return (SUCCESS);
	} else {

		/* 
		 * Set up NIS to be operational at next reboot.  Start by 
		 * getting info needed for chown, fail if unavailable 
		 */
		if (((gp = getgrnam(NIS_DIR_GROUP)) == NULL) || 
        	    ((pp = getpwnam(NIS_DIR_OWNER)) == NULL)) {
			if (gp == NULL)
				perror("getgrnam");
			else
				perror("getpwnam");
			return (FAIL_CLEAN);
		}

		/* 
		 * Now try renaming the saved directory to operational name.
		 */
		/* Should this be here for master/slave support?
		 * If so, we have to unlink NIS_DIR first
		 */
		/*if ((rename(NIS_BACKUP_DIR, NIS_DIR) != 0) && 
		 *   (errno != ENOENT)) {
		 *	perror("yp rename");
		 *	return (FAIL_CLEAN);
		 *}
		 */

		/* 
		 * Try creating the directory, EEXIST will show up if the 
		 * rename worked or if the request is actually just changing 
		 * domain name.  chmod and chown are done just to make sure 
		 * everything is alright.  chmod is necessary if the mkdir 
		 * creates the directory as it ignores the sgid bit in 
		 * NIS_DIR_MODE 
		 */
		if ((mkdir(NIS_DIR, NIS_DIR_MODE) != 0) && (errno != EEXIST)) {
			perror("yp mkdir");
			return (FAIL_CLEAN);
		}
		if (((status = chown(NIS_DIR, pp->pw_uid, gp->gr_gid)) != 0) ||
		    (chmod(NIS_DIR, (mode_t)NIS_DIR_MODE) != 0)) {
			if (status != 0)
				perror("yp chown");
			else
				perror("yp chmod");
			if (rename(NIS_DIR, NIS_BACKUP_DIR) != 0) {
				perror("backout rename");
				return (FAIL_DIRTY);
			} else
				return (FAIL_CLEAN);
		}

		/* 
		 * Now do the same thing for the binding directory
		 */
		if ((mkdir(NIS_BINDING_DIR, NIS_DIR_MODE) != 0) && 
		    (errno != EEXIST)) {
			perror("binding mkdir");
			if (rename(NIS_DIR, NIS_BACKUP_DIR) != 0) {
				perror("backout rename");
				return (FAIL_DIRTY);
			} else
				return (FAIL_CLEAN);
		}
		if (((status = 
		    chown(NIS_BINDING_DIR, pp->pw_uid, gp->gr_gid)) != 0) || 
		    (chmod(NIS_BINDING_DIR, (mode_t)NIS_DIR_MODE) != 0)) {
			if (status != 0)
				perror("binding chown");
			else
				perror("binding chmod");
			if (rename(NIS_DIR, NIS_BACKUP_DIR) != 0) {
				perror("backout rename");
				return (FAIL_DIRTY);
			} else
				return (FAIL_CLEAN);
		}

		/* 
		 * Record the domain name to use from now on
		 */
		if (rename(DOMAINNAME_TEMP_FILE, DOMAINNAME_FILE) != 0) {
			perror("defaultdomain rename");
			if (rename(NIS_DIR, NIS_BACKUP_DIR) != 0) {
				perror("backout rename");
				return (FAIL_DIRTY);
			} else
				return (FAIL_CLEAN);
		} else
			return (SUCCESS);
	}
}
