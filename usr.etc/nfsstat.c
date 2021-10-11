/*
 *	  @(#)nfsstat.c 1.1 92/07/30 SMI
 *
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * nfsstat: Network File System statistics
 *
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/vfs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <nlist.h>
#include <fcntl.h>
#include <kvm.h>
#include <mntent.h>
#include <string.h>

struct nlist nl[] = {
#define	X_RCSTAT	0
	{ "_rcstat" },
#define	X_CLSTAT	1
	{ "_clstat" },
#define	X_RSSTAT	2
	{ "_rsstat" },
#define	X_SVSTAT	3
	{ "_svstat" },
#define	X_ROOTVFS	4
	{ "_rootvfs" },
#define	X_NFS_VFSOPS	5
	{ "_nfs_vfsops" },
	"",
};

kvm_t *kd;			/* kernel id from kvm_open */
char *vmunix = NULL;		/* name for /vmunix */
char *core   = NULL;		/* name for /dev/kmem */
char *progname;			/* argv[0] */

/*
 * client side rpc statistics
 */
struct {
	int	rccalls;
	int	rcbadcalls;
	int	rcretrans;
	int	rcbadxids;
	int	rctimeouts;
	int	rcwaits;
	int	rcnewcreds;
	int	rcbadverfs;
	int	rctimers;
} rcstat;

/*
 * client side nfs statistics
 */
struct {
	u_int	nclsleeps;		/* client handle waits */
	u_int	nclgets;		/* client handle gets */
	u_int	ncalls;			/* client requests */
	u_int	nbadcalls;		/* rpc failures */
	u_int	reqs[32];		/* count of each request */
} clstat;

/*
 * Server side rpc statistics
 */
struct {
	int	rscalls;
	int	rsbadcalls;
	int	rsnullrecv;
	int	rsbadlen;
	int	rsxdrcall;
} rsstat;

/*
 * server side nfs statistics
 */
struct {
	u_int	ncalls;		/* number of calls received */
	u_int	nbadcalls;	/* calls that failed */
	u_int	reqs[32];	/* count for each request */
} svstat;

static int	ccode, scode; 	/* Identify server and client code present */

main(argc, argv)
	int argc;
	char *argv[];
{
	char *options;
	int	cflag = 0;		/* client stats */
	int	sflag = 0;		/* server stats */
	int	nflag = 0;		/* nfs stats */
	int	rflag = 0;		/* rpc stats */
	int	zflag = 0;		/* zero stats after printing */
	int	mflag = 0;		/* mount table stats */

	progname = argv[0];

	if (argc >= 2 && *argv[1] == '-') {
		options = &argv[1][1];
		while (*options) {
			switch (*options) {
			case 'c':
				cflag++;
				break;
			case 'n':
				nflag++;
				break;
			case 'm':
				mflag++;
				break;
			case 'r':
				rflag++;
				break;
			case 's':
				sflag++;
				break;
			case 'z':
				if (getuid()) {
					fprintf(stderr,
					    "Must be root for z flag\n");
					exit(1);
				}
				zflag++;
				break;
			default:
				usage();
			}
			options++;
		}
		argv++;
		argc--;
	}
	if (argc >= 2) {
		vmunix = argv[1];
		argv++;
		argc--;
		if (argc == 2) {
			core = argv[1];
			argv++;
			argc--;
		}
	}
	if (argc != 1) {
		usage();
	}

	setup(zflag);
	if (mflag) {
		mi_print();
		exit(0);
	}

	getstats();
	if (sflag && (!scode)) {
		fprintf(stderr, "nfsstat: kernel is not configured with the server nfs and rpc code.\n");
	}
	if ((sflag || (!sflag && !cflag)) && scode) {
		if (rflag || (!rflag && !nflag)) {
			sr_print(zflag);
		}
		if (nflag || (!rflag && !nflag)) {
			sn_print(zflag);
		}
	}
	if (cflag && (!ccode)) {
		fprintf(stderr, "nfsstat: kernel is not configured with the client nfs and rpc code.\n");
	}
	if ((cflag || (!sflag && !cflag)) && ccode) {
		if (rflag || (!rflag && !nflag)) {
			cr_print(zflag);
		}
		if (nflag || (!rflag && !nflag)) {
			cn_print(zflag);
		}
	}
	if (zflag) {
		putstats();
	}
	exit(0);
	/* NOTREACHED */
}

