#ifndef lint
static	char sccsid[] = "@(#)installboot.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


/*
 * installboot.c
 *
 * Install the bootblk program in the boot block.
 * The real boot program will be read via a table
 * of entries that point to where the boot program
 * is stored in the file system.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <ufs/fs.h>
#include <ufs/inode.h>
#include <sys/stat.h>
#include <sys/dir.h>

#define NDBLKS		64
#define SECTORSIZE	512
#define SUPERADDR	(16 * SECTORSIZE)
#define BOOTBLKSIZE	((16 - 1) * SECTORSIZE)
#define MAXBSIZE	8192		/* maximum fs block size supported */

long	superblock[SBSIZE / sizeof (long)];
long	ibuf[MAXBSIZE / sizeof (long)];
char	vflg = 0;
char	lflg = 0;
#if defined(sun4c) || defined (sun4m)
char	hflg = 1;
#else
char	hflg = 0;
#endif

/*
 * entries in table to describe location of boot on disk
 * written by installboot
 */
struct blktab {
	unsigned int	b_blknum:24;	/* starting block number */
	unsigned int	b_size:8;	/* number of DEV_BSIZE byte blocks */
};

struct biblk {
	struct blktab 	bi_blks[NDBLKS];
	int		bi_size;
	int		bi_checksum;
} biblk;

/*
 * Transfers must be limited to MAXBSIZE because the bootblk drivers
 * fail with larger requests.
 */
#define MAX_B_SIZE MAXBSIZE/SECTORSIZE	/* largest value for b_size field above */

#ifdef DEBUG
char	tflg = 0;
#endif	/* DEBUG */

char *malloc();

/* VARARGS2 */
extern char *sprintf();

