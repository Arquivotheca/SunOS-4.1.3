#ifndef lint
static  char sccsid[] = "@(#)fsirand.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * fsirand - Copyright (c) 1984 Sun Microsystems.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/time.h>
#include <ufs/fs.h>
#include <sys/vnode.h>
#include <ufs/inode.h>

extern int	errno;
extern long	lseek();
extern long	random();

char fsbuf[SBSIZE];
struct dinode dibuf[8192/sizeof (struct dinode)];

static char	*strerror(/*int errnum*/);

main(argc, argv)
int	argc;
char	*argv[];
{
	struct fs *fs;
	int fd;
	char *dev;
	int bno;
	struct dinode *dip;
	int inum, imax;
	int n;
	int i;
	off_t seekaddr;
	int bsize;
	int pflag = 0;
	struct timeval timeval;

	argv++;
	argc--;
	if (argc > 0 && strcmp(*argv, "-p") == 0) {
		pflag++;
		argv++;
		argc--;
	}
	if (argc <= 0) {
		(void) fprintf(stderr, "Usage: fsirand [-p] special\n");
		exit(1);
	}
	dev = *argv;
	fd = open(dev, pflag ? 0 : 2);
	if (fd == -1) {
		(void) fprintf(stderr, "fsirand: Cannot open %s: %s\n", dev,
		    strerror(errno));
		exit(1);
	}
	if (lseek(fd, SBLOCK * DEV_BSIZE, 0) == -1) {
		(void) fprintf(stderr,
		    "fsirand: Seek to superblock failed: %s\n",
		    strerror(errno));
		exit(1);
	}
	fs = (struct fs *) fsbuf;
	if ((n = read(fd, (char *) fs, SBSIZE)) != SBSIZE) {
		(void) fprintf(stderr,
		    "fsirand: Read of superblock failed: %s\n",
		    n == -1 ? strerror(errno) : "Short read");
		exit(1);
	}
	if (fs->fs_magic != FS_MAGIC) {
		(void) fprintf(stderr,
		    "fsirand: Not a file system (bad magic number in superblock)\n");
		exit(1);
	}
	if (pflag) {
		(void) printf("fsid: %x %x\n", fs->fs_id[0], fs->fs_id[1]);
	} else {
		n = getpid();
		srandom((int)(timeval.tv_sec + timeval.tv_usec + n));
		while (n--) {
			(void) random();
		}
	}
	bsize = INOPB(fs) * sizeof (struct dinode);
	inum = 0;
	imax = fs->fs_ipg * fs->fs_ncg;
	while (inum < imax) {
		bno = itod(fs, inum);
		seekaddr = fsbtodb(fs, bno) * DEV_BSIZE;
		if (lseek(fd, seekaddr, 0) == -1) {
			(void) fprintf(stderr,
			    "fsirand: Seek to %ld failed: %s\n", seekaddr,
			    strerror(errno));
			exit(1);
		}
		n = read(fd, (char *) dibuf, bsize);
		if (n != bsize) {
			(void) fprintf(stderr,
			    "fsirand: Read of ilist block failed: %s\n",
			    n == -1 ? strerror(errno) : "Short read");
			exit(1);
		}
		for (dip = dibuf; dip < &dibuf[INOPB(fs)]; dip++) {
			if (pflag) {
				(void) printf("ino %d gen %x\n", inum,
				    dip->di_gen);
			} else {
				dip->di_gen = random();
			}
			inum++;
		}
		if (!pflag) {
			if (lseek(fd, seekaddr, 0) == -1) {
				(void) fprintf(stderr,
				    "fsirand: Seek to %ld failed: %s\n",
				    seekaddr, strerror(errno));
				exit(1);
			}
			n = write(fd, (char *) dibuf, bsize);
			if (n != bsize) {
				(void) fprintf(stderr,
				    "fsirand: Write of ilist block failed: %s\n",
				    n == -1 ? strerror(errno) : "Short write");
				exit(1);
			}
		}
	}
	if (!pflag) {
		(void) gettimeofday(&timeval, (struct timezone *)NULL);
		fs->fs_id[0] = timeval.tv_sec;
		fs->fs_id[1] = timeval.tv_usec + getpid();
		if (lseek(fd, SBLOCK * DEV_BSIZE, 0) == -1) {
			(void) fprintf(stderr,
			    "fsirand: Seek to superblock failed: %s\n",
			    strerror(errno));
			exit(1);
		}
		if ((n = write(fd, (char *) fs, SBSIZE)) != SBSIZE) {
			(void) fprintf(stderr,
			    "fsirand: Write of superblock failed: %s\n",
			    n == -1 ? strerror(errno) : "Short write");
			exit(1);
		}
	}
	for (i = 0; i < fs->fs_ncg; i++ ) {
		seekaddr = fsbtodb(fs, cgsblock(fs, i)) * DEV_BSIZE;
		if (lseek(fd,  seekaddr, 0) == -1) {
			(void) fprintf(stderr,
			    "fsirand: Seek to alternate superblock failed: %s\n",
			    strerror(errno));
			exit(1);
		}
		if (pflag) {
			if ((n = read(fd, (char *) fs, SBSIZE)) != SBSIZE) {
				(void) fprintf(stderr,
				    "fsirand: Read of alternate superblock failed: %s\n",
				    n == -1 ? strerror(errno) : "Short read");
				exit(1);
			}
			if (fs->fs_magic != FS_MAGIC) {
				(void) fprintf(stderr,
				    "fsirand: Not a valid file system (bad magic number in alternate superblock)\n");
				exit(1);
			}
		} else {
			if ((n = write(fd, (char *) fs, SBSIZE)) != SBSIZE) {
				(void) fprintf(stderr,
				    "fsirand: Write of alternate superblock failed: %s\n",
				    n == -1 ? strerror(errno) : "Short write");
				exit(1);
			}
		}
	}
	exit(0);
	/* NOTREACHED */
}

static char *
strerror(errnum)
	int errnum;
{
	extern int sys_nerr;
	extern char *sys_errlist[];
	static char unknown_error[16+1];	/* "Error NNNNNNNNNN" + '\0' */

	if (errnum < 0 || errnum > sys_nerr) {
		(void) sprintf(unknown_error, "Error %d", errnum);
		return(unknown_error);
	} else
		return(sys_errlist[errnum]);
}
