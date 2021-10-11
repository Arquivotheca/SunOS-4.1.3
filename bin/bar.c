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

/* bar was derived from tar.c */
#ifndef lint
static  char sccsid[] = "@(#)bar.c 1.1 92/07/30 SMI";
#endif not lint


/*
 * bar: Backup Archival Program
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/dir.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#ifdef i386	/* was sun386 */
#include <bool.h>
#else
#ifndef TRUE
#define	TRUE 1
#endif
#define	FALSE 0
#endif

#define	VPIX
#include <sys/ioccom.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#ifdef i386		/* really sun386 - F_RAW ioctls */
#include <sundev/fdreg.h>
#endif i386

#define	TBLOCK	512
#define	NBLOCK	20
#define	VOLUME_MAGIC	'V'
#define	PTR_SIZE 4
#define	WRITING 1
#define	READING 0
#define	NO_CONT 1
#define	CONT 0
#define	FLOPPY_BLKSIZE 18
#define	TAPE_BLKSIZE   126

#define	writetape(b)	writetbuf(b, 1)
#define	min(a, b)  ((a) < (b) ? (a) : (b))
#define	max(a, b)  ((a) > (b) ? (a) : (b))

/* The bar file header */
union hblock {
	char dummy[TBLOCK];
	struct header {
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];
		char mtime[12];
		char chksum[8];
		char rdev[8];
		char linkflag;

		/* The following fields are specific to the volume  */
		/* header.  They are set to zero in all file headers*/
		/* in the archive.				    */
		char bar_magic[2];	/* magic number */
		char volume_num[4];	/* volume number */
		char compressed;	/* files compressed = 1*/
		char date[12];		/* date of archive mmddhhmm */
		char start_of_name;	/* start of the filename */
	} dbuf;
};

#ifdef DEBUG
int  bytes_written;
int  bytes_read;
#endif DEBUG

int  files_found;
int  files_wanted;

int  current_total = 0;
int  current_written = 0;
int  blks_to_skip = 0;

char last_file[MAXPATHLEN];
char filename[MAXPATHLEN];
char Vfilename[MAXPATHLEN];
char linkname[MAXPATHLEN];

struct linkbuf {
	ino_t	inum;
	dev_t	devnum;
	int	count;
	struct	linkbuf *nextp;
	char	pathname[MAXPATHLEN];
};

union	hblock dblock;
union	hblock vblock;
union	hblock oldvblock;
union	hblock tmpblock;
union	hblock *tbuf;
union	hblock *Vheader;
struct	linkbuf *ihead;
struct	stat stbuf;
struct	stat tmpstbuf;

int	volume_num = 1;

int	rflag;		/* Add to end of archive */
int	xflag;		/* extract from achive */
int	Xflag;		/* exclude files */
int	vflag;		/* verbose */
int	tflag;		/* list table of contents */
int	cflag;		/* create archive */
int	mflag;		/* keep mod times */
int	fflag;		/* specify archive file */
int	iflag;		/* Ignore checksum errors */
int	oflag;		/* Suppress owner info for dirs */
int	pflag;		/* Keep modes */
int	wflag;		/* Wait for confirmation */
int	hflag;		/* Follow all links */
int	Lflag;		/* Follow just directory links */
int	Bflag;		/* Perform multiple reads */
int	Fflag;		/* Exclude SCCS and .o's */
int	Iflag;		/* Get file names from file, rather than command line */
int	Sflag;		/* Substitute new pathname when extracting file */
int	Uflag;		/* UID for volume header */
int	Gflag;		/* GID for volume header */
int	Oflag;		/* On extract, make sure volume is owned by user */
int	sflag;		/* on extract, make uid/gid = this process's UID/GID */
int	Zflag;		/* Compress files before archiving, uncompress */
int	Rflag;		/* Read bar header */
int	Mflag;		/* Modify time for incrementals */
int	Dflag;		/* Force date to be str */
int	Nflag;		/* Check media for ownership */
int	Tflag;		/* On "x" or "t" stop after all specified files found */
int	Pflag;		/* Prompt for volume change */
int	Vflag;		/* Fake volume number for volume change prompt */

int	dflag;		/* Copies output to a disk file as well as to tape */
int	mt1;		/* file descriptor for disk file */
char	*debugoutput;	/* disk file name */
int	mt;		/* file descriptor for output file */
int	term;
int	chksum;
int	recno;
int	first;
int	prtlinkerr;
int	freemem = 1;
int	nblock = 0;
int	onintr();
int	onquit();
int	onhup();
extern	long atol();
#ifdef notdef
int	onterm();
#endif
unsigned int hash();

daddr_t	low;
daddr_t	high;
daddr_t	bsrch();

FILE	*vfile = stdout;
FILE	*tfile;
long	modtime;
char 	*prompt;
int	fakeVol;
char	tname[] = "/tmp/barXXXXXX";
char	*usefile;
char	user_id[16];
char	group_id[16];
char	*userid;
int	new_uid;
char	*groupid;
int	new_gid;
char	*ownerid;
char	*date;
char	*bar_id;
char	barid[] = "";
char	*newPath;
char	*oldPath;
char	**Ilist;
char	magtape[] = "/dev/rmt8";
char	*malloc();
long	time();
long	lseek();
char	*mktemp();
char	*sprintf();
char	*strcat();
char	*strcpy();
char	*rindex();
FILE	*fdopen();
char	*index();
char	*getcwd();
char	*getwd();
char	*getmem();
char	*getenv();
char	**build_list();
char	*nullstring = "";

main(argc, argv)
int	argc;
char	*argv[];
{
	char *cp, *ptr;
	struct tm time_data;
	struct tm *time_ptr;
	struct timeval tp;
	struct timezone tzp;
	char	year[10], month[10], day[10], hour[10], minute[10];
	char	newPrompt[MAXPATHLEN];


	if (argc < 2)
		usage();

	tfile = NULL;
	bar_id = barid;
	usefile =  magtape;
	argv[argc] = 0;
	argv++;
	for (cp = *argv++; *cp; cp++)
		switch (*cp) {

		case 'f':
			if (*argv == 0) {
				fprintf(stderr,
		"bar error: tapefile must be specified with 'f' option\n");
				done(1);
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
			"bar error: cannot create temporary file (%s)\n",
				    tname);
				done(1);
			}
			fprintf(tfile, "!!!!!/!/!/!/!/!/!/! 000\n");
			/* FALL THRU */

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
		"bar error: exclude file must be specified with 'X' option\n");
				usage();
			}
			Xflag = 1;
			build_exclude(*argv++);
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
		"bar error: blocksize must be specified with 'b' option\n");
				usage();
			}
			nblock = atoi(*argv);
			if (nblock <= 0) {
				fprintf(stderr,
				    "bar error: invalid blocksize \"%s\"\n",
				    *argv);
				done(1);
			}
			argv++;
			break;

		case 'l':
			prtlinkerr++;
			break;

		case 'L':
			Lflag++;
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

		/* Write argument in header on each volume */
		case 'H':
			if (*argv == 0) {
				fprintf(stderr,
		"bar error: bar ID  must be specified with 'H' option\n");
				usage();
			}
			bar_id = *argv++;
			break;

		/* Substitute this new pathname into the pathname of files */
		/* when extracing (with the x flag)			   */
		case 'S':
			if (*argv == 0) {
				fprintf(stderr,
	"bar error: old and new pathnames must be specified with 'S' option\n");
				usage();
			}
			oldPath = *argv++;
			if (*argv == 0) {
				fprintf(stderr,
	"bar error: old and new pathnames must be specified with 'S' option\n");
				usage();
			}
			newPath = *argv++;
			Sflag++;
			break;

		/* Use the userid given in the volume header */
		case 'U':
			if (*argv == 0) {
				fprintf(stderr,
			"bar error: UID must be specified with 'U' option\n");
				usage();
			}
			userid = *argv++;
			new_uid = atoi(userid);
			Uflag++;
			break;

		/* Use the group id given in the volume header */
		case 'G':
			if (*argv == 0) {
				fprintf(stderr,
			"bar error: GID must be specified with 'G' option\n");
				usage();
			}
			groupid = *argv++;
			new_gid = atoi(groupid);
			Gflag++;
			break;

		/* On extract, ensure archive is owned by uid on command line */
		case 'O':
			if (*argv == 0) {
				fprintf(stderr,
		"bar error: owner's UID must be specified with 'O' option\n");
				usage();
			}
			ownerid = *argv++;
			Oflag++;
			break;

		/* On extract or table, stop after all specified files found */
		case 'T':
			Tflag++;
			break;

		/* On extract, make files owned by uid and gid specified */
		/* on command line or of current process */
		case 's':
			sflag++;
			break;

		/* Read bar header and return */
		case 'R':
			Rflag++;
			break;

		/* Check media for ownership before writing */
		case 'N':
			Nflag++;
			break;

		/* Check the modify time of each file, don't archive if */
		/* file has not changed since modtime.			*/
		case 'M':
			if (*argv == 0) {
				fprintf(stderr,
		"bar error: modtime must be specified with 'M' option\n");
				usage();
			}
			modtime = atol(*argv++);
			Mflag++;
			break;

		case 'D':
			if (*argv == 0) {
				fprintf(stderr,
			"bar error: date must be specified with 'D' option\n");
				usage();
			}
			date = *argv++;
			Dflag++;
			break;

		/* Compress/ uncompress files */
		case 'Z':
			Zflag++;
			break;

		/* Get file names from file rather than command line */
		case 'I':
			if (*argv == 0) {
				fprintf(stderr,
		"bar error: filename file must be specified with 'I' option\n");
				usage();
			}
			Ilist = build_list(*argv++);
			Iflag++;
			break;

		case 'd':
			if (*argv == 0) {
				fprintf(stderr,
		"bar error: disk filename must be specified with 'd' option\n");
				usage();
			}
			debugoutput = *argv++;
			dflag++;
			break;

		case 'P':
			if (*argv == 0) {
				fprintf(stderr,
			"bar error: prompt must be specified with 'P' option\n");
				usage();
			}
			prompt = *argv++;
			Pflag++;
			break;

		case 'V':
			if (*argv == 0) {
				fprintf(stderr,
		"bar error: volume number must be specified with 'V' option\n");
				usage();
			}
			fakeVol = atoi(*argv++);
			Vflag++;
			break;

		default:
			fprintf(stderr, "bar error: %c: unknown option\n", *cp);
			usage();
		}

	if (!Uflag) {

		/* Get the user's ID because it was not */
		/* specified on command line */
		userid = user_id;
		sprintf(userid, "%d", getuid());
		new_uid = atoi(userid);
	}

	if (!Gflag) {

		/* Get the user's groupo ID because it was not */
		/* specified on command line */
		groupid = group_id;
		sprintf(groupid, "%d", getgid());
		new_gid = atoi(groupid);
	}

	if (!Dflag) {

		/* Get the current time, it was */
		/* not specified with D option  */

		time_ptr = &time_data;

		/* Get time of day */
		if (gettimeofday(&tp, &tzp) == -1) {
			fprintf(stderr, "bar error: error in gettimeofday ");
			perror(nullstring);
			done(-1);
		}
		time_ptr = localtime(&tp.tv_sec);

		date = malloc(12);	/*XXX*/
		sprintf(date, "%02d%02d%02d%02d%02d", time_ptr->tm_year,
		    time_ptr->tm_mon+1, time_ptr->tm_mday,
		    time_ptr->tm_hour, time_ptr->tm_min);
	}

	if (!rflag && !xflag && !tflag && !Rflag)
		usage();
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
		if (dflag)
			mt1 = open(debugoutput, O_RDWR|O_CREAT|O_TRUNC, 0666);
		mt = openmt(usefile, WRITING, NO_CONT);

		/* Set the environment variable AUTOMOUNT_FIXNAMES    */
		/* so that our pathnames are resolved into real names */
		if (putenv("AUTOMOUNT_FIXNAMES=true") != 0) {
			fprintf(stderr,
	"bar error: cannot set environment variable AUTOMOUNT_FIXNAMES: ");
			perror(nullstring);
			done(-1);
		}
		if (Iflag)
			dorep(Ilist);
		else
			dorep(argv);
		done(0);
	}
	/* If the archive is not in bar format, prompt the user */
	while ((mt = openmt(usefile, READING, NO_CONT)) == -2) {
		if (!Pflag)
			sprintf(newPrompt,
	"bar: Bad format. Insert volume %d and press Return when ready.",
			    volume_num);
		else {
			if (Vflag)
				sprintf(newPrompt, prompt, fakeVol);
			else
				sprintf(newPrompt, prompt, volume_num);
		}
		promptUser(newPrompt);
	}

	if (Rflag) {
		/* Parse the date */
		ptr = Vheader->dbuf.date;
		strcpy(year, ptr);
		year[2] = '\0';
		strcpy(month, ptr+2);
		month[2] = '\0';
		strcpy(day, ptr+4);
		day[2] = '\0';
		strcpy(hour, ptr+6);
		hour[2] = '\0';
		strcpy(minute, ptr+8);
		minute[2] = '\0';
		fprintf(vfile,
"bar: Archive ID = %s; date = %s/%s/%s; time = %s:%s; volume number = %s;\n",
		    Vfilename, month, day, year, hour, minute,
		    Vheader->dbuf.volume_num);
		done(0);
	}

	if (xflag) {
		if (Iflag)
			doxtract(Ilist);
		else
			doxtract(argv);
	} else {
		if (Iflag)
			dotable(Ilist);
		else
			dotable(argv);
	}
	done(0);
}

