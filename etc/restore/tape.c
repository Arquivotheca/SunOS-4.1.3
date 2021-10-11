/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef	lint
static char sccsid[] = "@(#)tape.c 1.1 92/07/30 SMI"; /* from UCB 5.13 2/22/88 */
#endif	not lint

#include "restore.h"
#include <protocols/dumprestore.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sun/dkio.h>
#include <sys/file.h>
#include <setjmp.h>
#include <sys/stat.h>

#define	MAXINO	65535		/* KLUDGE */

#define	MAXTAPES	128

static long	fssize = MAXBSIZE;
static int	mt = -1;
static int	pipein = 0;
static char	magtape[BUFSIZ];
static char	*archivefile;
static int	bct;
static char	*tbf = NULL;
static union	u_spcl endoftapemark;
static struct	s_spcl dumpinfo;
static long	blksread;
static u_char	tapesread[MAXTAPES];
static jmp_buf	restart;
static int	gettingfile = 0;	/* restart has a valid frame */

static int	ofile;
static char	*map;
static char	lnkbuf[MAXPATHLEN + 1];
static int	pathlen;
       char	*host;		/* used in dumprmt.c */
       int	Bcvt;		/* Swap Bytes (for CCI or sun) */
static int	Qcvt;		/* Swap quads (for sun) */
static int	inodeinfo;	/* Have starting volume information */
static int	hostinfo;	/* Have dump host information */
/*
 * Set up an input source
 */
setinput(source, archive)
	char *source;
	char *archive;
{

	flsht();
	archivefile = archive;
	if (bflag)
		newtapebuf(ntrec);
	else
		/* let's hope this is evaluated at compile time! */
		newtapebuf(CARTRIDGETREC > HIGHDENSITYTREC ?
		    (NTREC > CARTRIDGETREC ? NTREC : CARTRIDGETREC) :
		    (NTREC > HIGHDENSITYTREC ? NTREC : HIGHDENSITYTREC));
	terminal = stdin;
	if (index(source, ':')) {
		char *tape;

		host = source;
		tape = index(host, ':');
		*tape++ = '\0';
		(void) strcpy(magtape, tape);
		if (rmthost() == 0)
			done(1);
		setuid(getuid());
	} else {
		setuid(getuid());
		if (strcmp(source, "-") == 0) {
			/*
			 * Since input is coming from a pipe we must establish
			 * our own connection to the terminal.
			 */
			terminal = fopen("/dev/tty", "r");
			if (terminal == NULL) {
				perror("Cannot open(\"/dev/tty\")");
				terminal = fopen("/dev/null", "r");
				if (terminal == NULL) {
					perror("Cannot open(\"/dev/null\")");
					done(1);
				}
			}
			pipein++;
			if (archivefile) {
				fprintf(stderr, "Cannot specify an archive%s\n",
					" file when reading from a pipe");
				done(1);
			}
		}
		(void) strcpy(magtape, source);
	}
}

newtapebuf(size)
	long size;
{
	static tbfsize = -1;

	ntrec = size;
	if (size <= tbfsize)
		return;
	if (tbf != NULL)
		free(tbf);
	tbf = (char *)malloc(size * TP_BSIZE);
	if (tbf == NULL) {
		fprintf(stderr, "Cannot allocate space for buffer\n");
		done(1);
	}
	tbfsize = size;
}

/*
 * Verify that the tape drive can be accessed and
 * that it actually is a dump tape.
 */
