#ifndef lint
static	char *sccsid = "@(#)edquota.c 1.1 92/07/30 SMI"; /* from UCB 4.5 */
#endif

/*
 * Disk quota editor.
 */
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <pwd.h>
#include <ctype.h>
#include <mntent.h>

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <ufs/quota.h>

#define	DEFEDITOR	"/usr/ucb/vi"

struct fsquot {
	struct fsquot *fsq_next;
	struct dqblk fsq_dqb;
	char *fsq_fs;
	char *fsq_dev;
	char *fsq_qfile;
};

struct fsquot *fsqlist;

char	tmpfil[] = "/tmp/EdP.aXXXXX";
char	*qfname = "quotas";

main(argc, argv)
	int argc;
	char **argv;
{
	int uid;
	char *arg0;

	arg0 = *argv++;
	if (argc < 2) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "\t%s [-p username] username ...\n", arg0);
		fprintf(stderr, "\t%s -t\n", arg0);
		exit(1);
	}
	--argc;
	if (quotactl(Q_SYNC, NULL, 0, NULL) < 0 && errno == EINVAL) {
		printf("Warning: Quotas are not compiled into this kernel\n");
		sleep(3);
	}
	if (getuid()) {
		fprintf(stderr, "%s: permission denied\n", arg0);
		exit(1);
	}
	setupfs();
	if (fsqlist == NULL) {
		fprintf(stderr, "%s: no 4.2 filesystems with %s file\n",
		    MOUNTED, qfname);
		exit(1);
	}
	mktemp(tmpfil);
	close(creat(tmpfil, 0600));
	(void) chown(tmpfil, getuid(), getgid());
	if (strcmp(*argv, "-t") == 0) {
		gettimes(0);
		if (editit())
			puttimes(0);
		(void) unlink(tmpfil);
		exit(0);
	}
	if (argc > 2 && strcmp(*argv, "-p") == 0) {
		argc--, argv++;
		uid = getentry(*argv++);
		if (uid < 0) {
			(void) unlink(tmpfil);
			exit(1);
		}
		getprivs(uid);
		argc--;
		while (argc-- > 0) {
			uid = getentry(*argv++);
			if (uid < 0)
				continue;
			getdiscq(uid);
			putprivs(uid);
		}
		(void) unlink(tmpfil);
		exit(0);
	}
	while (--argc >= 0) {
		uid = getentry(*argv++);
		if (uid < 0)
			continue;
		getprivs(uid);
		if (editit())
			putprivs(uid);
	}
	(void) unlink(tmpfil);
	exit(0);
	/* NOTREACHED */
}

getentry(name)
	char *name;
{
	struct passwd *pw;
	int uid;

	if (alldigits(name))
		uid = atoi(name);
	else if (pw = getpwnam(name))
		uid = pw->pw_uid;
	else {
		fprintf(stderr, "%s: no such user\n", name);
		sleep(1);
		return (-1);
	}
	return (uid);
}

editit()
{
	register pid, xpid;
	int stat, omask;
	extern char *getenv();

#define	mask(s)	(1<<((s)-1))
	omask = sigblock(mask(SIGINT)|mask(SIGQUIT)|mask(SIGHUP));
 top:
	if ((pid = fork()) < 0) {
		extern errno;

		if (errno == EPROCLIM) {
			fprintf(stderr, "You have too many processes\n");
			return(0);
		}
		if (errno == EAGAIN) {
			sleep(1);
			goto top;
		}
		perror("fork");
		return (0);
	}
	if (pid == 0) {
		register char *ed;

		(void) sigsetmask(omask);
		(void) setgid(getgid());
		(void) setuid(getuid());
		if ((ed = getenv("EDITOR")) == (char *)0)
			ed = DEFEDITOR;
		execlp(ed, ed, tmpfil, 0);
		perror(ed);
		exit(1);
	}
	while ((xpid = wait(&stat)) >= 0)
		if (xpid == pid)
			break;
	(void) sigsetmask(omask);
	return (!stat);
}