usage()
{
	fprintf(stderr,
"bar: usage: bar -{txruR}[cvfblmhopwBisIHUZMSD] [tapefile] [blocksize]\n [includefile] [archive ID] [UID] [modtime] [new directory] [date] file1 file2...\n");
	done(1);
}

int
openmt(tape, writing, continuation)
	char *tape;
	int writing, continuation;
{
	int	size, blocks, tmp[10];
	int	is_floppy_flag = 0;

	fflush(stdout);
	if (strcmp(tape, "-") == 0) {
		/*
		 * Read from standard input or write to standard output.
		 */
		if (writing) {
			if (cflag == 0) {
				fprintf(stderr,
		"bar error: can only create standard output archives\n");
				done(1);
			}
			vfile = stderr;
			setlinebuf(vfile);
			mt = dup(1);

			/* Write the bar volume header*/
			if (cflag) {
				vblock.dbuf.bar_magic[0] = VOLUME_MAGIC;
				strcpy(filename, bar_id);
				strcpy(&vblock.dbuf.start_of_name, filename);
				sprintf(vblock.dbuf.volume_num, "%d",
				    volume_num);
				sprintf(vblock.dbuf.date, "%s", date);
				sprintf(vblock.dbuf.uid, "%s", userid);
				sprintf(vblock.dbuf.gid, "%s", groupid);
				sprintf(vblock.dbuf.size, "%lo", 0);
				if (Zflag)
					vblock.dbuf.compressed = '1';
				else
					vblock.dbuf.compressed = '0';
				sprintf(vblock.dbuf.chksum, "%6o",
				    header_checksum());
				current_total = 1;
				writetbuf((char *) &vblock, TBLOCK/TBLOCK);
			}
		} else {
			mt = dup(0);
			Bflag++;

			/* Read the volume header */
			readtape((char *) &vblock);

			/* If a volume header exists, save a copy of it*/
			if (vblock.dbuf.bar_magic[0] == VOLUME_MAGIC) {

				if (Oflag && strcmp(ownerid, vblock.dbuf.uid)) {
					fprintf(stderr,
				"bar error: user does not own this archive\n");
					done(-1);
				}

				/* Save a copy of the volume header */
				Vheader = (union hblock *)malloc(TBLOCK);
				if (Vheader == NULL) {
					fprintf(stderr,
				"bar error: cannot malloc for volume header: ");
					perror(nullstring);
					done(-1);
				}
				strcpy(Vfilename, &vblock.dbuf.start_of_name);
				bcopy(&(vblock.dbuf), &(Vheader->dbuf), TBLOCK);
			} else {
				/* No header, cannot read */
				fprintf(stderr,
				    "bar error: cannot read volume header\n");
				done(-1);
			}
		}
	} else {
		/*
		 * Use file or tape on local machine.
		 */
		if (writing) {

			/* If there is a volume header, read it and */
			/* check to see if we own this media */
			if (Nflag) {
				/* Remember where we were */
				/* use NDELAY so we don't touch floppy yet */
				mt = open(tape, O_RDONLY|O_NDELAY);
				if (mt < 0) {
					if (errno == EBUSY)
						fprintf(stderr,
					"bar error: device %s is busy\n",
						    tape);
					else
						fprintf(stderr,
					"bar error: cannot access %s\n",
						    tape);
					done(-1);
				}

				/* If this is the floppy drive, check to see */
				/* if there is a floppy in the drive. */
				if (is_floppy(tape, mt)) {
					nblock = FLOPPY_BLKSIZE;
					(void)check_for_floppy(tape, mt, 1);
					/* have diskette, close/open to sense */
					(void)close(mt);
					mt = open(tape, O_RDONLY);
				} else if (is_tape(tape, mt)) {
					nblock = TAPE_BLKSIZE;
				}
/*
				readtape((char *) &vblock);
*/
/* XXX FIX ME FIXME FIX THIS unprotected read, & why wasn't readtape used? */
				read(mt, &vblock, TBLOCK);

				/* See if a volume header exists */
				if (vblock.dbuf.bar_magic[0] == VOLUME_MAGIC) {

					/* See if user ids match */
					if (strcmp(vblock.dbuf.uid, userid) != 0) {
						fprintf(stderr,
				"bar error: cannot write media, not owner.\n");
						done(1);
					}
				}
				/* Closing/reopening does an effective rewind */
				close(mt);
			}

			/* use NDELAY so we don't touch floppy yet */
			if (cflag)
				mt = open(tape, O_RDWR|O_CREAT|O_TRUNC|O_NDELAY,
				    0666);
			else
				mt = open(tape, O_RDWR|O_NDELAY);

			if (mt < 0) {
				if (errno == EBUSY)
					fprintf(stderr,
					    "bar error: device %s is busy\n",
					    tape);
				else
					fprintf(stderr,
					    "bar error: cannot access %s\n",
					    tape);
				done(-1);
			}

			/* If this is the floppy drive, check to see */
			/* if there is a floppy in the drive. */
			if (is_floppy(tape, mt)) {
				nblock = FLOPPY_BLKSIZE;
				(void)check_for_floppy(tape, mt, 1);
				/* have diskette, close/open to sense */
				(void)close(mt);
				if (cflag)
					mt = open(tape, O_RDWR|O_CREAT|O_TRUNC,
					    0666);
				else
					mt = open(tape, O_RDWR);
			} else if (is_tape(tape, mt)) {
				nblock = TAPE_BLKSIZE;
			}

			/* Write the bar volume header*/
			if (cflag) {
				vblock.dbuf.bar_magic[0] = VOLUME_MAGIC;
				strcpy(filename, bar_id);
				strcpy(&vblock.dbuf.start_of_name, filename);
				sprintf(vblock.dbuf.volume_num, "%d",
				    volume_num);
				sprintf(vblock.dbuf.date, "%s", date);
				sprintf(vblock.dbuf.uid, "%s", userid);
				sprintf(vblock.dbuf.gid, "%s", groupid);
				sprintf(vblock.dbuf.size, "%lo", 0);
				if (Zflag)
					vblock.dbuf.compressed = '1';
				else
					vblock.dbuf.compressed = '0';
				sprintf(vblock.dbuf.chksum, "%6o",
				    header_checksum());
				if (first == 0) {
					current_total = 1;
					writetbuf((char *) &vblock,
					    TBLOCK/TBLOCK);
				}
			}

		} else {
			/* READING */

			/* use NDELAY so we don't touch floppy yet */
			if ((mt = open(tape, O_RDONLY|O_NDELAY)) < 0) {
				if (errno == EBUSY)
					fprintf(stderr,
					    "bar error: device %s is busy\n",
					    tape);
				else
					fprintf(stderr,
					    "bar error: cannot access %s\n",
					    tape);
				done(-1);
			}

			/* If this is the floppy drive, check to see */
			/* if there is a floppy in the drive. */
			if (is_floppy(tape, mt)) {
				nblock = FLOPPY_BLKSIZE;
				(void)check_for_floppy(tape, mt, 1);
				is_floppy_flag++;
				/* have diskette, close/open to sense */
				(void)close(mt);
				mt = open(tape, O_RDONLY);
			} else if (is_tape(tape, mt)) {
				nblock = TAPE_BLKSIZE;
			}


			/* Read the volume header */
			/* If we are on not on the first volume, */
			/* save the previous vol header */
			if (!continuation) {
				bcopy(&vblock, &oldvblock, TBLOCK);
			}
			readtape((char *) &vblock);

			/* If a volume header exists, save a copy of it */
			if (vblock.dbuf.bar_magic[0] == VOLUME_MAGIC) {

				/* If this is not the first volume, */
				/* check to see if it is the next   */
				/* consecutive volume.		    */
				if (!continuation) {
					sprintf(tmp, "%d", volume_num);
					if(strcmp(vblock.dbuf.volume_num,tmp) ||
					    strcmp(&vblock.dbuf.start_of_name, &oldvblock.dbuf.start_of_name) ||
				    	    strcmp(vblock.dbuf.date, oldvblock.dbuf.date)) {
						/* Copy back last vol header */
						bcopy(&oldvblock, &vblock,
						    TBLOCK);
						if (is_floppy_flag)
							/* eject the floppy */
							(void)ioctl(mt,
							    FDKEJECT, NULL);
						close(mt);
						return (-1);
					}
				}

				/* This is the first volume, see who owns it */
				else {
					if (Oflag &&
					     strcmp(ownerid, vblock.dbuf.uid)) {
						fprintf(stderr,
				"bar error: user does not own this archive\n");
						done(-1);
					}
				}

				/* Save a copy of the volume header */
				if (Vheader == NULL) {
					Vheader =
					    (union hblock *)malloc(TBLOCK);
					if(Vheader == NULL) {
						fprintf(stderr,
				"bar error: cannot malloc for volume header ");
						perror(nullstring);
						done(-1);
					}
				}
				strcpy(Vfilename, &vblock.dbuf.start_of_name);

				bcopy(&(vblock.dbuf), &(Vheader->dbuf), TBLOCK);

				/* If this a continuation volume */
				/* return, else throw away the   */
				/* blocks from this first file.  */
				if (continuation) {
					sscanf(vblock.dbuf.volume_num, "%d",
					    &volume_num);
					sscanf(vblock.dbuf.size, "%lo", &size);
					blocks = (size+(TBLOCK-1))/TBLOCK;

					while (blocks--)
						readtape(&tmpblock);
				}
			} else {		/* No header, cannot read */
				bcopy(&oldvblock, &vblock, TBLOCK);
				if (is_floppy_flag)
					(void) ioctl(mt, FDKEJECT, NULL);
				close(mt);
				recno = nblock;
				return (-2);
			}
		}
	}
	return (mt);
}

