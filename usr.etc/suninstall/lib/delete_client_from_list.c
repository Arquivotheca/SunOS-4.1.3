#ifndef lint
#ifdef SunB1
#ident			"@(#)delete_client_from_list.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)delete_client_from_list.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint


/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"


/*
 *	Name:		delete_client_from_list()
 *
 *	Description:	Delete 'client_p' from the appropriate CLIENT_LIST.
 */

int
delete_client_from_list(client_p)
	clnt_info *	client_p;
{
	char		buf[BUFSIZ];		/* input buffer */
	char		cmd[MAXPATHLEN * 2];	/* command buffer */
	int		found_it = 0;		/* was the name found? */
	FILE *		fp;			/* ptr to client_list.<arch> */
	char		pathname[MAXPATHLEN];	/* path to client_list.<arch>*/
	FILE *		tmp_fp;			/* ptr to client_list.tmp */
	static	int	delete_from_all_list(); /* del. client from
						** CLIENT_LIST_ALL list
						*/

	(void) sprintf(pathname, "%s.%s", CLIENT_LIST,
		       client_p->arch_str);
	fp = fopen(pathname, "r");
	/*
	 *	If there is no client list, then the client is already deleted.
	 */
	if (fp == NULL) {
		menu_log("%s: %s is not a %s client.", progname,
			 client_p->hostname, client_p->arch_str);
		return(0);
	}

	(void) sprintf(pathname, "%s.tmp", CLIENT_LIST);
	tmp_fp = fopen(pathname, "w");
	if (tmp_fp == NULL) {
		(void) fclose(fp);
		menu_log("%s: %s: cannot open file for writing.", progname,
			 pathname);
		return(-1);
	}

	/*
	 *	See if hostname is in the list.  Could use ed(1) here, but
	 *	we want to know if the client was really there.
	 */
	while (fscanf(fp, "%s\n", buf) != EOF) {
		delete_blanks(buf);

						/* this is not it so copy it */
		if (strcmp(buf, client_p->hostname) != 0)
			(void) fprintf(tmp_fp, "%s\n", buf);
		else
			found_it = 1;
	}
	(void) fclose(fp);
	(void) fclose(tmp_fp);

	if (!found_it) {
		menu_log("%s: %s is not a %s client.", progname,
			 client_p->hostname, client_p->arch_str);
		return(0);
	}

	(void) sprintf(cmd, "mv %s.tmp %s.%s 2>> %s", CLIENT_LIST, CLIENT_LIST,
		       client_p->arch_str, LOGFILE);
	x_system(cmd);

	return(delete_from_all_list(client_p));
} /* end delete_client_from_list() */




/*
 *	Name:		delete_from_all_list()
 *
 *	Description:	Delete 'client_p' from CLIENT_LIST_ALL
 */

static int
delete_from_all_list(client_p)
	clnt_info *	client_p;
{
	char		buf[BUFSIZ];		/* input buffer */
	char		cmd[MAXPATHLEN * 2];	/* command buffer */
	int		found_it = 0;		/* was the name found? */
	FILE *		fp;			/* ptr to client_list.<arch> */
	char		pathname[MAXPATHLEN];	/* path to client_list.<arch>*/
	FILE *		tmp_fp;			/* ptr to client_list.tmp */

	fp = fopen(CLIENT_LIST_ALL, "r");
	/*
	 *	If there is no client_all_list, don't check it
	 */
	if (fp == NULL) {
		return(1);
	}

	(void) sprintf(pathname, "%s.tmp", CLIENT_LIST_ALL);
	tmp_fp = fopen(pathname, "w");
	if (tmp_fp == NULL) {
		(void) fclose(fp);
		menu_log("%s: %s: cannot open file for writing.", progname,
			 pathname);
		return(-1);
	}

	/*
	 *	See if hostname is in the list.  Could use ed(1) here, but
	 *	we want to know if the client was really there.
	 */
	while (fscanf(fp, "%s\n", buf) != EOF) {
						/* this is not it so copy it */
		if (strcmp(buf, client_p->hostname) != 0)
			(void) fprintf(tmp_fp, "%s\n", buf);
		else
			found_it = 1;
	}
	(void) fclose(fp);
	(void) fclose(tmp_fp);

	if (!found_it) {
		menu_log("%s: %s is not a %s client.", progname,
			 client_p->hostname, client_p->arch_str);
		return(0);
	}

	(void) sprintf(cmd, "mv %s.tmp %s 2>> %s", CLIENT_LIST_ALL,
		       CLIENT_LIST_ALL, LOGFILE);
	x_system(cmd);

	return(1);
} /* end delete_client_from_list() */




