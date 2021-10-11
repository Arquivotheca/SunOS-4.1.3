/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)diskusg.c 1.1 92/07/30 SMI" /* from S5R3 acct:diskusg.c 1.10 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#ifdef SYSTEMV
#include <sys/ino.h>
#include <sys/fs/s5param.h>
#include <sys/fs/s5filsys.h>
#include <sys/fs/s5macros.h>
#include <sys/sysmacros.h>
#else
#include <sys/time.h>
#include <sys/vnode.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#endif
#include <pwd.h>
#include <fcntl.h>

#ifdef SYSTEMV

#ifndef Fs2BLK
#define Fs2BLK	0
#endif

#ifndef FsINOS
#define FsINOS(bsize, x)	((x&~07)+1)
#endif

#else /* SYSTEMV */

#define	FsBSIZE(bsize)		(bsize)

#endif /* SYSTEMV */

#define BLOCK		512	/* Block size for reporting */

#ifdef SYSTEMV

#if pdp11
#define		NINODE		(INOPB * 2 * 10)
#else
#define		NINODE		(INOPB * 2 * 32)
#endif

#else /* SYSTEMV */

#define	NINODE	(MAXBSIZE/sizeof(struct dinode))

#endif /* SYSTEMV */

#define		MAXIGN		10
#define		FAIL		-1
#define		MAXNAME		8
#define		SUCCEED		0
#define		TRUE		1
#define		FALSE		0

#ifdef SYSTEMV
struct	filsys	sblock;
#else	/* SYSTEMV */
union {
	struct fs u_sblock;
	char dummy[SBSIZE];
} sb_un;
#define sblock sb_un.u_sblock
#endif	/* SYSTEMV */
struct	dinode	dinode[NINODE];

extern char	*malloc();
extern struct	passwd *getpwent(), *fgetpwent();

int	VERBOSE = 0;
FILE	*ufd = 0;
unsigned ino, nfiles;

struct acct  {
	struct	acct *next;
	int	uid;
	long	usage;
	char	name [MAXNAME+1];
};

/*
 * Hash table for accounting records.
 */
#define	MAXUHASH	8209
#define	HASH(u)	(((unsigned int)(u)) % MAXUHASH)
struct	acct *accthash[MAXUHASH];

struct	acct *acctpool;		/* mallocate in chunks of 500 */
int	nacctpool;

#ifdef SYSTEMV
char	*ignlist[MAXIGN];
int	igncnt = {0};
#endif /* SYSTEMV */

char	*cmd;


struct acct *lookup(), *enter();

main(argc, argv)
int argc;
char **argv;
{
	extern	int	optind;
	extern	char	*optarg;
	register c;
	register FILE	*fd;
	register	rfd;
	struct	stat	sb;
	int	sflg = {FALSE};
	char	*pfile = {NULL};
	int	errfl = {FALSE};

	cmd = argv[0];
	while((c = getopt(argc, argv, "vu:p:si:")) != EOF) switch(c) {
	case 's':
		sflg = TRUE;
		break;
	case 'v':
		VERBOSE = 1;
		break;
#ifdef SYSTEMV
	case 'i':
		ignore(optarg);
		break;
#endif /* SYSTEMV */
	case 'u':
		ufd = fopen(optarg, "a");
		break;
	case 'p':
		pfile = optarg;
		break;
	case '?':
		errfl++;
		break;
	}
	if(errfl) {
#ifdef SYSTEMV
		fprintf(stderr, "Usage: %s [-sv] [-p pw_file] [-u file] [-i ignlist] [file ...]\n", cmd);
#else /* SYSTEMV */
		fprintf(stderr, "Usage: %s [-sv] [-p pw_file] [-u file] [file ...]\n", cmd);
#endif /* SYSTEMV */
		exit(10);
	}

	hashinit();
	nacctpool = 0;
	if(sflg == TRUE) {
		if(optind == argc){
			adduser(stdin);
		} else {
			for( ; optind < argc; optind++) {
				if( (fd = fopen(argv[optind], "r")) == NULL) {
					fprintf(stderr, "%s: Cannot open %s\n", cmd, argv[optind]);
					continue;
				}
				adduser(fd);
				fclose(fd);
			}
		}
	}
	else {
		setup(pfile);
		for( ; optind < argc; optind++) {
			if( (rfd = open(argv[optind], O_RDONLY)) < 0) {
				fprintf(stderr, "%s: Cannot open %s\n", cmd, argv[optind]);
				continue;
			}
			if(fstat(rfd, &sb) >= 0){
				if ( (sb.st_mode & S_IFMT) == S_IFCHR ||
				     (sb.st_mode & S_IFMT) == S_IFBLK ) {
					ilist(argv[optind], rfd);
				} else {
					fprintf(stderr, "%s: %s is not a special file -- ignored\n", cmd, argv[optind]);
				}
			} else {
				fprintf(stderr, "%s: Cannot stat %s\n", cmd, argv[optind]);
			}
			close(rfd);
		}
	}
	output();
	exit(0);
}

