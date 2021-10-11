/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1981 Regents of the University of California */

#ifndef lint
static  char *sccsid = "@(#)exrecover.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.23 */
#endif

#include <stdio.h>	/* BUFSIZ: stdio = 512, VMUNIX = 1024 */
#ifndef TRACE
#undef	BUFSIZ		/* BUFSIZ different */
#endif

#include "ex.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "uparm.h"
#include <pwd.h>
#include <dirent.h>

char xstr[1];		/* make loader happy */
short tfile = -1;	/* ditto */

/*
 *
 * This program searches through the specified directory and then
 * the directory usrpath(preserve) looking for an instance of the specified
 * file from a crashed editor or a crashed system.
 * If this file is found, it is unscrambled and written to
 * the standard output.
 *
 * If this program terminates without a "broken pipe" diagnostic
 * (i.e. the editor doesn't die right away) then the buffer we are
 * writing from is removed when we finish.  This is potentially a mistake
 * as there is not enough handshaking to guarantee that the file has actually
 * been recovered, but should suffice for most cases.
 */

/*
 * For lint's sake...
 */
#ifndef lint
#define	ignorl(a)	a
#endif

/*
 * This directory definition also appears (obviously) in expreserve.c.
 * Change both if you change either.
 */
char	mydir[256];
struct	passwd *getpwuid();

/*
 * Limit on the number of printed entries
 * when an, e.g. ``ex -r'' command is given.
 */
#define	NENTRY	50

extern char *ctime();
extern void setbuf();
char nb[BUFSIZ];
int vercnt;			/* Count number of versions of file found */
main(argc, argv)
	int argc;
	char *argv[];
{
	register char *cp;
	register int c, b, i;
	register int rflg = 0, errflg = 0;
	extern int optind;
	struct passwd *pp = getpwuid(getuid());

	strcpy(mydir, usrpath(preserve/));
	if (pp == NULL) {
		fprintf(stderr, "Unable to get user's id\n");
		exit(-1);
	}
	strcat(mydir, pp->pw_name);

	/*
	 * Initialize as though the editor had just started.
	 */
	fendcore = (line *) sbrk(0);
	dot = zero = dol = fendcore;
	one = zero + 1;
	endcore = fendcore - 2;
	iblock = oblock = -1;

	while ((c=getopt(argc, argv, "rx")) != EOF)
		switch (c) {
			case 'r':
				rflg++;
				break;
		

			case '?':
				errflg++;
				break;
		}
	argc -= optind;
	argv = &argv[optind];
	
	if (errflg) 
		exit(2);
	
	/*
	 * If given only a -r argument, then list the saved files.
	 * (NOTE: single -r argument is scheduled to be replaced by -L).
	 */
	if (rflg && argc == 0) {
		fprintf(stderr,"%s:\n", mydir);
		listfiles(mydir);
		fprintf(stderr,"%s:\n", TMPDIR);
		listfiles(TMPDIR);
		exit(0);
	}
	
	if (argc != 2)
		error(" Wrong number of arguments to exrecover", 0);

	CP(file, argv[1]);

	/*
	 * Search for this file.
	 */
	findtmp(argv[0]);

	/*
	 * Got (one of the versions of) it, write it back to the editor.
	 */
	cp = ctime(&H.Time);
	cp[19] = 0;
	fprintf(stderr, " [Dated: %s", cp);
	fprintf(stderr, vercnt > 1 ? ", newest of %d saved]" : "]", vercnt);
	
	fprintf(stderr, "\r\n [Hit return to continue]");
	fflush(stderr);
	setbuf(stdin, (char *)NULL);
	while ((c = getchar()) != '\n' && c != '\r')
		;
	H.Flines++;

	/*
	 * Allocate space for the line pointers from the temp file.
	 */
	if ((int) sbrk((int) (H.Flines * sizeof (line))) == -1)
		error(" Not enough core for lines", 0);
#ifdef DEBUG
	fprintf(stderr, "%d lines\n", H.Flines);
#endif

	/*
	 * Now go get the blocks of seek pointers which are scattered
	 * throughout the temp file, reconstructing the incore
	 * line pointers at point of crash.
	 */
	b = 0;
	while (H.Flines > 0) {
		ignorl(lseek(tfile, (long) blocks[b] * BUFSIZ, 0));
		i = H.Flines < BUFSIZ / sizeof (line) ?
			H.Flines * sizeof (line) : BUFSIZ;
		if (read(tfile, (char *) dot, i) != i)
			syserror();
		dot += i / sizeof (line);
		H.Flines -= i / sizeof (line);
		b++;
	}
	dot--; dol = dot;

	/*
	 * Due to sandbagging some lines may really not be there.
	 * Find and discard such.  This shouldn't happen often.
	 */
	scrapbad();


	/*
	 * Now if there were any lines in the recovered file
	 * write them to the standard output.
	 */
	if (dol > zero) {
		addr1 = one; addr2 = dol; io = 1;
		putfile();
	}
	/*
	 * Trash the saved buffer.
	 * Hopefully the system won't crash before the editor
	 * syncs the new recovered buffer; i.e. for an instant here
	 * you may lose if the system crashes because this file
	 * is gone, but the editor hasn't completed reading the recovered
	 * file from the pipe from us to it.
	 *
	 * This doesn't work if we are coming from an non-absolute path
	 * name since we may have chdir'ed but what the hay, noone really
	 * ever edits with temporaries in "." anyways.
	 */
	if (nb[0] == '/') {
		ignore(unlink(nb));
		(void) rmdir(mydir);
	}
	exit(0);
}

