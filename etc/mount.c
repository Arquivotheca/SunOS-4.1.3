#ifndef lint
static  char sccsid[] = "@(#)mount.c 1.1 92/07/30 Copyr 1985, 1989 Sun Micro";
#endif

/*
 * Copyright (c) 1985, 1989 Sun Microsystems, Inc.
 */

/*
 * mount
 */

#define	NFSCLIENT

#include <ctype.h>
#include <locale.h>
#include <strings.h>
#include <errno.h>
#include <sys/param.h>
#include <rpc/rpc.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdio.h>
#include <mntent.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/unistd.h>			/* static pathconf kludge */

/*
 * This file system type isn't yet universally known...
 */
#ifndef	MNTTYPE_TFS
#define	MNTTYPE_TFS	"tfs"
#endif

#define MNTTYPE_RFS     "rfs"

int	ro = 0;
int	quota = 0;
int	fake = 0;
int	freq = 1;
int	passno = 2;
int	all = 0;
int	verbose = 0;
int	printed = 0;			/* mountd messages */
int	nomtab = 0;


#define	NRETRY	10000	/* number of times to retry a mount request */
#define	BGSLEEP	5	/* initial sleep time for background mount in seconds */
#define	MAXSLEEP 120	/* max sleep time for background mount in seconds */
/*
 * Fake errno for RPC failures that don't map to real errno's
 */
#define	ERPC	(10000)

extern char	*strdup();
extern char	*realpath();
extern CLIENT	*clnt_create_vers();
void		clean_mtab();
static void	replace_opts();
static void	printmtab();
static char	host[MNTMAXSTR];
static char	type[MNTMAXSTR];
static char	opts[MNTMAXSTR];
static char	netname[MAXNETNAMELEN+1];

/*
 * Structure used to build a mount tree.  The tree is traversed to do
 * the mounts and catch dependencies.
 */
struct mnttree {
	struct mntent *mt_mnt;
	struct mnttree *mt_sib;
	struct mnttree *mt_kid;
};
static struct mnttree *maketree();

