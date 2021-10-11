/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char sccsid[] = "@(#)whereis.c 1.1 92/07/30 SMI"; /* from UCB 5.1 5/31/85 */
#endif not lint

#include <sys/param.h>
#include <sys/dir.h>
#include <stdio.h>
#include <ctype.h>

static char *bindirs[] = {
	"/usr/5bin",
	"/usr/5include",
	"/usr/5lib",
	"/etc",
	"/sbin",
	"/usr/bin",
	"/usr/games",
	"/usr/ucb",
	"/usr/lib",
	"/usr/local",
	"/usr/local/bin",
	"/usr/new",
	"/usr/old",
	"/usr/hosts",
	"/usr/include",
	"/usr/etc",
	"/usr/xpg2bin",
	"/usr/xpg2lib",
	"/usr/xpg2include",
	0
};
static char *mandirs[] = {
	"/usr/man/man1",
	"/usr/man/man2",
	"/usr/man/man3",
	"/usr/man/man4",
	"/usr/man/man5",
	"/usr/man/man6",
	"/usr/man/man7",
	"/usr/man/man8",
	"/usr/man/manl",
	"/usr/man/mann",
	"/usr/man/mano",
	0
};
static char *srcdirs[]  = {
	"/usr/src/5bin",
	"/usr/src/5include",
	"/usr/src/5lib",
	"/usr/src/bin",
	"/usr/src/usr.bin",
	"/usr/src/usr.bin/sunwindows/sunview",
	"/usr/src/usr.bin/sunwindows/utilities",
	"/usr/src/etc",
	"/usr/src/usr.etc",
	"/usr/src/ucb",
	"/usr/src/games",
	"/usr/src/usr.lib",
	"/usr/src/usr.lib/libsunwindow",
	"/usr/src/usr.lib/libpixrect",
	"/usr/src/usr.lib/libsuntool",
	"/usr/src/lib",
	"/usr/src/lang",
	"/usr/src/lang/pascal",
	"/usr/src/lang/pascal/utilities",
	"/usr/src/lib/libc/gen/common",
	"/usr/src/lib/libc/gen/sys5",
	"/usr/src/lib/libc/stdio/common",
	"/usr/src/lib/libc/stdio/sys5",
	"/usr/src/lib/libc/sys/common",
	"/usr/src/lib/libc/sys/sys5",
	"/usr/src/lib/libc/net",
	"/usr/src/lib/libc/inet",
	"/usr/src/lib/libc/rcp",
	"/usr/src/lib/libc/shared",
	"/usr/src/lib/libc/shlib.etc",
	"/usr/src/lib/libc/sun",
	"/usr/src/local",
	"/usr/src/new",
	"/usr/src/old",
	"/usr/src/include",
	"/usr/src/sys/stand",
	"/usr/src/sys/sys",
	"/usr/src/ucblib",
	"/usr/src/ucbinclude",
	"/usr/src/xpg2bin",
	"/usr/src/xpg2include",
	"/usr/src/xpg2lib",
	0
};

char	sflag = 1;
char	bflag = 1;
char	mflag = 1;
char	**Sflag;
int	Scnt;
char	**Bflag;
int	Bcnt;
char	**Mflag;
int	Mcnt;
char	uflag;
/*
 * whereis name
 * look for source, documentation and binaries
 */
main(argc, argv)
	int argc;
	char *argv[];
{

	argc--, argv++;
	if (argc == 0) {
usage:
		fprintf(stderr, "whereis [ -sbmu ] [ -SBM dir ... -f ] name...\n");
		exit(1);
	}
	do
		if (argv[0][0] == '-') {
			register char *cp = argv[0] + 1;
			while (*cp) switch (*cp++) {

			case 'f':
				break;

			case 'S':
				getlist(&argc, &argv, &Sflag, &Scnt);
				break;

			case 'B':
				getlist(&argc, &argv, &Bflag, &Bcnt);
				break;

			case 'M':
				getlist(&argc, &argv, &Mflag, &Mcnt);
				break;

			case 's':
				zerof();
				sflag++;
				continue;

			case 'u':
				uflag++;
				continue;

			case 'b':
				zerof();
				bflag++;
				continue;

			case 'm':
				zerof();
				mflag++;
				continue;

			default:
				goto usage;
			}
			argv++;
		} else
			lookup(*argv++);
	while (--argc > 0);
	exit(0);
	/* NOTREACHED */
}

