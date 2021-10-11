/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char sccsid[] = "@(#)cp.c 1.1 92/07/30 SMI"; /* from UCB 4.13 10/11/85 */
#endif not lint

/*
 * cp
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/time.h>
#include <sys/mman.h>

#ifndef MAXMAPSIZE
#define	MAXMAPSIZE	(256*1024)
#endif

int	iflag;
int	rflag;
int	pflag;
int	zflag;
char	*rindex();
caddr_t	mmap();

main(argc, argv)
	int argc;
	char **argv;
{
	struct stat stb;
	int rc, i;

	argc--, argv++;
	while (argc > 0 && **argv == '-') {
		(*argv)++;
		while (**argv) switch (*(*argv)++) {

		case 'i':
			iflag++; break;

		case 'R':
		case 'r':
			rflag++; break;

		case 'p':	/* preserve mtimes, atimes, and modes */
			pflag++;
			(void) umask(0);
			break;

		default:
			goto usage;
		}
		argc--; argv++;
	}
	if (argc < 2) 
		goto usage;
	if (argc > 2) {
		if (stat(argv[argc-1], &stb) < 0)
			goto usage;
		if ((stb.st_mode&S_IFMT) != S_IFDIR) 
			goto usage;
	}
	rc = 0;
	for (i = 0; i < argc-1; i++)
		rc |= copy(argv[i], argv[argc-1]);
	exit(rc);
usage:
	(void) fprintf(stderr,
	    "Usage: cp [-ip] f1 f2; or: cp [-ipr] f1 ... fn d2\n");
	exit(1);
	/* NOTREACHED */
}

copy(from, to)
	char *from, *to;
{
	int fold, fnew, n, exists;
	char *last, destname[MAXPATHLEN + 1], buf[MAXBSIZE];
	struct stat stfrom, stto;
	register caddr_t cp;
	int mapsize, munmapsize;
	register off_t filesize;
	register off_t offset;

	fold = open(from, 0);
	if (fold < 0) {
		Perror(from);
		return (1);
	}
	if (fstat(fold, &stfrom) < 0) {
		Perror(from);
		(void) close(fold);
		return (1);
	}
	if (stat(to, &stto) >= 0 &&
	   (stto.st_mode&S_IFMT) == S_IFDIR) {
		last = rindex(from, '/');
		if (last) last++; else last = from;
		if (strlen(to) + strlen(last) >= sizeof destname - 1) {
			(void) fprintf(stderr, "cp: %s/%s: Name too long\n",
			    to, last);
			(void) close(fold);
			return(1);
		}
		(void) sprintf(destname, "%s/%s", to, last);
		to = destname;
	}
	if (rflag && (stfrom.st_mode&S_IFMT) == S_IFDIR) {
		int fixmode = 0;	/* cleanup mode after rcopy */

		(void) close(fold);
		if (stat(to, &stto) < 0) {
			if (mkdir(to,
			    ((int)stfrom.st_mode & 07777) | 0700) < 0) {
				Perror(to);
				return (1);
			}
			fixmode = 1;
		} else if ((stto.st_mode&S_IFMT) != S_IFDIR) {
			(void) fprintf(stderr, "cp: %s: Not a directory.\n",
			    to);
			return (1);
		} else if (pflag)
			fixmode = 1;
		n = rcopy(from, to);
		if (fixmode)
			(void) chmod(to, (int)stfrom.st_mode & 07777);
		return (n);
	}

	if ((stfrom.st_mode&S_IFMT) == S_IFDIR) {
		(void) close(fold);
		(void) fprintf(stderr, "cp: %s: Is a directory (not copied).\n",
		    from);
		return (1);
	}

	exists = stat(to, &stto) == 0;
	if (exists) {
		if (stfrom.st_dev == stto.st_dev &&
		   stfrom.st_ino == stto.st_ino) {
			(void) fprintf(stderr,
				"cp: %s and %s are identical (not copied).\n",
					from, to);
			(void) close(fold);
			return (1);
		}
		if (iflag && isatty(fileno(stdin))) {
			int i, c;

			(void) fprintf (stderr, "overwrite %s? ", to);
			i = c = getchar();
			while (c != '\n' && c != EOF)
				c = getchar();
			if (i != 'y') {
				(void) close(fold);
				return(1);
			}
		}
	}
	fnew = creat(to, (int)stfrom.st_mode & 07777);
	if (fnew < 0) {
		Perror(to);
		(void) close(fold); return(1);
	}
	if (exists && pflag)
		(void) fchmod(fnew, (int)stfrom.st_mode & 07777);

	zopen(fnew, zflag);

	if ((stfrom.st_mode & S_IFMT) == S_IFREG) {
		/*
		 * Determine size of initial mapping.  This will determine
		 * the size of the address space chunk we work with.  This
		 * initial mapping size will be used to perform munmap() in
		 * the future.
		 */
		mapsize = MAXMAPSIZE;
		if (stfrom.st_size < mapsize)
			mapsize = stfrom.st_size;
		munmapsize = mapsize;

		/*
		 * Mmap time!
		 */
		cp = mmap((caddr_t)NULL, mapsize, PROT_READ, MAP_SHARED, fold,
		    (off_t)0);
		if (cp == (caddr_t)-1)
			mapsize = 0;	/* I guess we can't mmap today */
	} else
		mapsize = 0;		/* can't mmap non-regular files */

	if (mapsize != 0) {
		offset = 0;
		filesize = stfrom.st_size;
#ifdef MC_ADVISE
		(void) madvise(cp, mapsize, MADV_SEQUENTIAL);
#endif
		for (;;) {
			if (zwrite(fnew, cp, mapsize) < 0) {
				Perror(to);
				(void) close(fold);
				(void) close(fnew);
				(void) munmap(cp, munmapsize);
				return (1);
			}
			filesize -= mapsize;
			if (filesize == 0)
				break;
			offset += mapsize;
			if (filesize < mapsize)
				mapsize = filesize;
			if (mmap(cp, mapsize, PROT_READ, MAP_SHARED | MAP_FIXED,
			    fold, offset) == (caddr_t)-1) {
				Perror(from);
				(void) close(fold);
				(void) close(fnew);
				(void) munmap(cp, munmapsize);
				return (1);
			}
#ifdef MC_ADVISE
			(void) madvise(cp, mapsize, MADV_SEQUENTIAL);
#endif
		}
		(void) munmap(cp, munmapsize);
	} else {
		for (;;) {
			n = read(fold, buf, sizeof buf);
			if (n == 0)
				break;
			if (n < 0) {
				Perror(from);
				(void) close(fold);
				(void) close(fnew);
				return (1);
			}
			if (zwrite(fnew, buf, n) < 0) {
				Perror(to);
				(void) close(fold);
				(void) close(fnew);
				return (1);
			}
		}
	}
	(void) close(fold);
	if (zclose(fnew) < 0) {
		Perror(to);
		(void) close(fnew);
		return (1);
	}
	if (close(fnew) < 0) {
		Perror(to);
		return (1);
	}
	if (pflag)
		return (setimes(to, &stfrom));
	return (0);
}

