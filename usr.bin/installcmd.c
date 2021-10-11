/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1987 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char sccsid[] = "@(#)installcmd.c 1.1 92/07/30 SMI"; /* from UCB 1.4 7/25/87 */
#endif not lint

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#define	DEF_GROUP	"staff"		/* default group */
#define	DEF_OWNER	"root"		/* default owner */
#define	DEF_MODE	0755		/* default mode */
#define STRIP		"/usr/bin/strip"	/* strip utility */

char *group = DEF_GROUP;
char *owner = DEF_OWNER;
int mode    = DEF_MODE;
struct passwd *pp;
struct group *gp;
extern int errno;
extern char *sys_errlist[];
int copy();
int strip();
void usage();

main(argc, argv)
	int	argc;
	char	**argv;
{
	extern char	*optarg;
	extern int	optind;
	struct stat	stb;
	int	(*proc)() = copy;
	char	*dirname;
	int	ch;
	int	i;
	int	rc;
	int	dflag = 0;
	int	gflag = 0;
	int	oflag = 0;
	int	mflag = 0;

	while ((ch = getopt(argc, argv, "dcg:o:m:s")) != EOF)
		switch((char)ch) {
		case 'c':
			break;	/* always do "copy" */
		case 'd':
			dflag++;
			break;
		case 'g':
			gflag++;
			group = optarg;
			break;
		case 'm':
			mflag++;
			mode = atoo(optarg);
			break;
		case 'o':
			oflag++;
			owner = optarg;
			break;
		case 's':
			proc = strip;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* get group and owner id's */
	if (!(gp = getgrnam(group))) {
		fprintf(stderr, "install: unknown group %s.\n", group);
		exit(1);
	}
	if (!(pp = getpwnam(owner))) {
		fprintf(stderr, "install: unknown user %s.\n", owner);
		exit(1);
	}

	if (dflag) {		/* install a directory */
		int exists = 0;

		if (argc != 1)
			usage();
		dirname = argv[0];
		if (mkdirp(dirname, 0777) < 0) {
			exists = errno == EEXIST;
			if (!exists) {
				fprintf(stderr, "install: mkdir: %s: %s\n", dirname, sys_errlist[errno]);
				exit(1);
			}
		}
		if (stat(dirname, &stb) < 0) {
			fprintf(stderr, "install: stat: %s: %s\n", dirname, sys_errlist[errno]);
			exit(1);
		}
		if ((stb.st_mode&S_IFMT) != S_IFDIR) {
			fprintf(stderr, "install: %s is not a directory\n", dirname);	
		}
		/* make sure directory setgid bit is inherited */
		mode = (mode & ~S_ISGID) | (stb.st_mode & S_ISGID);
		if (mflag && chmod(dirname, mode)) {
			fprintf(stderr, "install: chmod: %s: %s\n", dirname, sys_errlist[errno]);
			if (!exists)
				(void) unlink(dirname);
			exit(1) ;
		}
		if (oflag && chown(dirname, pp->pw_uid, -1) && errno != EPERM) {
			fprintf(stderr, "install: chown: %s: %s\n", dirname, sys_errlist[errno]);
			if (!exists)
				(void) unlink(dirname);
			exit(1) ;
		}
		if (gflag && chown(dirname, -1, gp->gr_gid) && errno != EPERM) {
			fprintf(stderr, "install: chgrp: %s: %s\n", dirname, sys_errlist[errno]);
			if (!exists)
				(void) unlink(dirname);
			exit(1) ;
		}
		exit(0);
	}

	if (argc < 2)
		usage();

        if (argc > 2) {		/* last arg must be a directory */
                if (stat(argv[argc-1], &stb) < 0)
                        usage();
                if ((stb.st_mode&S_IFMT) != S_IFDIR) 
                        usage();
        }
        rc = 0;
        for (i = 0; i < argc-1; i++)
                rc |= install(argv[i], argv[argc-1], proc);
        exit(rc);
	/* NOTREACHED */
}

int
install(from, to, proc)
	char *from, *to;
	int (*proc)();
{
	int devnull;
	int status = 0;
	char *path;
	struct stat from_sb, to_sb;
	static char pbuf[MAXPATHLEN];

	/* check source */
	if (stat(from, &from_sb)) {
		fprintf(stderr, "install: %s: %s\n", from, sys_errlist[errno]);
		return (1);
	}
	/* special case for removing files */
	devnull = !strcmp(from, "/dev/null");
	if (!devnull && !((from_sb.st_mode&S_IFMT) == S_IFREG)) {
		fprintf(stderr, "install: %s isn't a regular file.\n", from);
		return (1);
	}

	/* build target path, find out if target is same as source */
	if (!stat(path = to, &to_sb)) {
		if ((to_sb.st_mode&S_IFMT) == S_IFDIR) {
			char *C, *rindex();

			(void) sprintf(path = pbuf, "%s/%s", to, (C = rindex(from, '/')) ? ++C : from);
			if (stat(path, &to_sb))
				goto nocompare;
		}
		if ((to_sb.st_mode&S_IFMT) != S_IFREG) {
			fprintf(stderr, "install: %s isn't a regular file.\n", path);
			return (1);
		}
		if (to_sb.st_dev == from_sb.st_dev && to_sb.st_ino == from_sb.st_ino) {
			fprintf(stderr, "install: %s and %s are the same file.\n", from, path);
			return (1);
		}
		/* unlink now... avoid ETXTBSY errors later */
		(void) unlink(path);
	}

nocompare:
	/* copy/strip */
	if ((status = (*proc)(from, path)) != 0)
		goto inst_done;

	if (chmod(path, mode)) {
		fprintf(stderr, "install: chmod: %s: %s\n", path, sys_errlist[errno]);
		status = 1;
		goto inst_done;
	}
	if (chown(path, pp->pw_uid, gp->gr_gid) && errno != EPERM) {
		fprintf(stderr, "install: chown: %s: %s\n", path, sys_errlist[errno]);
		status = 1;
		goto inst_done;
	}

inst_done:
	if (status)
		(void) unlink(path);
	return (status);
}

/*
 * copy --
 *	copy from one file to another
 */
int
copy(from_name, to_name)
	char *from_name, *to_name;
{
	int n, from_fd, to_fd;
	int status = 0;
	char buf[MAXBSIZE];

	from_fd = -1;
	to_fd = -1;
	if ((from_fd = open(from_name, O_RDONLY, 0)) < 0) {
		fprintf(stderr, "install: open: %s: %s\n", from_name, sys_errlist[errno]);
		return (1);
	}

	/* open target */
	if ((to_fd = open(to_name, O_CREAT|O_WRONLY|O_TRUNC, DEF_MODE)) < 0) {
		fprintf(stderr, "install: %s: %s\n", to_name, sys_errlist[errno]);
		status = 1;
		goto copy_done;
	}

	while ((n = read(from_fd, buf, sizeof(buf))) > 0)
		if (write(to_fd, buf, n) != n) {
			fprintf(stderr, "install: write: %s: %s\n", to_name, sys_errlist[errno]);
		status = 1;
		goto copy_done;
		}
	if (n == -1) {
		fprintf(stderr, "install: read: %s: %s\n", from_name, sys_errlist[errno]);
		status = 1;
		goto copy_done;
	}

copy_done:
	if (from_fd != -1)
		(void) close(from_fd);
	if (to_fd != -1)
		(void) close(to_fd);
	return (status);
}

/*
 * strip --
 *	copy file, then strip(1) it 
 */
int
strip(from_name, to_name)
	char *from_name, *to_name;
{
	int status = 0;
	int pid;
	union wait wstat;

	if ((status = copy(from_name, to_name)) != 0) {
		goto strip_done;
	}

	/* fork and exec the strip utility */
	if ((pid = fork()) == 0) {
		(void) execl(STRIP, "strip", to_name, (char *) 0);
		fprintf(stderr, "install: execl: %s\n", sys_errlist[errno]);
		_exit (127);
	}

	if (pid == -1) {
		fprintf(stderr, "install: fork: %s\n", sys_errlist[errno]);
		status = 1;
		goto strip_done;
	}

	if (wait4(pid, &wstat, 0, 0) != pid) {
		fprintf(stderr, "install: wait: %s\n", sys_errlist[errno]);
		status = 1;
		goto strip_done;
	}

	/* Check child's return value (allow errors from strip) */
	if (WIFEXITED(wstat)) {
		if (wstat.w_retcode == 127) {
			fprintf(stderr, "install: execl %s failed\n", STRIP);
			status = 1;
			goto strip_done;
		}
	} else if (WIFSIGNALED(wstat)) {
		fprintf(stderr, "install: strip: terminated by signal %d\n", 
			wstat.w_termsig);
		status = 1;
		goto strip_done;
	}

strip_done:
	return (status);
}

/*
 * atoo --
 *	octal string to int
 */
int
atoo(str)
	register char	*str;
{
	register int	val;

	for (val = 0; isdigit(*str); ++str)
		val = val * 8 + *str - '0';
	return(val);
}

/*
 * usage --
 *	print a usage message and die
 */
void
usage()
{
	fputs("usage: install [-cs] [-g group] [-m mode] [-o owner] file ...  destination\n", stderr);
	fputs("       install  -d   [-g group] [-m mode] [-o owner] dir\n", stderr);
	exit(1);
}

/*
 * mkdirp --
 *	make a directory and parents if needed
 */
int
mkdirp(dir, mode)
	char *dir;
	int mode;
{
	int err;
	char *slash;
	char *rindex();
	extern int errno;

	if (mkdir(dir, mode) == 0)
		return (0);
	if (errno != ENOENT)
		return (-1);
	slash = rindex(dir, '/');
	if (slash == NULL)
		return (-1);
	*slash = '\0';
	err = mkdirp(dir, 0777);
	*slash = '/';
	if (err)
		return (err);
	return mkdir(dir, mode);
}
