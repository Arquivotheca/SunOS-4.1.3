#ifndef lint
#ifdef SunB1
#ident			"@(#)clnt_info.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)clnt_info.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		clnt_info.c
 *
 *	Description:	This file contains the routines necessary for
 *		interfacing with the 'clnt_info' structure file.  The
 *		layout of a 'clnt_info' file is as follows:
 *
 *			hostname=
 *			arch_str=
 *			ip=
 *
 *			ip_minlab= (SunB1)
 *			ip_maxlab= (SunB1)
 *
 *			ether=
 *			swap_size=
 *			root_path=
 *			swap_path=
 *			home_path=
 *			exec_path=
 *			kvm_path=
 *			mail_path=
 *			share_path=
 *			crash_path=
 *			yp_type=
 *			domainname=
 *			created=
 *			small_kernel=
 *
 *		Note: the 'choice' field is not saved.
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"


/*
 *	Local variables:
 */
static	clnt_info	clnt;			/* local copy of clnt_info */


/*
 *	Key translation table:
 */
static	key_xlat	key_list[] = {
	"hostname",	0,		0,		clnt.hostname,
	"arch_str",	0,		0,		clnt.arch_str,
	"ip",		0,		0,		clnt.ip,
#ifdef SunB1
	"ip_minlab",	cv_str_to_lab,	cv_lab_to_str,	(char *) &clnt.ip_minlab,
	"ip_maxlab",	cv_str_to_lab,	cv_lab_to_str,	(char *) &clnt.ip_maxlab,
#endif /* SunB1 */
	"ether",	0,		0,		clnt.ether,
	"swap_size",	0,		0,		clnt.swap_size,
	"root_path",	0,		0,		clnt.root_path,
	"swap_path",	0,		0,		clnt.swap_path,
	"home_path",	0,		0,		clnt.home_path,
	"exec_path",	0,		0,		clnt.exec_path,
	"kvm_path",	0,		0,		clnt.kvm_path,
	"mail_path",	0,		0,		clnt.mail_path,
	"share_path",	0,		0,		clnt.share_path,
	"crash_path",	0,		0,		clnt.crash_path,
	"yp_type",	cv_str_to_yp,	cv_yp_to_str,	(char *) &clnt.yp_type,
	"domainname",	0,		0,		clnt.domainname,
	"created",	cv_str_to_ans,	cv_ans_to_str,	(char *) &clnt.created,
	"termtype",	0,		0,		clnt.termtype,
	"small_kernel", cv_str_to_ans,  cv_ans_to_str,  (char *) &clnt.small_kernel,
	NULL,		0,		0,		NULL
};


/*
 *	Name:		read_clnt_info()
 *
 *	Description:	Read client information from 'name' into 'clnt_p'.
 *
 *		Returns 1 if the file was read successfully, 0 if the file
 *		did not exist and -1 if there was an error.
 */

int
read_clnt_info(name, clnt_p)
	char *		name;
	clnt_info *	clnt_p;
{
	char		buf[BUFSIZ];		/* buffer for I/O */
	int		count;			/* counter for lines */
	FILE *		fp;			/* file pointer for name */


	/*
	 *	Always zero out the information buffer
	 */
	bzero((char *) clnt_p, sizeof(*clnt_p));

	fp = fopen(name, "r");
	if (fp == NULL)
		return(0);

	bzero((char *) &clnt, sizeof(clnt));
	bzero(buf, sizeof(buf));

	for (count = 0; key_list[count].key_name; count++) {
		if (fgets(buf, sizeof(buf), fp) == NULL) {
			(void) fclose(fp);
			menu_log("%s: %s: unexpected EOF.", progname, name);
			return(-1);
		}

		if (xlat_key(buf, key_list, count) != 1) {
			(void) fclose(fp);
			menu_log("%s: %s: cannot translate key: '%s'", progname,
			    name, buf);
			return(-1);
		}
	}

	if (fgets(buf, sizeof(buf), fp) != NULL) {
		(void) fclose(fp);
		menu_log("%s: %s: file is longer than expected.", progname,
			 name);
		return(-1);
	}

	*clnt_p = clnt;				/* copy valid clnt_info */

	(void) fclose(fp);

	return(1);
} /* end read_clnt_info() */




/*
 *	Name:		save_clnt_info()
 *
 *	Description:	Save the client information pointed to by 'clnt_p'
 *		into 'name'.
 *
 *		Returns 1 if the file was saved successfully, and -1 if
 *		there was an error.
 */

int
save_clnt_info(name, clnt_p)
	char *		name;
	clnt_info *	clnt_p;
{
	int		count;			/* counter for lines */
	FILE *		fp;			/* file pointer for name */


	clnt = *clnt_p;				/* copy valid clnt_info */

	fp = fopen(name, "w");
	if (fp == NULL) {
		menu_log("%s: %s: cannot open file for writing.", progname,
			 name);
		return(-1);
	}

	for (count = 0; key_list[count].key_name; count++) {
		(void) fprintf(fp, "%s=%s\n", key_list[count].key_name,
			       xlat_code(key_list, count));
	}

	(void) fclose(fp);

	return(1);
} /* end save_clnt_info() */




/*
 *	Name:		add_client_to_list()
 *
 *	Description:	Add 'client_p' to the list of clients for the
 *		architecture specified in client_p->arch.  The CLIENT_LIST
 *		is only updated if the client is not already in the file.
 */

int
add_client_to_list(client_p)
	clnt_info *	client_p;
{
	char		buf[BUFSIZ];		/* input buffer */
	FILE *		fp;			/* ptr to client_list.<arch> */
	char		pathname[MAXPATHLEN];	/* path to client_list.<arch>*/
	
	(void) sprintf(pathname, "%s.%s", CLIENT_LIST,
		       client_p->arch_str);
	if (fp = fopen(pathname, "r")) {
		/*
		 *	See if hostname is already in the list
		 */
		while (fgets(buf, sizeof(buf), fp)) {
			delete_blanks(buf);

						/* found it */
			if (strcmp(buf, client_p->hostname) == 0) {
				(void) fclose(fp);
				return(1);
			}
		}
		(void) fclose(fp);
	}

	/*
	 *	Either the file is not there or the hostname is not in
	 *	the list.  In any case, add the name to the list.
	 */
	fp = fopen(pathname, "a");
	if (fp == NULL) {
		menu_log("%s: %s: cannot open file for appending.", progname,
			 pathname);
		return(0);
	}

	(void) fprintf(fp, "%s\n", client_p->hostname);
	(void) fclose(fp);


	/*
	 *	Now, if we are not in miniroot, add this name to a private
	 *	list, so that add_client knows which clients to make.
	 */
	if (!is_miniroot()) {
		fp = fopen(CLIENT_LIST_ALL, "a");
		if (fp == NULL) {
			menu_log("%s: %s: cannot open file for appending.",
				 progname, CLIENT_LIST_ALL);
			return(0);
		}
		
		(void) fprintf(fp, "%s\n", client_p->hostname);
		(void) fclose(fp);
	}

	return(1);
} /* end add_client_to_list() */

