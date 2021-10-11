#ifndef lint
static  char sccsid[] = "@(#)ncheck.c 1.1 92/07/30 SMI"; /* from UCB 5.4 1/9/86  */
#endif

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

/*
 * ncheck -- obtain file names from reading filesystem
 */

#define	MAXNINDIR	(MAXBSIZE / sizeof (daddr_t))

#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <sys/file.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#include <ufs/fsdir.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

union {
	struct	fs	sblk;
	char xxx[SBSIZE];	/* because fs is variable length */
} real_fs;
#define sblock real_fs.sblk

struct	dinode	itab[MAXBSIZE/sizeof(struct dinode)];
struct 	dinode	*gip;
struct ilist {
	struct 	ilist *next;
	ino_t	ino;
	u_short	mode;
	short	uid, gid;
} *ilist = NULL;

struct	htab
{
	ino_t	h_ino;
	ino_t	h_pino;
	char	*h_name;
} *htab;

long hsize;

struct dirstuff {
	int loc;
	struct dinode *ip;
	char dbuf[MAXBSIZE];
};

int	aflg;
int	sflg;
int	mflg;
int	fi;
ino_t	ino;
int	nhent;

int	nerror;
static	void	bread(), check(), add_to_ilist();
static	daddr_t	bmap();
static	char	*alloc();
static	struct htab *lookup();
static	struct direct *dreaddir();

extern	long	atol();
extern	char 	*malloc(), *calloc();

main(argc, argv)
	int argc;
	char *argv[];
{
	audit_args(AU_MINPRIV, argc, argv);
	while (--argc) {
		argv++;
		if (**argv != '-') {
			check(*argv);
			continue;
		}
		switch ((*argv)[1]) {
		case 'a':
			aflg++;
			continue;

		case 'i':
			while (argv[1] != 0 && isdigit(*argv[1])) {
				add_to_ilist((ino_t)atol(argv[1]));
				argv++;
				argc--;
			}
			continue;

		case 'm':
			mflg++;
			continue;

		case 's':
			sflg++;
			continue;

		default:
			(void) fprintf(stderr,
					"ncheck: bad flag %c\n", (*argv)[1]);
			nerror++;
		}
	}
	return (nerror);
}

static void
check(file)
	char *file;
{
	register int i, j, c;
	register struct ilist *il;
	register struct htab *h;

	fi = open(file, O_RDONLY, 0);
	if (fi < 0) {
		(void) fprintf(stderr, "ncheck: cannot open %s\n", file);
		nerror++;
		return;
	}
	nhent = 0;
	(void) printf("%s:\n", file);
	sync();
	bread(SBLOCK, (char *)&sblock, SBSIZE);
	if (sblock.fs_magic != FS_MAGIC) {
		(void) printf("%s: not a file system\n", file);
		nerror++;
		return;
	}
	hsize = sblock.fs_ipg * sblock.fs_ncg - sblock.fs_cstotal.cs_nifree + 1;
	htab = (struct htab *)calloc((unsigned)hsize, sizeof (struct htab));
	if (htab == 0) {
		(void) printf("not enough memory to allocate tables\n");
		nerror++;
		return;
	}
	ino = 0;
	for (c = 0; c < sblock.fs_ncg; c++) {
		for (i = 0;
		     i < sblock.fs_ipg / INOPF(&sblock);
		     i += sblock.fs_frag) {
			bread(fsbtodb(&sblock, cgimin(&sblock, c) + i),
			    (char *)itab, sblock.fs_bsize);
			for (j = 0; j < INOPB(&sblock); j++) {
				if (itab[j].di_mode != 0)
					pass1(&itab[j]);
				ino++;
			}
		}

	}
	ino = 0;
	for (c = 0; c < sblock.fs_ncg; c++) {
		for (i = 0;
		     i < sblock.fs_ipg / INOPF(&sblock);
		     i += sblock.fs_frag) {
			bread(fsbtodb(&sblock, cgimin(&sblock, c) + i),
			    (char *)itab, sblock.fs_bsize);
			for (j = 0; j < INOPB(&sblock); j++) {
				if (itab[j].di_mode != 0)
					pass2(&itab[j]);
				ino++;
			}
		}
	}
	ino = 0;
	for (c = 0; c < sblock.fs_ncg; c++) {
		for (i = 0;
		     i < sblock.fs_ipg / INOPF(&sblock);
		     i += sblock.fs_frag) {
			bread(fsbtodb(&sblock, cgimin(&sblock, c) + i),
			    (char *)itab, sblock.fs_bsize);
			for (j = 0; j < INOPB(&sblock); j++) {
				if (itab[j].di_mode != 0)
					pass3(&itab[j]);
				ino++;
			}
		}
	}
	(void) close(fi);
	for (h = htab; h != htab + hsize; h++) {
		if (h->h_name != NULL)
			free(h->h_name);
	}
	free((char *)htab);
	while (ilist != NULL) {
		il = ilist->next;
		free((char *)ilist);
		ilist = il;
	}
}

pass1(ip)
	register struct dinode *ip;
{
	register struct ilist *il;
	register int fmt = ip->di_mode & IFMT;

	if (mflg) {
		for (il = ilist; il != NULL; il = il->next)
			if (ino == il->ino) {
				il->mode = ip->di_mode;
				il->uid = ip->di_uid;
				il->gid = ip->di_gid;
			}
	}
	if (sflg) {
		if (fmt == IFBLK || fmt == IFCHR ||
			fmt != IFDIR && (ip->di_mode & (ISUID|ISGID))) {
			add_to_ilist(ino);
			ilist->mode = ip->di_mode;
			ilist->uid = ip->di_uid;
			ilist->gid = ip->di_gid;
		}
	}
	if (fmt == IFDIR)
		(void) lookup(ino, 1);
}

