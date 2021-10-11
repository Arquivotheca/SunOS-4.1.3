#ifndef lint
static	char *sccsid = "@(#)icheck.c 1.1 92/07/30 SMI"; /* from UCB 2.4 */
#endif

/*
 * icheck
 */
#define	NB	500
#define	MAXFN	500
#define	MAXNINDIR	(MAXBSIZE / sizeof (daddr_t))

#ifndef STANDALONE
#include <stdio.h>
#endif
#ifndef SIMFS
#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#else
#include "../h/param.h"
#include "../h/time.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#endif

union {
	struct	fs sb;
	char pad[SBSIZE];
} sbun;
#define	sblock sbun.sb

union {
	struct	cg cg;
	char pad[MAXBSIZE];
} cgun;
#define	cgrp cgun.cg

struct	dinode	itab[MAXBSIZE / sizeof(struct dinode)];
daddr_t	blist[NB];
daddr_t	fsblist[NB];
char	*bmap;

int	mflg;
int	sflg;
int	dflg;
int	fi;
ino_t	ino;
int	cginit;

ino_t	nrfile;
ino_t	ndfile;
ino_t	nbfile;
ino_t	ncfile;
ino_t	nlfile;

daddr_t	nblock;
daddr_t	nfrag;
daddr_t	nindir;
daddr_t	niindir;

daddr_t	nffree;
daddr_t	nbfree;

daddr_t	ndup;

int	nerror;

extern int inside[], around[];
extern unsigned char *fragtbl[];

long	atol();
#ifndef STANDALONE
char	*malloc();
char	*calloc();
#endif

main(argc, argv)
	int argc;
	char *argv[];
{
	register i;
	long n;

	blist[0] = -1;
#ifndef STANDALONE
	audit_args(AU_MINPRIV, argc, argv);
	while (--argc) {
		argv++;
		if (**argv=='-')
		switch ((*argv)[1]) {
		case 'd':
			dflg++;
			continue;

		case 'm':
			mflg++;
			continue;

		case 's':
			sflg++;
			continue;

		case 'b':
			for(i=0; i<NB; i++) {
				if (argv[1] == 0)
					break;
				n = atol(argv[1]);
				if(n == 0)
					break;
				blist[i] = n;
				argv++;
				argc--;
			}
			blist[i] = -1;
			continue;

		default:
			printf("Bad flag\n");
		}
		check(*argv);
	}
#else
	{
		static char fname[128];

		printf("File: ");
		gets(fname);
		check(fname);
	}
#endif
	return(nerror);
}