main(argc, argv)
	register int argc;
	register char *argv[];
{
	register int i, bbsize;
	register int bfd, pfd, dfd;	/* boot, protoboot, device */
	register char *bootfile;
	register char *protobootblk;
	register char *bootdevice;
	char bb[BOOTBLKSIZE + roundup(sizeof(struct exec),SECTORSIZE)];
	off_t	seekoff;

	if (argc < 4 || argc >5) {
#ifdef DEBUG
		(void) fprintf(stderr,
		"Usage: %s [-vlht] bootfile protobootblk bootdevice\n", *argv);
#else
		(void) fprintf(stderr,
		"Usage: %s [-vlh] bootfile protobootblk bootdevice\n", *argv);
#endif	/* DEBUG */
		exit(1);
	}

	/* check arguments */
	if (argv[1][0] == '-') {
		for (i = 1; argv[1][i]; i++) {
			switch (argv[1][i]) {
#ifdef DEBUG
			case 't':		/* testing */
				tflg++;
				lflg++;		/* fall through */
#endif	/* DEBUG */
			case 'v':
				vflg++;		/* be verbose */
				break;
			case 'l':
				lflg++;
				break;		/* list blocks */
			case 'h':
				hflg++;
				break;		/* don't strip header */
			default:
				break;
			}
		}
		bootfile = argv[2];
		protobootblk = argv[3];
		bootdevice = argv[4];
	} else {
		bootfile = argv[1];
		protobootblk = argv[2];
		bootdevice = argv[3];
	}

	if (geteuid() != 0) {
		(void) fprintf(stderr, "%s: Not super user\n", *argv);
		exit(1);
	}

	/*
	 * make sure the booter is on the disk for getblks()
	 * and that the boot program is marked 0400 and owned by root
	 */
	if ((bfd = open(bootfile, O_RDWR)) == -1) {
		perror("bootfile open");
		exit(1);
	}
	if ((fsync(bfd) != 0) ||		/* put it on disk */
	    (chown(bootfile, 0, 3) != 0) ||	/* chown root bootfile */
	    (chmod(bootfile, 0444) != 0))	/* chmod 444 bootfile */
		perror("bootfile sync");

	if (vflg) {
		(void) printf("Primary boot: %s\n", protobootblk);
		(void) printf("Secondary boot: %s\n", bootfile);
		(void) printf("Boot device: %s\n", bootdevice);
	}

#ifdef DEBUG
	if (vflg) {
		(void) printf("Boot: bzero(0x%x, 0x%x)\n",
			&biblk, sizeof (biblk));
	}
#endif	/* DEBUG */

	bzero((char *)&biblk, sizeof (biblk));

	getblks(bootfile, bootdevice);

	if (lflg) {
		(void) printf("Block locations:\n");
		(void) printf("startblk size\n");
		for (i = 0;
		     biblk.bi_blks[i].b_size && biblk.bi_blks[i].b_blknum;
		     i++) {
			(void) printf("%x\t   %x\n",
			    biblk.bi_blks[i].b_blknum, biblk.bi_blks[i].b_size);
		}
	}

	if ((pfd = open(protobootblk, O_RDONLY)) == -1) {
		perror("open");
		exit(1);
	}
	bbsize = tdsize(pfd) + (hflg ? sizeof(struct exec) : 0);
#ifdef DEBUG
	if (tflg) {
		(void) printf("Bootblk size (computed): 0x%x\n", bbsize);
	}
#endif 	/* DEBUG */
	if (bbsize > BOOTBLKSIZE) {
		(void) printf("%s > 0x%x (0x%x), will not fit in boot block\n",
			protobootblk, BOOTBLKSIZE, bbsize);
		exit(1);
	}

	/*
	 * Read in the prototype bootblk.   Note that we are
	 * assuming that it still has a header. If hflg is false
	 * (the default), skip over the header so that we write out
	 * a bootblock to the device with no header.
	 */
	seekoff = (off_t)(hflg ? 0 : sizeof(struct exec));
	if (lseek(pfd, seekoff, 0) == (off_t)-1) {
		perror("seek protobootblk");
		exit(1);
	}
	if (read(pfd, bb, bbsize) != bbsize) {
		perror("read protobootblk");
		exit(1);
	}
	if (close(pfd) != 0)
		perror("close protobootblk");

#ifdef DEBUG
	if (tflg) {
		(void) printf("Bootblk size (read): 0x%x\n", bbsize);
	}
#endif 	/* DEBUG */
	if (hflg)
		(void) printf("Bootblock will contain a.out header\n");

	/*
	 * calculate and write a checksum of the real boot program
	 * in the bootblk to assure that the blocks that we read in
	 * belong to the right boot program
	 */
	chksum(bfd);
	if (close(bfd) != 0)
		perror("bootfile close");

	if (vflg) {
		(void) printf("Boot size: 0x%x\n", biblk.bi_size);
		(void) printf("Boot checksum: 0x%x\n", biblk.bi_checksum);
	}

	/*
	 * save the information about the blocks of the real boot
	 * into the boot block
	 */
#ifdef DEBUG
	if (vflg) {
		(void) printf("Boot: bcopy(0x%x, 0x%x, 0x%x)\n", &biblk,
		    bb + sizeof (int) + (hflg ? sizeof (struct exec) : 0),
		    sizeof (biblk));
	}
#endif	/* DEBUG */

	bcopy((char *)&biblk,
	    bb + sizeof (int) + (hflg ? sizeof (struct exec) : 0),
	    sizeof (biblk));

#ifdef DEBUG
	if (vflg)
		(void) printf("Block information written to bootblock\n");
	if (tflg)		/* if testing don't write anything */
		 exit(0);
#endif 	/* DEBUG */

	/*
	 * open bootdevice file
	 */
	if ((dfd = open(bootdevice, O_RDWR)) == -1) {
		perror("open");
		exit(1);
	}

#ifdef DEBUG
	if (vflg)
		(void) printf("Boot device opened\n");
#endif	/* DEBUG */

	if (lseek(dfd, (off_t) SECTORSIZE, 0) == (off_t)-1) {	/* skip label */
		perror("seek bootdevice");
		exit(1);
	}

	/*
	 * write out the boot block
	 */
#ifdef DEBUG
	if (vflg)
	    (void) printf("Boot: write(0x%x, 0x%x, 0x%x)\n", dfd, bb,
		roundup(bbsize, SECTORSIZE));
#endif	/* DEBUG */
	if ((i = write(dfd, bb, roundup(bbsize, SECTORSIZE))) == -1) {
		perror("write of boot block failed");
		exit(1);
	}
	if (close(dfd) != 0)
		perror("close bootdevice");

#ifdef DEBUG
	if (vflg)
		(void) printf("Bootblock (0x%x bytes) written to device\n", i);
#endif	/* DEBUG */

	(void) printf("Boot block installed\n");
	exit(0);
	/* NOTREACHED */
}

