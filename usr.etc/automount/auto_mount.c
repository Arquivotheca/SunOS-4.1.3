#ifndef lint
static  char sccsid[] = "@(#)auto_mount.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <mntent.h>
#include <netdb.h>
#include <errno.h>
#include "nfs_prot.h"
typedef nfs_fh fhandle_t;
#include <rpcsvc/mount.h>
#define NFSCLIENT
#include <sys/mount.h>
#include "automount.h"

#define MAXHOSTS  20

struct mapfs *find_server();
struct filsys *already_mounted();
void addtomtab();
char *inet_ntoa();
extern int verbose, trace;

static long last_mtab_time = 0;

nfsstat
do_mount(dir, me, rootfs, linkpath)
	struct autodir *dir;
	struct mapent *me;
	struct filsys **rootfs;
	char **linkpath;
{
	char mntpnt[MAXPATHLEN];
	static char linkbuf[MAXPATHLEN];
	enum clnt_stat pingmount();
	struct filsys *fs, *tfs;
	struct mapfs *mfs;
	struct mapent *m, *mapnext;
	nfsstat status = NFSERR_NOENT;
	struct in_addr prevhost;
	int imadeit;
	struct stat stbuf;
	extern dev_t tmpdev;

	*rootfs = NULL;
	*linkpath = "";
	prevhost.s_addr = (u_long)0;

	for (m = me ; m ; m = mapnext) {
		mapnext = m->map_next;

		(void) sprintf(mntpnt, "%s%s%s%s", tmpdir,
			dir->dir_name, me->map_root, m->map_mntpnt);

		/* check whether the mntpnt is already mounted on */

		if (m == me) {
			for (fs = HEAD(struct filsys, fs_q); fs;
	    			fs = NEXT(struct filsys, fs)) {
				if (strcmp(mntpnt, fs->fs_mntpnt) == 0) {
				    for (mfs = me->map_fs ; mfs ;
						mfs = mfs->mfs_next) {
					if (strcmp(mfs->mfs_host,
							fs->fs_host) == 0) {
					(void) sprintf(linkbuf, "%s%s%s%s", 
						tmpdir,
						dir->dir_name,
						me->map_root,
						mfs->mfs_subdir);
					if (trace > 1)
						(void) fprintf(stderr,
						"renew link for %s\n",
						linkbuf);
					*linkpath = linkbuf;
					return (NFS_OK);
				    }
				}
			    }
			}
		}

		tfs = NULL;
		mfs = find_server(m, &tfs, m == me, prevhost);
		if (mfs == NULL)
			continue;

		/*
		 * It may be possible to return a symlink
		 * to an existing mount point without
		 * actually having to do a mount.
		 */
		if (me->map_next == NULL && *me->map_mntpnt == '\0') {

			/* Is it my host ? */
			if ((mfs->mfs_addr.s_addr == my_addr.s_addr ||
			     mfs->mfs_addr.s_addr == htonl(INADDR_LOOPBACK))) {
				(void) strcpy(linkbuf, mfs->mfs_dir);
				(void) strcat(linkbuf, mfs->mfs_subdir);
				*linkpath = linkbuf;
				if (trace > 1)
					(void) fprintf(stderr,
						"It's on my host\n");
				return (NFS_OK);
			}

			/*
			 * An existing mount point ?
			 * XXX Note: this is a bit risky - the
			 * mount may belong to another automount
			 * daemon - it could unmount it anytime and
			 * this symlink would then point to an empty
			 * or non-existent mount point.
			 */
			if (tfs != NULL) {
				if (trace > 1)
					(void) fprintf(stderr,
					"already mounted %s:%s on %s (%s)\n",
					tfs->fs_host, tfs->fs_dir,
					tfs->fs_mntpnt, tfs->fs_opts);
	
				(void) strcpy(linkbuf, tfs->fs_mntpnt);
				(void) strcat(linkbuf, mfs->mfs_subdir);
				*linkpath = linkbuf;
				*rootfs = tfs;
				return (NFS_OK);
			}
		}

		if (nomounts)
			return (NFSERR_PERM);

		 /* Create the mount point if necessary */

		imadeit = 0;
		if (stat(mntpnt, &stbuf) != 0) {
			if (mkdir_r(mntpnt) == 0) {
				imadeit = 1;
				if (stat(mntpnt, &stbuf) < 0) {
					syslog(LOG_ERR,
					"Couldn't stat created mountpoint %s: %m",
					mntpnt);
					continue;
				}
			} else {
				if (verbose)
					syslog(LOG_ERR,
					"Couldn't create mountpoint %s: %m",
					mntpnt);
				if (trace > 1)
					(void) fprintf(stderr,
					"couldn't create mntpnt %s\n",
					mntpnt);
				continue;
			}
		}
		if (verbose && *rootfs == NULL && tmpdev != stbuf.st_dev)
			syslog(LOG_ERR, "WARNING: %s already mounted on",
				mntpnt);

		/*  Now do the mount */

		tfs = NULL;
		status = nfsmount(mfs->mfs_host, mfs->mfs_addr, mfs->mfs_dir,
				mntpnt, m->map_mntopts, &tfs);
		if (status == NFS_OK) {
			if (*rootfs == NULL) {
				*rootfs = tfs;
				(void) sprintf(linkbuf, "%s%s%s%s",
					tmpdir, dir->dir_name,
					me->map_root,
					mfs->mfs_subdir);
				*linkpath = linkbuf;
			}
			tfs->fs_rootfs = *rootfs;
			tfs->fs_mntpntdev = stbuf.st_dev;
			if (stat(mntpnt, &stbuf) < 0) {
				syslog(LOG_ERR, "Couldn't stat: %s: %m",
					mntpnt);
			} else {
				tfs->fs_mountdev = stbuf.st_dev;
			}
			prevhost.s_addr = mfs->mfs_addr.s_addr;
		} else {
			if (imadeit)
				safe_rmdir(mntpnt);
			mfs->mfs_ignore = 1;
			prevhost.s_addr = (u_long)0;
			mapnext = m;
			continue;  /* try another server */
		}
	}
	if (*rootfs != NULL) {
		addtomtab(*rootfs);
		return (NFS_OK);
	}
	return (status);
}

