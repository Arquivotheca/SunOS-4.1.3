#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)calc_client.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)calc_client.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		calc_client()
 *
 *	Description:	Calculate the amount of disk space used by clients
 *		for the architecture 'arch'.  Gets the names of the clients
 *		from CLIENT_LIST.'arch' and calls update_bytes() to do
 *		the disk space management.
 *
 *		Returns 1 if everything is okay, 0 if there is insufficent
 *		disk space and -1 if there was an error.
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"




/*
 *	External functions:
 */
extern	char *		sprintf();


int
calc_client(arch_str)
	char *		arch_str;
{
	char		buf[BUFSIZ];		/* I/O buffer */
	clnt_info	clnt;			/* client information */
	FILE *		fp;			/* ptr to client list */
	char *		part;			/* ptr to partition name */
	char		pathname[MAXPATHLEN];	/* pathname buffer */
	int		ret_code;		/* return code */


	(void) sprintf(pathname, "%s.%s", CLIENT_LIST, arch_str);
	if (fp = fopen(pathname, "r")) {
		while (fgets(buf, sizeof(buf), fp)) {
			delete_blanks(buf);

			(void) sprintf(pathname, "%s.%s", CLIENT_STR, buf);

			ret_code = read_clnt_info(pathname, &clnt);
			/*
			 *	Only a return code of 1 is okay here.  Must
			 *	add an error message for ret_code == 0.
			 *	Non-fatal errors are not okay here.
			 */
			if (ret_code != 1) {
				(void) fclose(fp);
				if (ret_code == 0)
					menu_log("%s: %s: cannot read file.",
						 progname, pathname);
				return(-1);
			}

			part = find_part(clnt.root_path);
			/*
			 *	Can't find the partition name.  Error
			 *	message provided by find_part().
			 */
			if (part == NULL) {
				(void) fclose(fp);
				return(-1);
			}

			ret_code = update_bytes(part, MIN_XROOT);
			/*
			 *	Only a return code of 1 is okay here.
			 *	Non-fatal errors are okay here so return
			 *	the actual code.
			 */
			if (ret_code != 1) {
				(void) fclose(fp);
				return(ret_code);
			}

			part = find_part(clnt.swap_path);
			if (part == NULL) {
				(void) fclose(fp);
				return(-1);
			}

			ret_code = update_bytes(part,
					      SWAP_FUDGE_SIZE(cv_swap_to_long(clnt.swap_size)));
			/*
			 *	Only a return code of 1 is okay here.
			 *	Non-fatal errors are okay here so return
			 *	the actual code.
			 */
			if (ret_code != 1) {
				(void) fclose(fp);
				return(ret_code);
			}
		}

		(void) fclose(fp);
	}

	return(1);
} /* end calc_client() */