/*
 * Print an error message (notably not in error
 * message file).  If terminal is in RAW mode, then
 * we should be writing output for "vi", so don't print
 * a newline which would mess up the screen.
 */
/*VARARGS2*/
error(str, inf)
	char *str;
	int inf;
{
	register int c;

	if (inf)
		fprintf(stderr, str, inf);
	else
		fprintf(stderr, str);
	
#ifndef USG
	gtty(2, &tty);
	if ((tty.sg_flags & RAW) == 0)
#else
	ioctl(2, TCGETA, &tty);
	if (tty.c_lflag & ICANON)
#endif
		fprintf(stderr, "\n");
	fprintf(stderr,"\r\n [Hit return to continue]");
	fflush(stderr);
	setbuf(stdin, (char *)NULL);
	while((c = getchar()) != '\n' && c != '\r');
	exit(1);
}

/*
 * Here we save the information about files, when
 * you ask us what files we have saved for you.
 * We buffer file name, number of lines, and the time
 * at which the file was saved.
 */
struct svfile {
	char	sf_name[FNSIZE + 1];
	int	sf_lines;
	char	sf_entry[MAXNAMLEN + 1];
	time_t	sf_time;
	short	sf_encrypted;
};

listfiles(dirname)
	char *dirname;
{
	register DIR *dir;
	struct dirent *direntry;
	int ecount, qucmp();
	register int f;
	char *cp;
	struct svfile *fp, svbuf[NENTRY];

	/*
	 * Open usrpath(preserve), and go there to make things quick.
	 */
	if ((dir = opendir(dirname)) == NULL) {
		if (errno == ENOENT)
			fprintf(stderr,"No files saved.\n");
		else
			perror(dirname);
		return;
	}
	if (chdir(dirname) < 0) {
		perror(dirname);
		return;
	}

	/*
	 * Look at the candidate files in usrpath(preserve).
	 */
	fp = &svbuf[0];
	ecount = 0;
	while ((direntry = readdir(dir)) != NULL) {
		if (direntry->d_name[0] != 'E')
			continue;
#ifdef DEBUG
		fprintf(stderr, "considering %s\n", direntry->d_name);
#endif
		/*
		 * Name begins with E; open it and
		 * make sure the uid in the header is our uid.
		 * If not, then don't bother with this file, it can't
		 * be ours.
		 */
		f = open(direntry->d_name, 0);
		if (f < 0) {
#ifdef DEBUG
			fprintf(stderr, "open failed\n");
#endif
			continue;
		}
		if (read(f, (char *) &H, sizeof H) != sizeof H) {
#ifdef DEBUG
			fprintf(stderr, "could not read header\n");
#endif
			ignore(close(f));
			continue;
		}
		ignore(close(f));
		if (getuid() != H.Uid) {
#ifdef DEBUG
			fprintf(stderr, "uid wrong\n");
#endif
			continue;
		}

		/*
		 * Saved the day!
		 */
		enter(fp++, direntry->d_name, ecount);
		ecount++;
#ifdef DEBUG
		fprintf(stderr, "entered file %s\n", direntry->d_name);
#endif
	}
	ignore(closedir(dir));
	/*
	 * If any files were saved, then sort them and print
	 * them out.
	 */
	if (ecount == 0) {
		fprintf(stderr, "No files saved.\n");
		return;
	}
	qsort(&svbuf[0], ecount, sizeof svbuf[0], qucmp);
	for (fp = &svbuf[0]; fp < &svbuf[ecount]; fp++) {
		cp = ctime(&fp->sf_time);
		cp[10] = 0;
		fprintf(stderr, "On %s at ", cp);
 		cp[16] = 0;
		fprintf(stderr, &cp[11]);
		fprintf(stderr, " saved %d lines of file \"%s\" ",
		    fp->sf_lines, fp->sf_name);
		fprintf(stderr, "%s\n",(fp->sf_encrypted) ? "[ENCRYPTED]" : "");
	}
}

