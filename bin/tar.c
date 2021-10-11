/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char sccsid[] = "@(#)tar.c 1.1 92/07/30 SMI"; /* from UCB 5.7 86/04/26 */
#endif not lint

/*
 * tar: Tape Archival Program
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#define TBLOCK	512
#define NBLOCK	20
#define NAMSIZ	100

#define	writetape(b)	writetbuf(b, 1)
#define	min(a,b)  ((a) < (b) ? (a) : (b))
#define	max(a,b)  ((a) > (b) ? (a) : (b))

union hblock {
	char dummy[TBLOCK];
	struct header {
		char name[NAMSIZ];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];
		char mtime[12];
		char chksum[8];
		char linkflag;
		char linkname[NAMSIZ];
	} dbuf;
};

struct linkbuf {
	ino_t	inum;
	dev_t	devnum;
	int	count;
	char	pathname[NAMSIZ];
	struct	linkbuf *nextp;
};

/* see comments before build_table() */
#define TABLE_SIZE 512
struct	file_list	{
	char	*name;			/* Name of file to {in,ex}clude */
	struct	file_list	*next;	/* Linked list */
};
struct	file_list	*exclude_tbl[TABLE_SIZE], *include_tbl[TABLE_SIZE];

union	hblock dblock;
union	hblock *tbuf;
struct	linkbuf *ihead;
struct	stat stbuf;

extern int errno;
extern char *sys_errlist[];

int	rflag;
int	xflag;
int	Xflag;
int	vflag;
int	tflag;
int	cflag;
int	mflag;
int	fflag;
int	iflag;
int	oflag;
int	pflag;
int	wflag;
int	hflag;
int	Bflag;
int	Fflag;
int	Iflag;
int	eflag;

int	mt;
int	term;
int	chksum;
int	recno;
int	first;
int	prtlinkerr;
int	freemem = 1;
int	nblock = 0;
int	eofmsg = 1;
int	eof_flag = 1;	/* Disable reading to EOF */
void	onintr();
void	onquit();
void	onhup();
#ifdef notdef
void	onterm();
#endif
unsigned int hash();

daddr_t	low;
daddr_t	high;
daddr_t	bsrch();

FILE	*vfile = stdout;
FILE	*tfile;
char	tname[] = "/tmp/tarXXXXXX";
char	*usefile;
char	magtape[] = "/dev/rmt8";
char	*malloc();
long	time();
long	lseek();
char	*mktemp();
char	*sprintf();
char	*strcat();
char	*strcpy();
char	*rindex();
char	*index();
char	*getcwd();
char	*getwd();
char	*getmem();
char	*getenv();

main(argc, argv)
int	argc;
char	*argv[];
{
	char *cp;

	if (argc < 2)
		usage();

	tfile = NULL;
        if ((usefile = getenv("TAPE")) == NULL)
		usefile =  magtape;
	argv[argc] = 0;
	argv++;
	for (cp = *argv++; *cp; cp++)
		switch(*cp) {

		case 'f':
			if (*argv == 0) {
				fprintf(stderr,
			"tar: tapefile must be specified with 'f' option\n");
				usage();
			}
			usefile = *argv++;
			fflag++;
			break;

		case 'c':
			cflag++;
			rflag++;
			break;

		case 'o':
			oflag++;
			break;

		case 'p':
			pflag++;
			break;
		
		case 'u':
			mktemp(tname);
			if ((tfile = fopen(tname, "w")) == NULL) {
				fprintf(stderr,
				 "tar: cannot create temporary file (%s)\n",
				 tname);
				done(1);
			}
			fprintf(tfile, "!!!!!/!/!/!/!/!/!/! 000\n");
			/*FALL THRU*/

		case 'r':
			rflag++;
			break;

		case 'v':
			vflag++;
			break;

		case 'w':
			wflag++;
			break;

		case 'x':
			xflag++;
			break;

		case 'X':
			if (*argv == 0) {
				fprintf(stderr,
		"tar: exclude file must be specified with 'X' option\n");
				usage();
			}
			Xflag = 1;
			build_table(exclude_tbl, *argv++);
			break;

		case 't':
			tflag++;
			break;

		case 'm':
			mflag++;
			break;

		case '-':
			break;

		case '0':
		case '1':
		case '4':
		case '5':
		case '7':
		case '8':
			magtape[8] = *cp;
			usefile = magtape;
			break;

		case 'b':
			if (*argv == 0) {
				fprintf(stderr,
			"tar: blocksize must be specified with 'b' option\n");
				usage();
			}
			nblock = atoi(*argv);
			if (nblock <= 0) {
				fprintf(stderr,
				    "tar: invalid blocksize \"%s\"\n", *argv);
				done(1);
			}
			argv++;
			break;

		case 'l':
			prtlinkerr++;
			break;

		case 'h':
			hflag++;
			break;

		case 'i':
			iflag++;
			break;

		case 'B':
			Bflag++;
			break;

		case 'F':
			Fflag++;
			break;
		
		case 'e':
			eflag++;
			break;

		default:
			fprintf(stderr, "tar: %c: unknown option\n", *cp);
			usage();
		}

	if (!rflag && !xflag && !tflag)
		usage();
	if ((rflag && xflag) || (xflag && tflag) || (rflag && tflag)) {
		fprintf(stderr, "tar: specify only one of [ctxru].\n");
		usage();
	}
	if (rflag) {
		if (cflag && tfile != NULL)
			usage();
		if (signal(SIGINT, SIG_IGN) != SIG_IGN)
			(void) signal(SIGINT, onintr);
		if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
			(void) signal(SIGHUP, onhup);
		if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
			(void) signal(SIGQUIT, onquit);
#ifdef notdef
		if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
			(void) signal(SIGTERM, onterm);
#endif
		if (cflag && *argv == NULL)
			usage();
		mt = openmt(usefile, 1);
		dorep(argv);
		done(0);
	}
	mt = openmt(usefile, 0);
	/* for each argument, check to see if there is a "-I file" pair.
	 * if so, move the 3rd argument into "-I"'s place, build_table()
	 * using "file"'s name and increment argc one (the second increment
	 * appears in the for loop) which removes the two args "-I" and "file"
	 * from the argument vector.
	 */
	for (argc = 0; argv[argc]; argc++)
		if (!strcmp(argv[argc], "-I"))
			if (!argv[argc+1]) {
				fprintf(stderr,
				    "tar: missing argument for -I flag\n");
				done(2);
			} else {
				Iflag = 1;
				argv[argc] = argv[argc+2];
				build_table(include_tbl, argv[++argc]);
			}
	if (xflag)
		doxtract(argv);
	else
		dotable(argv);
	 /* Always read the second end_of_file marker */
	 if (endtape ())
	 	readtape ((char *)&dblock);
	done(0);
	/* NOTREACHED */
}