getprivs(uid)
	register int uid;
{
	register struct fsquot *fsqp;
	FILE *fd;

	getdiscq(uid);
	if ((fd = fopen(tmpfil, "w")) == NULL) {
		fprintf(stderr, "edquota: ");
		perror(tmpfil);
		exit(1);
	}
	for (fsqp = fsqlist; fsqp; fsqp = fsqp->fsq_next) {
		fprintf(fd,
"fs %s blocks (soft = %ld, hard = %ld) inodes (soft = %ld, hard = %ld)\n"
			, fsqp->fsq_fs
			, dbtob(fsqp->fsq_dqb.dqb_bsoftlimit) / 1024
			, dbtob(fsqp->fsq_dqb.dqb_bhardlimit) / 1024
			, fsqp->fsq_dqb.dqb_fsoftlimit
			, fsqp->fsq_dqb.dqb_fhardlimit
		);
	}
	fclose(fd);
}

putprivs(uid)
	register int uid;
{
	FILE *fd;
	struct dqblk ndqb;
	char line[BUFSIZ];
	int changed = 0;

	fd = fopen(tmpfil, "r");
	if (fd == NULL) {
		fprintf(stderr, "Can't re-read temp file!!\n");
		return;
	}
	while (fgets(line, sizeof(line), fd) != NULL) {
		register struct fsquot *fsqp;
		char *cp, *dp, *next();
		int n;

		cp = next(line, " \t");
		if (cp == NULL)
			break;
		*cp++ = '\0';
		while (*cp && *cp == '\t' && *cp == ' ')
			cp++;
		dp = cp, cp = next(cp, " \t");
		if (cp == NULL)
			break;
		*cp++ = '\0';
		for (fsqp = fsqlist; fsqp; fsqp = fsqp->fsq_next) {
			if (strcmp(dp, fsqp->fsq_fs) == 0)
				break;
		}
		if (fsqp == NULL) {
			fprintf(stderr, "%s: unknown file system\n", cp);
			continue;
		}
		while (*cp && *cp == '\t' && *cp == ' ')
			cp++;
		n = sscanf(cp,
"blocks (soft = %ld, hard = %ld) inodes (soft = %ld, hard = %ld)\n"
			, &ndqb.dqb_bsoftlimit
			, &ndqb.dqb_bhardlimit
			, &ndqb.dqb_fsoftlimit
			, &ndqb.dqb_fhardlimit
		);
		if (n != 4) {
			fprintf(stderr, "%s: bad format\n", cp);
			continue;
		}
		changed++;
		ndqb.dqb_bsoftlimit = btodb(ndqb.dqb_bsoftlimit * 1024);
		ndqb.dqb_bhardlimit = btodb(ndqb.dqb_bhardlimit * 1024);
		/*
		 * It we are decreasing the soft limits, set the time limits
		 * to zero, in case the user is now over quota.
		 * the time limit will be started the next time the
		 * user does an allocation.
		 */
		if (ndqb.dqb_bsoftlimit < fsqp->fsq_dqb.dqb_bsoftlimit)
			fsqp->fsq_dqb.dqb_btimelimit = 0;
		if (ndqb.dqb_fsoftlimit < fsqp->fsq_dqb.dqb_fsoftlimit)
			fsqp->fsq_dqb.dqb_ftimelimit = 0;
		fsqp->fsq_dqb.dqb_bsoftlimit = ndqb.dqb_bsoftlimit;
		fsqp->fsq_dqb.dqb_bhardlimit = ndqb.dqb_bhardlimit;
		fsqp->fsq_dqb.dqb_fsoftlimit = ndqb.dqb_fsoftlimit;
		fsqp->fsq_dqb.dqb_fhardlimit = ndqb.dqb_fhardlimit;
	}
	fclose(fd);
	if (changed)
		putdiscq(uid);
}