setup()
{
	int i, j, *ip;
	struct stat stbuf;
	extern int xtrmap(), xtrmapskip();

	vprintf(stdout, "Verify volume and initialize maps\n");
	if (archivefile) {
		if ((mt = open(archivefile, 0)) < 0) {
			perror(archivefile);
			done(1);
		}
		volno = 0;
	} else if (host) {
		if ((mt = rmtopen(magtape, 0)) < 0) {
			perror(magtape);
			done(1);
		}
		volno = 1;
	} else {
		if (pipein)
			mt = 0;
		else if ((mt = open(magtape, 0)) < 0) {
			perror(magtape);
			done(1);
		}
		volno = 1;
	}
	setdumpnum();
	flsht();
	if (!pipein && !bflag)
		findtapeblksize();
	if (gethead(&spcl) == FAIL) {
		bct--; /* push back this block */
		cvtflag++;
		if (gethead(&spcl) == FAIL) {
			fprintf(stderr, "Volume is not in dump format\n");
			done(1);
		}
		fprintf(stderr, "Converting to new file system format.\n");
	}
	if (pipein) {
		endoftapemark.s_spcl.c_magic = cvtflag ? OFS_MAGIC : NFS_MAGIC;
		endoftapemark.s_spcl.c_type = TS_END;
		ip = (int *)&endoftapemark;
		j = sizeof(union u_spcl) / sizeof(int);
		i = 0;
		do
			i += *ip++;
		while (--j);
		endoftapemark.s_spcl.c_checksum = CHECKSUM - i;
	}
	if (vflag && command != 't')
		printdumpinfo();
	dumptime = spcl.c_ddate;
	dumpdate = spcl.c_date;
	if (stat(".", &stbuf) < 0) {
		perror("cannot stat .");
		done(1);
	}
	if (stbuf.st_blksize > 0 && stbuf.st_blksize <= MAXBSIZE)
		fssize = stbuf.st_blksize;
	if (((fssize - 1) & fssize) != 0) {
		fprintf(stderr, "bad block size %d\n", fssize);
		done(1);
	}
	if (checkvol(&spcl, (long)1) == FAIL) {
		fprintf(stderr, "This is not volume 1 of the dump\n");
		done(1);
	}
	if (readhdr(&spcl) == FAIL)
		panic("no header after volume mark!\n");
	findinode(&spcl);
	if (checktype(&spcl, TS_CLRI) == FAIL) {
		fprintf(stderr, "Cannot find file removal list\n");
		done(1);
	}
	maxino = (spcl.c_count * TP_BSIZE * NBBY) + 1;
	dprintf(stdout, "maxino = %d\n", maxino);
	/*
	 * Allocate space for at least MAXINO inodes to allow us
	 * to restore partial dump tapes written before dump was
	 * fixed to write out the entire inode map.
	 */
	map = calloc((unsigned)1,
	    (unsigned)howmany(maxino > MAXINO ? maxino : MAXINO, NBBY));
	if (map == (char *)NIL)
		panic("no memory for file removal list\n");
	clrimap = map;
	curfile.action = USING;
	getfile(xtrmap, xtrmapskip);
	if (MAXINO > maxino)
		maxino = MAXINO;
	if (checktype(&spcl, TS_BITS) == FAIL) {
		fprintf(stderr, "Cannot find file dump list\n");
		done(1);
	}
	map = calloc((unsigned)1, (unsigned)howmany(maxino, NBBY));
	if (map == (char *)NULL)
		panic("no memory for file dump list\n");
	dumpmap = map;
	curfile.action = USING;
	getfile(xtrmap, xtrmapskip);
}

/*
 * Initialize fssize variable for 'R' command to work.
 */
setupR()
{
	struct stat stbuf;

	if (stat(".", &stbuf) < 0) {
		perror("cannot stat .");
		done(1);
	}
	fssize = stbuf.st_blksize;
	if (fssize <= 0 || ((fssize - 1) & fssize) != 0) {
		fprintf(stderr, "bad block size %d\n", fssize);
		done(1);
	}
}

/*
 * Prompt user to load a new dump volume.
 * "Nextvol" is the next suggested volume to use.
 * This suggested volume is enforced when doing full
 * or incremental restores, but can be overrridden by
 * the user when only extracting a subset of the files.
 */
