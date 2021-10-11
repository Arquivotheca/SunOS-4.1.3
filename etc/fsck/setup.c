/*
 * Copyright (c) 1980, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ident	"@(#)setup.c 1.1 92/07/30 SMI" /* from UCB 5.3 5/15/86 */

#define	DKTYPENAMES
#include <stdio.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <mntent.h>
#include <strings.h>
#include "fsck.h"

struct bufarea asblk;
#define	altsblock (*asblk.b_un.b_fs)
#define	POWEROF2(num)	(((num) & ((num) - 1)) == 0)

/*
 * The size of a cylinder group is calculated by CGSIZE. The maximum size
 * is limited by the fact that cylinder groups are at most one block.
 * Its size is derived from the size of the maps maintained in the
 * cylinder group and the (struct cg) size.
 */
#define	CGSIZE(fs) \
    /* base cg */	(sizeof (struct cg) + \
    /* blktot size */	(fs)->fs_cpg * sizeof (long) + \
    /* blks size */	(fs)->fs_cpg * (fs)->fs_nrpos * sizeof (short) + \
    /* inode map */	howmany((fs)->fs_ipg, NBBY) + \
    /* block map */	howmany((fs)->fs_cpg * (fs)->fs_spc / NSPF(fs), NBBY))

char *
setup(dev)
	char *dev;
{
	dev_t rootdev;
	long size, i, j;
	long bmapsize;
	struct stat statb;
	static char devstr[MAXPATHLEN];
	char *raw, *rawname(), *unrawname();
	static void write_altsb();

	havesb = 0;
	if (stat("/", &statb) < 0)
		errexit("Can't stat root\n");
	rootdev = statb.st_dev;

	devname = devstr;
	strncpy(devstr, dev, sizeof (devstr));
restat:
	if (stat(devstr, &statb) < 0) {
		printf("Can't stat %s\n", devstr);
		return (0);
	}
	if ((statb.st_mode & S_IFMT) == S_IFDIR) {
		FILE *fstab;
		struct mntent *mnt;
		/*
		 * Check fstab for a mount point with this name
		 */
		if ((fstab = setmntent(MNTTAB, "r")) == NULL) {
			errexit("Can't open checklist file: %s\n", MNTTAB);
		}
		while (mnt = getmntent(fstab)) {
			if (strcmp(devstr, mnt->mnt_dir) == 0) {
				if (strcmp(mnt->mnt_type, MNTTYPE_42) != 0) {
					/*
					 * found the entry but it is not a
					 * 4.2 filesystem, don't check it
					 */
					endmntent(fstab);
					return (0);
				}
				strcpy(devstr, mnt->mnt_fsname);
				if (rflag) {
					raw =
					    rawname(unrawname(mnt->mnt_fsname));
					strcpy(devstr, raw);
				}
				endmntent(fstab);
				goto restat;
			}
		}
		/*
		 * Not found
		 */
		printf("%s not in fstab\n", dev);
		endmntent(fstab);
		return (0);
	} else if (((statb.st_mode & S_IFMT) != S_IFBLK) &&
	    ((statb.st_mode & S_IFMT) != S_IFCHR))
		if (!debug &&
		    reply("file is not a block or character device; OK") == 0)
			return (0);

	if (mounted(devstr))
		mountedfs++;
	if (rflag) {
		char blockname[MAXPATHLEN];
		/*
		 * For root device check, must check
		 * block devices.
		 */
		strcpy(blockname, devstr);
		if (stat(unrawname(blockname), &statb) < 0) {
			printf("Can't stat %s\n", blockname);
			return (0);
		}
	}
	if (rootdev == statb.st_rdev)
		hotroot++;
	if ((fsreadfd = open(devstr, O_RDONLY)) < 0) {
		printf("Can't open %s\n", devstr);
		return (0);
	}
	if (preen == 0 || debug != 0)
		printf("** %s", devstr);
	if (nflag || (fswritefd = open(devstr, O_WRONLY)) < 0) {
		fswritefd = -1;
		if (preen && !debug)
			pfatal("(NO WRITE ACCESS)\n");
		printf(" (NO WRITE)");
	}
	if (preen == 0)
		printf("\n");
	else if (debug)
		printf(" pid %d\n", getpid());
	if (debug && (hotroot || mountedfs)) {
		printf("** %s", devstr);
		if (hotroot) {
			printf(" is root fs");
			if (mountedfs)
				printf(" and");
		}
		if (mountedfs)
			printf(" is mounted");
		printf(".\n");
	}
	fsmodified = 0;
	isdirty = 0;
	lfdir = 0;
	initbarea(&sblk);
	initbarea(&asblk);
	sblk.b_un.b_buf = malloc(SBSIZE);
	asblk.b_un.b_buf = malloc(SBSIZE);
	if (sblk.b_un.b_buf == NULL || asblk.b_un.b_buf == NULL)
		errexit("cannot allocate space for superblock\n");
	/*
	 * Read in the superblock, looking for alternates if necessary
	 */
	if (readsb(1) == 0)
		return (0);
	maxfsblock = sblock.fs_size;
	maxino = sblock.fs_ncg * sblock.fs_ipg;
	/*
	 * Check and potentially fix certain fields in the super block.
	 */
	if (sblock.fs_optim != FS_OPTTIME && sblock.fs_optim != FS_OPTSPACE) {
		pfatal("UNDEFINED OPTIMIZATION IN SUPERBLOCK");
		if (reply("SET TO DEFAULT") == 1) {
			sblock.fs_optim = FS_OPTTIME;
			sbdirty();
		}
	}
	if ((sblock.fs_minfree < 0 || sblock.fs_minfree > 99)) {
		pfatal("IMPOSSIBLE MINFREE=%d IN SUPERBLOCK",
			sblock.fs_minfree);
		if (reply("SET TO DEFAULT") == 1) {
			sblock.fs_minfree = 10;
			sbdirty();
		}
	}
	if (cvtflag) {
		if (sblock.fs_postblformat == FS_42POSTBLFMT) {
			/*
			 * Requested to convert from old format to new format
			 */
			if (preen)
				pwarn("CONVERTING TO NEW FILE SYSTEM FORMAT\n");
			else if (!reply("CONVERT TO NEW FILE SYSTEM FORMAT"))
				return (0);
			isconvert = 1;
			sblock.fs_postblformat = FS_DYNAMICPOSTBLFMT;
			sblock.fs_nrpos = 8;
			sblock.fs_npsect = sblock.fs_nsect;
			sblock.fs_interleave = 1;
			sblock.fs_postbloff =
			    (char *)(&sblock.fs_opostbl[0][0]) -
			    (char *)(&sblock.fs_link);
			sblock.fs_rotbloff = &sblock.fs_space[0] -
			    (u_char *)(&sblock.fs_link);
			sblock.fs_cgsize =
				fragroundup(&sblock, CGSIZE(&sblock));
			/*
			 * Planning now for future expansion.
			 */
#if defined(mc68000) || defined(sparc)
				sblock.fs_qbmask.val[0] = 0;
				sblock.fs_qbmask.val[1] = ~sblock.fs_bmask;
				sblock.fs_qfmask.val[0] = 0;
				sblock.fs_qfmask.val[1] = ~sblock.fs_fmask;
#endif
#if defined(vax) || defined(i386)
				sblock.fs_qbmask.val[0] = ~sblock.fs_bmask;
				sblock.fs_qbmask.val[1] = 0;
				sblock.fs_qfmask.val[0] = ~sblock.fs_fmask;
				sblock.fs_qfmask.val[1] = 0;
#endif
			sbdirty();
			write_altsb(fswritefd);
		} else if (sblock.fs_postblformat == FS_DYNAMICPOSTBLFMT) {
			/*
			 * Requested to convert from new format to old format
			 */
			if (sblock.fs_nrpos != 8 || sblock.fs_ipg > 2048 ||
			    sblock.fs_cpg > 32 || sblock.fs_cpc > 16) {
				printf(
				"PARAMETERS OF CURRENT FILE SYSTEM DO NOT\n\t");
				errexit(
				"ALLOW CONVERSION TO OLD FILE SYSTEM FORMAT\n");
			}
			if (preen)
				pwarn("CONVERTING TO OLD FILE SYSTEM FORMAT\n");
			else if (!reply("CONVERT TO OLD FILE SYSTEM FORMAT"))
				return (0);
			isconvert = 1;
			sblock.fs_postblformat = FS_42POSTBLFMT;
			sblock.fs_cgsize = fragroundup(&sblock,
			    sizeof (struct ocg) + howmany(sblock.fs_fpg, NBBY));
			sblock.fs_npsect = 0;
			sblock.fs_interleave = 0;
			sbdirty();
			write_altsb(fswritefd);
		} else {
			errexit("UNKNOWN FILE SYSTEM FORMAT\n");
		}
	}
	/*
	 * read in the summary info.
	 */
	for (i = 0, j = 0; i < sblock.fs_cssize; i += sblock.fs_bsize, j++) {
		size = MIN(sblock.fs_cssize - i, sblock.fs_bsize);
		sblock.fs_csp[j] = (struct csum *)calloc(1, (unsigned)size);
		if (sblock.fs_csp[j] == NULL)
			errexit("cannot allocate space for cg summary\n");
		if (bread(fsreadfd, (char *)sblock.fs_csp[j],
		    fsbtodb(&sblock, sblock.fs_csaddr + j * sblock.fs_frag),
		    size) != 0)
			return (0);
	}
	/*
	 * if not forced, preening, not converting, and is clean; stop checking
	 */
	if ((fflag == 0) && preen && (isconvert == 0) &&
	    (FSOKAY == (fs_get_state(&sblock) + sblock.fs_time)) &&
	    ((sblock.fs_clean == FSCLEAN) ||
	    (sblock.fs_clean == FSSTABLE))) {
		iscorrupt = 0;
		printclean();
		return (0);
	}
	/*
	 * allocate and initialize the necessary maps
	 */
	bmapsize = roundup(howmany(maxfsblock, NBBY), sizeof (short));
	blockmap = calloc((unsigned)bmapsize, sizeof (char));
	if (blockmap == NULL) {
		printf("cannot alloc %d bytes for blockmap\n", bmapsize);
		goto badsb;
	}
	statemap = calloc((unsigned)(maxino + 1), sizeof (char));
	if (statemap == NULL) {
		printf("cannot alloc %d bytes for statemap\n", maxino + 1);
		goto badsb;
	}
	lncntp = (short *)calloc((unsigned)(maxino + 1), sizeof (short));
	if (lncntp == NULL) {
		printf("cannot alloc %d bytes for lncntp\n",
		    (maxino + 1) * sizeof (short));
		goto badsb;
	}
	if ((numdirs = sblock.fs_cstotal.cs_ndir) == 0) numdirs++;
	listmax = numdirs + 10;
	inpsort = (struct inoinfo **)calloc((unsigned)listmax,
	    sizeof (struct inoinfo *));
	inphead = (struct inoinfo **)calloc((unsigned)numdirs,
	    sizeof (struct inoinfo *));
	if (inpsort == NULL || inphead == NULL) {
		printf("cannot alloc %d bytes for inphead\n",
		    numdirs * sizeof (struct inoinfo *));
		goto badsb;
	}
	inplast = 0L;
	bufinit();
	return (devstr);

badsb:
	ckfini();
	return (0);
}

