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

#ident	"@(#)utilities.c 1.1 92/07/30 SMI" /* from UCB 5.23 2/7/90 */

#include <stdio.h>
#include <limits.h>
#include <sys/unistd.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#include <ufs/fsdir.h>
#include <mntent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include "fsck.h"

long	diskreads, totalreads;	/* Disk cache statistics */
extern	off_t	lseek();

ftypeok(dp)
	struct dinode *dp;
{
	switch (dp->di_mode & IFMT) {

	case IFDIR:
	case IFREG:
	case IFBLK:
	case IFCHR:
	case IFLNK:
	case IFSOCK:
	case IFIFO:
		return (1);

	default:
		if (debug)
			printf("bad file type 0%o\n", dp->di_mode);
		return (0);
	}
}

reply(question)
	char *question;
{
	char line[80];

	if (preen)
		pfatal("INTERNAL ERROR: GOT TO reply()");
	printf("\n%s? ", question);
	if (nflag || fswritefd < 0) {
		printf(" no\n\n");
		iscorrupt = 1;		/* known to be corrupt */
		return (0);
	}
	if (yflag) {
		printf(" yes\n\n");
		return (1);
	}
	if (getline(stdin, line, sizeof (line)) == -1)
		errexit("\n");
	printf("\n");
	if (line[0] == 'y' || line[0] == 'Y')
		return (1);
	else {
		iscorrupt = 1;		/* known to be corrupt */
		return (0);
	}
}

getline(fp, loc, maxlen)
	FILE *fp;
	char *loc;
{
	register n;
	register char *p, *lastloc;

	p = loc;
	lastloc = &p[maxlen-1];
	while ((n = getc(fp)) != '\n') {
		if (n == EOF)
			return (-1);
		if (!isspace(n) && p < lastloc)
			*p++ = n;
	}
	*p = 0;
	return (p - loc);
}
/*
 * Malloc buffers and set up cache.
 */
bufinit()
{
	register struct bufarea *bp;
	long bufcnt, i;
	char *bufp;

	bufp = malloc((u_int)sblock.fs_bsize);
	if (bufp == NULL)
		errexit("cannot allocate buffer pool\n");
	cgblk.b_un.b_buf = bufp;
	initbarea(&cgblk);
	bufhead.b_next = bufhead.b_prev = &bufhead;
	bufcnt = MAXBUFSPACE / sblock.fs_bsize;
	if (bufcnt < MINBUFS)
		bufcnt = MINBUFS;
	for (i = 0; i < bufcnt; i++) {
		bp = (struct bufarea *)malloc(sizeof (struct bufarea));
		bufp = malloc((unsigned int)sblock.fs_bsize);
		if (bp == NULL || bufp == NULL) {
			if (i >= MINBUFS)
				break;
			errexit("cannot allocate buffer pool\n");
		}
		bp->b_un.b_buf = bufp;
		bp->b_prev = &bufhead;
		bp->b_next = bufhead.b_next;
		bufhead.b_next->b_prev = bp;
		bufhead.b_next = bp;
		initbarea(bp);
	}
	bufhead.b_size = i;	/* save number of buffers */
	pbp = pdirbp = NULL;
}

/*
 * Manage a cache of directory blocks.
 */
struct bufarea *
getdatablk(blkno, size)
	daddr_t blkno;
	long size;
{
	register struct bufarea *bp;

	for (bp = bufhead.b_next; bp != &bufhead; bp = bp->b_next)
		if (bp->b_bno == fsbtodb(&sblock, blkno))
			goto foundit;
	for (bp = bufhead.b_prev; bp != &bufhead; bp = bp->b_prev)
		if ((bp->b_flags & B_INUSE) == 0)
			break;
	if (bp == &bufhead)
		errexit("deadlocked buffer pool\n");
	getblk(bp, blkno, size);
	/* fall through */
foundit:
	totalreads++;
	bp->b_prev->b_next = bp->b_next;
	bp->b_next->b_prev = bp->b_prev;
	bp->b_prev = &bufhead;
	bp->b_next = bufhead.b_next;
	bufhead.b_next->b_prev = bp;
	bufhead.b_next = bp;
	bp->b_flags |= B_INUSE;
	return (bp);
}

