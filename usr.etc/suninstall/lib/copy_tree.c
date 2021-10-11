#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)copy_tree.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)copy_tree.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		copy_tree()
 *
 *	Description:	Copy files from source 'src' to destination 'dest'.
 *		This implementation uses 'tar' to make the copy.
 */

#include "install.h"
#include "menu.h"


/*
 *	External functions:
 */
extern	char *		sprintf();




void
copy_tree(src, dest)
	char *		src;
	char *		dest;
{
	char		cmd[MAXPATHLEN * 2];	/* command buffer */


	menu_flash_on("Copying %s to %s\n", src, dest);

	(void) sprintf(cmd,
#ifdef SunB1
	      "(cd %s; tar cfHZ - .) 2>> %s | (cd %s; tar xfpHZ -) >> %s 2>&1",
#else
		  "(cd %s; tar cf - .) 2>> %s | (cd %s; tar xfp -) >> %s 2>&1",
#endif /* SunB1 */
		       src, LOGFILE, dest, LOGFILE);

#ifdef TEST_JIG
	menu_log("%s", cmd);
#else
	macex_on();
	x_system(cmd);
	macex_off();
#endif

} /* end copy_tree() */