dorep(argv)
	char *argv[];
{
	register char *cp, *cp2;
	char wdir[MAXPATHLEN], tempdir[MAXPATHLEN], *parent;

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

	/* Get filenames from command line */
	while (*argv && ! term) {
		cp2 = *argv;
		if (!strcmp(cp2, "-C") && argv[1]) {
			argv++;
			if (chdir(*argv) < 0) {
				fprintf(stderr,
				    "bar warning: can't change directory to ");
				perror(*argv);
			} else
				(void) getcwd(wdir);
			argv++;
			continue;
		}
		parent = wdir;
		for (cp = *argv; *cp; cp++)
			if (*cp == '/')
				cp2 = cp;
		if (cp2 != *argv) {
			*cp2 = '\0';
			if (chdir(*argv) < 0) {
				fprintf(stderr,
				    "bar warning: can't change directory to ");
				perror(*argv);
				continue;
			}
			parent = getcwd(tempdir);
			*cp2 = '/';
			cp2++;
		}
		putfile(*argv++, cp2, parent, FALSE);
		if (chdir(wdir) < 0) {
			fprintf(stderr, "bar warning: cannot change back?: ");
			perror(wdir);
		}
	}
	putempty();
	putempty();
	flushtape();
	if (prtlinkerr == 0)
		return;
	for (; ihead != NULL; ihead = ihead->nextp) {
		if (ihead->count == 0)
			continue;
		fprintf(stderr, "bar warning: missing links to %s\n",
		    ihead->pathname);
	}
}


endtape()
{
	return (filename[0] == '\0');
}

#define	FILEN 0
#define	LINKN 1
#define	DONEN 2

int readfilenames(buffer, length, pass)
char *buffer;
int   length;
int   *pass;
{
	int end, mallocflag=0;
	char *ptr, *bufptr;

	/* See if we have the complete filename in buffer */
	ptr = buffer;
	bufptr = buffer;
	end = length;
	while (end) {
		if (*ptr++ == '\0')
			break;
		end--;
	}

	/* If we found the whole filename */
	if (end) {
		if (*pass == FILEN) {
			strcpy(filename, bufptr);
			*pass = LINKN;
			bufptr += strlen(filename)+1;
		} else {
			strcpy(linkname, bufptr);
			*pass = DONEN;
			bufptr += strlen(filename)+1;
		}
	}

	/* Did not get the whole filename, read the next block */
	else {
		if (*pass == FILEN)
			strncpy(filename, bufptr, length);
		else
			strncpy(linkname, bufptr, length);

		/* Read the next block to get the rest of the name */
		bufptr = malloc(TBLOCK);
		mallocflag = 1;

		readtape(bufptr);
		end = readfilenames(bufptr, TBLOCK, pass);

	}
	if (*pass == LINKN) {
		end = readfilenames(bufptr, end - 1, pass);
	}
	if (mallocflag)
		free(bufptr);
	return (end-1);
}

int writefilenames(buffer, length, pass, chksumflag, hint)
char *buffer;
int   length;
int   *pass;
int   *chksumflag;
int   *hint;
{
	int end, mallocflag=0;
	char *ptr, *bufptr;

	/* See if we have the complete filename in buffer */
	ptr = buffer;
	bufptr = buffer;
	end = length;

	/* If we have room in this block for the whole filename*/
	if (*pass == FILEN && length >= strlen(filename+1)) {
		strcpy(ptr, filename);
		*pass = LINKN;
		end -= strlen(filename)+1;
		ptr += strlen(filename)+1;
	} else if (*pass == LINKN && length >= strlen(linkname+1)) {
		strcpy(ptr, linkname);
		*pass = DONEN;
		end -= strlen(filename)+1;
		ptr += strlen(filename)+1;
	}
	/* Cannot fit the whole filename into this block */
	else {
		if (*pass == FILEN)
			strncpy(ptr, filename, length);
		else
			strncpy(ptr, linkname, length);

		/* Write this block */
		if (chksumflag) {
			sprintf(dblock.dbuf.chksum, "%6o", checksum());
			chksumflag = 0;
			(void) writetape((char *)&dblock);
		} else
			(void) writetape(bufptr);

		/* Get a block for the rest of the filename */
		bufptr = ptr = malloc(TBLOCK);
		mallocflag = 1;
		end = writefilenames(ptr, TBLOCK, pass, chksumflag, hint);

	}
	if (*pass == LINKN) {
		end = writefilenames(ptr, end - 1, pass, chksumflag, hint);
		if (chksumflag) {
			sprintf(dblock.dbuf.chksum, "%6o", checksum());
			chksumflag = 0;
			*hint = writetape((char *)&dblock);
		} else
			*hint = writetape(bufptr);

	}
	if (mallocflag)
		free(bufptr);
	return (end-1);
}


getdir()
{
	register struct stat *sp;
	int i, charsleft, pass;

top:
	readtape((char *)&dblock);
	charsleft = TBLOCK - (&dblock.dbuf.start_of_name -
		&dblock.dbuf.mode[0]);
	pass = FILEN;
	charsleft = readfilenames(&dblock.dbuf.start_of_name, charsleft,
	    &pass);
	if (filename[0] == '\0')
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
		fprintf(stderr,
		    "bar error: directory checksum error (%d != %d)\n",
		    chksum, i);
#ifdef DEBUG
		fprintf(stderr, "bytes_read = %d\n", bytes_read);
#endif DEBUG
		if (iflag) {
			goto top;
		} else {
			done(2);
		}
	}
	if (tfile != NULL)
		fprintf(tfile, "%s %s\n", filename, dblock.dbuf.mtime);
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

