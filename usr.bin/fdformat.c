#ifndef lint
static	char sccsid[] = "@(#)fdformat.c 1.1 92/07/30 Copyright 1989 Sun Microsystems";
#endif

/*
 * fdformat program - formats floppy disks, and then adds a label to them
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioccom.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <sys/file.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <errno.h>

/* DEFINES */
#define getbyte(A, N)   (((unsigned char *)(&(A)))[N])
#define htols(S)        ((getbyte(S, 1) << 8) | getbyte(S, 0))
#define uppercase(c)	((c) >= 'a' && (c) <= 'z' ? (c) - 'a' + 'A' : (c))
#define min(a,b)        ((a) < (b) ? (a) : (b))

/* EXTERNS */
char	*strcpy();
char	*memset();
off_t	lseek();
char	*strcat();
char	*strncat();

/* INITIALIZED DATA */
char	*dev_name = "/dev/rfd0c";
char	*nullstring = "";
/* default ascii label is high density */
char	*asciilabel = "3.5\" floppy cyl 80 alt 0 hd 2 sec 18";

/* UNINITIALIZED DATA */
struct fdk_char fdchar;
struct dk_label Label;

/* for verify buffers */
char	*ibuf1;
char	*obuf;
extern char	*malloc();

int	b_flag = 0;	/* install a volume label to the dos diskette */
int	d_flag = 0;	/* format the diskette in dos format */
int	e_flag;		/* "eject" diskette when done (if supported) */
int	f_flag;		/* "force" (no confirmation before start) flag */
int	l_flag;		/* low density flag */
int	x_flag = 0;	/* skip the format, only install SunOS label or */
			/* DOS file system */
int	m_flag = 0;	/* medium density */
int	v_flag = 0;	/* verify format/diskette flag */
int	z_flag = 0;	/* debugging only, setting partial formatting */

char	*myname;

/* F_RAW ioctl command structures for seeking and formatting */
struct fdraw fdr_seek = {
	0x0f, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,
	0
};

struct fdraw fdr_form = {
	0x4d, 0, 2, 0, 0x54, 0xe5, 0, 0, 0, 0,
	6,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,	/* nbytes */
	0	/* addr */
};

/*
 *	     MS-DOS Disk 
 *	       layout:
 *	 ------------------- 
 *	|    Boot sector    |
 *	|-------------------|
 *	|   Reserved area   |
 *	|-------------------|
 *	|      FAT #1	    |
 *	|-------------------|
 *	|      FAT #2	    |
 *	|-------------------|
 *	|   Root directory  |
 *	|-------------------|
 *	|     File area     |
 *	|___________________|
 *
 */

/*
 * The following is a copy of MS-DOS 3.3 boot block.
 * It consists of the BIOS parameter block, and a disk
 * bootstrap program.
 *
 * The BIOS parameter block contains the right value
 * for the 3.5" quad-density 1.44M floppy format.
 *
 */