/*
 * Enter a new file into the saved file information.
 */
enter(fp, fname, count)
	struct svfile *fp;
	char *fname;
{
	register char *cp, *cp2;
	register struct svfile *f, *fl;
	time_t curtime, itol();

	f = 0;
	if (count >= NENTRY) {
	        /*
		 * Trash the oldest as the most useless.
		 */
		fl = fp - count + NENTRY - 1;
		curtime = fl->sf_time;
		for (f = fl; --f > fp-count; )
			if (f->sf_time < curtime)
				curtime = f->sf_time;
		for (f = fl; --f > fp-count; )
			if (f->sf_time == curtime)
				break;
		fp = f;
	}

	/*
	 * Gotcha.
	 */
	fp->sf_time = H.Time;
	fp->sf_lines = H.Flines;
	fp->sf_encrypted = H.encrypted;
	for (cp2 = fp->sf_name, cp = savedfile; *cp;)
		*cp2++ = *cp++;
	*cp2++ = 0;
	for (cp2 = fp->sf_entry, cp = fname; *cp && cp-fname < 14;)
		*cp2++ = *cp++;
	*cp2++ = 0;
}

/*
 * Do the qsort compare to sort the entries first by file name,
 * then by modify time.
 */
qucmp(p1, p2)
	struct svfile *p1, *p2;
{
	register int t;

	if (t = strcmp(p1->sf_name, p2->sf_name))
		return(t);
	if (p1->sf_time > p2->sf_time)
		return(-1);
	return(p1->sf_time < p2->sf_time);
}

/*
 * Scratch for search.
 */
char	bestnb[BUFSIZ];		/* Name of the best one */
long	besttime = 0;		/* Time at which the best file was saved */
int	bestfd;			/* Keep best file open so it dont vanish */

/*
 * Look for a file, both in the users directory option value
 * (i.e. usually /tmp) and in usrpath(preserve).
 * Want to find the newest so we search on and on.
 */
findtmp(dir)
	char *dir;
{
	char firstfname[BUFSIZ];

	/*
	 * No name or file so far.
	 */
	bestnb[0] = 0;
	bestfd = -1;

	/*
	 * Search usrpath(preserve) and, if we can get there, /tmp
	 * (actually the user's "directory" option).
	 */
	searchdir(dir);
	strcpy(firstfname, nb);
	if (chdir(mydir) == 0) {
		searchdir(mydir);
		if (vercnt > 1)
			ignore(unlink(nb));
	}
	if (bestfd != -1) {
		/*
		 * Gotcha.
		 * Put the file (which is already open) in the file
		 * used by the temp file routines, and save its
		 * name for later unlinking.
		 */
		tfile = bestfd;
		CP(nb, bestnb);
		ignorl(lseek(tfile, 0l, 0));

		/*
		 * Gotta be able to read the header or fall through
		 * to lossage.
		 */
		if (read(tfile, (char *) &H, sizeof H) == sizeof H)
			return;
	}

	/*
	 * Extreme lossage...
	 */
	error(" File not found", 0);
}

/*
 * Search for the file in directory dirname.
 *
 * Don't chdir here, because the users directory
 * may be ".", and we would move away before we searched it.
 * Note that we actually chdir elsewhere (because it is too slow
 * to look around in usrpath(preserve) without chdir'ing there) so we
 * can't win, because we don't know the name of '.' and if the path
 * name of the file we want to unlink is relative, rather than absolute
 * we won't be able to find it again.
 */