pass2(ip)
	register struct dinode *ip;
{
	register struct direct *dp;
	struct dirstuff dirp;
	struct htab *hp;

	if ((ip->di_mode&IFMT) != IFDIR)
		return;
	dirp.loc = 0;
	dirp.ip = ip;
	gip = ip;
	for (dp = dreaddir(&dirp); dp != NULL; dp = dreaddir(&dirp)) {
		if (dp->d_ino == 0)
			continue;
		hp = lookup(dp->d_ino, 0);
		if (hp == 0)
			continue;
		if (dotname(dp))
			continue;
		hp->h_pino = ino;
		hp->h_name = alloc((u_int)(strlen(dp->d_name) + 1));
		(void) strcpy(hp->h_name, dp->d_name);
	}
}

pass3(ip)
	register struct dinode *ip;
{
	register struct direct *dp;
	struct dirstuff dirp;
	register struct ilist *il;

	if ((ip->di_mode & IFMT) != IFDIR)
		return;
	dirp.loc = 0;
	dirp.ip = ip;
	gip = ip;
	for (dp = dreaddir(&dirp); dp != NULL; dp = dreaddir(&dirp)) {
		if (aflg == 0 && dotname(dp))
			continue;
		if (sflg == 0 && ilist == NULL)
			goto pr;
		for (il = ilist; il != NULL; il = il->next)
			if (il->ino == dp->d_ino)
				break;
		if (il == NULL)
			continue;
		if (mflg)
			(void) printf("mode %-6o uid %-5d gid %-5d ino ",
			    il->mode, il->uid, il->gid);
	pr:
		(void) printf("%-5u\t", dp->d_ino);
		pname(ino, 0);
		(void) printf("/%s", dp->d_name);
		if (lookup(dp->d_ino, 0))
			(void) printf("/.");
		(void) printf("\n");
	}
}

/*
 * get next entry in a directory.
 */
static struct direct *
dreaddir(dirp)
	register struct dirstuff *dirp;
{
	register struct direct *dp;
	daddr_t lbn, d;

	for (;;) {
		if (dirp->loc >= dirp->ip->di_size)
			return NULL;
		if (blkoff(&sblock, dirp->loc) == 0) {
			lbn = lblkno(&sblock, dirp->loc);
			d = bmap(lbn);
			if (d == 0)
				return NULL;
			bread(fsbtodb(&sblock, d), dirp->dbuf,
			    (int)dblksize(&sblock, dirp->ip, (int)lbn));
		}
		dp = (struct direct *)
		    (dirp->dbuf + blkoff(&sblock, dirp->loc));
		dirp->loc += dp->d_reclen;
		if (dp->d_ino == 0)
			continue;
		return (dp);
	}
}

dotname(dp)
	struct direct *dp;
{
	register char *p = dp->d_name;

	return (p[0] == '.' && (p[1] == 0 || (p[1] == '.' && p[2] == 0)));
}

pname(i, lev)
	ino_t i;
	int lev;
{
	register struct htab *hp;

	if (i == ROOTINO)
		return;
	if ((hp = lookup(i, 0)) == 0) {
		(void) printf("???");
		return;
	}
	if (lev > 10) {
		(void) printf("...");
		return;
	}
	pname(hp->h_pino, ++lev);
	(void) printf("/%s", hp->h_name);
}

static struct htab *
lookup(i, ef)
	ino_t i;
	int ef;
{
	register struct htab *hp;

	for (hp = &htab[i % hsize]; hp->h_ino;) {
		if (hp->h_ino == i)
			return (hp);
		if (++hp >= &htab[hsize])
			hp = htab;
	}
	if (ef == 0)
		return (0);
	if (++nhent >= hsize) {
		(void) fprintf(stderr,
			"ncheck: hsize of %d is too small\n", hsize);
		exit(1);
	}
	hp->h_ino = i;
	return (hp);
}

static void
bread(bno, buf, cnt)
	daddr_t bno;
	char *buf;
	int cnt;
{
	int got;

	if (lseek(fi, (long)(bno * DEV_BSIZE), L_SET) == (long) -1) {
		(void) fprintf(stderr,
				"ncheck: lseek error %d\n", bno * DEV_BSIZE);
		bzero(buf, cnt);
		return;
	}

	got = read(fi, buf, cnt);
	if (got != cnt) {
		(void) fprintf(stderr,
			"ncheck: read error %d (wanted %d got %d)\n",
				bno, cnt, got);
		if (got == -1)
			perror("ncheck");
		bzero(buf, cnt);
	}
}

static daddr_t
bmap(i)
	daddr_t i;
{
	daddr_t ibuf[MAXNINDIR];

	if (i < NDADDR)
		return (gip->di_db[i]);
	i -= NDADDR;
	if (i > NINDIR(&sblock)) {
		(void) fprintf(stderr, "ncheck: %u - huge directory\n", ino);
		return ((daddr_t)0);
	}
	bread(fsbtodb(&sblock, gip->di_ib[0]), (char *)ibuf, sizeof (ibuf));
	return (ibuf[i]);
}

static void
add_to_ilist(inum)
	ino_t inum;
{
	struct ilist *il;

	il = (struct ilist *)alloc(sizeof (struct ilist));
	il->ino = inum;
	il->next = ilist;
	ilist = il;
}

static char *
alloc(n)
	unsigned n;
{
	char *p;

	p = malloc(n);
	if (p == NULL) {
		perror("ncheck");
		exit(1);
	}
	return p;
}
