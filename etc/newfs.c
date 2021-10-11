#ident	"@(#)newfs.c 1.1 92/07/30 SMI"	/* from UCB 5.2 9/11/85 */
/*
 * newfs: friendly front end to mkfs
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <ufs/fs.h>
#include <sys/dir.h>

#include <stdio.h>
#include <disktab.h>

#include <sys/ioctl.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
struct disktab *getdiskbydev();

#define	EPATH "PATH=/bin:/usr/bin:/usr/etc:"

int	Nflag;			/* run mkfs without writing file system */
int	verbose;		/* show mkfs line before exec */
int	fssize;			/* file system size */
int	fsize;			/* fragment size */
int	bsize;			/* block size */
int	ntracks;		/* # tracks/cylinder */
int	nsectors;		/* # sectors/track */
int	cpg;			/* cylinders/cylinder group */
int	minfree = -1;		/* free space threshold */
int	opt;			/* optimization preference (space or time) */
int	rpm;			/* revolutions/minute of drive */
int	nrpos = 8;		/* # of distinguished rotational positions */
				/* 8 is the historical default */
int	density;		/* number of bytes per inode */
int	apc;			/* alternates per cylinder */
int 	rot = -1;		/* rotational delay (msecs) */
int	maxcontig = -1;		/* maximum number of contig blocks */

char	device[MAXPATHLEN];
char	cmd[BUFSIZ];

extern	char *index();
extern	char *strcpy();
extern	char *strcat();
extern	char *sprintf();
extern	char *getenv();
extern	char *malloc();
extern	void exit();

main(argc, argv)
	char *argv[];
{
	static char *nmiss = "-n: missing # of rotational positions\n";
	static char *nmiss2 = "%s: bad # of rotational positions\n";
	static char *amiss = "-a: missing # of alternates per cyl";
	static char *omiss = "-o: missing optimization preference";
	static char *badop = "%s: bad optimization preference %s";
	char *cp, *special;
	register struct disktab *dp;
	register struct partition *pp;
	struct stat st;
	struct dk_info dkinfo;
	int fd, status, cacheing_drive;

	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++)
			switch (*cp) {

			case 'v':
				verbose++;
				break;

			case 'N':
				Nflag++;
				break;

			case 's':
				if (argc < 1)
					fatal("-s: missing file system size");
				argc--, argv++;
				fssize = atoi(*argv);
				if (fssize < 0)
					fatal("%s: bad file system size",
						*argv);
				goto next;

			case 'C':
				if (argc < 1)
					fatal("-C: missing maxcontig");
				argc--, argv++;
				maxcontig = atoi(*argv);
				if (maxcontig <= 0)
					fatal("%s: bad maxcontig", *argv);
				goto next;

			case 'd':
				if (argc < 1)
					fatal("-d: missing rotational delay");
				argc--, argv++;
				rot = atoi(*argv);
				if (rot < 0)
					fatal("%s: bad rotational delay",
					    *argv);
				goto next;

			case 't':
				if (argc < 1)
					fatal("-t: missing track total");
				argc--, argv++;
				ntracks = atoi(*argv);
				if (ntracks < 0)
					fatal("%s: bad total tracks", *argv);
				goto next;

			case 'o':
				if (argc < 1)
					fatal(omiss);
				argc--, argv++;
				if (strcmp(*argv, "space") == 0)
					opt = FS_OPTSPACE;
				else if (strcmp(*argv, "time") == 0)
					opt = FS_OPTTIME;
				else
					fatal(badop, *argv,
					    "(options are `space' or `time')");
				goto next;

			case 'a':
				if (argc < 1)
					fatal(amiss);
				argc--, argv++;
				apc = atoi(*argv);
				if (apc < 0)
					fatal("%s: bad altenates per cyl ",
					    *argv);
				goto next;

			case 'b':
				if (argc < 1)
					fatal("-b: missing block size");
				argc--, argv++;
				bsize = atoi(*argv);
				if (bsize < 0 || bsize < MINBSIZE)
					fatal("%s: bad block size", *argv);
				goto next;

			case 'f':
				if (argc < 1)
					fatal("-f: missing frag size");
				argc--, argv++;
				fsize = atoi(*argv);
				if (fsize < 0)
					fatal("%s: bad frag size", *argv);
				goto next;

			case 'c':
				if (argc < 1)
					fatal("-c: missing cylinders/group");
				argc--, argv++;
				cpg = atoi(*argv);
				if (cpg < 0)
					fatal("%s: bad cylinders/group", *argv);
				goto next;

			case 'm':
				if (argc < 1)
					fatal("-m: missing free space %%\n");
				argc--, argv++;
				minfree = atoi(*argv);
				if (minfree < 0 || minfree > 99)
					fatal("%s: bad free space %%\n",
						*argv);
				goto next;

			case 'n':
				if (argc < 1)
					fatal(nmiss);
				argc--, argv++;
				nrpos = atoi(*argv);
				if (nrpos < 0)
					fatal(nmiss2, *argv);
				goto next;

			case 'r':
				if (argc < 1)
					fatal("-r: missing revs/minute\n");
				argc--, argv++;
				rpm = atoi(*argv);
				if (rpm < 0)
					fatal("%s: bad revs/minute\n", *argv);
				goto next;

			case 'i':
				if (argc < 1)
					fatal("-i: missing bytes per inode\n");
				argc--, argv++;
				density = atoi(*argv);
				if (density < 0)
					fatal("%s: bad bytes per inode\n",
						*argv);
				goto next;

			default:
				fatal("-%c: unknown flag", *cp);
			}