struct bufarea *
getblk(bp, blk, size)
	register struct bufarea *bp;
	daddr_t blk;
	long size;
{
	daddr_t dblk;

	dblk = fsbtodb(&sblock, blk);
	if (bp->b_bno == dblk)
		return (bp);
	flush(fswritefd, bp);
	diskreads++;
	bp->b_errs = bread(fsreadfd, bp->b_un.b_buf, dblk, size);
	bp->b_bno = dblk;
	bp->b_size = size;
	return (bp);
}

flush(fd, bp)
	int fd;
	register struct bufarea *bp;
{
	register int i, j;

	if (!bp->b_dirty)
		return;
	if (bp->b_errs != 0)
		pfatal("WRITING ZERO'ED BLOCK %d TO DISK\n", bp->b_bno);
	bp->b_dirty = 0;
	bp->b_errs = 0;
	bwrite(fd, bp->b_un.b_buf, bp->b_bno, (long)bp->b_size);
	if (bp != &sblk)
		return;
	for (i = 0, j = 0; i < sblock.fs_cssize; i += sblock.fs_bsize, j++) {
		bwrite(fswritefd, (char *)sblock.fs_csp[j],
		    fsbtodb(&sblock, sblock.fs_csaddr + j * sblock.fs_frag),
		    MIN(sblock.fs_cssize - i, sblock.fs_bsize));
	}
}

rwerror(mesg, blk)
	char *mesg;
	daddr_t blk;
{

	if (preen == 0)
		printf("\n");
	pfatal("CANNOT %s: BLK %ld", mesg, blk);
	if (reply("CONTINUE") == 0)
		errexit("Program terminated\n");
}

ckfini()
{
	register struct bufarea *bp, *nbp;
	int cnt = 0;

	flush(fswritefd, &sblk);
	if (havesb && sblk.b_bno != SBLOCK) {
		sblk.b_bno = SBLOCK;
		sbdirty();
		flush(fswritefd, &sblk);
	}
	flush(fswritefd, &cgblk);
	if (cgblk.b_un.b_buf) {
		free(cgblk.b_un.b_buf);
		cgblk.b_un.b_buf = NULL;
	}
	for (bp = bufhead.b_prev; bp && bp != &bufhead; bp = nbp) {
		cnt++;
		flush(fswritefd, bp);
		nbp = bp->b_prev;
		free(bp->b_un.b_buf);
		free((char *)bp);
	}
	pbp = pdirbp = NULL;
	if (bufhead.b_size != cnt)
		errexit("Panic: lost %d buffers\n", bufhead.b_size - cnt);
	if (debug)
		printf("cache missed %d of %d (%d%%)\n",
		    diskreads, totalreads,
		    totalreads ? diskreads * 100 / totalreads : 0);
	(void)close(fsreadfd);
	(void)close(fswritefd);
}

bread(fd, buf, blk, size)
	int fd;
	char *buf;
	daddr_t blk;
	long size;
{
	char *cp;
	int i, errs;
	int nblks = btodb(size);

	if (debug && blk < SBLOCK) {
		printf("WARNING: fsck bread() passed blkno < %d (%ld)\n",
		    SBLOCK, blk);
	}
	if (lseek(fd, (off_t)dbtob(blk), L_SET) < 0)
		rwerror("SEEK", blk);
	else if (read(fd, buf, (int)size) == size)
		return (0);
	rwerror("READ", blk);
	if (lseek(fd, (off_t)dbtob(blk), L_SET) < 0)
		rwerror("SEEK", blk);
	errs = 0;
	bzero(buf, (int)size);
	pwarn("THE FOLLOWING SECTORS COULD NOT BE READ:");
	for (cp = buf, i = 0; i < nblks; i++, cp += DEV_BSIZE) {
		if (lseek(fd, (off_t)dbtob(blk + i), L_SET) < 0 ||
		    read(fd, cp, DEV_BSIZE) < 0) {
			printf(" %d,", blk + i);
			errs++;
		}
	}
	printf("\n");
	return (errs);
}