getvol(nextvol)
	long nextvol;
{
	long newvol;
	long savecnt, i;
	union u_spcl tmpspcl;
#	define tmpbuf tmpspcl.s_spcl
	char buf[TP_BSIZE];
	extern char *ctime();

	if (nextvol == 1) {
		for (i = 0;  i < MAXTAPES;  i++)
			tapesread[i] = 0;
		gettingfile = 0;
	}
	if (pipein) {
		if (nextvol != 1)
			panic("Changing volumes on pipe input?\n");
		if (volno == 1)
			return;
		goto gethdr;
	}
	savecnt = blksread;
again:
	if (pipein)
		done(1); /* pipes do not get a second chance */
	if (command == 'R' || command == 'r' || curfile.action != SKIP)
		newvol = nextvol;
	else
		newvol = 0;
	while (newvol <= 0) {
		int n = 0;

		for (i = 0;  i < MAXTAPES;  i++)
			if (tapesread[i])
				n++;
		if (n == 0) {
			fprintf(stderr, "%s%s%s%s%s",
			    "You have not read any volumes yet.\n",
			    "Unless you know which volume your",
			    " file(s) are on you should start\n",
			    "with the last volume and work",
			    " towards the first.\n");
		} else {
			fprintf(stderr, "You have read volumes");
			strcpy(tbf, ": ");
			for (i = 0; i < MAXTAPES; i++)
				if (tapesread[i]) {
					fprintf(stderr, "%s%d", tbf, i+1);
					strcpy(tbf, ", ");
				}
			fprintf(stderr, "\n");
		}
		do	{
			fprintf(stderr, "Specify next volume #: ");
			(void) fflush(stderr);
			(void) fgets(tbf, BUFSIZ, terminal);
		} while (!feof(terminal) && tbf[0] == '\n');
		if (feof(terminal))
			done(1);
		newvol = atoi(tbf);
		if (newvol <= 0) {
			fprintf(stderr,
			    "Volume numbers are positive numerics\n");
		}
		if (newvol > MAXTAPES) {
			fprintf(stderr,
			    "This program can only deal with %d volumes\n",
			    MAXTAPES);
			newvol = 0;
		}
	}
	if (newvol == volno) {
		tapesread[volno-1]++;
		return;
	}
	closemt();
	fprintf(stderr, "Mount volume %d\n", newvol);
	fprintf(stderr, "then enter volume name (default: %s) ", magtape);
	(void) fflush(stderr);
	(void) fgets(tbf, BUFSIZ, terminal);
	if (feof(terminal))
		done(1);
	if (tbf[0] != '\n') {
		(void) strcpy(magtape, tbf);
		magtape[strlen(magtape) - 1] = '\0';
	}
	if ((host != 0 && (mt = rmtopen(magtape, 0)) == -1) ||
	    (host == 0 && (mt = open(magtape, 0)) == -1)) {
		fprintf(stderr, "Cannot open %s\n", magtape);
		volno = -1;
		goto again;
	}
gethdr:
	volno = newvol;
	setdumpnum();
	flsht();
	if (readhdr(&tmpbuf) == FAIL) {
		fprintf(stderr, "volume is not in dump format\n");
		volno = 0;
		goto again;
	}
	if (checkvol(&tmpbuf, volno) == FAIL) {
		fprintf(stderr, "Wrong volume (%d)\n", tmpbuf.c_volume);
		volno = 0;
		goto again;
	}
	if (tmpbuf.c_date != dumpdate || tmpbuf.c_ddate != dumptime) {
		fprintf(stderr, "Wrong dump date\n\tgot: %s",
			ctime(&tmpbuf.c_date));
		fprintf(stderr, "\twanted: %s", ctime(&dumpdate));
		volno = 0;
		goto again;
	}
	tapesread[volno-1]++;
	blksread = savecnt;
	if (curfile.action == USING) {
		if (volno == 1)
			panic("active file into volume 1\n");
		return;
	}
	/*
	 * Skip up to the beginning of the next record
	 */
	if (tmpbuf.c_type == TS_TAPE && (tmpbuf.c_flags & DR_NEWHEADER))
		for (i = tmpbuf.c_count; i > 0; i--)
			readtape(buf);
	(void) gethead(&spcl);
	findinode(&spcl);
	if (gettingfile) {
		gettingfile = 0;
		longjmp(restart, 1);
	}
}

/*
 * handle multiple dumps per tape by skipping forward to the
 * appropriate one.
 */
setdumpnum()
{
	struct mtop tcom;

	if (dumpnum == 1 || volno != 1)
		return;
	if (pipein) {
		fprintf(stderr, "Cannot have multiple dumps on pipe input\n");
		done(1);
	}
	tcom.mt_op = MTFSF;
	tcom.mt_count = dumpnum - 1;
	if (host)
		rmtioctl(MTFSF, dumpnum - 1);
	else
		if (ioctl(mt, (int)MTIOCTOP, (char *)&tcom) < 0)
			perror("ioctl MTFSF");
}

printdumpinfo()
{
	int i;

	fprintf(stdout, "Dump   date: %s", ctime(&dumpinfo.c_date));
	fprintf(stdout, "Dumped from: %s",
	    (dumpinfo.c_ddate == 0) ? "the epoch\n" : ctime(&dumpinfo.c_ddate));
	if (hostinfo) {
		fprintf(stdout, "Level %d dump of %s on %s:%s\n",
		    dumpinfo.c_level, dumpinfo.c_filesys,
		    dumpinfo.c_host, dumpinfo.c_dev);
		fprintf(stdout, "Label: %s\n", dumpinfo.c_label);
	}
	if (inodeinfo) {
		fprintf(stdout, "Starting inode numbers by volume:\n");
		for (i = 1; i <= dumpinfo.c_volume; i++)
			fprintf(stdout, "\tVolume %d: %6d\n",
			    i, dumpinfo.c_inos[i]);
	}
}