check(file)
	char *file;
{
	register i, j, c;
	daddr_t d, cgd, cbase, b;
	long n;
	char buf[BUFSIZ];

	fi = open(file, sflg ? 2 : 0);
	if (fi < 0) {
		perror(file);
		nerror |= 04;
		return;
	}
	printf("%s:\n", file);
	nrfile = 0;
	ndfile = 0;
	ncfile = 0;
	nbfile = 0;
	nlfile = 0;

	nblock = 0;
	nfrag = 0;
	nindir = 0;
	niindir = 0;

	ndup = 0;
#ifndef STANDALONE
	sync();
#endif
	getsb(&sblock, file);
	if (nerror)
		return;
	for (n=0; blist[n] != -1; n++)
		fsblist[n] = dbtofsb(&sblock, blist[n]);
	ino = 0;
	n = roundup(howmany(sblock.fs_size, NBBY), sizeof(short));
#ifdef STANDALONE
	bmap = NULL;
#else
	bmap = malloc((unsigned)n);
#endif
	if (bmap==NULL) {
		printf("Not enough core; duplicates unchecked\n");
		dflg++;
		if (sflg) {
			printf("No Updates\n");
			sflg = 0;
		}
	}
	ino = 0;
	cginit = 1;
	if (!dflg) {
		for (i = 0; i < (unsigned)n; i++)
			bmap[i] = 0;
		for (c = 0; c < sblock.fs_ncg; c++) {
			cgd = cgtod(&sblock, c);
			if (c == 0)
				d = cgbase(&sblock, c);
			else
				d = cgsblock(&sblock, c);
			sprintf(buf, "spare super block %d", c);
			for (; d < cgd; d += sblock.fs_frag)
				chk(d, buf, sblock.fs_bsize);
			d = cgimin(&sblock, c);
			sprintf(buf, "cylinder group %d", c);
			while (cgd < d) {
				chk(cgd, buf, sblock.fs_bsize);
				cgd += sblock.fs_frag;
			}
			d = cgdmin(&sblock, c);
			i = INOPB(&sblock);
			for (; cgd < d; cgd += sblock.fs_frag) {
				sprintf(buf, "inodes %d-%d", ino, ino + i);
				chk(cgd, buf, sblock.fs_bsize);
				ino += i;
			}
			if (c == 0) {
				d += howmany(sblock.fs_cssize, sblock.fs_fsize);
				for (; cgd < d; cgd++)
					chk(cgd, "csum", sblock.fs_fsize);
			}
		}
	}
	ino = 0;
	cginit = 0;
	for (c = 0; c < sblock.fs_ncg; c++) {
		for (i = 0;
		     i < sblock.fs_ipg / INOPF(&sblock);
		     i += sblock.fs_frag) {
			bread(fsbtodb(&sblock, cgimin(&sblock, c) + i),
			    (char *)itab, sblock.fs_bsize);
			for (j = 0; j < INOPB(&sblock); j++) {
				pass1(&itab[j]);
				ino++;
			}
		}
	}
	ino = 0;
#ifndef STANDALONE
	sync();
#endif
	if (sflg) {
		makecg();
		close(fi);
#ifndef STANDALONE
		if (bmap)
			free(bmap);
#endif
		return;
	}
	nffree = 0;
	nbfree = 0;
	for (c = 0; c < sblock.fs_ncg; c++) {
		cbase = cgbase(&sblock, c);
		bread(fsbtodb(&sblock, cgtod(&sblock, c)), (char *)&cgrp,
			sblock.fs_cgsize);
		if (!cg_chkmagic(&cgrp))
			printf("cg %d: bad magic number\n", c);
		for (b = 0; b < sblock.fs_fpg; b += sblock.fs_frag) {
			if (isblock(&sblock, cg_blksfree(&cgrp),
			    b / sblock.fs_frag)) {
				nbfree++;
				chk(cbase+b, "free block", sblock.fs_bsize);
			} else {
				for (d = 0; d < sblock.fs_frag; d++)
					if (isset(cg_blksfree(&cgrp), b+d)) {
						chk(cbase+b+d, "free frag", sblock.fs_fsize);
						nffree++;
					}
			}
		}
	}
	close(fi);
#ifndef STANDALONE
	if (bmap)
		free(bmap);
#endif

	i = nrfile + ndfile + ncfile + nbfile + nlfile;
#ifndef STANDALONE
	printf("files %6u (r=%u,d=%u,b=%u,c=%u,sl=%u)\n",
		i, nrfile, ndfile, nbfile, ncfile, nlfile);
#else
	printf("files %u (r=%u,d=%u,b=%u,c=%u,sl=%u)\n",
		i, nrfile, ndfile, nbfile, ncfile, nlfile);
#endif
	n = (nblock + nindir + niindir) * sblock.fs_frag + nfrag;
#ifdef STANDALONE
	printf("used %ld (i=%ld,ii=%ld,b=%ld,f=%ld)\n",
		n, nindir, niindir, nblock, nfrag);
	printf("free %ld (b=%ld,f=%ld)\n", nffree + sblock.fs_frag * nbfree,
	    nbfree, nffree);
#else
	printf("used %7ld (i=%ld,ii=%ld,b=%ld,f=%ld)\n",
		n, nindir, niindir, nblock, nfrag);
	printf("free %7ld (b=%ld,f=%ld)\n", nffree + sblock.fs_frag * nbfree,
	    nbfree, nffree);
#endif
	if(!dflg) {
		n = 0;
		for (d = 0; d < sblock.fs_size; d++)
			if(!duped(d, sblock.fs_fsize)) {
				if(mflg)
					printf("%ld missing\n", d);
				n++;
			}
		printf("missing%5ld\n", n);
	}
}