bwrite(fd, buf, blk, size)
	int fd;
	char *buf;
	daddr_t blk;
	long size;
{
	int i, n;
	char *cp;
	int nblks = btodb(size);

	if (fd < 0)
		return;
	if (blk < SBLOCK) {
		printf("WARNING: attempt to write illegal blkno %d on %s\n",
		    blk, devname);
		return;
	}
	if (lseek(fd, (off_t)dbtob(blk), L_SET) < 0)
		rwerror("SEEK", blk);
	else  {
		if ((n = write(fd, buf, (int)size)) > 0)
			fsmodified = 1;
		if (n == size)
			return;
	}
	rwerror("WRITE", blk);
	if (lseek(fd, (off_t)dbtob(blk), L_SET) < 0)
		rwerror("SEEK", blk);
	pwarn("THE FOLLOWING SECTORS COULD NOT BE WRITTEN:");
	for (cp = buf, i = 0; i < nblks; i++, cp += DEV_BSIZE) {
		n = 0;
		if (lseek(fd, (off_t)dbtob(blk + i), L_SET) < 0 ||
		    (n = write(fd, cp, DEV_BSIZE)) < 0) {
			printf(" %d,", blk + i);
		} else if (n > 0) {
			fsmodified = 1;
		}
	}
	printf("\n");
}

/*
 * allocate a data block with the specified number of fragments
 */
allocblk(frags)
	long frags;
{
	register int i, j, k;

	if (frags <= 0 || frags > sblock.fs_frag)
		return (0);
	for (i = 0; i < maxfsblock - sblock.fs_frag; i += sblock.fs_frag) {
		for (j = 0; j <= sblock.fs_frag - frags; j++) {
			if (testbmap(i + j))
				continue;
			for (k = 1; k < frags; k++)
				if (testbmap(i + j + k))
					break;
			if (k < frags) {
				j += k;
				continue;
			}
			for (k = 0; k < frags; k++)
				setbmap(i + j + k);
			n_blks += frags;
			return (i + j);
		}
	}
	return (0);
}

/*
 * Free a previously allocated block
 */
freeblk(blkno, frags)
	daddr_t blkno;
	long frags;
{
	struct inodesc idesc;

	idesc.id_blkno = blkno;
	idesc.id_numfrags = frags;
	pass4check(&idesc);
}

/*
 * Find a pathname
 */
getpathname(namebuf, curdir, ino)
	char *namebuf;
	ino_t curdir, ino;
{
	int len;
	register char *cp;
	struct inodesc idesc;
	struct inoinfo *inp;
	extern int findname();

	if (statemap[curdir] != DSTATE && statemap[curdir] != DFOUND) {
		strcpy(namebuf, "?");
		return;
	}
	bzero((char *)&idesc, sizeof (struct inodesc));
	idesc.id_type = DATA;
	cp = &namebuf[MAXPATHLEN - 1];
	*cp = '\0';
	if (curdir != ino) {
		idesc.id_parent = curdir;
		goto namelookup;
	}
	while (ino != ROOTINO) {
		idesc.id_number = ino;
		idesc.id_func = findino;
		idesc.id_name = "..";
		idesc.id_fix = NOFIX;
		if ((ckinode(ginode(ino), &idesc) & FOUND) == 0) {
			inp = getinoinfo(ino);
			if (inp->i_parent == 0)
				break;
			idesc.id_parent = inp->i_parent;
		}
	namelookup:
		idesc.id_number = idesc.id_parent;
		idesc.id_parent = ino;
		idesc.id_func = findname;
		idesc.id_name = namebuf;
		idesc.id_fix = NOFIX;
		if ((ckinode(ginode(idesc.id_number), &idesc)&FOUND) == 0)
			break;
		len = strlen(namebuf);
		cp -= len;
		if (cp < &namebuf[MAXNAMLEN])
			break;
		bcopy(namebuf, cp, len);
		*--cp = '/';
		ino = idesc.id_number;
	}
	if (ino != ROOTINO) {
		strcpy(namebuf, "?");
		return;
	}
	bcopy(cp, namebuf, &namebuf[MAXPATHLEN] - cp);
}

void
catch()
{
	ckfini();
	exit(12);
}

/*
 * When preening, allow a single quit to signal
 * a special exit after filesystem checks complete
 * so that reboot sequence may be interrupted.
 */
void
catchquit()
{
	extern returntosingle;

	printf("returning to single-user after filesystem check\n");
	returntosingle = 1;
	(void)signal(SIGQUIT, SIG_DFL);
}

/*
 * Ignore a single quit signal; wait and flush just in case.
 * Used by child processes in preen.
 */
void
voidquit()
{
	sleep((u_int)1);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_DFL);
}

/*
 * determine whether an inode should be fixed.
 */
