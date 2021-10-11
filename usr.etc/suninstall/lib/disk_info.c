#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)disk_info.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)disk_info.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		disk_info.c
 *
 *	Description:	This file contains the routines necessary for
 *		interfacing with the 'disk_info' structure file.  The
 *		layout of a 'disk_info' file is as follows:
 *
 *			disk_name=
 *			label_source=
 *			free_hog=
 *			display_unit=
 *			is_hot=
 *			swap_size=
 *
 *			disk_minlab= (SunB1)
 *			disk_maxlab= (SunB1)
 *
 *			partition='a'
 *				start_cyl=
 *				blocks=
 *				size=
 *				mount_pt=
 *				preserve=
 *				avail_bytes=
 *
 *				fs_minlab= (SunB1)
 *				fs_maxlab= (SunB1)
 *
 *			...
 *			partition='a' + NDKMAP
 *				start_cyl=
 *				blocks=
 *				size=
 *				mount_pt=
 *				preserve=
 *				avail_bytes=
 *
 *				fs_minlab= (SunB1)
 *				fs_maxlab= (SunB1)
 *
 *		Note that the dk_geom structure, 'geom_buf', is not saved.
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"




/*
 *	Local functions:
 */
static	char *		cv_du_to_str();
static	char *		cv_ls_to_str();
static	int		cv_str_to_du();
static	int		cv_str_to_ls();




/*
 *	Local variables:
 */
static	disk_info	disk;			/* local copy of disk_info */
static	part_info	part;			/* local copy of part_info */
static	char		partition;		/* current partition */


/*
 *	Key translation tables:
 */
static	key_xlat	disk_hdr[] = {
	"disk_name",	0,		0,		disk.disk_name,
	"label_source",	cv_str_to_ls,	cv_ls_to_str,	(char *) &disk.label_source,
	"free_hog",	cv_str_to_int,	cv_int_to_str,	(char *) &disk.free_hog,
	"display_unit",	cv_str_to_du,	cv_du_to_str,	(char *) &disk.display_unit,
	"is_hot",	cv_str_to_ans,	cv_ans_to_str,	(char *) &disk.is_hot,
	"swap_size",	cv_str_to_long,	cv_long_to_str,	(char *) &disk.swap_size,
#ifdef SunB1
	"disk_minlab",	cv_str_to_lab,	cv_lab_to_str,	(char *) &disk.disk_minlab,
	"disk_maxlab",	cv_str_to_lab,	cv_lab_to_str,	(char *) &disk.disk_maxlab,
#endif /* SunB1 */

	NULL,		0,		0,		NULL
};

static	key_xlat	disk_tbl[] = {
	"partition",	cv_str_to_char,	cv_char_to_str,	&partition,
	"start_cyl",	0,		0,		part.start_str,
	"blocks",	0,		0,		part.block_str,
	"size",		0,		0,		part.size_str,
	"mount_pt",	0,		0,		part.mount_pt,
	"preserve",	cv_str_to_char,	cv_char_to_str,	&part.preserve_flag,
	"avail_bytes",	cv_str_to_long,	cv_long_to_str,	(char *) &part.avail_bytes,
#ifdef SunB1
	"fs_minlab",	cv_str_to_lab,	cv_lab_to_str,	(char *) &part.fs_minlab,
	"fs_maxlab",	cv_str_to_lab,	cv_lab_to_str,	(char *) &part.fs_maxlab,
#endif /* SunB1 */

	NULL,		0,		0,		NULL
};


/*
 *	Name:		read_disk_info()
 *
 *	Description:	Read disk information from 'name' into 'disk_p'.
 *		Returns 1 if the file was read successfully, 0 if the file
 *		did not exist and -1 if there was an error.
 */

int
read_disk_info(name, disk_p)
	char *		name;
	disk_info *	disk_p;
{
	char		buf[BUFSIZ];		/* buffer for I/O */
	int		count;			/* counter for lines */
	int		done = 0;		/* done with input? */
	FILE *		fp;			/* file pointer for name */
	int		i;			/* index variable */

	extern	long		atol();


	bzero((char *) &disk, sizeof(disk));
	disk.geom_buf = disk_p->geom_buf;	/* save the disk geometry */
	/*
	 *	Always zero out the information buffer
	 */
	bzero((char *) disk_p, sizeof(*disk_p));

	fp = fopen(name, "r");
	if (fp == NULL)
		return(0);

	bzero((char *) &part, sizeof(part));
	bzero(buf, sizeof(buf));

	for (count = 0; disk_hdr[count].key_name; count++) {
		if (fgets(buf, sizeof(buf), fp) == NULL) {
			(void) fclose(fp);
			menu_log("%s: %s: unexpected EOF.", progname, name);
			return(-1);
		}

		if (xlat_key(buf, disk_hdr, count) != 1) {
			(void) fclose(fp);
			menu_log("%s: %s: cannot translate key: '%s'", progname,
			         name, buf);
			return(-1);
		}
	}

#ifdef SunB1
	/*
	 *	Set the file system labels to something initially.  This
	 *	value is replaced if there is an entry in the file.
	 */
	for (i = 0; i < NDKMAP; i++) {
		disk.partitions[i].fs_minlab = disk.disk_minlab;
		disk.partitions[i].fs_maxlab = disk.disk_maxlab;
	}
#endif /* SunB1 */

	bzero(buf, sizeof(buf));
	for (i = 0; i < NDKMAP && !done; i++) {
		for (count = 0; disk_tbl[count].key_name; count++) {
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

			if (xlat_key(buf, disk_tbl, count) == 0) {
				(void) fclose(fp);
				menu_log("%s: %s: cannot translate key: '%s'.",
				         progname, name, buf);
				return(-1);
			}
		}

		if (!done) {
			i = partition - 'a';

			disk.partitions[i] = part;
			disk.partitions[i].map_buf.dkl_cylno =
					    atol(disk.partitions[i].start_str);
			disk.partitions[i].map_buf.dkl_nblk =
					    atol(disk.partitions[i].block_str);
		}
	}

	if (fgets(buf, sizeof(buf), fp) != NULL) {
		(void) fclose(fp);
		menu_log("%s: %s: file is longer than expected.", progname,
			 name);
		return(-1);
	}

	*disk_p = disk;				/* copy valid disk_info */

	(void) fclose(fp);

	return(1);
} /* end read_disk_info() */




