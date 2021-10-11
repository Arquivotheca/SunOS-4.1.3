#ifndef lint
static	char sccsid[] = "@(#)quota.c 1.1 92/07/30 SMI"; /* from UCB 4.4 06/21/83 */
#endif

/*
 * Disk quota reporting program.
 */
#include <stdio.h>
#include <mntent.h>
#include <ctype.h>
#include <pwd.h>
#include <errno.h>

#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ufs/quota.h>

int	vflag;
int	nolocalquota;

#define QFNAME	"quotas"

#define kb(n)   (howmany(dbtob(n), 1024))

main(argc, argv)
	int argc;
	char *argv[];
{
	register char *cp;

	argc--,argv++;
	while (argc > 0) {
		if (argv[0][0] == '-')
			for (cp = &argv[0][1]; *cp; cp++) switch (*cp) {

			case 'v':
				vflag++;
				break;

			default:
				fprintf(stderr, "quota: %c: unknown option\n",
					*cp);
				exit(1);
			}
		else
			break;
		argc--, argv++;
	}
	if (quotactl(Q_SYNC, NULL, 0, NULL) < 0 && errno == EINVAL) {
		if (vflag)
			fprintf(stderr,"There are no quotas on this system\n");
		nolocalquota++;
	}
	if (argc == 0) {
		showuid(getuid());
		exit(0);
	}
	for (; argc > 0; argc--, argv++) {
		if (alldigits(*argv))
			showuid(atoi(*argv));
		else
			showname(*argv);
	}
	exit(0);
	/* NOTREACHED */
}

showuid(uid)
	int uid;
{
	struct passwd *pwd = getpwuid(uid);

	if (uid == 0) {
		if (vflag)
			printf("no disk quota for uid 0\n");
		return;
	}
	if (pwd == NULL)
		showquotas(uid, "(no account)");
	else
		showquotas(uid, pwd->pw_name);
}

showname(name)
	char *name;
{
	struct passwd *pwd = getpwnam(name);

	if (pwd == NULL) {
		fprintf(stderr, "quota: %s: unknown user\n", name);
		return;
	}
	if (pwd->pw_uid == 0) {
		if (vflag)
			printf("no disk quota for %s (uid 0)\n", name);
		return;
	}
	showquotas(pwd->pw_uid, name);
}

showquotas(uid, name)
	int uid;
	char *name;
{
	register struct mntent *mntp;
	FILE *mtab;
	struct dqblk dqblk;
	int myuid;

	myuid = getuid();
	if (uid != myuid && myuid != 0) {
		printf("quota: %s (uid %d): permission denied\n", name, uid);
		return;
	}
	if (vflag)
		heading(uid, name);
	mtab = setmntent(MOUNTED, "r");
	while (mntp = getmntent(mtab)) {
		if (strcmp(mntp->mnt_type, MNTTYPE_42) == 0) {
			if (nolocalquota ||
			    (quotactl(Q_GETQUOTA,
			      mntp->mnt_fsname, uid, &dqblk) != 0 &&
			     !(vflag && getdiskquota(mntp, uid, &dqblk))) )
					continue;
		} else if (strcmp(mntp->mnt_type, MNTTYPE_NFS) == 0) {
			if ((!vflag && hasmntopt(mntp, MNTOPT_NOQUOTA)) ||
			    !getnfsquota(mntp, uid, &dqblk))
				continue;
		} else {
			continue;
		}
		if (dqblk.dqb_bsoftlimit == 0 && dqblk.dqb_bhardlimit == 0 &&
		    dqblk.dqb_fsoftlimit == 0 && dqblk.dqb_fhardlimit == 0)
			continue;
		if (vflag)
			prquota(mntp, &dqblk);
		else
			warn(mntp, &dqblk);
	}
	endmntent(mtab);
}