main(argc, argv)
	int argc;
	char **argv;
{
	struct mntent mnt;
	struct mntent *mntp;
	FILE *mnttab;
	char *name, *dir;
	char *options;
	struct mnttree *mtree;

	/*
	 * This code calls upon libc functions that in turn use
	 * their notion of the correct ctype table. We should load
	 * the ctype table dependant on the valid setting for locale
	 * Note no other part of locale yet required to switch
	 */
	setlocale(LC_CTYPE, "");

	if (argc == 1) {
		mnttab = setmntent(MOUNTED, "r");
		while ((mntp = getmntent(mnttab)) != NULL) {
			if (strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) {
				continue;
			}
			printent(mntp);
		}
		(void) endmntent(mnttab);
		exit(0);
	}

	(void) close(2);
	if (dup2(1, 2) < 0) {
		perror("dup");
		exit(1);
	}

	opts[0] = '\0';
	type[0] = '\0';

	/*
	 * Set options
	 */
	while (argc > 1 && argv[1][0] == '-') {
		options = &argv[1][1];
		while (*options) {
			switch (*options) {
			case 'a':
				all++;
				break;
			case 'd':	/* For RFS SVID compatibility */
				(void) strcpy(type, "rfs");
				break;
			case 'f':
				fake++;
				break;
			case 'o':
				if (argc < 3) {
					usage();
				}
				(void) strcpy(opts, argv[2]);
				argv++;
				argc--;
				break;
			case 'n':
				nomtab++;
				break;
			case 'p':
				if (argc != 2) {
					usage();
				}
				printmtab(stdout);
				exit(0);
			case 'q':
				quota++;
				break;
			case 'r':
				ro++;
				break;
			case 't':
				if (argc < 3) {
					usage();
				}
				(void) strcpy(type, argv[2]);
				argv++;
				argc--;
				break;
			case 'v':
				verbose++;
				break;
			default:
				(void) fprintf(stderr,
				    "mount: unknown option: %c\n", *options);
				usage();
			}
			options++;
		}
		argc--, argv++;
	}

	if (geteuid() != 0) {
		(void) fprintf(stderr, "Must be root to use mount\n");
		exit(1);
	}

	if (all) {
		struct stat mnttab_stat;
		long mnttab_size;
		int off;

		if (argc != 1) {
			usage();
		}
		mnttab = setmntent(MNTTAB, "r");
		if (mnttab == NULL) {
			(void) fprintf(stderr, "mount: ");
			perror(MNTTAB);
			exit(1);
		}
		if (fstat(fileno(mnttab), &mnttab_stat) == -1) {
			(void) fprintf(stderr, "mount: ");
			perror(MNTTAB);
			exit(1);
		}
		mnttab_size = mnttab_stat.st_size;
		mtree = NULL;

		for (;;) {
			if ((mntp = getmntent(mnttab)) == NULL) {
				off = ftell(mnttab);
				if (off >= mnttab_size)
					break;		/* it's EOF */
				(void) fprintf(stderr,
				    "mount: %s: illegal entry on line %d\n",
				    MNTTAB, lineof(mnttab, off)-1);
				continue;
			}
			if ((strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) ||
			    (strcmp(mntp->mnt_type, MNTTYPE_SWAP) == 0) ||
			    hasmntopt(mntp, MNTOPT_NOAUTO) ||
			    (strcmp(mntp->mnt_dir, "/") == 0)) {
				continue;
			}
			if (type[0] != '\0' &&
			    strcmp(mntp->mnt_type, type) != 0) {
				continue;
			}
			mtree = maketree(mtree, mntp);
		}
		(void) endmntent(mnttab);
		exit(mounttree(mtree));
	}

	/*
	 * Command looks like: mount <dev>|<dir>
	 * we walk through /etc/fstab til we match either fsname or dir.
	 */
	if (argc == 2) {
		struct stat mnttab_stat;
		long mnttab_size;
		int off;

		mnttab = setmntent(MNTTAB, "r");
		if (mnttab == NULL) {
			(void) fprintf(stderr, "mount: ");
			perror(MNTTAB);
			exit(1);
		}
		if (fstat(fileno(mnttab), &mnttab_stat) == -1) {
			(void) fprintf(stderr, "mount: ");
			perror(MNTTAB);
			exit(1);
		}
		mnttab_size = mnttab_stat.st_size;

		for (;;) {
			if ((mntp = getmntent(mnttab)) == NULL) {
				off = ftell(mnttab);
				if (off >= mnttab_size)
					break;		/* it's EOF */
				(void) fprintf(stderr,
				    "mount: %s: illegal entry on line %d\n",
				    MNTTAB, lineof(mnttab, off)-1);
				continue;
			}
			if ((strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) ||
			    (strcmp(mntp->mnt_type, MNTTYPE_SWAP) == 0)) {
				continue;
			}
			if ((strcmp(mntp->mnt_fsname, argv[1]) == 0) ||
			    (strcmp(mntp->mnt_dir, argv[1]) == 0)) {
				if (opts[0] != '\0') {
					/*
					 * "-o" specified; override fstab with
					 * command line options, unless it's
					 * "-o remount", in which case do
					 * nothing if the fstab says R/O (you
					 * can't remount from R/W to R/O, and
					 * remounting from R/O to R/O is not
					 * only invalid but pointless).
					 */
					if (strcmp(opts, MNTOPT_REMOUNT) == 0 &&
					    hasmntopt(mntp, MNTOPT_RO))
						exit(0);
					mntp->mnt_opts = opts;
				}
				replace_opts(mntp->mnt_opts, ro, MNTOPT_RO,
				    MNTOPT_RW);
				if (strcmp(type, MNTTYPE_42) == 0)
					replace_opts(mntp->mnt_opts, quota,
					    MNTOPT_QUOTA, MNTOPT_NOQUOTA);
				exit(mounttree(maketree((struct mnttree *)NULL,
				    mntp)));
			}
		}
		(void) fprintf(stderr, "mount: %s not found in %s\n", argv[1],
		    MNTTAB);
		exit(1);
	}

	if (argc != 3) {
		usage();
	}
	name = argv[1];
	dir  = argv[2];

	/*
	 * If the file system type is not given then
	 * assume "nfs" if the name is of the form
	 *	host:path
	 * otherwise assume "4.2".
	 */
	if (type[0] == '\0')
		(void) strcpy(type,
		    index(name, ':') ? MNTTYPE_NFS : MNTTYPE_42);

	if (dir[0] != '/') {
		(void) fprintf(stderr,
		    "mount: directory path must begin with '/'\n");
		exit(1);
	}

	replace_opts(opts, ro, MNTOPT_RO, MNTOPT_RW);
	if (strcmp(type, MNTTYPE_42) == 0)
		replace_opts(opts, quota, MNTOPT_QUOTA, MNTOPT_NOQUOTA);

	if (strcmp(type, MNTTYPE_NFS) == 0 || strcmp(type, MNTTYPE_RFS) == 0) {
		passno = 0;
		freq = 0;
	}

	mnt.mnt_fsname = name;
	mnt.mnt_dir = dir;
	mnt.mnt_type = type;
	mnt.mnt_opts = opts;
	mnt.mnt_freq = freq;
	mnt.mnt_passno = passno;
	exit(mounttree(maketree((struct mnttree *)NULL, &mnt)));
	/* NOTREACHED */
}

/*
 * Given an offset into a file, return the line number
 */
int
lineof(fd, offset)
	FILE *fd; long offset;
{
	int c, cnt, lcnt;

	cnt = 0;
	lcnt = 1;
	rewind(fd);
	while ((c = getc(fd)) != EOF) {
		if (c == '\n')
			lcnt++;
		if (cnt >= offset)
			break;
		cnt++;
	}
	(void) fseek(fd, offset, 0);
	return lcnt;
}

