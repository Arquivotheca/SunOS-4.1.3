#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)media_file.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)media_file.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 *	Name:		media_file.c
 *
 *	Description:	This file contains the routines necessary for
 *		interfacing with the 'media_file' structure file.  The
 *		layout of a 'media_file' file is as follows:
 *
 *			arch_str=
 *			media_vol=
 *			media_count=
 *			media_no=
 *				file_no=
 *				mf_name=
 *				mf_select=
 *				mf_loaded=
 *				mf_size=
 *				mf_kind=
 *				mf_type=
 *				mf_iflag=
 *				mf_loadpt=
 *				mf_depsel=
 *				mf_depcount=
 *				mf_dependent=
 *				...
 *				mf_dependent=
 *			...
 *			media_no=
 *				file_no=
 *				mf_name=
 *				mf_select=
 *				mf_loaded=
 *				mf_size=
 *				mf_kind=
 *				mf_type=
 *				mf_iflag=
 *				mf_loadpt=
 *				mf_depsel=
 *				mf_depcount=
 *				mf_dependent=
 *				...
 *				mf_dependent=
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"




/*
 *	Local variables:
 */
static	char		arch_str[MEDIUM_STR];	/* architecture string */
static	char *		char_p;			/* ptr to dependent */
static	media_file	file;			/* media file information */
static	int		media_count;		/* media count */
static	int		media_vol;		/* media volume number */




/*
 *	Key translation tables:
 */
static	key_xlat	soft_hdr[] = {
	"arch_str",	0,		0,		arch_str,
	"media_vol",	cv_str_to_int,	cv_int_to_str,	(char *) &media_vol,
	"media_count",	cv_str_to_int,	cv_int_to_str,	(char *) &media_count,

	NULL,		0,		0,		NULL
};


static	key_xlat	soft_tbl[] = {
	"media_no",	cv_str_to_int,	cv_int_to_str,	(char *) &file.media_no,
	"file_no",	cv_str_to_int,	cv_int_to_str,	(char *) &file.file_no,
	"mf_name",	cv_str_to_cpp,	cv_cpp_to_str,	(char *) &file.mf_name,
	"mf_select",	cv_str_to_ans,	cv_ans_to_str,	(char *) &file.mf_select,
	"mf_loaded",	cv_str_to_ans,	cv_ans_to_str,	(char *) &file.mf_loaded,
	"mf_size",	cv_str_to_long,	cv_long_to_str,	(char *) &file.mf_size,
	"mf_kind",	cv_str_to_kind,	cv_kind_to_str,	(char *) &file.mf_kind,
	"mf_type",      cv_str_to_type, cv_type_to_str, (char *) &file.mf_type,
	"mf_iflag",	cv_str_to_iflag, cv_iflag_to_str, (char *) &file.mf_iflag,
	"mf_loadpt",	cv_str_to_cpp,	cv_cpp_to_str,	(char *) &file.mf_loadpt,
	"mf_depsel",	cv_str_to_ans,	cv_ans_to_str,	(char *) &file.mf_depsel,
	"mf_depcount",	cv_str_to_int,	cv_int_to_str,	(char *) &file.mf_depcount,

	NULL,		0,	NULL
};


static	key_xlat	dep_tbl[] = {
	"mf_dependent",	cv_str_to_cpp,	cv_cpp_to_str,	(char *) &char_p,

	NULL,		0,	NULL
};




/*
 *	External references:
 */
extern	char *		calloc();




/*
 *	Name:		free_media_file()
 *
 *	Description:	Free the run-time space used by the media files.
 */

void
free_media_file(soft_p)
	soft_info *	soft_p;
{
	int		i;			/* index variable */
	media_file *	mf;			/* media file pointer */


	/*
	 *	Free up any previously allocated space.
	 */
	for (mf = soft_p->media_files; soft_p->media_files &&
	     mf < &soft_p->media_files[soft_p->media_count]; mf++) {
		if (mf->mf_name)		/* free media file name */
			(void) free(mf->mf_name);

		if (mf->mf_loadpt)		/* free load point */
			(void) free(mf->mf_loadpt);

		if (mf->mf_deplist) {		/* free dependency list */
			for (i = 0; mf->mf_deplist[i]; i++)
				(void) free(mf->mf_deplist[i]);

			(void) free((char *) mf->mf_deplist);
		}
	}
	if (soft_p->media_files) {
		(void) free((char *) soft_p->media_files);
		soft_p->media_files = 0;
		soft_p->media_count = 0;
	}
} /* end free_media_file() */