kio(rdwr, id, buf, len)
	int rdwr, id, len;
	char *buf;
{
	if (nl[id].n_type == 0) {
		fprintf(stderr, "nfsstat: '%s' not in namelist\n",
			nl[id].n_name);
		bzero(buf, len);
		return;
	}
	if (rdwr == 0) {
		if (kvm_read(kd, nl[id].n_value, buf, len) != len) {
			fprintf(stderr, "nfsstat: kernel read error\n");
			exit(1);
		}
	} else {
		if (kvm_write(kd, nl[id].n_value, buf, len) != len) {
			fprintf(stderr, "nfsstat: kernel write error\n");
			exit(1);
		}
	}
}

#define kread(id, buf, len)  kio(0, id, (char *)(buf), len)
#define kwrite(id, buf, len) kio(1, id, (char *)(buf), len)

getstats()
{
	if (ccode) {
		kread(X_RCSTAT, &rcstat, sizeof rcstat);
		kread(X_CLSTAT, &clstat, sizeof clstat);
	}
	if (scode) {
		kread(X_RSSTAT, &rsstat, sizeof rsstat);
		kread(X_SVSTAT, &svstat, sizeof svstat);
	}
}

putstats()
{
	if (ccode) {
		kwrite(X_RCSTAT, &rcstat, sizeof rcstat);
		kwrite(X_CLSTAT, &clstat, sizeof clstat);
	}
	if (scode) {
		kwrite(X_RSSTAT, &rsstat, sizeof rsstat);
		kwrite(X_SVSTAT, &svstat, sizeof svstat);
	}
}

setup(zflag)
	int zflag;
{
	if ((kd = kvm_open(vmunix, core, NULL, zflag ? O_RDWR : O_RDONLY, progname)) == NULL)
		exit(1);
	if (kvm_nlist(kd, nl) < 0) {
		fprintf(stderr, "nfsstat: bad namelist\n");
		exit(1);
	}
	/* check to see if the client code is present */
	ccode = (nl[X_RCSTAT].n_type != 0);
	scode = (nl[X_RSSTAT].n_type != 0);
}

cr_print(zflag)
	int zflag;
{
	fprintf(stdout, "\nClient rpc:\n");
	fprintf(stdout,
	 "calls    badcalls retrans  badxid   timeout  wait     newcred  timers\n");
	fprintf(stdout,
	    "%-9d%-9d%-9d%-9d%-9d%-9d%-9d%-9d\n",
	    rcstat.rccalls,
	    rcstat.rcbadcalls,
	    rcstat.rcretrans,
	    rcstat.rcbadxids,
	    rcstat.rctimeouts,
	    rcstat.rcwaits,
	    rcstat.rcnewcreds,
	    rcstat.rctimers);
	if (zflag) {
		bzero((char *)&rcstat, sizeof rcstat);
	}
}

sr_print(zflag)
	int zflag;
{
	fprintf(stdout, "\nServer rpc:\n");
	fprintf(stdout,
	    "calls      badcalls   nullrecv   badlen     xdrcall\n");
	fprintf(stdout,
	    "%-11d%-11d%-11d%-11d%-11d\n",
	   rsstat.rscalls,
	   rsstat.rsbadcalls,
	   rsstat.rsnullrecv,
	   rsstat.rsbadlen,
	   rsstat.rsxdrcall);
	if (zflag) {
		bzero((char *)&rsstat, sizeof rsstat);
	}
}

#define RFS_NPROC	18
char *nfsstr[RFS_NPROC] = {
	"null",
	"getattr",
	"setattr",
	"root",
	"lookup",
	"readlink",
	"read",
	"wrcache",
	"write",
	"create",
	"remove",
	"rename",
	"link",
	"symlink",
	"mkdir",
	"rmdir",
	"readdir",
	"fsstat" };

cn_print(zflag)
	int zflag;
{
	fprintf(stdout, "\nClient nfs:\n");
	fprintf(stdout,
	    "calls      badcalls   nclget     nclsleep\n");
	fprintf(stdout,
	    "%-11d%-11d%-11d%-11d\n",
	    clstat.ncalls,
	    clstat.nbadcalls,
	    clstat.nclgets,
	    clstat.nclsleeps);
	req_print((u_int *)clstat.reqs, clstat.ncalls);
	if (zflag) {
		bzero((char *)&clstat, sizeof clstat);
	}
}

