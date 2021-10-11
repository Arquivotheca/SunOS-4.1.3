#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)tune_audit.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)tune_audit.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		tune_audit()
 *
 *	Description:	Tune auditing.  Create the AUDIT_CONTROL file
 *		from the sec_info structure pointed to by sec_p.  Cannot
 *		use RMP_AUDIT_CONTROL since we don't know if this is the
 *		miniroot.  Makes the audit directories for the system
 *		and for any clients.  Updates the FSTAB file for remote
 *		mounted audit directories.  Updates the EXPORTS file on
 *		the server for any clients that have remote mounted audit
 *		directories.  'prefix' specifies the path prefix to apply
 *		to get to the right place.
 *
 *		This implementation assumes that the 'audit_dirs' have
 *		AUDIT_DIR as a prefix and do not include the 'files'
 *		component of the pathname, e.g. 'AUDIT_DIR'/fred
 */

#ifdef SunB1
#include <sys/types.h>
#include <sys/label.h>
#include <sys/wait.h>
#endif /* SunB1 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "install.h"
#include "menu.h"


/*
 *	External functions:
 */
extern	char *		sprintf();


#ifdef SunB1
/*
 *	External variables:
 */
extern	int		errno;
#endif /* SunB1 */


void
tune_audit(client_p, prefix, sec_p)
	clnt_info *	client_p;
	char *		prefix;
	sec_info *	sec_p;
{
	char		buf[MAXPATHLEN];	/* scratch cmd & pathname */
	char *		cp;			/* scratch char pointer */
	FILE *		fp;			/* ptr to AUDIT_CONTROL */
	int		i;			/* index variable */
#ifdef SunB1
	int		pid;			/* process id from fork() */
	int		ret_code;		/* return code */
	union wait	status;			/* child's return status */
	blabel_t	system_high;		/* system_high label */
#endif /* SunB1 */
	char		value[MAXPATHLEN + SMALL_STR];	/* value to add */


#ifdef SunB1
	/*
	 *	Spin off a child process so we can change our process label
	 *	to system_high.  This is expensive, but necessary to avoid
	 *	breaking security rules.  We also have to make sure that
	 *	anything we create outside of the audit area has the right
	 *	label (e.g. FSTAB at system_low).
	 */
	pid = fork();
	if (pid == -1) {
		menu_log("%s: cannot create child process.", progname);
		menu_abort(1);
	}

	/*
	 *	This is the child.
	 */
	if (pid == 0) {

		/*
		 *	Auditing on SunsOS MLS is done at system_high,
		 *	so we must tune the auditing stuff at system_high.
		 *	To avoid the pitfalls of being at system_high we
		 *	must also be mac-exempt.
		 */
		(void) blhigh(BL_ALLPARTS, &system_high);
		(void) setplabel(&system_high);
		macex_on();

		/*
		 *	Since this is a child process there will be no
		 *	setpabel() to system_low or macex_off() call.
		 */
#endif /* SunB1 */

		menu_flash_on("Tuning auditing");

		(void) sprintf(buf, "%s%s", prefix, AUDIT_DIR);

#ifdef SunB1
		mkldir_path(buf, &system_high);	/* make the audit directory */
#else
		mkdir_path(buf);		/* make the audit directory */
#endif /* SunB1 */

		(void) sprintf(buf, "%s%s", prefix, AUDIT_CONTROL);

		fp = fopen(buf, "w");

		if (fp == NULL) {
			menu_log("%s: %s: %s", progname, buf, err_mesg(errno));
			menu_log("\tCannot open file for writing.");
			menu_abort(1);
		}

		(void) fprintf(fp, "flags:%s\n", sec_p->audit_flags);
		(void) fprintf(fp, "minfree:%s\n", sec_p->audit_min);
		for (i = 0; sec_p->audit_dirs[i][0]; i++) {
			/*
			 *	If this is a network filesystem, then only add
			 *	the pathname to the audit control file.
			 */
			cp = strchr(sec_p->audit_dirs[i], ':');
			if (cp == NULL)
				cp = sec_p->audit_dirs[i];
			else
				cp++;

			(void) fprintf(fp, "dir:%s/files\n", cp);

			/*
			 *	If this is a local path, then just make
			 *	the 'files' directory for auditing into.
			 */
			if (cp == sec_p->audit_dirs[i]) {
				(void) sprintf(buf, "%s%s/files", prefix, cp);

#ifdef SunB1
				mkldir_path(buf, &system_high);
#else
				mkdir_path(buf);
#endif /* SunB1 */
			}
			/*
			 *	This is a network path.
			 */
			else {
				/*
				 *	Make the mount point.
				 */
				(void) sprintf(buf, "%s%s", prefix, cp);

#ifdef SunB1
				mkldir_path(buf, &system_high);
#else
				mkdir_path(buf);
#endif /* SunB1 */

				/*
				 *	If this is a client, then make the
				 *	directory to mount on the server,
				 *	and export the directory.
				 */
				if (client_p) {
					(void) sprintf(buf, "%s%s/files",
						       dir_prefix(), cp);

#ifdef SunB1
					mkldir_path(buf, &system_high);
#else
					mkdir_path(buf);
#endif /* SunB1 */

					(void) sprintf(buf, "%s%s",
						       dir_prefix(), EXPORTS);
					(void) sprintf(value,
						       "-access=%s,root=%s",
						       client_p->hostname,
						       client_p->hostname);
					add_key_entry(cp, value, buf, KEY_ONLY);

#ifdef SunB1
					/*
					 *	Force the file's label to
					 *	system_low so we can get to it.
					 */
					(void) sprintf(buf,
						    "setlabel system_low %s%s",
						       dir_prefix(), EXPORTS);
					x_system(buf);
#endif /* SunB1 */
				}

				(void) sprintf(buf, "%s%s", prefix, FSTAB);
				(void) sprintf(value, "%s %s rw 0 0", cp,
					       DEFAULT_NET_FS);
				add_key_entry(sec_p->audit_dirs[i], value,
					      buf, KEY_ONLY);

#ifdef SunB1
				/*
				 *	Force the file's label to system_low
				 *	so we can get to it.
				 */
				(void) sprintf(buf, "setlabel system_low %s%s",
					       prefix, FSTAB);
				x_system(buf);
#endif /* SunB1 */
			}
		}

		(void) fclose(fp);

		(void) sprintf(buf, "chmod -R go-rwx %s%s", prefix, AUDIT_DIR);
		x_system(buf);

		(void) sprintf(buf, "chown -R audit.audit %s%s", prefix,
			       AUDIT_DIR);
		x_system(buf);

		/*
		 *	Do the AUDIT_DIR on the server too since we just tuned
		 *	a client and may have made new directories.
		 */
		if (client_p) {
			(void) sprintf(buf, "chmod -R go-rwx %s%s",
				       dir_prefix(), AUDIT_DIR);
			x_system(buf);

			(void) sprintf(buf, "chown -R audit.audit %s%s",
				       dir_prefix(), AUDIT_DIR);
			x_system(buf);
		}

		menu_flash_off(REDISPLAY);

#ifdef SunB1
		/*
		 *	Force the AUDIT_CONTROL file's label to system_low
		 *	so we can get to it.
		 */
		(void) sprintf(buf, "setlabel system_low %s%s", prefix,
			       AUDIT_CONTROL);
		x_system(buf);

		exit(0);
	}

	while ((ret_code = wait(&status)) != pid)
		if (ret_code == -1)
			break;

	if (ret_code == -1) {
		menu_log("%s: %s", progname, err_mesg(errno));
		menu_abort(1);
	}

	if (status.w_status != 0) {
		menu_log(
		      "%s: tune_audit(): child process terminated abnormally.",
			 progname);
		menu_abort(1);
	}
#endif /* SunB1 */
} /* end tune_audit() */
