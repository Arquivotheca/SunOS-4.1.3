#ifndef lint
static	char sccsid[] = "@(#)eject.c 1.1 92/07/30 SMI";
#else
extern char *sprintf();
#endif
/*
 * Copyright (c) 1989 Sun Microsystems, Inc. All rights reserved.
 */

/*
 * eject - program to eject removeable media
 *
 * XXX limited to floppies and CDROMs (for now)
 *	planned to later enhance it to eject SCSI floppies, tapes, CDROMS,
 * has more dodads than maybe it should, but it is very friendly.
 *
 * returns exit codes:	(KEEP THESE - especially important for query)
 *	0 = -n, -d or eject operation was ok, -q = media in drive
 *	1 = -q only = media not in drive
 *	2 = various parameter errors, etc.
 *	3 = eject ioctl failed
 */
#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <mntent.h>
#include <sys/ioccom.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>

#define	STILL_MOUNTED	0
#define	UNMOUNTED	1

#define	IT_IS_UNKNOWN	0
#define	IT_IS_FLOPPY	1
#define	IT_IS_CDROM	2

char *myname;
char *default_dev = { "/dev/rfd0c" };

struct niknames {
	char	*nn;
	char	*rn;
	short	dt;
} niknames[] = {
/* nicknames for floppy */
	{ "fd", "/dev/rfd0c", IT_IS_FLOPPY },
	{ "fd0", "/dev/rfd0c", IT_IS_FLOPPY },
	{ "floppy", "/dev/rfd0c", IT_IS_FLOPPY },
	/* so people trying to do ioctls on block devices don't freak */
	{ "/dev/fd0", "/dev/rfd0c", IT_IS_FLOPPY },
	{ "/dev/fd0a", "/dev/rfd0c", IT_IS_FLOPPY },
	{ "/dev/fd0b", "/dev/rfd0c", IT_IS_FLOPPY },
	{ "/dev/fd0c", "/dev/rfd0c", IT_IS_FLOPPY },
/* nicknames for cdrom */
	{ "sr", "/dev/rsr0", IT_IS_CDROM },
	{ "sr0", "/dev/rsr0", IT_IS_CDROM },
	{ "cd", "/dev/rsr0", IT_IS_CDROM },
	{ "cdrom", "/dev/rsr0", IT_IS_CDROM },
	{ "/dev/sr0", "/dev/rsr0", IT_IS_CDROM },
	{ "sr1", "/dev/rsr1", IT_IS_CDROM },
	{ "/dev/sr1", "/dev/rsr1", IT_IS_CDROM },
	{ "sr2", "/dev/rsr2", IT_IS_CDROM },
	{ "/dev/sr2", "/dev/rsr2", IT_IS_CDROM },
	{ "sr3", "/dev/rsr3", IT_IS_CDROM },
	{ "/dev/sr3", "/dev/rsr3", IT_IS_CDROM },
	{ 0, 0, 0 }
};