/*
 * Read in the super block and its summary info.
 */
readsb(listerr)
	int listerr;
{
	daddr_t super = bflag ? bflag : SBLOCK;

	if (bread(fsreadfd, (char *)&sblock, super, (long)SBSIZE) != 0)
		return (0);
	sblk.b_bno = super;
	sblk.b_size = SBSIZE;
	/*
	 * run a few consistency checks of the super block
	 */
	if (sblock.fs_magic != FS_MAGIC)
		{ badsb(listerr, "MAGIC NUMBER WRONG"); return (0); }
	if (sblock.fs_ncg < 1)
		{ badsb(listerr, "NCG OUT OF RANGE"); return (0); }
	if (sblock.fs_cpg < 1)
		{ badsb(listerr, "CPG OUT OF RANGE"); return (0); }
	if (sblock.fs_ncg * sblock.fs_cpg < sblock.fs_ncyl ||
	    (sblock.fs_ncg - 1) * sblock.fs_cpg >= sblock.fs_ncyl)
		{ badsb(listerr,
		    "NCYL DOES NOT JIVE WITH NCG*CPG"); return (0); }
	if (sblock.fs_sbsize < 0 || sblock.fs_sbsize > SBSIZE)
		{ badsb(listerr, "SIZE PREPOSTEROUSLY LARGE"); return (0); }
	/*
	 * Set all possible fields that could differ, then do check
	 * of whole super block against an alternate super block.
	 * When an alternate super-block is specified this check is skipped.
	 */
	getblk(&asblk, cgsblock(&sblock, sblock.fs_ncg - 1), sblock.fs_sbsize);
	if (asblk.b_errs)
		return (0);
	if (bflag) {
		/*
		 * Invalidate clean flag and state information
		 */
		sblock.fs_clean = FSACTIVE;
		fs_set_state(&sblock, (long)sblock.fs_time);
		sbdirty();
		havesb = 1;
		return (1);
	}
	altsblock.fs_link = sblock.fs_link;
	altsblock.fs_rlink = sblock.fs_rlink;
	altsblock.fs_time = sblock.fs_time;
	fs_set_state(&altsblock, fs_get_state(&sblock));
	altsblock.fs_cstotal = sblock.fs_cstotal;
	altsblock.fs_cgrotor = sblock.fs_cgrotor;
	altsblock.fs_fmod = sblock.fs_fmod;
	altsblock.fs_clean = sblock.fs_clean;
	altsblock.fs_ronly = sblock.fs_ronly;
	altsblock.fs_flags = sblock.fs_flags;
	altsblock.fs_maxcontig = sblock.fs_maxcontig;
	altsblock.fs_minfree = sblock.fs_minfree;
	altsblock.fs_optim = sblock.fs_optim;
	altsblock.fs_rotdelay = sblock.fs_rotdelay;
	altsblock.fs_maxbpg = sblock.fs_maxbpg;
	bcopy((char *)sblock.fs_csp, (char *)altsblock.fs_csp,
		sizeof (sblock.fs_csp));
	bcopy((char *)sblock.fs_fsmnt, (char *)altsblock.fs_fsmnt,
		sizeof (sblock.fs_fsmnt));
	bcopy((char *)sblock.fs_sparecon, (char *)altsblock.fs_sparecon,
		sizeof (sblock.fs_sparecon));
	/*
	 * The following should not have to be copied.
	 */
	altsblock.fs_fsbtodb = sblock.fs_fsbtodb;
	altsblock.fs_interleave = sblock.fs_interleave;
	altsblock.fs_npsect = sblock.fs_npsect;
	altsblock.fs_nrpos = sblock.fs_nrpos;
	/*
	 * in case of an aborted growfs
	 */
	altsblock.fs_dsize = sblock.fs_dsize;
	altsblock.fs_size = sblock.fs_size;
	altsblock.fs_ncyl = sblock.fs_ncyl;
	altsblock.fs_ncg = sblock.fs_ncg;
	altsblock.fs_csshift = sblock.fs_csshift;
	altsblock.fs_csmask = sblock.fs_csmask;
	altsblock.fs_csaddr = sblock.fs_csaddr;
	altsblock.fs_cssize = sblock.fs_cssize;
	if (bcmp((char *)&sblock, (char *)&altsblock, (int)sblock.fs_sbsize)) {
		badsb(listerr, "TRASHED VALUES IN SUPER BLOCK"); return (0);
	}
	havesb = 1;
	return (1);
}

badsb(listerr, s)
	int listerr;
	char *s;
{

	if (!listerr)
		return;
	if (preen)
		printf("%s: ", devname);
	printf("BAD SUPER BLOCK: %s\n", s);
	pwarn("USE -b OPTION TO FSCK TO SPECIFY LOCATION OF AN ALTERNATE\n");
	pfatal("SUPER-BLOCK TO SUPPLY NEEDED INFORMATION; SEE fsck(8).\n");
}

/*
 * Write out the super block into each of the alternate super blocks.
 */
static void
write_altsb(fd)
	int fd;
{
	int cylno;

	for (cylno = 0; cylno < sblock.fs_ncg; cylno++)
		bwrite(fd, (char *)&sblock, fsbtodb(&sblock,
		    cgsblock(&sblock, cylno)), sblock.fs_sbsize);
}