extractfile(name)
	char *name;
{
	int mode;
	time_t timep[2];
	struct entry *ep;
	extern int xtrlnkfile(), xtrlnkskip();
	extern int xtrfile(), xtrskip();

	curfile.name = name;
	curfile.action = USING;
	timep[0] = curfile.dip->di_atime;
	timep[1] = curfile.dip->di_mtime;
	mode = curfile.dip->di_mode;
	switch (mode & IFMT) {

	default:
		fprintf(stderr, "%s: unknown file mode 0%o\n", name, mode);
		skipfile();
		return (FAIL);

	case IFSOCK:
		vprintf(stdout, "skipped socket %s\n", name);
		skipfile();
		return (GOOD);

	case IFDIR:
		if (mflag) {
			ep = lookupname(name);
			if (ep == NIL || ep->e_flags & EXTRACT)
				panic("unextracted directory %s\n", name);
			skipfile();
			return (GOOD);
		}
		vprintf(stdout, "extract file %s\n", name);
		return (genliteraldir(name, curfile.ino));

	case IFLNK:
		lnkbuf[0] = '\0';
		pathlen = 0;
		getfile(xtrlnkfile, xtrlnkskip);
		if (pathlen == 0) {
			vprintf(stdout,
			    "%s: zero length symbolic link (ignored)\n", name);
			return (GOOD);
		}
		return (linkit(lnkbuf, name, SYMLINK));

	case IFCHR:
	case IFBLK:
	case IFIFO:
		vprintf(stdout, "extract special file %s\n", name);
		if (mknod(name, mode, (int)curfile.dip->di_rdev) < 0) {
			fprintf(stderr, "%s: ", name);
			(void) fflush(stderr);
			perror("cannot create special file");
			skipfile();
			return (FAIL);
		}
		(void) chown(name, curfile.dip->di_uid, curfile.dip->di_gid);
		(void) chmod(name, mode);
		skipfile();
		utime(name, timep);
		return (GOOD);

	case IFREG:
		vprintf(stdout, "extract file %s\n", name);
		if ((ofile = creat(name, 0666)) < 0) {
			fprintf(stderr, "%s: ", name);
			(void) fflush(stderr);
			perror("cannot create file");
			skipfile();
			return (FAIL);
		}
		(void) fchown(ofile, curfile.dip->di_uid, curfile.dip->di_gid);
		(void) fchmod(ofile, mode);
		getfile(xtrfile, xtrskip);
		(void) close(ofile);
		utime(name, timep);
		return (GOOD);
	}
	/* NOTREACHED */
}

/*
 * skip over bit maps on the tape
 */
skipmaps()
{

	while (checktype(&spcl, TS_CLRI) == GOOD ||
	       checktype(&spcl, TS_BITS) == GOOD)
		skipfile();
}

/*
 * skip over a file on the tape
 */
skipfile()
{
	extern int null();

	curfile.action = SKIP;
	getfile(null, null);
}

/*
 * Do the file extraction, calling the supplied functions
 * with the blocks
 */
getfile(f1, f2)
	int	(*f2)(), (*f1)();
{
	register int i;
	int curblk = 0;
	off_t size = spcl.c_dinode.di_size;
	static char clearedbuf[MAXBSIZE];
	char buf[MAXBSIZE / TP_BSIZE][TP_BSIZE];
	char junk[TP_BSIZE];

	if (checktype(&spcl, TS_END) == GOOD)
		panic("ran off end of volume\n");
	if (ishead(&spcl) == FAIL)
		panic("not at beginning of a file\n");
	if (!gettingfile && setjmp(restart) != 0)
		return;
	gettingfile++;
loop:
	for (i = 0; i < spcl.c_count; i++) {
		if (spcl.c_addr[i]) {
			readtape(&buf[curblk++][0]);
			if (curblk == fssize / TP_BSIZE) {
				(*f1)(buf, size > TP_BSIZE ?
				     (long) (fssize) :
				     (curblk - 1) * TP_BSIZE + size);
				curblk = 0;
			}
		} else {
			if (curblk > 0) {
				(*f1)(buf, size > TP_BSIZE ?
				     (long) (curblk * TP_BSIZE) :
				     (curblk - 1) * TP_BSIZE + size);
				curblk = 0;
			}
			(*f2)(clearedbuf, size > TP_BSIZE ?
				(long) TP_BSIZE : size);
		}
		if ((size -= TP_BSIZE) <= 0) {
			for (i++; i < spcl.c_count; i++)
				if (spcl.c_addr[i])
					readtape(junk);
			break;
		}
	}
	if (readhdr(&spcl) == GOOD && size > 0) {
		if (checktype(&spcl, TS_ADDR) == GOOD)
			goto loop;
		dprintf(stdout, "Missing address (header) block for %s\n",
			curfile.name);
	}
	if (curblk > 0)
		(*f1)(buf, (curblk * TP_BSIZE) + size);
	findinode(&spcl);
	gettingfile = 0;
}

