#ifndef lint
static  char sccsid[] = "@(#)auto_all.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <mntent.h>
#include <netdb.h>
#include <errno.h>
#include "nfs_prot.h"
#define NFSCLIENT
typedef nfs_fh fhandle_t;
#include <rpcsvc/mount.h>
#include <sys/mount.h>
#include "automount.h"

char *optlist[] = {
	MNTOPT_RO,	MNTOPT_RW,	MNTOPT_GRPID,	MNTOPT_SOFT,
	MNTOPT_HARD,	MNTOPT_NOSUID,	MNTOPT_INTR,	MNTOPT_SECURE,
	MNTOPT_NOAC,	MNTOPT_NOCTO,	MNTOPT_PORT,	MNTOPT_RETRANS,
	MNTOPT_RSIZE,	MNTOPT_WSIZE,	MNTOPT_TIMEO,	MNTOPT_ACTIMEO,
	MNTOPT_ACREGMIN,MNTOPT_ACREGMAX,MNTOPT_ACDIRMIN,MNTOPT_ACDIRMAX,
	MNTOPT_QUOTA,	MNTOPT_NOQUOTA,
#ifdef  MNTOPT_POSIX
	MNTOPT_POSIX,
#endif
	"suid",
	NULL
};

char *
opt_check(opts)
	char *opts;
{
	static char buf [256];
	register char *p, *pe, *pb;
	register char **q;

	if (strlen(opts) > 254)
		return (NULL);
	(void) strcpy(buf, opts);
	pb = buf;

	while (p = strtok(pb, ",")) {
		pb = NULL;
		if (pe = strchr(p, '='))
			*pe = '\0';
		for (q = optlist ; *q ; q++) {
			if (strcmp(p, *q) == 0)
				break;
		}
		if (*q == NULL)
			return (p);
	}
	return (NULL);
}

do_unmount(fsys)
	struct filsys *fsys;
{
	struct queue tmpq;
	struct filsys *fs, *nextfs, *rootfs;
	nfsstat remount();
	extern int trace;
	dev_t olddev;
	int newdevs;

	tmpq.q_head = tmpq.q_tail = NULL;
	for (fs = HEAD(struct filsys, fs_q); fs; fs = nextfs) {
		nextfs = NEXT(struct filsys, fs);
		if (fs->fs_rootfs == fsys) {
			REMQUE(fs_q, fs);
			INSQUE(tmpq, fs);
		}
	}
	/* walk backwards trying to unmount */
	for (fs = TAIL(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = PREV(struct filsys, fs);
		if (fs->fs_unmounted)
			continue;
		if (trace > 1)
			fprintf(stderr, "unmount %s\n", fs->fs_mntpnt);
		if (!pathok(tmpq, fs) || unmount(fs->fs_mntpnt) < 0) {
			if (trace > 1)
				fprintf(stderr, "unmount %s: BUSY\n", fs->fs_mntpnt);
			goto inuse;
		}
		fs->fs_unmounted = 1;
		if (trace > 1)
			fprintf(stderr, "unmount %s: OK\n", fs->fs_mntpnt);
	}
	/* all ok - walk backwards removing directories */
	clean_mtab(0, HEAD(struct filsys, tmpq));
	for (fs = TAIL(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = PREV(struct filsys, fs);
		nfsunmount(fs);
		safe_rmdir(fs->fs_mntpnt);
		REMQUE(tmpq, fs);
		INSQUE(fs_q, fs);
		free_filsys(fs);
	}
	/* success */
	return (1);

inuse:
	/* remount previous unmounted ones */
	newdevs = 0;
	for (fs = HEAD(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = NEXT(struct filsys, fs);
		if (fs->fs_unmounted) {
			olddev = fs->fs_mountdev;
			if (remount(fs) == NFS_OK) {
				fs->fs_unmounted = 0;
				if (fs->fs_mountdev != olddev)
					newdevs++;
			}
		}
	}

	if (newdevs) {/* remove entries with old dev ids */
		clean_mtab(0, HEAD(struct filsys, tmpq));
		rootfs = HEAD(struct filsys, tmpq)->fs_rootfs;
	}

	/* put things back on the correct list */
	for (fs = HEAD(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = NEXT(struct filsys, fs);
		REMQUE(tmpq, fs);
		INSQUE(fs_q, fs);
	}
	if (newdevs) /* put updated entries back */
		addtomtab(rootfs);

	return (0);
}

/*
 * Check a path prior to using it.  This avoids hangups in
 * the unmount system call from dead mount points in the
 * path to the mount point we're trying to unmount.
 * If all the mount points ping OK then return 1.
 */
pathok(tmpq, tfs)
	struct queue tmpq;
	struct filsys *tfs;
{
	struct filsys *fs, *nextfs;
	extern dev_t tmpdev;
	enum clnt_stat pingmount();


	while (tfs->fs_mntpntdev != tmpdev && tfs->fs_rootfs != tfs) {
		for (fs = HEAD(struct filsys, tmpq); fs; fs = nextfs) {
			nextfs = NEXT(struct filsys, fs);
			if (tfs->fs_mntpntdev == fs->fs_mountdev)
				break;
		}
		if (fs == NULL) {
			syslog(LOG_ERR,
				"pathok: couldn't find devid %04x(%04x) for %s",
				tfs->fs_mntpntdev & 0xFFFF,
				tmpdev & 0xFFFF,
				tfs->fs_mntpnt);
			return (1);
		}
		if (trace > 1)
			fprintf(stderr, "pathok: %s\n", fs->fs_mntpnt);
		if (pingmount(fs->fs_addr.sin_addr) != RPC_SUCCESS) {
			if (trace > 1)
				fprintf(stderr, "pathok: %s is dead\n",
					fs->fs_mntpnt);
			return (0);
		}
		if (trace > 1)
			fprintf(stderr, "pathok: %s is OK\n",
				fs->fs_mntpnt);
		tfs = fs;
	}
	return (1);
}

freeex(ex)
	struct exports *ex;
{
	struct groups *groups, *tmpgroups;
	struct exports *tmpex;

	while (ex) {
		free(ex->ex_name);
		groups = ex->ex_groups;
		while (groups) {
			tmpgroups = groups->g_next;
			free((char *)groups);
			groups = tmpgroups;
		}
		tmpex = ex->ex_next;
		free((char *)ex);
		ex = tmpex;
	}
}

mkdir_r(dir)
	char *dir;
{
	int err;
	char *slash;
	char *rindex();

	if (mkdir(dir, 0555) == 0 || errno == EEXIST)
		return (0);
	if (errno != ENOENT)
		return (-1);
	slash = rindex(dir, '/');
	if (slash == NULL)
		return (-1);
	*slash = '\0';
	err = mkdir_r(dir);
	*slash++ = '/';
	if (err || !*slash)
		return (err);
	return mkdir(dir, 0555);
}

safe_rmdir(dir)
	char *dir;
{
	extern dev_t tmpdev;
	struct stat stbuf;

	if (stat(dir, &stbuf))
		return;
	if (stbuf.st_dev == tmpdev)
		(void) rmdir(dir);
}
