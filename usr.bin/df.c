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
#include <sys/file.h>

#include <stdio.h>
#include <mntent.h>
#include <string.h>

extern char	*getenv();
extern char	*getwd();
extern char	*realpath();
extern off_t	lseek();

static	void	usage(), pheader();
static	char	*mpath(), *zap_chroot();
static	char	*pathsuffix();
static	char	*xmalloc();
static	int	chroot_stat();
static 	int	bread();
static	int	iflag, aflag;
static	int	type;
static	char	*typestr;
static	dev_t	devfromopts();
static	void	dfreedev(), dfreefile();
static	int	abspath(), subpath();
static	void	show_inode_usage();

/*
 * cached information recording previous chroot history.
 */
static	char	*chrootpath;
static	int	chrootlen;

#ifndef	MNTTYPE_LO
#define	MNTTYPE_LO	"lo"
#endif

union {
	struct fs iu_fs;
	char dummy[SBSIZE];
} sb;
#define	sblock	sb.iu_fs

/*
 * This structure is used to chain mntent structures into a list
 * and to cache stat information for each member of the list.
 */
struct mntlist {
	struct mntent	*mntl_mnt;
	struct mntlist	*mntl_next;
	dev_t		mntl_dev;
	int		mntl_devvalid;
};

static struct mntlist	*mkmntlist();
static struct mntent	*mntdup();
static struct mntlist	*findmntent();
int errcode = 0;
char nullstring[] = "\0";

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

		case 'i':
			iflag++;
			break;

		case 't':
			type++;
			argv++;
			argc--;
			if (argc <= 0)
				usage();
			typestr = *argv;
			break;

		case 'a':
			aflag++;
			break;

		default:
			usage();
		}
		argc--, argv++;
	}

	if (argc > 0 && type) {
		usage();
		/* NOTREACHED */
	}

	/*
	 * Cache CHROOT information for later use; assume that $CHROOT matches
	 * the cumulative arguments given to chroot calls.
	 */
	chrootpath = getenv("CHROOT");
	if (chrootpath != NULL && strcmp(chrootpath, "/") == 0)
		chrootpath = NULL;
	if (chrootpath != NULL)
		chrootlen = strlen(chrootpath);

	/*
	 * Make sure that the information reported is up to date.
	 */
	sync();

	if (argc <= 0) {
		register FILE	*mtabp;

		if ((mtabp = setmntent(MOUNTED, "r")) == NULL) {
			(void) fprintf(stderr, "df: ");
			perror(MOUNTED);
			exit(1);
		}
		pheader();
		while ((mnt = getmntent(mtabp)) != NULL) {
			if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
			    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0 ||
			    strcmp(mnt->mnt_type, MNTTYPE_LO) == 0)
				continue;
			if (type && strcmp(typestr, mnt->mnt_type) != 0) {
				continue;
			}
			dfreefile(mnt->mnt_dir, mnt->mnt_fsname);
		}
		(void) endmntent(mtabp);
	} else {
		register int		i;
		register struct mntlist	*mntl;
		register struct stat	*argstat;

		/*
		 * Obtain stat information for each argument before
		 * constructing the list of mounted file systems.  This
		 * ordering forces the automounter to establish any
		 * mounts required to access the arguments, so that the
		 * corresponding mount table entries will exist when
		 * we look for them.
		 */
		argstat = (struct stat *)xmalloc(argc * sizeof *argstat);
		for (i = 0; i < argc; i++) {
			if (stat(argv[i], &argstat[i]) < 0) {
				(void) fprintf(stderr, "df: ");
				perror(argv[i]);
				errcode = errno;
				/*
				 * Mark as no longer interesting.
				 */
				argv[i] = NULL;
				continue;
			}
		}

		pheader();
		aflag++;
		/*
		 * Construct the list of mounted file systems.
		 */
		mntl = mkmntlist();

		/*
		 * Iterate through the argument list, reporting on each one.
		 */
		for (i = 0; i < argc; i++) {
			register char		*cp;
			register struct mntlist	*mlp;

			/*
			 * Skip if we've already determined that we can't
			 * process it.
			 */
			if (argv[i] == NULL)
				continue;

			/*
			 * If the argument names a device, report on the file
			 * system associated with the device rather than on
			 * the one containing the device's directory entry.
			 */
			if ((argstat[i].st_mode & S_IFMT) == S_IFBLK ||
			    (argstat[i].st_mode & S_IFMT) == S_IFCHR) {
				dfreedev(argv[i]);
				continue;
			} else
				cp = argv[i];

			/*
			 * Get this argument's corresponding mount table
			 * entry.
			 */
			mlp = findmntent(cp, &argstat[i], mntl);

			if (mlp == NULL) {
				(void) fprintf(stderr,
				    "Could not find mount point for %s\n",
				    argv[i]);
				continue;
			}

			dfreefile(mlp->mntl_mnt->mnt_dir,
			    mlp->mntl_mnt->mnt_fsname);
		}
	}
	exit(errcode);
	/* NOTREACHED */
}