pass1(ip)
	register struct dinode *ip;
{
	daddr_t ind1[MAXNINDIR];
	daddr_t ind2[MAXNINDIR];
	daddr_t db, ib;
	register int i, j, k, siz;
	int lbn;
	char buf[BUFSIZ];

	i = ip->di_mode & IFMT;
	if(i == 0)
		return;
	switch (i) {
	case IFCHR:
		ncfile++;
		return;
	case IFBLK:
		nbfile++;
		return;
	case IFDIR:
		ndfile++;
		break;
	case IFREG:
		nrfile++;
		break;
	case IFLNK:
		nlfile++;
		break;
	default:
		printf("bad mode %u\n", ino);
		return;
	}
	for (i = 0; i < NDADDR; i++) {
		db = ip->di_db[i];
		if (db == 0)
			continue;
		siz = dblksize(&sblock, ip, i);
		sprintf(buf, "logical data block %d", i);
		chk(db, buf, siz);
		if (siz == sblock.fs_bsize)
			nblock++;
		else
			nfrag += howmany(siz, sblock.fs_fsize);
	}
	for(i = 0; i < NIADDR; i++) {
		ib = ip->di_ib[i];
		if (ib == 0)
			continue;
		if (chk(ib, "1st indirect", sblock.fs_bsize))
			continue;
		bread(fsbtodb(&sblock, ib), (char *)ind1, sblock.fs_bsize);
		nindir++;
		for (j = 0; j < NINDIR(&sblock); j++) {
			ib = ind1[j];
			if (ib == 0)
				continue;
			if (i == 0) {
				lbn = NDADDR + j;
				siz = dblksize(&sblock, ip, lbn);
				sprintf(buf, "logical data block %d", lbn);
				chk(ib, buf, siz);
				if (siz == sblock.fs_bsize)
					nblock++;
				else
					nfrag += howmany(siz, sblock.fs_fsize);
				continue;
			}
			if (chk(ib, "2nd indirect", sblock.fs_bsize))
				continue;
			bread(fsbtodb(&sblock, ib), (char *)ind2,
				sblock.fs_bsize);
			niindir++;
			for (k = 0; k < NINDIR(&sblock); k++) {
				ib = ind2[k];
				if (ib == 0)
					continue;
				lbn = NDADDR + NINDIR(&sblock) * (i + j) + k;
				siz = dblksize(&sblock, ip, lbn);
				sprintf(buf, "logical data block %d", lbn);
				chk(ib, buf, siz);
				if (siz == sblock.fs_bsize)
					nblock++;
				else
					nfrag += howmany(siz, sblock.fs_fsize);
			}
		}
	}
}

chk(bno, s, size)
	daddr_t bno;
	char *s;
	int size;
{
	register n, cg;
	int frags;

	cg = dtog(&sblock, bno);
	if (cginit == 0 && bno >= sblock.fs_frag * sblock.fs_size) {
		printf("%ld bad; inode=%u, class=%s\n", bno, ino, s);
		return(1);
	}
	frags = numfrags(&sblock, size);
	if (frags == sblock.fs_frag) {
		if (duped(bno, size)) {
			printf("%ld dup block; inode=%u, class=%s\n",
			    bno, ino, s);
			ndup += sblock.fs_frag;
		}
	} else {
		for (n = 0; n < frags; n++) {
			if (duped(bno + n, sblock.fs_fsize)) {
				printf("%ld dup frag; inode=%u, class=%s\n",
				    bno, ino, s);
				ndup++;
			}
		}
	}
	for (n=0; blist[n] != -1; n++)
		if (fsblist[n] >= bno && fsblist[n] < bno + frags)
			printf("%ld arg; frag %d of %d, inode=%u, class=%s\n",
				blist[n], fsblist[n] - bno, frags, ino, s);
	return(0);
}