main(argc, argv)
int	argc;
char	*argv[];
{
	int	fd;
	int	err;
	int	force = 0;
	int	query = 0;
	int	out = -1;
	char	*fname, *opened, c;
	struct niknames *np;
	extern int errno;
	extern char *optarg;
	extern int optind, opterr;
	struct stat st;

	myname = argv[0];

	while ((c = getopt(argc, argv, "dfnq")) != -1) {
		switch (c) {
		case 'd':	/* show default */
			(void)printf("%s: default device is \"%s\"\n", myname,
			    default_dev);
			out = 0;
			break;

		case 'f':	/* force eject, even if busy */
			force++;
			break;

		case 'n':	/* print nicknames */
			(void)printf("%s: nicknames are:\n", myname);
			for (np = niknames; np->nn; np++) {
				(void)printf("\t%s -> %s\n", np->nn, np->rn);
			}
			out = 0;
			break;

		case 'q':	/* query - see if device is there? */
			query++;
			break;

		case '?':
			(void)printf("usage: %s -dfnq [ name | nickname ]\n",
			    myname);
			(void)printf("\toptions: -d  show default device\n");
			(void)printf(
			    "\t\t -f  force eject even if device is mounted\n");
			(void)printf("\t\t -n  show nicknames\n");
			(void)printf("\t\t -q  query for media present\n");
			out = 2;
		}
	}
	if (out != -1)
		exit(out);	/* 0 = ok (for -d, -n); 2 = error - bad flag */
		/* NOTREACHED */

	if (optind == argc)
		fname = default_dev;
	else
		fname = argv[optind];

	/* go to /dev so short path names work smoothly */
	(void)chdir("/dev");

	st.st_mode = S_IFCHR;	/* in case open fails */
	if (((fd = open(fname, O_RDONLY|O_NDELAY)) == -1) ||
	    (fstat(fd, &st) == -1) ||
	    ((st.st_mode & S_IFMT) != S_IFCHR)) {
		/*
		 * we can't open the named -OR- it isn't a character device
		 * (upon which ioctls are possible), check for nicknames
		 * like "fd", "sf", ... before giving up.
		 * block devices handled by nicknames lookup
		 */
		if ((errno == ENOENT) || ((st.st_mode & S_IFMT) != S_IFCHR)) {
			/* look thru nicknames */
			for (np = niknames; np->nn; np++) {
				if (strcmp(fname, np->nn) == 0)
					break;
			}
			/* if we had found one, try to open it */
			if (np->nn) {
				if ((fd=open(np->rn,
					O_RDONLY|O_NDELAY)) != -1) {
					opened = np->rn;
					goto got_one;
				} /* else */
				(void)fprintf(stderr,
				    "%s: Open fail on %s -> %s: ",
				    myname, fname, np->rn);
				perror("");
				exit(2);
			} else {
				(void)fprintf(stderr,
			    "%s: Open fail on %s, and no nickname found: ",
				    myname, fname);
				perror("");
				exit(2);
			}
		}
/*XXX*/
		/* give up! */
		(void)fprintf(stderr, "%s: Open fail on %s: ", myname, fname);
		perror("");
		exit(2);
	} else {
		/* printf("Opening file...%s\n", fname); */
		opened = fname;
	}

got_one:
	/* XXX query overrides eject */
	if (query) {
		int	chgd;

		switch (whatisthisdevice(fd, opened)) {
		case IT_IS_FLOPPY:

			if (ioctl(fd, FDKGETCHANGE, &chgd) != 0) {
				fprintf(stderr,
					"cannot do FDKGETCHANGE on %s\n",
					opened);
				perror(opened);
				exit(3);
			}
			/*
			 * if disk changed is now clear, a diskette is in
			 * the drive.
			 */
			if ((chgd & FDKGC_CURRENT) == 0) {
				fprintf(stdout, "diskette in drive\n");
				exit(0);
			} else {
				fprintf(stdout, "diskette NOT in drive\n");
				exit(1);
			}

			/*
			 * if is is CDROM, the caddy is in. You cannot open
			 * the device if there is no media in the drive.
			 * Therefore always true.
			 */
		case IT_IS_CDROM:
			fprintf(stdout, "CD-ROM is in drive\n");
			exit(0);

		case IT_IS_UNKNOWN:
			fprintf(stderr, "query not supported on %s\n", opened);
			exit(3);
		}
	}

	/*
	 * XXX - Check to see if this device contains a mounted filesystem.
	 * This is kind of hacky, as it depends on conventional device names.
	 * Try to unmount any mounted partitions.).
	 */
	if (try_unmount(opened) == STILL_MOUNTED) {
		if (force) {
			(void)fprintf(stderr,
			    "Warning: device \"%s\" has mounted partitions.\n",
			    fname);
		} else {
			(void)fprintf(stderr, "%s: Eject of \"%s\" failed: ",
			    myname, fname);
			perror("");
			exit(1);
		}
	}

	/* FUTURE: have to find out what kind of thing this is */
	/* FUTURE: for floppy, try a FDKIOCGCHAR ioctl to see if floppy */

/*XXX* assume floppy, should be switch (whatisthisdevice(fd)) { *XXX*/
	err = ioctl(fd, FDKEJECT);

	if (err == -1) {
		(void)fprintf(stderr, "%s: Eject of \"%s\" failed: ",
		    myname, fname);
		perror("");
		exit(3);
	}
	exit(0);
}