dofix(idesc, msg)
	register struct inodesc *idesc;
	char *msg;
{
	switch (idesc->id_fix) {

	case DONTKNOW:
		if (idesc->id_type == DATA)
			direrror(idesc->id_number, msg);
		else
			pwarn(msg);
		if (preen) {
			printf(" (SALVAGED)\n");
			idesc->id_fix = FIX;
			return (ALTERED);
		}
		if (reply("SALVAGE") == 0) {
			idesc->id_fix = NOFIX;
			return (0);
		}
		idesc->id_fix = FIX;
		return (ALTERED);

	case FIX:
		return (ALTERED);

	case NOFIX:
		return (0);

	default:
		errexit("UNKNOWN INODESC FIX MODE %d\n", idesc->id_fix);
	}
	/* NOTREACHED */
}

#include <varargs.h>

/* VARARGS */
errexit(va_alist)
	va_dcl
{
	va_list args;
	char *fmt;

	va_start(args);
	fmt = va_arg(args, char *);
	vprintf(fmt, args);
	va_end(args);
	exit(8);
}

/*
 * An unexpected inconsistency occured.
 * Die if preening, otherwise just print message and continue.
 */
/* VARARGS */
pfatal(va_alist)
	va_dcl
{
	va_list args;
	char *fmt;

	if (preen)
		printf("%s: ", devname);
	va_start(args);
	fmt = va_arg(args, char *);
	vprintf(fmt, args);
	va_end(args);
	if (preen) {
		printf("\n");
		printf("%s: UNEXPECTED INCONSISTENCY; RUN fsck MANUALLY.\n",
			devname);
		exit(8);
	}
}

/*
 * Pwarn just prints a message when not preening,
 * or a warning (preceded by filename) when preening.
 */
/* VARARGS */
pwarn(va_alist)
	va_dcl
{
	va_list args;
	char *fmt;

	if (preen)
		printf("%s: ", devname);
	va_start(args);
	fmt = va_arg(args, char *);
	vprintf(fmt, args);
	va_end(args);
}

/*
 * Stub for routines from kernel.
 */
panic(s)
	char *s;
{

	pfatal("INTERNAL INCONSISTENCY:");
	errexit(s);
}

/*
 * Check to see if unraw version of name is already mounted.
 * Since we do not believe /etc/mtab, we stat the mount point
 * to see if it is really looks mounted.
 */
mounted(name)
	char *name;
{
	struct mntent *mnt;
	FILE *mnttab;
	struct stat device_stat, mount_stat;
	char *blkname, *unrawname();
	static char buf[MAXPATHLEN];

	mnttab = setmntent(MOUNTED, "r");
	if (mnttab == NULL) {
		printf("can't open %s\n", MOUNTED);
		return (0);
	}
	(void) strncpy(buf, name, sizeof (buf));
	blkname = unrawname(buf);
	while ((mnt = getmntent(mnttab)) != NULL) {
		if (strcmp(mnt->mnt_type, MNTTYPE_42) != 0) {
			continue;
		}
		if (strcmp(blkname, mnt->mnt_fsname) == 0) {
			if (stat(mnt->mnt_dir, &mount_stat) < 0) {
				return (0);
			}
			if (stat(mnt->mnt_fsname, &device_stat) < 0) {
				printf("can't stat %s\n", mnt->mnt_fsname);
				return (0);
			}
			return (device_stat.st_rdev == mount_stat.st_dev);
		}
	}
	endmntent(mnttab);
	return (0);
}

/*
 * Check to see if name corresponds to an entry in fstab, and that the entry
 * does not have option ro.
 */
writable(name)
	char *name;
{
	int rw = 1;
	struct mntent *mnt;
	FILE *mnttab;

	mnttab = setmntent(MNTTAB, "r");
	if (mnttab == NULL) {
		printf("can't open %s\n", MNTTAB);
		return (1);
	}
	while ((mnt = getmntent(mnttab)) != NULL) {
		if (strcmp(mnt->mnt_type, MNTTYPE_42) != 0) {
			if ((strcmp(name, mnt->mnt_fsname) == 0) ||
			    (strcmp(name, mnt->mnt_dir) == 0)) {
				printf ("%s mount type is %s - ignored\n",
				    name, mnt->mnt_type);
				rw = 0;
			}
			continue;
		}
		if ((strcmp(name, mnt->mnt_fsname) == 0) ||
		    (strcmp(name, mnt->mnt_dir) == 0)) {
			if (hasmntopt(mnt, MNTOPT_RO)) {
				rw = 0;
			}
			break;
		}
	}
	endmntent(mnttab);
	return (rw);
}