/*
 * attempt to mount file system, return errno or 0
 */
int
mountfs(print, mnt, opts)
	int print;
	struct mntent *mnt;
	char *opts;
{
	int error;
	int flags = 0;
	union data {
		struct ufs_args	ufs_args;
		struct nfs_args nfs_args;
#ifdef	PCFS
		struct pc_args pc_args;
#endif
	} data;

	if (hasmntopt(mnt, MNTOPT_REMOUNT) == 0) {
		if (mounted(mnt)) {
			if (print && verbose) {
				(void) fprintf(stderr,
				    "mount: %s already mounted\n",
				    mnt->mnt_fsname);
			}
			return (0);
		}
	} else
		if (print && verbose)
			(void) fprintf (stderr,
			    "mountfs: remount ignoring mtab\n");
	if (fake) {
		addtomtab(mnt);
		return (0);
	}

	/*
	 * For file system types that make resources already visible in the
	 * local name space appear elsewhere (e.g., tfs and lofs), make sure
	 * that the name for the thing being mounted (the "file system") is in
	 * canonical form.
	 */
	if (strcmp(mnt->mnt_type, MNTTYPE_LO) == 0 ||
	    strcmp(mnt->mnt_type, MNTTYPE_TFS) == 0) {
		char canon_fsname[MAXPATHLEN];

		/*
		 * If realpath fails, use the original name; otherwise replace
		 * it.
		 */
		if (realpath(mnt->mnt_fsname, canon_fsname) != NULL) {
			free(mnt->mnt_fsname);
			mnt->mnt_fsname = strdup(canon_fsname);
		}
	}

	if (strcmp(mnt->mnt_type, MNTTYPE_42) == 0) {
		error = mount_42(mnt, &data.ufs_args);
	} else if (strcmp(mnt->mnt_type, MNTTYPE_NFS) == 0) {
		error = mount_nfs(mnt, &data.nfs_args);
#ifdef	PCFS
	} else if (strcmp(mnt->mnt_type, MNTTYPE_PC) == 0) {
		error = mount_pc(mnt, &data.pc_args);
#endif
	} else {
		/*
		 * Invoke "mount" command for particular file system type.
		 */
		char mountcmd[128];
		int pid;
		union wait status;

		pid = fork();
		switch (pid) {

		case -1:
			(void) fprintf(stderr,
			    "mount: %s on ", mnt->mnt_fsname);
			perror(mnt->mnt_dir);
			return (errno);
		case 0:
			(void) sprintf(mountcmd, "mount_%s", mnt->mnt_type);
			execlp(mountcmd, mountcmd, mnt->mnt_fsname,
			    mnt->mnt_dir, mnt->mnt_type, mnt->mnt_opts,
			    (char *)NULL);
			if (errno == ENOENT) {
				(void) fprintf(stderr,
				    "mount: unknown filesystem type: %s\n",
				    mnt->mnt_type);
			} else if (print) {
				(void) fprintf(stderr, "%s: %s on %s ",
				    mountcmd, mnt->mnt_fsname,
				    mnt->mnt_dir);
				perror("exec failed");
			}
			exit(errno);
		default:
			while (wait(&status) != pid);
			error = status.w_retcode;
			if (!error) {
				*mnt->mnt_opts = '\0';
				goto itworked;
			}
		}
	}
	if (error) {
		return (error);
	}

	flags |= eatmntopt(mnt, MNTOPT_RO) ? M_RDONLY : 0;
	flags |= eatmntopt(mnt, MNTOPT_NOSUID) ? M_NOSUID : 0;
	flags |= eatmntopt(mnt, MNTOPT_GRPID) ? M_GRPID : 0;
	flags |= eatmntopt(mnt, MNTOPT_REMOUNT) ? M_REMOUNT : 0;
	flags |= M_NEWTYPE;

	if (mount(mnt->mnt_type, mnt->mnt_dir, flags, &data) < 0) {
		if (errno == ENODEV) {
			/*
			 * might be an old kernel, need to try again
			 * with file type number instead of string
			 */
#define	MAXTYPE 3
			int typeno;
			static char *typetab[] =
			    { MNTTYPE_42, MNTTYPE_NFS, MNTTYPE_PC };

			for (typeno = 0; typeno < MAXTYPE; typeno++) {
				if (strcmp(typetab[typeno], mnt->mnt_type) == 0)
					break;
			}
			if (typeno < MAXTYPE) {
				error = errno;
				if (mount(typeno, mnt->mnt_dir, flags, &data)
				    >= 0) {
					goto itworked;
				}
				errno = error;  /* restore original error */
			}
		}
		if (print) {
			(void) fprintf(stderr, "mount: %s on ",
			    mnt->mnt_fsname);
			perror(mnt->mnt_dir);
		}
		return (errno);
	}

itworked:
	fixopts(mnt, opts);
	if (*opts) {
		(void) fprintf(stderr,
		    "mount: %s on %s WARNING unknown options %s\n",
		    mnt->mnt_fsname, mnt->mnt_dir, opts);
	}
	if (hasmntopt(mnt, MNTOPT_REMOUNT)) {
		clean_mtab(mnt->mnt_dir);	/* remove old entry */
		rmopt(mnt, MNTOPT_REMOUNT);
	}
	addtomtab(mnt);
	return (0);
}