putfile(longname, shortname, parent, followlink)
	char *longname;
	char *shortname;
	char *parent;
	int   followlink;
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
	char newparent[MAXPATHLEN+64];
	extern int errno;
	int	maxread;
	int	hint;		/* amount to write to get "in sync" */
	int pid, charsleft, pass, flag;
	char *tmp_buf, tmp_file[40], *ptr;
	char  newLongname[MAXPATHLEN], newShortname[MAXPATHLEN];
	struct statfs statfsbuf;

	if (hflag || followlink == TRUE)
		i = stat(shortname, &stbuf);
	else
		i = lstat(shortname, &stbuf);
	if (i < 0) {
		return;
	}
	if (tfile != NULL && checkupdate(longname) == 0)
		return;
	if (checkw('r', longname) == 0)
		return;
	if (Fflag && checkf(shortname, stbuf.st_mode, Fflag) == 0)
		return;
	if (Xflag) {
		if (excluded(longname)) {
			if (vflag) {
				fprintf(vfile, "a %s excluded %d\n",
				    longname, volume_num);
				fflush(vfile);
			}
			return;
		}
	}

	switch (stbuf.st_mode & S_IFMT) {
	case S_IFDIR:
		for (i = 0, cp = buf; *cp++ = longname[i++]; )
			;
		*--cp = '/';
		*++cp = 0;
		if (!oflag && followlink == FALSE) {
			stbuf.st_size = 0;
			tomodes(&stbuf);
			strcpy(filename, buf);

			/* Compute the total number of blocks this file */
			/* will take up on the tape. */
			current_total =
			    (&dblock.dbuf.start_of_name - &dblock.dbuf.mode[0] +
			    strlen(filename) + 1 + strlen(linkname) + 1 +
			    TBLOCK) / TBLOCK;

			charsleft = TBLOCK - (&dblock.dbuf.start_of_name -
				&dblock.dbuf.mode[0]);
			pass = FILEN;
			flag = 1;
			charsleft = writefilenames(&dblock.dbuf.start_of_name,
				charsleft, &pass, &flag, &hint);
		}
		if (*shortname != '/')
			sprintf(newparent, "%s/%s", parent, shortname);
		else
			sprintf(newparent, "%s", shortname);
		if (chdir(shortname) < 0) {
			fprintf(stderr, "bar warning: ");
			perror(newparent);
			return;
		}
		if (vflag) {
			if (followlink == FALSE) {
				fprintf(vfile, "a %s/ directory %d\n",
				    longname, volume_num);
				fflush(vfile);
			}
		}
		if ((dirp = opendir(".")) == NULL) {
			fprintf(stderr, "bar warning: can't open directory ");
			perror(longname);
			if (chdir(parent) < 0) {
				fprintf(stderr,
				    "bar warning: cannot change back?: ");
				perror(parent);
			}
			return;
		}
		while ((dp = readdir(dirp)) != NULL && !term) {
			if (!strcmp(".", dp->d_name) ||
			    !strcmp("..", dp->d_name))
				continue;
			strcpy(cp, dp->d_name);
			l = telldir(dirp);
			closedir(dirp);
			putfile(buf, cp, newparent, FALSE);
			dirp = opendir(".");
			seekdir(dirp, l);
		}
		closedir(dirp);
		if (chdir(parent) < 0) {
			fprintf(stderr, "bar warning: cannot change back?: ");
			perror(parent);
		}
		break;

	case S_IFLNK:
		tomodes(&stbuf);

		/* Copy filename to header */
		strcpy(filename, longname);

		/* Copy linkname to header */
		i = readlink(shortname, linkname, MAXPATHLEN - 1);
		if (i < 0) {
			fprintf(stderr,
			    "bar warning: can't read symbolic link ");
			perror(longname);
			return;
		}
		linkname[i] = '\0';
		dblock.dbuf.linkflag = '2';
		if (vflag) {
			fprintf(vfile, "a %s symbolic link %d\n",
			    longname, volume_num);
			fflush(vfile);
		}
		sprintf(dblock.dbuf.size, "%11lo", 0);

		/* Compute the length of the header for this link */
		current_total =
		    (&dblock.dbuf.start_of_name - &dblock.dbuf.mode[0] +
		    strlen(filename) + 1 + strlen(linkname) + 1 + TBLOCK) /
		    TBLOCK;

		charsleft = TBLOCK - (&dblock.dbuf.start_of_name -
			&dblock.dbuf.mode[0]);
		pass = FILEN;
		flag = 1;
		charsleft = writefilenames(&dblock.dbuf.start_of_name,
			charsleft, &pass, &flag, &hint);

		/* If the user specified to follow directory links, */
		/* see if this is a link to a directory, and if so follow it */
		if (Lflag) {
			i = stat(shortname, &stbuf);
			if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {

				/* Get the longname, shortname, and parent */
				/* If linkname is an absolute path:	   */
				if (linkname[0] == '/') {
					strcpy(newparent, linkname);
					/* Strip off trailing slashes */
					while (newparent[strlen(newparent)] ==
					    '/')
						newparent[strlen(newparent)] =
						    '\0';
					ptr = rindex(newparent, '/');
					*ptr = '\0';
					strcpy(newShortname, (ptr + 1));
					strcpy(newLongname, linkname);

					/* Change directories to parent */
					if (chdir(newparent) < 0) {
						fprintf(stderr,
	 "bar warning: cannot change directory when following link %s: ",
						    longname);
						perror(newparent);
						break;
					}
				}
				/* Else linkname is relative */
				else {
					sprintf(newLongname, "%s/%s", parent,
					    linkname);
					strcpy(newparent, newLongname);
					/* Strip off trailing slashes */
					while (newparent[strlen(newparent)] ==
					    '/')
						newparent[strlen(newparent)] =
						    '\0';
					ptr = rindex(newparent, '/');
					*ptr = '\0';
					strcpy(newShortname, (ptr + 1));

					/* Change directories to parent */
					if (chdir(newparent) < 0) {
						fprintf(stderr,
	"bar warning: cannot change directory when following link %s: ",
						    longname);
						perror(newparent);
						break;
					}
				}
				putfile(newLongname, newShortname, newparent,
				    TRUE);
			}
		}
		break;

	case S_IFBLK:
	case S_IFCHR:
	case S_IFIFO:
		tomodes(&stbuf);

		/* Copy filename to header */
		strcpy(filename, longname);
		linkname[i] = '\0';
		dblock.dbuf.linkflag = '3';

		if (vflag) {
			fprintf(vfile, "a %s special file %d\n", longname,
			    volume_num);
			fflush(vfile);
		}
		sprintf(dblock.dbuf.size, "%11lo", 0);

		/* Compute the length of the header for this link */
		current_total =
		    (&dblock.dbuf.start_of_name - &dblock.dbuf.mode[0] +
		    strlen(filename) + 1 + strlen(linkname) + 1 + TBLOCK) /
		    TBLOCK;

		charsleft = TBLOCK - (&dblock.dbuf.start_of_name -
			&dblock.dbuf.mode[0]);
		pass = FILEN;
		flag = 1;
		charsleft = writefilenames(&dblock.dbuf.start_of_name,
			charsleft, &pass, &flag, &hint);
		break;

	case S_IFREG:
		if (Mflag) {
			fprintf(stderr, "mtime of %s is %ld\n", longname,
			    stbuf.st_mtime);
			if (stbuf.st_mtime < modtime) {
				fprintf(stderr, "%s did not change\n",
				    longname);
				return;
			} else
				fprintf(stderr, "%s changed\n", longname);
		}
		if (Zflag) {
			/* Compress the file */
			/*
			 * relitively new way is to compress into a temp file
			 * so we save a write and read to/from disk
			 * FIX-ME - it really should go direct to bar output -
			 * but I didn't have time to figure it all out.
			 */
			/* NOTE: we don't do a compress */
			/* on the real file because we don;t want to */
			/* change the modify time. */
			pid = getpid();
			sprintf(tmp_file, "/tmp/tmp_%d", pid);

			if ((tmp_buf = malloc(MAXPATHLEN*6)) == NULL) {
				fprintf(stderr,"bar error: cannot malloc ");
				perror(nullstring);
				done(-1);
			}
			/*
			 * See if there is enough disk space to hole the com-
			 * pressed version of the file
			 */
			if (statfs("/tmp", &statfsbuf) == -1) {
				fprintf(stderr,
				    "bar warning: cannot stat /tmp ");
				perror(nullstring);
				free(tmp_buf);
				return;
			}
			if ((statfsbuf.f_bsize * statfsbuf.f_bavail) >=
			    stbuf.st_size) {
				sprintf(tmp_buf, "compress -fc %s > %s",
				    shortname, tmp_file);
				system(tmp_buf);

				/* Stat the temp file for the new information */
				if (!hflag)
					i = lstat(tmp_file, &tmpstbuf);
				else
					i = stat(tmp_file, &tmpstbuf);

				stbuf.st_size = tmpstbuf.st_size;
				stbuf.st_blocks = tmpstbuf.st_blocks;
				if ((infile = open(tmp_file, 0)) < 0) {
					fprintf(stderr, "bar warning: ");
					perror(longname);
					unlink(tmp_file);
					free(tmp_buf);
					return;
				}
			}
			/* Not enough space to compress this file, archive */
			/* it uncompressed.				   */
			else {
				free(tmp_buf);
				tmp_buf = NULL;
				goto open_the_file;
			}
		} else {
open_the_file:
			if ((infile = open(shortname, 0)) < 0) {
				fprintf(stderr, "bar warning: ");
				perror(longname);
				return;
			}
		}
		tomodes(&stbuf);
		dblock.dbuf.linkflag = '0';

		/* Write the filename to the header */
		strcpy(filename, longname);
		strcpy(linkname, nullstring);
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
				strcpy(linkname, lp->pathname);
				dblock.dbuf.linkflag = '1';

				/* Compute length of the header for this link */
				current_total =
				    (&dblock.dbuf.start_of_name -
				    &dblock.dbuf.mode[0] + strlen(filename) +
				    1 + strlen(linkname) + 1 + TBLOCK) / TBLOCK;

				charsleft = TBLOCK -
				    (&dblock.dbuf.start_of_name -
				    &dblock.dbuf.mode[0]);
				pass = FILEN;
				flag = 1;
				charsleft = writefilenames(
				    &dblock.dbuf.start_of_name, charsleft,
				    &pass, &flag, &hint);
				if (vflag) {
					fprintf(vfile, "a %s link %d\n",
					    longname, volume_num);
					fflush(vfile);
				}

				lp->count--;
				close(infile);

				if (Zflag) {
					unlink(tmp_file);
					if (tmp_buf)
						free(tmp_buf);
					tmp_buf = NULL;
				}
				return;
			}
			lp = (struct linkbuf *) getmem(sizeof (*lp));
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

		if (vflag) {
			fprintf(vfile, "a %s %ld blocks %d\n", longname,
			    blocks, volume_num);
			fflush(vfile);
		}

		/* Compute the length of the header */
		current_total = (&dblock.dbuf.start_of_name -
		     &dblock.dbuf.mode[0] + strlen(filename) + 1 +
		     strlen(linkname) + 1 + TBLOCK) / TBLOCK;

		/* Add the length of the file */
		current_total += blocks;

		charsleft = TBLOCK - (&dblock.dbuf.start_of_name -
			&dblock.dbuf.mode[0]);
		pass = FILEN;
		flag = 1;

		charsleft = writefilenames(&dblock.dbuf.start_of_name,
			charsleft, &pass, &flag, &hint);
		maxread = max(stbuf.st_blksize, (nblock * TBLOCK));
		if ((bigbuf = malloc((unsigned)maxread)) == 0) {
			maxread = TBLOCK;
			bigbuf = buf;
		}

		while ((i = read(infile, bigbuf,
		    min((hint*TBLOCK), maxread))) > 0 && blocks > 0) {
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
		if (i < 0) {
			fprintf(stderr, "bar warning: Read error on ");
			perror(longname);
		} else if (blocks != 0 || i != 0)
			fprintf(stderr, "bar warning: %s: file changed size\n",
			    longname);
		while (--blocks >=  0)
			putempty();
		if (Zflag) {
			unlink(tmp_file);
			if (tmp_buf)
				free(tmp_buf);
			tmp_buf = NULL;
		}
		break;

	default:
		fprintf(stderr, "bar warning: %s is not a file. Not dumped\n",
		    longname);
		break;
	}
}

