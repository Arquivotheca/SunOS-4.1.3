#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)cv_yp.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)cv_yp.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 *	Name:		cv_yp.c
 *
 *	Description:	Convert NIS codes into strings and back again.
 */

#include "install.h"


static	conv		list[] = {
	"client",	YP_CLIENT,
	"master",	YP_MASTER,
	"none",		YP_NONE,
	"slave",	YP_SLAVE,

	(char *) 0,	0
};


/*
 *	Name:		cv_tp_to_str()
 *
 *	Description:	Convert a NIS code into a string.  Returns
 *		NULL if the code cannot be converted.
 */

char *
cv_yp_to_str(yp_p)
	int *		yp_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = list; cv->conv_text; cv++)
		if (cv->conv_value == *yp_p)
			return(cv->conv_text);
	return(NULL);
} /* end cv_yp_to_str() */




/*
 *	Name:		cv_str_to_yp()
 *
 *	Description:	Convert a string into a NIS code.
 *		Returns 1 if string was converted and 0 otherwise.
 */

int
cv_str_to_yp(str, data_p)
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
} /* end cv_str_to_yp() */