usage()
{
	fprintf(stderr,
"tar: usage: tar -{ctxru}[vfblmhopwBiX] [tapefile] [blocksize] file1 file2...\n");
	done(1);
}

int
openmt(tape, writing)
	char *tape;
	int writing;
{
	if (strcmp(tape, "-") == 0) {
		/*
		 * Read from standard input or write to standard output.
		 */
		if (writing) {
			if (cflag == 0) {
				fprintf(stderr,
			 "tar: can only create standard output archives\n");
				done(1);
			}
			vfile = stderr;
			setlinebuf(vfile);
			mt = dup(1);
		} else {
			mt = dup(0);
			Bflag++;
		}
	} else {
		/*
		 * Use file or tape on local machine.
		 */
		if (writing) {
			if (cflag)
				mt = open(tape, O_RDWR|O_CREAT|O_TRUNC, 0666);
			else
				mt = open(tape, O_RDWR);
		} else
			mt = open(tape, O_RDONLY);
		if (mt < 0)
			vperror(1, tape);
	}
	return(mt);
}

dorep(argv)
	char *argv[];
{
	register char *cp, *cp2, *p;
	char wdir[MAXPATHLEN], tempdir[MAXPATHLEN], *parent;
	char file[MAXPATHLEN], origdir[MAXPATHLEN];
	FILE *fp = (FILE *)NULL;

	if (!cflag) {
		getdir();
		do {
			passtape();
			if (term)
				done(0);
			getdir();
		} while (!endtape());
		backtape();
		if (tfile != NULL) {
			char buf[200];

			sprintf(buf,
"sort +0 -1 +1nr %s -o %s; awk '$1 != prev {print; prev=$1}' %s >%sX; mv %sX %s",
				tname, tname, tname, tname, tname, tname);
			fflush(tfile);
			system(buf);
			freopen(tname, "r", tfile);
			fstat(fileno(tfile), &stbuf);
			high = stbuf.st_size;
		}
	}

	(void) getcwd(wdir);
	strcpy(origdir, wdir);
	while ((*argv || fp) && !term) {
		cp2 = 0;
		if (fp || !strcmp(*argv, "-I")) {
			if (fp == NULL) {
				if (*++argv == NULL) {
					fprintf(stderr,
				    "tar: missing file name for -I flag.\n");
					done(1);
				} else if ((fp = fopen(*argv++, "r")) == NULL)
					vperror(0, argv[-1]);
				continue;
			} else if ((fgets(file, MAXPATHLEN-1, fp)) == NULL) {
				fclose(fp);
				fp = NULL;
				continue;
			} else {
				cp = cp2 = file;
				if (p = index(cp2, '\n'))
					*p = 0;
			}
		} else {
			if (!strcmp(*argv, "-C") && argv[1]) {
				if (chdir(*++argv) < 0)
					vperror(0,
					   "can't change directories to %s",
					   *argv);
				else
					(void) getcwd(wdir);
				argv++;
				continue;
			}
			cp = cp2 = strcpy(file, *argv++);
		}
		parent = wdir;
		for (; *cp; cp++)
			if (*cp == '/')
				cp2 = cp;
		if (cp2 != file) {
			*cp2 = '\0';
			if (chdir(file) < 0) {
				vperror(0,
				    "can't change directories to %s", file);
				continue;
			}
			parent = getcwd(tempdir);
			*cp2 = '/';
			cp2++;
		}
		putfile(file, cp2, parent);
		if (chdir(origdir) < 0)
			vperror(0, "cannot change back?: ", wdir);
	}
	putempty();
	putempty();
	flushtape();
	if (prtlinkerr == 0)
		return;
	for (; ihead != NULL; ihead = ihead->nextp) {
		if (ihead->count == 0)
			continue;
		fprintf(stderr, "tar: missing links to %s\n", ihead->pathname);
		if (eflag)
			done(1);
	}
}

