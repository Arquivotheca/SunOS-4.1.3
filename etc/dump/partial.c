#if !defined(lint) && !defined(NOID)
static char sccsid[] = "@(#)partial.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1989, Sun Microsystems, Inc.
 */

#include "dump.h"
#include <ftw.h>

static dev_t blockdev;
static int partial;
static char realdisk[32];

extern int errno;
extern char *estrdup();
extern char *strcpy(), *strcat(), *rindex();

static int ftw_check();
static int ftw_mark();
static char *strerror();


partial_check()
{
	struct stat st;

	if (stat(disk, &st) < 0 ||
		(st.st_mode & S_IFMT) == S_IFCHR ||
		(st.st_mode & S_IFMT) == S_IFBLK)
		return;

	blockdev = st.st_dev;

	if (lftw("/dev", ftw_check, 0) <= 0) {
		msg("Cannot find block device %d, %d\n",
			major(blockdev), minor(blockdev));
		dumpabort();
	}

	disk = strcpy(realdisk, rawname(realdisk));

	partial = 1;
	incno = '0';
	uflag = 0;
}

static
ftw_check(name, st, flag)
	char *name;
	struct stat *st;
	int flag;
{
	if (flag == FTW_F &&
		(st->st_mode & S_IFMT) == S_IFBLK &
		st->st_rdev == blockdev) {
		(void) strcpy(realdisk, name);
		return 1;
	}
	return 0;
}


partial_mark(argc, argv)
	int argc;
	char *argv[];
{
	char *path;
	struct stat st;

	if (partial == 0)
		return 1;

	while (--argc >= 0) {
		path = *argv++;

		if (stat(path, &st) < 0 ||
			st.st_dev != blockdev) {
			msg("%s is not on device %s\n",
				path, disk);
			dumpabort();
		}

		if (mark_root(blockdev, path)) {
			msg("Cannot find filesystem mount point for %s\n",
 				path);
			dumpabort();
		}

		if (lftw(path, ftw_mark, getdtablesize() / 2) < 0) {
			msg("Error in ftw (%s)\n", strerror(errno));
			dumpabort();
		}
	}

	return 0;
}

/* mark directories between target and root */
static
mark_root(dev, path)
	dev_t dev;
	char *path;
{
	struct stat st;
	char dotdot[MAXPATHLEN + 16];
	char *slash;

	strcpy(dotdot, path);

	if (stat(dotdot, &st) < 0)
		return 1;

	/* if target is a regular file, find directory */
	if ((st.st_mode & S_IFMT) != S_IFDIR)
		if (slash = rindex(dotdot, '/'))
			/* "/file" -> "/" */
			if (slash == dotdot)
				slash[1] = 0;
			/* "dir/file" -> "dir" */
			else
				slash[0] = 0;
		else
			/* "file" -> "." */
			strcpy(dotdot, ".");

	/* keep marking parent until we hit mount point */
	do {
		if (stat(dotdot, &st) < 0 ||
			(st.st_mode & S_IFMT) != S_IFDIR ||
			st.st_dev != dev)
			return 1;
		markino(st.st_ino);
		strcat(dotdot, "/..");
	} while (st.st_ino != 2);

	return 0;
}

/*ARGSUSED*/
static
ftw_mark(name, st, flag)
	char *name;
	struct stat *st;
	int flag;
{
	if (flag != FTW_NS)
		markino(st->st_ino);

	return 0;
}

static
markino(i)
	ino_t i;
{
	struct dinode *dp;

	dp = getino(ino = i);
	mark(dp);
}

static char *
strerror(err)
	int err;
{
	extern int sys_nerr;
	extern char *sys_errlist[];

	static char errmsg[32];

	if (err >= 0 && err < sys_nerr)
		return sys_errlist[sys_nerr];

	(void) sprintf(errmsg, "Error %d", err);
	return errmsg;
}