duped(bno, size)
	daddr_t bno;
	int size;
{
	if(dflg)
		return(0);
	if (size != sblock.fs_fsize && size != sblock.fs_bsize)
		printf("bad size %d to duped\n", size);
	if (size == sblock.fs_fsize) {
		if (isset(bmap, bno))
			return(1);
		setbit(bmap, bno);
		return (0);
	}
	if (bno % sblock.fs_frag != 0)
		printf("bad bno %d to duped\n", bno);
	if (isblock(&sblock, bmap, bno/sblock.fs_frag))
		return (1);
	setblock(&sblock, bmap, bno/sblock.fs_frag);
	return(0);
}


makecg()
{
	int c, blk;
	daddr_t dbase, d, dlower, dupper, dmax;
	long i, j, s;
	register struct csum *cs;
	register struct dinode *dp;

	sblock.fs_cstotal.cs_nbfree = 0;
	sblock.fs_cstotal.cs_nffree = 0;
	sblock.fs_cstotal.cs_nifree = 0;
	sblock.fs_cstotal.cs_ndir = 0;
	for (c = 0; c < sblock.fs_ncg; c++) {
		dbase = cgbase(&sblock, c);
		dmax = dbase + sblock.fs_fpg;
		if (dmax > sblock.fs_size) {
			for ( ; dmax >= sblock.fs_size; dmax--)
				clrbit(cg_blksfree(&cgrp), dmax - dbase);
			dmax++;
		}
		dlower = cgsblock(&sblock, c) - dbase;
		dupper = cgdmin(&sblock, c) - dbase;
		cs = &sblock.fs_cs(&sblock, c);
		cgrp.cg_time = time(0);
		cgrp.cg_magic = CG_MAGIC;
		cgrp.cg_cgx = c;
		if (c == sblock.fs_ncg - 1)
			cgrp.cg_ncyl = sblock.fs_ncyl % sblock.fs_cpg;
		else
			cgrp.cg_ncyl = sblock.fs_cpg;
		cgrp.cg_niblk = sblock.fs_ipg;
		cgrp.cg_ndblk = dmax - dbase;
		cgrp.cg_cs.cs_ndir = 0;
		cgrp.cg_cs.cs_nffree = 0;
		cgrp.cg_cs.cs_nbfree = 0;
		cgrp.cg_cs.cs_nifree = 0;
		cgrp.cg_rotor = 0;
		cgrp.cg_frotor = 0;
		cgrp.cg_irotor = 0;
		cgrp.cg_btotoff = &cgrp.cg_space[0] - (u_char *)(&cgrp.cg_link);
		cgrp.cg_boff = cgrp.cg_btotoff + sblock.fs_cpg * sizeof(long);
		cgrp.cg_iusedoff = cgrp.cg_boff + 
			sblock.fs_cpg * sblock.fs_nrpos * sizeof(short);
		cgrp.cg_freeoff = cgrp.cg_iusedoff + howmany(sblock.fs_ipg, NBBY);
		cgrp.cg_nextfreeoff = cgrp.cg_freeoff +
			howmany(sblock.fs_cpg * sblock.fs_spc / NSPF(&sblock), NBBY);
		for (i = 0; i < sblock.fs_frag; i++) {
			cgrp.cg_frsum[i] = 0;
		}
		bzero((caddr_t)cg_inosused(&cgrp), cgrp.cg_freeoff-cgrp.cg_iusedoff);
		for (i = 0;
		     i < sblock.fs_ipg / INOPF(&sblock);
		     i += sblock.fs_frag) {
			bread(fsbtodb(&sblock, cgimin(&sblock, c) + i),
			      (char *)itab, sblock.fs_bsize);
			for (j = 0; j < INOPB(&sblock); j++) { 
				cgrp.cg_cs.cs_nifree++;
				clrbit(cg_inosused(&cgrp), j + i * INOPB(&sblock));
				dp = &itab[j];
				if ((dp->di_mode & IFMT) != 0) {
					if ((dp->di_mode & IFMT) == IFDIR)
						cgrp.cg_cs.cs_ndir++;
					cgrp.cg_cs.cs_nifree--;
					setbit(cg_inosused(&cgrp),
					    j + i * INOPB(&sblock));
					continue;
				}
			}
		}
		if (c == 0)
			for (i = 0; i < ROOTINO; i++) {
				setbit(cg_inosused(&cgrp), i);
				cgrp.cg_cs.cs_nifree--;
			}
		bzero((caddr_t)cg_blktot(&cgrp), cgrp.cg_boff - cgrp.cg_btotoff);
		bzero((caddr_t)cg_blks(&sblock, &cgrp, 0),
		    cgrp.cg_iusedoff - cgrp.cg_boff);
		bzero((caddr_t)cg_blksfree(&cgrp), cgrp.cg_nextfreeoff - cgrp.cg_freeoff);
		if (c == 0) {
			dupper += howmany(sblock.fs_cssize, sblock.fs_fsize);
		}
		for (d = dlower; d < dupper; d++)
			clrbit(cg_blksfree(&cgrp), d);
		for (d = 0; (d + sblock.fs_frag) <= dmax - dbase;
		    d += sblock.fs_frag) {
			j = 0;
			for (i = 0; i < sblock.fs_frag; i++) {
				if (!isset(bmap, dbase + d + i)) {
					setbit(cg_blksfree(&cgrp), d + i);
					j++;
				} else
					clrbit(cg_blksfree(&cgrp), d+i);
			}
			if (j == sblock.fs_frag) {
				cgrp.cg_cs.cs_nbfree++;
				cg_blktot(&cgrp)[cbtocylno(&sblock, d)]++;
				cg_blks(&sblock, &cgrp, cbtocylno(&sblock, d))
				    [cbtorpos(&sblock, d)]++;
			} else if (j > 0) {
				cgrp.cg_cs.cs_nffree += j;
				blk = blkmap(&sblock, cg_blksfree(&cgrp), d);
				fragacct(&sblock, blk, cgrp.cg_frsum, 1);
			}
		}
		for (j = d; d < dmax - dbase; d++) {
			if (!isset(bmap, dbase + d)) {
				setbit(cg_blksfree(&cgrp), d);
				cgrp.cg_cs.cs_nffree++;
			} else
				clrbit(cg_blksfree(&cgrp), d);
		}
		for (; d % sblock.fs_frag != 0; d++)
			clrbit(cg_blksfree(&cgrp), d);
		if (j != d) {
			blk = blkmap(&sblock, cg_blksfree(&cgrp), j);
			fragacct(&sblock, blk, cgrp.cg_frsum, 1);
		}
		sblock.fs_cstotal.cs_nffree += cgrp.cg_cs.cs_nffree;
		sblock.fs_cstotal.cs_nbfree += cgrp.cg_cs.cs_nbfree;
		sblock.fs_cstotal.cs_nifree += cgrp.cg_cs.cs_nifree;
		sblock.fs_cstotal.cs_ndir += cgrp.cg_cs.cs_ndir;
		*cs = cgrp.cg_cs;
		bwrite(fsbtodb(&sblock, cgtod(&sblock, c)), &cgrp,
			sblock.fs_cgsize);
	}
	for (i = 0, j = 0; i < sblock.fs_cssize; i += sblock.fs_bsize, j++) {
		bwrite(fsbtodb(&sblock, sblock.fs_csaddr + j * sblock.fs_frag),
		    (char *)sblock.fs_csp[j],
		    sblock.fs_cssize - i < sblock.fs_bsize ?
		    sblock.fs_cssize - i : sblock.fs_bsize);
	}
	sblock.fs_ronly = 0;
	sblock.fs_fmod = 0;
	bwrite(SBLOCK, (char *)&sblock, SBSIZE);
}

