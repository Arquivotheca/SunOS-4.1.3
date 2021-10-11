#ifdef lint
#ident		"@(#)extract_diskette.c 1.1 92/07/30 SMI";
#endif
/*
 *
 * extracting diskette - to get data off of a set of floppys for
 *	suninstall, verifying that they are the correct diskettes.
 *	all error messages go to stdout.
 *
 *  extract_diskette arch mediadev volno bs keyword [mediaserver]
 *
 * argv[0] - cmdname
 * argv[1] - arch
 * argv[2] - install media device
 * argv[3] - volume number
 * argv[4] - block size (ignored)
 * argv[5] - "keyword" - which catagory to extract
 * [ argv[6] - mediaserver - since diskettes can`t be remote : NOT USED! ]
 */
/*
 * Assumptions:
 *
 *	o On diskette, each suninstall catagory is a compressed tar(1) format
 *	  file that is dd'd onto the diskette(s).  It starts at the volume
 *	  number given in the xdrtoc, which is passed into this code.
 *	  (It starts at an offset, which is passed in as the "archieveno",
 *	  but this script doesn't need to use it).
 *	  NOTE: the compresed tar file may cross diskettes, so this program
 *	  must handle that, verifying in turn that each particular volume is
 *	  mounted.  This program checks the "suninstall label" aka "label",
 *	  which consists of ascii strings of the form:
 *	volume=## arch=<arch>
 *	where:
 *		## is a decimal number
 *		<arch> is "sun4c" or other Sun arch string
 *
 */

char *progname = "extracting_diskette";

#include <signal.h>
#include <errno.h>
extern char *sys_errlist[];
#include <string.h>

#define	SUBROUTINE_ONLY		/* use subs from verifying diskette */
#include "./verify_diskette.c"

int	volnum;

int	xdrvol;
int	xdroffset;
char	xdrname[100];
int	xdrcount;

char	cmd[512];
FILE *pf;

/* the args */
char	*cmdname;	/* 0 */
char	*arch;		/* 1 */
/* char	*mediadev;	2 */
char	*volno;		/* 3 */
/*** char	*bs;		4 */
char	*keyword;	/* 5 */
/*** char	*mediaserver;	6 */

int	showflag = 0;

main(argc, argv)
int	argc;
char	**argv;
{
	int	fd;
	int	chgd;
	char	*cp;
	int	toread;

	(void)signal(SIGPIPE, SIG_IGN);

	/*
	 * if this is show_diskette keyword, set up for showing the files
	 */
	cp = strrchr(argv[0], '/');
	if (cp == NULL)
		cp = argv[0];
	else
		cp++;
	if (*cp == 's') {
		showflag++;
		progname = "show_diskette";
	}

	if ((argc < 6) || (argc > 7)) {
		sprintf(msg, "Usage: %s arch mediadev volno bs keyword\n", cp);
		domsg(0);
		exit(1);
	}
	if ((argc == 7) && (*argv[6] != '\0')) {
		sprintf(msg,
	"ERROR: remote diskette installation not supported...press <RETURN>");
		domsg(1);
		exit(1);
	}

	cmdname	= argv[ 0 ];
	arch	= argv[ 1 ];
	get_mediadev(argv[ 2 ]);
	volno	= argv[ 3 ];
	/* block size not used */
	keyword	= argv[ 5 ];

	(void)sscanf(volno, "%d", &volnum);

	sprintf(msg, "%s \"%s\" files from diskette drive \"%s\"\n",
		(showflag?"Listing":"Extracting"), keyword, mediadev);
	domsg(0);

	/* *** cyl1dev=`expr X$mediadev : 'X\(.*fd[0-1]\).*'` *** */
	/* *** cyl1dev=${cyl1dev}b *** */
	strcpy(cyl1dev, mediadev);
	cp = cyl1dev;
	/* find the unit number in the string */
	while ((*cp != '0') && (*cp != '1'))
		cp++;
	cp++;		/* jump over the unit number */
	*cp++ = 'b';	/* tack on a "b" for label partition */
	*cp = '\0';	/* and end string nicely */

	verify_diskette(cyl1dev, volnum, arch);

	/*
	 * we got here with correct volume and an image of the xdrtoc in buf.
	 * dual pipes to xdrtoc and scan it for this keyword
	 */
	sprintf(cmd, "/usr/etc/install/xdrtoc | awk 'BEGIN { volno=%d; keyword=\"%s\"; } { if ($1 != volno) next; if ($3 != keyword) next; print $0; } ' ",
	    volnum, keyword);

	if (p2open(cmd, buf, XDRSIZE, sizeof (buf)) <= 0) {
		sprintf(msg, "%s: couldn't process xdrtoc\n", progname);
		domsg(0);
		exit(1);
	} else
	if (sscanf(buf, "%d %d %s %d",
	    &xdrvol, &xdroffset, xdrname, &xdrcount) != 4) {
		sprintf(msg, "%s: bad scanf of label: vol %d, count %d\n",
		    progname, xdrvol, xdrcount);
		domsg(0);
		exit(1); /* XXX  or retry diskette? */
	}

#ifdef DEBUG
fprintf(stderr, "%s: found vol %d, offset %d, name %s, count %d\n",
		    progname, xdrvol, xdroffset, xdrname, xdrcount);
#endif DEBUG

/* XXX check for preposterous counts? */
#ifdef XXX
	DKIOCGPART() to get partition size
	compare
#endif XXX

	/*
	 * found correct diskette,
	 * now we popen "compress | tar" and then start to
	 * read diskette in and stuff the bytes onto stdout
	 */
	/* use O_NDELAY - these diskette don't have a label */
	if ((fd = open(mediadev, O_RDONLY|O_NDELAY)) < 0) {
		sprintf(msg, "%s: couldn't open %s",  mediadev);
		domsg(0);
		exit(1);
	}
	sprintf(buf, "uncompress -c | tar %s -  2>> %s",
		(showflag)?("tvpBf"):("xpBf"), LOGFILE);
	if ((pf = popen(buf, "w")) == NULL) {
		sprintf(msg, "%s: fatal error: couldn't popen \"%s\"",
		    progname, buf);
		domsg(0);
		exit(1);
	}
	/*
	 * this major while loop reads each diskette in the catagory
	 */
	while (xdrcount) {
		/* each diskette needs to have a seek done */
		if (lseek(fd, xdroffset, 0) != xdroffset) {
			sprintf(msg, "%s: couldn't seek on %s, vol %d",
			    progname, mediadev, volnum);
			domsg(0);
			exit(1);
		}
		toread = disksize - xdroffset;	/* possible read on this disk */
		if (toread > xdrcount)
			toread = xdrcount;	/* just one disk */
		(void)readit(fd, toread);
		xdrcount -= toread;
		if (xdrcount == 0)
			break;		/* read all - don't grab next disk! */
		xdroffset += toread;
		if (xdroffset >= disksize) {
			/* new disk - reset offset, re-verify, and seek to 0 */
			xdroffset = 0;
			(void)ioctl(fd, FDKEJECT, NULL); /* eject the floppy */
			sprintf(msg,
	"insert suninstall diskette %d for \"%s\", then press <return>:",
			    ++volnum, arch);
			domsg(1);
			(void)verify_diskette(cyl1dev, volnum, arch);
		}
	}
	fflush(pf);
	close(fd);
	pclose(pf);
	exit(0);
}