/*
 *	Name:		read_media_file()
 *
 *	Description:	Read media file information from the media_file file.
 *		Note: this routine does NOT initialize the space pointed to
 *		by 'soft_p->media_files' since run-time memory is allocated.
 *		The caller must initialize this space to zeros.
 *
 *		Returns 1 if the file was read successfully, 0 if the file
 *		did not exist and -1 if there was an error.
 */

int
read_media_file(name, soft_p)
	char *		name;
	soft_info *	soft_p;
{
	char		buf[BUFSIZ];		/* buffer for I/O */
	int		count;			/* counter for lines */
	FILE *		fp;			/* file pointer for name */
	int		i;			/* scratch index variable */


	free_media_file(soft_p);

	fp = fopen(name, "r");
	if (fp == NULL)
		return(0);

	for (count = 0; soft_hdr[count].key_name; count++) {
		if (fgets(buf, sizeof(buf), fp) == NULL) {
			(void) fclose(fp);
			menu_log("%s: %s: unexpected EOF.", progname, name);
			return(-1);
		}

		if (xlat_key(buf, soft_hdr, count) != 1) {
			(void) fclose(fp);
			menu_log("%s: %s: cannot translate key: '%s'", progname,
			         name, buf);
			return(-1);
		}
	}

	(void) strcpy(soft_p->arch_str, arch_str);
	soft_p->media_vol = media_vol;
	soft_p->media_count = media_count;
	if (media_count) {
		soft_p->media_files = (media_file *) calloc(
					    (unsigned int) soft_p->media_count,
					    (unsigned int) sizeof(media_file));
		if (soft_p->media_files == NULL) {
			menu_log("%s: cannot allocate memory for media files.",
				 progname);
			menu_abort(1);
		}
	}
		
	for (i = 0; i < soft_p->media_count; i++) {
		bzero((char *) &file, sizeof(file));
		for (count = 0; soft_tbl[count].key_name; count++) {
			if (fgets(buf, sizeof(buf), fp) == NULL) {
				(void) fclose(fp);
				menu_log("%s: %s: unexpected EOF.", progname,
				         name);
				return(-1);
			}

			if (xlat_key(buf, soft_tbl, count) != 1) {
				(void) fclose(fp);
				menu_log("%s: %s: cannot translate key: '%s'.",
				         progname, name, buf);
				return(-1);
			}
		}

		if (file.mf_depcount) {
			file.mf_deplist = (char **) calloc(
				   	   (unsigned int) file.mf_depcount + 1,
						(unsigned int) sizeof(char *));

			if (file.mf_deplist == NULL) {
				menu_log(
			      "%s: cannot allocate memory for dependent list.",
				         progname);
				menu_abort(1);
			}
		}

		for (count = 0; count < file.mf_depcount; count++) {
			if (fgets(buf, sizeof(buf), fp) == NULL) {
				(void) fclose(fp);
				menu_log("%s: %s: unexpected EOF.", progname,
				         name);
				return(-1);
			}

			if (xlat_key(buf, dep_tbl, 0) != 1) {
				(void) fclose(fp);
				menu_log("%s: %s: cannot translate key: '%s'.",
				         progname, name, buf);
				return(-1);
			}
			file.mf_deplist[count] = char_p;
		}

		soft_p->media_files[i] = file;
	}

	if (fgets(buf, sizeof(buf), fp) != NULL) {
		(void) fclose(fp);
		menu_log("%s: %s: file is longer than expected.", progname,
			 name);
		return(-1);
	}

	(void) fclose(fp);

	return(1);
} /* end read_media_file() */