/*
 * update the frsum fields to reflect addition or deletion 
 * of some frags
 */
fragacct(fs, fragmap, fraglist, cnt)
	struct fs *fs;
	int fragmap;
	long fraglist[];
	int cnt;
{
	int inblk;
	register int field, subfield;
	register int siz, pos;

	inblk = (int)(fragtbl[fs->fs_frag][fragmap] << 1);
	fragmap <<= 1;
	for (siz = 1; siz < fs->fs_frag; siz++) {
		if ((inblk & (1 << (siz + (fs->fs_frag % NBBY)))) == 0)
			continue;
		field = around[siz];
		subfield = inside[siz];
		for (pos = siz; pos <= fs->fs_frag; pos++) {
			if ((fragmap & field) == subfield) {
				fraglist[siz] += cnt;
				pos += siz;
				field <<= siz;
				subfield <<= siz;
			}
			field <<= 1;
			subfield <<= 1;
		}
	}
}

getsb(fs, file)
	register struct fs *fs;
	char *file;
{
	int i, j, size;

	if (bread(SBLOCK, fs, SBSIZE)) {
		printf("bad super block");
		perror(file);
		nerror |= 04;
		return;
	}
	if (fs->fs_magic != FS_MAGIC) {
		printf("%s: bad magic number\n", file);
		nerror |= 04;
		return;
	}
	for (i = 0, j = 0; i < sblock.fs_cssize; i += sblock.fs_bsize, j++) {
		size = sblock.fs_cssize - i < sblock.fs_bsize ?
		    sblock.fs_cssize - i : sblock.fs_bsize;
		sblock.fs_csp[j] = (struct csum *)calloc(1, size);
		bread(fsbtodb(fs, fs->fs_csaddr + (j * fs->fs_frag)),
		      (char *)fs->fs_csp[j], size);
	}
}

