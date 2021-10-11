#ifndef lint
static char sccsid[] = "@(#)sys.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1985-1989 Sun Microsystems, Inc.
 */

#ifndef KADB

/*
 * Basic file system reading code for standalone I/O system.
 * Simulates a primitive UNIX I/O system (read(), write(), open(), etc).
 * Does not attempt writes to file systems, tho writing to whole devices
 * is supported, when the driver supports it.
 */

#include <stand/param.h>
#include <stand/saio.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <ufs/fsdir.h>
#include <ufs/fs.h>
#include <ufs/inode.h>

#define NULL 0
#define	skipblank(p)	{ while (*(p) == ' ') (p)++; }

struct direct *readdir();

/*
 * This struct keeps track of an open file in the standalone I/O system.
 *
 * It includes an IOB for device addess, an inode, a buffer for reading
 * indirect blocks and inodes, and a buffer for the superblock of the
 * file system (if any).
 */
struct iob {
	struct saioreq	i_si;		/* I/O request block for this file */
	struct inode	i_ino;		/* Inode for this file */
	char		i_buf[MAXBSIZE];/* Buffer for reading inodes & dirs */
	union {	
		struct fs ui_fs;	/* Superblock for file system */
		char dummy[SBSIZE];
	}		i_un;
};
#define i_flgs		i_si.si_flgs
#define i_boottab	i_si.si_boottab
#define i_devdata	i_si.si_devdata
#define i_ctlr		i_si.si_ctlr
#define i_unit		i_si.si_unit
#define i_boff		i_si.si_boff
#define i_cyloff	i_si.si_cyloff
#define i_offset	i_si.si_offset
#define i_bn		i_si.si_bn
#define i_ma		i_si.si_ma
#define i_cc		i_si.si_cc
#define i_fs		i_un.ui_fs

/* These are the pools of buffers, iob's, etc. */
#define	NBUFS	(NIADDR+1)	/* NOT! a variable */
char		b[NBUFS][MAXBSIZE];
daddr_t		blknos[NBUFS];
struct saioreq	*open_sip;
struct iob	iob[NFILES];

ino_t	dlook();

struct dirstuff {
	int loc;
	struct iob *io;
};

static
openi(n,io)
	ino_t n;
	register struct iob *io;
{
	register struct dinode *dp;

	io->i_offset = 0;
	io->i_bn = fsbtodb(&io->i_fs, itod(&io->i_fs, n));
	io->i_cc = io->i_fs.fs_bsize;
	io->i_ma = io->i_buf;
	/* FIXME: this call to devread() does not check for errors! */
	devread(&io->i_si);
	dp = (struct dinode *)io->i_buf;
	io->i_ino.i_ic = dp[itoo(&io->i_fs, n)].di_ic;
}

static ino_t
find(path, file)
	register char *path;
	struct iob *file;
{
	register char *q;
	char c;
	ino_t n;

	if (path==NULL || *path=='\0') {
		printf("null path\n");
		return(0);
	}

	openi((ino_t) ROOTINO, file);
	while (*path) {
		while (*path == '/')
			path++;
		q = path;
		while(*q != '/' && *q != '\0')
			q++;
		c = *q;
		*q = '\0';

		if ((n=dlook(path, file))!=0) {
			if (c=='\0')
				break;
			openi(n, file);
			*q = c;
			path = q;
			continue;
		} else {
			printf("%s not found\n",path);
			return(0);
		}
	}
	return(n);
}

static daddr_t
sbmap(io, bn)
	register struct iob *io;
	daddr_t bn;
{
	register struct inode *ip;
	register int i, j, sh;
	register daddr_t nb, *bap;
	register daddr_t *db;

	ip = &io->i_ino;
	db = ip->i_db;

	/*
	 * blocks 0..NDADDR are direct blocks
	 */
	if(bn < NDADDR) {
		nb = db[bn];
		return(nb);
	}

#ifndef BOOTBLOCK
	/*
	 * addresses NIADDR have single and double indirect blocks.
	 * the first step is to determine how many levels of indirection.
	 */
	sh = 1;
	bn -= NDADDR;
	for (j = NIADDR; j > 0; j--) {
		sh *= NINDIR(&io->i_fs);
		if (bn < sh)
			break;
		bn -= sh;
	}
	if (j == 0) {
		printf("bn ovf %D\n", bn);
		return ((daddr_t)0);
	}

	/*
	 * fetch the first indirect block address from the inode
	 */
	nb = ip->i_ib[NIADDR - j];
	if (nb == 0) {
		printf("bn void %D\n",bn);
		return((daddr_t)0);
	}

	/*
	 * fetch through the indirect blocks
	 */
	for (; j <= NIADDR; j++) {
		if (blknos[j] != nb) {
			io->i_bn = fsbtodb(&io->i_fs, nb);
			io->i_ma = b[j];
			io->i_cc = io->i_fs.fs_bsize;
			if (devread(&io->i_si) != io->i_cc)
				return((daddr_t)0);
			blknos[j] = nb;
		}
		bap = (daddr_t *)b[j];
		sh /= NINDIR(&io->i_fs);
		i = (bn / sh) % NINDIR(&io->i_fs);
		nb = bap[i];
		if(nb == 0) {
			printf("bn void %D\n",bn);
			return((daddr_t)0);
		}
	}
	return(nb);
#else	BOOTBLOCK
	return((daddr_t)0);
#endif	BOOTBLOCK
}