char bootsec[512] = {
	0xeb, 0x34, 0x90,		/* 8086 short jump + displacement + NOP */
	'M', 'S', 'D', 'O', 'S', '3', '.', '3',	/* OEM name & version */
	0, 2, 1, 1, 0,				/* Start of BIOS parameter block */
	2, 224, 0, 0x40, 0xb, 0xf0, 9, 0,
	18, 0, 2, 0, 0, 0,			/* End of BIOS parameter block */
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x12,
	0x0, 0x0, 0x0, 0x0,
	0x1, 0x0, 0xfa, 0x33,			/* 0x34, start of the bootstrap. */
	0xc0, 0x8e, 0xd0, 0xbc, 0x0, 0x7c, 0x16, 0x7,
	0xbb, 0x78, 0x0, 0x36, 0xc5, 0x37, 0x1e, 0x56,
	0x16, 0x53, 0xbf, 0x2b, 0x7c, 0xb9, 0xb, 0x0,
	0xfc, 0xac, 0x26, 0x80, 0x3d, 0x0, 0x74, 0x3,
	0x26, 0x8a, 0x5, 0xaa, 0x8a, 0xc4, 0xe2, 0xf1,
	0x6, 0x1f, 0x89, 0x47, 0x2, 0xc7, 0x7, 0x2b,
	0x7c, 0xfb, 0xcd, 0x13, 0x72, 0x67, 0xa0, 0x10,
	0x7c, 0x98, 0xf7, 0x26, 0x16, 0x7c, 0x3, 0x6,
	0x1c, 0x7c, 0x3, 0x6, 0xe, 0x7c, 0xa3, 0x3f,
	0x7c, 0xa3, 0x37, 0x7c, 0xb8, 0x20, 0x0, 0xf7,
	0x26, 0x11, 0x7c, 0x8b, 0x1e, 0xb, 0x7c, 0x3,
	0xc3, 0x48, 0xf7, 0xf3, 0x1, 0x6, 0x37, 0x7c,
	0xbb, 0x0, 0x5, 0xa1, 0x3f, 0x7c, 0xe8, 0x9f,
	0x0, 0xb8, 0x1, 0x2, 0xe8, 0xb3, 0x0, 0x72,
	0x19, 0x8b, 0xfb, 0xb9, 0xb, 0x0, 0xbe, 0xd6,
	0x7d, 0xf3, 0xa6, 0x75, 0xd, 0x8d, 0x7f, 0x20,
	0xbe, 0xe1, 0x7d, 0xb9, 0xb, 0x0, 0xf3, 0xa6,
	0x74, 0x18, 0xbe, 0x77, 0x7d, 0xe8, 0x6a, 0x0,
	0x32, 0xe4, 0xcd, 0x16, 0x5e, 0x1f, 0x8f, 0x4,
	0x8f, 0x44, 0x2, 0xcd, 0x19, 0xbe, 0xc0, 0x7d,
	0xeb, 0xeb, 0xa1, 0x1c, 0x5, 0x33, 0xd2, 0xf7,
	0x36, 0xb, 0x7c, 0xfe, 0xc0, 0xa2, 0x3c, 0x7c,
	0xa1, 0x37, 0x7c, 0xa3, 0x3d, 0x7c, 0xbb, 0x0,
	0x7, 0xa1, 0x37, 0x7c, 0xe8, 0x49, 0x0, 0xa1,
	0x18, 0x7c, 0x2a, 0x6, 0x3b, 0x7c, 0x40, 0x38,
	0x6, 0x3c, 0x7c, 0x73, 0x3, 0xa0, 0x3c, 0x7c,
	0x50, 0xe8, 0x4e, 0x0, 0x58, 0x72, 0xc6, 0x28,
	0x6, 0x3c, 0x7c, 0x74, 0xc, 0x1, 0x6, 0x37,
	0x7c, 0xf7, 0x26, 0xb, 0x7c, 0x3, 0xd8, 0xeb,
	0xd0, 0x8a, 0x2e, 0x15, 0x7c, 0x8a, 0x16, 0xfd,
	0x7d, 0x8b, 0x1e, 0x3d, 0x7c, 0xea, 0x0, 0x0,
	0x70, 0x0, 0xac, 0xa, 0xc0, 0x74, 0x22, 0xb4,
	0xe, 0xbb, 0x7, 0x0, 0xcd, 0x10, 0xeb, 0xf2,
	0x33, 0xd2, 0xf7, 0x36, 0x18, 0x7c, 0xfe, 0xc2,
	0x88, 0x16, 0x3b, 0x7c, 0x33, 0xd2, 0xf7, 0x36,
	0x1a, 0x7c, 0x88, 0x16, 0x2a, 0x7c, 0xa3, 0x39,
	0x7c, 0xc3, 0xb4, 0x2, 0x8b, 0x16, 0x39, 0x7c,
	0xb1, 0x6, 0xd2, 0xe6, 0xa, 0x36, 0x3b, 0x7c,
	0x8b, 0xca, 0x86, 0xe9, 0x8a, 0x16, 0xfd, 0x7d,
	0x8a, 0x36, 0x2a, 0x7c, 0xcd, 0x13, 0xc3, '\r',
	'\n', 'N', 'o', 'n', '-', 'S', 'y', 's',
	't', 'e', 'm', ' ', 'd', 'i', 's', 'k',
	' ', 'o', 'r', ' ', 'd', 'i', 's', 'k',
	' ', 'e', 'r', 'r', 'o', 'r', '\r', '\n',
	'R', 'e', 'p', 'l', 'a', 'c', 'e', ' ',
	'a', 'n', 'd', ' ', 's', 't', 'r', 'i',
	'k', 'e', ' ', 'a', 'n', 'y', ' ', 'k',
	'e', 'y', ' ', 'w', 'h', 'e', 'n', ' ',
	'r', 'e', 'a', 'd', 'y', '\r', '\n', '\0',
	'\r', '\n', 'D', 'i', 's', 'k', ' ', 'B',
	'o', 'o', 't', ' ', 'f', 'a', 'i', 'l',
	'u', 'r', 'e', '\r', '\n', '\0', 'I', 'O',
	' ', ' ', ' ', ' ', ' ', ' ', 'S', 'Y',
	'S', 'M', 'S', 'D', 'O', 'S', ' ', ' ',
	' ', 'S', 'Y', 'S', '\0', 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0x55, 0xaa
};


