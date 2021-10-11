#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)xlat.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)xlat.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		xlat.c
 *
 *	Description:	This file contains the data file translation
 *		routines for suninstall.
 */

#include <stdio.h>
#include <string.h>
#include "install.h"


/*
 *	Name:		xlat_code()
 *
 *	Description:	Translate the data in the n'th element in 'list'
 *		into a string.  If the data does not have a translation
 *		routine, then assume it is a string and copy it.
 */

char *
xlat_code(list, n)
	key_xlat	list[];
	int		n;
{
	static	char		buf[BUFSIZ];	/* output buffer */


	/*
 	 *	If no translation is needed, then just copy
 	 *	the code value to a safe place.
 	 */
	if (list[n].code_func == NULL) {
		(void) strcpy(buf, list[n].data_p);
		return(buf);
	}

	/*
	 *	Need to translate the code value:
	 */
	return((list[n].code_func)(list[n].data_p));
} /* end xlat_code() */




/*
 *	Name:		xlat_key()
 *
 *	Description:	Translate data buffer 'key' using the n'th element
 *		in 'list'.  Return 0 if the key does not match or the
 *		value does not translate.  Return 1 if the key matches
 *		and the value translates or if the key matches and the
 *		value is just a string with no translation needed.
 */

int
xlat_key(buf, list, n)
	char *		buf;
	key_xlat	list[];
	int		n;
{
	char		key_name[SMALL_STR];	/* key name */
	char		key_value[MAXPATHLEN];	/* key value */
	char *		ptr;			/* scratch char pointer */


	delete_blanks(buf);

						/* break out key name */
	bzero(key_name, sizeof(key_name));
	for (ptr = key_name; *buf != '='; ptr++, buf++)
		*ptr = *buf;
						/* break out key value */
	bzero(key_value, sizeof(key_value));
	for (ptr = key_value, buf++; *buf != '\0'; ptr++, buf++)
		*ptr = *buf;

	/*
	 *	Not the right key
	 */
	if (strcmp(key_name, list[n].key_name) != 0)
		return(0);

	/*
 	 *	If no translation is needed, then just copy
 	 *	the key value to a safe place.
 	 */
	if (list[n].key_func == NULL) {
		(void) strcpy(list[n].data_p, key_value);
		return(1);
	}

	/*
	 *	Need to translate the key value:
	 */
	return((list[n].key_func)(key_value, list[n].data_p));
} /* end xlat_key() */
