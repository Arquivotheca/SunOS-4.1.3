#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)cv_iflag.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)cv_iflag.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		cv_iflag.c
 *
 *	Description:	Convert sundist 'iflag' values into strings and
 *		back again.
 */

#include "install.h"


static	conv		list[] = {
	"required",	IFLAG_REQ,
	"desirable",	IFLAG_DES,
	"common",	IFLAG_COMM,
	"optional",	IFLAG_OPT,

	(char *) 0,	0
};


/*
 *	Name:		cv_iflag_to_str()
 *
 *	Description:	Convert a sundist 'iflag' value into a string.
 *		Returns NULL if the 'iflag' value cannot be converted.
 */

char *
cv_iflag_to_str(iflag_p)
	int *		iflag_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = list; cv->conv_text; cv++)
		if (cv->conv_value == *iflag_p)
			return(cv->conv_text);
	return(NULL);
} /* end cv_iflag_to_str() */




/*
 *	Name:		cv_str_to_iflag()
 *
 *	Description:	Convert a string into a sundist 'iflag' value.
 *		Returns 1 if string was converted and 0 otherwise.
 */

int
cv_str_to_iflag(str, data_p)
	char *		str;
	int *		data_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = list; cv->conv_text; cv++)
		if (strcmp(cv->conv_text, str) == 0) {
			*data_p = cv->conv_value;
			return(1);
		}

	*data_p = 0;
	return(0);
} /* end cv_str_to_iflag() */
