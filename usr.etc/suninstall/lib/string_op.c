#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)string_op.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)string_op.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 * 	Copyright (c) 1990 by Sun Microsystems,Inc
 */

/***************************************************************************
**
** this file holds all the string manipulation functions
**
****************************************************************************
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>


extern	char	*sprintf();
extern	char	*malloc();

#define mstr(s) (strcpy(malloc((unsigned)strlen((s))),(s)) )

char	*strlwr();
char	*strupr();
char	*strstr_ignore();
void	itoa();


/**************************************************************************
** function : (char *) strstr_ignore()
**
** This function trys to find the first occurance of string s2 in s1, and
** is the same as strstr(), but it is case insensitive, and is a
** non-destructive function.
**
** PARAMETERS : s1 : string to search
**		s2 : string to search for
**
** RETURN VALUE char *   :  points to location of s1 where s2 starts
**		NULL     :  s2 is not contained in s1
**
**************************************************************************
*/
char *
strstr_ignore(s1,s2)
	char	*s1;
	char	*s2;
{
	char	*t1 = mstr(s1);		/* point to copied string */
	char	*t2 = mstr(s2);		/* point to copied string */
	char 	*result;
	char	*strlwr();
	
	/* convert both copied strings to lower case */
	(void) strlwr(t1);
	(void) strlwr(t2);

	result = strstr(t1, t2);

	if (result != (char *)NULL)
		result = s1 + (unsigned long)(result - t1);

	free(t1);
	free(t2);
	return(result);

}


/**************************************************************************
** function : (char *) strlwr()
**
** This function takes a string and converts it to lower case.
**
** WARNING : it is a destructive function and will change 
** 		string passed to it.
**
** PARAMETERS : the string to convert to lower case, which must be null
**		terminated.
**
** RETURN VALUE char *   :  points to the lower case string
**
**************************************************************************
*/
char *
strlwr(string)
	char 	*string;
{
	char *p;

	p = string;

	while(*p) {
		if (isupper(*p))
			*p = tolower(*p);
		p++;
	}
	return(string);
}

/**************************************************************************
** function : (char *) strupr()
**
** This function takes a string and converts it to upper case.
**
** WARNING : it is a destructive function and will change 
** 		string passed to it.
**
** PARAMETERS : the string to convert to upper case must be null terminated.
**
** RETURN VALUE char *   :  pointer to the upper case string
**
**************************************************************************
*/
char *
strupr(string)
	char 	*string;
{
	char *p;

	p = string;

	while(*p) {
		if (islower(*p))
			*p = toupper(*p);
		p++;
	}
	return(string);
}


/**************************************************************************
** function : (void) itoa()
**
** This function takes an integer and converts it to a string.
**
** PARAMETERS : the integer "number" is put into the character array
**		pointed to by "string"
**
** RETURN VALUE : none
**
**************************************************************************
*/
void
itoa(number, string)
	int number;		/* number to convert */
	char string[];
{
	(void) sprintf(string, "%d", number);
}