bwrite(blk, buf, size)
	char *buf;
	daddr_t blk;
	register size;
{
	if (lseek(fi, blk * DEV_BSIZE, 0) < 0) {
		perror("FS SEEK");
		return(1);
	}
	if (write(fi, buf, size) != size) {
		perror("FS WRITE");
		return(1);
	}
	return (0);
}

bread(bno, buf, cnt)
	daddr_t bno;
	char *buf;
{
	register i;

	lseek(fi, bno * DEV_BSIZE, 0);
	if ((i = read(fi, buf, cnt)) != cnt) {
		if (sflg) {
			printf("No Update\n");
			sflg = 0;
		}
		for(i=0; i<sblock.fs_bsize; i++)
			buf[i] = 0;
		return (1);
	}
	return (0);
}

/*
 * check if a block is available
 */
isblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	int h;
{
	unsigned char mask;

	switch (fs->fs_frag) {
	case 8:
		return (cp[h] == 0xff);
	case 4:
		mask = 0x0f << ((h & 0x1) << 2);
		return ((cp[h >> 1] & mask) == mask);
	case 2:
		mask = 0x03 << ((h & 0x3) << 1);
		return ((cp[h >> 2] & mask) == mask);
	case 1:
		mask = 0x01 << (h & 0x7);
		return ((cp[h >> 3] & mask) == mask);
	default:
#ifdef STANDALONE
		printf("isblock bad fs_frag %d\n", fs->fs_frag);
#else
		fprintf(stderr, "isblock bad fs_frag %d\n", fs->fs_frag);
#endif
		return;
	}
}

/*
 * take a block out of the map
 */
clrblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	int h;
{
	switch ((fs)->fs_frag) {
	case 8:
		cp[h] = 0;
		return;
	case 4:
		cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] &= ~(0x01 << (h & 0x7));
		return;
	default:
#ifdef STANDALONE
		printf("clrblock bad fs_frag %d\n", fs->fs_frag);
#else
		fprintf(stderr, "clrblock bad fs_frag %d\n", fs->fs_frag);
#endif
		return;
	}
}

