#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)mount_list.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)mount_list.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		mount_list.c
 *
 *	Description:	This file contains the routines necessary for
 *		interfacing with the 'mount_list' file.  The layout of
 *		a 'mount_list' file is as follows:
 *
 *			partition=
 *				mount_pt=
 *				preserve=
 *				fs_minlab= (SunB1)
 *				fs_maxlab= (SunB1)
 *			...
 *			partition=
 *				mount_pt=
 *				preserve=
 *				fs_minlab= (SunB1)
 *				fs_maxlab= (SunB1)
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"




/*
 *	Local variable definitions:
 */
static	mnt_ent		mnt;			/* mount entry */

/*
 *	Key translation table:
 */
static	key_xlat	mnt_key_list[] = {
	"partition",	0,		0,		mnt.partition,
	"mount_pt",	0,		0,		mnt.mount_pt,
	"preserve",	cv_str_to_char,	cv_char_to_str,	&mnt.preserve,
#ifdef SunB1
	"fs_minlab",	cv_str_to_lab,	cv_lab_to_str,	(char *) &mnt.fs_minlab,
	"fs_maxlab",	cv_str_to_lab,	cv_lab_to_str,	(char *) &mnt.fs_maxlab,
#endif /* SunB1 */

	NULL,		0,		0,		NULL
};




/*
 *	Local functions:
 */
static	int		mnt_cmp();




/*
 *	Name:		elem_count()
 *
 *	Description:	Return the number of path elements in 'path'.
 */

int
elem_count(path)
	char *		path;
{
	int		count = 0;		/* number of slashes */
	char *		s;			/* scratch pointer */


	for (s = path; *s; s++)
		if (*s == '/')
			count++;

	return(count);
} /* end elem_count() */




/*
 *	Name:		mnt_cmp()
 *
 *	Description:	Compare two mount_list entries.  Returns 1, 0, and
 *		-1 if 'a' is greater than, equal to or less than 'b'.
 *		
 */

static int
mnt_cmp(a_p, b_p)
	mnt_ent *	a_p;
	mnt_ent *	b_p;
{
	if (a_p->count == 0)
		return(1);

	if (b_p->count == 0)
		return(-1);

	if (a_p->count == b_p->count)
		return(strcmp(a_p->mount_pt, b_p->mount_pt));

	if (a_p->count < b_p->count)
		return(-1);

	return(1);
} /* end mnt_cmp() */




/*
 *	Name:		read_mount_list()
 *
 *	Description:	Read a mount_list from 'name' into the buffer
 *		pointed to by 'mount_list'.  'mount_list' must point
 *		to at least NMOUNT mnt_ent structures.
 *
 *		Returns 1 if the file was read successfully, 0 if the file
 *		did not exist and -1 if there was an error.
 */

int
read_mount_list(name, mount_list)
	char *		name;
	mnt_ent		mount_list[];
{
	char		buf[BUFSIZ];		/* buffer for I/O */
	int		count = 0;		/* count of lines */
	int		done = 0;		/* are we done? */
	FILE *		fp;			/* ptr to 'mount_list' */
	int		j;			/* index variable */


	/*
	 *	Always zero out the information buffer
	 */
	bzero((char *) mount_list, sizeof(mnt_ent) * NMOUNT);

	fp = fopen(name, "r");
	if (fp == NULL)
		return(0);

	for (j = 0; j < NMOUNT && !done; j++) {
		bzero((char *) &mnt, sizeof(mnt));

		for (count = 0; mnt_key_list[count].key_name; count++) {
			if (fgets(buf, sizeof(buf), fp) == NULL) {
				if (count == 0) {
					done = 1;
					break;
				}
				(void) fclose(fp);
				menu_log("%s: %s: unexpected EOF.", progname,
				         name);
				return(-1);
			}
			if (xlat_key(buf, mnt_key_list, count) != 1) {
				(void) fclose(fp);
				menu_log("%s: %s: cannot translate key: '%s'.",
				         progname, name, buf);
				return(-1);
			}
		}

		if (!done) {
			mnt.count = elem_count(mnt.mount_pt);
			mount_list[j] = mnt;
		}
	}

	if (fgets(buf, sizeof(buf), fp) != NULL) {
		(void) fclose(fp);
		menu_log("%s: %s: file is longer than expected.", progname,
			 name);
		return(-1);
	}

	(void) fclose(fp);

	return(1);
} /* end read_mount_list() */




/*
 *	Name:		save_mount_list()
 *
 *	Description:	Save the mount_list buffer into 'name'.  Saves
 *		upto NMOUNT structures in 'name'.
 *
 *		Returns 1 if the file was saved successfully, and -1 if
 *		there was an error.
 */

int
save_mount_list(name, mount_list)
	char *		name;
	mnt_ent		mount_list[];
{
	int		count;			/* counter for lines */
	FILE *		fp;			/* ptr to 'mount_list' */
	int		j;			/* scratch index variable */


	fp = fopen(name, "w");
	if (fp == NULL) {
		menu_log("%s: %s: cannot open file for writing.", progname,
			 name);
		return(-1);
	}

	for (j = 0; j < NMOUNT; j++) {
		mnt = mount_list[j];	/* copy valid mount entry */

		if (mnt.partition[0] == 0)
			break;

		for (count = 0; mnt_key_list[count].key_name; count++) {
			if (count > 0)
				(void) fputc('\t', fp);
			(void) fprintf(fp, "%s=%s\n",
				       mnt_key_list[count].key_name,
				       xlat_code(mnt_key_list, count));
		}
	}

	(void) fclose(fp);

	return(1);
} /* end save_mount_list() */




/*
 *	Name:		sort_mount_list()
 *
 *	Description:	Sort the entries in mount_list.
 */

void
sort_mount_list(mount_list)
	mnt_ent		mount_list[];
{
	int		count = 0;		/* number of entries */


	for (count = 0; mount_list[count].partition[0]; count++)
		/* NULL statement */ ;

	qsort((char *) mount_list, count, sizeof(mnt_ent), mnt_cmp);
} /* end sort_mount_list() */
