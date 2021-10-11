#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)get_sec_info.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)get_sec_info.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		get_sec_info()
 *
 *	Description:	Use a menu based interface to get information from
 *		the user about security configuration for a system.  This
 *		program is typically called from suninstall, add_client,
 *		and add_services.  If invoked with a client name, then the
 *		get_sec_info deals with the security information for that
 *		client.  If get_sec_info is not invoked with a name, then
 *		it deals with the security information for the system.
 */

#include <stdio.h>
#include <string.h>
#include "sec_impl.h"




/*
 *	External functions:
 */
extern	char *		sprintf();




/*
 *	Global string constants:
 */
char		DEF_AUDIT_DIR[] =	"%s/etc/security/audit/%s";
char		DEF_AUDIT_FLAGS[] =	"ad,lo,p0,p1";
char		DEF_AUDIT_MIN[] =	"20";
char		DIR_FMT[] =		"%d_dir";
char		SALT[] =		"SI";




/*
 *	Local functions:
 */
static	void		init_sec_info();




/*
 *	Global variables:
 */
#ifdef SunB1
int		old_mac_state;			/* old mac-exempt state */
#endif /* SunB1 */
char *		progname;			/* name for error messages */




main(argc,argv)
	int argc;
        char **argv;
{
						/* ptrs for audit dir objs */
	pointer		dir_ptrs[MAX_AUDIT_DIRS][2];
	form *		form_p;			/* pointer to SECURITY form */
	char *		name_p;			/* pointer to name */
	char		pathname[MAXPATHLEN];	/* path to SEC_INFO */
	sec_info	sec;			/* security info buffer */
	sys_info	sys;			/* system info buffer */


	(void) umask(UMASK);			/* set file creation mask */

	progname = basename(argv[0]);		/* get name for error mesgs */

	if (argc > 2) {
		(void) fprintf(stderr, "Usage: %s [client_name]\n", progname);
		exit(1);
	}

	set_menu_log(LOGFILE);
						/* get system name */
	if (read_sys_info(SYS_INFO, &sys) != 1) {
		menu_log("%s: Error in %s.", progname, SYS_INFO);
		menu_abort(1);
	}

	/*
	 *	Grab the argument and assume it is a client's name.
	 */
	if (argc == 2)
		name_p = argv[1];
	/*
	 *	Use the system's name
	 */
	else
		name_p = sys.hostname0;

	get_terminal(sys.termtype);		/* get terminal type */

	(void) sprintf(pathname, "%s.%s", SEC_INFO, name_p);

	switch (read_sec_info(pathname, &sec)) {
	/*
	 *	If SEC_INFO.<name> is not there, then just set defaults.
	 */
	case 0:
		init_sec_info(name_p, &sys, &sec);
		break;

	case 1:
		break;

	case -1:
		menu_abort(1);
	}

	form_p = create_sec_form(dir_ptrs, &sec);

	/*
	 *	Ignore the 4.0 "repaint"
	 */
	(void) signal(SIGQUIT, SIG_IGN);

	if (use_form(form_p) != 1)		/* use SECURITY form */
		menu_abort(1);

	(void) signal(SIGQUIT, SIG_DFL);

						/* save the new sec_info */
	if (save_sec_info(pathname, &sec) != 1)
		menu_abort(1);

	end_menu();				/* done with menu system */

	exit(0);
	/*NOTREACHED*/
} /* end main() */




/*
 *	Name:		init_sec_info()
 *
 *	Description:	Use the host's name, information in system info
 *		structure and global constants to initialize the security
 *		info struct.
 */

static	void
init_sec_info(name_p, sys_p, sec_p)
	char *		name_p;
	sys_info *	sys_p;
	sec_info *	sec_p;
{
	char		prefix[MAXHOSTNAMELEN];	/* buffer for server prefix */


	(void) strncpy(sec_p->hostname, name_p, MAXHOSTNAMELEN);
	sec_p->hostname[MAXHOSTNAMELEN] = NULL;
	(void) strcpy(sec_p->audit_flags, DEF_AUDIT_FLAGS);
	(void) strcpy(sec_p->audit_min, DEF_AUDIT_MIN);

	/*
	 *	We are doing security for the system here so no prefix.
	 */
	if (strcmp(sys_p->hostname0, name_p) == 0)
		prefix[0] = NULL;
	else
		(void) sprintf(prefix, "%s:", sys_p->hostname0);

	(void) sprintf(sec_p->audit_dirs[0], DEF_AUDIT_DIR, prefix, name_p);
} /* end init_sec_info() */