adduser(fd)
register FILE	*fd;
{
	register struct acct *acctp;
	int	usrid;
	long	blcks;
	char	login[MAXNAME+10];

	while(fscanf(fd, "%d %s %ld\n", &usrid, login, &blcks) == 3) {
		if( (acctp = lookup(usrid)) == NULL) {
			if ((acctp = enter(usrid, login)) == NULL) {
				fprintf(stderr,"diskacct: Out of memory\n");
				return (FAIL);
			}
		}
		acctp->usage += blcks;
	}
}

ilist(file, fd)
char	*file;
register fd;
{
#ifdef SYSTEMV
	register int	bsize;
#else /* SYSTEMV */
	unsigned int ino;
	daddr_t iblk;
#endif /* SYSTEMV */
	register i, j;

	if (fd < 0 ) {
		return (FAIL);
	}

	sync();

#ifdef SYSTEMV
	/* Fake out block size to be 512 */
	bsize = 512;

	/* Read in super-block of filesystem */
	bread(fd, 1, &sblock, sizeof(sblock), bsize);

	/* Check for filesystem names to ignore */
	if(!todo(sblock.s_fname))
		return;
	/* Check for size of filesystem to be 512 or 1K */
	if (sblock.s_magic == FsMAGIC )
		bsize = 512 * sblock.s_type;

	nfiles = (sblock.s_isize-2) * FsINOPB(bsize);

	/* Determine physical block 2 */
	i = FsINOS(bsize, 2);
	i = FsITOD(bsize, i);

	/* Start at physical block 2, inode list */
	for (ino = 0; ino < nfiles; i += NINODE/FsINOPB(bsize)) {
		bread(fd, i, dinode, sizeof(dinode), bsize);
		for (j = 0; j < NINODE && ino++ < nfiles; j++)
			if (dinode[j].di_mode & S_IFMT)
				if(count(j, bsize) == FAIL) {
					if(VERBOSE)
						fprintf(stderr,"BAD UID: file system = %s, inode = %u, uid = %u\n",
					    	file, ino, dinode[j].di_uid);
					if(ufd)
						fprintf(ufd, "%s %u %u\n", file, ino, dinode[j].di_uid);
				}
	}
#else
	/* Read in super-block of filesystem */
	bread(fd, SBLOCK, (char *)&sblock, SBSIZE, DEV_BSIZE);

	nfiles = sblock.fs_ipg * sblock.fs_ncg;
	for (ino = 0; ino < nfiles; ) {
		iblk = fsbtodb(&sblock, itod(&sblock, ino));
		bread(fd, iblk, (char *)dinode, sblock.fs_bsize, DEV_BSIZE);
		for (j = 0; j < INOPB(&sblock) && ino < nfiles; j++, ino++) {
			if (ino < ROOTINO)
				continue;
			if (count(j) == FAIL) {
				if(VERBOSE)
					fprintf(stderr,"BAD UID: file system = %s, inode = %u, uid = %u\n",
					file, ino, dinode[j].di_uid);
				if(ufd)
					fprintf(ufd, "%s %u %u\n", file, ino, dinode[j].di_uid);
			}
		}
	}
#endif
	return (0);
}

#ifdef SYSTEMV
ignore(str)
register char	*str;
{
	char	*skip();

	for( ; *str && igncnt < MAXIGN; str = skip(str), igncnt++)
		ignlist[igncnt] = str;
	if(igncnt == MAXIGN) {
		fprintf(stderr, "%s: ignore list overflow. Recompile with larger MAXIGN\n", cmd);
	}
}
#endif

bread(fd, bno, buf, cnt, bsize)
register fd;
register unsigned bno;
register char *buf;
register int bsize;
{
	register int i;

	lseek(fd, (long)bno*FsBSIZE(bsize), 0);
	if ((i = read(fd, buf, cnt)) != cnt)
	{
		fprintf(stderr, "%s: read error at block %u: ", bno, cmd);
		if (i < 0)
			perror("");
		else
			fprintf(stderr, "Short read\n");
		exit(1);
	}
}