static void
pheader()
{
	(void) printf("Filesystem            ");
	if (iflag)
		(void) printf(" iused   ifree  %%iused");
	else
		(void) printf("kbytes    used   avail capacity");
	(void) printf("  Mounted on\n");
}

/*
 * Report on a block or character special device.
 * N.B. checks for a valid 4.2BSD superblock.
 */
static void
dfreedev(file)
	char *file;
{
	long totalblks, availblks, avail, free, used;
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
	if (iflag) {
		show_inode_usage(sblock.fs_ncg * sblock.fs_ipg,
		    sblock.fs_cstotal.cs_nifree);
	} else {
		totalblks = sblock.fs_dsize;
		free = sblock.fs_cstotal.cs_nbfree * sblock.fs_frag +
		    sblock.fs_cstotal.cs_nffree;
		used = totalblks - free;
		availblks = totalblks * (100 - sblock.fs_minfree) / 100;
		avail = availblks > used ? availblks - used : 0;
		(void) printf("%8d%8d%8d",
		    totalblks * sblock.fs_fsize / 1024,
		    used * sblock.fs_fsize / 1024,
		    avail * sblock.fs_fsize / 1024);
		(void) printf("%6.0f%%",
		    availblks==0? 0.0: (double)used/(double)availblks * 100.0);
		(void) printf("  ");
	}
	(void) printf("  %s\n", mpath(file));
	(void) close(fi);
}

/*
 * Report on a file/directory on a particular filesystem.
 */
static void
dfreefile(file, fsname)
	char *file;
	char *fsname;
{
	struct statfs fs;

	if (statfs(file, &fs) < 0 &&
	    chroot_stat(file, statfs, (char *)&fs, &file) < 0) {
		perror(file);
		return;
	}

	if (!aflag && fs.f_blocks == 0) {
		return;
	}
	if (strlen(fsname) > 20) {
		(void) printf("%s\n", fsname);
		(void) printf("                    ");
	} else {
		(void) printf("%-20.20s", fsname);
	}
	if (iflag) {
		show_inode_usage(fs.f_files, fs.f_ffree);
	} else {
		long totalblks, avail, free, used, reserved;

		totalblks = fs.f_blocks;
		free = fs.f_bfree;
		used = totalblks - free;
		avail = fs.f_bavail;
		reserved = free - avail;
		if (avail < 0)
			avail = 0;
		(void) printf("%8d%8d%8d", totalblks * fs.f_bsize / 1024,
		    used * fs.f_bsize / 1024, avail * fs.f_bsize / 1024);
		totalblks -= reserved;
		(void) printf("%6.0f%%",
		    totalblks==0? 0.0: (double)used/(double)totalblks * 100.0);
		(void) printf("  ");
	}
	(void) printf("  %s\n", file);
}

static void
show_inode_usage(total, free)
	long total, free;
{
	long used = total - free;
	int missing_info = (total == -1 || free == -1);

	if (missing_info)
		(void)printf("%8s", "*");
	else
		(void)printf("%8ld", used);
	if (free == -1)
		(void)printf("%8s", "*");
	else
		(void)printf("%8ld", free);
	if (missing_info)
		(void)printf("%6s  ", "*");
	else
		(void)printf("%6.0f%% ", (double)used / (double)total * 100.0);
}

