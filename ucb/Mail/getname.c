#ifndef lint
static	char *sccsid = "@(#)getname.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif


/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 */

#include "rcv.h"
#include <pwd.h>

/*
 * Search the passwd file for a uid.  Return name through ref parameter
 * if found, indicating success with 0 return.  Return -1 on error.
 * If -1 is passed as the user id, close the passwd file.
 */

getname(uid, namebuf)
	char namebuf[];
{
	register struct passwd *pw;

	if (uid == -1) {
		endpwent();
		return(0);
	}
	if ((pw = getpwuid(uid)) == NULL)
		return(-1);
	strcpy(namebuf, pw->pw_name);
	return(0);
}

/*
 * Convert the passed name to a user id and return it.  Return -1
 * on error.  Iff the name passed is NULL close the pwfile.
 */

getuserid(name)
	char name[];
{
	register struct passwd *pw;

	if (name == NULL) {
		endpwent();
		return(0);
	}
	if ((pw = getpwnam(name)) == NULL)
		return(-1);
	return(pw->pw_uid);
}