mount_42(mnt, args)
	struct mntent *mnt;
	struct ufs_args *args;
{
	static char name[MAXPATHLEN];

	(void) strcpy(name, mnt->mnt_fsname);
	args->fspec = name;
	return (0);
}

mount_nfs(mnt, args)
	struct mntent *mnt;
	struct nfs_args *args;
{
	static struct sockaddr_in sin;
	static struct fhstatus fhs;
	static struct pathcnf p;
	char *cp;
	char *hostp = host;
	char *path;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
	u_short port;
	int posix = 0;
	u_long outvers = 0;
        int vers_to_try = (int)MOUNTVERS_POSIX;
	int actimeo;

	cp = mnt->mnt_fsname;
	while ((*hostp = *cp) != ':') {
		if (*cp == '\0') {
			(void) fprintf(stderr,
			    "mount: nfs file system; use host:path\n");
			return (1);
		}
		hostp++;
		cp++;
	}
	*hostp = '\0';
	path = ++cp;

	args->flags = 0;
	if (eatmntopt(mnt, MNTOPT_SOFT)) {
		args->flags |= NFSMNT_SOFT;
	}
	if (eatmntopt(mnt, MNTOPT_INTR)) {
		args->flags |= NFSMNT_INT;
	}
	if (eatmntopt(mnt, MNTOPT_SECURE)) {
		args->flags |= NFSMNT_SECURE;
	}
	if (eatmntopt(mnt, MNTOPT_NOAC)) {
		args->flags |= NFSMNT_NOAC;
	}
	if (eatmntopt(mnt, MNTOPT_NOCTO)) {
		args->flags |= NFSMNT_NOCTO;
	}
	if (eatmntopt(mnt, MNTOPT_POSIX)) {
		posix++;
	}

	/*
	 * get fhandle of remote path from server's mountd
	 */
	while ((client = clnt_create_vers(host, (u_long)MOUNTPROG, &outvers,
	    (u_long)MOUNTVERS, (u_long)vers_to_try, "udp")) == NULL) {
		if (rpc_createerr.cf_stat == RPC_UNKNOWNHOST){
			(void) fprintf(stderr,
			    "mount: ");
			clnt_pcreateerror(mnt->mnt_fsname);
			return (ERPC);
		}
                /*
		 * back off and try the previous version - patch to the
		 * problem of version numbers not being contigous and
		 * clnt_create_vers failing (SunOS4.1 clients & SGI servers)
		 * The problem happens with most non-Sun servers who
		 * don't support mountd protocol #2. So, in case the
		 * call fails, we re-try the call anyway.
		 */
		vers_to_try--;
		if (vers_to_try >= (int)MOUNTVERS)
			continue;
		if (!printed) {
			(void) fprintf(stderr,
			    "mount: %s server not responding",
			    mnt->mnt_fsname);
			clnt_pcreateerror("");
			printed = 1;
		}
		return (ETIMEDOUT);
	}
/* static pathconf kludge BEGIN */
	if (posix  &&  outvers != MOUNTVERS_POSIX) {
		(void) fprintf(stderr,
		    "mount: no pathconf info for %s.\n",
		    mnt->mnt_fsname);
		return ERPC;
	}
/* static pathconf kludge END */
	client->cl_auth = authunix_create_default();
	timeout.tv_usec = 0;
	timeout.tv_sec = 20;
	rpc_stat = clnt_call(client, MOUNTPROC_MNT, xdr_path, &path,
	    xdr_fhstatus, &fhs, timeout);
	errno = 0;
	if (rpc_stat != RPC_SUCCESS) {
		if (!printed) {
			(void) fprintf(stderr,
			    "mount: %s server not responding",
			    mnt->mnt_fsname);
			clnt_perror(client, "");
			printed = 1;
		}
		switch (rpc_stat) {
		case RPC_TIMEDOUT:
		case RPC_PMAPFAILURE:
		case RPC_PROGNOTREGISTERED:
			errno = ETIMEDOUT;
			break;
		case RPC_AUTHERROR:
			errno = EACCES;
			break;
		default:
			errno = ERPC;
			break;
		}
	}
/* static pathconf kludge BEGIN */
	if (!errno  &&  posix) {
		rpc_stat = clnt_call(client, MOUNTPROC_PATHCONF,
		    xdr_path, &path, xdr_pathcnf, &p, timeout);
		if (rpc_stat != RPC_SUCCESS) {
			if (!printed) {
				(void) fprintf(stderr,
				    "mount: %s server not responding",
				    mnt->mnt_fsname);
				clnt_perror(client, "");
				printed = 1;
			}
			switch (rpc_stat) {
			case RPC_TIMEDOUT:
			case RPC_PMAPFAILURE:
			case RPC_PROGNOTREGISTERED:
				errno = ETIMEDOUT;
				break;
			case RPC_AUTHERROR:
				errno = EACCES;
				break;
			default:
				errno = ERPC;
				break;
			}
		}
		else {
			if (_PC_ISSET(_PC_ERROR, p.pc_mask)) {
				(void) fprintf(stderr,
				    "mount: no pathconf info for %s.\n",
				    mnt->mnt_fsname);
				return ERPC;
			}
			args->pathconf = &p;
			args->flags |= NFSMNT_POSIX;
		}
	}
/* static pathconf kludge END */
	/*
	 * grab the sockaddr for the kernel
	 */
	clnt_control(client, CLGET_SERVER_ADDR, &sin);
	clnt_destroy(client);