doxtract(argv)
	char *argv[];
{
	long blocks, bytes;
	int	ofile;
	int	i;
	int	rdev;
	char *tmp_buf = NULL;
	register char **cp;
	FILE *pipef;
	int	firstZ;	/* first time thru for a (possibly) compresed file */

	/* See how many files we want and set the number already found */
	if (Tflag) {
		files_wanted = 0;
		files_found = 0;
		strcpy(last_file, nullstring);
		for (cp = argv; *cp; cp++)
			files_wanted++;
	}

	for (;;) {
		if ((i = wantit(argv)) == 0) {
			continue;
		}
		if (i == -1) {
			break;		/* end of tape */
		}
		if (checkw('x', filename) == 0) {
			passtape();
			continue;
		}
		if (Fflag) {
			char *s;

			if ((s = rindex(filename, '/')) == 0)
				s = filename;
			else
				s++;
			if (checkf(s, stbuf.st_mode, Fflag) == 0) {
				passtape();
				continue;
			}
		}
		if (checkdir(filename)) {	/* have a directory */
			if (mflag == 0)
				dodirtimes(&dblock);
			continue;
		}
		if (dblock.dbuf.linkflag == '2') {	/* symlink */
			/*
			 * only unlink non directories or empty
			 * directories
			 */
			if (rmdir(filename) < 0) {
				if (errno == ENOTDIR)
					unlink(filename);
			}
			if (symlink(linkname, filename)<0) {
				fprintf(stderr,
				    "bar warning: %s: symbolic link failed: ",
				    filename);
				perror(nullstring);
				continue;
			}
			if (vflag) {
				fprintf(vfile, "x %s symbolic link %d\n",
				    filename, volume_num);
				fflush(vfile);
			}
#ifdef notdef
			/* See if the file extracted should be owned by */
			/* either the current user or the user specified*/
			/* on the command line with the U and G flags   */
			if (sflag) {
				if (chown(filename, new_uid, new_gid) == -1) {
					fprintf(stderr,
				"bar warning: cannot change ownership of %s: ",
					    filename);
					perror(nullstring);
				}
			}

			/* else ignore alien orders */
			else
				chown(filename, stbuf.st_uid, stbuf.st_gid);
			if (mflag == 0)
				setimes(filename, stbuf.st_mtime);
			if (pflag)
				chmod(filename, stbuf.st_mode & 07777);
#endif
			continue;
		}
		if (dblock.dbuf.linkflag == '1') {	/* regular link */
			/*
			 * only unlink non directories or empty
			 * directories
			 */
			if (rmdir(filename) < 0) {
				if (errno == ENOTDIR)
					unlink(filename);
			}
			if (link(linkname, filename) < 0) {
				fprintf(stderr,
				    "bar warning: can't link %s to %s: ",
				    filename, linkname);
				perror(nullstring);
				continue;
			}
			if (vflag) {
				fprintf(vfile, "x %s link %d\n", filename,
				    volume_num);
				fflush(vfile);
			}
			continue;
		}

		if (dblock.dbuf.linkflag == '3') {	/* special file */
			sscanf(dblock.dbuf.mode, "%o", &i);
			if ((i & S_IFMT) == S_IFIFO)
				rdev = 0;
			else sscanf(dblock.dbuf.rdev, "%o", &rdev);

			if (mknod(filename, i, rdev) < 0) {
				fprintf(stderr,
				    "bar warning: cannot mknod for %s\n",
				    filename);
				perror(nullstring);
				continue;
			}
			if (vflag) {
				fprintf(vfile, "x %s special file %d\n",
				    filename, volume_num);
				fflush(vfile);
			}

			/* See if the file extracted should be owned by */
			/* either the current user or the user specified*/
			/* on the command line with the U and G flags   */
			if (sflag) {
				if (chown(filename, new_uid, new_gid)== -1) {
					fprintf(stderr,
				"bar warning: cannot change ownership of %s: ",
					    filename);
					perror(nullstring);
				}
			}

			/* else ignore alien orders */
			else
				chown(filename, stbuf.st_uid, stbuf.st_gid);

			if (mflag == 0)
				setimes(filename, stbuf.st_mtime);
			if (pflag)
				chmod(filename, stbuf.st_mode & 07777);
			continue;
		}

		if ((ofile = creat(filename, stbuf.st_mode & 0xfff)) < 0) {
			fprintf(stderr, "bar warning: can't create %s: ",
			    filename);
			perror(nullstring);
			passtape();
			continue;
		}
		blocks = ((bytes = stbuf.st_size) + TBLOCK-1)/TBLOCK;
		if (vflag) {
			fprintf(vfile, "x %s, %ld bytes, %ld tape blocks %d\n",
			    filename, bytes, blocks, volume_num);
			fflush(vfile);
		}
		/*
		 * if uncompressing - the new way is just popen "uncompress -c"
		 * to write the file in its place. - no out of temp messages!
		 */
		firstZ = 0;
		pipef = (FILE *)0;
		if (Zflag)
			firstZ = 1;
		for ( ; blocks > 0; ) {
			register int nread;
			char	*bufp;
			register int nwant;

			nwant = NBLOCK*TBLOCK;
			if (nwant > (blocks*TBLOCK))
				nwant = (blocks*TBLOCK);
			nread = readtbuf(&bufp, nwant);
			/*
			 * first time thru - if it might be compressed - check
			 * the magic on the file - it might NOT really be so.
			 */
			if (firstZ) {
				firstZ = 0;
				/* HARDCODED compress magic numbers! */
				/* but bar is a mess anyway - why didn't each */
				/* file get marked as to its comressed state? */
				/* too late now - its "history" */
				if ((bufp[0] == (char)0x1F) &&
				    (bufp[1] == (char)0x9D)) {
					/* setup uncompress */
					if ((tmp_buf = malloc(MAXPATHLEN*6))
					    == NULL) {
						fprintf(stderr,
						    "bar error: can't malloc ");
						perror(nullstring);
						done(-1);
					}
					if (access(filename, W_OK) !=0) {
					  struct stat sb2;
					  fstat(ofile, &sb2);
					  sprintf(tmp_buf, "chmod +w %s; uncompress -c > %s; chmod 0%o %s", filename, filename, sb2.st_mode, filename);
					}
					else 
					  sprintf(tmp_buf, "uncompress -c > %s", filename);
					if ((pipef = popen(tmp_buf, "w")) ==
					    NULL) {
						fprintf(stderr,
					"bar error: can't popen uncompress: ");
						perror(nullstring);
						done(2);
					}
					/* substitute pipef's fd for ofile */
					(void)close(ofile);
					ofile = fileno(pipef);
				}
				/* (else) just normal */
			}
			/* NOTE: either writing to pipe or to the file */
			if (write(ofile, bufp, (int)min(nread, bytes)) < 0) {
				fprintf(stderr,
			"bar error: %s: HELP - extract write error: ",
				    filename);
				if (pipef != (FILE *)NULL)
					fprintf(stderr, "(during uncompress) ");
				perror(nullstring);
				done(2);
			}
			bytes -= nread;
			blocks -= (((nread-1)/TBLOCK)+1);
		}
		if (Zflag && (tmp_buf != NULL)) {
			free(tmp_buf);
			tmp_buf = NULL;
		}
		if (Zflag && (pipef != (FILE *)NULL))
			pclose(pipef);
		else
			(void)close(ofile);
		/* See if the file extracted should be owned by */
		/* either the current user or the user specified*/
		/* on the command line with the U and G flags   */
		if (sflag) {
			if (chown(filename, new_uid, new_gid)== -1) {
				fprintf(stderr,
			"bar warning: cannot change ownership of %s:",
				    filename);
				perror(nullstring);
			}
		} else
			chown(filename, stbuf.st_uid, stbuf.st_gid);

		if (mflag == 0)
			setimes(filename, stbuf.st_mtime);
		if (pflag)
			chmod(filename, stbuf.st_mode & 07777);
	}

	if (mflag == 0) {
		filename[0] = '\0';	/* process the whole stack */
		dodirtimes(&dblock);
	}
}

