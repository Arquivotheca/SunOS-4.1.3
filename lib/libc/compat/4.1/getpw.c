#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)getpw.c 1.1 92/07/30  Copyr 1984 Sun Micro";
#endif

/* 
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <pwd.h>

getpw(uid, buf)
	int uid;
	char buf[];
{
	struct passwd *pw;
	char numbuf[20];

	pw = getpwuid(uid);
	if(pw == 0)
		return 1;
	strcpy(buf, pw->pw_name);
	strcat(buf, ":");
	strcat(buf, pw->pw_passwd);
	strcat(buf, ":");
	sprintf(numbuf, "%d", pw->pw_uid);
	strcat(buf, numbuf);
	strcat(buf, ":");
	sprintf(numbuf, "%d", pw->pw_gid);
	strcat(buf, numbuf);
	strcat(buf, ":");
#if 0
	sprintf(numbuf, "%d", pw->pw_quota);
	strcat(buf, numbuf);
	strcat(buf, ":");
	strcat(buf, pw->pw_comment);
	strcat(buf, ":");
#endif
	strcat(buf, pw->pw_gecos);
	strcat(buf, ":");
	strcat(buf, pw->pw_dir);
	strcat(buf, ":");
	strcat(buf, pw->pw_shell);
	return 0;
}