endtape()
{
	return (dblock.dbuf.name[0] == '\0');
}

getdir()
{
	register struct stat *sp;
	int i;

top:
	readtape((char *)&dblock);
	if (dblock.dbuf.name[0] == '\0')
		return;
	sp = &stbuf;
	sscanf(dblock.dbuf.mode, "%o", &i);
	sp->st_mode = i;
	sscanf(dblock.dbuf.uid, "%o", &i);
	sp->st_uid = i;
	sscanf(dblock.dbuf.gid, "%o", &i);
	sp->st_gid = i;
	sscanf(dblock.dbuf.size, "%lo", &sp->st_size);
	sscanf(dblock.dbuf.mtime, "%lo", &sp->st_mtime);
	sscanf(dblock.dbuf.chksum, "%o", &chksum);
	if (chksum != (i = checksum())) {
		fprintf(stderr, "tar: directory checksum error (%d != %d)\n",
		    chksum, i);
		if (iflag)
			goto top;
		done(2);
	}
	if (tfile != NULL)
		fprintf(tfile, "%s %s\n", dblock.dbuf.name, dblock.dbuf.mtime);
}

passtape()
{
	long blocks;
	char *bufp;

	if (dblock.dbuf.linkflag == '1')
		return;
	blocks = stbuf.st_size;
	blocks += TBLOCK-1;
	blocks /= TBLOCK;

	while (blocks-- > 0)
		(void) readtbuf(&bufp, TBLOCK);
}

putfile(longname, shortname, parent)
	char *longname;
	char *shortname;
	char *parent;
{
	int infile = 0;
	long blocks;
	char buf[TBLOCK];
	char *bigbuf;
	register char *cp;
	struct direct *dp;
	DIR *dirp;
	register int i;
	long l;
	char newparent[NAMSIZ+64];
	int	maxread;
	int	hint;		/* amount to write to get "in sync" */

	if (!hflag)
		i = lstat(shortname, &stbuf);
	else
		i = stat(shortname, &stbuf);
	if (i < 0) {
		vperror(0, longname);
		return;
	}
	if (tfile != NULL && checkupdate(longname) == 0)
		return;
	if (checkw('r', longname) == 0)
		return;
	if (Fflag && checkf(shortname, stbuf.st_mode, Fflag) == 0)
		return;
	if (Xflag) {
		if (is_in_table(exclude_tbl, longname)) {
			if (vflag) {
				fprintf(vfile, "a %s excluded\n", longname);
			}
			return;
		}
	}

	switch (stbuf.st_mode & S_IFMT) {
	case S_IFDIR:
		for (i = 0, cp = buf; *cp++ = longname[i++];)
			;
		*--cp = '/';
		*++cp = 0  ;
		if (!oflag) {
			if ((cp - buf) >= NAMSIZ) {
				fprintf(stderr, "tar: %s: file name too long\n",
				    longname);
				if (eflag)
				    done(1);
				return;
			}
			stbuf.st_size = 0;
			tomodes(&stbuf);
			strcpy(dblock.dbuf.name,buf);
			sprintf(dblock.dbuf.chksum, "%6o", checksum());
			(void) writetape((char *)&dblock);
		}
		if (*shortname != '/')
			sprintf(newparent, "%s/%s", parent, shortname);
		else
			sprintf(newparent, "%s", shortname);
		if (chdir(shortname) < 0) {
			vperror(0, newparent);
			return;
		}
		if ((dirp = opendir(".")) == NULL) {
			vperror(0, "can't open directory %s", longname);
			if (chdir(parent) < 0)
				vperror(0, "cannot change back?: %s", parent);
			return;
		}
		while ((dp = readdir(dirp)) != NULL && !term) {
			if (!strcmp(".", dp->d_name) ||
			    !strcmp("..", dp->d_name))
				continue;
			strcpy(cp, dp->d_name);
			if ((l = telldir(dirp)) < 0)
				break; /* bugfix 1068938 */
			closedir(dirp);
			putfile(buf, cp, newparent);
			dirp = opendir(".");
			seekdir(dirp, l);
		}
		closedir(dirp);
		if (chdir(parent) < 0)
			vperror(0, "cannot change back?: %s", parent);
		break;

	case S_IFLNK:
		tomodes(&stbuf);
		if (strlen(longname) >= NAMSIZ) {
			fprintf(stderr, "tar: %s: file name too long\n",
			    longname);
			if (eflag)
				done(1);
			return;
		}
		strcpy(dblock.dbuf.name, longname);
		if (stbuf.st_size + 1 >= NAMSIZ) {
			fprintf(stderr, "tar: %s: symbolic link too long\n",
			    longname);
			if (eflag)
				done(1);
			return;
		}
		i = readlink(shortname, dblock.dbuf.linkname, NAMSIZ - 1);
		if (i < 0) {
			vperror(0, "can't read symbolic link %s", longname);
			return;
		}
		dblock.dbuf.linkname[i] = '\0';
		dblock.dbuf.linkflag = '2';
		if (vflag)
			fprintf(vfile, "a %s symbolic link to %s\n",
			    longname, dblock.dbuf.linkname);
		sprintf(dblock.dbuf.size, "%11lo", 0);
		sprintf(dblock.dbuf.chksum, "%6o", checksum());
		(void) writetape((char *)&dblock);
		break;

	case S_IFREG:
		if ((infile = open(shortname, 0)) < 0) {
			vperror(0, longname);
			return;
		}
		tomodes(&stbuf);
		if (strlen(longname) >= NAMSIZ) {
			fprintf(stderr, "tar: %s: file name too long\n",
			    longname);
			close(infile);
			if (eflag)
				done(1);
			return;
		}
		strcpy(dblock.dbuf.name, longname);
		if (stbuf.st_nlink > 1) {
			struct linkbuf *lp;
			int found = 0;

			for (lp = ihead; lp != NULL; lp = lp->nextp)
				if (lp->inum == stbuf.st_ino &&
				    lp->devnum == stbuf.st_dev) {
					found++;
					break;
				}
			if (found) {
				strcpy(dblock.dbuf.linkname, lp->pathname);
				dblock.dbuf.linkflag = '1';
				sprintf(dblock.dbuf.chksum, "%6o", checksum());
				(void) writetape( (char *) &dblock);
				if (vflag)
					fprintf(vfile, "a %s link to %s\n",
					    longname, lp->pathname);
				lp->count--;
				close(infile);
				return;
			}
			lp = (struct linkbuf *) getmem(sizeof(*lp));
			if (lp != NULL) {
				lp->nextp = ihead;
				ihead = lp;
				lp->inum = stbuf.st_ino;
				lp->devnum = stbuf.st_dev;
				lp->count = stbuf.st_nlink - 1;
				strcpy(lp->pathname, longname);
			}
		}
		blocks = (stbuf.st_size + (TBLOCK-1)) / TBLOCK;
		if (vflag)
			fprintf(vfile, "a %s %ld blocks\n", longname, blocks);
		sprintf(dblock.dbuf.chksum, "%6o", checksum());
		hint = writetape((char *)&dblock);
		maxread = max(stbuf.st_blksize, (nblock * TBLOCK));
		if ((bigbuf = malloc((unsigned)maxread)) == 0) {
			maxread = TBLOCK;
			bigbuf = buf;
		}

		while ((i = read(infile, bigbuf, min((hint*TBLOCK), maxread))) > 0
		  && blocks > 0) {
		  	register int nblks;

			nblks = ((i-1)/TBLOCK)+1;
		  	if (nblks > blocks)
		  		nblks = blocks;
			hint = writetbuf(bigbuf, nblks);
			blocks -= nblks;
		}
		close(infile);
		if (bigbuf != buf)
			free(bigbuf);
		if (i < 0)
			vperror(0, "Read error on %s", longname);
		else if (blocks != 0 || i != 0) {
			fprintf(stderr, "tar: %s: file changed size\n",
			    longname);
			if (eflag)
				done(1);
		}
		while (--blocks >=  0)
			putempty();
		break;

	default:
		fprintf(stderr, "tar: %s is not a file. Not dumped\n",
		    longname);
		if (eflag)
			done(1);
		break;
	}
}

