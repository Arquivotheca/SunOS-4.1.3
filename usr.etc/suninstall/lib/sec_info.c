#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)sec_info.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)sec_info.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		sec_info.c
 *
 *	Description:	This file contains the routines necessary for
 *		interfacing with the 'sec_info' structure file.  The
 *		layout of a 'sec_info' file is as follows:
 *
 *			hostname=
 *			root_word=
 *			audit_word=
 *			audit_flags=
 *			audit_min=
 *			audit_dircount=
 *			audit_dir=
 *			...
 *			audit_dir=
 *
 *		Note:	the root_ck and audit_ck fields are not saved.
 */

#include <stdio.h>
#include <string.h>
#include "install.h"
#include "menu.h"




/*
 *	Local variables:
 */
static	char		audit_dir[MAXPATHLEN];	/* audit directory name */
static	int		dir_count;		/* directory count */
static	sec_info	sec;			/* security information */




/*
 *	Key translation tables:
 */
static	key_xlat	sec_hdr[] = {
	"hostname",	0,		0,		sec.hostname,
	"root_word",	0,		0,		sec.root_word,
	"audit_word",	0,		0,		sec.audit_word,
	"audit_flags",	0,		0,		sec.audit_flags,
	"audit_min",	0,		0,		sec.audit_min,
	"audit_dircount", cv_str_to_int, cv_int_to_str, (char *) &dir_count,

	NULL,		0,		0,		NULL
};


static	key_xlat	sec_tbl[] = {
	"audit_dir",	0,		0,		audit_dir,

	NULL,		0,	NULL
};




/*
 *	Name:		read_sec_info()
 *
 *	Description:	Read security information from the sec_info file.
 *
 *		Returns 1 if the file was read successfully, 0 if the file
 *		did not exist and -1 if there was an error.
 */

int
read_sec_info(name, sec_p)
	char *		name;
	sec_info *	sec_p;
{
	char		buf[BUFSIZ];		/* buffer for I/O */
	int		count;			/* counter for lines */
	FILE *		fp;			/* file pointer for name */
	int		i;			/* scratch index variable */


	/*
	 *	Always zero out the information buffer.
	 */
	bzero((char *) sec_p, sizeof(*sec_p));

	fp = fopen(name, "r");
	if (fp == NULL)
		return(0);

	for (count = 0; sec_hdr[count].key_name; count++) {
		if (fgets(buf, sizeof(buf), fp) == NULL) {
			(void) fclose(fp);
			menu_log("%s: %s: unexpected EOF.", progname, name);
			return(-1);
		}

		if (xlat_key(buf, sec_hdr, count) != 1) {
			(void) fclose(fp);
			menu_log("%s: %s: cannot translate key: '%s'", progname,
			         name, buf);
			return(-1);
		}
	}

	for (i = 0; i < dir_count; i++) {
		bzero(audit_dir, sizeof(audit_dir));

		for (count = 0; sec_tbl[count].key_name; count++) {
			if (fgets(buf, sizeof(buf), fp) == NULL) {
				(void) fclose(fp);
				menu_log("%s: %s: unexpected EOF.", progname,
				         name);
				return(-1);
			}

			if (xlat_key(buf, sec_tbl, count) != 1) {
				(void) fclose(fp);
				menu_log("%s: %s: cannot translate key: '%s'.",
				         progname, name, buf);
				return(-1);
			}
		}

		(void) strcpy(sec.audit_dirs[i], audit_dir);
	}

	if (fgets(buf, sizeof(buf), fp) != NULL) {
		(void) fclose(fp);
		menu_log("%s: %s: file is longer than expected.", progname,
			 name);
		return(-1);
	}

	*sec_p = sec;				/* copy valid sec_info */

	(void) fclose(fp);

	return(1);
} /* end read_sec_info() */




/*
 *	Name:		save_sec_info()
 *
 *	Description:	Save the security information pointed to by 'sec_p'
 *		into 'name'.
 *
 *		Returns 1 if the file was saved successfully, and -1 if
 *		there was an error.
 */

int
save_sec_info(name, sec_p)
	char *		name;
	sec_info *	sec_p;
{
	int		count;			/* counter for lines */
	FILE *		fp;			/* file pointer for name */
	int		i;			/* scratch index variable */


	sec = *sec_p;

	fp = fopen(name, "w");
	if (fp == NULL) {
		menu_log("%s: %s: cannot open file for writing.", progname,
			 name);
		return(-1);
	}

	for (dir_count = 0, i = 0; i < MAX_AUDIT_DIRS; i++)
		if (sec.audit_dirs[i][0])
			dir_count++;

	for (count = 0; sec_hdr[count].key_name; count++) {
		(void) fprintf(fp, "%s=%s\n", sec_hdr[count].key_name,
			       xlat_code(sec_hdr, count));
	}

	for (i = 0; i < MAX_AUDIT_DIRS; i++) {
		if (sec.audit_dirs[i][0] == NULL)
			continue;

		(void) strcpy(audit_dir, sec.audit_dirs[i]);

		for (count = 0; sec_tbl[count].key_name; count++) {
			(void) fprintf(fp, "%s=%s\n",
				       sec_tbl[count].key_name,
				       xlat_code(sec_tbl, count));
		}
	}

	(void) fclose(fp);

	return(1);
} /* end save_sec_info() */