warn(mntp, dqp)
	register struct mntent *mntp;
	register struct dqblk *dqp;
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	if (dqp->dqb_bhardlimit &&
	     dqp->dqb_curblocks >= dqp->dqb_bhardlimit) {
		printf(
"Block limit reached on %s\n",
		    mntp->mnt_dir
		);
	} else if (dqp->dqb_bsoftlimit &&
	     dqp->dqb_curblocks >= dqp->dqb_bsoftlimit) {
		if (dqp->dqb_btimelimit == 0) {
			printf(
"Over disk quota on %s, remove %dK\n",
			    mntp->mnt_dir,
			    kb(dqp->dqb_curblocks - dqp->dqb_bsoftlimit + 1)
			);
		} else if (dqp->dqb_btimelimit > tv.tv_sec) {
			char btimeleft[80];

			fmttime(btimeleft, dqp->dqb_btimelimit - tv.tv_sec);
			printf(
"Over disk quota on %s, remove %dK within %s\n",
			    mntp->mnt_dir,
			    kb(dqp->dqb_curblocks - dqp->dqb_bsoftlimit + 1),
			    btimeleft
			);
		} else {
			printf(
"Over disk quota on %s, time limit has expired, remove %dK\n",
			    mntp->mnt_dir,
			    kb(dqp->dqb_curblocks - dqp->dqb_bsoftlimit + 1)
			);
		}
	}
	if (dqp->dqb_fhardlimit &&
	    dqp->dqb_curfiles >= dqp->dqb_fhardlimit) {
		printf(
"File count limit reached on %s\n",
		    mntp->mnt_dir
		);
	} else if (dqp->dqb_fsoftlimit &&
	    dqp->dqb_curfiles >= dqp->dqb_fsoftlimit) {
		if (dqp->dqb_ftimelimit == 0) {
			printf(
"Over file quota on %s, remove %d file%s\n",
			    mntp->mnt_dir,
			    dqp->dqb_curfiles - dqp->dqb_fsoftlimit + 1,
			    ((dqp->dqb_curfiles - dqp->dqb_fsoftlimit + 1) > 1 ?
				"s" :
				"" )
			);
		} else if (dqp->dqb_ftimelimit > tv.tv_sec) {
			char ftimeleft[80];

			fmttime(ftimeleft, dqp->dqb_ftimelimit - tv.tv_sec);
			printf(
"Over file quota on %s, remove %d file%s within %s\n",
			    mntp->mnt_dir,
			    dqp->dqb_curfiles - dqp->dqb_fsoftlimit + 1,
			    ((dqp->dqb_curfiles - dqp->dqb_fsoftlimit + 1) > 1 ?
				"s" :
				"" ),
			    ftimeleft
			);
		} else {
			printf(
"Over file quota on %s, time limit has expired, remove %d file%s\n",
			    mntp->mnt_dir,
			    dqp->dqb_curfiles - dqp->dqb_fsoftlimit + 1,
			    ((dqp->dqb_curfiles - dqp->dqb_fsoftlimit + 1) > 1 ?
				"s" :
				"" )
			);
		}
	}
}

heading(uid, name)
	int uid;
	char *name;
{
	printf("Disk quotas for %s (uid %d):\n", name, uid);
	printf("%-12s %7s%7s%7s%12s%7s%7s%7s%12s\n"
		, "Filesystem"
		, "usage"
		, "quota"
		, "limit"
		, "timeleft"
		, "files"
		, "quota"
		, "limit"
		, "timeleft"
	);
}

prquota(mntp, dqp)
	register struct mntent *mntp;
	register struct dqblk *dqp;
{
	struct timeval tv;
	char ftimeleft[80], btimeleft[80];
	char *cp;

	gettimeofday(&tv, NULL);
	if (dqp->dqb_bsoftlimit && dqp->dqb_curblocks >= dqp->dqb_bsoftlimit) {
		if (dqp->dqb_btimelimit == 0) {
			strcpy(btimeleft, "NOT STARTED");
		} else if (dqp->dqb_btimelimit > tv.tv_sec) {
			fmttime(btimeleft, dqp->dqb_btimelimit - tv.tv_sec);
		} else {
			strcpy(btimeleft, "EXPIRED");
		}
	} else {
		btimeleft[0] = '\0';
	}
	if (dqp->dqb_fsoftlimit && dqp->dqb_curfiles >= dqp->dqb_fsoftlimit) {
		if (dqp->dqb_ftimelimit == 0) {
			strcpy(ftimeleft, "NOT STARTED");
		} else if (dqp->dqb_ftimelimit > tv.tv_sec) {
			fmttime(ftimeleft, dqp->dqb_ftimelimit - tv.tv_sec);
		} else {
			strcpy(ftimeleft, "EXPIRED");
		}
	} else {
		ftimeleft[0] = '\0';
	}
	if (strlen(mntp->mnt_dir) > 12) {
		printf("%s\n", mntp->mnt_dir);
		cp = "";
	} else {
		cp = mntp->mnt_dir;
	}
	printf("%-12.12s %7d%7d%7d%12s%7d%7d%7d%12s\n",
	    cp,
	    kb(dqp->dqb_curblocks),
	    kb(dqp->dqb_bsoftlimit),
	    kb(dqp->dqb_bhardlimit),
	    btimeleft,
	    dqp->dqb_curfiles,
	    dqp->dqb_fsoftlimit,
	    dqp->dqb_fhardlimit,
	    ftimeleft
	);
}

