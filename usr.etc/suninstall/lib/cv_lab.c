#ifndef lint
static	char		mls_sccsid[] = "@(#)cv_lab.c 1.1 92/07/30 SMI; SunOS MLS";
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		cv_lab.c
 *
 *	Description:	Convert SunOS MLS label codes into strings
 *		and back again
 */

#include "install.h"


static	conv		lab_list[] = {
	"other",	LAB_OTHER,
	"system_high",	LAB_SYS_HIGH,
	"system_low",	LAB_SYS_LOW,

	(char *) 0,	0
};


/*
 *	Name:		cv_lab_to_str()
 *
 *	Description:	Convert a SunOS MLS label code into a string.
 *		Returns NULL if the label code cannot be converted.
 */

char *
cv_lab_to_str(lab_p)
	int *		lab_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = lab_list; cv->conv_text; cv++)
		if (cv->conv_value == *lab_p)
			return(cv->conv_text);
	return(NULL);
} /* end cv_lab_to_str() */




/*
 *	Name:		cv_str_to_lab()
 *
 *	Description:	Convert a string into a SunOS MLS label code.
 *		Returns 1 if string was converted and 0 otherwise.
 */

int
cv_str_to_lab(str, data_p)
	char *		str;
	int *		data_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = lab_list; cv->conv_text; cv++)
		if (strcmp(cv->conv_text, str) == 0) {
			*data_p = cv->conv_value;
			return(1);
		}

	*data_p = 0;
	return(0);
} /* end cv_str_to_lab() */