dotable(argv)
	char *argv[];
{
	register int i;
	char year[10], month[5], day[5], hour[5], minute[5], *ptr;
	register char **cp;

	/* We read the volume header in openmt */

	/* Parse through date for month, day, hour, minute*/
	ptr = Vheader->dbuf.date;
	strcpy(year, ptr);
	year[2] = '\0';
	strcpy(month, ptr + 2);
	month[2] = '\0';
	strcpy(day, ptr + 4);
	day[2] = '\0';
	strcpy(hour, ptr + 6);
	hour[2] = '\0';
	strcpy(minute, ptr + 8);
	minute[2] = '\0';

	/* See how many files we want and set the number already found */
	if (Tflag) {
		files_wanted = 0;
		files_found = 0;
		strcpy(last_file, nullstring);
		for (cp = argv; *cp; cp++)
			files_wanted++;
	}

	for (;;) {
		if ((i = wantit(argv)) == 0)
			continue;
		if (i == -1)
			break;		/* end of tape */
		if (vflag)
			longt(&stbuf);
		printf("%s", filename);
		printf(" \"%s\" %s %s %s %s ", Vfilename, month, day, hour,
		    minute);
		if (dblock.dbuf.linkflag == '1')
			printf(" link ");
		if (dblock.dbuf.linkflag == '2')
			printf(" symbolic link ");
#ifndef FLOPPYINSTALL
		printf("%d\n", volume_num);
#endif !FLOPPYINSTALL
		passtape();
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

	for (mp = &m[0]; mp < &m[9]; )
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
 * a directory on the bar tape (indicated by a trailing '/'),
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
				perror(name);
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

onintr()
{
	(void) signal(SIGINT, SIG_IGN);
	term++;
}

onquit()
{
	(void) signal(SIGQUIT, SIG_IGN);
	term++;
}

onhup()
{
	(void) signal(SIGHUP, SIG_IGN);
	term++;
}

#ifdef notdef
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
	sprintf(dblock.dbuf.mode, "%7o", sp->st_mode & 077777);
	sprintf(dblock.dbuf.uid, "%6o ", sp->st_uid);
	sprintf(dblock.dbuf.gid, "%6o ", sp->st_gid);
	sprintf(dblock.dbuf.size, "%11lo ", sp->st_size);
	sprintf(dblock.dbuf.mtime, "%11lo ", sp->st_mtime);
	sprintf(dblock.dbuf.rdev, "%o", sp->st_rdev);
}

checksum()
{
	register i;
	register char *cp;

	for (cp = dblock.dbuf.chksum;
	    cp < &dblock.dbuf.chksum[sizeof (dblock.dbuf.chksum)]; cp++)
		*cp = ' ';
	i = 0;
	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		i += *cp;
	return (i);
}

header_checksum()
{
	register i;
	register char *cp;

	for (cp = vblock.dbuf.chksum;
	    cp < &vblock.dbuf.chksum[sizeof (vblock.dbuf.chksum)]; cp++)
		*cp = ' ';
	i = 0;
	for (cp = vblock.dummy; cp < &vblock.dummy[TBLOCK]; cp++)
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
			return (0);
		return (1);
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
	fflush(stderr);
	fflush(vfile);
	close(mt);
	if (dflag)
		close(mt1);
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
	char oldName[MAXPATHLEN];
	char rootDir[2];
	int len;

	getdir();
	if (endtape()) {
		return (-1);
	}

	/* This means extract everything */
	if (*argv == 0) {
		cp = argv;
		strcpy(rootDir, "/");
		goto gotit;
	}

	/* See if the current file is one that we want */
	for (cp = argv; *cp; cp++) {
		if (prefix(*cp, filename)) {
			goto gotit;
		}
	}
	/* See if we found everything in the last directory we looked through */
	/* NOTICE:  This assumes that the files in the archive are in order, */
	/* ie. files in each directory archived are grouped together. */
	if (Tflag && strcmp(last_file, nullstring)) {
		files_found++;
		strcpy(last_file, nullstring);
		if (files_found == files_wanted) {
			return (-1);
		}
	}
	passtape();
	return (0);

gotit:
	if (Xflag) {
		if (excluded(filename)) {
			if (vflag) {
				fprintf(stderr, "\"%s\" excluded %d\n",
					filename);
			}
			passtape();
			return (0);
		}
	}
	if (Sflag) {

		len = 0;
		strcpy(oldName, filename);
		if ((prefix(oldPath, oldName)) == 1) {

			/* Strip off the old pathname */
			len = strlen(oldPath);
			strcpy(oldName, &filename[len]);

			/* Prepend the new path */
			if (newPath[strlen(newPath)] == '/' ||
			    oldName[0] == '/')
				sprintf(filename, "%s%s", newPath, oldName);
			else
				sprintf(filename, "%s/%s", newPath, oldName);
		}

	}

	/* See if we found everything in the last directory we looked through */
	/* NOTICE:  This assumes that the files in the archive are in order, */
	/* ie. files in each directory archived are grouped together. */
	if (Tflag && *argv != 0) {
		if (strcmp(last_file, nullstring)) {
			if (!prefix(*cp, last_file)) {
				files_found++;
				strcpy(last_file, *cp);
				if (files_found == files_wanted) {
					return (-1);
				}
			}
		} else
			strcpy(last_file, *cp);
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

	for (i=0; s[i]; i++)
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
	for (i=0; i<N; i++) {
		if (b[i] == '\n')
			break;
		m++;
	}
	if (m >= h)
		return (-1L);
	m1 = m;
	j = i;
	for (i++; i<N; i++) {
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
		done(-1);
	for (i=0; i<n; i++) {
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
	return (TBLOCK);
}

readtbuf(bufpp, size)
	char **bufpp;
	int size;
{
	register int i;

	if (recno >= nblock || first == 0) {
		if ((i = bread(mt, (char *)tbuf, TBLOCK*nblock)) < 0) {
			fprintf("bar error: could not read\n");
			done(-1);
		}
		if (i > 0) {
			if (first == 0) {
				if ((i % TBLOCK) != 0) {
					fprintf(stderr,
				"bar error: tape blocksize error\n");
					done(3);
				}
				i /= TBLOCK;
				if (i != nblock) {
					nblock = i;
				}
				first = 1;
			}
			recno = 0;
		}
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
	char *savebuf;

	if (first == 0) {
		getbuf();
		first = 1;
	}
	if (recno >= nblock) {
		errno = 0;
		i = write(mt, (char *)tbuf, TBLOCK*nblock);

		if (i != TBLOCK*nblock) {
#ifdef DEBUG
			fprintf(stderr,
		"bar write failed: bytes written = %d instead of %d\n",
			    i, TBLOCK * nblock);
#endif
			mterr("write", -1);
#ifdef DEBUG
			fprintf(stderr,
			    "1:bytes written to previous volume = %d\n",
			    bytes_written);
			bytes_written = 0;
#endif DEBUG
			/* Fill in size with the number of bytes to skip */
			/* to the beginning of the first complete file   */
			sprintf(vblock.dbuf.size, "%lo", blks_to_skip*TBLOCK);

			savebuf = malloc(TBLOCK*nblock);
			bcopy(tbuf, savebuf, TBLOCK*nblock);

			/* Now write the volume header */
			bcopy(&vblock, tbuf, TBLOCK);
			bcopy(savebuf, &tbuf[1], TBLOCK*(nblock-1));
			i = write(mt, (char *)tbuf, TBLOCK*nblock);
			if (dflag)
				write(mt1, (char *)tbuf, TBLOCK*nblock);

			/* Figure out how many blocks we really wrote*/
			/* we didn't really write one of the blocks */
			/* because we wrote the volume header */
			if (current_written  == 0)
				current_written = current_total - 1;
			else
				current_written -= 1;
			blks_to_skip = current_total - current_written;

			recno = 1;
			bcopy(savebuf+(TBLOCK*(nblock-1)), tbuf, TBLOCK);
			current_written += 1;
			if (current_written == current_total)
				current_written = 0;
			free(savebuf);
		} else {
			if (dflag)
				write(mt1, (char *)tbuf, TBLOCK*nblock);
			recno = 0;
			if (current_written == 0)
				/* Next buffer starts on a file boundary */
				blks_to_skip = 0;
			else {
				blks_to_skip = current_total - current_written;
			}
		}
#ifdef DEBUG
		if (i == TBLOCK*nblock)
			bytes_written += TBLOCK*nblock;
#endif DEBUG

	}

	/*
	 *  Special case:  We have an empty tape buffer, and the
	 *  users data size is >= the tape block size:  Avoid
	 *  the bcopy and dma direct to tape.  BIG WIN.  Add the
	 *  residual to the tape buffer.
	 */
	while (recno == 0 && n >= nblock) {
		/* Compute how much of this file is left to be written */
		current_written += nblock;
		/* we're trying to write a buffer the size of blocking factor */
		if (current_written == current_total)
			/* This whole file is in write buffer */
			current_written = 0;

		errno = 0;
		i = write(mt, buffer, TBLOCK*nblock);

		if (i != TBLOCK*nblock) {
#ifdef DEBUG
			fprintf(stderr, "2: bytes written to this vol = %d\n",
				bytes_written);
			bytes_written = 0;
#endif DEBUG
#ifdef DEBUG
			fprintf(stderr,
		"bar write failed: bytes written = %d instead of %d\n",
			    i, TBLOCK * nblock);
#endif
			mterr("write", -1);

			/* Fill in size with the number of bytes to skip */
			/* to the beginning of the first complete file   */
			sprintf(vblock.dbuf.size, "%lo", blks_to_skip*TBLOCK);

			/* Now write the volume header */
			bcopy(&vblock, tbuf, TBLOCK);
			bcopy(buffer, &tbuf[1], TBLOCK*(nblock-1));
			i = write(mt, (char *)tbuf, TBLOCK*nblock);
			if (dflag)
				write(mt1, (char *)tbuf, TBLOCK*nblock);

			/* Figure out how many blocks we really wrote*/
			/* we didn't really write one of the blocks */
			/* because we wrote the volume header */
			if (current_written == 0)
				current_written = current_total - 1;
			else
				current_written -= 1;
			blks_to_skip = current_total - current_written;

			recno = 1;
			bcopy(buffer+(TBLOCK*(nblock-1)), tbuf, TBLOCK);
			current_written += 1;
			if (current_written == current_total)
				current_written = 0;

		} else {
			if (dflag)
				write(mt1, buffer, TBLOCK*nblock);
			recno = 0;
			if (current_written == 0)
				/* Next buffer starts on a file boundary */
				blks_to_skip = 0;
			else {
				blks_to_skip = current_total - current_written;
			}
		}

#ifdef DEBUG
		if (i == TBLOCK*nblock)
			bytes_written += TBLOCK*nblock;
#endif DEBUG
		n -= nblock;
		buffer += (nblock * TBLOCK);
	}

	while (n-- > 0) {
		bcopy(buffer, (char *)&tbuf[recno++], TBLOCK);

		/* Compute how much of this file is left to be written */
		/* we just added one block to the write buffer */
		current_written += 1;

		if (current_written == current_total)
			/* This whole file is in write buffer */
			current_written = 0;

		buffer += TBLOCK;
		if (recno >= nblock) {
			errno = 0;
			i = write(mt, (char *)tbuf, TBLOCK*nblock);
			if (i != TBLOCK*nblock) {
#ifdef DEBUG
				fprintf(stderr,
				    "3: %d bytes written to this volume\n",
				    bytes_written);
				bytes_written = 0;
#endif DEBUG
#ifdef DEBUG
			fprintf(stderr,
		"bar write failed: bytes written = %d instead of %d\n",
			    i, TBLOCK*nblock);
#endif
				mterr("write", -1);
				/* Fill in size with number of bytes to skip */
				/* to the beginning of first complete file */
				sprintf(vblock.dbuf.size, "%lo",
				    blks_to_skip*TBLOCK);

				savebuf = malloc(TBLOCK*nblock);
				bcopy(tbuf, savebuf, TBLOCK*nblock);

				/* Now write the volume header */
				bcopy(&vblock, tbuf, TBLOCK);
				bcopy(savebuf, &tbuf[1], TBLOCK*(nblock-1));
				i = write(mt, (char *)tbuf, TBLOCK*nblock);
				if (dflag)
					write(mt1, (char *)tbuf, TBLOCK*nblock);

				/* Figure out how many blocks we really wrote*/
				/* we didn't really write one of the blocks */
				/* because we wrote the volume header */
				if (current_written == 0)
					current_written = current_total -1;
				else
					current_written -= 1;
				blks_to_skip = current_total - current_written;

				recno = 1;
				bcopy(savebuf+(TBLOCK*(nblock-1)), tbuf,
				    TBLOCK);
				current_written += 1;
				if (current_written == current_total)
					current_written = 0;
				free(savebuf);
			} else {
				if (dflag)
					write(mt1, (char *)tbuf, TBLOCK*nblock);
				recno = 0;
				if (current_written == 0)
					/* Next buffer starts on file boundry */
					blks_to_skip = 0;
				else {
					blks_to_skip =
					    current_total - current_written;
				}
			}
#ifdef DEBUG
			if (i == TBLOCK*nblock)
				bytes_written += TBLOCK*nblock;
#endif DEBUG
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
		if (ioctl(mt, MTIOCTOP, (char *)&mtop) < 0) {
			fprintf(stderr, "bar error: tape backspace error: ");
			perror(nullstring);
			done(4);
		}
	} else
		lseek(mt, (long) -TBLOCK*nblock, 1);
	recno--;
}

flushtape()
{
	register int i;
	char *savebuf;

	errno = 0;
	i = write(mt, (char *)tbuf, TBLOCK*nblock);
	if (i != TBLOCK*nblock) {
#ifdef DEBUG
		fprintf(stderr,
		    "bar write failed: bytes written = %d instead of %d\n",
		    i, TBLOCK*nblock);
#endif
		mterr("write", -1);

#ifdef DEBUG
		fprintf(stderr, "1:bytes written to previous volume = %d\n",
			bytes_written);
		bytes_written = 0;
#endif DEBUG
		/* Fill in size with the number of bytes to skip */
		/* to the beginning of the first complete file   */
		sprintf(vblock.dbuf.size, "%lo", blks_to_skip*TBLOCK);

		savebuf = malloc(TBLOCK*nblock);
		bcopy(tbuf, savebuf, TBLOCK*nblock);

		/* Now write the volume header */
		bcopy(&vblock, tbuf, TBLOCK);
		bcopy(savebuf, &tbuf[1], TBLOCK*(nblock-1));
		i = write(mt, (char *)tbuf, TBLOCK*nblock);
		if (dflag)
			write(mt1, (char *)tbuf, TBLOCK*nblock);

		/* And write the last block */
		bcopy(savebuf+(TBLOCK*(nblock-1)), tbuf, TBLOCK);
		i = write(mt, (char *)tbuf, TBLOCK*nblock);
		free(savebuf);
	}
}

mterr(operation, exitcode)
	char *operation;
	int exitcode;
{
	struct stat statb;
	int save_errno;

#ifdef DEBUG
	char str[MAXPATHLEN];
	sprintf(str, "bar: errno = %d", errno);
	perror(str);
#endif DEBUG

	/* Save the error code */
	save_errno = errno;

	/* See if this is the type of file that could have */
	/* given us a change reel error. */
	fstat(mt, &statb);

	/* We cannot change reels unless the file is a special */
	/* character device file. */
	if (((statb.st_mode & S_IFMT) == S_IFCHR) &&
	    (save_errno == ENXIO || save_errno == ENOSPC ||
	    save_errno == EIO || save_errno == 0)) {

		/* If this is the floppy drive, check to see if */
		/* the user yanked the floppy from the drive */
		if (is_floppy(usefile, mt)) {
			/* If there is no floppy in the drive */
			/* tell the user to put it back in.   */
			if (check_for_floppy(usefile, mt, 0) == 0) {
				/* If there was a floppy in the */
				/* drive, this is a real error. */
				/* so eject the floppy */
				(void) ioctl(mt, FDKEJECT, NULL);
				chgreel(operation, exitcode);
			} else {
				fprintf(stderr,
			"bar error: diskette removed from drive during %s\n",
				    operation);
				done(exitcode);
			}
		} else
			chgreel(operation, exitcode);
	} else {
		errno = save_errno;
		fprintf(stderr, "bar error: %s error: ", operation);
		perror(nullstring);
		done(exitcode);
	}
}

bread(fd, buf, size)
	int fd;
	char *buf;
	int size;
{
	int count;
#define	NOREAD -2147483648
	static int lastread = NOREAD;
	char *newbuf;
	int newsize;

	if (!Bflag) {
		newbuf = buf;
		newsize = size;
		errno = 0;
		lastread = read(fd, newbuf, newsize);
		if (lastread != newsize) {
#ifdef DEBUG
			fflush(stdout);
			bytes_read = 0;
			fprintf(stderr,
		"bar read failed: bytes written = %d instead of %d\n",
			    lastread, newsize);
#endif
			mterr("read", -1);

			/* Reading volume header will get us new data */
			return (0);
		} else {
#ifdef DEBUG
			bytes_read += lastread;
#endif DEBUG
			return (lastread);
		}
	}

	for (count = 0; count < size; count += lastread) {
		if (lastread <= 0 && lastread != NOREAD) {
			if (count > 0)
				return (count);
			return (lastread);
		}
		lastread = read(fd, buf, size - count);
		if (lastread <= 0) {
			errno = 0;
			mterr("read", -1);
#ifdef DEBUG
			fprintf(stderr, "bytes read on previous vol = %d\n",
				bytes_read);
			bytes_read = 0;
#endif DEBUG
			/* Reading the volume header  will get us new data */
			lastread = size-count;
		}
#ifdef DEBUG
		bytes_read += size;
#endif DEBUG
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
		fprintf(stderr, "bar error: %s\n", buf);
		done(-1);
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
		fprintf(stderr,
		    "bar error: blocksize %d too big, can't get memory\n",
		    nblock);
		done(1);
	}
}

/*
 * Following code handles excluding files.
 * A hash table of file names to be excluded is built.
 * Before writing or extracting a file check to see if it is
 * in the exclude table.
 */
#define	NEXCLUDE 512
struct	exclude	{
	char	*name;			/* Name of file to exclude */
	struct	exclude	*next;		/* Linked list */
};
struct	exclude	**exclude_tbl;		/* Hash table of exclude names */

build_exclude(file)
char	*file;
{
	FILE	*fp;
	char	buf[512];

	fp = fopen(file, "r");
	if (fp == NULL) {
		fprintf(stderr, "bar error: could not open exclude file %s: ",
			file);
		perror(nullstring);
		done(-1);
	}
	if (exclude_tbl == NULL) {
		exclude_tbl = (struct exclude **)
			calloc(sizeof (struct exclude *), NEXCLUDE);
	}
	while (fgets(buf, sizeof (buf), fp) != NULL) {
		buf[strlen(buf) - 1] = '\0';
		add_exclude(buf);
	}
	fclose(fp);
}

/*
 * Add a file name to the exclude table, if the file name has any trailing
 * '/'s then delete them before inserting into the table
 */
add_exclude(str)
char	*str;
{
	char	name[MAXPATHLEN];
	unsigned int h;
	struct	exclude	*exp;

	strcpy(name, str);
	while (name[strlen(name) - 1] == '/') {
		name[strlen(name) - 1] = NULL;
	}

	h = hash(name);
	exp = (struct exclude *) malloc(sizeof (struct exclude));
	if (exp == NULL) {
		fprintf(stderr, "malloc 1 failed in add_exclude -- ");
		perror("");
		done(-1);
	}
	if ((exp->name = malloc(strlen(name)+1)) == NULL) {
		fprintf(stderr, "malloc 2 failed in add_exclude -- ");
		perror("");
		done(-1);
	}
	(void)strcpy(exp->name, name);
	exp->next = exclude_tbl[h];
	exclude_tbl[h] = exp;
}

/*
 * See if a file name or any of the file's parent directories is in the
 * excluded table, if the file name has any trailing '/'s then delete
 * them before searching the table
 */
excluded(str)
char	*str;
{
	char	name[MAXPATHLEN];
	unsigned int	h;
	struct	exclude	*exp;
	char	*ptr;

	strcpy(name, str);
	while (name[strlen(name) - 1] == '/') {
		name[strlen(name) - 1] = NULL;
	}

	/*
	 * check for the file name in the exclude list
	 */
	h = hash(name);
	exp = exclude_tbl[h];
	while (exp != NULL) {
		if (strcmp(name, exp->name) == 0) {
			return (1);
		}
		exp = exp->next;
	}

	/*
	 * check for any parent directories in the exclude list
	 */
	while (ptr = rindex(name, '/')) {
		*ptr = NULL;
		h = hash(name);
		exp = exclude_tbl[h];
		while (exp != NULL) {
			if (strcmp(name, exp->name) == 0) {
				return (1);
			}
			exp = exp->next;
		}
	}

	return (0);
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
	return (h % NEXCLUDE);
}

#define	NINCLUDE 512

/*
 * build_list(file)
 *	Build a list of files in memory from a list in a file
 */
char **build_list(file)
char	*file;
{
	FILE	*fp;
	char	buf[MAXPATHLEN];
	int	room;
	int	num_files;
	char **file_list, **orig_ptr, *newbuf, *tmp_ptr;

	fp = fopen(file, "r");
	if (fp == NULL) {
		fprintf(stderr, "bar error: could not open file %s: ", file);
		perror(nullstring);
		done(-1);
	}

	/* Allocate space for the pointer array */
	if ((file_list = (char **)malloc(PTR_SIZE*NINCLUDE)) == NULL){
		fprintf(stderr, "bar error: cannot malloc ");
		perror(nullstring);
		done(-1);
	}
	orig_ptr = file_list;
	room = NINCLUDE-1;
	num_files = 0;

	while (fgets(buf, sizeof (buf), fp) != NULL) {
		if ((tmp_ptr = index(buf, ';')) != NULL)
			*tmp_ptr = '\0';

		if ((newbuf = malloc(strlen(buf)+1)) == NULL) {
			fprintf(stderr, "cannot malloc ");
			perror(nullstring);
			done(-1);
		}
		strcpy(newbuf, buf);
		*file_list++ = newbuf;
		num_files++;

		/* If out of room in ptr list, realloc ptr list */
		if (num_files == room) {
			/* Allocate space for the pointer array */
			orig_ptr = (char **)realloc(orig_ptr, PTR_SIZE * room);
			if (orig_ptr == NULL) {
				fprintf(stderr, "bar error: cannot malloc ");
				perror(nullstring);
				done(-1);
			}
			file_list = orig_ptr + num_files;
			room += NINCLUDE;
		}

	}
	fclose(fp);
	return (orig_ptr);
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
char dirstack[MAXPATHLEN];
#define	NTIM (MAXPATHLEN/2+1)		/* a/b/c/d/... */
time_t mtime[NTIM];

dodirtimes(hp)
	union hblock *hp;
{
	register char *p = dirstack;
	register char *q = &hp->dbuf.start_of_name;
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
	if (utimes(path, tv) < 0) {
		fprintf(stderr, "bar error: can't set time on %s: ", path);
		perror(nullstring);
	}
}

char *
getmem(size)
{
	char *p = malloc((unsigned) size);

	if (p == NULL && freemem) {
		fprintf(stderr,
	"bar error: out of memory, link and directory modtime info lost\n");
		freemem = 0;
	}
	return (p);
}

chgreel(operation, exitcode)
	char *operation;
	int exitcode;
{
	char newPrompt[MAXPATHLEN], tmp[MAXPATHLEN];
	int	wrongVol, badVol;

	/* Ask the user to change the reels */

	wrongVol = 0;
	badVol = 0;
	while (1) {
		close(mt);

		if (wrongVol) {
			if (!Pflag)
				sprintf(newPrompt,
	"bar: Wrong volume. Insert volume %d and press Return when ready.",
				    volume_num + 1);
			else {
				if (Vflag) {
					sprintf(tmp,
					    "bar: Wrong volume, %s", prompt);
					sprintf(newPrompt, tmp, fakeVol+1);
				} else {
					sprintf(tmp, "bar: Wrong volume, %s",
					    prompt);
					sprintf(newPrompt, tmp, volume_num + 1);
				}
			}
			promptUser(newPrompt);
			wrongVol = 0;
		} else if (badVol) {
			if (!Pflag)
				sprintf(newPrompt,
	"bar: Bad format. Insert volume %d and press Return when ready.",
				    volume_num + 1);
			else {
				if (Vflag) {
					sprintf(tmp, "bar: Bad format, %s",
					    prompt);
					sprintf(newPrompt, tmp, fakeVol + 1);
				} else {
					sprintf(tmp, "bar: Bad format, %s",
					    prompt);
					sprintf(newPrompt, tmp, volume_num + 1);
				}
			}
			promptUser(newPrompt);
			badVol = 0;
		} else {
			if (!Pflag)
				sprintf(newPrompt,
			"bar: Insert volume %d and press Return when ready.",
				    volume_num + 1);
			else {
				if (Vflag)
					sprintf(newPrompt, prompt, fakeVol + 1);
				else
					sprintf(newPrompt, prompt,
					    volume_num + 1);
			}
			promptUser(newPrompt);
		}

		/* We only get back here if the user responds with 'Y' */
		/* Increment the volume number */
		volume_num++;

		if (Vflag)
			fakeVol++;

		/* Open this new volume */
		if (strcmp("read", operation))
			mt = openmt(usefile, WRITING, CONT);
		else
			mt = openmt(usefile, READING, CONT);

		if (mt == -1) {
			/* The wrong volume was inserted */
			wrongVol = 1;

			volume_num--;
			if (Vflag)
				fakeVol--;
			recno = nblock;	/* Fake out readtape */
		} else if (mt == -2) {
			/* Volume inserted not in bar format */
			badVol = 1;
			volume_num--;
			if (Vflag)
				fakeVol--;
			recno = nblock;	/* Fake out readtape */
		} else
			break;
	}
	return (mt);
}

/*ARGSUSED*/
is_floppy(name, mt)
	char *name;
	int mt;
{
#ifdef i386	/* really "sun386" but compiler no longer defines it */
	struct stat statb;

	/*
	 * This HACK XXX should be removed.  Either the 386 folks should
	 * add the FDKIOGCHAR ioctl to their driver, or another (new)
	 * ioctl should be used.
	 */
	if (!strcmp(name, "/dev/rfd0a") || !strcmp(name, "/dev/rfd0c") ||
		!strcmp(name, "/dev/rfd0g") || !strcmp(name, "rfd0g") ||
		!strcmp(name, "rfd0a") || !strcmp(name, "rfd0c") ||
		!strcmp(name, "/dev/rfdl0a") || !strcmp(name, "/dev/rfdl0c") ||
		!strcmp(name, "rfdl0a") || !strcmp(name, "rfd0c")) {

		/* See if this is a character device */
		fstat(mt, &statb);

		/* We do not check for the floppy in the drive unless */
		/* this really is the floppy device. */
		if ((statb.st_mode & S_IFMT) == S_IFCHR)
			return (1);
	}
#else /* not sun386 */
	struct fdk_char stuff;

	/*
	 * try to get the floppy drive characteristics, just to see if
	 * the thing is in fact a floppy drive(er).
	 */
	if (ioctl(mt, FDKIOGCHAR, &stuff) != -1) {
		/* the ioctl succeded, must have been a floppy */
		return (1);
	}
#endif /* not sun386 */
	return (0);
}

/*ARGSUSED*/
is_tape(name, mt)
	char *name;
	int mt;
{
#ifdef i386	/* really "sun386" but compiler no longer defines it for you */
	struct stat statb;
	char str1[MAXPATHLEN], str2[MAXPATHLEN];

	strcpy(str1, name);
	strcpy(str2, name);
	str1[8] = '\0';
	str2[3] = '\0';
	if (!strcmp(str1, "/dev/rst") || !strcmp(str2, "rst")) {

		/* See if this is a character device */
		fstat(mt, &statb);
		if ((statb.st_mode & S_IFMT) == S_IFCHR) {
#ifdef DEBUG
			fprintf(stderr, "this is a tape\n");
#endif DEBUG
			return (1);
		}
	}
#else /* !sun386 */
	struct mtget stuff;

	/*
	 * try to do a generic tape ioctl, just to see if
	 * the thing is in fact a tape drive(er).
	 */
	if (ioctl(mt, MTIOCGET, &stuff) != -1) {
		/* the ioctl succeded, must have been a tape */
		return (1);
	}
#endif /* !sun386 */
	return (0);
}

/*
 * check_for_floppy - if wait..., will prompt iff NO diskette is in drive,
 *	otherwise returns 0;
 *	if !wait, will return 0 if diskette is in drive, else rets -1.
 */
check_for_floppy(tape, mt, waitForFloppy)
	char *tape;
	int mt, waitForFloppy;
{
	char newPrompt[MAXPATHLEN];
	char tmpPrompt[MAXPATHLEN];

#ifdef i386	/* really "sun386" but compiler no longer defines it for you */
	struct fdraw fdcmd;

	struct fdraw *fd = &fdcmd;
	char *fc = &fdcmd.fr_cmd[0];
	int f, err;

recheck:
	*fc = READDIR;

	if ((err = ioctl(mt, F_RAW,  &fdcmd)) == -1){
		fprintf(stderr, "bar error: cannot access %s\n", tape);
		done(-1);
	}

	if (! (fdcmd.fr_result[0] & 0x80)) {
		return (0);
	}

	fc = &fdcmd.fr_cmd[0];
	*fc++ = 0xf;		/* seek */
	*fc++ = 0;		/* head */
	*fc++ = 20;		/* cylinder */
	fd->fr_cnum = 3;	/* number of cmd bytes */

	if ((err = ioctl(mt, F_RAW,  &fdcmd)) == -1){
		fprintf(stderr, "bar error: %s seek failed\n", tape);
		done(-1);
	}
	fc = &fdcmd.fr_cmd[0];
	*fc++ = 0xf;		/* seek */
	*fc++ = 0;		/* head */
	*fc++ = 0;		/* cylinder */
	fd->fr_cnum = 3;	/* number of cmd bytes */

	if ((err = ioctl(mt, F_RAW,  &fdcmd)) == -1){
		fprintf(stderr, "bar error: %s seek failed\n", tape);
		done(-1);
	}


	fc = &fdcmd.fr_cmd[0];
	*fc = READDIR;

	if ((err = ioctl(mt, F_RAW,  &fdcmd)) == -1){
		fprintf(stderr, "bar error: cannot access %s\n", tape);
		done(-1);
	}
	if (! (fdcmd.fr_result[0] & 0x80)) {
		return (0);
	}

#else /* !sun386 */
	int	chgd;

recheck:
	/*
	 * XXX for those systems that don't support this ioctl, tuff!
	 */
	if (ioctl(mt, FDKGETCHANGE, &chgd) != 0) {
		fprintf(stderr, "bar: cannot do FDKGETCHANGE on %s\n", tape);
		done(-1);
	}
	/* if disk changed is currently clear, a diskette is in the drive */
	if ((chgd & FDKGC_CURRENT) == 0) {
		return (0);
	}

#endif /* !sun386 */

	/*
	 * we got here because there was no floppy in the drive
	 */
	if (waitForFloppy) {
		if (!Pflag) {
			sprintf(newPrompt,
"bar: No diskette in drive. Insert volume %d and press Return when ready.",
			    volume_num);
		} else {
			if (Vflag) {
				sprintf(tmpPrompt,
				    "bar: No diskette in drive, %s", prompt);
				sprintf(newPrompt, tmpPrompt, fakeVol);
			} else {
				sprintf(tmpPrompt,
				    "bar: No diskette in drive, %s", prompt);
				sprintf(newPrompt, tmpPrompt, volume_num);
			}
		}
		promptUser(newPrompt);

		/* If we get back here the user responded with yes, use a */
		/* gorpy goto to avoid too much recursion - it happened! */
		goto recheck;
		/* ******* */
	} else {
		return (-1);
	}
}

promptUser(prompt1)
	char *prompt1;
{
	char str[MAXPATHLEN];

	/* Tell the user to insert the floppy */
	fprintf(stderr, prompt1);
	fflush(stderr);
	(void) signal(SIGINT, SIG_DFL);
	gets(str);
	(void) signal(SIGINT, onintr);
	if (*str == 'N' || *str == 'n')
		done(-1);
}