getblks(file, device)
	char *file;
	char *device;
{
	register struct	fs *fs;
	register struct dinode *dinode;
	int	devfd;
	int	block;
	int	offset;
	int	seekaddr;
	u_int	*blk;
	register int	size;
	register int	i, j;
	register int	bt;
	register int	spf;
	register int	fpb;
	register int	spb;
	struct  stat    statb;
	struct	stat	statrd;
	struct	stat	statd;
	int	inode;
	int	indirect;
	char	buf[64];

	/*
	 * check booter
	 */
	if (stat(file, &statb) == -1) {
		(void) fprintf(stderr, "File %s not found\n", file);
		exit(1);
	}

	/* check for zero-length file */
	if (statb.st_size <= 0) {
		(void) fprintf(stderr,
	"File %s is zero-length, no bootblocks installed\n", file);
		exit(1);
	}

	inode = statb.st_ino;

	/*
	 * check bootdevice
	 */
	if (stat(device, &statrd) == -1) {
		(void) fprintf(stderr,
			"bootdevice %s doesn't make sense (couldn't stat)\n",
			device);
		exit(1);
	}

	/*
	 * check that the device we are going to write is a raw device
	 * if it is not raw, give up.
	 *
	 * block devices are not kept consistent with control information
	 * (ie. incore inodes) if you create a file, the block containing
	 * the inode will not be updated with the new inode information
	 */
	if ((statrd.st_mode & S_IFMT) != S_IFCHR) {
		(void) fprintf(stderr, "%s not raw device\n", device);
		exit(1);
	}

	/*
	 * look for /dev/r____, build name of block device
	 * used for booting, check it
	 */
	if (strncmp(device, "/dev/r", sizeof "/dev/r" - 1) != 0) {
		(void) fprintf(stderr,
"unexpected bootdevice %s; name must be of the form /dev/r____\n", device);
		exit(1);
	}
	(void)sprintf(buf, "/dev/%s", device + sizeof "/dev/r" - 1);
	if (stat(buf, &statd) == -1 || (statd.st_mode & S_IFMT) != S_IFBLK) {
		(void) fprintf(stderr,
			"couldn't stat block bootdevice %s\n", buf);
		exit(1);
	}

	/*
	 * if the file's device (st_dev) == the device's device (st_rdev)
	 * the file is on the device
	 */
	if (statb.st_dev != statd.st_rdev) {
		(void) fprintf(stderr,
			 "bootfile %s not on bootdevice %s\n", file, device);
		exit(1);
	}

	/*
	 * read in and check the superblock
	 */
	if ((devfd = open(device, O_RDONLY)) == -1) {
		perror("Can't open raw device");
		exit(1);
	}
	if (lseek(devfd, (off_t)SUPERADDR, 0) == (off_t)-1) {
		perror("Seek to superblock failed");
		exit(1);
	}
	if (read(devfd, (char *)superblock,
		roundup(sizeof (struct fs), SECTORSIZE)) == -1) {
		perror("Read of superblock failed");
		exit(1);
	}

	fs = (struct fs *)superblock;
	if (fs->fs_magic != FS_MAGIC) {
		(void) fprintf(stderr, "Not a superblock\n");
		exit(1);
	}
	block = itod(fs, inode);
	offset = itoo(fs, inode);
	seekaddr = fsbtodb(fs, block) * SECTORSIZE;

	/*
	 * find the block containing the inode for the bootfile
	 */
	if (lseek(devfd, (off_t) seekaddr, 0) == (off_t)-1) {
		(void) fprintf(stderr, "Seek to block %d offset %d failed\n",
			block, offset);
		exit(1);
	}
	if (read(devfd, (char *)ibuf, (int)fs->fs_bsize) == -1) {
		(void) fprintf(stderr, "Read of inode %d failed\n", inode);
		exit(1);
	}
	dinode = (struct dinode *)&ibuf
			[offset * sizeof (struct dinode) / sizeof (long)];

	spf = fs->fs_fsize / SECTORSIZE;	/* sectors per frag */
	fpb = fs->fs_bsize / fs->fs_fsize;	/* frags per block */
	spb = fpb * spf;			/* sectors per block */
	size = dinode->di_size;

	/* get first level blocks pointers */
	bt = 0;
	for (i = 0; (i < NDADDR) && (size > 0); i++) {
		u_int	temp_bsize;

		/*
		 * try to coalesce sequential blocks
		 * into a blktab single entry
		 */
		if (  (bt > 0)
		   && ((biblk.bi_blks[bt-1].b_blknum +
		        biblk.bi_blks[bt-1].b_size)
		        == fsbtodb(fs, dinode->di_db[i]))
		   && ((biblk.bi_blks[bt-1].b_size + spb) < MAX_B_SIZE)
		     ) { 
			temp_bsize = biblk.bi_blks[bt-1].b_size;
			biblk.bi_blks[bt-1].b_size = temp_bsize + spb;
		} else {
			if (bt+1 >= NDBLKS) {
				(void)fprintf(stderr,
			"The boot program has too many discontinuous blocks\n");
				exit(1);
			}
			biblk.bi_blks[bt].b_blknum =
				fsbtodb(fs, dinode->di_db[i]);
			biblk.bi_blks[bt].b_size = (spb & 0xff);
			bt++;
		}
		size -= fs->fs_bsize;
	}

	blk = (u_int *)malloc((unsigned) fs->fs_bsize);

	/* now go for indirect blocks */
	for (i = 0; (i < NIADDR) && (size > 0); i++) {
		indirect = dinode->di_ib[i];
		seekaddr = fsbtodb(fs, indirect) * SECTORSIZE;
		if (lseek(devfd, (off_t) seekaddr, 0) == (off_t)-1) {
			(void) fprintf(stderr,
				"Seek to indirect block %d failed\n", indirect);
			exit(1);
		}
		if (read(devfd, (char *)blk, (int) fs->fs_bsize) == -1) {
			(void) fprintf(stderr,
				"Read of indirect block %d failed\n", indirect);
			exit(1);
		}
		/* save block numbers from indirect blocks */
		for (j = 0;
		     (j < fs->fs_bsize / sizeof (int)) && (size > 0);
		     j++, size -= fs->fs_bsize) {
			u_int	temp_bsize;

			if (  (bt > 0)
			   && ((biblk.bi_blks[bt-1].b_blknum +
				biblk.bi_blks[bt-1].b_size)
				== fsbtodb(fs, blk[j]))
			   && ((biblk.bi_blks[bt-1].b_size + spb) < MAX_B_SIZE)
			     ) { 
				temp_bsize = biblk.bi_blks[bt-1].b_size;
				biblk.bi_blks[bt-1].b_size = temp_bsize + spb;
			} else {
				if (bt+1 >= NDBLKS) {
					(void)fprintf(stderr,
			"The boot program has too many discontinuous blocks\n");
					exit(1);
				}
				biblk.bi_blks[bt].b_blknum =
				    fsbtodb(fs, blk[j]);
				biblk.bi_blks[bt].b_size = (spb & 0xff);
				bt++;
			}
		}
	}
}

