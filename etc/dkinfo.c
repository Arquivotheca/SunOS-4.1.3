#ifndef lint
static	char sccsid[] = "@(#)dkinfo.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Report information about a disk's geometry and partitioning
 *
 * Usage: dkinfo XXN
 * where XX is controller name (ip, xy, etc) and N is disk number.
 * or dkinfo rawname where rawname is a character special file for a disk.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <sun/dklabel.h>
#include <sun/dkio.h>

struct dk_geom g;
struct dk_map p;
struct dk_info inf;

main(argc, argv)
	char **argv;
{
	while (--argc)
		dk(*++argv);
	exit(0);
	/* NOTREACHED */
}

dk(name)
	char *name;
{
	char nbuf[100];
	struct stat st;
	int fd, c, found, nparts;

	sprintf(nbuf, "/dev/r%s", name);
	if (stat(nbuf, &st) == 0) {
		fd = open(nbuf, 0);
		if (fd < 0) {
			perror(nbuf);
			return;
		}
		if (isdisk(fd)) {
			info(fd);
			geom(fd);
			if (parts(0, fd) == 0)
				printf("Not a valid partition.\n");
		} else
			printf("%s: Not a valid disk.\n", name);
		close(fd);
		return;
	}

	found = 0;
	nparts = 0;
	/* first, get disk goemetry */
	for (c = 'a'; c <= 'h' && !found; c++) {
		sprintf(nbuf, "/dev/r%s%c", name, c);
		if (stat(nbuf, &st))
			continue;
		fd = open(nbuf, 0);
		if (fd < 0)
			continue;
		if (isdisk(fd)) {
			printf("%s: ", name);
			info(fd);
			geom(fd);
			found = 1;
		}
		close(fd);
	}
	if (!found) {
		printf("%s: no such disk\n", name);
		return;
	}
	/* now, get partition information */
	for (c = 'a'; c <= 'h'; c++) {
		sprintf(nbuf, "/dev/r%s%c", name, c);
		if (stat(nbuf, &st))
			continue;
		fd = open(nbuf, 0);
		if (fd < 0) {
			nbuf[0] = c;
			nbuf[1] = 0;
			perror(nbuf);
			continue;
		}
		nparts += parts(c, fd);
		close(fd);
	}
	if (nparts == 0)
		printf("%s: no valid partitions\n", name);
}

char *ctlrname[] = {
	"Unknown",			/* 0 */
	"Unknown",			/* 1 used to be ip2180 */
	"Unknown",			/* 2 used to be wdc2880 */
	"Unknown",			/* 3 used to be ip2181 */
	"Unknown",			/* 4 used to be xy440 */
	"Unknown",			/* 5 used to be dsd5215 */
	"Xylogics 450/451",		/* 6 */
	"Adaptec ACB4000",		/* 7 */
	"Emulex MD21",			/* 8 */
	"Unknown",			/* 9 used to be xd751 */
	"SCSI Floppy",			/* 10 */
	"Xylogics 7053",		/* 11 */
	"SCSI SMS Floppy",		/* 12 */
	"SCSI CCS",			/* 13 */
	"Onboard 82072 Floppy",		/* 14 */
	"IPI Panther",			/* 15 */
	"Unknown",			/* 16 meta disk */
	"Unknown",			/* 17 CDC */
	"Unknown",			/* 18 fujitsu */
	"Onboard 82077 Floppy",		/* 19 */
};
#define	NCTLR	(sizeof (ctlrname)/sizeof (ctlrname[0]))
#define	CTLRNAME(n)	ctlrname[(n) >= NCTLR ? 0 : (n)]

isdisk(fd)
{
	return (ioctl(fd, DKIOCINFO, &inf) == 0 &&
	    ioctl(fd, DKIOCGGEOM, &g) == 0);
}

info(fd)
{
	printf("%s controller at addr %x, unit # %d\n",
		CTLRNAME(inf.dki_ctype), inf.dki_ctlr, inf.dki_unit);
}

geom(fd)
{
	printf("%d cylinders", g.dkg_ncyl);
	if (g.dkg_bcyl)
		printf(" (base %d)", g.dkg_bcyl);
	printf(" %d heads", g.dkg_nhead);
	if (g.dkg_bhead)
		printf(" (base %d)", g.dkg_bhead);
	printf(" %d sectors/track\n", g.dkg_nsect);
}

parts(pa, fd)
{
	int n;

	if (ioctl(fd, DKIOCGPART, &p) == 0) {
		if (p.dkl_nblk == 0)
			return (0);
		if (pa) printf("%c: ", pa);
		printf("%d sectors ", p.dkl_nblk);
		printf("(%d cyls", p.dkl_nblk/(g.dkg_nhead*g.dkg_nsect));
		n = (p.dkl_nblk % (g.dkg_nhead*g.dkg_nsect)) / g.dkg_nsect;
		if (n) printf(", %d tracks", n);
		n = p.dkl_nblk % g.dkg_nsect;
		if (n) printf(", %d sectors", n);
		printf(")\n");
		if (pa) printf("   ");
		printf("starting cylinder %d\n", p.dkl_cylno);
		return (p.dkl_nblk ? 1 : 0);
	} else {
		if (pa) printf("%c: ", pa);
		printf("can't get partition info\n");
		return (0);
	}
}