/*
 * Return the suffix of path obtained by stripping off the prefix
 * that is the value of the CHROOT environment variable.  If this
 * value isn't obtainable or if it's not a prefix of path, return NULL.
 */
static char *
zap_chroot(path)
	char	*path;
{
	return (pathsuffix(path, chrootpath));
}

/*
 * Stat/statfs a file after stripping off leading directory to which we are
 * chroot'd.  Used to find the TFS mount that applies to the current
 * activated NSE environment.
 */
static int
chroot_stat(dir, statfunc, statp, dirp)
	char *dir;
	int (*statfunc)();
	char *statp;
	char **dirp;
{
	if ((dir = zap_chroot(dir)) == NULL)
		return (-1);
	if (dirp)
		*dirp = dir;
	return (*statfunc)(dir, statp);
}

/*
 * Given a name like /dev/rrp0h, returns the mounted path, like /usr.
 */
static char *
mpath(file)
	char *file;
{
	FILE *mntp;
	register struct mntent *mnt;
	struct stat		argstat;
	char			devname[MAXPATHLEN];
	register char		*cp;

	(void) strcpy(devname, file);
	if (stat(devname, &argstat) != 0) {
		(void) fprintf(stderr, "df: ");
		perror(devname);
		return (nullstring);
	}
	/*
	 * If this is a raw device, we have to build the block device name.
	 * N.B.: This assumes device names of the form [foo]r[bar]
	 * for a raw device, and [foo][bar] for the corresponding block
	 * device.
	 */
	if ((argstat.st_mode & S_IFMT) == S_IFCHR) {
		cp = strchr(devname, 'r');
		do {
			*cp = *(cp + 1);
		} while (*(++cp));
		if (stat(devname, &argstat) != 0) {
			(void) fprintf(stderr, "df: ");
			perror(devname);
			return (nullstring);
		}
	}
	if ((mntp = setmntent(MOUNTED, "r")) == 0) {
		(void) fprintf(stderr, "df: ");
		perror(MOUNTED);
		exit(1);
	}

	while ((mnt = getmntent(mntp)) != 0) {
		if (strcmp(devname, mnt->mnt_fsname) == 0) {
			(void) endmntent(mntp);
			return (mnt->mnt_dir);
		}
	}
	(void) endmntent(mntp);
	return (nullstring);
}

static int
bread(file, fi, bno, buf, cnt)
	char *file;
	int fi;
	daddr_t bno;
	char *buf;
	int cnt;
{
	register int n;
	extern int errno;

	(void) lseek(fi, (off_t)(bno * DEV_BSIZE), L_SET);
	if ((n = read(fi, buf, cnt)) < 0) {
		/* probably a dismounted disk if errno == EIO */
		if (errno != EIO) {
			(void) fprintf(stderr, "df: read error on ");
			perror(file);
			(void) fprintf(stderr, "bno = %ld\n", bno);
		} else {
			(void) fprintf(stderr, "df: premature EOF on %s\n",
			    file);
			(void) fprintf(stderr,
			    "bno = %ld expected = %d count = %d\n",
			    bno, cnt, n);
		}
		return (0);
	}
	return (1);
}

static char *
xmalloc(size)
	int size;
{
	register char *ret;
	char *malloc();

	if ((ret = malloc((unsigned)size)) == NULL) {
		perror("df");
		exit(1);
	}
	bzero(ret, size);
	return (ret);
}