fmttime(buf, time)
	char *buf;
	register long time;
{
	int i;
	static struct {
		int c_secs;		/* conversion units in secs */
		char * c_str;		/* unit string */
	} cunits [] = {
		{60*60*24*28, "months"},
		{60*60*24*7, "weeks"},
		{60*60*24, "days"},
		{60*60, "hours"},
		{60, "mins"},
		{1, "secs"}
	};

	if (time <= 0) {
		strcpy(buf, "EXPIRED");
		return;
	}
	for (i = 0; i < sizeof(cunits)/sizeof(cunits[0]); i++) {
		if (time >= cunits[i].c_secs)
			break;
	}
	sprintf(buf, "%.1f %s", (double)time/cunits[i].c_secs, cunits[i].c_str);
}

alldigits(s)
	register char *s;
{
	register c;

	c = *s++;
	do {
		if (!isdigit(c))
			return (0);
	} while (c = *s++);
	return (1);
}

int
getdiskquota(mntp, uid, dqp)
	struct mntent *mntp;
	int uid;
	struct dqblk *dqp;
{
	int fd;
	dev_t fsdev;
	struct stat statb;
	char qfilename[MAXPATHLEN];
	extern int errno;

	if (stat(mntp->mnt_fsname, &statb) < 0 ||
	    (statb.st_mode & S_IFMT) != S_IFBLK)
		return (0);
	fsdev = statb.st_rdev;
	sprintf(qfilename, "%s/%s", mntp->mnt_dir, QFNAME);
	if (stat(qfilename, &statb) < 0 || statb.st_dev != fsdev)
		return (0);
	if ((fd = open(qfilename, O_RDONLY)) < 0)
		return (0);
	(void) lseek(fd, (long)dqoff(uid), L_SET);
	switch (read(fd, dqp, sizeof(struct dqblk))) {
	case 0:				/* EOF */
		/*
		 * Convert implicit 0 quota (EOF)
		 * into an explicit one (zero'ed dqblk).
		 */
		bzero((caddr_t)dqp, sizeof(struct dqblk));
		break;

	case sizeof(struct dqblk):	/* OK */
		break;

	default:			/* ERROR */
		close(fd);
		return (0);
	}
	close(fd);
	return (1);
}

#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <sys/socket.h>
#include <netdb.h>
#include <rpcsvc/rquota.h>

int
getnfsquota(mntp, uid, dqp)
	struct mntent *mntp;
	int uid;
	struct dqblk *dqp;
{
	char *hostp;
	char *cp;
	struct getquota_args gq_args;
	struct getquota_rslt gq_rslt;
	extern char *index();