/*
 * The next routines are called during file extraction to
 * put the data into the right form and place.
 */
xtrfile(buf, size)
	char	*buf;
	long	size;
{

	if (write(ofile, buf, (int) size) == -1) {
		fprintf(stderr, "write error extracting inode %d, name %s\n",
			curfile.ino, curfile.name);
		perror("write");
		done(1);
	}
}

xtrskip(buf, size)
	char *buf;
	long size;
{

#ifdef	lint
	buf = buf;
#endif
	if (lseek(ofile, size, 1) == (long)-1) {
		fprintf(stderr, "seek error extracting inode %d, name %s\n",
			curfile.ino, curfile.name);
		perror("lseek");
		done(1);
	}
}

xtrlnkfile(buf, size)
	char	*buf;
	long	size;
{

	pathlen += size;
	if (pathlen > MAXPATHLEN) {
		fprintf(stderr, "symbolic link name: %s->%s%s; too long %d\n",
		    curfile.name, lnkbuf, buf, pathlen);
		done(1);
	}
	(void) strcat(lnkbuf, buf);
}

xtrlnkskip(buf, size)
	char *buf;
	long size;
{

#ifdef	lint
	buf = buf, size = size;
#endif
	fprintf(stderr, "unallocated block in symbolic link %s\n",
		curfile.name);
	done(1);
}

xtrmap(buf, size)
	char	*buf;
	long	size;
{

	bcopy(buf, map, size);
	map += size;
}

xtrmapskip(buf, size)
	char *buf;
	long size;
{

#ifdef	lint
	buf = buf;
#endif
	panic("hole in map\n");
	map += size;
}

null() {;}

/*
 * Do the tape i/o, dealing with volume changes
 * etc..
 */
readtape(b)
	char *b;
{
	register long i;
	long rd, newvol;
	int cnt;
	struct s_spcl *sp;

	if (bct < ntrec) {
		/*
		 * an end-of-media record may occur in the middle of
		 * a buffer
		 */
		sp = &((union u_spcl *) &tbf[bct*TP_BSIZE])->s_spcl;
		if (sp->c_magic == NFS_MAGIC && sp->c_type == TS_EOM) {
			for (i = 0; i < ntrec; i++)
				((struct s_spcl *)&tbf[i*TP_BSIZE])->c_magic = 0;
			bct = 0;
			cnt = ntrec*TP_BSIZE;
			rd = 0;
			i = 0;
			goto nextvol;
		}
		/*
		 * copy next record from buffer
		 */
		bcopy(&tbf[(bct++*TP_BSIZE)], b, (long)TP_BSIZE);
		blksread++;
		return;
	}
	for (i = 0; i < ntrec; i++)
		((struct s_spcl *)&tbf[i*TP_BSIZE])->c_magic = 0;
	bct = 0;
	cnt = ntrec*TP_BSIZE;
	rd = 0;
getmore:
	if (host)
		i = rmtread(&tbf[rd], cnt);
	else
		i = read(mt, &tbf[rd], cnt);
	if (i > 0 && i != ntrec*TP_BSIZE) {
		if (pipein) {
			rd += i;
			cnt -= i;
			if (cnt > 0)
				goto getmore;
			i = rd;
		} else {
			if (i % TP_BSIZE != 0)
				panic("partial block read: %d should be %d\n",
					i, ntrec * TP_BSIZE);
			bcopy((char *)&endoftapemark, &tbf[i],
				(long)TP_BSIZE);
		}
	}
	if (i < 0) {
		fprintf(stderr, "Read error while ");
		switch (curfile.action) {
		default:
			fprintf(stderr, "trying to set up volume\n");
			break;
		case UNKNOWN:
			fprintf(stderr, "trying to resynchronize\n");
			break;
		case USING:
			fprintf(stderr, "restoring %s\n", curfile.name);
			break;
		case SKIP:
			fprintf(stderr, "skipping over inode %d\n",
				curfile.ino);
			break;
		}
		if (!yflag && !reply("continue"))
			done(1);
		i = ntrec*TP_BSIZE;
		bzero(tbf, i);
		if ((host != 0 && rmtseek(i, 1) < 0) ||
		    (host == 0 && lseek(mt, i, 1) == (long)-1)) {
			perror("continuation failed");
			done(1);
		}
	}
	/*
	 * change volumes if at the end of file or if the first
	 * record in the buffer just read is an end-of-media record
	 */
	sp = &((union u_spcl *) tbf)->s_spcl;
	if (i != 0 && sp->c_magic == NFS_MAGIC && sp->c_type == TS_EOM)
		i = 0;
nextvol:
	if (i == 0) {
		if (!pipein) {
			newvol = volno + 1;
			volno = 0;
			getvol(newvol);
			readtape(b);
			return;
		}
		if (rd % TP_BSIZE != 0)
			panic("partial block read: %d should be %d\n",
				rd, ntrec * TP_BSIZE);
		bcopy((char *)&endoftapemark, &tbf[rd], (long)TP_BSIZE);
	}
	bcopy(&tbf[(bct++*TP_BSIZE)], b, (long)TP_BSIZE);
	blksread++;
}