/*
 *	Name:		save_disk_info()
 *
 *	Description:	Save disk information pointed to by 'disk_p' in 'name'.
 *		Returns 1 if the file was saved successfully, and -1 if
 *		there was an error.
 */

int
save_disk_info(name, disk_p)
	char *		name;
	disk_info *	disk_p;
{
	int		count;			/* counter for lines */
	FILE *		fp;			/* file pointer for name */
	int		i;			/* scratch index variable */


	disk = *disk_p;				/* copy valid disk_info */

	fp = fopen(name, "w");
	if (fp == NULL) {
		menu_log("%s: %s: cannot open file for writing.", progname,
			 name);
		return(-1);
	}

	for (count = 0; disk_hdr[count].key_name; count++) {
		(void) fprintf(fp, "%s=%s\n", disk_hdr[count].key_name,
			       xlat_code(disk_hdr, count));
	}

	for (i = 0; i < NDKMAP; i++) {
		partition = i + 'a';
		part = disk.partitions[i];

		if (map_blk(partition) == 0)
			continue;

		(void) fprintf(fp, "%s=%s\n", disk_tbl[0].key_name,
			       xlat_code(disk_tbl, 0));

		/*
		 *	Skip the 0'th key since it is already done.
		 */
		for (count = 1; disk_tbl[count].key_name; count++) {
			(void) fprintf(fp, "\t%s=%s\n",
				       disk_tbl[count].key_name,
				       xlat_code(disk_tbl, count));
		}
	}

	(void) fclose(fp);

	return(1);
} /* end save_disk_info() */




static	conv		du_list[] = {
	"Kbytes",	DU_KBYTES,
	"Mbytes",	DU_MBYTES,
	"blocks",	DU_BLOCKS,
	"cylinders",	DU_CYLINDERS,

	(char *) 0,	0
};


/*
 *	Name:		cv_du_to_str()
 *
 *	Description:	Convert disk units into a string.  If the disk
 *		unit cannot be converted, then NULL is returned.
 */

static char *
cv_du_to_str(du_p)
	int *		du_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = du_list; cv->conv_text; cv++)
		if (cv->conv_value == *du_p)
			return(cv->conv_text);
	return(NULL);
} /* end cv_du_to_str() */




/*
 *	Name:		cv_str_to_du()
 *
 *	Description:	Convert string to disk units.  If the string
 *		cannot be converted, then 0 is returned.
 */

static int
cv_str_to_du(str, data_p)
	char *		str;
	int *		data_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = du_list; cv->conv_text; cv++)
		if (strcmp(cv->conv_text, str) == 0) {
			*data_p = cv->conv_value;
			return(1);
		}

	*data_p = 0;
	return(0);
} /* end cv_str_to_du() */




static	conv		ls_list[] = {
	"default",	DKL_DEFAULT,
	"mod_label",	DKL_MODIFY,
	"use_label",	DKL_EXISTING,
	"saved_label",	DKL_SAVED,

	(char *) 0,	0
};


/*
 *	Name:		cv_ls_to_str()
 *
 *	Description:	Convert a disk label code into a string.  If the
 *		disk label code cannot be converted, then NULL is returned.
 */

static char *
cv_ls_to_str(ls_p)
	int *		ls_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = ls_list; cv->conv_text; cv++)
		if (cv->conv_value == *ls_p)
			return(cv->conv_text);
	return(NULL);
} /* end cv_ls_to_str() */




/*
 *	Name:		cv_str_to_ls()
 *
 *	Description:	Convert a string into a disk label code.  If the
 *		string cannot be converted, then 0 is returned.
 */

static int
cv_str_to_ls(str, data_p)
	char *		str;
	int *		data_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = ls_list; cv->conv_text; cv++)
		if (strcmp(cv->conv_text, str) == 0) {
			*data_p = cv->conv_value;
			return(1);
		}

	*data_p = 0;
	return(0);
} /* end cv_str_to_ls() */