/*
 * put a block into the map
 */
setblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	int h;
{
	switch (fs->fs_frag) {
	case 8:
		cp[h] = 0xff;
		return;
	case 4:
		cp[h >> 1] |= (0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] |= (0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] |= (0x01 << (h & 0x7));
		return;
	default:
#ifdef STANDALONE
		printf("setblock bad fs_frag %d\n", fs->fs_frag);
#else
		fprintf(stderr, "setblock bad fs_frag %d\n", fs->fs_frag);
#endif
		return;
	}
}

/*	tables.c	4.1	82/03/25	*/

/* merged into kernel:	tables.c 2.1 3/25/82 */

/* last monet version:	partab.c	4.2	81/03/08	*/

/*
 * bit patterns for identifying fragments in the block map
 * used as ((map & around) == inside)
 */
int around[9] = {
	0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff, 0x1ff, 0x3ff
};
int inside[9] = {
	0x0, 0x2, 0x6, 0xe, 0x1e, 0x3e, 0x7e, 0xfe, 0x1fe
};

/*
 * given a block map bit pattern, the frag tables tell whether a
 * particular size fragment is available. 
 *
 * used as:
 * if ((1 << (size - 1)) & fragtbl[fs->fs_frag][map] {
 *	at least one fragment of the indicated size is available
 * }
 *
 * These tables are used by the scanc instruction on the VAX to
 * quickly find an appropriate fragment.
 */

unsigned char fragtbl124[256] = {
	0x00, 0x16, 0x16, 0x2a, 0x16, 0x16, 0x26, 0x4e,
	0x16, 0x16, 0x16, 0x3e, 0x2a, 0x3e, 0x4e, 0x8a,
	0x16, 0x16, 0x16, 0x3e, 0x16, 0x16, 0x36, 0x5e,
	0x16, 0x16, 0x16, 0x3e, 0x3e, 0x3e, 0x5e, 0x9e,
	0x16, 0x16, 0x16, 0x3e, 0x16, 0x16, 0x36, 0x5e,
	0x16, 0x16, 0x16, 0x3e, 0x3e, 0x3e, 0x5e, 0x9e,
	0x2a, 0x3e, 0x3e, 0x2a, 0x3e, 0x3e, 0x2e, 0x6e,
	0x3e, 0x3e, 0x3e, 0x3e, 0x2a, 0x3e, 0x6e, 0xaa,
	0x16, 0x16, 0x16, 0x3e, 0x16, 0x16, 0x36, 0x5e,
	0x16, 0x16, 0x16, 0x3e, 0x3e, 0x3e, 0x5e, 0x9e,
	0x16, 0x16, 0x16, 0x3e, 0x16, 0x16, 0x36, 0x5e,
	0x16, 0x16, 0x16, 0x3e, 0x3e, 0x3e, 0x5e, 0x9e,
	0x26, 0x36, 0x36, 0x2e, 0x36, 0x36, 0x26, 0x6e,
	0x36, 0x36, 0x36, 0x3e, 0x2e, 0x3e, 0x6e, 0xae,
	0x4e, 0x5e, 0x5e, 0x6e, 0x5e, 0x5e, 0x6e, 0x4e,
	0x5e, 0x5e, 0x5e, 0x7e, 0x6e, 0x7e, 0x4e, 0xce,
	0x16, 0x16, 0x16, 0x3e, 0x16, 0x16, 0x36, 0x5e,
	0x16, 0x16, 0x16, 0x3e, 0x3e, 0x3e, 0x5e, 0x9e,
	0x16, 0x16, 0x16, 0x3e, 0x16, 0x16, 0x36, 0x5e,
	0x16, 0x16, 0x16, 0x3e, 0x3e, 0x3e, 0x5e, 0x9e,
	0x16, 0x16, 0x16, 0x3e, 0x16, 0x16, 0x36, 0x5e,
	0x16, 0x16, 0x16, 0x3e, 0x3e, 0x3e, 0x5e, 0x9e,
	0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x7e,
	0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x7e, 0xbe,
	0x2a, 0x3e, 0x3e, 0x2a, 0x3e, 0x3e, 0x2e, 0x6e,
	0x3e, 0x3e, 0x3e, 0x3e, 0x2a, 0x3e, 0x6e, 0xaa,
	0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x7e,
	0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x7e, 0xbe,
	0x4e, 0x5e, 0x5e, 0x6e, 0x5e, 0x5e, 0x6e, 0x4e,
	0x5e, 0x5e, 0x5e, 0x7e, 0x6e, 0x7e, 0x4e, 0xce,
	0x8a, 0x9e, 0x9e, 0xaa, 0x9e, 0x9e, 0xae, 0xce,
	0x9e, 0x9e, 0x9e, 0xbe, 0xaa, 0xbe, 0xce, 0x8a,
};