gettimes(uid)
	register int uid;
{
	register struct fsquot *fsqp;
	FILE *fd;
	char btime[80], ftime[80];

	getdiscq(uid);
	if ((fd = fopen(tmpfil, "w")) == NULL) {
		fprintf(stderr, "edquota: ");
		perror(tmpfil);
		exit(1);
	}
	for (fsqp = fsqlist; fsqp; fsqp = fsqp->fsq_next) {
		fmttime(btime, fsqp->fsq_dqb.dqb_btimelimit);
		fmttime(ftime, fsqp->fsq_dqb.dqb_ftimelimit);
		fprintf(fd,
"fs %s blocks time limit = %s, files time limit = %s\n"
			, fsqp->fsq_fs
			, btime
			, ftime
		);
	}
	fclose(fd);
}

puttimes(uid)
	register int uid;
{
	FILE *fd;
	char line[BUFSIZ];
	int changed = 0;
	double btimelimit, ftimelimit;
	char bunits[80], funits[80];

	fd = fopen(tmpfil, "r");
	if (fd == NULL) {
		fprintf(stderr, "Can't re-read temp file!!\n");
		return;
	}
	while (fgets(line, sizeof(line), fd) != NULL) {
		register struct fsquot *fsqp;
		char *cp, *dp, *next();
		int n;

		cp = next(line, " \t");
		if (cp == NULL)
			break;
		*cp++ = '\0';
		while (*cp && *cp == '\t' && *cp == ' ')
			cp++;
		dp = cp, cp = next(cp, " \t");
		if (cp == NULL)
			break;
		*cp++ = '\0';
		for (fsqp = fsqlist; fsqp; fsqp = fsqp->fsq_next) {
			if (strcmp(dp, fsqp->fsq_fs) == 0)
				break;
		}
		if (fsqp == NULL) {
			fprintf(stderr, "%s: unknown file system\n", cp);
			continue;
		}
		while (*cp && *cp == '\t' && *cp == ' ')
			cp++;
		n = sscanf(cp,
"blocks time limit = %lf %[^,], files time limit = %lf %s\n"
			, &btimelimit
			, bunits
			, &ftimelimit
			, funits
		);
		if (n != 4 ||
		    !unfmttime(btimelimit, bunits,
		      &fsqp->fsq_dqb.dqb_btimelimit) ||
		    !unfmttime(ftimelimit, funits,
		      &fsqp->fsq_dqb.dqb_ftimelimit)) {
			fprintf(stderr, "%s: bad format\n", cp);
			continue;
		}
		changed++;
	}
	fclose(fd);
	if (changed)
		putdiscq(uid);
}