doxtract(argv)
	char *argv[];
{
	long blocks, sav_blocks, bytes;
	int ofile, i;

	for (;;) {
		if ((i = wantit(argv)) == 0)
			continue;
		if (i == -1)
			break;		/* end of tape */
		if (checkw('x', dblock.dbuf.name) == 0) {
			passtape();
			continue;
		}
		if (Fflag) {
			char *s;

			if ((s = rindex(dblock.dbuf.name, '/')) == 0)
				s = dblock.dbuf.name;
			else
				s++;
			if (checkf(s, stbuf.st_mode, Fflag) == 0) {
				passtape();
				continue;
			}
		}
		if (checkdir(dblock.dbuf.name)) {	/* have a directory */
			if (mflag == 0)
				dodirtimes(&dblock);
			continue;
		}
		if (dblock.dbuf.linkflag == '2') {	/* symlink */
			/*
			 * only unlink non directories or empty
			 * directories
			 */
			if (rmdir(dblock.dbuf.name) < 0) {
				if (errno == ENOTDIR)
					unlink(dblock.dbuf.name);
			}
			if (symlink(dblock.dbuf.linkname, dblock.dbuf.name)<0) {
				vperror(0, "%s: symbolic link failed",
				    dblock.dbuf.name);
				continue;
			}
			if (vflag)
				fprintf(vfile, "x %s symbolic link to %s\n",
				    dblock.dbuf.name, dblock.dbuf.linkname);
#ifdef notdef
			/* ignore alien orders */
			chown(dblock.dbuf.name, stbuf.st_uid, stbuf.st_gid);
			if (mflag == 0)
				setimes(dblock.dbuf.name, stbuf.st_mtime);
			if (pflag)
				chmod(dblock.dbuf.name, stbuf.st_mode & 07777);
#endif
			continue;
		}
		if (dblock.dbuf.linkflag == '1') {	/* regular link */
			/*
			 * only unlink non directories or empty
			 * directories
			 */
			if (rmdir(dblock.dbuf.name) < 0) {
				if (errno == ENOTDIR)
					unlink(dblock.dbuf.name);
			}
			if (link(dblock.dbuf.linkname, dblock.dbuf.name) < 0) {
				vperror(0, "can't link %s to %s",
				    dblock.dbuf.name, dblock.dbuf.linkname);
				continue;
			}
			if (vflag)
				fprintf(vfile, "%s linked to %s\n",
				    dblock.dbuf.name, dblock.dbuf.linkname);
			continue;
		}
		if ((ofile = creat(dblock.dbuf.name,stbuf.st_mode&0xfff)) < 0) {
			vperror(0, "can't create %s", dblock.dbuf.name);
			passtape();
			continue;
		}
		chown(dblock.dbuf.name, stbuf.st_uid, stbuf.st_gid);
		sav_blocks = blocks = ((bytes = stbuf.st_size) + TBLOCK-1)/TBLOCK;
		if (vflag) {
			fprintf(vfile, "x %s, %ld bytes, %ld tape blocks\n",
			    dblock.dbuf.name, bytes, blocks);
		}
		for (; blocks > 0;) {
			register int nread;
			char	*bufp;
			register int nwant;
			
			nwant = NBLOCK*TBLOCK;
			if (nwant > (blocks*TBLOCK))
				nwant = (blocks*TBLOCK);
			nread = readtbuf(&bufp, nwant);
			if (write(ofile, bufp, (int)min(nread, bytes)) < 0)
				vperror(2, "%s: HELP - extract write error",
				    dblock.dbuf.name);
			bytes -= nread;
			blocks -= (((nread-1)/TBLOCK)+1);
		}
		close(ofile);
		if (mflag == 0)
			setimes(dblock.dbuf.name, stbuf.st_mtime);
		if (pflag)
			chmod(dblock.dbuf.name, stbuf.st_mode & 07777);

	}
	if (mflag == 0) {
		dblock.dbuf.name[0] = '\0';	/* process the whole stack */
		dodirtimes(&dblock);
	}
	/* Read to EOF, if enabled */
	if (eof_flag == 0) {
		eofmsg = 0;
		for (; ;) {
			char	*bufp;

			if (readtbuf(&bufp, TBLOCK*nblock) <= 0)
				break;
		}
		eofmsg = 1;
	}
}

