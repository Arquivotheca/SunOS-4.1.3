/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)uucpname.c 1.1 92/07/30"	/* from SVR3.2 uucp:uucpname.c 2.3 */

#include "uucp.h"

/*
 * get the uucp name
 * return:
 *	none
 */
void
uucpname(name)
register char *name;
{
	char *s;

#ifdef BSD4_2
	int nlen;
	char	NameBuf[MAXBASENAME + 1];

	gethostname(NameBuf, MAXBASENAME);
	/* strip off any domain name part */
	if ((s = index(NameBuf, '.')) != NULL)
		*s = '\0';
	s = NameBuf;
	s[MAXBASENAME] = '\0';
#else /* !BSD4_2 */
#ifdef UNAME
	struct utsname utsn;

	uname(&utsn);
	s = utsn.nodename;
#else /* !UNAME */
	char	NameBuf[MAXBASENAME + 1], *strchr();
	FILE	*NameFile;

	s = MYNAME;
	NameBuf[0] = '\0';

	if ((NameFile = fopen("/etc/whoami", "r")) != NULL) {
		/* etc/whoami wins */
		(void) fgets(NameBuf, MAXBASENAME + 1, NameFile);
		(void) fclose(NameFile);
		NameBuf[MAXBASENAME] = '\0';
		if (NameBuf[0] != '\0') {
			if ((s = strchr(NameBuf, '\n')) != NULL)
				*s = '\0';
			s = NameBuf;
		}
	}
#endif /* UNAME */
#endif /* BSD4_2 */

	(void) strncpy(name, s, MAXBASENAME);
	name[MAXBASENAME] = '\0';
	return;
}
