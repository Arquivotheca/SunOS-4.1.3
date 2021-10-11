/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif

#ifndef lint
static	char *sccsid = "@(#)chown.c 1.1 92/07/30 SMI"; /* from UCB 5.6 5/29/86 */
#endif

/*
 * chown [-fR] uid[.gid] file ...
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sys/dir.h>
#include <grp.h>
#include <strings.h>

#define MAXRET	254	/* ensure error status modulo 256 is non-zero */

struct	passwd *pwd;
struct	passwd *getpwnam();
struct	stat stbuf;
int	uid;
int	status;
int	fflag;
int	rflag;

main(argc, argv)
	char *argv[];
{
	register int c, gid;
	register char *cp, *group;
	struct group *grp;

	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++) switch (*cp) {

		case 'f':
			fflag++;
			break;

		case 'R':
			rflag++;
			break;

		default:
			fatal(255, "unknown option: %c", *cp);
		}
		argv++, argc--;
	}
	if (argc < 2) {
		fprintf(stderr, "usage: chown [-fR] owner[.group] file ...\n");
		exit(-1);
	}
	if (geteuid() != 0)  {
		if (!fflag)
			(void) fprintf(stderr, "Must be root to use chown\n");
		exit(1);
	}
	gid = -1;
	group = index(argv[0], '.');
	if (group != NULL) {
		*group++ = '\0';
		if (!isnumber(group)) {
			if ((grp = getgrnam(group)) == NULL)
				fatal(255, "unknown group: %s",group);
			gid = grp -> gr_gid;
			(void) endgrent();
		} else if (*group != '\0')
			gid = atoi(group);
	}
	if (!isnumber(argv[0])) {
		if ((pwd = getpwnam(argv[0])) == NULL)
			fatal(255, "unknown user id: %s",argv[0]);
		uid = pwd->pw_uid;
	} else
		uid = atoi(argv[0]);
	for (c = 1; c < argc; c++) {
		/* do stat for directory arguments */
		if (lstat(argv[c], &stbuf) < 0) {
			status += Perror(argv[c]);
			continue;
		}
		if (rflag && ((stbuf.st_mode&S_IFMT) == S_IFDIR)) {
			status += chownr(argv[c], uid, gid);
			continue;
		}
		if (chown(argv[c], uid, gid)) {
			status += Perror(argv[c]);
			continue;
		}
	}
	if (status > MAXRET)
		status = MAXRET;
	exit(status);
	/* NOTREACHED */
}

isnumber(s)
	char *s;
{
	register c;

	while(c = *s++)
		if (!isdigit(c))
			return (0);
	return (1);
}

chownr(dir, uid, gid)
	char *dir;
{
	register DIR *dirp;
	register struct direct *dp;
	struct stat st;
	char savedir[1024];
	int ecode;
	extern char *getwd();

	ecode = 0;

	if (getwd(savedir) == (char *)0)
		fatal(255, "%s", savedir);
	/*
	 * Change what we are given before doing it's contents.
	 */
	if (chown(dir, uid, gid) < 0)
		ecode += Perror(dir);

	if (chdir(dir) < 0) {
		Perror(dir);
		return (1);
	}
	if ((dirp = opendir(".")) == NULL) {
		Perror(dir);
		return (1);
	}
	dp = readdir(dirp);
	dp = readdir(dirp); /* read "." and ".." */
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if (lstat(dp->d_name, &st) < 0) {
			ecode += Perror(dp->d_name);
			continue;
		}
		if ((st.st_mode&S_IFMT) == S_IFDIR) {
			ecode += chownr(dp->d_name, uid, gid);
			continue;
		}
		if (chown(dp->d_name, uid, gid) < 0) {
			ecode += Perror(dp->d_name);
			continue;
		}
	}
	closedir(dirp);
	if (chdir(savedir) < 0)
		fatal(255, "can't change back to %s", savedir);
	return (ecode);
}

error(fmt, a)
	char *fmt, *a;
{

	if (!fflag) {
		fprintf(stderr, "chown: ");
		fprintf(stderr, fmt, a);
		putc('\n', stderr);
	}
	return (!fflag);
}

fatal(status, fmt, a)
	int status;
	char *fmt, *a;
{

	fflag = 0;
	(void) error(fmt, a);
	exit(status);
}

Perror(s)
	char *s;
{

	if (!fflag) {
		fprintf(stderr, "chown: ");
		perror(s);
	}
	return (!fflag);
}