dotable(argv)
	char *argv[];
{
	register int i;

	for (;;) {
		if ((i = wantit(argv)) == 0)
			continue;
		if (i == -1)
			break;		/* end of tape */
		if (vflag)
			longt(&stbuf);
		printf("%s", dblock.dbuf.name);
		if (dblock.dbuf.linkflag == '1')
			printf(" linked to %s", dblock.dbuf.linkname);
		if (dblock.dbuf.linkflag == '2')
			printf(" symbolic link to %s", dblock.dbuf.linkname);
		printf("\n");
		passtape();
	}
	/* Read to EOF, if enabled */
	if (eof_flag == 0) {
		eofmsg = 0;
		for (; ;) {
			char	*bufp;

			if (readtbuf(&bufp, TBLOCK*nblock) <= 0) {
				break;
			}
		}
		eofmsg = 1;
	}
}

putempty()
{
	char buf[TBLOCK];

	bzero(buf, sizeof (buf));
	(void) writetape(buf);
}

longt(st)
	register struct stat *st;
{
	register char *cp;
	char *ctime();

	pmode(st);
	printf("%3d/%1d", st->st_uid, st->st_gid);
	printf("%7ld", st->st_size);
	cp = ctime(&st->st_mtime);
	printf(" %-12.12s %-4.4s ", cp+4, cp+20);
}

#define	SUID	04000
#define	SGID	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000
int	m1[] = { 1, ROWN, 'r', '-' };
int	m2[] = { 1, WOWN, 'w', '-' };
int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int	m4[] = { 1, RGRP, 'r', '-' };
int	m5[] = { 1, WGRP, 'w', '-' };
int	m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int	m7[] = { 1, ROTH, 'r', '-' };
int	m8[] = { 1, WOTH, 'w', '-' };
int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

pmode(st)
	register struct stat *st;
{
	register int **mp;

	for (mp = &m[0]; mp < &m[9];)
		selectbits(*mp++, st);
}

selectbits(pairp, st)
	int *pairp;
	struct stat *st;
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n>=0 && (st->st_mode&*ap++)==0)
		ap++;
	putchar(*ap);
}

/*
 * Make all directories needed by `name'.  If `name' is itself
 * a directory on the tar tape (indicated by a trailing '/'),
 * return 1; else 0.
 */
checkdir(name)
	register char *name;
{
	register char *cp;

	/*
	 * Quick check for existence of directory.
	 */
	if ((cp = rindex(name, '/')) == 0)
		return (0);
	*cp = '\0';
	if (access(name, 0) == 0) {	/* already exists */
		*cp = '/';
		return (cp[1] == '\0');	/* return (lastchar == '/') */
	}
	*cp = '/';

	/*
	 * No luck, try to make all directories in path.
	 */
	for (cp = name; *cp; cp++) {
		if (*cp != '/')
			continue;
		*cp = '\0';
		if (access(name, 0) < 0) {
			if (mkdir(name, 0777) < 0) {
				vperror(0, name);
				*cp = '/';
				return (0);
			}
			chown(name, stbuf.st_uid, stbuf.st_gid);
			if (pflag && cp[1] == '\0')	/* dir on the tape */
				chmod(name, stbuf.st_mode & 07777);
		}
		*cp = '/';
	}
	return (cp[-1]=='/');
}

void
onintr()
{
	(void) signal(SIGINT, SIG_IGN);
	term++;
}

void
onquit()
{
	(void) signal(SIGQUIT, SIG_IGN);
	term++;
}