	if (errno) {
		return (errno);
	}

	if (errno = fhs.fhs_status) {
		if (errno == EACCES) {
			(void) fprintf(stderr,
			    "mount: access denied for %s:%s\n", host, path);
		} else {
			(void) fprintf(stderr, "mount: ");
			perror(mnt->mnt_fsname);
		}
		return (errno);
	}
	if (printed) {
		(void) fprintf(stderr, "mount: %s server ok\n",
		    mnt->mnt_fsname);
		printed = 0;
	}

	/*
	 * set mount args
	 */
	args->fh = (caddr_t)&fhs.fhs_fh;
	args->hostname = host;
	args->flags |= NFSMNT_HOSTNAME;
	if (args->rsize = nopt(mnt, MNTOPT_RSIZE)) {
		args->flags |= NFSMNT_RSIZE;
	}
	if (args->wsize = nopt(mnt, MNTOPT_WSIZE)) {
		args->flags |= NFSMNT_WSIZE;
	}
	if (args->timeo = nopt(mnt, MNTOPT_TIMEO)) {
		args->flags |= NFSMNT_TIMEO;
	}
	if (args->retrans = nopt(mnt, MNTOPT_RETRANS)) {
		args->flags |= NFSMNT_RETRANS;
	}
	if (actimeo = nopt(mnt, MNTOPT_ACTIMEO)) {
		args->acregmin = args->acregmax = actimeo;
		args->acdirmin = args->acdirmax = actimeo;
		args->flags |= (NFSMNT_ACREGMIN | NFSMNT_ACREGMAX |
				NFSMNT_ACDIRMIN | NFSMNT_ACDIRMAX);
	} else {
		if (args->acregmin = nopt(mnt, MNTOPT_ACREGMIN)) {
			args->flags |= NFSMNT_ACREGMIN;
		}
		if (args->acregmax = nopt(mnt, MNTOPT_ACREGMAX)) {
			args->flags |= NFSMNT_ACREGMAX;
		}
		if (args->acdirmin = nopt(mnt, MNTOPT_ACDIRMIN)) {
			args->flags |= NFSMNT_ACDIRMIN;
		}
		if (args->acdirmax = nopt(mnt, MNTOPT_ACDIRMAX)) {
			args->flags |= NFSMNT_ACDIRMAX;
		}
	}
	if (args->flags & NFSMNT_SECURE) {
		/*
		 * XXX: need to support other netnames outside domain
		 * and not always just use the default conversion
		 */
		if (!host2netname(netname, host, (char *)NULL)) {
			return (EHOSTDOWN); /* really unknown host */
		}
		args->netname = netname;
	}
	if (port = nopt(mnt, MNTOPT_PORT)) {
		sin.sin_port = htons(port);
	} else {
		sin.sin_port = htons(NFS_PORT);	/* XXX should use portmapper */
	}
	args->addr = &sin;

	/*
	 * should clean up mnt ops to not contain defaults
	 */
	return (0);
}

#ifdef	PCFS
mount_pc(mnt, args)
	struct mntent *mnt;
	struct pc_args *args;
{
	args->fspec = mnt->mnt_fsname;
	return (0);
}
#endif

printent(mnt)
	struct mntent *mnt;
{
	rmopt(mnt, MNTINFO_DEV);
	(void) fprintf(stdout, "%s on %s type %s (%s)\n",
	    mnt->mnt_fsname, mnt->mnt_dir, mnt->mnt_type, mnt->mnt_opts);
}