unsigned char fragtbl8[256] = {
	0x00, 0x01, 0x01, 0x02, 0x01, 0x01, 0x02, 0x04,
	0x01, 0x01, 0x01, 0x03, 0x02, 0x03, 0x04, 0x08,
	0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x03, 0x05,
	0x02, 0x03, 0x03, 0x02, 0x04, 0x05, 0x08, 0x10,
	0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x03, 0x05,
	0x01, 0x01, 0x01, 0x03, 0x03, 0x03, 0x05, 0x09,
	0x02, 0x03, 0x03, 0x02, 0x03, 0x03, 0x02, 0x06,
	0x04, 0x05, 0x05, 0x06, 0x08, 0x09, 0x10, 0x20,
	0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x03, 0x05,
	0x01, 0x01, 0x01, 0x03, 0x03, 0x03, 0x05, 0x09,
	0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x03, 0x05,
	0x03, 0x03, 0x03, 0x03, 0x05, 0x05, 0x09, 0x11,
	0x02, 0x03, 0x03, 0x02, 0x03, 0x03, 0x02, 0x06,
	0x03, 0x03, 0x03, 0x03, 0x02, 0x03, 0x06, 0x0a,
	0x04, 0x05, 0x05, 0x06, 0x05, 0x05, 0x06, 0x04,
	0x08, 0x09, 0x09, 0x0a, 0x10, 0x11, 0x20, 0x40,
	0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x03, 0x05,
	0x01, 0x01, 0x01, 0x03, 0x03, 0x03, 0x05, 0x09,
	0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x03, 0x05,
	0x03, 0x03, 0x03, 0x03, 0x05, 0x05, 0x09, 0x11,
	0x01, 0x01, 0x01, 0x03, 0x01, 0x01, 0x03, 0x05,
	0x01, 0x01, 0x01, 0x03, 0x03, 0x03, 0x05, 0x09,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x07,
	0x05, 0x05, 0x05, 0x07, 0x09, 0x09, 0x11, 0x21,
	0x02, 0x03, 0x03, 0x02, 0x03, 0x03, 0x02, 0x06,
	0x03, 0x03, 0x03, 0x03, 0x02, 0x03, 0x06, 0x0a,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x07,
	0x02, 0x03, 0x03, 0x02, 0x06, 0x07, 0x0a, 0x12,
	0x04, 0x05, 0x05, 0x06, 0x05, 0x05, 0x06, 0x04,
	0x05, 0x05, 0x05, 0x07, 0x06, 0x07, 0x04, 0x0c,
	0x08, 0x09, 0x09, 0x0a, 0x09, 0x09, 0x0a, 0x0c,
	0x10, 0x11, 0x11, 0x12, 0x20, 0x21, 0x40, 0x80,
};

/*
 * the actual fragtbl array
 */
unsigned char *fragtbl[MAXFRAG + 1] = {
	0, fragtbl124, fragtbl124, 0, fragtbl124, 0, 0, 0, fragtbl8,
};
