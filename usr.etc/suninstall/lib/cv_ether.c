#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)cv_ether.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)cv_ether.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		cv_ether.c
 *
 *	Description:	Convert ethernet codes into strings and back again.
 */

#include "install.h"


static	conv		ether_list[] = {
	"ie",		ETHER_IE,
	"le",		ETHER_LE,

	(char *) 0,	0
};


/*
 *	Name:		cv_ether_to_str()
 *
 *	Description:	Convert an ethernet code into a string.  Returns
 *		NULL if the ether code cannot be converted.
 */

char *
cv_ether_to_str(ether_p)
	int *		ether_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = ether_list; cv->conv_text; cv++)
		if (cv->conv_value == *ether_p)
			return(cv->conv_text);
	return(NULL);
} /* end cv_ether_to_str() */




/*
 *	Name:		cv_str_to_ether()
 *
 *	Description:	Convert a string into an ethernet code.  Returns 1 if
 *		string was converted and 0 otherwise.
 */

int
cv_str_to_ether(str, data_p)
	char *		str;
	int *		data_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = ether_list; cv->conv_text; cv++)
		if (strcmp(cv->conv_text, str) == 0) {
			*data_p = cv->conv_value;
			return(1);
		}

	*data_p = 0;
	return(0);
} /* end cv_str_to_ether() */
