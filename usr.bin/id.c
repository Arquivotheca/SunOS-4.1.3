/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)id.c 1.1 92/07/30 SMI"; /* from S5R3 1.4 */
#endif

#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <sys/param.h>

main()
{
	int uid, gid, euid, egid;
	static char stdbuf[BUFSIZ];

	setbuf (stdout, stdbuf);

	uid = getuid();
	gid = getgid();
	euid = geteuid();
	egid = getegid();

	puid ("uid", uid);
	pgid (" gid", gid);
	if (uid != euid)
		puid (" euid", euid);
	if (gid != egid)
		pgid (" egid", egid);
	pgroups ();
	putchar ('\n');

	exit(0);
	/* NOTREACHED */
}

puid (s, id)
	char *s;
	int id;
{
	struct passwd *pw;
	struct passwd *getpwuid();

	printf ("%s=%d", s, id);
	setpwent();
	pw = getpwuid(id);
	if (pw)
		printf ("(%s)", pw->pw_name);
}

pgid (s, id)
	char *s;
	int id;
{
	struct group *gr;
	struct group *getgrgid();

	printf ("%s=%d", s, id);
	setgrent();
	gr = getgrgid(id);
	if (gr)
		printf ("(%s)", gr->gr_name);
}

pgroups ()
{
	int groupids[NGROUPS];
	int *idp;
	struct group *gr;
	struct group *getgrgid();
	int i;

	i = getgroups(NGROUPS, groupids);
	if (i > 0) {
		printf (" groups=");
		for (idp = groupids; i--; idp++) {
			printf ("%d", *idp);
			setgrent();
			gr = getgrgid(*idp);
			if (gr)
				printf ("(%s)", gr->gr_name);
			if (i)
				putchar (',');
		}
	}
}