findtapeblksize()
{
	register long i;

	for (i = 0; i < ntrec; i++)
		((struct s_spcl *)&tbf[i * TP_BSIZE])->c_magic = 0;
	bct = 0;
	if (host)
		i = rmtread(tbf, ntrec * TP_BSIZE);
	else
		i = read(mt, tbf, ntrec * TP_BSIZE);
	if (i <= 0) {
		perror("Media read error");
		done(1);
	}
	if (i % TP_BSIZE != 0) {
		fprintf(stderr, "Record size (%d) %s (%d)\n",
			i, "is not a multiple of dump block size", TP_BSIZE);
		done(1);
	}
	ntrec = i / TP_BSIZE;
	vprintf(stdout, "Media block size is %d\n", ntrec*2);
}

flsht()
{

	bct = ntrec+1;
}

closemt()
{
	if (mt < 0)
		return;
	if (host)
		rmtclose();
	else {
		/* only way to tell if this is a floppy is to issue an ioctl */
		/* but why waste one - if the eject failes, tough! */
		(void)ioctl( mt, FDKEJECT, 0 );
		(void) close(mt);
	}
}

checkvol(b, t)
	struct s_spcl *b;
	long t;
{

	if (b->c_volume != t)
		return (FAIL);
	return (GOOD);
}

readhdr(b)
	struct s_spcl *b;
{

	if (gethead(b) == FAIL) {
		dprintf(stdout, "readhdr fails at %d blocks\n", blksread);
		return (FAIL);
	}
	return (GOOD);
}

/*
 * read the tape into buf, then return whether or
 * or not it is a header block.
 */
gethead(buf)
	struct s_spcl *buf;
{
	long i, *j;
	union u_ospcl {
		char dummy[TP_BSIZE];
		struct	s_ospcl {
			long	c_type;
			long	c_date;
			long	c_ddate;
			long	c_volume;
			long	c_tapea;
			u_short	c_inumber;
			long	c_magic;
			long	c_checksum;
			struct odinode {
				unsigned short odi_mode;
				u_short	odi_nlink;
				u_short	odi_uid;
				u_short	odi_gid;
				long	odi_size;
				long	odi_rdev;
				char	odi_addr[36];
				long	odi_atime;
				long	odi_mtime;
				long	odi_ctime;
			} c_dinode;
			long	c_count;
			char	c_baddr[256];
		} s_ospcl;
	} u_ospcl;