struct mapfs *
find_server(me, fsp, rootmount, preferred)
	struct mapent *me;
	struct filsys **fsp;
	int rootmount;
	    struct in_addr preferred;
{
	int entrycount, localcount;
	struct mapfs *mfs, *mfs_one;
	struct hostent *hp;
	struct in_addr addrs[MAXHOSTS], addr, *addrp, trymany();


	/*
	 * get addresses & see if any are myself
	 * or were mounted from previously in a
	 * hierarchical mount.
	 */
	entrycount = localcount = 0;
	for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next) {
		if (mfs->mfs_addr.s_addr == 0) {
			hp = gethostbyname(mfs->mfs_host);
			if (hp)
				bcopy(hp->h_addr, (char *)&mfs->mfs_addr,
					hp->h_length);
		}
		if (mfs->mfs_addr.s_addr && !mfs->mfs_ignore) {
			entrycount++;
			mfs_one = mfs;
			if (mfs->mfs_addr.s_addr == my_addr.s_addr  ||
			    mfs->mfs_addr.s_addr == htonl(INADDR_LOOPBACK) ||
			    mfs->mfs_addr.s_addr == preferred.s_addr) {
				return (mfs);
			}
		}
	}
	if (entrycount == 0)
		return (NULL);

	/* see if any already mounted */
	if (rootmount)
		for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next) {
			if (mfs->mfs_ignore)
				continue;
			if (*fsp = already_mounted(mfs->mfs_host, mfs->mfs_dir,
					me->map_mntopts, mfs->mfs_addr)) 
				return (mfs);
		}

	if (entrycount == 1) 	/* no replication involved */
		return (mfs_one);

	/* try local net */
	addrp = addrs;
	for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next) {
		if (mfs->mfs_ignore)
			continue;
		if (inet_netof(mfs->mfs_addr) == inet_netof(my_addr)) {
			localcount++;
			*addrp++ = mfs->mfs_addr;
		}
	}
	if (localcount > 0) {	/* got some */
		(*addrp).s_addr = 0;	/* terminate list */
		addr = trymany(addrs, mount_timeout / 2);
		if (addr.s_addr) {	/* got one */
			for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next)
			if (addr.s_addr == mfs->mfs_addr.s_addr)
				return (mfs);
		}
	}
	if (entrycount == localcount)
		return (NULL);

	/* now try them all */
	addrp = addrs;
	for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next)
		if (!mfs->mfs_ignore)
			*addrp++ = mfs->mfs_addr;
	(*addrp).s_addr = 0;	/* terminate list */
	addr = trymany(addrs, mount_timeout / 2);
	if (addr.s_addr) {	/* got one */
		for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next)
		if (addr.s_addr == mfs->mfs_addr.s_addr)
				return (mfs);
	}
	return (NULL);
}

/*
 * Read the mtab and correlate with internal fs info
 */