#ifdef SYSTEMV
count(j, bsize)
#else
count(j)
#endif
register j;
#ifdef SYSTEMV
register int bsize;
#endif
{
	register struct acct *acctp;
	long	blocks();

	if ( dinode[j].di_nlink == 0 || dinode[j].di_mode == 0 )
		return(SUCCEED);
	if( (acctp = lookup(dinode[j].di_uid)) == NULL)
		return (FAIL);
#ifdef SYSTEMV
	acctp->usage += blocks(j, bsize);
#else
	acctp->usage += dinode[(j)].di_blocks*DEV_BSIZE/BLOCK;
#endif
	return (SUCCEED);
}


output()
{
	register struct acct *acctp;
	struct acct **hp;

	for (hp = &accthash[0]; hp < &accthash[MAXUHASH]; hp++) {
		for (acctp = *hp; acctp != NULL; acctp = acctp->next) {
			if (acctp->usage != 0)
				printf("%u	%s	%ld\n",
				    acctp->uid, acctp->name, acctp->usage);
		}
	}
}

#ifdef SYSTEMV
#define SNGLIND(bsize)	(FsNINDIR(bsize))
#define DBLIND(bsize)	(FsNINDIR(bsize)*FsNINDIR(bsize))
#define	TRPLIND(bsize)	(FsNINDIR(bsize)*FsNINDIR(bsize)*FsNINDIR(bsize))

long
blocks(j, bsize)
register int j;
register int bsize;
{
	register long blks;

	blks = (dinode[j].di_size + FsBSIZE(bsize) - 1)/FsBSIZE(bsize);
	if(blks > 10) {
		blks += (blks-10+SNGLIND(bsize)-1)/SNGLIND(bsize);
		blks += (blks-10-SNGLIND(bsize)+DBLIND(bsize)-1)/DBLIND(bsize);
		blks += (blks-10-SNGLIND(bsize)-DBLIND(bsize)+TRPLIND(bsize)-1)/TRPLIND(bsize);
	}
	if(FsBSIZE(bsize) != BLOCK) {
		blks = (blks+BLOCK/FsBSIZE(bsize))*FsBSIZE(bsize)/BLOCK;
	}
	return(blks);
}
#endif

/*
 * Initialize the hash table.
 */
hashinit()
{
	register struct acct **hp;

	for (hp = accthash; hp < &accthash[MAXUHASH]; hp++)
		*hp = NULL;
}

/*
 * Find the hash bucket for this user ID, and search the chain to see if
 * there's a record for them.
 */
struct acct *
lookup(uid)
register int uid;
{
	struct acct **hp;
	register struct acct *acctp;

	hp = &accthash[HASH(uid)];
	for (acctp = *hp; acctp != NULL; acctp = acctp->next) {
		if (acctp->uid == uid)
			break;
	}
	return (acctp);
}

/*
 * Find the hash bucket for this user ID, and add a new record to that bucket.
 */
struct acct *
enter(uid, name)
int uid;
char *name;
{
	struct acct **hp;
	register struct acct *acctp;

	hp = &accthash[HASH(uid)];

	if (nacctpool == 0) {
		/*
		 * Grab 500 more records.
		 */
		if ((acctpool =
		    (struct acct *)malloc(500 * sizeof (struct acct)))
		      == NULL)
			return (NULL);	/* out of memory, you lose */
		nacctpool = 500;
	}
	nacctpool--;
	acctp = acctpool;
	acctpool++;
	acctp->next = *hp;
	*hp = acctp;
	acctp->uid = uid;
	acctp->usage = 0;
	strncpy(acctp->name, name, MAXNAME);
	acctp->name[MAXNAME] = '\0';
	return (acctp);
}

setup(pfile)
char	*pfile;
{
	register struct passwd	*pw;
	register FILE *pwf;

	if( pfile != NULL) {
		if( (pwf = fopen(pfile, "r")) == NULL) {
			fprintf(stderr, "%s: Cannot open %s\n", cmd, pfile);
			exit(5);
		}
	}
	while ( (pw=(pfile == NULL ? getpwent() : fgetpwent(pwf))) != NULL )
	{
		if ( lookup(pw->pw_uid) == NULL )
		{
			if (enter(pw->pw_uid, pw->pw_name) == NULL)
			{
				fprintf(stderr,"diskacct: Out of memory\n");
				return (FAIL);
			}
		}
	}
	if (pfile != NULL)
		fclose(pwf);
}

#ifdef SYSTEMV
todo(fname)
register char	*fname;
{
	register	i;

	for(i = 0; i < igncnt; i++) {
		if(strncmp(fname, ignlist[i], 6) == 0) return(FALSE);
	}
	return(TRUE);
}

char	*
skip(str)
register char	*str;
{
	while(*str) {
		if(*str == ' ' ||
		    *str == ',') {
			*str = '\0';
			str++;
			break;
		}
		str++;
	}
	return(str);
}
#endif /* SYSTEMV */