	if (!cvtflag) {
		readtape((char *)buf);
		if (buf->c_magic != NFS_MAGIC) {
			if (swabl(buf->c_magic) != NFS_MAGIC)
				return (FAIL);
			if (!Bcvt) {
				vprintf(stdout, "Note: Doing Byte swapping\n");
				Bcvt = 1;
			}
		}
		if (checksum((int *)buf) == FAIL)
			return (FAIL);
		if (Bcvt)
			swabst("8l4s31l", (char *)buf);
		goto good;
	}
	readtape((char *)(&u_ospcl.s_ospcl));
	bzero((char *)buf, (long)TP_BSIZE);
	buf->c_type = u_ospcl.s_ospcl.c_type;
	buf->c_date = u_ospcl.s_ospcl.c_date;
	buf->c_ddate = u_ospcl.s_ospcl.c_ddate;
	buf->c_volume = u_ospcl.s_ospcl.c_volume;
	buf->c_tapea = u_ospcl.s_ospcl.c_tapea;
	buf->c_inumber = u_ospcl.s_ospcl.c_inumber;
	buf->c_checksum = u_ospcl.s_ospcl.c_checksum;
	buf->c_magic = u_ospcl.s_ospcl.c_magic;
	buf->c_dinode.di_mode = u_ospcl.s_ospcl.c_dinode.odi_mode;
	buf->c_dinode.di_nlink = u_ospcl.s_ospcl.c_dinode.odi_nlink;
	buf->c_dinode.di_uid = u_ospcl.s_ospcl.c_dinode.odi_uid;
	buf->c_dinode.di_gid = u_ospcl.s_ospcl.c_dinode.odi_gid;
	buf->c_dinode.di_size = u_ospcl.s_ospcl.c_dinode.odi_size;
	buf->c_dinode.di_rdev = u_ospcl.s_ospcl.c_dinode.odi_rdev;
	buf->c_dinode.di_atime = u_ospcl.s_ospcl.c_dinode.odi_atime;
	buf->c_dinode.di_mtime = u_ospcl.s_ospcl.c_dinode.odi_mtime;
	buf->c_dinode.di_ctime = u_ospcl.s_ospcl.c_dinode.odi_ctime;
	buf->c_count = u_ospcl.s_ospcl.c_count;
	bcopy(u_ospcl.s_ospcl.c_baddr, buf->c_addr, (long)256);
	if (u_ospcl.s_ospcl.c_magic != OFS_MAGIC ||
	    checksum((int *)(&u_ospcl.s_ospcl)) == FAIL)
		return (FAIL);
	buf->c_magic = NFS_MAGIC;

good:
	j = buf->c_dinode.di_ic.ic_size.val;
	i = j[1];
	if (buf->c_dinode.di_size == 0 &&
	    (buf->c_dinode.di_mode & IFMT) == IFDIR && Qcvt==0) {
		if (*j || i) {
			printf("Note: Doing Quad swapping\n");
			Qcvt = 1;
		}
	}
	if (Qcvt) {
		j[1] = *j; *j = i;
	}
	switch (buf->c_type) {

	case TS_CLRI:
	case TS_BITS:
		/*
		 * Have to patch up missing information in bit map headers
		 */
		buf->c_inumber = 0;
		buf->c_dinode.di_size = buf->c_count * TP_BSIZE;
		for (i = 0; i < buf->c_count; i++)
			buf->c_addr[i]++;
		break;

	case TS_TAPE:
	case TS_END:
		if (dumpinfo.c_date == 0) {
			dumpinfo.c_date = spcl.c_date;
			dumpinfo.c_ddate = spcl.c_ddate;
		}
		if (!hostinfo && spcl.c_host[0] != '\0') {
			bcopy(spcl.c_label, dumpinfo.c_label, LBLSIZE);
			bcopy(spcl.c_filesys, dumpinfo.c_filesys, NAMELEN);
			bcopy(spcl.c_dev, dumpinfo.c_dev, NAMELEN);
			bcopy(spcl.c_host, dumpinfo.c_host, NAMELEN);
			dumpinfo.c_level = spcl.c_level;
			hostinfo++;
		}
		if (!inodeinfo && (spcl.c_flags & DR_INODEINFO)) {
			dumpinfo.c_volume = spcl.c_volume;
			bcopy(spcl.c_inos, dumpinfo.c_inos,
			    sizeof(spcl.c_inos));
			inodeinfo++;
		}
		buf->c_inumber = 0;
		break;

	case TS_INODE:
	case TS_ADDR:
		break;

	default:
		panic("gethead: unknown inode type %d\n", buf->c_type);
		break;
	}
	if (dflag)
		accthdr(buf);
	return (GOOD);
}

/*
 * Check that a header is where it belongs and predict the next header
 */
accthdr(header)
	struct s_spcl *header;
{
	static ino_t previno = 0x7fffffff;
	static int prevtype;
	static long predict;
	long blks, i;

	if (header->c_type == TS_TAPE) {
		fprintf(stderr, "Volume header\n");
		previno = 0x7fffffff;
		return;
	}
	if (previno == 0x7fffffff)
		goto newcalc;
	switch (prevtype) {
	case TS_BITS:
		fprintf(stderr, "Dump mask header");
		break;
	case TS_CLRI:
		fprintf(stderr, "Remove mask header");
		break;
	case TS_INODE:
		fprintf(stderr, "File header, ino %d", previno);
		break;
	case TS_ADDR:
		fprintf(stderr, "File continuation header, ino %d", previno);
		break;
	case TS_END:
		fprintf(stderr, "End of media header");
		break;
	}
	if (predict != blksread - 1)
		fprintf(stderr, "; predicted %d blocks, got %d blocks",
			predict, blksread - 1);
	fprintf(stderr, "\n");
newcalc:
	blks = 0;
	if (header->c_type != TS_END)
		for (i = 0; i < header->c_count; i++)
			if (header->c_addr[i] != 0)
				blks++;
	predict = blks;
	blksread = 0;
	prevtype = header->c_type;
	previno = header->c_inumber;
}