sn_print(zflag)
	int zflag;
{
	fprintf(stdout, "\nServer nfs:\n");
	fprintf(stdout, "calls      badcalls\n");
	fprintf(stdout, "%-11d%-11d\n", svstat.ncalls, svstat.nbadcalls);
	req_print((u_int *)svstat.reqs, svstat.ncalls);
	if (zflag) {
		bzero((char *)&svstat, sizeof svstat);
	}
}

req_print(req, tot)
	u_int	*req;
	u_int	tot;
{
	int	i, j;
	char	fixlen[128];

	for (i = 0; i <= RFS_NPROC / 7; i++) {
		for (j = i * 7; j < min(i * 7 + 7, RFS_NPROC); j++) {
			fprintf(stdout, "%-11s", nfsstr[j]);
		}
		fprintf(stdout, "\n");
		for (j = i * 7; j < min(i * 7 + 7, RFS_NPROC); j++) {
			if (tot) {
				sprintf(fixlen, "%d %2d%% ", req[j],
					(int)((double)req[j] * 100.0/tot));
			} else {
				sprintf(fixlen, "%d 0%% ", req[j]);
			}
			fprintf(stdout, "%-11s", fixlen);
		}
		fprintf(stdout, "\n");
	}
}

/*
 * Print the mount table info
 */
struct vfsrec {
	u_long	vfs_next;
	u_long	vfs_ops;
	u_long	vfs_vnodecovered;
	int	vfs_flag;
	int	vfs_bsize;
	fsid_t	vfs_fsid;
	u_long	vfs_stats;
	u_long	vfs_data;
} vfsrec;

struct mirec {
	int		garbage;	/* really sockaddr_in */
	struct in_addr	mi_addr;
	int		mi_zero[2];
	int		mi_rootvp;
	u_int		 mi_hard : 1;	/* hard or soft mount */
	u_int		 mi_printed : 1; /* not responding message printed */
	u_int		 mi_int : 1;	/* interrupts allowed on hard mount */
	u_int		 mi_down : 1;	/* server is down */
	u_int		 mi_noac : 1;	/* don't cache attributes */
	u_int		 mi_nocto : 1;	/* no close-to-open consistency */
	u_int		 mi_dynamic : 1; /* do dynamic transfer size */
	int		 mi_refct;	/* active vnodes for this vfs */
	long		 mi_tsize;	/* client read size */
	long		 mi_stsize;	/* server transfer size */
	long		 mi_bsize;	/* server's disk block size */
	int		 mi_mntno;	/* kludge to set client rdev for stat */
	int		 mi_timeo;	/* inital timeout in 10th sec */
	int		 mi_retrans;	/* times to retry request */
	char		 mi_hostname[32]; /* server's hostname */
	char		*mi_netname;	/* server's netname */
	int		 mi_netnamelen;	/* length of netname */
	int		 mi_authflavor;	/* authentication type */
	u_int		 mi_acregmin;	/* min secs to hold cached file attr */
	u_int		 mi_acregmax;	/* max secs to hold cached file attr */
	u_int		 mi_acdirmin;	/* min secs to hold cached dir attr */
	u_int		 mi_acdirmax;	/* max secs to hold cached dir attr */
	/*
	 * Extra fields for congestion control, one per NFS call type,
	 * plus a global one.
	 */
	struct rpc_timers {
		u_short	rt_srtt;
		u_short	rt_deviate;
		u_long  rt_rtxcur;
	} mi_timers[4];
	long		 mi_curread;	/* current read size */
	long		 mi_curwrite;	/* current write size */
} mirec;

struct myrec {
	fsid_t my_fsid;
	char my_dir[256];
	char my_path[256];
	struct myrec *next;
	};