static void
printmtab(outp)
	FILE *outp;
{
	FILE *mnttab;
	struct mntent *mntp;
	int maxfsname = 0;
	int maxdir = 0;
	int maxtype = 0;
	int maxopts = 0;

	/*
	 * first go through and find the max width of each field
	 */
	mnttab = setmntent(MOUNTED, "r");
	while ((mntp = getmntent(mnttab)) != NULL) {
		if (strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0)
			continue;
		if (strlen(mntp->mnt_fsname) > maxfsname) {
			maxfsname = strlen(mntp->mnt_fsname);
		}
		if (strlen(mntp->mnt_dir) > maxdir) {
			maxdir = strlen(mntp->mnt_dir);
		}
		if (strlen(mntp->mnt_type) > maxtype) {
			maxtype = strlen(mntp->mnt_type);
		}
		if (strlen(mntp->mnt_opts) > maxopts) {
			maxopts = strlen(mntp->mnt_opts);
		}
	}
	(void) endmntent(mnttab);

	/*
	 * now print them out in pretty format
	 */
	mnttab = setmntent(MOUNTED, "r");
	while ((mntp = getmntent(mnttab)) != NULL) {
		if (strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0)
			continue;
		(void) fprintf(outp, "%-*s", maxfsname+1, mntp->mnt_fsname);
		(void) fprintf(outp, "%-*s", maxdir+1, mntp->mnt_dir);
		(void) fprintf(outp, "%-*s", maxtype+1, mntp->mnt_type);
		rmopt(mntp, MNTINFO_DEV);
		(void) fprintf(outp, "%-*s", maxopts+1, mntp->mnt_opts);
		(void) fprintf(outp, " %d %d\n", mntp->mnt_freq,
		    mntp->mnt_passno);
	}
	(void) endmntent(mnttab);
}

/*
 * Check to see if mntck is already mounted.
 * We have to be careful because getmntent modifies its static struct.
 */
mounted(mntck)
	struct mntent *mntck;
{
	int found = 0;
	struct mntent *mnt, mntsave;
	FILE *mnttab;

	if (nomtab) {
		return (0);
	}
	mnttab = setmntent(MOUNTED, "r");
	if (mnttab == NULL) {
		(void) fprintf(stderr, "mount: ");
		perror(MOUNTED);
		exit(1);
	}
	mntcp(mntck, &mntsave);
	while ((mnt = getmntent(mnttab)) != NULL) {
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0) {
			continue;
		}
		rmopt(mnt, MNTINFO_DEV);
		if ((strcmp(mntsave.mnt_fsname, mnt->mnt_fsname) == 0) &&
		    (strcmp(mntsave.mnt_dir, mnt->mnt_dir) == 0) &&
		    (strcmp(mntsave.mnt_opts, mnt->mnt_opts) == 0)) {
			found = 1;
			break;
		}
	}
	(void) endmntent(mnttab);
	*mntck = mntsave;
	return (found);
}

mntcp(mnt1, mnt2)
	struct mntent *mnt1, *mnt2;
{
	static char fsname[128], dir[128], type[128], opts[128];

	mnt2->mnt_fsname = fsname;
	(void) strcpy(fsname, mnt1->mnt_fsname);
	mnt2->mnt_dir = dir;
	(void) strcpy(dir, mnt1->mnt_dir);
	mnt2->mnt_type = type;
	(void) strcpy(type, mnt1->mnt_type);
	mnt2->mnt_opts = opts;
	(void) strcpy(opts, mnt1->mnt_opts);
	mnt2->mnt_freq = mnt1->mnt_freq;
	mnt2->mnt_passno = mnt1->mnt_passno;
}

/*
 * Return the value of a numeric option of the form foo=x, if
 * option is not found or is malformed, return 0.
 */
nopt(mnt, opt)
	struct mntent *mnt;
	char *opt;
{
	int val = 0;
	char *equal;
	char *str;

	if (str = hasmntopt(mnt, opt)) {
		if (equal = index(str, '=')) {
			val = atoi(&equal[1]);
		} else {
			(void) fprintf(stderr,
			    "mount: bad numeric option '%s'\n", str);
		}
	}
	rmopt(mnt, opt);
	return (val);
}

/*
 * same as hasmntopt but remove the option from the option string and return
 * true or false
 */
eatmntopt(mnt, opt)
	struct mntent *mnt;
	char *opt;
{
	int has;

	has = (hasmntopt(mnt, opt) != NULL);
	rmopt(mnt, opt);
	return (has);
}

/*
 * remove an option string from the option list
 */
rmopt(mnt, opt)
	struct mntent *mnt;
	char *opt;
{
	char *str;
	char *optstart;

	if (optstart = hasmntopt(mnt, opt)) {
		for (str = optstart; *str != ',' && *str != '\0'; str++)
			;
		if (*str == ',') {
			str++;
		} else if (optstart != mnt->mnt_opts) {
			optstart--;
		}
		while (*optstart++ = *str++)
			;
	}
}

/*
 * mnt->mnt_ops has un-eaten opts, opts is the original opts list.
 * Set mnt->mnt_opts to the original list minus the un-eaten opts.
 * Set "opts" to the un-eaten opts minus the "default" options ("rw",
 * "hard", "noquota", "noauto") and "bg".  If there are any options left after
 * this, they are uneaten because they are unknown; our caller will print a
 * warning message.
 */