rcopy(from, to)
	char *from, *to;
{
	DIR *fold = opendir(from);
	struct direct *dp;
	struct stat statb;
	int errs = 0;
	char fromname[MAXPATHLEN + 1];

	if (fold == 0 || (pflag && fstat(fold->dd_fd, &statb) < 0)) {
		Perror(from);
		return (1);
	}
	for (;;) {
		dp = readdir(fold);
		if (dp == 0) {
			(void) closedir(fold);
			if (pflag)
				return (setimes(to, &statb) + errs);
			return (errs);
		}
		if (dp->d_ino == 0)
			continue;
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (strlen(from)+1+strlen(dp->d_name) >= sizeof fromname - 1) {
			(void) fprintf(stderr, "cp: %s/%s: Name too long\n",
			    from, dp->d_name);
			errs++;
			continue;
		}
		(void) sprintf(fromname, "%s/%s", from, dp->d_name);
		errs += copy(fromname, to);
	}
}

int
setimes(path, statp)
	char *path;
	struct stat *statp;
{
	struct timeval tv[2];
	
	tv[0].tv_sec = statp->st_atime;
	tv[1].tv_sec = statp->st_mtime;
	tv[0].tv_usec = tv[1].tv_usec = 0;
	if (utimes(path, tv) < 0) {
		Perror(path);
		return (1);
	}
	return (0);
}

Perror(s)
	char *s;
{

	(void) fprintf(stderr, "cp: ");
	perror(s);
}


/*
 * sparse file support
 */

#include <errno.h>
#include <sys/file.h>

static int zbsize;
static int zlastseek;
off_t lseek();

/* is it ok to try to create holes? */
zopen(fd, flag)
	int fd;
{
	struct stat st;

	zbsize = 0;
	zlastseek = 0;

	if (flag &&
		fstat(fd, &st) == 0 &&
		(st.st_mode & S_IFMT) == S_IFREG)
		zbsize = st.st_blksize;
}

/* write and/or seek */
zwrite(fd, buf, nbytes)
	int fd;
	register char *buf;
	register int nbytes;
{
	register int block = zbsize ? zbsize : nbytes;

	do {
		if (block > nbytes)
			block = nbytes;
		nbytes -= block;

		if (!zbsize || notzero(buf, block)) {
			register int n, count = block;

			do {
				if ((n = write(fd, buf, count)) < 0)
					return -1;
				buf += n;
			} while ((count -= n) > 0);
			zlastseek = 0;
		}
		else {
			if (lseek(fd, (off_t) block, L_INCR) < 0)
				return -1;
			buf += block;
			zlastseek = 1;
		}
	} while (nbytes > 0);

	return 0;
}

/* write last byte of file if necessary */
zclose(fd)
	int fd;
{
	zbsize = 0;

	if (zlastseek &&
		(lseek(fd, (off_t) -1, L_INCR) < 0 ||
		zwrite("", 1) < 0))
		return -1;
 	else
		return 0;
}

/* return true if buffer is not all zeros */
notzero(p, n)
	register char *p;
	register int n;
{
	register int result = 0;

	while ((int) p & 3 && --n >= 0)
		result |= *p++;

	while ((n -= 4 * sizeof (int)) >= 0) {
		result |= ((int *) p)[0];
		result |= ((int *) p)[1];
		result |= ((int *) p)[2];
		result |= ((int *) p)[3];
		if (result)
			return result;
		p += 4 * sizeof (int);
	}
	n += 4 * sizeof (int);

	while (--n >= 0)
		result |= *p++;

	return result;	
}
