#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)cv_ans.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)cv_ans.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		cv_ans.c
 *
 *	Description:	Routines for converting yes/no answers from binary
 *		to strings and back again.
 */

#include "install.h"


static	conv		list[] = {
	"no",		ANS_NO,
	"yes",		ANS_YES,

	(char *) 0,	0
};


/*
 *	Name:		cv_ans_to_str()
 *
 *	Description:	Convert an answer into a string.  Returns NULL
 *		if the answer cannot be converted.
 */

char *
cv_ans_to_str(ans_p)
	int *		ans_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = list; cv->conv_text; cv++)
		if (cv->conv_value == *ans_p)
			return(cv->conv_text);
	return(NULL);
} /* end cv_ans_to_str() */




/*
 *	Name:		cv_str_to_ans()
 *
 *	Description:	Convert a string into an answer.  Returns 1 if
 *		string was converted and 0 otherwise.
 */

int
cv_str_to_ans(str, data_p)
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
} /* end cv_str_to_ans() */