	hostp = mntp->mnt_fsname;
	cp = index(mntp->mnt_fsname, ':');
	if (cp == 0) {
		fprintf(stderr, "cannot find hostname for %s\n", mntp->mnt_dir);
		return (0);
	}
	*cp = '\0';
	gq_args.gqa_pathp = cp + 1;
	gq_args.gqa_uid = uid;
	if (callaurpc(hostp, RQUOTAPROG, RQUOTAVERS,
	    (vflag? RQUOTAPROC_GETQUOTA: RQUOTAPROC_GETACTIVEQUOTA),
	    xdr_getquota_args, &gq_args, xdr_getquota_rslt, &gq_rslt) != 0) {
		*cp = ':';
		return (0);
	}
	switch (gq_rslt.gqr_status) {
	case Q_OK:
		{
		struct timeval tv;

		if (!vflag && gq_rslt.gqr_rquota.rq_active == FALSE)
			return (0);
		gettimeofday(&tv, NULL);
		dqp->dqb_bhardlimit =
		    gq_rslt.gqr_rquota.rq_bhardlimit *
		    gq_rslt.gqr_rquota.rq_bsize / DEV_BSIZE;
		dqp->dqb_bsoftlimit =
		    gq_rslt.gqr_rquota.rq_bsoftlimit *
		    gq_rslt.gqr_rquota.rq_bsize / DEV_BSIZE;
		dqp->dqb_curblocks =
		    gq_rslt.gqr_rquota.rq_curblocks *
		    gq_rslt.gqr_rquota.rq_bsize / DEV_BSIZE;
		dqp->dqb_fhardlimit = gq_rslt.gqr_rquota.rq_fhardlimit;
		dqp->dqb_fsoftlimit = gq_rslt.gqr_rquota.rq_fsoftlimit;
		dqp->dqb_curfiles = gq_rslt.gqr_rquota.rq_curfiles;
		dqp->dqb_btimelimit =
		    tv.tv_sec + gq_rslt.gqr_rquota.rq_btimeleft;
		dqp->dqb_ftimelimit =
		    tv.tv_sec + gq_rslt.gqr_rquota.rq_ftimeleft;
		*cp = ':';
		return (1);
		}

	case Q_NOQUOTA:
		break;

	case Q_EPERM:
		fprintf(stderr, "quota permission error, host: %s\n", hostp);
		break;

	default:
		fprintf(stderr, "bad rpc result, host: %s\n",  hostp);
		break;
	}
	*cp = ':';
	return (0);
}

callaurpc(host, prognum, versnum, procnum, inproc, in, outproc, out)
	char *host;
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct hostent *hp;
	struct timeval timeout, tottimeout;

	static CLIENT *client = NULL;
	static int socket = RPC_ANYSOCK;
	static int valid = 0;
	static int oldprognum, oldversnum;
	static char oldhost[256];

	if (valid && oldprognum == prognum && oldversnum == versnum
		&& strcmp(oldhost, host) == 0) {
		/* reuse old client */		
	}
	else {
		valid = 0;
		close(socket);
		socket = RPC_ANYSOCK;
		if (client) {
			clnt_destroy(client);
			client = NULL;
		}
		if ((hp = gethostbyname(host)) == NULL)
			return ((int) RPC_UNKNOWNHOST);
		timeout.tv_usec = 0;
		timeout.tv_sec = 6;
		bcopy(hp->h_addr, &server_addr.sin_addr, hp->h_length);
		server_addr.sin_family = AF_INET;
		/* ping the remote end via tcp to see if it is up */
		server_addr.sin_port =  htons(PMAPPORT);
		if ((client = clnttcp_create(&server_addr, PMAPPROG,
		    PMAPVERS, &socket, 0, 0)) == NULL) {
			return ((int) rpc_createerr.cf_stat);
		} else {
			/* the fact we succeeded means the machine is up */
			close(socket);
			socket = RPC_ANYSOCK;
			clnt_destroy(client);
			client = NULL;
		}
		/* now really create a udp client handle */
		server_addr.sin_port =  0;
		if ((client = clntudp_create(&server_addr, prognum,
		    versnum, timeout, &socket)) == NULL)
			return ((int) rpc_createerr.cf_stat);
		client->cl_auth = authunix_create_default();
		valid = 1;
		oldprognum = prognum;
		oldversnum = versnum;
		strcpy(oldhost, host);
	}
	tottimeout.tv_sec = 25;
	tottimeout.tv_usec = 0;
	clnt_stat = clnt_call(client, procnum, inproc, in,
	    outproc, out, tottimeout);
	/* 
	 * if call failed, empty cache
	 */
	if (clnt_stat != RPC_SUCCESS)
		valid = 0;
	return ((int) clnt_stat);
}