void
onhup()
{
	(void) signal(SIGHUP, SIG_IGN);
	term++;
}

#ifdef notdef
void
onterm()
{
	(void) signal(SIGTERM, SIG_IGN);
	term++;
}
#endif

tomodes(sp)
register struct stat *sp;
{
	register char *cp;

	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		*cp = '\0';
	sprintf(dblock.dbuf.mode, "%6o ", sp->st_mode & 07777);
	sprintf(dblock.dbuf.uid, "%6o ", sp->st_uid);
	sprintf(dblock.dbuf.gid, "%6o ", sp->st_gid);
	sprintf(dblock.dbuf.size, "%11lo ", sp->st_size);
	sprintf(dblock.dbuf.mtime, "%11lo ", sp->st_mtime);
}

checksum()
{
	register i;
	register char *cp;

	for (cp = dblock.dbuf.chksum;
	     cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)]; cp++)
		*cp = ' ';
	i = 0;
	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		i += *cp;
	return (i);
}

checkw(c, name)
	char *name;
{
	if (!wflag)
		return (1);
	printf("%c ", c);
	if (vflag)
		longt(&stbuf);
	printf("%s: ", name);
	return (response() == 'y');
}

response()
{
	char c;

	c = getchar();
	if (c != '\n')
		while (getchar() != '\n')
			;
	else
		c = 'n';
	return (c);
}

checkf(name, mode, howmuch)
	char *name;
	int mode, howmuch;
{
	int l;

	if ((mode & S_IFMT) == S_IFDIR){
		if ((strcmp(name, "SCCS")==0) || (strcmp(name, "RCS")==0))
			return(0);
		return(1);
	}
	if ((l = strlen(name)) < 3)
		return (1);
	if (howmuch > 1 && name[l-2] == '.' && name[l-1] == 'o')
		return (0);
	if (strcmp(name, "core") == 0 ||
	    strcmp(name, "errs") == 0 ||
	    (howmuch > 1 && strcmp(name, "a.out") == 0))
		return (0);
	/* SHOULD CHECK IF IT IS EXECUTABLE */
	return (1);
}

/* Is the current file a new file, or the newest one of the same name? */
checkupdate(arg)
	char *arg;
{
	char name[100];
	long mtime;
	daddr_t seekp;
	daddr_t	lookup();

	rewind(tfile);
	for (;;) {
		if ((seekp = lookup(arg)) < 0)
			return (1);
		fseek(tfile, seekp, 0);
		fscanf(tfile, "%s %lo", name, &mtime);
		return (stbuf.st_mtime > mtime);
	}
}

done(n)
{
	unlink(tname);
	exit(n);
}

/*
 * Do we want the next entry on the tape, i.e. is it selected?  If
 * not, skip over the entire entry.  Return -1 if reached end of tape.
 */
wantit(argv)
	char *argv[];
{
	register char **cp;
	FILE *fp = (FILE *)NULL;

	getdir();
	if (endtape())
		return (-1);
	if (Iflag) {
		if (is_in_table(include_tbl, dblock.dbuf.name))
			goto gotit;
	} else
		if (*argv == 0)
			goto gotit;
	for (cp = argv; *cp; cp++)
		if (prefix(*cp, dblock.dbuf.name))
			goto gotit;
	passtape();
	return (0);

gotit:
	if (Xflag) {
		if (is_in_table(exclude_tbl, dblock.dbuf.name)) {
			if (vflag) {
				fprintf(stderr, "%s excluded\n",
					dblock.dbuf.name);
			}
			passtape();
			return (0);
		}
	}
	return (1);
}

/*
 * Does s2 begin with the string s1, on a directory boundary?
 */
prefix(s1, s2)
	register char *s1, *s2;
{
	while (*s1)
		if (*s1++ != *s2++)
			return (0);
	if (*s2)
		return (*s2 == '/');
	return (1);
}

#define	N	200
int	njab;

daddr_t
lookup(s)
	char *s;
{
	register i;
	daddr_t a;

	for(i=0; s[i]; i++)
		if (s[i] == ' ')
			break;
	a = bsrch(s, i, low, high);
	return (a);
}

daddr_t
bsrch(s, n, l, h)
	daddr_t l, h;
	char *s;
{
	register i, j;
	char b[N];
	daddr_t m, m1;

	njab = 0;

loop:
	if (l >= h)
		return (-1L);
	m = l + (h-l)/2 - N/2;
	if (m < l)
		m = l;
	fseek(tfile, m, 0);
	fread(b, 1, N, tfile);
	njab++;
	for(i=0; i<N; i++) {
		if (b[i] == '\n')
			break;
		m++;
	}
	if (m >= h)
		return (-1L);
	m1 = m;
	j = i;
	for(i++; i<N; i++) {
		m1++;
		if (b[i] == '\n')
			break;
	}
	i = cmp(b+j, s, n);
	if (i < 0) {
		h = m;
		goto loop;
	}
	if (i > 0) {
		l = m1;
		goto loop;
	}
	return (m);
}

cmp(b, s, n)
	char *b, *s;
{
	register i;

	if (b[0] != '\n')
		exit(2);
	for(i=0; i<n; i++) {
		if (b[i+1] > s[i])
			return (-1);
		if (b[i+1] < s[i])
			return (1);
	}
	return (b[i+1] == ' '? 0 : -1);
}

