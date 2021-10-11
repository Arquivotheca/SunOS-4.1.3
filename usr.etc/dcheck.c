#ifndef lint
static	char *sccsid = "@(#)dcheck.c 1.1 92/07/30 SMI"; /* from UCB 2.4 */
#endif
/*
 * dcheck - check directory consistency
 */
#define	NB	10
#define	MAXNINDIR	(MAXBSIZE / sizeof (daddr_t))

#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/types.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#include <ufs/fsdir.h>
#include <stdio.h>

union {
	struct	fs fs;
	char pad[SBSIZE];
} fsun;
#define	sblock	fsun.fs

struct dirstuff {
	int loc;
	struct dinode *ip;
	char dbuf[MAXBSIZE];
};

struct	dinode	itab[MAXBSIZE / sizeof(struct dinode)];
struct	dinode	*gip;
ino_t	ilist[NB];

int	fi;
ino_t	ino;
ino_t	*ecount;
int	headpr;
int	nfiles;

int	nerror;
daddr_t	bmap();
struct	direct *dreaddir();
long	atol();
char	*malloc();

main(argc, argv)
char *argv[];
{
	register i;
	long n;

	audit_args(AU_MINPRIV, argc, argv);
	while (--argc) {
		argv++;
		if (**argv=='-')
		switch ((*argv)[1]) {

		case 'i':
			i =0;
			do {
				n = atol(argv[1]);
				if (n < 2) {
					if (argv[1][0] != '/') {
			fprintf(stderr, "dcheck: %d not a valid inode\n", n);
					} else {
						break;
					}
				} else {
					ilist[i++] = n;
				}
				argv++;
				argc--;
			} while (i < NB);
			ilist[i] = 0;
			continue;

		default:
			printf("Bad flag %c\n", (*argv)[1]);
			nerror++;
		}
		check(*argv);
	}
	return(nerror);
}

check(file)
char *file;
{
	register i, j, c;

	fi = open(file, 0);
	if(fi < 0) {
		printf("cannot open %s\n", file);
		nerror++;
		return;
	}
	headpr = 0;
	printf("%s:\n", file);
	sync();
	bread(SBLOCK, (char *)&sblock, SBSIZE);
	if (sblock.fs_magic != FS_MAGIC) {
		printf("%s: not a file system\n", file);
		nerror++;
		return;
	}
	nfiles = sblock.fs_ipg * sblock.fs_ncg;
	ecount = (ino_t *)malloc((nfiles+1) * sizeof (*ecount));
	if (ecount == 0) {
		printf("%s: not enough core for %d files\n", file, nfiles);
		exit(04);
	}
	for (i = 0; i<=nfiles; i++)
		ecount[i] = 0;
	ino = 0;
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
	for (c = 0; c < sblock.fs_ncg; c++) {
		for (i = 0;
		     i < sblock.fs_ipg / INOPF(&sblock);
		     i += sblock.fs_frag) {
			bread(fsbtodb(&sblock, cgimin(&sblock, c) + i),
			    (char *)itab, sblock.fs_bsize);
			for (j = 0; j < INOPB(&sblock); j++) {
				pass2(&itab[j]);
				ino++;
			}
		}
	}
	free(ecount);
}

pass1(ip)
	register struct dinode *ip;
{
	register struct direct *dp;
	struct dirstuff dirp;
	int k;

	if((ip->di_mode&IFMT) != IFDIR)
		return;
	dirp.loc = 0;
	dirp.ip = ip;
	gip = ip;
	for (dp = dreaddir(&dirp); dp != NULL; dp = dreaddir(&dirp)) {
		if(dp->d_ino == 0)
			continue;
		if(dp->d_ino > nfiles || dp->d_ino < ROOTINO) {
			printf("%d bad; %d/%s\n",
			    dp->d_ino, ino, dp->d_name);
			nerror++;
			continue;
		}
		for (k = 0; ilist[k] != 0; k++)
			if (ilist[k] == dp->d_ino) {
				printf("%d arg; %d/%s\n",
				     dp->d_ino, ino, dp->d_name);
				nerror++;
			}
		ecount[dp->d_ino]++;
	}
}

pass2(ip)
register struct dinode *ip;
{
	register i;

	i = ino;
	if ((ip->di_mode&IFMT)==0 && ecount[i]==0)
		return;
	if (ip->di_nlink==ecount[i] && ip->di_nlink!=0)
		return;
	if (headpr==0) {
		printf("     entries  link cnt\n");
		headpr++;
	}
	printf("%u\t%d\t%d\n", ino,
	    ecount[i], ip->di_nlink);
}

/*
 * get next entry in a directory.
 */
struct direct *
dreaddir(dirp)
	register struct dirstuff *dirp;
{
	register struct direct *dp;
	daddr_t lbn, d;

	for(;;) {
		if (dirp->loc >= dirp->ip->di_size)
			return NULL;
		if ((lbn = lblkno(&sblock, dirp->loc)) == 0) {
			d = bmap(lbn);
			if(d == 0)
				return NULL;
			bread(fsbtodb(&sblock, d), dirp->dbuf,
			    dblksize(&sblock, dirp->ip, lbn));
		}
		dp = (struct direct *)
		    (dirp->dbuf + blkoff(&sblock, dirp->loc));
		dirp->loc += dp->d_reclen;
		if (dp->d_ino == 0)
			continue;
		return (dp);
	}
}

bread(bno, buf, cnt)
daddr_t bno;
char *buf;
{
	register i;

	lseek(fi, bno * DEV_BSIZE, 0);
	if (read(fi, buf, cnt) != cnt) {
		printf("read error %d\n", bno);
		for(i=0; i < cnt; i++)
			buf[i] = 0;
	}
}

daddr_t
bmap(i)
{
	daddr_t ibuf[MAXNINDIR];

	if(i < NDADDR)
		return(gip->di_db[i]);
	i -= NDADDR;
	if(i > NINDIR(&sblock)) {
		printf("%u - huge directory\n", ino);
		return((daddr_t)0);
	}
	bread(fsbtodb(&sblock, gip->di_ib[0]), (char *)ibuf, sizeof(ibuf));
	return(ibuf[i]);
}
