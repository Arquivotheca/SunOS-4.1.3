#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)soft_info.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)soft_info.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		soft_info.c
 *
 *	Description:	This file contains the routines necessary for
 *		interfacing with the 'soft_info' structure file.  The
 *		layout of a 'soft_info' file is as follows:
 *
 *			arch_str=
 *			exec_path=
 *			kvm_path=
 *			share_path=
 *			choice=
 *			media_dev=
 *			media_path=
 *			media_loc=
 *			media_host=
 *			media_ip=
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"




/*
 *	Local functions:
 */
static	char *	cv_choice_to_str();
static	char *	cv_loc_to_str();
static	int	cv_str_to_choice();
static	int	cv_str_to_loc();




/*
 *	Local variables:
 */
static	soft_info	soft;			/* software information */




/*
 *	Key translation tables:
 */
static	key_xlat	soft_hdr[] = {
	"arch_str",	0,		0,		soft.arch_str,
	"exec_path",	0,		0,		soft.exec_path,
	"kvm_path",	0,		0,		soft.kvm_path,
	"share_path",	0,		0,		soft.share_path,
	"choice",	cv_str_to_choice, cv_choice_to_str, (char *) & soft.choice,
	"media_dev",	cv_str_to_media, cv_media_to_str, (char *) &soft.media_dev,
	"media_path",	0,		0,		soft.media_path,
	"media_loc",	cv_str_to_loc,	cv_loc_to_str,	(char *) &soft.media_loc,
	"media_host",	0,		0,		soft.media_host,
	"media_ip",	0,		0,		soft.media_ip,

	NULL,		0,		0,		NULL
};




/*
 *	External references:
 */
extern	char *		calloc();




/*
 *	Name:		read_soft_info()
 *
 *	Description:	Read software information from soft_info file.
 *		Note: this routine does NOT initialize the space pointed
 *		to by 'soft_p' since run-time memory is allocated.  The
 *		caller must initialize this space.
 *
 *		Returns 1 if the file was read successfully, 0 if the file
 *		did not exist and -1 if there was an error.
 */

int
read_soft_info(name, soft_p)
	char *		name;
	soft_info *	soft_p;
{
	char		buf[BUFSIZ];		/* buffer for I/O */
	int		count;			/* counter for lines */
	FILE *		fp;			/* file pointer for name */


	fp = fopen(name, "r");
	if (fp == NULL)
		return(0);

	bzero((char *) &soft, sizeof(soft));

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

	if (fgets(buf, sizeof(buf), fp) != NULL) {
		(void) fclose(fp);
		menu_log("%s: %s: file is longer than expected.", progname,
			 name);
		return(-1);
	}

	*soft_p = soft;				/* copy valid soft_info */

	/* setup rest of info - device handling is really a mess */
	(void) media_set_soft(soft_p, soft_p->media_dev);

	(void) fclose(fp);

	return(1);
} /* end read_soft_info() */




/*
 *	Name:		save_soft_info()
 *
 *	Description:	Save the software info pointed to by 'soft_p'
 *		in the file 'name'.
 *
 *		Returns 1 if the file was saved successfully, and -1 if
 *		there was an error.
 */

int
save_soft_info(name, soft_p)
	char *		name;
	soft_info *	soft_p;
{
	int		count;			/* counter for lines */
	FILE *		fp;			/* file pointer for name */


	soft = *soft_p;				/* copy valid soft_info */

	fp = fopen(name, "w");
	if (fp == NULL) {
		menu_log("%s: %s: cannot open file for writing.", progname,
			 name);
		return(-1);
	}

	for (count = 0; soft_hdr[count].key_name; count++) {
		(void) fprintf(fp, "%s=%s\n", soft_hdr[count].key_name,
			       xlat_code(soft_hdr, count));
	}

	(void) fclose(fp);

	return(1);
} /* end save_soft_info() */




static	conv		choice_list[] = {
	"all",		SOFT_ALL,
	"default",	SOFT_DEF,
	"own",		SOFT_OWN,
	"required",	SOFT_REQ,
	"saved",	SOFT_SAVED,

	(char *) 0,	0
};




/*
 *	Name:		cv_choice_to_str()
 *
 *	Description:	Convert a software choice into a string.  Returns
 *		NULL if the choice cannot be converted.
 */

static char *
cv_choice_to_str(choice_p)
	int *		choice_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = choice_list; cv->conv_text; cv++)
		if (cv->conv_value == *choice_p)
			return(cv->conv_text);
	return(NULL);
} /* end cv_choice_to_str() */




/*
 *	Name:		cv_str_to_choice()
 *
 *	Description:	Convert a string into a software choice.  If the
 *		string cannot be converted, then 0 is returned.
 */

static int
cv_str_to_choice(str, data_p)
	char *		str;
	int *		data_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = choice_list; cv->conv_text; cv++)
		if (strcmp(cv->conv_text, str) == 0) {
			*data_p = cv->conv_value;
			return(1);
		}

	*data_p = 0;
	return(0);
} /* end cv_str_to_choice() */




static	conv		loc_list[] = {
	"local",	LOC_LOCAL,
	"remote",	LOC_REMOTE,

	(char *) 0,	0
};




/*
 *	Name:		cv_loc_to_str()
 *
 *	Description:	Convert a media location into a string.  If the media
 *		location cannot be converted, then NULL is returned.
 */

static char *
cv_loc_to_str(loc_p)
	int *		loc_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = loc_list; cv->conv_text; cv++)
		if (cv->conv_value == *loc_p)
			return(cv->conv_text);
	return(NULL);
} /* end cv_loc_to_str() */




/*
 *	Name:		cv_str_to_loc()
 *
 *	Description:	Convert a string into a media location.  If the
 *		string cannot be converted, then 0 is returned.
 */

static int
cv_str_to_loc(str, data_p)
	char *		str;
	int *		data_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = loc_list; cv->conv_text; cv++)
		if (strcmp(cv->conv_text, str) == 0) {
			*data_p = cv->conv_value;
			return(1);
		}

	*data_p = 0;
	return(0);
} /* end cv_str_to_loc() */