readtape(buffer)
	char *buffer;
{
	char *bufp;

	if (first == 0)
		getbuf();
	(void) readtbuf(&bufp, TBLOCK);
	bcopy(bufp, buffer, TBLOCK);
	return(TBLOCK);
}

readtbuf(bufpp, size)
	char **bufpp;
	int size;
{
	register int i;

	if (recno >= nblock || first == 0) {
		if ((i = bread(mt, (char *)tbuf, TBLOCK*nblock)) <= 0) {

			/* If reading to EOF, suppress error message */
			if (i == 0  &&  eofmsg == 0)
				return (0);	/* EOF */
			else
				mterr("read", i, 3);
		}
		if (first == 0) {
			if ((i % TBLOCK) != 0) {
				fprintf(stderr, "tar: tape blocksize error\n");
				done(3);
			}
			i /= TBLOCK;
			if (i != nblock) {
				fprintf(stderr, "tar: blocksize = %d\n", i);
				nblock = i;
			}
			first = 1;
		}
		recno = 0;
	}
	if (size > ((nblock-recno)*TBLOCK))
		size = (nblock-recno)*TBLOCK;
	*bufpp = (char *)&tbuf[recno];
	recno += (size/TBLOCK);
	return (size);
}

writetbuf(buffer, n)
	register char *buffer;
	register int n;
{
	register int i;

	if (first == 0) {
		getbuf();
		first = 1;
	}
	if (recno >= nblock) {
		i = write(mt, (char *)tbuf, TBLOCK*nblock);
		if (i != TBLOCK*nblock)
			mterr("write", i, 2);
		recno = 0;
	}

	/*
	 *  Special case:  We have an empty tape buffer, and the
	 *  users data size is >= the tape block size:  Avoid
	 *  the bcopy and dma direct to tape.  BIG WIN.  Add the
	 *  residual to the tape buffer.
	 */
	while (recno == 0 && n >= nblock) {
		i = write(mt, buffer, TBLOCK*nblock);
		if (i != TBLOCK*nblock)
			mterr("write", i, 2);
		n -= nblock;
		buffer += (nblock * TBLOCK);
	}
		
	while (n-- > 0) {
		bcopy(buffer, (char *)&tbuf[recno++], TBLOCK);
		buffer += TBLOCK;
		if (recno >= nblock) {
			i = write(mt, (char *)tbuf, TBLOCK*nblock);
			if (i != TBLOCK*nblock)
				mterr("write", i, 2);
			recno = 0;
		}
	}

	/* Tell the user how much to write to get in sync */
	return (nblock - recno);
}

backtape()
{
	static int mtdev = 1;
	static struct mtop mtop = {MTBSR, 1};
	struct mtget mtget;
	
	if (mtdev == 1)
		mtdev = ioctl(mt, MTIOCGET, (char *)&mtget);
	if (mtdev == 0) {
		if (ioctl(mt, MTIOCTOP, (char *)&mtop) < 0)
			vperror(4, "tape backspace error");
	} else
		lseek(mt, (long) -TBLOCK*nblock, 1);
	recno--;
}

flushtape()
{
	register int i;

	if (recno > 0) {
		i = write(mt, (char *)tbuf, TBLOCK*nblock);
		if (i != TBLOCK*nblock)
			mterr("write", i, 2);
	}
}

/* vperror() --variable argument perror.
 * Takes 3 args: exit_status, formats, args.  If exit status is 0, then
 * the eflag (exit on error) is checked -- if it is non-zero, tar exits
 * with the value of whatever "errno" is set to.  If exit_status is not
 * zero, then it exits with that error status. If eflag and exit_status
 * are both zero, the routine returns to where it was called.
 */
vperror(exit_status, fmt, args)
	register char *fmt;
{
	fputs("tar: ", stderr);
	vfprintf(stderr, fmt, &args);
	fprintf(stderr, ": %s\n", sys_errlist[errno]);
	if (exit_status)
		done(exit_status);
	else if (eflag)
		done(errno);
}

mterr(operation, i, exitcode)
	char *operation;
	int i;
{
	fprintf(stderr, "tar: %s error: ", operation);
	if (i < 0)
		perror("");
	else
		fprintf(stderr, "unexpected EOF\n");
	done(exitcode);
}

bread(fd, buf, size)
	int fd;
	char *buf;
	int size;
{
	int count;
#define	NOREAD -2147483648
	static int lastread = NOREAD;

	if (!Bflag)
		return (read(fd, buf, size));

	for (count = 0; count < size; count += lastread) {
		if (lastread <= 0 && lastread != NOREAD) {
			if (count > 0)
				return (count);
			return (lastread);
		}
		lastread = read(fd, buf, size - count);
		buf += lastread;
	}
	return (count);
#undef NOREAD
}

char *
getcwd(buf)
	char *buf;
{
	if (getwd(buf) == NULL) {
		fprintf(stderr, "tar: %s\n", buf);
		exit(1);
	}
	return (buf);
}

getbuf()
{
	
	if (nblock == 0) {
		fstat(mt, &stbuf);
		if ((stbuf.st_mode & S_IFMT) == S_IFCHR)
			nblock = NBLOCK;
		else {
			nblock = stbuf.st_blksize / TBLOCK;
			if (nblock == 0)
				nblock = NBLOCK;
		}
	}
	tbuf = (union hblock *)malloc((unsigned)nblock*TBLOCK);
	if (tbuf == NULL) {
		fprintf(stderr, "tar: blocksize %d too big, can't get memory\n",
		    nblock);
		done(1);
	}
}