mi_print()
{
	u_long vfs;		/* "current" VFS pointer */
	u_long nfs_vfsops; 	/* to determine if this VFS is NFS or not */
	FILE *mntFile;
	struct mntent *m;
	struct myrec *list, *mrp;
	struct statfs buf;
	extern char *malloc(), *inet_ntoa();

	mntFile = setmntent("/etc/mtab", "r");
	if (mntFile == NULL) {
		printf("Unable to open mount table\n");
		exit(0);
	}
	list = NULL;
	while (m = getmntent(mntFile)) {
		if (strcmp(m->mnt_type, MNTTYPE_IGNORE) == 0)
			continue;
		if (strcmp(m->mnt_type, MNTTYPE_SWAP) == 0)
			continue;
		(void) statfs(m->mnt_dir, &buf);
		mrp = (struct myrec *)malloc(sizeof (struct myrec));
		mrp->my_fsid.val[0] = buf.f_fsid.val[0];
		mrp->my_fsid.val[1] = buf.f_fsid.val[1];
		(void)strcpy(mrp->my_dir,  m->mnt_dir);
		(void)strcpy(mrp->my_path,  m->mnt_fsname);
		mrp->next = list;
		list = mrp;
	}
	(void)endmntent(mntFile);

	nfs_vfsops = nl[X_NFS_VFSOPS].n_value;
	kread(X_ROOTVFS, &vfs, sizeof vfs);

	for (; vfs != 0; vfs = vfsrec.vfs_next) {
		if (kvm_read(kd, vfs, (char *)&vfsrec, sizeof vfsrec)
			!= sizeof vfsrec) {
			fprintf(stderr, "nfsstat: kernel read error\n");
			exit(1);
		}
		if (vfsrec.vfs_data == 0)
			continue;
		if (vfsrec.vfs_ops != nfs_vfsops) {
			continue;
		}
		if (kvm_read(kd, vfsrec.vfs_data, (char *)&mirec, sizeof mirec)
			!= sizeof mirec) {
			fprintf(stderr, "nfsstat: kernel read error\n");
			exit(1);
		}
		for (mrp = list; mrp; mrp = mrp->next) {
			if (mrp->my_fsid.val[0] == vfsrec.vfs_fsid.val[0] &&
			    mrp->my_fsid.val[1] == vfsrec.vfs_fsid.val[1]) {
				printf("%s from %s", mrp->my_dir, mrp->my_path);
				break;
			}
		}
		if (mrp == 0)
			printf(" Server %s, ", mirec.mi_hostname);
		printf(" (Addr %s)\n", inet_ntoa(mirec.mi_addr));
		printf("Flags: ");
		if (mirec.mi_hard)
			printf("hard ");
		if (mirec.mi_printed)
			printf("print ");
		if (mirec.mi_int)
			printf("int ");
		if (mirec.mi_down)
			printf("down ");
		if (mirec.mi_noac)
			printf("noac ");
		if (mirec.mi_nocto)
			printf("nocto ");
		if (mirec.mi_dynamic)
			printf("dynamic ");
		printf(" read size=%d, write size=%d, ",
			mirec.mi_curread, mirec.mi_curwrite);
		printf(" count = %d\n", mirec.mi_retrans),

# define srtt_to_ms(x) x, (x * 2 + x/2)
# define  dev_to_ms(x) x, (x * 5)
		printf(" Lookups: srtt=%d (%dms), dev=%d (%dms), cur=%d (%dms)\n",
			srtt_to_ms(mirec.mi_timers[0].rt_srtt),
			dev_to_ms(mirec.mi_timers[0].rt_deviate),
			mirec.mi_timers[0].rt_rtxcur,
			mirec.mi_timers[0].rt_rtxcur*20 );
		printf(" Reads: srtt=%d (%dms), dev=%d (%dms), cur=%d (%dms)\n",
			srtt_to_ms(mirec.mi_timers[1].rt_srtt),
			dev_to_ms(mirec.mi_timers[1].rt_deviate),
			mirec.mi_timers[1].rt_rtxcur,
			mirec.mi_timers[1].rt_rtxcur*20 );
		printf(" Writes: srtt=%d (%dms), dev=%d (%dms), cur=%d (%dms)\n",
			srtt_to_ms(mirec.mi_timers[2].rt_srtt),
			dev_to_ms(mirec.mi_timers[2].rt_deviate),
			mirec.mi_timers[2].rt_rtxcur,
			mirec.mi_timers[2].rt_rtxcur*20 );
		printf(" All: srtt=%d (%dms), dev=%d (%dms), cur=%d (%dms)\n",
			srtt_to_ms(mirec.mi_timers[3].rt_srtt),
			dev_to_ms(mirec.mi_timers[3].rt_deviate),
			mirec.mi_timers[3].rt_rtxcur,
			mirec.mi_timers[3].rt_rtxcur*20 );
		printf("\n");
	}
}

usage()
{
	fprintf(stderr, "nfsstat [-cnrsmz] [vmunix] [core]\n");
	exit(1);
}

min(a, b)
	int a, b;
{
	return (a < b) ? a : b;
}