struct bios_param_blk {
	u_char	b_bps[2];		/* bytes per sector */
	u_char  b_spcl;			/* sectors per alloction unit */
	u_char  b_res_sec[2];		/* reserved sectors, starting at 0 */
	u_char  b_nfat;			/* number of FATs */
	u_char  b_rdirents[2];		/* number of root directory entries */
	u_char  b_totalsec[2];		/* total sectors in logical image */
	char    b_mediadescriptor;	/* media descriptor byte */
	u_char	b_fatsec[2];		/* number of sectors per FAT */
	u_char	b_spt[2];		/* sectors per track */
	u_char	b_nhead[2];		/* number of heads */
	u_char	b_hiddensec[2];		/* number of hidden sectors */
};


main(argc, argv)
int	argc;
char	**argv;
{
	int	fd;
	char	*args;
	int	i, j;
	int	transfer_rate;	/* transfer rate code */
	int	spt;		/* sectors per track */
	int	sec_size;	/* sector size */
	u_char	max_cyl;	/* max number of cylinders */
	int	medium;		/* = 1 if medium density */
	int	cyl, hd;
	u_char	*fbuf, *p;
	int	chgd;	/* for testing disk changed/present */
	char	filename[DK_DEVLEN+8];
	char	*doslabel;

	/* XXX myname = *argv; ***/
	myname = "fdformat";
	argc--;			/* skip cmd name */
	argv++;

	while (argc-- > 0) {
		args = *argv++;
		if (*args == '-') {
			args++;			/* skip "-" */
			while (*args) {
				switch (*args) {

				case 'b':
					b_flag++;
					if ((argc-- == 0) || (**argv == '-'))
						usage("missing label");
					doslabel = strdup(*argv++); 
					break;

				case 'd':
					/* format a dos diskette */
					d_flag++;
					break;

				case 'e':
					/* eject diskette when done */
					e_flag++;
					break;

				case 'f':
					/* don't ask for confirmation */
					f_flag++;
					break;

				case 'l':
				case 'L':
					/* format a 720 k disk */
					l_flag++;
					break;
#ifdef FD_MEDIUM_DENSITY
				case 'm':
					/* format a 1.2 Mb disk */
					m_flag++;
					break;
#endif

				case 'v':
				case 'V':
					/* verify the diskette */
					v_flag++;
					break;
#ifdef FD_DEBUG
				case 'x':
					x_flag++;
					break;

				case 'Z':
					/* for debug only, format cyl 0 only */
					(void)printf("\nFormat cyl Zero only\n");
					z_flag++;
					break;
#endif
				default:
					usage("unknown argument");
					/* NOTREACHED */
				}
				args++;
			}
		} else {
			/* must have just a device name left */
			if (argc > 0) {
				usage("more than one device name argument");
				/* NOTREACHED */
			}
			dev_name = args;
		}
	}

	/*
	 * If there isn't an absolute pathname then build one
	 * up.  It is assumed that if it is a relative path name then
	 * it is just the logical device.  This is the same way format
	 * does it.
	 */
	if (dev_name[0] != '/') {
		filename[0] = '\0';
		(void) strcat(filename, "/dev/r");
		(void) strncat(filename, dev_name, DK_DEVLEN);
		i = strlen(filename);
		filename[i] = 'c';
		filename[i+1] = '\0';
		dev_name = filename;
	}

	if (!f_flag && !x_flag) {
		(void)printf("Press return to start formatting floppy. ");

		while (getchar() != '\n')
			;
	}

	if ((fd = open(dev_name, O_NDELAY | O_RDWR | O_EXCL)) == -1){
		if (errno == EROFS) {
			(void)fprintf(stderr,
			    "fdformat: \"%s\" is write protected\n", dev_name);
			exit(1);
		}
		/* XXX ought to check for "drive not installed", etc. */
		(void)fprintf(stderr, "fdformat: could not open \"%s\": ",
		    dev_name);
		perror(nullstring);
		exit(1);
	}

	/*
	 * for those systems that support this ioctl, they will
	 * return whether or not a diskette is in the drive.
	 * for those that don't support it, we ignore error.
	 */
	if ((ioctl(fd, FDKGETCHANGE, &chgd) == 0) &&
	    (chgd & FDKGC_CURRENT)) {
		(void)fprintf(stderr, "%s: no diskette in drive\n", myname);
		exit(4);
	}

	/* find out the characteristics of the drive/controller */
	if (ioctl(fd, FDKIOGCHAR, &fdchar) == -1) {
		(void)fprintf(stderr, "%s: FDKIOGCHAR failed, ", myname);
		perror(nullstring);
		exit(3);
	}

	if (l_flag) {
		medium = 0;
		transfer_rate = 250;
		spt = 9;
		max_cyl = 80;
		sec_size = 512;
	} else if (m_flag) {
		medium = 1;
		transfer_rate = 500;
		spt = 8;
		max_cyl = 77;
		sec_size = 1024;
	} else {
		medium = 0;
		transfer_rate = 500;
		spt = 18;
		max_cyl = 80;
		sec_size = 512;
	}

	if ((transfer_rate != fdchar.transfer_rate) || (spt != fdchar.secptrack)) {
		fdchar.transfer_rate = transfer_rate;
		fdchar.secptrack = spt;
		fdchar.ncyl = max_cyl;
		fdchar.sec_size = sec_size;
		fdchar.medium = medium;

		if (ioctl(fd, FDKIOSCHAR, &fdchar) == -1) {
			(void)fprintf(stderr, "%s: FDKIOSCHAR failed, ",
				myname);
			perror(nullstring);
			exit(3);
		}
	}

	if (x_flag)
		goto skipformat;

	/*
	 * do the format, a track at a time
	 */
	if ((fbuf = (u_char *)malloc((unsigned)(4 * spt))) == 0) {
		(void)fprintf(stderr, "%s: can't malloc format header buffer, ",
			myname);
		perror(nullstring);
		exit(3);
	}
	for (cyl = 0; cyl < (z_flag ? 1 : fdchar.ncyl); cyl++) {
		fdr_seek.fr_cmd[2] = cyl;
		if (ioctl(fd, F_RAW, &fdr_seek) == -1) {	/* seek */
			(void)fprintf(stderr, "%s: seek to cyl %d failed\n",
			    myname, cyl);
			exit(3);
		}
		for (hd = 0; hd < fdchar.nhead; hd++) {
			p = (u_char *)fbuf;
			for (i=1; i<=spt; i++) {
				*p++ = cyl;
				*p++ = hd;
				*p++ = i; /* sector# */
				*p++ = m_flag ? 3 : 2;
			}
			fdr_form.fr_cmd[1] = (hd<<2);
			fdr_form.fr_cmd[2] = m_flag ? 3 : 2;
			fdr_form.fr_cmd[3] = spt;
			fdr_form.fr_nbytes = 4*spt;
			fdr_form.fr_addr = (char *)fbuf;
			if (ioctl(fd, F_RAW, &fdr_form) == -1) {
				(void)fprintf(stderr,
				    "%s: format of cyl %d head %d failed\n",
				    myname, cyl, hd);
				exit(3);
			}
			if (fdr_form.fr_result[0] & 0xc0) {
				if (fdr_form.fr_result[1] & 0x02) {
					(void)fprintf(stderr,
					    "%s: diskette is write protected\n",
					    myname);
					exit(3);
				}
				(void)fprintf(stderr,
				    "%s: format of cyl %d head %d failed\n",
				    myname, cyl, hd);
				exit(3);
			}

		}
		/*
		 *  do a verify
		 */
		if (v_flag || (cyl == 0)) {
			/* HARDCODED bytes per sector! */
			verify(fd, cyl * fdchar.secptrack,
			    fdchar.secptrack * (m_flag ? 1024 : 512) * 2);
		}
		/* write some progress msg when each cylinder is done. */
		printf(".");
		fflush(stdout);
	}
	printf("\n");

skipformat:
	if (lseek(fd, (off_t)0, 0) != 0) {
		(void)fprintf(stderr, "%s: seek to blk 0 failed, ",
		    myname);
		perror(nullstring);
		exit(3);
	}

	if (d_flag) {
		u_short fatsec;
		u_short rdirsec;
		u_short totalsec;
		char	fat_rdir[512];
		struct	bios_param_blk *bpb;

		bpb = (struct bios_param_blk *)&(bootsec[0xb]);
		bpb->b_bps[0] = 512 & 0xff;
		bpb->b_bps[1] = 512 >> 8;
		bpb->b_res_sec[0] = 1;
		bpb->b_res_sec[1] = 0;
		bpb->b_nfat = 2;
		totalsec = fdchar.ncyl * fdchar.nhead * fdchar.secptrack;
		bpb->b_totalsec[0] = getbyte(totalsec, 1);
		bpb->b_totalsec[1] = getbyte(totalsec, 0);
		bpb->b_spt[0] = fdchar.secptrack;
		bpb->b_spt[1] = 0;
		bpb->b_nhead[0] = fdchar.nhead;
		bpb->b_nhead[1] = 0;
		bpb->b_hiddensec[0] = 0;
		bpb->b_hiddensec[1] = 0;
		if (l_flag) {
			bpb->b_spcl = 2;
			rdirsec = 112;
			bpb->b_mediadescriptor = 0xf9;
			bpb->b_fatsec[0] = 3;
			bpb->b_fatsec[1] = 0;
		} else {
			bpb->b_spcl = 1;
			rdirsec = 224;
			bpb->b_mediadescriptor = 0xf0;
			bpb->b_fatsec[0] = 9;
			bpb->b_fatsec[1] = 0;
		}
		bpb->b_rdirents[0] = getbyte(rdirsec, 1);
		bpb->b_rdirents[1] = getbyte(rdirsec, 0);
		if (write(fd, &bootsec[0], 512) != 512) {
			(void)fprintf(stderr, "%s: write of MS-DOS boot sector failed, ",
			    myname);
			perror(nullstring);
			exit(3);
		}
		(void) memset(fat_rdir, (char)0, 512);
		fatsec = bpb->b_fatsec[0];
		for (i = 0; i < bpb->b_nfat * fatsec; ++i) {
			if ((i % fatsec) == 0) {
				fat_rdir[0] = bpb->b_mediadescriptor;
				fat_rdir[1] = 0xff;
				fat_rdir[2] = 0xff;
			} else {
				fat_rdir[0] = 0;
				fat_rdir[1] = 0;
				fat_rdir[2] = 0;
			}
			if (write(fd, &fat_rdir[0], 512) != 512) {
				(void)fprintf(stderr, "%s: write of MS-DOS File Allocation Table failed, ",
			    	myname);
				perror(nullstring);
				exit(3);
			}
		}
		rdirsec = htols(bpb->b_rdirents[0]) * 32 / 512;
		if (b_flag) {
			struct  timeval tv;
			struct	tm	*tp;
			u_short	dostime;
			u_short	dosday;

			j = min(11, strlen(doslabel)); /* the label can be no */
						       /* more than 11 characters */
			for (i = 0; i < j; i++) {
				fat_rdir[i] = uppercase(doslabel[i]);
			}
			for (; i < 11; i++) {
				fat_rdir[i] = ' ';
			}
			fat_rdir[0xb] = 0x28;
			(void) gettimeofday(&tv, (struct timezone *)0);
			tp = localtime(&tv.tv_sec);
			dostime = tp->tm_sec / 2;   /* get the time & day into DOS format */
			dostime |= tp->tm_min << 5;
			dostime |= tp->tm_hour << 11;
			dosday = tp->tm_mday;
			dosday |= (tp->tm_mon + 1) << 5;
			dosday |= (tp->tm_year - 80) << 9;
			fat_rdir[0x16] = getbyte(dostime, 1);
			fat_rdir[0x17] = getbyte(dostime, 0);
			fat_rdir[0x18] = getbyte(dosday, 1);
			fat_rdir[0x19] = getbyte(dosday, 0);

			if (write(fd, &fat_rdir[0], 512) != 512) {
				(void)fprintf(stderr, "%s: write of MS-DOS root directory failed, ",
			    	myname);
				perror(nullstring);
				exit(3);
			}
			(void) memset(fat_rdir, (char)0, 512);
			i = 1;
		} else {
			i = 0;
		}
		for (; i < rdirsec; ++i) {
			if (write(fd, &fat_rdir[0], 512) != 512) {
				(void)fprintf(stderr, "%s: write of MS-DOS root directory failed, ",
			    	myname);
				perror(nullstring);
				exit(3);
			}
		}
	} else {
		/*
		 *  write a SunOS label
	 	 */
		register short *sp;
		short	sum;
		register short count;

		/* for compatibility, the ascii label */
		asciilabel = "3.5\" floppy cyl 80 alt 0 hd 2 sec 18";
		if (l_flag) {
			asciilabel = "3.5\" floppy cyl 80 alt 0 hd 2 sec 9";
		}
		if (m_flag) {
			asciilabel = "3.5\" floppy cyl 77 alt 0 hd 2 sec 8";
		}
		(void)strcpy(Label.dkl_asciilabel, asciilabel);

		Label.dkl_rpm = m_flag ? 360 : 300;	/* rotations per minute */
		Label.dkl_pcyl = max_cyl;	/* # physical cylinders */
		Label.dkl_apc = 0;	/* alternates per cylinder */
		Label.dkl_intrlv = 1;	/* interleave factor */ /* XXX TBD better? */
		Label.dkl_ncyl = max_cyl;	/* # of data cylinders */
		Label.dkl_acyl = 0;	/* # of alternate cylinders */
		Label.dkl_nhead = 2;	/* # of heads in this partition */
		if (l_flag) {
			Label.dkl_nsect = 9;	/* # of 512 byte sectors per track */
			j = 9;
		} else {
			Label.dkl_nsect = 18;	/* # of 512 byte sectors per track */
			j = 18;
		}
		if (m_flag) {
			Label.dkl_nsect = 8;	/* # of sectors per track */
			j = 8;
		}
		/* logical partitions */
		/* part. "A" is all but last cylinder */
		Label.dkl_map[ 0 ].dkl_cylno = 0;
		Label.dkl_map[ 0 ].dkl_nblk = ((max_cyl - 1) * j * 2) << m_flag;
		/* part. "B" is the last cylinder */
		Label.dkl_map[ 1 ].dkl_cylno = max_cyl - 1;
		Label.dkl_map[ 1 ].dkl_nblk = (j * 2) << m_flag;
		/* part. "C" is the whole diskette */
		Label.dkl_map[ 2 ].dkl_cylno = 0;
		Label.dkl_map[ 2 ].dkl_nblk = (max_cyl * j * 2) << m_flag;
	
		Label.dkl_magic = DKL_MAGIC;	/* identifies this label format */

		/*
		 * This calculates the label checksum
		 * stole this from /usr/src/usr.etc/format/label.c
		 */
		sum = 0;
		count = (sizeof (struct dk_label)) / (sizeof (short));
		/*
		 * we are generating a checksum, don't include the checksum
		 * in the rolling xor.
		 */
		count -= 1;
		sp = (short *)&Label;
		/*
		 * Take the xor of all the half-words in the label.
		 */
		while (count--)
			sum ^= *sp++;
		/* we are generating the checksum, fill it in.  */
		Label.dkl_cksum = sum;		/* xor checksum of sector */
		if (write(fd, (char *)&Label, sizeof (Label)) != sizeof (Label)) {
			(void)fprintf(stderr, "%s: write of SunOS label failed, ",
			    myname);
			perror(nullstring);
			exit(3);
		}
	}

	if (e_flag) {
/* XXX do ioctl to query ejectability of diskette */
		/* eject it if possible */
		if (ioctl(fd, FDKEJECT, 0)) {
			(void)fprintf(stderr, "%s: could not eject diskette, ",
			    myname);
			perror(nullstring);
			exit(3);
		}
	}
	exit(0);
	/* NOTREACHED */
}

