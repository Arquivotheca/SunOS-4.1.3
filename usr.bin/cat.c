/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)cat.c 1.1 92/07/30 SMI"; /* from UCB 5.2 12/6/85 */
#endif not lint

/*
 * Concatenate files.
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/file.h>

/* pre-4.1 compatibility */
#ifdef nl_printf
#include <locale.h>
#endif

#ifndef MAXMAPSIZE
#define	MAXMAPSIZE	(256*1024)
#endif

#define	ofd		fileno(stdout)

int	bflg, eflg, nflg, sflg, tflg, uflg, vflg, zflg;
int	spaced, lno, inline, ibsize, obsize;

off_t lseek();

main(argc, argv)
char **argv;
{
	int fflg = 0;
	register FILE *fi;
	char *filenm;
	register c;
	int dev, ino = -1;
	struct stat statb;
	int retval = 0;

#ifdef LC_ALL
	(void) setlocale(LC_ALL, "");
#endif
	lno = 1;
	for( ; argc>1 && argv[1][0]=='-'; argc--,argv++) {
		switch(argv[1][1]) {
		case 0:
			break;
		case 'u':
			setbuf(stdout, (char *)NULL);
			uflg++;
			continue;
		case 'n':
			nflg++;
			continue;
		case 'b':
			bflg++;
			nflg++;
			continue;
		case 'v':
			vflg++;
			continue;
		case 's':
			sflg++;
			continue;
		case 'e':
			eflg++;
			vflg++;
			continue;
		case 't':
			tflg++;
			vflg++;
			continue;
		}
		break;
	}
	if (fstat(ofd, &statb) == 0) {
		statb.st_mode &= S_IFMT;
		if (statb.st_mode!=S_IFCHR && statb.st_mode!=S_IFBLK) {
			dev = statb.st_dev;
			ino = statb.st_ino;
		}
		obsize = statb.st_blksize;
	}
	else
		obsize = 0;
	if (obsize == 0 || statb.st_mode != S_IFREG)
		zflg = 0;
	if (argc < 2) {
		argc = 2;
		fflg++;
	}
	while (--argc > 0) {
		if (fflg || (*++argv)[0]=='-' && (*argv)[1]=='\0') {
			filenm = "standard input";
			fi = stdin;
		} else {
			filenm = *argv;
			if ((fi = fopen(filenm, "r")) == NULL) {
				fprintf(stderr, "cat: ");
				perror(filenm);
				retval = 1;
				continue;
			}
		}
		if (fstat(fileno(fi), &statb) == 0) {
			if ((statb.st_mode & S_IFMT) == S_IFREG &&
			    statb.st_dev==dev && statb.st_ino==ino) {
				fprintf(stderr, "cat: input %s is output\n",
				   fflg?"-": filenm);
				fclose(fi);
				retval = 1;
				continue;
			}
			ibsize = statb.st_blksize;
		}
		else
			ibsize = 0;
		if (nflg||sflg||vflg) {
			if (copyopt(fi, filenm))
				retval = 1;
		} else if (uflg) {
			while ((c = getc(fi)) != EOF)
				putchar(c);
			if (ferror(fi))
				retval = readerr(filenm);
		} else {
			/* no flags specified */
			if (fastcat(fileno(fi), &statb, filenm))
				retval = 1;
		}
		if (fi!=stdin)
			fclose(fi);
		else
			clearerr(fi);		/* reset sticky eof */
		if (ferror(stdout)) {
			fprintf(stderr, "cat: output write error\n");
			retval = 1;
			break;
		}
	}
	if (zflg)
		zclose();
	exit(retval);

	/* NOTREACHED */
}

copyopt(f, filenm)
	register FILE *f;
	char *filenm;
{
	register int c;

top:
	c = getc(f);
	if (c == EOF)
		return ferror(f) ? readerr(filenm) : 0;
	if (c == '\n') {
		if (inline == 0) {
			if (sflg && spaced)
				goto top;
			spaced = 1;
		}
		if (nflg && bflg==0 && inline == 0)
			printf("%6d\t", lno++);
		if (eflg)
			putchar('$');
		putchar('\n');
		inline = 0;
		goto top;
	}
	if (nflg && inline == 0)
		printf("%6d\t", lno++);
	inline = 1;
	if (vflg) {
		if (tflg==0 && c == '\t')
			putchar(c);
		else {
			if (c >= 0200 && !isprint(c) && !iscntrl(c)) {
				printf("M-");
				c &= 0177;
			}
			if (isprint(c))
				putchar(c);
			else if (c < ' ')
				printf("^%c", c+'@');
			else if (c == 0177)
				printf("^?");
			else
				putchar(c);
		}
	} else
		putchar(c);
	spaced = 0;
	goto top;
}