/*
 * Attempt to discern whether the device to be ejected has any mounted
 * partitions. Be lenient by design: if any part of the test fails,
 * allow the eject. Hopefully even if many things go wrong, you won't
 * have to resort to the paper clip.
 */
int
try_unmount(rawd)
	char *rawd;
{
	char buf[MAXPATHLEN], *cp;
	struct stat st;
	dev_t bdevno;
	FILE *fp;
	struct mntent *mnt;

	/*
	 * Get the block device corresponding to the raw device opened,
	 * and find its device number.
	 */
	if (stat(rawd, &st) != 0) {
		(void)fprintf(stderr, "%s: Couldn't stat \"%s\": ",
		    myname, rawd);
		perror("");
		return (UNMOUNTED);
	}
	/*
	 * If this is a raw device, we have to build the block device name.
	 */
	if ((st.st_mode & S_IFMT) == S_IFCHR) {
		cp = strchr(rawd, 'r');
		cp = (cp == NULL) ? NULL : ++cp;
		(void)sprintf(buf, "/dev/%s", cp);
		if (stat(buf, &st) != 0) {
			(void)fprintf(stderr, "%s: Couldn't stat \"%s\": ",
			    myname, buf);
			perror("");
			return (UNMOUNTED);
		}
	}
	if ((st.st_mode & S_IFMT) != S_IFBLK) {
		/*
		 * What is this thing? Ferget it!
		 */
		return (UNMOUNTED);
	}
	bdevno = st.st_rdev & (dev_t)(~0x07);	/* Mask out partition. */

	/*
	 * Now go through the mtab, looking at all 4.2 filesystems.
	 * Compare the device mounted with our bdevno.
	 */
	if ((fp = setmntent(MOUNTED, "r")) == NULL) {
		(void)fprintf(stderr, "Couldn't open \"%s\"\n", MOUNTED);
		return (UNMOUNTED);
	}
	while ((mnt = getmntent(fp)) != NULL) {
		/* avoid obvious excess stat(2)'s */
		if (strcmp(mnt->mnt_type, MNTTYPE_NFS) == 0) {
			continue;
		}
		if (stat(mnt->mnt_fsname, &st) != 0) {
			continue;
		}
		if (((st.st_mode & S_IFMT) == S_IFBLK) &&
		    ((st.st_rdev & (dev_t)(~0x07)) == bdevno)) {
			/*
			 * First use the unmount syscall, since it
			 * returns reliable status. Umount(8) returns
			 * error status 0 even if the unmount failed.
			 */
			if (unmount(mnt->mnt_dir) < 0) {
				(void)endmntent(fp);
				return (STILL_MOUNTED);
			}
			/*
			 * Now call umount. It's either this or clean
			 * up the mtab ourselves.
			 */
			(void)sprintf(buf, "umount -v %s >/dev/null 2>&1",
			    mnt->mnt_dir);
			if (system(buf)) {
				(void)endmntent(fp);
				return (STILL_MOUNTED);
			}
		}
	}
	(void)endmntent(fp);
	return (UNMOUNTED);
}

/*ARGSUSED*/
whatisthisdevice(fd, opened)
int	fd;
char	*opened;
{
	struct niknames	*np;

	for (np = niknames; np->nn; np++) {
		if (strcmp(opened, np->rn) == 0) {
			return (np->dt);
		}
	}
	return (IT_IS_UNKNOWN);
}
