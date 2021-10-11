#ifdef lint
#ident		"@(#)verify_diskette.c 1.1 92/07/30 SMI";
#endif
/*
 *
 * verify_diskette - to verify that the floppy in the drive is the right one
 *	for suninstall.  All error messages and prompts go to stderr.
 *
 *  verify_diskette arch volno archieveno mediadev name [mediaserver]
 *
 * argv[0] - cmdname
 * argv[1] - arch or "-" allowed if just reading a toc
 * argv[2] - volume number or "-1" to say just reading a toc
 * argv[3] - archieveno - which file on tape / offset of file on floppy
 * argv[4] - install media device
 * argv[5] - "name" - IGNORED
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
 *	  NOTE: the tar file may cross diskettes, so this program must handle
 *	  that, verifying in turn that each particular volume is mounted.
 *	  This program checks the "suninstall label" aka "label", which is
 *	  an ascii string of the form:
 *	Volume ## of <Application SunOS - or other text> for ARCH
 *	where:
 *		## is a decimal number
 *		ARCH is "sun4c" or other Sun arch string
 *
 */

/*
 * NOTE: this file serves both as source for itself, and as the subroutines
 * for extracting diskette - as set off by ifdef SUBROUTINE_ONLY
 * (the subroutines come first)
 */

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sun/dkio.h>
#include "install.h"		/* LOGFILE is a char[] from strings.o */

#ifndef SUBROUTINE_ONLY		/* so extracting_diskette can use subs */
char *progname = "verify_diskette";
#endif !SUBROUTINE_ONLY

/* media size is set to 79 * 18 * 2 * 512 bytes! HARDCODED */
/* FIX ME : use DKIOCGAPART */
int disksize = 1456128;
#define	XDRSIZE (512*(36-(4+1)))
#define	CYLSIZE (512*36)
#define	VLBLOFFSET (35*512)		/* offset of per volume label */
#define	XDRTOCOFFSET (4*512)		/* offset of xdrtoc on diskette */
#define	VLBLSIZE 512

char	msg[512];	/* for error messages, etc. */
FILE *logf = NULL;

char	vlbljunk[100];
int	vlblvol;
char	vlblarch[100];

char	*nullstring = "";
char	buf[ CYLSIZE ];	/* for diskette, and input responses */
char	cyl1dev[50];
char	mediadev[50];

/*
 * verify that the correct diskette is in the drive
 */
verify_diskette(device, volnum, arch)
char	*device;
int	volnum;
char	*arch;
{
	int	fd;
	int	chgd;

	/*
	 * first read the diskette mini label off to see which one this is
	 * and if it is of the correct set.
	 * O_NDELAY is set so we don't touch the disk just yet
	 */
	if ((fd = open(device, O_RDONLY|O_NDELAY)) < 0) {
		sprintf(msg,
		    "%s: fatal error: can't open %s", progname, cyl1dev);
		domsg(0);
		exit(1);
	}
	while (1) {
		/* verify diskette in drive */
		if (ioctl(fd, FDKGETCHANGE, &chgd) != 0) {
			sprintf(msg, "%s: fatal error on FDKGETCHANGE of %s\n",
			    progname, cyl1dev);
			domsg(0);
			exit(1);
		}
		/* if disk changed is currently clear, a diskette is in drive */
		if ((chgd & FDKGC_CURRENT) != 0) {
			sprintf(msg, "NO DISKETTE IN DRIVE\n");
			goto ask_for_another;
		}
		

		/* seek to where the vol label is */
		if (lseek(fd, VLBLOFFSET, 0) != VLBLOFFSET) {
			sprintf(msg, "%s: fatal error: couldn't seek on %s",
			    progname, cyl1dev);
			domsg(0);
			exit(1);
		} else
		if (read(fd, buf, VLBLSIZE) != VLBLSIZE) {
			sprintf(msg, "%s: couldn't read %s", progname, cyl1dev);
			domsg(0);
		} else
		if (grabscan(buf)) {
			sprintf(msg,
		"%s: bad scan of label: not a suninstall diskette?\n",
			    progname);
			domsg(0);
		} else if ((((volnum == -1) && (*arch != '-')) ||
		    (volnum != -1)) &&
		    (strcmp(arch, vlblarch) != 0)) {
			sprintf(msg, "%s: diskette arch \"%s\" is not \"%s\"\n",
				progname, vlblarch, arch);
			domsg(0);
		} else
		/* if only checking for one with a toc, don`t check volume */
		if ((volnum != -1) && (volnum != vlblvol)) {
			sprintf(msg, "%s: diskette number %d is not %d\n",
				progname, vlblvol, volnum);
			domsg(0);
		} else
		/*
		 * we got here with the correct diskette - but we need to
		 * parse the xdrtoc for the starting offset and size
		 * (or send it to std out if volnum == -1)
		 */
		/* go to where xdrtoc resides */
		if (lseek(fd, XDRTOCOFFSET, 0) != XDRTOCOFFSET) {
			sprintf(msg, "%s: fatal error: couldn't seek on %s",
			    progname, cyl1dev);
			domsg(0);
			exit(1);
		} else
		if (read(fd, buf, XDRSIZE) != XDRSIZE) {
			sprintf(msg, "%s: couldn't read %s", progname, cyl1dev);
			domsg(0);
		} else {
			break;
		}

		/* if we get here, we failed a check, so eject the floppy */
		(void)ioctl(fd, FDKEJECT, NULL);

		/* ask (as precisely as possible) user to put in a diskette */
		msg[0] = '\0';
ask_for_another:
		if (volnum == -1) {
			sprintf(&msg[strlen(msg)],
			    "insert any suninstall diskette");
			if (*arch != '-')
				sprintf(&msg[strlen(msg)], " for \"%s\"", arch);
		} else {
			sprintf(&msg[strlen(msg)],
			    "insert suninstall diskette %d for \"%s\"",
			    volnum, arch);
		}
		sprintf(&msg[strlen(msg)], ", then press <return>:");
		domsg(1);
		continue;
	}
	if (volnum == -1)
		(void)write(1, buf, XDRSIZE);
	close(fd);
	return (0);
}
/*
 * parse the buffer for strings (all builtin)
 */