/*
 * Try to determine which volume a file resides on.
 */
volnumber(inum)
	ino_t inum;
{
	int i;

	if (inodeinfo == 0)
		return (0);
	for (i = 1; i <= dumpinfo.c_volume; i++)
		if (inum < dumpinfo.c_inos[i])
			break;
	return (i - 1);
}

/*
 * Find an inode header.
 */
findinode(header)
	struct s_spcl *header;
{
	static long skipcnt = 0;
	long i;
	char buf[TP_BSIZE];

	curfile.name = "<name unknown>";
	curfile.action = UNKNOWN;
	curfile.dip = (struct dinode *)NIL;
	curfile.ino = 0;
	curfile.ts = 0;
	if (ishead(header) == FAIL) {
		skipcnt++;
		while (gethead(header) == FAIL || header->c_date != dumpdate)
			skipcnt++;
	}
	for (;;) {
		if (checktype(header, TS_ADDR) == GOOD) {
			/*
			 * Skip up to the beginning of the next record
			 */
			for (i = 0; i < header->c_count; i++)
				if (header->c_addr[i])
					readtape(buf);
			(void) gethead(header);
			continue;
		}
		if (checktype(header, TS_INODE) == GOOD) {
			curfile.dip = &header->c_dinode;
			curfile.ino = header->c_inumber;
			curfile.ts = TS_INODE;
			break;
		}
		if (checktype(header, TS_END) == GOOD) {
			curfile.ino = maxino;
			curfile.ts = TS_END;
			break;
		}
		if (checktype(header, TS_CLRI) == GOOD) {
			curfile.name = "<file removal list>";
			curfile.ts = TS_CLRI;
			break;
		}
		if (checktype(header, TS_BITS) == GOOD) {
			curfile.name = "<file dump list>";
			curfile.ts = TS_BITS;
			break;
		}
		while (gethead(header) == FAIL)
			skipcnt++;
	}
	if (skipcnt > 0)
		fprintf(stderr, "resync restore, skipped %d blocks\n", skipcnt);
	skipcnt = 0;
}

/*
 * return whether or not the buffer contains a header block
 */
ishead(buf)
	struct s_spcl *buf;
{

	if (buf->c_magic != NFS_MAGIC)
		return (FAIL);
	return (GOOD);
}

checktype(b, t)
	struct s_spcl *b;
	int	t;
{

	if (b->c_type != t)
		return (FAIL);
	return (GOOD);
}

checksum(b)
	register int *b;
{
	register int i, j;

	j = sizeof(union u_spcl) / sizeof(int);
	i = 0;
	if(!Bcvt) {
		do
			i += *b++;
		while (--j);
	} else {
		/* What happens if we want to read restore tapes
			for a 16bit int machine??? */
		do
			i += swabl(*b++);
		while (--j);
	}

	if (i != CHECKSUM) {
		fprintf(stderr, "Checksum error %o, inode %d file %s\n", i,
			curfile.ino, curfile.name);
		return (FAIL);
	}
	return (GOOD);
}

/* VARARGS1 */
msg(cp, a1, a2, a3)
	char *cp;
{

	fprintf(stderr, cp, a1, a2, a3);
}

swabst(cp, sp)
	register char *cp, *sp;
{
	int n = 0;
	char c;
	while (*cp) {
		switch (*cp) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = (n * 10) + (*cp++ - '0');
			continue;

		case 's': case 'w': case 'h':
			c = sp[0]; sp[0] = sp[1]; sp[1] = c;
			sp++;
			break;

		case 'l':
			c = sp[0]; sp[0] = sp[3]; sp[3] = c;
			c = sp[2]; sp[2] = sp[1]; sp[1] = c;
			sp += 3;
		}
		sp++; /* Any other character, like 'b' counts as byte. */
		if (n <= 1) {
			n = 0; cp++;
		} else
			n--;
	}
}

swabl(x)
{
	unsigned long l = x;

	swabst("l", (char *)&l);
	return (l);
}
