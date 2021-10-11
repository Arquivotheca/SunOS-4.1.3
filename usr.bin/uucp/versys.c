/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)versys.c 1.1 92/07/30"	/* from SVR3.2 uucp:versys.c 2.3 */

#include "uucp.h"

/*
 * verify system name
 * input:
 *	name	-> system name (char name[NAMESIZE])
 * returns:  
 *	0	-> success
 *	FAIL	-> failure
 */
versys(name)
char *name;
{
	register char *iptr;
	char line[BUFSIZ];
	extern char *aliasFind();

	if (name == 0 || *name == 0)
		return(FAIL);

	if ((iptr = aliasFind(name)) != NULL) {
		/* overwrite the original name with the real name */
		strncpy(name, iptr, MAXBASENAME);
		name[MAXBASENAME] = '\0';
	}

	if (EQUALS(name, Myname))
		return(0);

	while (getsysline(line, sizeof(line))) {
		if((line[0] == '#') || (line[0] == ' ') || (line[0] == '\t') || 
			(line[0] == '\n'))
			continue;

		if ((iptr=strpbrk(line, " \t")) == NULL)
		    continue;	/* why? */
		*iptr = '\0';
		if (EQUALS(name, line)) {
			sysreset();
			return(0);
		}
	}
	sysreset();
	return(FAIL);
}