fixopts(mnt, opts)
	struct mntent *mnt;
	char *opts;
{
	char *comma;
	char *ue;
	char uneaten[1024];

	rmopt(mnt, MNTOPT_RW);
	rmopt(mnt, MNTOPT_HARD);
	rmopt(mnt, MNTOPT_QUOTA);
	rmopt(mnt, MNTOPT_NOQUOTA);
	rmopt(mnt, MNTOPT_NOAUTO);
	rmopt(mnt, "bg");
	rmopt(mnt, "fg");
	rmopt(mnt, "suid");
	(void) strcpy(uneaten, mnt->mnt_opts);
	(void) strcpy(mnt->mnt_opts, opts);
	(void) strcpy(opts, uneaten);

	for (ue = uneaten; *ue; ) {
		for (comma = ue; *comma != '\0' && *comma != ','; comma++)
			;
		if (*comma == ',') {
			*comma = '\0';
			rmopt(mnt, ue);
			ue = comma+1;
		} else {
			rmopt(mnt, ue);
			ue = comma;
		}
	}
	if (*mnt->mnt_opts == '\0') {
		(void) strcpy(mnt->mnt_opts, MNTOPT_RW);
	}
}

/*
 * update /etc/mtab
 */
addtomtab(mnt)
	struct mntent *mnt;
{
	FILE *mnted;
	struct stat st;
	char buf[16];

	if (nomtab) {
		return;
	}

	if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) != 0) {
		if (stat(mnt->mnt_dir, &st) < 0) {
			perror(mnt->mnt_dir);
			return;
		}
		(void) sprintf(buf, ",%s=%04x",
			MNTINFO_DEV, (st.st_dev & 0xFFFF));
		(void) strcat(mnt->mnt_opts, buf);
	}

	mnted = setmntent(MOUNTED, "r+");
	if (mnted == NULL) {
		(void) fprintf(stderr, "mount: ");
		perror(MOUNTED);
		exit(1);
	}
	if (addmntent(mnted, mnt)) {
		(void) fprintf(stderr, "mount: ");
		perror(MOUNTED);
		exit(1);
	}
	(void) endmntent(mnted);

	if (verbose) {
		(void) fprintf(stdout, "%s mounted on %s\n",
		    mnt->mnt_fsname, mnt->mnt_dir);
	}
}

/*
 *  Remove an entry from /etc/mtab
 */
void
clean_mtab(mntpnt)
	char *mntpnt;
{
	FILE *fold, *fnew;
	struct mntent *mnt;
	char tmpnam[64];

	fold = setmntent(MOUNTED, "r+");
	if (fold == NULL) {
		(void) fprintf(stderr, "mount: ");
		perror(MOUNTED);
		return;
	}
	(void) sprintf(tmpnam, "%s.XXXXXX", MOUNTED);
	(void) mktemp(tmpnam);
	fnew = setmntent(tmpnam, "w");
	if (fnew == NULL) {
		(void) fprintf(stderr, "mount: ");
		perror(tmpnam);
		(void) endmntent(fold);
		return;
	}

	while (mnt = getmntent(fold)) {

		if (strcmp(mnt->mnt_dir, mntpnt) == 0)
			continue;
		if (addmntent(fnew, mnt)) {
			(void) fprintf(stderr, "mount: addmntent: ");
			perror(tmpnam);
			(void) endmntent(fold);
			(void) endmntent(fnew);
			(void) unlink(tmpnam);
			return;
		}
	}

	(void) endmntent(fold);
	(void) endmntent(fnew);
	if (rename(tmpnam, MOUNTED) < 0) {
		(void) fprintf(stderr, "mount: rename %s %s", tmpnam, MOUNTED);
		perror("");
		(void) unlink(tmpnam);
		return;
	}
}

char *
xmalloc(size)
	int size;
{
	char *ret;

	if ((ret = (char *)malloc((unsigned)size)) == NULL) {
		(void) fprintf(stderr, "mount: ran out of memory!\n");
		exit(1);
	}
	return (ret);
}

struct mntent *
mntdup(mnt)
	struct mntent *mnt;
{
	struct mntent *new;

	new = (struct mntent *)xmalloc(sizeof (*new));

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

/*
 * Build the mount dependency tree
 */
static struct mnttree *
maketree(mt, mnt)
	struct mnttree *mt;
	struct mntent *mnt;
{
	if (mt == NULL) {
		mt = (struct mnttree *)xmalloc(sizeof (struct mnttree));
		mt->mt_mnt = mntdup(mnt);
		mt->mt_sib = NULL;
		mt->mt_kid = NULL;
	} else {
		if (substr(mt->mt_mnt->mnt_dir, mnt->mnt_dir)) {
			mt->mt_kid = maketree(mt->mt_kid, mnt);
		} else {
			mt->mt_sib = maketree(mt->mt_sib, mnt);
		}
	}
	return (mt);
}

printtree(mt)
	struct mnttree *mt;
{
	if (mt) {
		printtree(mt->mt_sib);
		(void) printf("   %s\n", mt->mt_mnt->mnt_dir);
		printtree(mt->mt_kid);
	}
}

int
mounttree(mt)
	struct mnttree *mt;
{
	int error, retval = 0;
	int slptime;
	int forked;
	int retry;
	int firsttry;
	int background;
	char dir[MAXPATHLEN];
	char opts[1024];
	struct mntent *mnt;

