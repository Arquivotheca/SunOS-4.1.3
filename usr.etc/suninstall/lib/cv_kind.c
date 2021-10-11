#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)cv_kind.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)cv_kind.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		cv_kind.c
 *
 *	Description:	Convert sundist media file 'kinds' into strings
 *		and back again.
 */

#include "install.h"


static	conv		kind_list[] = {
	"amorphous",		KIND_AMORPHOUS,
	"contents",		KIND_CONTENTS,
	"executable",		KIND_EXECUTABLE,
	"install_tool",		KIND_INSTALL_TOOL,
	"installable",		KIND_INSTALLABLE,
	"standalone",		KIND_STANDALONE,
	"undefined",		KIND_UNDEFINED,

	(char *) 0,	0
};


/*
 *	Name:		cv_kind_to_str()
 *
 *	Description:	Convert a sundist media file 'kind' into a string.
 *		Returns NULL if the 'kind' cannot be converted.
 */

char *
cv_kind_to_str(kind_p)
	int *		kind_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = kind_list; cv->conv_text; cv++)
		if (cv->conv_value == *kind_p)
			return(cv->conv_text);
	return(NULL);
} /* end cv_kind_to_str() */




/*
 *	Name:		cv_str_to_kind()
 *
 *	Description:	Convert a string into a sundist media file kind.
 *		Returns 1 if string was converted and 0 otherwise.
 */

int
cv_str_to_kind(str, data_p)
	char *		str;
	int *		data_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = kind_list; cv->conv_text; cv++)
		if (strcmp(cv->conv_text, str) == 0) {
			*data_p = cv->conv_value;
			return(1);
		}

	*data_p = 0;
	return(0);
} /* end cv_str_to_kind() */
