#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)cv_type.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)cv_type.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		cv_type.c
 *
 *	Description:	Convert sundist media file 'types' into strings
 *		and back again.
 */

#include "install.h"
#include "toc.h"


static	conv		type_list[] = {
	"unknown",		UNKNOWN,
	"toc",			TOC,
	"image",		IMAGE,
	"tar",			TAR,
	"cpio",			CPIO,
	"bar",			BAR,
	"tarZ",			TARZ,

	(char *) 0,	0
};


/*
 *	Name:		cv_type_to_str()
 *
 *	Description:	Convert a sundist media file 'type' into a string.
 *		Returns NULL if the 'type' cannot be converted.
 */

char *
cv_type_to_str(type_p)
	int *		type_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = type_list; cv->conv_text; cv++)
		if (cv->conv_value == *type_p)
			return(cv->conv_text);
	return(NULL);
} /* end cv_type_to_str() */




/*
 *	Name:		cv_str_to_type()
 *
 *	Description:	Convert a string into a sundist media file type.
 *		Returns 1 if string was converted and 0 otherwise.
 */

int
cv_str_to_type(str, data_p)
	char *		str;
	int *		data_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = type_list; cv->conv_text; cv++)
		if (strcmp(cv->conv_text, str) == 0) {
			*data_p = cv->conv_value;
			return(1);
		}

	*data_p = 0;
	return(0);
} /* end cv_str_to_type() */