char progress[ 4 ] = { '|', '/', '-', '\\' };
/*
 * read "toread" bytes off of diskette" pipe to stdout
 */
readit(fd, toread)
{
	int	cnt;
	int	rc;
	int	prog = 0;	/* progress semaphore */
	int	dcnt;

	/* round up to cyl size for 1st read */
	cnt = CYLSIZE - (xdroffset % CYLSIZE);
	while (toread) {
		if (cnt > toread)
			cnt = toread;
		/* for last read of odd numbers of bytes, round up to 512 */
		dcnt = ((cnt+511)/512)*512;
		rc = read(fd, buf, dcnt);
		if (rc != dcnt) {
			sprintf(msg, "%s: %s: read error, vol %d (%s)\n",
			    progname, mediadev, volnum, keyword);
			domsg(0);
/*XXX* recover? */
			exit(1);
		}
		toread -= cnt;
		if (fwrite(buf, 1, cnt, pf) == NULL) {
			sprintf(msg,
			    "%s: pipe write error, %d left, vol %d (%s): %s\n",
			    progname, toread, volnum, keyword,
			    sys_errlist[errno]);
			domsg(0);
/*XXX* recover? */
			exit(1);
		}
		cnt = CYLSIZE;
		/* progress? */
		if (! showflag) {
			fprintf(stdout, "%c\b", progress[ prog & 0x3 ]);
			prog++;
			fflush(stdout);
		}
	}
	if (prog) {
		fprintf(stdout, "\b");
		fflush(stdout);
	}
	return (0);
}

/*
 * basically a 2-way popen
 *	writes buf to pipe, reads input back to buf
 * RETURNS: -1 on error, else number of bytes read from pipe
 */
#define	RDR	0
#define	WTR	1
p2open(cmd, buf, wcount, maxread)
char	*cmd;			/* command to execute */
caddr_t	buf;			/* where to write from/read to */
int	wcount;			/* how many to write */
int	maxread;		/* maximum to read */
{
	int	in[2];
	int	out[2];
	int	cnt;
	int	rcnt;
	int	nread;
	caddr_t	rp;
	int	pid;
	long	toread;
	int	readable;

	if (pipe(in) < 0)
		return (-1);
	if (pipe(out) < 0) {
		close(in[0]);
		close(in[1]);
		return (-1);
	}
	if ((pid = vfork()) == 0) {
		(void)close(in[1]);
		(void)close(out[0]);
		(void)dup2(in[0], 0);
		(void)dup2(out[1], 1);
		(void) execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
/*XXX error msg here */
		_exit(127);
	}
	if (pid == -1) {
		close(in[0]);
		close(in[1]);
		close(out[0]);
		close(out[1]);
		return (-1);
	}
	(void)close(in[0]);
	(void)close(out[1]);
	nread = 0;
	readable = 0;	/* keep track of free space in buf */
	rp = buf;
	while (wcount) {
		cnt = wcount;
		if (cnt > 1024)
			cnt = 1024;
		cnt = write(in[1], buf, cnt);
		if (cnt == -1) {
			/* error OR more likely broken pipe */
			break;
		}
		wcount -= cnt;
		buf += cnt;
		readable += cnt;
		if (ioctl(out[0], FIONREAD, &toread) < 0) {
			sprintf(msg, "%s: pipe FIONREAD error\n", progname);
			domsg(0);
			exit(1);
		}
		if (toread == 0)
			continue;
		/* got something, read it back */
		rcnt = read(out[0], rp, readable);
		if (rcnt <= 0)
			continue;	/* was error or EOF already */
		readable -= rcnt;
		nread += rcnt;
		rp += rcnt;
	}
	fflush(stderr);
	/* to end the chain, close the input pipe */
	(void) close(in[1]);
	while (nread < maxread) {
		rcnt = read(out[0], rp, maxread - nread);
		if (rcnt <= 0)
			break;	/* was error or EOF already */
		nread += rcnt;
		rp += rcnt;
	}
cleanup:
	close(in[1]);
	close(out[0]);
	/* let init have the carcass! */
	return (nread);
}