grabscan(buf)
char *buf;
{
	char	*sp;
	int	expect;
	int	x;
	char	*strtok();

	expect = 2;

	sp = buf;
	while ((sp = strtok(sp, " \n\0")) != NULL) {
		x = sscanf(sp, "volume=%d", &vlblvol);
		if (x == 1) {
			expect--;
		}
		x = sscanf(sp, "arch=%s", vlblarch);
		if (x == 1) {
			expect--;
		}
		sp = NULL;	/* arm strtok() for next pass */
	}
	return (expect);
}


domsg(flg)
int	flg;	/* 0 just print, 1 = wait for return */
{
	fprintf(stderr, msg);
	if (logf == NULL) {
		/* if we can't open the log file, just tuff! */
		logf = fopen(LOGFILE, "a");
	}
	if (logf != NULL)
		fprintf(logf, msg);
	if (flg) {
		/* wait for user to type a return */
		/* (void)fgets(buf, 512, stdin); */
		while (1) {
			(void) read(0, buf, 1);
			if ((buf[0] == '\n') || (buf[0] == '\r'))
				break;
		}
		if (logf != NULL)
			fprintf(logf, "\n");
	}
}


/*
 * get_mediadev - parse input for correct media device and forces it
 *	to partition "a"
 */
void
get_mediadev(cp)
char	*cp;
{

	strcpy(mediadev, cp);

	/* find the unit number in the string */
	/* ASSUMES its the only numeric in the string XXX */
	cp = mediadev;
	while ((*cp != '0') && (*cp != '1'))
		cp++;
	cp++;		/* jump over the unit number */
	*cp++ = 'a';	/* tack on a "a" for suninstall data partition */
	*cp = '\0';	/* and end string nicely */

}

#ifndef SUBROUTINE_ONLY		/* so extracting_diskette can use subs */

int	volnum;

int	xdrvol;
int	xdroffset;
char	xdrname[100];
int	xdrcount;

/* the args */
char	*cmdname;	/* 0 */
char	*arch;		/* 1 */
char	*volno;		/* 2 */
char	*archieveno;	/* 3 */
/* char	*mediadev;	4 */
/* char	*name;		5 */
/* char	*mediaserver;	6 */


main(argc, argv)
int	argc;
char	**argv;
{
	int	fd;
	int	chgd;
	char	*cp;

	if ((argc < 6) || (argc > 7)) {
		sprintf(msg,
	"Usage: %s [arch|-] [volno|-1] offset mediadev name [mediaserver]\n",
		    argv[0]);
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
	volno	= argv[ 2 ];
	archieveno = argv[ 3 ];
	get_mediadev(argv[ 4 ]);

	if ((volnum = atoi(volno)) == 0) {
		sprintf(msg, "%s: invalid volume number, volno arg was %s\n",
		    progname, volno);
		domsg(0);
		exit(1);
	}

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

	exit(0);
}

#endif !SUBROUTINE_ONLY
