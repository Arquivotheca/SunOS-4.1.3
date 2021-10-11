#ifndef lint
#ifdef SunB1
#ident			"@(#)toc_xlat.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)toc_xlat.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		toc_xlat()
 *
 *	Description:	Read the TOC from the media into 'name' and call
 *		the toc_xlat program to convert it into a media file list
 *		in MEDIA_FILE.tmp.  Read the contents of MEDIA_FILE.tmp
 *		into 'tmp_p'.  'tmp_p' is expected to contain values
 *		specifying the media volume to get and the architecture
 *		to expect.
 *
 *		Returns 1 if the TOC was read successfully, 0 if the TOC
 *		could not be read and -1 if there was an error.
 */

#include <sys/file.h>
#include <sys/wait.h>
#include "install.h"
#include "menu.h"


/*
 *	External functions:
 */
extern	char *		sprintf();




int
toc_xlat(sys_p, name, tmp_p)
	sys_info *	sys_p;
	char *		name;
	soft_info *	tmp_p;
{
	char		pathname[MAXPATHLEN];	/* path to toc_xlat program */
	int		pid;			/* child's process id */
	int		ret_code;		/* wait() return code */
	union wait	status;			/* child's return status */


	(void) sprintf(pathname, "%s/toc_xlat", TOOL_DIR);

	pid = fork();
	if (pid == -1) {
		menu_log("%s: cannot start child process.", progname);
		menu_abort(1);
	}
	else if (pid == 0) {			/* this is the child */
		/*
		 *	Redirect the standard files to /dev/null
		 */
		(void) close(0);
		(void) close(1);
		(void) close(2);
		(void) open("/dev/null", O_RDWR);
		(void) dup(0);
		(void) dup(0);

		execl(pathname, "toc_xlat", name,
		      sys_p->arch_str, CP_NULL);
		exit(1);
	}

	while ((ret_code = wait(&status)) != pid)
		if (ret_code == -1)
			break;

	switch (status.w_retcode) {
	case 0:
		break;

	case 1:
		menu_log("%s : could not execute program %s.",
			 progname, pathname);
		return(-1);

	default:
		menu_log("%s: error in reading file %s.",
			 progname, name);
		return(-1);
	} /* end switch */

	(void) sprintf(pathname, "%s.tmp", MEDIA_FILE);

	ret_code = read_media_file(pathname, tmp_p);
	/*
	 *	Only a return of 1 is okay here.  This file is not
	 *	optional.  read_media_file() prints the error
	 *	message for ret_code == -1 so we have to print one
	 *	for ret_code == 0.
	 */
	if (ret_code != 1) {
		if (ret_code == 0)
			menu_log("%s: cannot read file :%s.",
				 progname, pathname);
		return(-1);
	}

	return(1);
} /* end toc_xlat() */