static ino_t
dlook(s, io)
	char *s;
	register struct iob *io;
{
	register struct direct *dp;
	register struct inode *ip;
	struct dirstuff dirp;
	register int len;

	ip = &io->i_ino;
#ifndef BOOTBLOCK
	if (s == NULL || *s == '\0')
		return(0);
	if ((ip->i_mode&IFMT) != IFDIR) {
		printf("not a directory\n");
		return(0);
	}
	if (ip->i_size == 0) {
		printf("zero length directory\n");
		return(0);
	}
#endif BOOTBLOCK
	len = strlen(s);
	dirp.loc = 0;
	dirp.io = io;
	for (dp = readdir(&dirp); dp != NULL; dp = readdir(&dirp)) {
		if(dp->d_ino == 0)
			continue;
		if (dp->d_namlen == len && !strcmp(s, dp->d_name))
			return(dp->d_ino);
#ifndef		BOOTBLOCK
		/* Allow "*" to print all names at that level, w/out match */
		if (!strcmp(s, "*"))
			printf("%s\n", dp->d_name);
#endif		BOOTBLOCK
	}
	return(0);
}

/*
 * get next entry in a directory.
 */
struct direct *
readdir(dirp)
	register struct dirstuff *dirp;
{
	register struct direct *dp;
	register struct iob *io;
	register daddr_t lbn, d;
	register int off;

	io = dirp->io;
	for(;;) {
		if (dirp->loc >= io->i_ino.i_size)
			return NULL;
		off = blkoff(&io->i_fs, dirp->loc);
		if (off == 0) {
			lbn = lblkno(&io->i_fs, dirp->loc);
			d = sbmap(io, lbn);
			if(d == 0)
				return NULL;
			io->i_bn = fsbtodb(&io->i_fs, d);
			io->i_ma = io->i_buf;
			io->i_cc = blksize(&io->i_fs, &io->i_ino, lbn);
			if (devread(&io->i_si) != io->i_cc)
				return NULL;
		}
		dp = (struct direct *)(io->i_buf + off);
		dirp->loc += dp->d_reclen;
		if (dp->d_ino == 0)
			continue;
		return (dp);
	}
}

/*ARGSUSED*/
lseek(fdesc, addr, ptr)
	int	fdesc;
	register off_t	addr;
	int	ptr;
{
	register struct iob *io;

#ifndef	BOOTBLOCK
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((io = &iob[fdesc])->i_flgs & F_ALLOC) == 0)
		return(-1);
#else	BOOTBLOCK
	io = &iob[fdesc - 3];
#endif	BOOTBLOCK
	io->i_si.si_flgs &= ~F_EOF;
	io->i_offset = addr;
	io->i_bn = addr / DEV_BSIZE;
	io->i_cc = 0;
	return(0);
}

/* FIXME: possibly make this static and pass iob, eliminating assorted tests? */
getc(fdesc)
	int	fdesc;
{
	register struct iob *io;
	register struct fs *fs;
	register char *p;
	register int c, off, size, diff;
	register daddr_t lbn;


#ifndef BOOTBLOCK
	if (fdesc >= 0 && fdesc <= 2)
		return(getchar());
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((io = &iob[fdesc])->i_flgs&F_ALLOC) == 0)
		return(-1);
#else	BOOTBLOCK
	io = &iob[fdesc -= 3];
#endif	BOOTBLOCK
	p = io->i_ma;
	if (io->i_cc <= 0) {
		if ((io->i_flgs & F_FILE) != 0) {
			diff = io->i_ino.i_size - io->i_offset;
			if (diff <= 0)
				return (-1);
			fs = &io->i_fs;
			lbn = lblkno(fs, io->i_offset);
			io->i_bn = fsbtodb(fs, sbmap(io, lbn));
			off = blkoff(fs, io->i_offset);
			size = blksize(fs, &io->i_ino, lbn);
		} else {
			io->i_bn = io->i_offset / DEV_BSIZE;
			off = 0;
			size = DEV_BSIZE;
		}
		io->i_ma = io->i_buf;
		io->i_cc = size;
		if (devread(&io->i_si) != io->i_cc)	/* Trap errors */
			return(-1);
		if ((io->i_flgs & F_FILE) != 0) {
			if (io->i_offset - off + size >= io->i_ino.i_size)
				io->i_cc = diff + off;
			io->i_cc -= off;
		}
		p = &io->i_buf[off];
	}
	io->i_cc--;
	io->i_offset++;
	c = (unsigned)*p++;
	io->i_ma = p;
	return(c);
}