searchdir(dirname)
	char *dirname;
{
	struct dirent *direntry;
	register DIR *dir;

	if ((dir = opendir(dirname)) == NULL) 
		return;
	while ((direntry = readdir(dir)) != NULL) {
		if (direntry->d_name[0] != 'E' || direntry->d_name[1] != 'x')
			continue;
		/*
		 * Got a file in the directory starting with Ex...
		 * Save a consed up name for the file to unlink
		 * later, and check that this is really a file
		 * we are looking for.
		 */
		ignore(strcat(strcat(strcpy(nb, dirname), "/"), direntry->d_name));
		if (yeah(nb)) {
			/*
			 * Well, it is the file we are looking for.
			 * Is it more recent than any version we found before?
			 */
			if (H.Time > besttime) {
				/*
				 * A winner.
				 */
				ignore(close(bestfd));
				bestfd = dup(tfile);
				besttime = H.Time;
				CP(bestnb, nb);
			}
			/*
			 * Count versions and tell user
			 */
			vercnt++;
		}
		ignore(close(tfile));
	}
	ignore(closedir(dir));
}

/*
 * Given a candidate file to be recovered, see
 * if it's really an editor temporary and of this
 * user and the file specified.
 */
yeah(name)
	char *name;
{

	tfile = open(name, 2);
	if (tfile < 0)
		return (0);
	if (read(tfile, (char *) &H, sizeof H) != sizeof H) {
nope:
		ignore(close(tfile));
		return (0);
	}
	if (!eq(savedfile, file))
		goto nope;
	if (getuid() != H.Uid)
		goto nope;
	/*
	 * Old code: puts a word LOST in the header block, so that lost lines
	 * can be made to point at it.
	 */
	ignorl(lseek(tfile, (long)(BUFSIZ*HBLKS-8), 0));
	ignore(write(tfile, "LOST", 5));
	return (1);
}

/*
 * Find the true end of the scratch file, and ``LOSE''
 * lines which point into thin air.  This lossage occurs
 * due to the sandbagging of i/o which can cause blocks to
 * be written in a non-obvious order, different from the order
 * in which the editor tried to write them.
 *
 * Lines which are lost are replaced with the text LOST so
 * they are easy to find.  We work hard at pretty formatting here
 * as lines tend to be lost in blocks.
 *
 * This only seems to happen on very heavily loaded systems, and
 * not very often.
 */
scrapbad()
{
	register line *ip;
	struct stat stbuf;
	off_t size, maxt;
	int bno, cnt, bad, was;
	char bk[BUFSIZ];

	ignore(fstat(tfile, &stbuf));
	size = stbuf.st_size;
	maxt = (size >> SHFT) | (BNDRY-1);
	bno = (maxt >> OFFBTS) & BLKMSK;
#ifdef DEBUG
	fprintf(stderr, "size %ld, maxt %o, bno %d\n", size, maxt, bno);
#endif

	/*
	 * Look for a null separating two lines in the temp file;
	 * if last line was split across blocks, then it is lost
	 * if the last block is.
	 */
	while (bno > 0) {
		ignorl(lseek(tfile, (long) BUFSIZ * bno, 0));
		cnt= read(tfile, (char *) bk, BUFSIZ);
#ifdef DEBUG
      		fprintf(stderr,"UNENCRYPTED: BLK %d\n",bno);
#endif
		while (cnt > 0)
			if (bk[--cnt] == 0)
				goto null;
		bno--;
	}
null:

	/*
	 * Magically calculate the largest valid pointer in the temp file,
	 * consing it up from the block number and the count.
	 */
	maxt = ((bno << OFFBTS) | (cnt >> SHFT)) & ~1;
#ifdef DEBUG
	fprintf(stderr, "bno %d, cnt %d, maxt %o\n", bno, cnt, maxt);
#endif

	/*
	 * Now cycle through the line pointers,
	 * trashing the Lusers.
	 */
	was = bad = 0;
	for (ip = one; ip <= dol; ip++)
		if (*ip > maxt) {
#ifdef DEBUG
			fprintf(stderr, "%d bad, %o > %o\n", ip - zero, *ip, maxt);
#endif
			if (was == 0)
				was = ip - zero;
			*ip = ((HBLKS*BUFSIZ)-8) >> SHFT;
		} else if (was) {
			if (bad == 0)
				fprintf(stderr, " [Lost line(s):");
			fprintf(stderr, " %d", was);
			if ((ip - 1) - zero > was)
				fprintf(stderr, "-%d", (ip - 1) - zero);
			bad++;
			was = 0;
		}
	if (was != 0) {
		if (bad == 0)
			fprintf(stderr, " [Lost line(s):");
		fprintf(stderr, " %d", was);
		if (dol - zero != was)
			fprintf(stderr, "-%d", dol - zero);
		bad++;
	}
	if (bad)
		fprintf(stderr, "]");
}