void
read_mtab()
{
	struct stat stbuf;
	struct filsys *fs, *fsnext;
	FILE *mtab;
	register struct mntent *mt;
	char *index();
	int found, c;
	long mtab_size;
	struct stat st;
	struct hostent *hp;
	char *tmphost, *tmppath, *p, tmpc;

	/* reset the present flags */
	for (fs = HEAD(struct filsys, fs_q); fs;
	    fs = NEXT(struct filsys, fs))
		    fs->fs_present = 0;

	/* now see what's been mounted */

	mtab = setmntent(MOUNTED, "r");
	if (fstat(fileno(mtab), &st) < 0) {
		syslog(LOG_ERR, "couldn't stat %s: %m", MOUNTED);
		return;
	}
	mtab_size = st.st_size;
		
	for (c = 1 ;; c++) {
		mt = getmntent(mtab);
		if (mt == NULL) {
			if (ftell(mtab) >= mtab_size)
				break;  /* it's really EOF */
			syslog(LOG_ERR, "WARNING: %s: line %d: bad entry",
				MOUNTED, c);
			continue;
		}

		if (strcmp(mt->mnt_type, MNTTYPE_NFS) != 0)
			continue;
		p = index(mt->mnt_fsname, ':');
		if (p == NULL)
			continue;
		tmpc = *p;
		*p = '\0';
		tmphost = mt->mnt_fsname;
		tmppath = p+1;
		if (tmppath[0] != '/')
			continue;
		found = 0;
		for (fs = HEAD(struct filsys, fs_q); fs;
			fs = NEXT(struct filsys, fs)) {
			if (strcmp(mt->mnt_dir, fs->fs_mntpnt) == 0 &&
			    strcmp(tmphost, fs->fs_host) == 0 &&
			    strcmp(tmppath, fs->fs_dir) == 0 &&
			    (fs->fs_mine ||
				    strcmp(mt->mnt_opts, fs->fs_opts) == 0)) {
				fs->fs_present = 1;
				found++;
				break;
			}
		}
		if (!found) {
			fs = alloc_fs(tmphost, tmppath,
				mt->mnt_dir, mt->mnt_opts);
			if (fs == NULL)
				break;
			fs->fs_present = 1;
			hp = gethostbyname(tmphost);
			if (hp != NULL) {
				bcopy(hp->h_addr, &fs->fs_addr.sin_addr, hp->h_length);
			}
		}
		*p = tmpc;
	}
	(void) endmntent(mtab);

	/* free fs's that are no longer present */
	for (fs = HEAD(struct filsys, fs_q); fs; fs = fsnext) {
		fsnext = NEXT(struct filsys, fs);
		if (!fs->fs_present) {
			if (fs->fs_mine)
				syslog(LOG_ERR,
					"%s:%s no longer mounted",
					fs->fs_host, fs->fs_dir);
			if (trace > 1)
				(void) fprintf(stderr,
					"%s:%s no longer mounted\n",
					fs->fs_host, fs->fs_dir);
			flush_links(fs);
			free_filsys(fs);
		}
	}

	if (stat(MOUNTED, &stbuf) != 0) {
		syslog(LOG_ERR, "Cannot stat %s: %m", MOUNTED);
		return;
	}
	last_mtab_time = stbuf.st_mtime;
}

/*
 *  If mtab has changed update internal fs info
 */
void
check_mtab()
{
	struct stat stbuf;

	if (stat(MOUNTED, &stbuf) != 0) {
		syslog(LOG_ERR, "Cannot stat %s: %m", MOUNTED);
		return;
	}
	if (stbuf.st_mtime != last_mtab_time)
		read_mtab();
}

/*
 * Search the mount table to see if the given file system is already
 * mounted. 
 */
struct filsys *
already_mounted(host, fsname, opts, hostaddr)
	char *host;
	char *fsname;
	char *opts;
	struct in_addr hostaddr;
{
	struct filsys *fs;
	struct stat stbuf;
	struct mntent m1, m2;
	int has1, has2;
	int fd;
	struct autodir *dir;
	int mydir;
	extern int verbose;

	check_mtab();
	m1.mnt_opts = opts;
	for (fs = HEAD(struct filsys, fs_q); fs; fs = NEXT(struct filsys, fs)){
		if (strcmp(fsname, fs->fs_dir) != 0)
			continue;
		if (hostaddr.s_addr != fs->fs_addr.sin_addr.s_addr)
			continue;

		/*
		 * Check it's not on one of my mount points.
		 * I might be mounted on top of a previous 
		 * mount of the same file system.
		 */

		for (mydir = 0, dir = HEAD(struct autodir, dir_q); dir;
			dir = NEXT(struct autodir, dir)) {
			if (strcmp(dir->dir_name, fs->fs_mntpnt) == 0) {
				mydir = 1;
				if (verbose)
					syslog(LOG_ERR,
					"%s:%s already mounted on %s",
					host, fsname, fs->fs_mntpnt);
				break;
			}
		}
		if (mydir)
			continue;

		m2.mnt_opts = fs->fs_opts;
		has1 = hasmntopt(&m1, MNTOPT_RO) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_RO) != NULL;
		if (has1 != has2)
			continue;
		has1 = hasmntopt(&m1, MNTOPT_NOSUID) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_NOSUID) != NULL;
		if (has1 != has2)
			continue;
		has1 = hasmntopt(&m1, MNTOPT_SOFT) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_SOFT) != NULL;
		if (has1 != has2)
			continue;
		has1 = hasmntopt(&m1, MNTOPT_INTR) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_INTR) != NULL;
		if (has1 != has2)
			continue;
#ifdef MNTOPT_SECURE
		has1 = hasmntopt(&m1, MNTOPT_SECURE) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_SECURE) != NULL;
		if (has1 != has2)
			continue;
#endif MNTOPT_SECURE
		return (fs);
	}
	return(0);
}

nfsunmount(fs)
	struct filsys *fs;
{
	struct sockaddr_in sin;
	struct timeval timeout;
	CLIENT *cl;
	enum clnt_stat rpc_stat;
	int s;