read(fdesc, buf, count)
	int	fdesc;
	register char	*buf;
	int	count;
{
	register i,j;
	register struct iob *file;

#ifndef	BOOTBLOCK
	if (fdesc >= 0 && fdesc <= 2) {
		i = count;
		do {
			*buf = getchar();
		} while (--i && *buf++ != '\n');
		return(count - i);
	}
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0)
		return(-1);
	if ((file->i_flgs&F_READ) == 0)
		return(-1);
	fdesc += 3;
#else	BOOTBLOCK
	file = &iob[fdesc - 3];
#endif	BOOTBLOCK
	if ((file->i_flgs & F_FILE) == 0) {
		file->i_cc = count;
		file->i_ma = buf;
		file->i_bn = (file->i_offset / DEV_BSIZE);
		i = devread(&file->i_si);
		file->i_offset += count;
		return(i);
	} else {
		if (file->i_offset+count > file->i_ino.i_size)
			count = file->i_ino.i_size - file->i_offset;
		if ((i = count) <= 0)
			return(0);
#ifdef		BOOTBLOCK
		do {
			*buf++ = getc(fdesc);
		} while (--i);
#else		BOOTBLOCK
		while (i > 0) {
			if ((j = file->i_cc) <= 0) {
				*buf++ = getc(fdesc);
				i--;
			} else {
				if (i < j)
					j = i;
				bcopy(file->i_ma, buf, (unsigned)j);
				buf += j;
				file->i_ma += j;
				file->i_offset += j;
				file->i_cc -= j;
				i -= j;
			}
		}
#endif		BOOTBLOCK
		return(count);
	}
}

#ifndef BOOTBLOCK
write(fdesc, buf, count)
	int	fdesc;
	char	*buf;
	int	count;
{
	register i;
	register struct iob *file;

	if (fdesc >= 0 && fdesc <= 2) {
		i = count;
		while (i--)
			putchar(*buf++);
		return(count);
	}
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0)
		return(-1);
	if ((file->i_flgs&F_WRITE) == 0)
		return(-1);
	if (count % DEV_BSIZE)
		printf("count=%d?\n", count);
	file->i_cc = count;
	file->i_ma = buf;
	file->i_bn = (file->i_offset / DEV_BSIZE);
	i = devwrite(&file->i_si);
	file->i_offset += count;
	return(i);
}
#endif BOOTBLOCK

/*
 * Open a file on a physical device, or open the device itself.
 *
 * The device is identified by a struct bootparam which gives pointers
 * to the struct boottab, containing the device open, strategy and close
 * routines; and the device parameters such as controller #, unit #, etc.
 *
 * If *str == 0, the device is opened, else the named file in the file
 * system on the device is opened.
 */
int
physopen(bp, str, how)
	register struct bootparam *bp;
	char		*str;
	int		how;
{
	int fdesc;
	register struct iob *file;

#ifdef	BOOTBLOCK
	fdesc = 0;
	file = &iob[0];
	file->i_flgs |= F_ALLOC;
#else	BOOTBLOCK
	fdesc = getiob();		/* Allocate an IOB */
	file = &iob[fdesc];
#endif	BOOTBLOCK

	file->i_boottab = bp->bp_boottab; /* Record pointer to boot table */
	file->i_ctlr = bp->bp_ctlr;
	file->i_unit = bp->bp_unit;
	file->i_boff = bp->bp_part;
	file->i_ino.i_dev = 0;	/* Call it device 0 for chuckles */

	return(openfile(fdesc, file, str, how));
}

#ifndef BOOTBLOCK

int	openfirst = 1;

/*
 * Allocate an IOB for a newly opened file.
 */
int
getiob()
{
	register int fdesc;

	if (openfirst) {
		openfirst = 0;
		for (fdesc = 0; fdesc < NFILES; fdesc++)
			iob[fdesc].i_flgs = 0;
	}

	for (fdesc = 0; fdesc < NFILES; fdesc++)
		if (iob[fdesc].i_flgs == 0)
			goto gotfile;
	_stop("No more file slots");
gotfile:
	(&iob[fdesc])->i_flgs |= F_ALLOC;
	return fdesc;
}