char *
next(cp, match)
	register char *cp;
	char *match;
{
	register char *dp;

	while (cp && *cp) {
		for (dp = match; dp && *dp; dp++)
			if (*dp == *cp)
				return (cp);
		cp++;
	}
	return ((char *)0);
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

static struct {
	int c_secs;			/* conversion units in secs */
	char * c_str;			/* unit string */
} cunits [] = {
	{60*60*24*28, "month"},
	{60*60*24*7, "week"},
	{60*60*24, "day"},
	{60*60, "hour"},
	{60, "min"},
	{1, "sec"}
};

fmttime(buf, time)
	char *buf;
	register u_long time;
{
	double value;
	int i;

	if (time == 0) {
		strcpy(buf, "0 (default)");
		return;
	}
	for (i = 0; i < sizeof(cunits)/sizeof(cunits[0]); i++) {
		if (time >= cunits[i].c_secs)
			break;
	}
	value = (double)time / cunits[i].c_secs;
	sprintf(buf, "%.2f %s%s", value, cunits[i].c_str, value > 1.0? "s": "");
}

int
unfmttime(value, units, timep)
	double value;
	char *units;
	u_long *timep;
{
	int i;

	if (value == 0.0) {
		*timep = 0;
		return (1);
	}
	for (i = 0; i < sizeof(cunits)/sizeof(cunits[0]); i++) {
		if (strncmp(cunits[i].c_str,units,strlen(cunits[i].c_str)) == 0)
			break;
	}
	if (i >= sizeof(cunits)/sizeof(cunits[0]))
		return (0);
	*timep = (u_long)(value * cunits[i].c_secs);
	return (1);
}

setupfs()
{
	register struct mntent *mntp;
	register struct fsquot *fsqp;
	struct stat statb;
	dev_t fsdev;
	FILE *mtab;
	char qfilename[MAXPATHLEN + 1];
	extern int errno;
	extern char *malloc();

	mtab = setmntent(MOUNTED, "r");
	while (mntp = getmntent(mtab)) {
		if (strcmp(mntp->mnt_type, MNTTYPE_42) != 0)
			continue;
		if (stat(mntp->mnt_fsname, &statb) < 0)
			continue;
		if ((statb.st_mode & S_IFMT) != S_IFBLK)
			continue;
		fsdev = statb.st_rdev;
		sprintf(qfilename, "%s/%s", mntp->mnt_dir, qfname);
		if (stat(qfilename, &statb) < 0 || statb.st_dev != fsdev)
			continue;
		fsqp = (struct fsquot *)malloc(sizeof(struct fsquot));
		if (fsqp == NULL) {
			fprintf(stderr, "out of memory\n");
			exit (1);
		}
		fsqp->fsq_next = fsqlist;
		fsqp->fsq_fs = malloc(strlen(mntp->mnt_dir) + 1);
		fsqp->fsq_dev = malloc(strlen(mntp->mnt_fsname) + 1);
		fsqp->fsq_qfile = malloc(strlen(qfilename) + 1);
		if (fsqp->fsq_fs == NULL || fsqp->fsq_dev == NULL ||
		    fsqp->fsq_qfile == NULL) {
			fprintf(stderr, "out of memory\n");
			exit (1);
		}
		strcpy(fsqp->fsq_fs, mntp->mnt_dir);
		strcpy(fsqp->fsq_dev, mntp->mnt_fsname);
		strcpy(fsqp->fsq_qfile, qfilename);
		fsqlist = fsqp;
	}
	endmntent(mtab);
}

getdiscq(uid)
	register uid;
{
	register struct fsquot *fsqp;
	int fd;

	for (fsqp = fsqlist; fsqp; fsqp = fsqp->fsq_next) {
		if (quotactl(Q_GETQUOTA,fsqp->fsq_dev,uid,&fsqp->fsq_dqb)!=0) {
			if ((fd = open(fsqp->fsq_qfile, O_RDONLY)) < 0) {
				fprintf(stderr, "edquota: ");
				perror(fsqp->fsq_qfile);
				continue;
			}
			(void) lseek(fd, (long)dqoff(uid), L_SET);
			switch (read(fd, &fsqp->fsq_dqb, sizeof(struct dqblk))){
			case 0:
				/*
				 * Convert implicit 0 quota (EOF)
				 * into an explicit one (zero'ed dqblk)
				 */
				bzero((caddr_t)&fsqp->fsq_dqb,
				    sizeof (struct dqblk));
				break;

			case sizeof (struct dqblk):	/* OK */
				break;

			default:			/* ERROR */
				fprintf(stderr, "edquota: read error in ");
				perror(fsqp->fsq_qfile);
				break;
			}
			close(fd);
		}
	}
}

putdiscq(uid)
	register uid;
{
	register struct fsquot *fsqp;

	for (fsqp = fsqlist; fsqp; fsqp = fsqp->fsq_next) {
		if (quotactl(Q_SETQLIM,fsqp->fsq_dev,uid,&fsqp->fsq_dqb) != 0) {
			register int fd;

			if ((fd = open(fsqp->fsq_qfile, O_RDWR)) < 0) {
				fprintf(stderr, "edquota: ");
				perror(fsqp->fsq_qfile);
				continue;
			}
			(void) lseek(fd, (long)dqoff(uid), L_SET);
			if (write(fd, &fsqp->fsq_dqb, sizeof(struct dqblk)) !=
			    sizeof(struct dqblk)) {
				fprintf(stderr, "edquota: ");
				perror(fsqp->fsq_qfile);
			}
			close(fd);
		}
	}
}