fastcat(fd, statp, filenm)
register int fd;
register struct stat *statp;
char *filenm;
{
	register int	buffsize, n;
	register char	*buff;
	int		mapsize, munmapsize;
	register off_t	filesize;
	register off_t	mapoffset;
	char		*mmap();
	char		*malloc();

	if ((statp->st_mode & S_IFMT) == S_IFREG) {

		/* Check file is positioned at the beginning */
		if (lseek(fd, (off_t) 0, L_INCR) != 0) {
			mapsize = 0;
			goto doit;
		}
		mapsize = MAXMAPSIZE;
		if (statp->st_size < mapsize)
			mapsize = statp->st_size;
		munmapsize = mapsize;

		/*
		 * Mmap time!
		 */
		buff = mmap((caddr_t)NULL, mapsize, PROT_READ, MAP_SHARED, fd,
		    (off_t)0);
		if (buff == (caddr_t)-1)
			mapsize = 0;	/* I guess we can't mmap today */
	} else
		mapsize = 0;		/* can't mmap non-regular files */

doit:

	if (mapsize != 0) {
#ifdef MC_ADVISE
		(void) madvise(buff, mapsize, MADV_SEQUENTIAL);
#endif
		mapoffset = 0;
		filesize = statp->st_size;
		for (;;) {
			zwrite(buff, mapsize);
			filesize -= mapsize;
			if (filesize == 0)
				break;
			mapoffset += mapsize;
			if (filesize < mapsize)
				mapsize = filesize;
			if (mmap(buff, mapsize, PROT_READ, MAP_SHARED|MAP_FIXED,
			    fd, mapoffset) == (caddr_t)-1) {
				perror("cat: mmap error");
				(void) munmap(buff, munmapsize);
				return (1);
			}
#ifdef MC_ADVISE
			(void) madvise(buff, mapsize, MADV_SEQUENTIAL);
#endif
		}
		(void) munmap(buff, munmapsize);
	} else {
		if (obsize)	/* common case, use output blksize */
			buffsize = obsize;
		else if (ibsize)
			buffsize = ibsize;
		else
			buffsize = BUFSIZ;

		if ((buff = malloc((unsigned) buffsize)) == NULL) {
			perror("cat: no memory");
			return (1);
		}

		while (1) {
			n = read(fd, buff, buffsize);

			if (n > 0)
				zwrite(buff, n);
			else if (n == 0 || errno != EWOULDBLOCK)
				break;
		}

		free(buff);
		if (n < 0)
 			return readerr(filenm);
	}
	return (0);
}

readerr(filenm)
	char *filenm;
{
	fprintf(stderr, "cat: read error on ");
	perror(filenm);
	return 1;
}

/*
 * sparse file support
 */

#include <sys/file.h>

static int zlastseek;

/* write and/or seek */
zwrite(buf, nbytes)
	register char *buf;
	register int nbytes;
{
	register int block = zflg ? obsize : nbytes;

	do {
		if (block > nbytes)
			block = nbytes;
		nbytes -= block;

		if (!zflg || notzero(buf, block)) {
			register int n, count = block;

			do {
				if ((n = write(ofd, buf, count)) < 0) {
					if (errno == EWOULDBLOCK) {
						n = 0;
						continue;
					}
					perror("cat: write error");
					exit(2);
				}
				buf += n;
			} while ((count -= n) > 0);
			zlastseek = 0;
		}
		else {
			if (lseek(ofd, (off_t) block, L_INCR) < 0) {
				perror("cat: lseek error");
				exit(2);
			}
			buf += block;
			zlastseek = 1;
		}
	} while (nbytes > 0);

	return;
}

/* write last byte of file if necessary */
zclose()
{
	if (zlastseek) {
		if (lseek(ofd, (off_t) -1, L_INCR) < 0) {
			perror("cat: lseek error");
			exit(2);
		}
		zflg = 0;
		zwrite("", 1);
	}
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