/*
 *	Name:		replace_media_file()
 *
 *	Description:	Replace the named media file with the contents of
 *		'soft_p'.  Copy any useful information in the media file
 *		to 'soft_p' before replacing it.  Currently, the only useful
 *		piece of information is the load status.  Returns 1 if
 *		everything is okay, and -1 if there was an error.
 */

int
replace_media_file(name, soft_p)
	char *		name;
	soft_info *	soft_p;
{
	media_file *	new_p;			/* ptr to new media files */
	media_file *	old_p;			/* ptr to old media files */
	soft_info	tmp;			/* scratch software info */


	/*
	 *	Software info structure must be initialized before the
	 *	first call to read_software_info() since it allocates
	 *	and frees its own run-time memory.
	 */
	bzero((char *) &tmp, sizeof(tmp));

	switch (read_media_file(name, &tmp)) {
	/*
	 *	Return code of 1 or 0 is okay here.
	 */
	case 1:
		for (old_p = tmp.media_files;
		     old_p < &tmp.media_files[tmp.media_count]; old_p++) {

			/*
			 *	This media file is not interesting since
			 *	it is not loaded.
			 */
			if ((old_p->mf_loaded == ANS_NO) &&
			    (old_p->mf_select == ANS_NO))
				continue;

			/*
			 *	Find the media file in the new list and
			 *	mark it as loaded.  Silently ignores a
			 *	media file that is in the old list, but
			 *	not in the new list.
			 */
			for (new_p = soft_p->media_files;
			     new_p < &soft_p->media_files[soft_p->media_count];
			     new_p++) {
				if (strcmp(old_p->mf_name, new_p->mf_name))
					continue;

				if (old_p->mf_loaded == ANS_YES) {
					new_p->mf_loaded = ANS_YES;
					new_p->mf_select = ANS_NO;
				} else if (old_p->mf_select == ANS_YES) {
					new_p->mf_select = ANS_YES;
				}
			}
		}
		break;

	case 0:
		break;

	case -1:
		return(-1);
	} /* end switch */

	free_media_file(&tmp);

	if (save_media_file(name, soft_p) != 1)
		return(-1);

	return(1);
} /* end replace_media_file() */




/*
 *	Name:		save_media_file()
 *
 *	Description:	Save the media file pointed to by 'soft_p' in 'name'.
 *		Returns 1 if the file was saved successfully, and -1 if
 *		there was an error.
 */

int
save_media_file(name, soft_p)
	char *		name;
	soft_info *	soft_p;
{
	int		count;			/* counter for lines */
	FILE *		fp;			/* file pointer for name */
	int		i;			/* scratch index variable */


	fp = fopen(name, "w");
	if (fp == NULL) {
		menu_log("%s: %s: cannot open file for writing.", progname,
			 name);
		return(-1);
	}

	(void) strcpy(arch_str, soft_p->arch_str);
	media_vol = soft_p->media_vol;
	media_count = soft_p->media_count;

	for (count = 0; soft_hdr[count].key_name; count++) {
		(void) fprintf(fp, "%s=%s\n", soft_hdr[count].key_name,
			       xlat_code(soft_hdr, count));
	}

	for (i = 0; i < soft_p->media_count; i++) {
		file = soft_p->media_files[i];

		if (file.media_no == 0)
			break;

		(void) fprintf(fp, "%s=%s\n", soft_tbl[0].key_name,
			       xlat_code(soft_tbl, 0));
		/*
		 *	Skip the 0'th key since it is already done.
		 */
		for (count = 1; soft_tbl[count].key_name; count++) {
			(void) fprintf(fp, "\t%s=%s\n",
				       soft_tbl[count].key_name,
				       xlat_code(soft_tbl, count));
		}

		for (count = 0; count < file.mf_depcount; count++) {
			char_p = file.mf_deplist[count];
			(void) fprintf(fp, "\t\t%s=%s\n",
				       dep_tbl[0].key_name,
				       xlat_code(dep_tbl, 0));
		}
	}

	(void) fclose(fp);

	return(1);
} /* end save_media_file() */