usage(str)
char	*str;
{
	(void)printf("fdformat: %s\n", str);
	(void)printf("   usage: fdformat [-eflv] [devname] %s\n");
	(void)printf("      -b label install \"label\" on MS-DOS floppy\n");
	(void)printf("      -d       format MS-DOS floppy\n");
	(void)printf("      -e       eject the diskette when done\n");
	(void)printf("      -f       \"force\" - don't wait for confirmation\n");
	(void)printf("      -l       format 720k low density floppy\n");
#ifdef FD_MEDIUM_DENSITY
	(void)printf("      -m       format 1.2  Med density floppy\n");
#endif
	(void)printf("      -v       verify each block of the diskette\n");
	(void)printf("      devname defaults to %s\n", dev_name);
	exit(1);
}

int	inited = 0;

verify(fd, blk, len)
int	fd;
int	blk;
int	len;
{
	off_t	off;

	if (! inited) {
		if ((ibuf1 = malloc((unsigned)(len))) == 0 ||
		    (obuf = malloc((unsigned)(len))) == 0) {
			(void)fprintf(stderr,
			    "%s: can't malloc verify buffer, ", myname);
			perror(nullstring);
			exit(4);
		}
		inited = 1;
		(void)memset(ibuf1, (char)0xA5, len);
	}

	off = (off_t)(blk * (m_flag ? 1024 : 512));

	if (lseek(fd, off, 0) != off) {
		(void)fprintf(stderr, "%s: can't seek to write verify, ",
		    myname);
		perror(nullstring);
		exit(4);
	}
	if (write(fd, ibuf1, len) != len) {
		if (blk == 0)
			(void)fprintf(stderr, "%s: check density?, ", myname);
		else
			(void)fprintf(stderr, "%s: can't write verify data, ",
			    myname);
		perror(nullstring);
		exit(4);
	}
	if (lseek(fd, off, 0) != off) {
		(void)fprintf(stderr, "%s: bad seek to read verify, ", myname);
		perror(nullstring);
		exit(4);
	}
	if (read(fd, obuf, len) != len) {
		(void)fprintf(stderr, "%s: can't read verify data, ", myname);
		perror(nullstring);
		exit(4);
	}
	if (memcmp(ibuf1, obuf, len)) {
		(void)fprintf(stderr, "%s: verify data failure", myname);
		exit(4);
	}
}