/*
 * Aw shucks, if we only had a (void) cast.
 */
#ifdef lint
Ignorl(a)
	long a;
{

	a = a;
}

Ignore(a)
	char *a;
{

	a = a;
}

Ignorf(a)
	int (*a)();
{

	a = a;
}

ignorl(a)
	long a;
{

	a = a;
}
#endif

int	cntch, cntln, cntodd, cntnull;
/*
 * Following routines stolen mercilessly from ex.
 */
putfile()
{
	line *a1;
	register char *fp, *lp;
	register int nib;

	a1 = addr1;
	clrstats();
	cntln = addr2 - a1 + 1;
	if (cntln == 0)
		return;
	nib = BUFSIZ;
	fp = genbuf;
	do {
#ifdef DEBUG
		fprintf(stderr,"GETTING A LINE \n");
#endif
		getline(*a1++);
		lp = linebuf;
#ifdef DEBUG
		fprintf(stderr,"LINE:%s\n",linebuf);
#endif
		for (;;) {
			if (--nib < 0) {
				nib = fp - genbuf;
				if (write(io, genbuf, nib) != nib)
					wrerror();
				cntch += nib;
				nib = 511;
				fp = genbuf;
			}
			if ((*fp++ = *lp++) == 0) {
				fp[-1] = '\n';
				break;
			}
		}
	} while (a1 <= addr2);
	nib = fp - genbuf;
	if (write(io, genbuf, nib) != nib)
		wrerror();
	cntch += nib;
}

wrerror()
{

	syserror();
}

clrstats()
{

	ninbuf = 0;
	cntch = 0;
	cntln = 0;
	cntnull = 0;
	cntodd = 0;
}

#define	READ	0
#define	WRITE	1

getline(tl)
	line tl;
{
	register char *bp, *lp;
	register int nl;

	lp = linebuf;
	bp = getblock(tl);
	nl = nleft;
	tl &= ~OFFMSK;
	while (*lp++ = *bp++)
		if (--nl == 0) {
			bp = getblock(tl += INCRMT);
			nl = nleft;
		}
}

int	read();
int	write();

char *
getblock(atl)
	line atl;
{
	register int bno, off;
	
	bno = (atl >> OFFBTS) & BLKMSK;
#ifdef DEBUG
	fprintf(stderr,"GETBLOCK: BLK %d\n",bno);
#endif
	off = (atl << SHFT) & LBTMSK;
	if (bno >= NMBLKS)
		error(" Tmp file too large");
	nleft = BUFSIZ - off;
	if (bno == iblock) 
		return (ibuff + off);
	iblock = bno;
	blkio(bno, ibuff, read);
#ifdef DEBUG
	fprintf(stderr,"UNENCRYPTED: BLK %d\n",bno);
#endif

	return (ibuff + off);
}

blkio(b, buf, iofcn)
	short b;
	char *buf;
	int (*iofcn)();
{

	int rc;
	lseek(tfile, (long) (unsigned) b * BUFSIZ, 0);
	if ((rc =(*iofcn)(tfile, buf, BUFSIZ)) != BUFSIZ) {
		fprintf(stderr,"Failed on BLK: %d with %d/%d\n",b,rc,BUFSIZ); 
		syserror();
	}
}

syserror()
{
	extern int sys_nerr;
	extern char *sys_errlist[];

	write(2, " ", 1);
	if (errno >= 0 && errno <= sys_nerr)
		error(sys_errlist[errno]);
	else
		error("System error %d", errno);
	exit(1);
}


/*
 * Encryption routines.  These are essentially unmodified from ex.
 */