next:
		argc--, argv++;
	}
	if (argc < 1) {
		fprintf(stderr, "usage: newfs [ -v ] [ mkfs-options ] %s\n",
			"raw-special-device");
		fprintf(stderr, "where mkfs-options are:\n");
		fprintf(stderr, "\t-N do not create file system, %s\n",
			"just print out parameters");
		fprintf(stderr, "\t-s file system size (sectors)\n");
		fprintf(stderr, "\t-b block size\n");
		fprintf(stderr, "\t-f frag size\n");
		fprintf(stderr, "\t-t tracks/cylinder\n");
		fprintf(stderr, "\t-c cylinders/group\n");
		fprintf(stderr, "\t-m minimum free space %%\n");
		fprintf(stderr, "\t-o optimization preference %s\n",
			"(`space' or `time')");
		fprintf(stderr, "\t-r revolutions/minute\n");
		fprintf(stderr, "\t-i number of bytes per inode\n");
		fprintf(stderr, "\t-a number of alternates per cylinder\n");
		fprintf(stderr, "\t-C maxcontig\n");
		fprintf(stderr, "\t-d rotational delay\n");
		fprintf(stderr, "\t-n number of rotational positions\n");
		exit(1);
	}
	special = argv[0];
again:
	if (stat(special, &st) < 0) {
		if (*special != '/') {
			if (*special == 'r')
				special++;
			special = sprintf(device, "/dev/r%s", special);
			goto again;
		}
		fprintf(stderr, "newfs: "); perror(special);
		exit(2);
	}
	if ((st.st_mode & S_IFMT) != S_IFCHR)
		fatal("%s: not a raw disk device", special);
	dp = getdiskbydev(special);
	if (dp == 0)
		fatal("%s: unknown disk information", argv[1]);
	cp = index(argv[0], '\0') - 1;
	if (cp == 0 || *cp < 'a' || *cp > 'h')
		fatal("%s: can't figure out file system partition", argv[0]);
	pp = &dp->d_partitions[*cp - 'a'];
	if (fssize == 0) {
		fssize = pp->p_size;
		if (fssize < 0)
			fatal("%s: no default size for `%c' partition",
				argv[1], *cp);
	}
	if (nsectors == 0) {
		nsectors = dp->d_nsectors;
		if (nsectors < 0)
			fatal("%s: no default #sectors/track", argv[1]);
	}
	if (ntracks == 0) {
		ntracks = dp->d_ntracks;
		if (ntracks < 0)
			fatal("%s: no default #tracks", argv[1]);
	}
	if (bsize == 0) {
		bsize = pp->p_bsize;
		if (bsize < 0)
			fatal("%s: no default block size for `%c' partition",
				argv[1], *cp);
	}
	if (fsize == 0) {
		fsize = pp->p_fsize;
		if (fsize < 0)
			fatal("%s: no default frag size for `%c' partition",
				argv[1], *cp);
	}
	if (rpm == 0) {
		rpm = dp->d_rpm;
		if (rpm < 0)
			fatal("%s: no default revolutions/minute value",
				argv[1]);
	}
	if ((fd = open(special, 2)) == -1)
		fatal("can't open %s", special);
	/*
	 * Best default depends on controller type.
	 *
	 * The controller should tell us (via some
	 * ioctl) what rotdelay and maxcontig it might
	 * prefer.
	 *
	 * For both rotdelay and maxcontig we need to infer
	 * from both controller type and, in the SCSI case,
	 * additional ioctl information, whether it might
	 * be a good idea to set rotdelay to zero and
	 * maxcontig to a reasonably large default value
	 * (in the absence of a command argument overriding
	 * either).
	 */

	cacheing_drive = 0;
	if (ioctl(fd, DKIOCINFO, &dkinfo) >= 0) {
		switch (dkinfo.dki_ctype) {
		    case DKC_MD21:
		    case DKC_SCSI_CCS:
			if (scsidisk_has_cache(fd) == 0) {
				break;
			}
		    /* FALLTHROUGH */
		    case DKC_XD7053:
		    case DKC_SUN_IPI1:
		    case DKC_CDC_9057:
		    case DKC_FJ_M1060:
			cacheing_drive = 1;
			break;
		}
	}

	if (rot == -1 && cacheing_drive) {
		rot = 0;
	}
	if (maxcontig == -1 && cacheing_drive) {
		/*
		 * As long as maxphys is commonly 56K
		 * we need to use 7.  If we grow it to
		 * 124K on all our machines, we can grow
		 * maxcontig to 15.
		 */
		maxcontig = 7;
	}
	close(fd);
	/* XXX - following defaults are both here and in mkfs */
	if (density <= 0)
		density = 2048;
	if (cpg == 0)
		cpg = 16;
	if (minfree < 0)
		minfree = 10;
	if (minfree < 10 && opt != FS_OPTSPACE) {
		fprintf(stderr, "setting optimization for space ");
		fprintf(stderr, "with minfree less than 10%\n");
		opt = FS_OPTSPACE;
	}
	/*
	 * If alternates-per-cylinder is ever implemented:
	 * need to get apc from dp->d_apc if no -a switch???
	 */
	(void) sprintf(cmd,
	    "mkfs %s%s %d %d %d %d %d %d %d %d %d %s %d %d %d %d",
	    Nflag ? "-N " : " ", special,
	    fssize, nsectors, ntracks, bsize, fsize, cpg, minfree, rpm/60,
	    density, opt == FS_OPTSPACE ? "s" : "t", apc, rot, nrpos,
	    maxcontig);

	if (verbose)
		printf("%s\n", cmd);
	exenv();
	if (status = system(cmd))
		exit(status >> 8);
	if (Nflag)
		exit(0);
	(void)sprintf(cmd, "fsirand %s", special);
	if (status = system(cmd))
		printf("%s: failed, status = %d\n", cmd, status);

	exit(0);
	/* NOTREACHED */
}