getlist(argcp, argvp, flagp, cntp)
	char ***argvp;
	int *argcp;
	char ***flagp;
	int *cntp;
{

	(*argvp)++;
	*flagp = *argvp;
	*cntp = 0;
	for ((*argcp)--; *argcp > 0 && (*argvp)[0][0] != '-'; (*argcp)--)
		(*cntp)++, (*argvp)++;
	(*argcp)++;
	(*argvp)--;
}


zerof()
{

	if (sflag && bflag && mflag)
		sflag = bflag = mflag = 0;
}
int	count;
int	print;


lookup(cp)
	register char *cp;
{
	register char *dp;

	for (dp = cp; *dp; dp++)
		continue;
	for (; dp > cp; dp--) {
		if (*dp == '.') {
			*dp = 0;
			break;
		}
	}
	for (dp = cp; *dp; dp++)
		if (*dp == '/')
			cp = dp + 1;
	if (uflag) {
		print = 0;
		count = 0;
	} else
		print = 1;
again:
	if (print)
		printf("%s:", cp);
	if (sflag) {
		looksrc(cp);
		if (uflag && print == 0 && count != 1) {
			print = 1;
			goto again;
		}
	}
	count = 0;
	if (bflag) {
		lookbin(cp);
		if (uflag && print == 0 && count != 1) {
			print = 1;
			goto again;
		}
	}
	count = 0;
	if (mflag) {
		lookman(cp);
		if (uflag && print == 0 && count != 1) {
			print = 1;
			goto again;
		}
	}
	if (print)
		printf("\n");
}

looksrc(cp)
	char *cp;
{
	if (Sflag == 0) {
		find(srcdirs, cp);
	} else
		findv(Sflag, Scnt, cp);
}

lookbin(cp)
	char *cp;
{
	if (Bflag == 0)
		find(bindirs, cp);
	else
		findv(Bflag, Bcnt, cp);
}

lookman(cp)
	char *cp;
{
	if (Mflag == 0) {
		find(mandirs, cp);
	} else
		findv(Mflag, Mcnt, cp);
}

findv(dirv, dirc, cp)
	char **dirv;
	int dirc;
	char *cp;
{

	while (dirc > 0)
		findin(*dirv++, cp), dirc--;
}

find(dirs, cp)
	char **dirs;
	char *cp;
{

	while (*dirs)
		findin(*dirs++, cp);
}

findin(dir, cp)
	char *dir, *cp;
{
	DIR *dirp;
	struct direct *dp;

	dirp = opendir(dir);
	if (dirp == NULL)
		return;
	while ((dp = readdir(dirp)) != NULL) {
		if (itsit(cp, dp->d_name)) {
			count++;
			if (print)
				printf(" %s/%s", dir, dp->d_name);
		}
	}
	closedir(dirp);
}

itsit(cp, dp)
	register char *cp, *dp;
{
	register int i = strlen(dp);

	if (dp[0] == 's' && dp[1] == '.' && itsit(cp, dp+2))
		return (1);
	while (*cp && *dp && *cp == *dp)
		cp++, dp++, i--;
	if (*cp == 0 && *dp == 0)
		return (1);
	while (isdigit(*dp))
		dp++;
	if (*cp == 0 && *dp++ == '.') {
		--i;
		while (i > 0 && *dp)
			if (--i, *dp++ == '.')
				return (*dp++ == 'C' && *dp++ == 0);
		return (1);
	}
	return (0);
}