static struct mntent *
mntdup(mnt)
	register struct mntent *mnt;
{
	register struct mntent *new;

	new = (struct mntent *)xmalloc(sizeof *new);

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

static void
usage()
{

	(void) fprintf(stderr,
	    "usage: df [ -i ] [ -a ] [ -t type | file... ]\n");
	exit(1);
}

static struct mntlist *
mkmntlist()
{
	FILE *mounted;
	struct mntlist *mntl;
	struct mntlist *mntst = NULL;
	struct mntent *mnt;

	if ((mounted = setmntent(MOUNTED, "r"))== NULL) {
		(void) fprintf(stderr, "df : ");
		perror(MOUNTED);
		exit(1);
	}
	while ((mnt = getmntent(mounted)) != NULL) {
		mntl = (struct mntlist *)xmalloc(sizeof *mntl);
		mntl->mntl_mnt = mntdup(mnt);
		mntl->mntl_next = mntst;
		mntst = mntl;
	}
	(void) endmntent(mounted);
	return (mntst);
}

/*
 * Convert the path given in raw to canonical, absolute, symlink-free
 * form, storing the result in the buffer named by canon, which must be
 * at least MAXPATHLEN bytes long.  If wd is non-NULL, assume that it
 * points to a path for the current working directory and use it instead
 * of invoking getwd; accepting this value as an argument lets our caller
 * cache the value, so that realpath (called from this routine) doesn't have
 * to recalculate it each time it's given a relative pathname.
 *
 * Return 0 on success, -1 on failure.
 */
static int
abspath(wd, raw, canon)
	char		*wd;
	register char	*raw;
	char		*canon;
{
	char	absbuf[MAXPATHLEN];

	/*
	 * Preliminary sanity check.
	 */
	if (raw == NULL || canon == NULL)
		return (-1);

	/*
	 * If the path is relative, convert it to absolute form,
	 * using wd if it's been supplied.
	 */
	if (raw[0] != '/') {
		register char	*limit = absbuf + sizeof absbuf;
		register char	*d;

		/* Fill in working directory. */
		if (wd != NULL)
			(void) strncpy(absbuf, wd, sizeof absbuf);
		else if (getwd(absbuf) == NULL)
			return (-1);

		/* Add separating slash. */
		d = absbuf + strlen(absbuf);
		if (d < limit)
			*d++ = '/';

		/* Glue on the relative part of the path. */
		while (d < limit && (*d++ = *raw++))
			continue;

		raw = absbuf;
	}

	/*
	 * Call realpath to canonicalize and resolve symlinks.
	 */
	return (realpath(raw, canon) == NULL ? -1 : 0);
}

/*
 * Return a pointer to the trailing suffix of full that follows the prefix
 * given by pref.  If pref isn't a prefix of full, return NULL.  Apply
 * pathname semantics to the prefix test, so that pref must match at a
 * component boundary.
 */
static char *
pathsuffix(full, pref)
	register char	*full;
	register char	*pref;
{
	register int	preflen;

	if (full == NULL || pref == NULL)
		return (NULL);

	preflen = strlen(pref);
	if (strncmp(pref, full, preflen) != 0)
		return (NULL);

	/*
	 * pref is a substring of full.  To be a subpath, it cannot cover a
	 * partial component of full.  The last clause of the test handles the
	 * special case of the root.
	 */
	if (full[preflen] != '\0' && full[preflen] != '/' && preflen > 1)
		return (NULL);

	if (preflen == 1 && full[0] == '/')
		return (full);
	else
		return (full + preflen);
}

/*
 * Return zero iff the path named by sub is a leading subpath
 * of the path named by full.
 *
 * Treat null paths as matching nothing.
 */
static int
subpath(full, sub)
	register char	*full;
	register char	*sub;
{
	return (pathsuffix(full, sub) == NULL);
}

/*
 * Find the entry in mlist that corresponds to the file named by path
 * (i.e., that names a mount table entry for the file system in which
 * path lies).  The pstat argument must point to stat information for
 * path.
 *
 * Ignore list entries whose type is MNTTYPE_SWAP.  Do not ignore
 * MNTTYPE_IGNORE entries, as they may contain information about TFS mounts
 * that must be examined in order to find the correct file system for files
 * inside an activated NSE environment.  Handle MNTTYPE_LO entries as
 * glorified symlinks (see below).
 *
 * Return the entry or NULL if there's no match.
 *
 * As it becomes necessary to obtain stat information about previously
 * unexamined mlist entries, gather the information and cache it with the
 * entries.
 *
 * The routine's strategy is to convert path into its canonical, symlink-free
 * representation canon (which will require accessing the file systems on the
 * branch from the root to path and thus may cause the routine to hang if any
 * of them are inaccessible) and to use it to search for a mount point whose
 * name is a substring of canon and whose corresponding device matches that of
 * canon.  This technique avoids accessing unnecessary file system resources
 * and thus prevents the program from hanging on inaccessible resources unless
 * those resources are necessary for accessing path.
 */
static struct mntlist *
findmntent(path, pstat, mlist)
	char		*path;
	struct stat	*pstat;
	struct mntlist	*mlist;
{
	static char		cwd[MAXPATHLEN];
	char			canon[MAXPATHLEN];
	char			scratch[MAXPATHLEN];
	register struct mntlist	*mlp;

	/*
	 * If path is relative and we haven't already determined the current
	 * working directory, do so now.  Calculating the working directory
	 * here lets us do the work once, instead of (potentially) repeatedly
	 * in realpath().
	 */
	if (*path != '/' && cwd[0] == '\0') {
		if (getwd(cwd) == NULL) {
			cwd[0] = '\0';
			return (NULL);
		}
	}

	/*
	 * Find an absolute pathname in the native file system name space that
	 * corresponds to path, stuffing it into canon.
	 *
	 * If CHROOT is set in the environment, assume that chroot($CHROOT)
	 * (or an equivalent series of calls) was executed and convert the
	 * path to the equivalent name in the native file system's name space.
	 * Doing so allows direct comparison with the names in mtab entires,
	 * which are assumed to be recorded relative to the native name space.
	 */
	if (abspath(cwd, path, scratch) < 0)
		return (NULL);
	if (strcmp(scratch, "/") == 0 && chrootpath != NULL) {
		/*
		 * Force canon to be in canonical form; if the result from
		 * abspath was "/" and chrootpath isn't the null string, we
		 * must strip off a trailing slash.
		 */
		scratch[0] = '\0';
	}
	(void) sprintf(canon, "%s%s", chrootpath ? chrootpath : "", scratch);

again:
	for (mlp = mlist; mlp; mlp = mlp->mntl_next) {
		register struct mntent	*mnt = mlp->mntl_mnt;

		/*
		 * Ignore uninteresting mounts (see comments above).
		 */
		if (strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0)
			continue;

		if (subpath(canon, mnt->mnt_dir) != 0)
			continue;

		/*
		 * The mount entry covers some prefix of the file.
		 * See whether it's the entry for the file system
		 * containing the file by comparing device ids.
		 *
		 * Use cached information if we have it.
		 */
		if (!mlp->mntl_devvalid) {
			struct stat	fs_sb;

			fs_sb.st_dev = devfromopts(mnt);
			if (fs_sb.st_dev == NODEV &&
			    stat(mnt->mnt_dir, &fs_sb) < 0 &&
			    chroot_stat(mnt->mnt_dir, stat, (char *)&fs_sb,
						(char **)NULL) < 0) {
					continue;
			}
			mlp->mntl_dev = fs_sb.st_dev;
			mlp->mntl_devvalid = 1;
		}

		/*
		 * If the file lies within a loopback mount, resolve away the
		 * mount and start over.  It's not necessary to recompute the
		 * information in pstat, because loopback mounts leave the
		 * device information intact.
		 */
		if (strcmp(mnt->mnt_type, MNTTYPE_LO) == 0) {
			/*
			 * Revise the pathname under consideration to resolve
			 * out the loopback mount.  Note that since subpath
			 * succeeded, pathsuffix will return non-NULL.  The
			 * ifs below handle special cases resulting from the
			 * root or a null string appearing as one component or
			 * the other.
			 */
			(void) strcpy(scratch,
			    pathsuffix(canon, mnt->mnt_dir));
			if (strcmp(mnt->mnt_fsname, "/") == 0)
				(void) strcpy(canon, scratch);
			else
				(void) sprintf(canon, "%s%s", mnt->mnt_fsname,
					scratch);
			if (strcmp(canon, "") == 0)
				(void) strcpy(canon, "/");
			goto again;
		}

		if (pstat->st_dev == mlp->mntl_dev)
			return (mlp);
	}

	return (NULL);
}

/*
 *  The device id for the mount should be available in
 *  the mount option string as "dev=%04x".  If it's there
 *  extract the device id and avoid having to stat.
 */
static dev_t
devfromopts(mnt)
	struct mntent *mnt;
{
	char *str, *equal;
	extern long strtol();

	str = hasmntopt(mnt, MNTINFO_DEV);
	if (str != NULL && (equal = strchr(str, '=')))
		return (dev_t)strtol(equal + 1, (char **)NULL, 16);

	return (NODEV);
}