	for ( ; mt; mt = mt->mt_sib) {
		forked = 0;
		printed = 0;
		firsttry = 1;
		slptime = BGSLEEP;
		mnt = mt->mt_mnt;
		retry = nopt(mnt, "retry");
		if (retry == 0) {
			retry = NRETRY;
		}
		if (realpath(mnt->mnt_dir, dir) == NULL) {
			(void) fprintf(stderr, "mount: ");
			perror(mnt->mnt_dir);
			retval = 1;
			continue;
		}
		free(mnt->mnt_dir);
		mnt->mnt_dir = strdup(dir);
		if (mnt->mnt_dir == NULL) {
			(void) fprintf(stderr,
			    "mount: ran out of memory!\n");
			exit(1);
		}
		(void) strcpy(opts, mnt->mnt_opts);

		do {
			error = mountfs(!forked, mnt, opts);
			if (error != ETIMEDOUT && error != ENETDOWN &&
			    error != ENETUNREACH && error != ENOBUFS &&
			    error != ECONNREFUSED && error != ECONNABORTED) {
				break;
			}
			/*
			 * mount failed due to network problems, reset options
			 * string and retry (maybe)
			 */
			(void) strcpy(mnt->mnt_opts, opts);
			background = eatmntopt(mnt, "bg");
			if (!forked && background) {
				(void) fprintf(stderr,
				    "mount: backgrounding\n");
				(void) fprintf(stderr, "   %s\n",
				    mnt->mnt_dir);
				printtree(mt->mt_kid);
				if (fork()) {
					goto done_fork;
				} else {
					forked = 1;
				}
			}
			if (!forked && firsttry) {
				(void) fprintf(stderr, "mount: retrying\n");
				(void) fprintf(stderr, "   %s\n",
				    mnt->mnt_dir);
				printtree(mt->mt_kid);
				firsttry = 0;
			}
			sleep((unsigned)slptime);
			slptime = MIN(slptime << 1, MAXSLEEP);
		} while (retry--);

		if (!error) {
			if (mounttree(mt->mt_kid))
				retval = 1;
		} else {
			retval = 1;
			(void) fprintf(stderr, "mount: giving up on:\n");
			(void) fprintf(stderr, "   %s\n", mnt->mnt_dir);
			printtree(mt->mt_kid);
		}
		if (forked) {
			exit(0);
		}
	done_fork:;
	}
	return (retval);
}

/*
 * Returns true if s1 is a pathname substring of s2.
 */
substr(s1, s2)
	char *s1;
	char *s2;
{
	while (*s1 == *s2) {
		s1++;
		s2++;
	}
	if (*s1 == '\0' && *s2 == '/') {
		return (1);
	}
	return (0);
}

usage()
{
	(void) fprintf(stdout,
	    "Usage: mount [-rqavpfto [type|option]] ... [fsname] [dir]\n");
	exit(1);
}

/*
 * Returns the next option in the option string.
 */
static char *
getnextopt(p)
	char **p;
{
	char *cp = *p;
	char *retstr;

	while (*cp && isspace(*cp))
		cp++;
	retstr = cp;
	while (*cp && *cp != ',')
		cp++;
	if (*cp) {
		*cp = '\0';
		cp++;
	}
	*p = cp;
	return (retstr);
}

/*
 * "trueopt" and "falseopt" are two settings of a Boolean option.
 * If "flag" is true, forcibly set the option to the "true" setting; otherwise,
 * if the option isn't present, set it to the false setting.
 */
static void
replace_opts(options, flag, trueopt, falseopt)
	char *options;
	int flag;
	char *trueopt;
	char *falseopt;
{
	register char *f;
	char tmptopts[MNTMAXSTR];
	char *tmpoptsp;
	register int found;

	(void) strcpy(tmptopts, options);
	tmpoptsp = tmptopts;
	(void) strcpy(options, "");

	found = 0;
	for (f = getnextopt(&tmpoptsp); *f; f = getnextopt(&tmpoptsp)) {
		if (options[0] != '\0')
			(void) strcat(options, ",");
		if (strcmp(f, trueopt) == 0) {
			(void) strcat(options, f);
			found++;
		} else if (strcmp(f, falseopt) == 0) {
			if (flag)
				(void) strcat(options, trueopt);
			else
				(void) strcat(options, f);
			found++;
		} else
			(void) strcat(options, f);
	}
	if (!found) {
		if (options[0] != '\0')
			(void) strcat(options, ",");
		(void) strcat(options, flag ? trueopt : falseopt);
	}
}
