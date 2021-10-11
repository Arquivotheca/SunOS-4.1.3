/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)chmod.c 1.1 92/07/30 SMI"; /* from UCB 5.5 5/22/86 */
#endif

/*
 * chmod options mode files
 * where
 *	mode is [ugoa][+-=][rwxXstugo] or an octal number
 *	options are -Rf
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>

#define MAXRET	254	/* ensure error status modulo 256 is non-zero */

char	*modestring, *ms;
#ifndef S5EMUL
int	um;
#endif
int	status;
int	fflag;
int	rflag;

main(argc, argv)
	char *argv[];
{
	register char *p, *flags;
	register int i;
	struct stat st;

	if (argc < 3) {
		fprintf(stderr,
		    "Usage: chmod [-Rf] [ugoa][+-=][rwxXstugo] file ...\n");
		exit(-1);
	}
	argv++, --argc;
	while (argc > 0 && argv[0][0] == '-') {
		for (p = &argv[0][1]; *p; p++) switch (*p) {

		case 'R':
			rflag++;
			break;

		case 'f':
			fflag++;
			break;

		default:
			goto done;
		}
		argc--, argv++;
	}
done:
	modestring = argv[0];
#ifndef S5EMUL
	um = umask(0);
#endif
	(void) newmode(0);
	for (i = 1; i < argc; i++) {
		p = argv[i];
		/* do stat for directory arguments */
		if (lstat(p, &st) < 0) {
			status += Perror(p);
			continue;
		}
		if (rflag && (st.st_mode&S_IFMT) == S_IFDIR) {
			status += chmodr(p, st.st_mode);
			continue;
		}
		if ((st.st_mode&S_IFMT) == S_IFLNK && stat(p, &st) < 0) {
			status += Perror(p);
			continue;
		}
		if (chmod(p, newmode(st.st_mode)) < 0) {
			status += Perror(p);
			continue;
		}
	}
	if (status > MAXRET)
		status = MAXRET;
	exit(status);
	/* NOTREACHED */
}

chmodr(dir, mode)
	char *dir;
{
	register DIR *dirp;
	register struct direct *dp;
	register struct stat st;
	char savedir[1024];
	int ecode;

	ecode = 0;

	if (getwd(savedir) == 0)
		fatal(255, "%s", savedir);
	/*
	 * Change what we are given before doing it's contents
	 */
	if (chmod(dir, newmode(mode)) < 0)
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
			ecode += chmodr(dp->d_name, st.st_mode);
			continue;
		}
		if ((st.st_mode&S_IFMT) == S_IFLNK) {
			continue;
		}
		if (chmod(dp->d_name, newmode(st.st_mode)) < 0) {
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
		fprintf(stderr, "chmod: ");
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
		fprintf(stderr, "chmod: ");
		perror(s);
	}
	return (!fflag);
}

#define	USER	05700	/* user's bits */
#define	GROUP	02070	/* group's bits */
#define	OTHER	00007	/* other's bits */
#define	ALL	01777	/* all (note absence of setuid, etc) */

#define	READ	00444	/* read permit */
#define	WRITE	00222	/* write permit */
#define	EXEC	00111	/* exec permit */
#define	SETID	06000	/* set[ug]id */
#define	STICKY	01000	/* sticky bit */

newmode(nm)
	unsigned nm;
{
	register o, m, b;
	int savem;

	ms = modestring;
	savem = nm;
	m = abs();
	if (*ms == '\0') {
		/* Must use symbolic mode to change set-gid bit of directory */
		if ((nm & S_IFMT) == S_IFDIR) {
			m &= ~(SETID & GROUP);
			m |= nm & (SETID & GROUP);
		}
		return (m);
	}
	if (ms != modestring)
		fatal(255, "invalid mode");
	do {
		m = who();
		while (o = what()) {
			b = where(nm);
			switch (o) {
			case '+':
				nm |= b & m;
				break;
			case '-':
				nm &= ~(b & m);
				break;
			case '=':
				/* Use +- to change set-gid bit of directory */
				if ((nm & S_IFMT) == S_IFDIR) {
					nm &= ~(m & ~(SETID & GROUP));
					nm |= (b & ~(SETID & GROUP)) & m;
				} else {
					nm &= ~m;
					nm |= b & m;
				}
				break;
			}
		}
	} while (*ms++ == ',');
	if (*--ms)
		fatal(255, "invalid mode");
	return (nm);
}

abs()
{
	register c, i;

	i = 0;
	while ((c = *ms++) >= '0' && c <= '7')
		i = (i << 3) + (c - '0');
	ms--;
	return (i);
}

who()
{
	register m;

	m = 0;
	for (;;) switch (*ms++) {
	case 'u':
		m |= USER;
		continue;
	case 'g':
		m |= GROUP;
		continue;
	case 'o':
		m |= OTHER;
		continue;
	case 'a':
		m |= ALL;
		continue;
	default:
		ms--;
		if (m == 0)
#ifdef S5EMUL
			m = ALL;
#else
			m = ALL & ~um;
#endif
		return (m);
	}
}

what()
{

	switch (*ms) {
	case '+':
	case '-':
	case '=':
		return (*ms++);
	}
	return (0);
}

where(om)
	register om;
{
	register m;

 	m = 0;
	switch (*ms) {
	case 'u':
		m = (om & USER) >> 6;
		goto dup;
	case 'g':
		m = (om & GROUP) >> 3;
		goto dup;
	case 'o':
		m = (om & OTHER);
	dup:
		m &= (READ|WRITE|EXEC);
		m |= (m << 3) | (m << 6);
		++ms;
		return (m);
	}
	for (;;) switch (*ms++) {
	case 'r':
		m |= READ;
		continue;
	case 'w':
		m |= WRITE;
		continue;
	case 'x':
		m |= EXEC;
		continue;
	case 'X':
		if (((om & S_IFMT) == S_IFDIR) || (om & EXEC))
			m |= EXEC;
		continue;
	case 's':
		m |= SETID;
		continue;
	case 't':
		m |= STICKY;
		continue;
	default:
		ms--;
		return (m);
	}
}