/*
 * debugclean
 */
debugclean()
{
	if (debug == 0)
		return;

	if ((iscorrupt == 0) && (isdirty == 0))
		return;

	if ((sblock.fs_clean != FSSTABLE) && (sblock.fs_clean != FSCLEAN))
		return;

	if (FSOKAY != (fs_get_state(&sblock) + sblock.fs_time))
		return;

	printf("WARNING: inconsistencies detected on `%s' filesystem %s\n",
	    sblock.fs_clean == FSSTABLE ? "stable" : "clean", devname);
}

/*
 * updateclean
 *	Carefully and transparently update the clean flag.
 */
updateclean()
{
	struct bufarea	cleanbuf;
	unsigned int	size;
	daddr_t		bno;
	unsigned int	fsclean;

	/*
	 * debug stuff
	 */
	debugclean();

	/*
	 * set fsclean to its appropriate value
	 */
	fsclean = sblock.fs_clean;
	if (FSOKAY != (fs_get_state(&sblock) + sblock.fs_time))
		fsclean = FSACTIVE;

	/*
	 * if necessary, update fs_clean and fs_state
	 */
	switch (fsclean) {

	case FSACTIVE:
		if (iscorrupt == 0) fsclean = FSSTABLE;
		break;

	case FSCLEAN:
	case FSSTABLE:
		if (iscorrupt) fsclean = FSACTIVE;
		break;

	default:
		if (iscorrupt)
			fsclean = FSACTIVE;
		else
			fsclean = FSSTABLE;
	}
	/*
	 * if fs_clean and fs_state are ok, do nothing
	 */
	if ((sblock.fs_clean == fsclean) &&
	    (FSOKAY == (fs_get_state(&sblock) + sblock.fs_time)))
		return;

	/*
	 * if user allows, update superblock clean flag
	 */
	if (preen == 0)
		if ((reply("CLEAN FLAG IN SUPERBLOCK IS WRONG; FIX")) == 0)
			return;

	sblock.fs_clean = fsclean;
	fs_set_state(&sblock, FSOKAY - sblock.fs_time);

	/*
	 * if superblock can't be written, return
	 */
	if (fswritefd < 0)
		return;

	/*
	 * read private copy of superblock, update clean flag, and write it
	 */
	bno  = sblk.b_bno;
	size = sblk.b_size;

	if (lseek(fsreadfd, (off_t)dbtob(bno), L_SET) == -1)
		return;

	if ((cleanbuf.b_un.b_buf = malloc(size)) == NULL)
		errexit("cannot allocate cleanbuf\n");
	if (read(fsreadfd, cleanbuf.b_un.b_buf, (int)size) != size)
		return;

	cleanbuf.b_un.b_fs->fs_clean = sblock.fs_clean;
	fs_set_state(cleanbuf.b_un.b_fs, fs_get_state(&sblock));
	cleanbuf.b_un.b_fs->fs_time  = sblock.fs_time;

	if (lseek(fswritefd, (off_t)dbtob(bno), L_SET) == -1)
		return;
	if (write(fswritefd, cleanbuf.b_un.b_buf, (int)size) != size)
		return;
}

/*
 * print out clean info
 */
printclean()
{
	char	*s;

	if (FSOKAY != (fs_get_state(&sblock) + sblock.fs_time))
		s = "unknown";
	else
		switch (sblock.fs_clean) {

		case FSACTIVE:
			s = "active";
			break;

		case FSCLEAN:
			s = "clean";
			break;

		case FSSTABLE:
			s = "stable";
			break;

		default:
			s = "unknown";
		}

	if (preen)
		pwarn("is %s.\n", s);
	else
		printf("** %s is %s.\n", devname, s);
}

extern int errno;
extern char *sys_errlist[];

/*
 * Like perror(), but writes to stdout instead of stderr.  This is
 * because fsck writes all of its messages to stdout and it's too
 * late to turn back the tide.
 */
perrout(s)
	char *s;
{
	if (s != NULL && *s != '\0')
		printf("%s: ", s);
	printf("%s\n", sys_errlist[errno]);
}