exenv()
{
	char *epath;				/* executable file path */
	char *cpath;				/* current path */

	if ((cpath = getenv("PATH")) == NULL) {
		fprintf(stderr, "newfs: no entry in env\n");
	}
	if ((epath = malloc((unsigned)strlen(EPATH)+strlen(cpath)+1)) == NULL) {
		fprintf(stderr, "newfs: malloc failed\n");
		exit(1);
	}
	strcpy(epath, EPATH);
	strcat(epath, cpath);
	if (putenv(epath) < 0) {
		fprintf(stderr, "newfs: putenv failed\n");
		exit(1);
	}
}

/*VARARGS*/
fatal(fmt, arg1, arg2)
	char *fmt;
{

	fprintf(stderr, "newfs: ");
	fprintf(stderr, fmt, arg1, arg2);
	putc('\n', stderr);
	exit(10);
}

struct disktab *
getdiskbydev(disk)
	char *disk;
{
	static struct disktab d;
	struct dk_geom g;
	struct dk_map m;
	int fd, part;

	part = disk[strlen(disk)-1] - 'a';
	if ((fd = open(disk, 0)) < 0) {
		perror(disk);
		exit(1);
	}
	if (ioctl(fd, DKIOCGGEOM, &g) < 0) {
		perror("geom ioctl");
		exit(1);
	}
	d.d_secsize = 512;
	d.d_ntracks = g.dkg_nhead;
	d.d_nsectors = g.dkg_nsect;
	d.d_ncylinders = g.dkg_ncyl;
	d.d_rpm = g.dkg_rpm;
	if (d.d_rpm <= 0)
		d.d_rpm = 3600;
	d.d_apc = g.dkg_apc;
	if (ioctl(fd, DKIOCGPART, &m) < 0) {
		perror("part ioctl");
		exit(1);
	}
	close(fd);
	d.d_partitions[part].p_size = m.dkl_nblk;
	d.d_partitions[part].p_bsize = 8192;
	d.d_partitions[part].p_fsize = 1024;
	return (&d);
}


#include <scsi/scsi.h>

#define	MSDSIZE	(MODE_HEADER_LENGTH + sizeof (struct block_descriptor) + 32)

static int
modesense(localbuf, length, param, fd)
char *localbuf;
int length, param, fd;
{
	auto struct dk_cmd cmdblk;

	bzero(localbuf, length);
	bzero((char *)&cmdblk, sizeof (cmdblk));
	cmdblk.dkc_bufaddr = localbuf;
	cmdblk.dkc_buflen = length;
	cmdblk.dkc_blkno = param;
	cmdblk.dkc_cmd = SCMD_MODE_SENSE;
	cmdblk.dkc_flags = DK_ISOLATE|DK_SILENT;
	return (ioctl(fd, DKIOCSCMD, (char *) &cmdblk));
}

int
scsidisk_has_cache(fd)
int fd;
{
	struct mode_header *mh;
	struct mode_page *mp;
	auto char buf[MSDSIZE];
	static int page_codes[] = { 0x8, 0x38, -1 };
	int i;

	mh = (struct mode_header *) buf;

	for (i = 0; page_codes[i] != -1; i++) {
		(void) bzero (buf, MSDSIZE);
		if (modesense(buf, MSDSIZE, page_codes[i], fd) == 0) {
			mp = MODE_PAGE_ADDR(mh, struct mode_page);
			if (mp && mp->code && mp->length) {
				return (1);
			}
		}
	}
	return (0);
}