/*
 * Description: Reads string for hex variable.
 *              Also ignores blanks.  Returns a pointer pointing at the next
 *              non-blank character, skipping over one ','.
 *
 * Synopsis:    status = gethex(p, ip)
 *              status  :(char *) pointer to next non-blank character
 *              p       :(char *) pointer to location of start of hex number
 *              ip      :(int)    hex variable read
 */
char *
gethex(p, ip)
	register char *p;
	int *ip;
{
	register int ac = 0;

	skipblank(p);
	while (*p) {
		if (*p >= '0' && *p <= '9')
			ac = (ac<<4) + (*p - '0');
		else if (*p >= 'a' && *p <= 'f')
			ac = (ac<<4) + (*p - 'a' + 10);
		else if (*p >= 'A' && *p <= 'F')
			ac = (ac<<4) + (*p - 'A' + 10);
		else
			break;
		p++;
	}
	skipblank(p);
	if (*p == ',')
		p++;
	skipblank(p);
	*ip = ac;
	return (p);
}

/*
 * Open a device or file-in-file-system-on-device.
 */
int
open(str, how)
	char	*str;
	int	how;
{
	register char *cp;
	register struct iob *file;
	register struct boottab *dp;
	register struct boottab **tablep;
	int	fdesc;
	long	atol();

	extern struct boottab *(devsw[]);

	fdesc = getiob();		/* Allocate an IOB */
	file = &iob[fdesc];
	open_sip = &file->i_si;

	for (cp = str; *cp && *cp != '('; cp++)
			;
	if (*cp++ != '(') {
		file->i_flgs = 0;
		goto badsyntax;
	}
	for (tablep = devsw; 0 != (dp = *tablep); tablep++) {
		if (str[0] == dp->b_dev[0] && str[1] == dp->b_dev[1])
			goto gotdev;
	}
	printf("Unknown device: %c%c; known devices:\n", str[0], str[1]);
	for (tablep = devsw; 0 != (dp = *tablep); tablep++)
		printf("  %s\n", dp->b_desc);
	file->i_flgs = 0;
	return(-1);
gotdev:
	file->i_boottab = dp;		/* Record pointer to boot table */
	file->i_ino.i_dev = tablep-devsw;
	cp = gethex(cp, &file->i_ctlr);
	cp = gethex(cp, &file->i_unit);
	cp = gethex(cp, (int *)&file->i_boff);
	if (*cp == '\0') goto doit;
	if (*cp++ != ')') {
badsyntax:
		printf("%s: bad syntax, try dev(ctlr,unit,part)name\n", str);
		return -1;
	}
	skipblank(cp);

doit:
	return(openfile(fdesc, file, cp, how));
}
#endif NOOPEN


/*
 * File processing for open call
 */
openfile(fdesc, file, cp, how)
	int		fdesc;
	struct iob	*file;
	char 		*cp;
	int		how;
{
	ino_t	ino;

	if (devopen(&file->i_si)) {
		file->i_flgs = 0;
		return(-1);	/* if devopen fails, open fails */
	}

	if (*cp == '\0') {	/* Opening a device */
		file->i_flgs |= how+1;
		file->i_cc = 0;
		file->i_offset = 0;
		return(fdesc+3);
	}
	/* Opening a file system; read the superblock. */
	file->i_ma = (char *)(&file->i_fs);
	file->i_cc = SBSIZE;
	file->i_bn = SBLOCK;
	file->i_offset = 0;
	if (devread(&file->i_si) != SBSIZE) {
		file->i_flgs = 0;
		return(-1);
	}
#ifndef BOOTBLOCK
	if (file->i_fs.fs_magic != FS_MAGIC) {
		printf("Not a file system\n");
		return(-1);
	}
#endif	BOOTBLOCK
	if ((ino = find(cp, file)) == 0) {
		file->i_flgs = 0;
		return(-1);
	}
#ifndef BOOTBLOCK
	if (how != 0) {
		printf("Can't write files yet.. Sorry\n");
		file->i_flgs = 0;
		return(-1);
	}
#endif	BOOTBLOCK
	openi(ino, file);
	file->i_offset = 0;
	file->i_cc = 0;
	file->i_flgs |= F_FILE | (how+1);
	return(fdesc+3);
}


close(fdesc)
	int	fdesc;
{
	register struct iob *file;

	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0)
		return(-1);
	devclose(&file->i_si);	/* For a file-in-filesys or for raw dev */
	file->i_flgs = 0;
	return(0);
}

exit()
{
	_stop((char *)0);
}

_stop(s)
	char	*s;
{
	register int i;

	for (i = 0; i < NFILES; i++)
		if (iob[i].i_flgs != 0)
			close(i);
	if (s) printf("%s\n", s);
	_exit();
}

panic(s)
	char    *s;
{
	_stop(s);
}

#endif KADB
