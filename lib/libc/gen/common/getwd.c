#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)getwd.c 1.1 92/07/30 SMI"; /* from UCB 5.1 83/05/30 */
#endif not lint
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * getwd() returns the pathname of the current working directory. On error
 * an error message is copied to pathname and null pointer is returned.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <mntent.h>
#include <sys/errno.h>
#include <strings.h>

#define GETWDERR(s)	strcpy(pathname, (s));

struct lastpwd {	/* Info regarding the previous call to getwd */
	dev_t dev;
	ino_t ino;
	char name[MAXPATHLEN];
};

extern int errno;
static struct lastpwd *lastone = NULL;	/* Cached entry */
static int pathsize;		/* pathname length */

extern char *calloc();
extern long strtol();

/*LINTLIBRARY*/

char *
getwd(pathname)
	char *pathname;
{
	char pathbuf[MAXPATHLEN];	/* temporary pathname buffer */
	char *pnptr = &pathbuf[(sizeof pathbuf)-1]; /* pathname pointer */
	char curdir[MAXPATHLEN];	/* current directory buffer */
	char *dptr = curdir;		/* directory pointer */
	char *prepend();		/* prepend dirname to pathname */
	dev_t cdev, rdev;		/* current & root device number */
	ino_t cino, rino;		/* current & root inode number */
	DIR *dirp;			/* directory stream */
	struct direct *dir;		/* directory entry struct */
	struct stat d, dd;		/* file status struct */
	dev_t newone_dev;		/* Device number of the new pwd */
	ino_t newone_ino;		/* Inode number of the new pwd */

	pathsize = 0;
	*pnptr = '\0';
	strcpy(dptr, "./");
	dptr += 2;
	if (stat(curdir, &d) < 0) {
		GETWDERR("getwd: can't stat .");
		return (NULL);
	}

	/* Cache the pwd entry */
	if (lastone == NULL) {
		lastone = (struct lastpwd *) calloc(1, sizeof (struct lastpwd));
	} else if ((d.st_dev == lastone->dev) && (d.st_ino == lastone->ino)) {
		if ((stat(lastone->name, &dd) == 0) &&
		    (d.st_dev == dd.st_dev) && (d.st_ino == dd.st_ino)) {
			/* Cache hit. */
			strcpy(pathname, lastone->name);
			return (pathname);
		}
	}

	newone_dev = d.st_dev;
	newone_ino = d.st_ino;

	if (stat("/", &dd) < 0) {
		GETWDERR("getwd: can't stat /");
		return (NULL);
	}
	rdev = dd.st_dev;
	rino = dd.st_ino;
	for (;;) {
		cino = d.st_ino;
		cdev = d.st_dev;
		strcpy(dptr, "../");
		dptr += 3;
		if ((dirp = opendir(curdir)) == NULL) {
			GETWDERR("getwd: can't open ..");
			return (NULL);
		}
		if (fstat(dirp->dd_fd, &d) == -1) {
			GETWDERR("getwd: can't stat ..");
			return (NULL);
		}
		if (cdev == d.st_dev) {
			if (cino == d.st_ino) {
				/* reached root directory */
				closedir(dirp);
				break;
			}
			if (cino == rino && cdev == rdev) {
				/*
				 * This case occurs when '/' is loopback
				 * mounted on the root filesystem somewhere.
				 */
				goto do_mount_pt;
			}
			do {
				if ((dir = readdir(dirp)) == NULL) {
					closedir(dirp);
					GETWDERR("getwd: read error in ..");
					return (NULL);
				}
			} while (dir->d_ino != cino);
		} else { /* It is a mount point */
			char tmppath[MAXPATHLEN];

do_mount_pt:
			/*
			 * Get the path name for the given dev number
			 */
			if (getdevinfo(cino, cdev, d.st_ino, d.st_dev, tmppath)) {
				closedir(dirp);
				pnptr = prepend(tmppath, pnptr);
				break;
			}

			do {
				if ((dir = readdir(dirp)) == NULL) {
					closedir(dirp);
					GETWDERR("getwd: read error in ..");
					return (NULL);
				}
				strcpy(dptr, dir->d_name);
				(void) lstat(curdir, &dd);
			} while (dd.st_ino != cino || dd.st_dev != cdev);
		}
		pnptr = prepend("/", prepend(dir->d_name, pnptr));
		closedir(dirp);
	}
	if (*pnptr == '\0')	/* current dir == root dir */
		strcpy(pathname, "/");
	else
		strcpy(pathname, pnptr);
	lastone->dev = newone_dev;
	lastone->ino = newone_ino;
	strcpy(lastone->name, pathname);
	return (pathname);
}

/*
 * prepend() tacks a directory name onto the front of a pathname.
 */
static char *
prepend(dirname, pathname)
	register char *dirname;
	register char *pathname;
{
	register int i;			/* directory name size counter */

	for (i = 0; *dirname != '\0'; i++, dirname++)
		continue;
	if ((pathsize += i) < MAXPATHLEN)
		while (i-- > 0)
			*--pathname = *--dirname;
	return (pathname);
}

/*
 * Gets the path name for the given device number. Returns 1 if
 * successful, else returns 0.
 */
static int
getdevinfo(ino, dev, parent_ino, parent_dev, path)
	ino_t ino;
	dev_t dev;
	ino_t parent_ino;
	dev_t parent_dev;
	char *path;
{
	register struct mntent *mnt;
	FILE *mounted;
	dev_t mntdev;
	char *str;
	char *equal;
	struct stat statb;
	int retval = 0;

	/*
	 * It reads the device id from /etc/mtab file and compares it
	 * with the given dev/ino combination.
	 */
	if ((mounted = setmntent(MOUNTED, "r")) == NULL)
		return (retval);
	while (mnt = getmntent(mounted)) {
		if ((strcmp(mnt->mnt_type, MNTTYPE_IGNORE)) &&
		    (str = hasmntopt(mnt, MNTINFO_DEV)) &&
		    (equal = index(str, '=')) &&
		    (mntdev = (dev_t)strtol(&equal[1], (char **)NULL, 16)) &&
		    (mntdev == dev)) {
			/* Verify once again */
			if ((lstat(mnt->mnt_dir, &statb) < 0) ||
			    (statb.st_dev != dev) ||
			    (statb.st_ino != ino))
				continue;
			/*
			 * verify that the parent dir is correct (may not
			 * be if there are loopback mounts around.)
			 */
			strcpy(path, mnt->mnt_dir);
			strcat(path, "/..");
			if ((lstat(path, &statb) < 0) ||
			    (statb.st_dev != parent_dev) ||
			    (statb.st_ino != parent_ino))
				continue;
			*rindex(path, '/') = '\0';	/* Delete /.. */
			retval = 1;
			break;
		}
	}
	(void) endmntent(mounted);
	return (retval);
}