/*
 * Following code handles excluded and included files.
 * A hash table of file names to be {in,ex}cluded is built.
 * For excluded files, before writing or extracting a file
 * check to see if it is in the exluce_tbl.
 * For included files, the wantit() procedure will check to
 * see if the named file is in the include_tbl.
 */
build_table(table, file)
struct file_list *table[];
char	*file;
{
	FILE	*fp;
	char	buf[512];

	if ((fp = fopen(file, "r")) == (FILE *)NULL)
		vperror(1, "could not open %s", file);
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		buf[strlen(buf) - 1] = '\0';
		add_file_to_table(table, buf);
	}
	fclose(fp);
}

/*
 * Add a file name to the the specified table, if the file name has any
 * trailing '/'s then delete them before inserting into the table
 */
add_file_to_table(table, str)
struct file_list *table[];
char	*str;
{
	char	name[NAMSIZ];
	unsigned int h;
	struct	file_list	*exp;

	strcpy(name, str);
	while (name[strlen(name) - 1] == '/') {
		name[strlen(name) - 1] = NULL;
	}

	h = hash(name);
	exp = (struct file_list *) malloc(sizeof(struct file_list));
	exp->name = strcpy(malloc(strlen(name)+1), name);
	exp->next = table[h];
	table[h] = exp;
}

/*
 * See if a file name or any of the file's parent directories is in the
 * specified table, if the file name has any trailing '/'s then delete
 * them before searching the table
 */
is_in_table(table, str)
struct file_list *table[];
char	*str;
{
	char	name[NAMSIZ];
	unsigned int	h;
	struct	file_list	*exp;
	char	*ptr;

	strcpy(name, str);
	while (name[strlen(name) - 1] == '/') {
		name[strlen(name) - 1] = NULL;
	}

	/*
	 * check for the file name in the passed list
	 */
	h = hash(name);
	exp = table[h];
	while (exp != NULL) {
		if (strcmp(name, exp->name) == 0) {
			return(1);
		}
		exp = exp->next;
	}

	/*
	 * check for any parent directories in the file list
	 */
	while (ptr = rindex(name, '/')) {
		*ptr = NULL;
		h = hash(name);
		exp = table[h];
		while (exp != NULL) {
			if (strcmp(name, exp->name) == 0) {
				return(1);
			}
			exp = exp->next;
		}
	}

	return(0);
}

/*
 * Compute a hash from a string.
 */
unsigned int
hash(str)
char	*str;
{
	char	*cp;
	unsigned int	h;

	h = 0;
	for (cp = str; *cp; cp++) {
		h += *cp;
	}
	return(h % TABLE_SIZE);
}

/*
 * Save this directory and its mtime on the stack, popping and setting
 * the mtimes of any stacked dirs which aren't parents of this one.
 * A null directory causes the entire stack to be unwound and set.
 *
 * Since all the elements of the directory "stack" share a common
 * prefix, we can make do with one string.  We keep only the current
 * directory path, with an associated array of mtime's, one for each
 * '/' in the path.  A negative mtime means no mtime.  The mtime's are
 * offset by one (first index 1, not 0) because calling this with a null
 * directory causes mtime[0] to be set.
 *
 * This stack algorithm is not guaranteed to work for tapes created
 * with the 'r' option, but the vast majority of tapes with
 * directories are not.  This avoids saving every directory record on
 * the tape and setting all the times at the end.
 */
char dirstack[NAMSIZ];
#define NTIM (NAMSIZ/2+1)		/* a/b/c/d/... */
time_t mtime[NTIM];

dodirtimes(hp)
	union hblock *hp;
{
	register char *p = dirstack;
	register char *q = hp->dbuf.name;
	register int ndir = 0;
	char *savp;
	int savndir;

	/* Find common prefix */
	while (*p == *q && *p) {
		if (*p++ == '/')
			++ndir;
		q++;
	}

	savp = p;
	savndir = ndir;
	while (*p) {
		/*
		 * Not a child: unwind the stack, setting the times.
		 * The order we do this doesn't matter, so we go "forward."
		 */
		if (*p++ == '/')
			if (mtime[++ndir] >= 0) {
				*--p = '\0';	/* zap the slash */
				setimes(dirstack, mtime[ndir]);
				*p++ = '/';
			}
	}
	p = savp;
	ndir = savndir;

	/* Push this one on the "stack" */
	while (*p = *q++)	/* append the rest of the new dir */
		if (*p++ == '/')
			mtime[++ndir] = -1;
	mtime[ndir] = stbuf.st_mtime;	/* overwrite the last one */
}

setimes(path, mt)
	char *path;
	time_t mt;
{
	struct timeval tv[2];

	tv[0].tv_sec = time((time_t *) 0);
	tv[1].tv_sec = mt;
	tv[0].tv_usec = tv[1].tv_usec = 0;
	if (utimes(path, tv) < 0)
		vperror(0, "can't set time on %s", path);
}

char *
getmem(size)
{
	char *p = malloc((unsigned) size);

	if (p == NULL && freemem) {
		fprintf(stderr,
		    "tar: out of memory, link and directory modtime info lost\n");
		freemem = 0;
		if (eflag)
			done(1);
	}
	return (p);
}
