#ifndef lint
static	char sccsid[] = "@(#)df.c 1.1 92/07/30 SMI"; /* from UCB 4.18 84/02/02 */
#endif
/*
 * df
 */
#include <sys/param.h>
#include <errno.h>
#include <ufs/fs.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <stdio.h>
#include <mntent.h>

void	usage(), pheader();
char	*mpath();
int	tflag;
int	aflag;
char	*typestr;

union {
	struct fs iu_fs;
	char dummy[SBSIZE];
} sb;
#define sblock sb.iu_fs

/*
 * This structure is used to build a list of mntent structures
 * in reverse order from /etc/mtab.
 */
struct mntlist {
	struct mntent *mntl_mnt;
	struct mntlist *mntl_next;
};

struct mntlist *mkmntlist();
struct mntent *mntdup();

int
main(argc, argv)
	int argc;
	char **argv;
{
	register struct mntent *mnt;

	/*
	 * Skip over command name, if present.
	 */
	if (argc > 0) {
		argv++;
		argc--;
	}

	while (argc > 0 && (*argv)[0]=='-') {
		switch ((*argv)[1]) {

		case 't':
			tflag++;
			break;
		default:
			usage();
		}
		argc--, argv++;
	}
	sync();
	if (argc <= 0) {
		register FILE *mtabp;

		if ((mtabp = setmntent(MOUNTED, "r")) == NULL) {
			(void) fprintf(stderr, "df: ");
			perror(MOUNTED);
			exit(1);
		}
		pheader();
		while ((mnt = getmntent(mtabp)) != NULL) {
			if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
			    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0)
				continue;
			dfreemnt(mnt->mnt_dir, mnt);
		}
		(void) endmntent(mtabp);
	} else {
		int num = argc ;
		int i ;
		struct mntlist *mntl ;

		pheader();
		aflag++;
		/*
		 * Reverse the list and start comparing.
		 */
		for (mntl = mkmntlist(); mntl != NULL && num ; 
				mntl = mntl->mntl_next) {
		   struct stat dirstat, filestat ;

		   mnt = mntl->mntl_mnt ;
		   if (stat(mnt->mnt_dir, &dirstat)<0) {
			continue ;
		   }
		   for (i = 0; i < argc; i++) {
			if (argv[i]==NULL) continue ;
			if (stat(argv[i], &filestat) < 0) {
				(void) fprintf(stderr, "df: ");
				perror(argv[i]);
				argv[i] = NULL ;
				--num;
			} else {
			       if ((filestat.st_mode & S_IFMT) == S_IFBLK ||
			          (filestat.st_mode & S_IFMT) == S_IFCHR) {
					char *cp ;

					cp = mpath(argv[i]);
					if (*cp == '\0') {
						dfreedev(argv[i]);
						argv[i] = NULL ;
						--num;
						continue;
					}
					else {
					  if (stat(cp, &filestat) < 0) {
						(void) fprintf(stderr, "df: ");
						perror(argv[i]);
						argv[i] = NULL ;
						--num;
						continue ;
					  }
					}
				}
				if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
				    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0)
					continue;
				if ((filestat.st_dev == dirstat.st_dev)) {
					dfreemnt(mnt->mnt_dir, mnt);
					argv[i] = NULL ;
					--num ;
				}
			}
		}
	     }
	     if (num) {
		     for (i = 0; i < argc; i++) 
			if (argv[i]==NULL) 
				continue ;
			else
			   (void) fprintf(stderr, 
				"Could not find mount point for %s\n", argv[i]) ;
	     }
	}
	exit(0);
	/*NOTREACHED*/
}

void
pheader()
{
	(void) printf("Filesystem            bavail  iavail");
	if (tflag)
		(void) printf("  btotal  itotal");
	(void) printf("  Mounted on\n");
}

/*
 * Report on a block or character special device.  Assumed not to be 
 * mounted.  N.B. checks for a valid 4.2BSD superblock.  
 */
dfreedev(file)
	char *file;
{
	long availblks, avail, free, used;
	int fi;

	fi = open(file, 0);
	if (fi < 0) {
		(void) fprintf(stderr, "df: ");
		perror(file);
		return;
	}
	if (bread(file, fi, SBLOCK, (char *)&sblock, SBSIZE) == 0) {
		(void) close(fi);
		return;
	}
	if (sblock.fs_magic != FS_MAGIC) {
		(void) fprintf(stderr, "df: %s: not a UNIX filesystem\n", 
		    file);
		(void) close(fi);
		return;
	}
	(void) printf("%-20.20s", file);

	free = sblock.fs_cstotal.cs_nbfree * sblock.fs_frag +
	    sblock.fs_cstotal.cs_nffree;
	availblks = sblock.fs_dsize * (100 - sblock.fs_minfree) / 100;
	used = sblock.fs_dsize - free;
	avail = availblks > (sblock.fs_dsize - free) ? availblks - used : 0;

	(void) printf("%8d%8ld", avail * sblock.fs_fsize / 512, 
			sblock.fs_cstotal.cs_nifree);
	if (tflag) {
		(void) printf("%8d%8ld", sblock.fs_dsize * sblock.fs_fsize / 512,
			sblock.fs_ncg * sblock.fs_ipg);
	}
	(void) printf("  %s\n", mpath(file));
	(void) close(fi);
}

