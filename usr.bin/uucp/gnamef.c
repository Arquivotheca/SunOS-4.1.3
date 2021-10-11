/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)gnamef.c 1.1 92/07/30"	/* from SVR3.2 uucp:gnamef.c 2.3 */

#include "uucp.h"

/*
 * get next file name from directory
 *	p	 -> file description of directory file to read
 *	filename -> address of buffer to return filename in
 *		    must be of size DIRSIZ+1
 * returns:
 *	FALSE	-> end of directory read
 *	TRUE	-> returned name
 */
gnamef(p, filename)
register char *filename;
DIR *p;
{
#if defined(POSIXDIRLIB) || defined(BSD4_2)
	register DIRENT *dp;
#else /* !defined(POSIXDIRLIB) && !defined(BSD4_2) */
	DIRENT dentry;
	register DIRENT *dp = &dentry;
#endif /* !defined(POSIXDIRLIB) && !defined(BSD4_2) */

	while (1) {
#if defined(POSIXDIRLIB) || defined(BSD4_2)
		if ((dp = readdir(p)) == NULL)
			return(FALSE);
#else
		if (fread((char *)dp,  sizeof(dentry), 1, p) != 1)
			return(FALSE);
		if (dp->d_ino == 0)
			continue;
#endif
		if (dp->d_name[0] != '.')
			break;
	}

	(void) strncpy(filename, dp->d_name, MAXBASENAME);
	filename[MAXBASENAME] = '\0';
	return(TRUE);
}

/*
 * get next directory name from directory
 *	p	 -> file description of directory file to read
 *	filename -> address of buffer to return filename in
 *		    must be of size DIRSIZ+1
 * returns:
 *	FALSE	-> end of directory read
 *	TRUE	-> returned dir
 */
gdirf(p, filename, dir)
register char *filename;
DIR *p;
char *dir;
{
	char statname[MAXNAMESIZE];

	while (1) {
		if(gnamef(p, filename) == FALSE)
			return(FALSE);
		(void) sprintf(statname, "%s/%s", dir, filename);
		DEBUG(4, "stat %s\n", statname);
		if (DIRECTORY(statname))
		    break;
	}

	return(TRUE);
}