/*
 * read the header of the file and return
 * the sum of its text and data segments
 */
tdsize(bootfd)
	register int bootfd;
{
	long buf[((sizeof (struct exec) * 2) + 4) / sizeof (long)];
	register struct exec *e;

	if (read(bootfd, (char *)buf, sizeof (struct exec))== -1) {
		(void) printf("trouble reading header\n");
	}
	e = (struct exec *)buf;

#ifdef DEBUG
	if (vflg) {
		(void) printf("tdsize: text 0x%x data 0x%x\n",
			e->a_text, e->a_data);
	}
#endif	/* DEBUG */

	return (e->a_text + e->a_data);
}


chksum(fd)
	register int fd;
{
	register unsigned int sum;
	register int *p;
	register int x;
	register int size;
	static int buf[DEV_BSIZE / sizeof (int)];

	/*
	 * The boot program loaded by the bootblk should be stripped.
	 * We used to enforce that it be headerless as well, but we
	 * don't any more; that's the user's problem.
	 */
	if (lseek(fd, (off_t) 0, 0) != (off_t)0)
		perror("seek bootfile");

	sum = 0;
	size = 0;
	bzero ((char *) buf, sizeof buf);

	while (x = read(fd, (char *) buf, sizeof buf)) {
		size += x;
		for (p = buf; x > 0; x -= 4) {
			 sum += *p++;
		}
		bzero ((char *) buf, sizeof buf);
	}

	biblk.bi_size = size;
	biblk.bi_checksum = sum;
}