dfreemnt(file, mnt)
	char *file;
	struct mntent *mnt;
{
	struct statfs fs;

	if (statfs(file, &fs) < 0) {
		(void) fprintf(stderr, "df: ");
		perror(file);
		return;
	}

	if (!aflag && fs.f_blocks == 0) {
		return;
	}
	if (strlen(mnt->mnt_fsname) > 20) {
		(void) printf("%s\n", mnt->mnt_fsname);
		(void) printf("                    ");
	} else {
		(void) printf("%-20.20s", mnt->mnt_fsname);
	}
	(void) printf("%8d%8ld", fs.f_bavail* fs.f_bsize / 512, fs.f_ffree);
	if (tflag) {
		(void) printf("%8d%8ld", fs.f_blocks * fs.f_bsize / 512,
							fs.f_files);
	}
	(void) printf("  %s\n", mnt->mnt_dir);
}

/*
 * Given a name like /dev/rrp0h, returns the mounted path, like /usr.
 */
char *
mpath(file)
	char *file;
{
	FILE *mntp;
	register struct mntent *mnt;

	if ((mntp = setmntent(MOUNTED, "r")) == 0) {
		(void) fprintf(stderr, "df: ");
		perror(MOUNTED);
		exit(1);
	}

	while ((mnt = getmntent(mntp)) != 0) {
		if (strcmp(file, mnt->mnt_fsname) == 0) {
			(void) endmntent(mntp);
			return (mnt->mnt_dir);
		}
	}
	(void) endmntent(mntp);
	return "";
}

long lseek();

int
bread(file, fi, bno, buf, cnt)
	char *file;
	int fi;
	daddr_t bno;
	char *buf;
	int cnt;
{
	register int n;
	extern int errno;

	(void) lseek(fi, (long)(bno * DEV_BSIZE), 0);
	if ((n = read(fi, buf, (unsigned) cnt)) < 0) {
		/* probably a dismounted disk if errno == EIO */
		if (errno != EIO) {
			(void) fprintf(stderr, "df: read error on ");
			perror(file);
			(void) fprintf(stderr, "bno = %ld\n", bno);
		} else {
			(void) fprintf(stderr, "df: premature EOF on %s\n",
			    file);
			(void) fprintf(stderr, 
			   "bno = %ld expected = %d count = %d\n", bno, cnt, n);
		}
		return (0);
	}
	return (1);
}

char *
xmalloc(size)
	unsigned int size;
{
	register char *ret;
	char *malloc();
	
	if ((ret = (char *)malloc(size)) == NULL) {
		(void) fprintf(stderr, "umount: ran out of memory!\n");
		exit(1);
	}
	return (ret);
}

struct mntent *
mntdup(mnt)
	register struct mntent *mnt;
{
	register struct mntent *new;

	new = (struct mntent *)xmalloc(sizeof(*new));

	new->mnt_fsname = (char *)xmalloc(strlen(mnt->mnt_fsname) + 1);
	(void) strcpy(new->mnt_fsname, mnt->mnt_fsname);

	new->mnt_dir = (char *)xmalloc(strlen(mnt->mnt_dir) + 1);
	(void) strcpy(new->mnt_dir, mnt->mnt_dir);

	new->mnt_type = (char *)xmalloc(strlen(mnt->mnt_type) + 1);
	(void) strcpy(new->mnt_type, mnt->mnt_type);

	new->mnt_opts = (char *)xmalloc(strlen(mnt->mnt_opts) + 1);
	(void) strcpy(new->mnt_opts, mnt->mnt_opts);

	new->mnt_freq = mnt->mnt_freq;
	new->mnt_passno = mnt->mnt_passno;

	return (new);
}

void
usage()
{

	(void) fprintf(stderr, "usage: df [ -t ] [ file... ]\n");
	exit(0);
}

struct mntlist *
mkmntlist()
{
	FILE *mounted;
	struct mntlist *mntl;
	struct mntlist *mntst = NULL;
	struct mntent *mnt;

	if ((mounted = setmntent(MOUNTED, "r"))== NULL) {
		(void) fprintf(stderr, "df : ") ;
		perror(MOUNTED);
		exit(1);
	}
	while ((mnt = getmntent(mounted)) != NULL) {
		mntl = (struct mntlist *)xmalloc(sizeof(*mntl));
		mntl->mntl_mnt = mntdup(mnt);
		mntl->mntl_next = mntst;
		mntst = mntl;
	}
	(void) endmntent(mounted);
	return(mntst);
}
