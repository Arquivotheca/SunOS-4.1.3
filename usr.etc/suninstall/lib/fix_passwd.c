#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)fix_passwd.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)fix_passwd.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		fix_passwd()
 *
 *	Description:	Fix PASSWD and PWD_ADJUNCT.  Cannot use RMP_PASSWD
 *		and RMP_PWD_ADJUNCT here since we don't know if this is the
 *		miniroot.
 */

#include <sys/file.h>
#include <stdio.h>
#include "install.h"
#include "menu.h"


/*
 *	External functions:
 */
extern	char *		sprintf();


void
fix_passwd(prefix, sec_p)
	char *		prefix;
	sec_info *	sec_p;
{
	int		adj_exists;		/* does PWD_ADJUNCT exist? */
	char		cmd[MAXPATHLEN * 2];	/* command buffer */
	char		pathname[MAXPATHLEN];	/* scratch pathname */
	FILE *		pp;			/* ptr to ed(1) processes */


	(void) sprintf(pathname, "%s%s", prefix, PWD_ADJUNCT);
	mkdir_path(dirname(pathname));

	if (access(pathname, F_OK) == 0)
		adj_exists = 1;
	else {
		adj_exists = 0;
		/*
		 *	Copy the installed PASSWD file to the PWD_ADJUNCT file
		 *	so we can clean PWD_ADJUNCT up later.
		 */
		(void) sprintf(cmd, "cp %s%s %s%s", prefix, PASSWD, prefix,
			       PWD_ADJUNCT);
		x_system(cmd);
	}

	/*
	 *	Fix all the passwd field entries in PASSWD to be ##user.
	 */
	(void) sprintf(cmd, "ed %s%s > /dev/null 2>&1", prefix, PASSWD);
	pp = popen(cmd, "w");
	if (pp == NULL) {
		menu_log("%s: '%s' failed.", progname, cmd);
		menu_abort(1);
	}

	(void) fprintf(pp, "1,$ s@^\\([^:]*\\):\\([^:]*\\)@\\1:##\\1@g\n");
	(void) fprintf(pp, "/^+:##+/s/##+//\n");
	(void) fprintf(pp, "w\nq\n");

	(void) pclose(pp);


	/*
	 *	Fix PWD_ADJUNCT file entries
	 */
	(void) sprintf(cmd, "ed %s%s > /dev/null 2>&1", prefix, PWD_ADJUNCT);
	pp = popen(cmd, "w");
	if (pp == NULL) {
		menu_log("%s: '%s' failed.", progname, cmd);
		menu_abort(1);
	}

	/*
	 *	Only change the non-root and non-adjunct file entries if
	 *	there was no PWD_ADJUNCT file before.
	 */
	if (adj_exists == 0)
		(void) fprintf(pp,
			   "1,$ s@^\\([^:]*\\):\\([^:]*\\).*@\\1:\\2:::::@\n");
	(void) fprintf(pp, "/^root:/s@root:[^:]*@root:%s@\n",
		       sec_p->root_word);
	(void) fprintf(pp, "/^audit:/s@audit:[^:]*@audit:%s@\n",
		       sec_p->audit_word);
	(void) fprintf(pp, "w\nq\n");

	(void) pclose(pp);
} /* end fix_passwd() */