	sin = fs->fs_addr;
	if (pingmount(sin.sin_addr) != RPC_SUCCESS)
		return;
	/*
	 * Port number of "fs->fs_addr" is NFS port number; make port
	 * number 0, so "clntudp_create" finds port number of mount
	 * service.
	 */
	sin.sin_port = 0;
	s = RPC_ANYSOCK;
	timeout.tv_sec = 2; /* retry interval */
	timeout.tv_usec = 0;
	if ((cl = clntudp_create(&sin, MOUNTPROG, 1,
	    timeout, &s)) == NULL) {
		syslog(LOG_WARNING, "%s:%s %s", fs->fs_host, fs->fs_dir,
		    clnt_spcreateerror("server not responding"));
		return;
	}
#ifdef OLDMOUNT
	if (bindresvport(s, NULL))
		syslog(LOG_ERR, "Warning: cannot do local bind");
#endif
	cl->cl_auth = authunix_create_default();
	timeout.tv_sec = 10;
	rpc_stat = clnt_call(cl, MOUNTPROC_UMNT, xdr_path, &fs->fs_dir,
	    xdr_void, (char *)NULL, timeout);
	(void) close(s);
	AUTH_DESTROY(cl->cl_auth);
	clnt_destroy(cl);
	if (rpc_stat != RPC_SUCCESS)
		syslog(LOG_WARNING, "%s", clnt_sperror(cl, "unmount"));
}

enum clnt_stat
pingmount(hostaddr)
	struct in_addr hostaddr;
{
	struct timeval timeout;
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat = RPC_TIMEDOUT;
	CLIENT *cl;
	static struct in_addr goodhost, deadhost;
	static time_t goodtime, deadtime;
	int cache_time = 60;  /* sec */
	int sock = RPC_ANYSOCK;

	if (goodtime > time_now && hostaddr.s_addr == goodhost.s_addr)
			return (RPC_SUCCESS);
	if (deadtime > time_now && hostaddr.s_addr == deadhost.s_addr)
			return (RPC_TIMEDOUT);

	if (trace > 1)
		(void) fprintf(stderr, "ping %s ", inet_ntoa(hostaddr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr = hostaddr;
	server_addr.sin_port = htons(NFS_PORT);
	timeout.tv_sec = 2;	/* retry interval */
	timeout.tv_usec = 0;
	cl = clntudp_create(&server_addr, NFS_PROGRAM, NFS_VERSION,
		timeout, &sock);
	if (cl) {
		timeout.tv_sec = 10;
		clnt_stat = clnt_call(cl, NULLPROC, xdr_void, 0, xdr_void, 0,
			timeout);
		clnt_destroy(cl);
	}

	if (clnt_stat == RPC_SUCCESS) {
		goodhost = hostaddr;
		goodtime = time_now + cache_time;
	} else {
		deadhost = hostaddr;
		deadtime = time_now + cache_time;
	}

	if (trace > 1)
		(void) fprintf(stderr, "%s\n", clnt_stat == RPC_SUCCESS ?
			"OK" : "NO RESPONSE");

	return (clnt_stat);
}

struct in_addr gotaddr;

/* ARGSUSED */
catchfirst(raddr)
	struct sockaddr_in *raddr;
{
	gotaddr = raddr->sin_addr;
	return (1);	/* first one ends search */
}

/*
 * ping a bunch of hosts at once and find out who
 * responds first
 */
struct in_addr
trymany(addrs, timeout)
	struct in_addr *addrs;
	int timeout;
{
	enum clnt_stat nfs_cast();
	enum clnt_stat clnt_stat;

	if (trace > 1) {
		register struct in_addr *a;

		(void) fprintf(stderr, "nfs_cast: ");
		for (a = addrs ; a->s_addr ; a++)
			(void) fprintf(stderr, "%s ", inet_ntoa(*a));
		(void) fprintf(stderr, "\n");
	}
		
	gotaddr.s_addr = 0;
	clnt_stat = nfs_cast(addrs, catchfirst, timeout);
	if (trace > 1) {
		(void) fprintf(stderr, "nfs_cast: got %s\n",
			(int) clnt_stat ? "no response" : inet_ntoa(gotaddr));
	}
	if (clnt_stat)
		syslog(LOG_ERR, "trymany: servers not responding: %s",
			clnt_sperrno(clnt_stat));
	return (gotaddr);
}

/*
 * Return 1 if the path s2 is a subdirectory of s1.
 */
subdir(s1, s2)
	char *s1, *s2;
{
	while (*s1 == *s2)
		s1++, s2++;
	return (*s1 == '\0' && *s2 == '/');
}

nfsstat
nfsmount(host, hostaddr, dir, mntpnt, opts, fsp)
	char *host, *dir, *mntpnt, *opts;
	struct in_addr hostaddr;
	struct filsys **fsp;
{
	struct filsys *fs;
	char netname[MAXNETNAMELEN+1];
	char remname[MAXPATHLEN];
	struct mntent m;
	struct nfs_args args;
	int flags;
	static struct sockaddr_in sin;
	static struct fhstatus fhs;
	static int s = -1;
	struct timeval timeout;
	static CLIENT *cl = NULL;
	static time_t time_valid;
	int cache_time = 60;  /* sec */
	enum clnt_stat rpc_stat;
	enum clnt_stat pingmount();
	u_short port;
	nfsstat status;
	struct stat stbuf;
	static u_long prev_vers = 0;
	u_long vers;

	/* Make sure the mountpoint is safe to mount on */
	if (lstat(mntpnt, &stbuf) < 0) {
		syslog(LOG_ERR, "Couldn't stat %s: %m", mntpnt);
		return (NFSERR_NOENT);
	}
	/* A symbolic link could be dangerous e.g. -> /usr */
	if ((stbuf.st_mode & S_IFMT) == S_IFLNK) {
		if (realpath(mntpnt, remname) == NULL) {
			syslog(LOG_ERR, "%s: realpath failed: %m",
				mntpnt);
			return (NFSERR_NOENT);
		}
		if (!subdir(tmpdir, remname)) {
			syslog(LOG_ERR,
				"%s:%s -> %s : dangerous symbolic link",
				host, dir, remname);
			return (NFSERR_NOENT);
		}
	}

	if (pingmount(hostaddr) != RPC_SUCCESS) {
		syslog(LOG_ERR, "host %s not responding", host);
		return (NFSERR_NOENT);
	}
	(void) sprintf(remname, "%s:%s", host, dir);
	m.mnt_opts = opts;
#ifdef MOUNTVERS_POSIX
	vers = hasmntopt(&m, MNTOPT_POSIX) != NULL ?
		MOUNTVERS_POSIX : MOUNTVERS;
#else
	vers = MOUNTVERS;
#endif
	/*
	 * Get a new client handle if the host is different
	 */
	if (cl == NULL ||
		hostaddr.s_addr != sin.sin_addr.s_addr ||
		vers != prev_vers || time_now > time_valid) {
		prev_vers = vers;
		if (cl) {
			if (cl->cl_auth)
				AUTH_DESTROY(cl->cl_auth);
			clnt_destroy(cl);
		}
		bzero((char *)&sin, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = hostaddr.s_addr;
		timeout.tv_usec = 0;
		timeout.tv_sec = 2;  /* retry interval */
		s = RPC_ANYSOCK;
		if ((cl = clntudp_create(&sin, MOUNTPROG, vers,
		    timeout, &s)) == NULL) {
			syslog(LOG_ERR, "%s: %s", remname,
			clnt_spcreateerror("server not responding"));
			return (NFSERR_NOENT);
		}
#ifdef OLDMOUNT
		if (bindresvport(s, NULL))
			syslog(LOG_ERR, "Warning: cannot do local bind");
#endif
		cl->cl_auth = authunix_create_default();
		time_valid = time_now + cache_time;
	}

	/*
	 * Get fhandle of remote path from server's mountd.
	 */
	timeout.tv_usec = 0;
	timeout.tv_sec = mount_timeout;
	rpc_stat = clnt_call(cl, MOUNTPROC_MNT, xdr_path, &dir,
	    xdr_fhstatus, &fhs, timeout);
	if (rpc_stat != RPC_SUCCESS) {
		/*
		 * Given the way "clnt_sperror" works, the "%s" immediately
		 * following the "not responding" is correct.
		 */
		syslog(LOG_ERR, "%s: server not responding%s", remname,
		    clnt_sperror(cl, ""));
		return (NFSERR_NOENT);
	}

	if (errno = fhs.fhs_status)  {
                if (errno == EACCES) {
			if (verbose)
				syslog(LOG_ERR,
				"can't mount %s: no permission", remname);
			status = NFSERR_ACCES;
                } else {
			syslog(LOG_ERR, "can't mount %s: %m", remname);
			status = NFSERR_IO;
                }
                return (status);
        }        

	/*
	 * set mount args
	 */
	args.fh = (caddr_t)&fhs.fhs_fh;
	args.hostname = host;
	args.flags = 0;
	args.flags |= NFSMNT_HOSTNAME;
	args.addr = &sin;

	if (hasmntopt(&m, MNTOPT_SOFT) != NULL) {
		args.flags |= NFSMNT_SOFT;
	}
	if (hasmntopt(&m, MNTOPT_INTR) != NULL) {
		args.flags |= NFSMNT_INT;
	}
#ifdef MNTOPT_NOAC
	if (hasmntopt(&m, MNTOPT_NOAC) != NULL) {
		args.flags |= NFSMNT_NOAC;
	}
#endif
#ifdef MNTOPT_NOCTO
	if (hasmntopt(&m, MNTOPT_NOCTO) != NULL) {
		args.flags |= NFSMNT_NOCTO;
	}
#endif
#ifdef MNTOPT_SECURE
	if (hasmntopt(&m, MNTOPT_SECURE) != NULL) {
		args.flags |= NFSMNT_SECURE;
		/*
                 * XXX: need to support other netnames outside domain
                 * and not always just use the default conversion
                 */
                if (!host2netname(netname, host, NULL)) {
                        return (NFSERR_NOENT); /* really unknown host */
                }
                args.netname = netname;
	}
#endif MNTOPT_SECURE
	if (port = nopt(&m, MNTOPT_PORT)) {
		sin.sin_port = htons(port);
	} else {
		sin.sin_port = htons(NFS_PORT);	/* XXX should use portmapper */
	}

	if (args.rsize = nopt(&m, MNTOPT_RSIZE)) {
		args.flags |= NFSMNT_RSIZE;
	}
	if (args.wsize = nopt(&m, MNTOPT_WSIZE)) {
		args.flags |= NFSMNT_WSIZE;
	}
	if (args.timeo = nopt(&m, MNTOPT_TIMEO)) {
		args.flags |= NFSMNT_TIMEO;
	}
	if (args.retrans = nopt(&m, MNTOPT_RETRANS)) {
		args.flags |= NFSMNT_RETRANS;
	}
	if (args.acregmax = nopt(&m, MNTOPT_ACTIMEO)) {
		args.acdirmax = args.acregmax;
		args.flags |= NFSMNT_ACREGMAX;
		args.flags |= NFSMNT_ACDIRMAX;
	} else {
		if (args.acregmin = nopt(&m, MNTOPT_ACREGMIN)) {
			args.flags |= NFSMNT_ACREGMIN;
		}
		if (args.acregmax = nopt(&m, MNTOPT_ACREGMAX)) {
			args.flags |= NFSMNT_ACREGMAX;
		}
		if (args.acdirmin = nopt(&m, MNTOPT_ACDIRMIN)) {
			args.flags |= NFSMNT_ACDIRMIN;
		}
		if (args.acdirmax = nopt(&m, MNTOPT_ACDIRMAX)) {
			args.flags |= NFSMNT_ACDIRMAX;
		}
	}
#ifdef MOUNTVERS_POSIX
	if (vers == MOUNTVERS_POSIX) {
		struct pathcnf *get_pathconf();

		args.pathconf = get_pathconf(cl, dir, remname);
		if (args.pathconf == (struct pathcnf *) 0)
			return (NFSERR_IO);
		args.flags |= NFSMNT_POSIX;
	}
#endif
	

	flags = 0;
	flags |= (hasmntopt(&m, MNTOPT_RO) == NULL) ? 0 : M_RDONLY;
	flags |= (hasmntopt(&m, MNTOPT_NOSUID) == NULL) ? 0 : M_NOSUID;
	flags |= (hasmntopt(&m, MNTOPT_GRPID) == NULL) ? 0 : M_GRPID;
	flags |= (hasmntopt(&m, MNTOPT_NOSUB) == NULL) ? 0 : M_NOSUB;

	if (trace > 1) {
		(void) fprintf(stderr, "mount %s %s (%s)\n",
			remname, mntpnt, opts);
	}
#ifdef OLDMOUNT
#define MOUNT_NFS 1
	if (mount(MOUNT_NFS, mntpnt, flags, &args)) {
#else
	if (mount("nfs", mntpnt, flags | M_NEWTYPE, &args)) {
#endif
		syslog(LOG_ERR, "Mount of %s on %s: %m", remname, mntpnt);
		return (NFSERR_IO);
	}
	if (trace > 1) {
		(void) fprintf(stderr, "mount %s OK\n", remname);
	}
	if (*fsp)
		fs = *fsp;
	else {
		fs = alloc_fs(host, dir, mntpnt, opts);
		if (fs == NULL)
			return (NFSERR_NOSPC);
	}
	fs->fs_type = MNTTYPE_NFS;
	fs->fs_mine = 1;
	fs->fs_nfsargs = args;
	fs->fs_mflags = flags;
	fs->fs_nfsargs.hostname = fs->fs_host;
	fs->fs_nfsargs.addr = &fs->fs_addr;
	fs->fs_nfsargs.fh = (caddr_t)&fs->fs_rootfh;
	fs->fs_addr = sin;
	bcopy(&fhs.fhs_fh, &fs->fs_rootfh, sizeof fs->fs_rootfh);
	*fsp = fs;
	return (NFS_OK);
}

#ifdef MOUNTVERS_POSIX
struct pathcnf *
get_pathconf(cl, path, fsname)
	CLIENT *cl;
	char *path, *fsname;
{
	static struct pathcnf p;
	enum clnt_stat rpc_stat;
	struct timeval timeout;

	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	rpc_stat = clnt_call(cl, MOUNTPROC_PATHCONF, 
	    xdr_path, &path, xdr_pathcnf, &p, timeout);
	if (rpc_stat != RPC_SUCCESS) {
			syslog(LOG_ERR,
				"pathconf: %s: server not responding: %s",
				fsname,
				clnt_sperror(cl, ""));
		return ((struct pathcnf *) 0);
	}
	if (_PC_ISSET(_PC_ERROR, p.pc_mask)) {
		syslog(LOG_ERR, "pathconf: no info for %s", fsname);
		return ((struct pathcnf *) 0);
	}
	return (&p);
}
#endif MOUNTVERS_POSIX

nfsstat
remount(fs)
	struct filsys *fs;
{
	char remname[1024];
	struct stat stbuf;

	if (fs->fs_nfsargs.fh == 0) 
		return nfsmount(fs->fs_host, fs->fs_addr.sin_addr, fs->fs_dir,
				fs->fs_mntpnt, fs->fs_opts, &fs);
	(void) sprintf(remname, "%s:%s", fs->fs_host, fs->fs_dir);
	if (trace > 1) {
		(void) fprintf(stderr, "remount %s %s (%s)\n",
			remname, fs->fs_mntpnt, fs->fs_opts);
	}
	if (pingmount(fs->fs_addr.sin_addr) != RPC_SUCCESS) {
		if (verbose || fs->fs_unmounted++ < 5)
			syslog(LOG_ERR,
				"remount %s on %s: server not responding",
				remname, fs->fs_mntpnt);
		if (trace > 1)
			(void) fprintf(stderr, "remount FAILED: server not responding\n");
		return (NFSERR_IO);
	}
	if (stat(fs->fs_mntpnt, &stbuf) < 0) {
		syslog(LOG_ERR, "remount: couldn't stat: %s: %m", fs->fs_mntpnt);
		return (NFSERR_IO);
	}
#ifdef OLDMOUNT
	if (mount(MOUNT_NFS, fs->fs_mntpnt, fs->fs_mflags, &fs->fs_nfsargs)) {
#else
	if (mount("nfs", fs->fs_mntpnt, fs->fs_mflags | M_NEWTYPE, &fs->fs_nfsargs)) {
#endif
		if (verbose || fs->fs_unmounted++ < 5)
			syslog(LOG_ERR, "remount of %s on %s: %m", remname,
		    fs->fs_mntpnt);
		if (trace > 1)
			perror("remount FAILED ");
		return (NFSERR_IO);
	}

	fs->fs_mntpntdev = stbuf.st_dev;
	if (stat(fs->fs_mntpnt, &stbuf) < 0) {
		syslog(LOG_ERR, "remount: couldn't stat: %s: %m", fs->fs_mntpnt);
		return (NFSERR_IO);
	}
	fs->fs_mountdev = stbuf.st_dev;

	if (trace > 1)
		(void) fprintf(stderr, "remount OK\n");
	return (NFS_OK);

}


/*
 * Add one or more entries to /etc/mtab
 */
void
addtomtab(rootfs)
	struct filsys *rootfs;
{
	FILE *fp;
	struct filsys *fs;
	struct stat stbuf;
	struct mntent mnt;
	char remname[MAXPATHLEN];
	char opts[1024];
	int forked;
	
	/*
	 * In SunOs 4.0 only mount and automount flock the mtab.
	 * In releases prior to 4.0 any programs that browse mtab
	 * (df, find etc) also lock the mtab and deadlock with
	 * the automounter is a distinct possibility. Avoid
	 * deadlock here by backgrounding the update.
	 */
	forked = 0;
	if (access(MOUNTED, W_OK) < 0) {
		syslog(LOG_ERR, "%s: %m", MOUNTED);
		return;
	}
	if (islocked(MOUNTED)) {
		if (fork())
			return;
		forked++;
	}

	fp = setmntent(MOUNTED, "r+");
	if (fp == NULL) {
		syslog(LOG_ERR, "%s: %m", MOUNTED);
		if (forked)
			_exit(0);
		return;
	}

	for (fs = TAIL(struct filsys, fs_q); fs; fs = PREV(struct filsys, fs)) {
		if (fs->fs_rootfs != rootfs)
			continue;

		(void) sprintf(remname, "%s:%s", fs->fs_host, fs->fs_dir);
		mnt.mnt_fsname = remname;
		mnt.mnt_dir = fs->fs_mntpnt;
		mnt.mnt_type = MNTTYPE_NFS;
		mnt.mnt_freq = mnt.mnt_passno = 0;

		(void) sprintf(opts, "%s,%s=%04x", fs->fs_opts,
			MNTINFO_DEV, (fs->fs_mountdev & 0xFFFF));
		mnt.mnt_opts = opts;

		if (addmntent(fp, &mnt)) {
			(void) endmntent(fp);
			syslog(LOG_ERR, "%s: %m", MOUNTED);
			if (forked)
				_exit(0);
			return;
		}
	}
	(void) endmntent(fp);
	if (stat(MOUNTED, &stbuf) < 0)
		syslog(LOG_ERR, "%s: %m", MOUNTED);
	else
		last_mtab_time = stbuf.st_mtime;

	if (forked)
		_exit(0);
}

#include <sys/file.h>
#include <fcntl.h>
/*
 * Check whether the file is flock'ed.
 */
int
islocked(file)
	char *file;
{
	int f;

	f = open(file, O_RDONLY);
	if (f < 0) {
		syslog(LOG_ERR, "%s: %m", MOUNTED);
		return 1;
	}
	if (flock(f, LOCK_EX | LOCK_NB) < 0) {
		sleep(1);
		if (flock(f, LOCK_EX | LOCK_NB) < 0) {
			(void) close(f);
			return 1;
		}
	}
	(void) close(f);	/* close and release the lock */
	return 0;
}

/*
 * This structure is used to build a list of
 * mntent structures from /etc/mtab.
 */
struct mntlist {
	struct mntent  *mntl_mnt;
	struct mntlist *mntl_next;
};

/*
 * Duplicate a mntent structure
 */
struct mntent *
dupmntent(mnt)
	struct mntent *mnt;
{
	struct mntent *new;
	void freemntent();

	new = (struct mntent *)malloc(sizeof(*new));
	if (new == NULL)
		goto alloc_failed;
	bzero((char *)new, sizeof(*new));
	new->mnt_fsname = strdup(mnt->mnt_fsname);
	if (new->mnt_fsname == NULL)
		goto alloc_failed;
	new->mnt_dir = strdup(mnt->mnt_dir);
	if (new->mnt_dir == NULL)
		goto alloc_failed;
	new->mnt_type = strdup(mnt->mnt_type);
	if (new->mnt_type == NULL)
		goto alloc_failed;
	new->mnt_opts = strdup(mnt->mnt_opts);
	if (new->mnt_opts == NULL)
		goto alloc_failed;
	new->mnt_freq   = mnt->mnt_freq;
	new->mnt_passno = mnt->mnt_passno;

	return (new);

alloc_failed:
	syslog(LOG_ERR, "dupmntent: memory allocation failed");
	freemntent(new);
	return NULL;
}

/*
 * Free a single mntent structure
 */
void
freemntent(mnt)
	struct mntent *mnt;
{
	if (mnt) {
		if (mnt->mnt_fsname)
			free(mnt->mnt_fsname);
		if (mnt->mnt_dir)
			free(mnt->mnt_dir);
		if (mnt->mnt_type)
			free(mnt->mnt_type);
		if (mnt->mnt_opts)
			free(mnt->mnt_opts);
		free(mnt);
	}
}

/*
 * Free a list of mntent structures
 */
void
freemntlist(mntl)
	struct mntlist *mntl;
{
	register struct mntlist *mntl_tmp;

	while (mntl) {
		freemntent(mntl->mntl_mnt);
		mntl_tmp = mntl;
		mntl = mntl->mntl_next;
		free(mntl_tmp);
	}
}

/*
 * Remove one or more entries from the mount table.
 * If mntpnt is non-null then remove the entry
 * for that mount point.
 * Otherwise use rootfs - it is the root fs of
 * a mounted hierarchy.  Remove all entries for
 * the hierarchy.
 */
void
clean_mtab(mntpnt, rootfs)
	char *mntpnt;
	struct filsys *rootfs;
{
	FILE *mtab;
	struct mntent *mnt;
	struct stat stbuf;
	struct filsys *fs;
	struct mntlist *mntl_head = NULL;
	struct mntlist *mntl_prev, *mntl;
	long mtab_size;
	int delete, c;

	mtab = setmntent(MOUNTED, "r+");
	if (mtab == NULL) {
		syslog(LOG_ERR, "%s: %m", MOUNTED);
		return;
	}

	/*
	 * Read the entire mtab into memory except for the
	 * entries we're trying to delete.
	 */
	if (fstat(fileno(mtab), &stbuf) < 0) {
		syslog(LOG_ERR, "couldn't stat %s: %m", MOUNTED);
		return;
	}
	mtab_size = stbuf.st_size;
		
	for (c = 1 ;; c++) {
		mnt = getmntent(mtab);
		if (mnt == NULL) {
			if (ftell(mtab) >= mtab_size)
				break;  /* it's really EOF */
			syslog(LOG_ERR, "WARNING: %s: line %d: bad entry removed",
				MOUNTED, c);
			continue;
		}


		if (mntpnt) {
			if (strcmp(mnt->mnt_dir, mntpnt) == 0)
				continue;
		} else {
			delete = 0;
			for (fs = rootfs ; fs ; fs = NEXT(struct filsys, fs)) {
				if (strcmp(mnt->mnt_dir, fs->fs_mntpnt) == 0) {
					delete = 1;
					break;
				}
			}
			if (delete)
				continue;
		}
		mntl = (struct mntlist *)malloc(sizeof(*mntl));
		if (mntl == NULL)
			goto alloc_failed;
		if (mntl_head == NULL)
			mntl_head = mntl;
		else
			mntl_prev->mntl_next = mntl;
		mntl_prev = mntl;
		mntl->mntl_next = NULL;
		mntl->mntl_mnt = dupmntent(mnt);
		if (mntl->mntl_mnt == NULL)
			goto alloc_failed;
	}

	/* now truncate the mtab and write almost all of it back */

	rewind(mtab);
	if (ftruncate(fileno(mtab), 0) < 0) {
		syslog(LOG_ERR, "truncate %s: %m", MOUNTED);
		(void) endmntent(mtab);
		return;
	}

	for (mntl = mntl_head ; mntl ; mntl = mntl->mntl_next) {
		if (addmntent(mtab, mntl->mntl_mnt)) {
			syslog(LOG_ERR, "addmntent: %m");
			(void) endmntent(mtab);
			return;
		}
	}
	(void) endmntent(mtab);
	freemntlist(mntl_head);

	if (stat(MOUNTED, &stbuf) < 0)
		syslog(LOG_ERR, "%s: %m", MOUNTED);
	else
		last_mtab_time = stbuf.st_mtime;
	return;

alloc_failed:
	(void) endmntent(mtab);
	freemntlist(mntl_head);
	return;
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
			syslog(LOG_ERR, "Bad numeric option '%s'", str);
		}
	}
	return (val);
}
