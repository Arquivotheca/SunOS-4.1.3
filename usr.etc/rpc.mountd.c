#ifndef lint
static char sccsid[] = "@(#)rpc.mountd.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 Sun Microsystems, Inc. 
 */
#include <sys/param.h>
#include <rpc/rpc.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <exportent.h>
#include <string.h>
#include <sys/pathconf.h>
#include <sys/unistd.h>

#define MAXRMTABLINELEN		(MAXPATHLEN + MAXHOSTNAMELEN + 2)

extern int errno;

char XTAB[]  = "/etc/xtab";
char RMTAB[] = "/etc/rmtab";

int mnt();
char *exmalloc();
struct groups **newgroup();
struct exports **newexport();
void log_cant_reply();
void mnt_pathconf();
void mount();
void check_exports();
void xent_free();

/*
 * mountd's version of a "struct mountlist". It is the same except
 * for the added ml_pos field.
 */
struct mountdlist {
/* same as XDR mountlist */
	char *ml_name;
	char *ml_path;
	struct mountdlist *ml_nxt;
/* private to mountd */
	long ml_pos;		/* position of mount entry in RMTAB */
};

struct mountdlist *mountlist;

struct xent_list {		/* cached export list */
	struct xent_list *x_next;
	struct exportent  x_xent;
} *xent_list;

int nfs_portmon = 1;
char *domain;

void rmtab_load();
void rmtab_delete();
long rmtab_insert();

main(argc, argv)
	int argc;
	char **argv;
{
	SVCXPRT *transp;
	int pid;
	register int i;
	int sock;
	int proto;

	if (argc == 2) {
		if (strcmp(argv[1], "-n") == 0) {
			nfs_portmon = 0;
		} else {
			usage();
		}
	} else if (argc > 2) {
		usage();
	}

	if (issock(0)) {
		/*
		 * Started from inetd 
		 */
		sock = 0;
		proto = 0;	/* don't register with portmapper */
	} else {
		/*
		 * Started from shell, background.
		 */
		pid = fork();
		if (pid < 0) {
			perror("mountd: can't fork");
			exit(1);
		}
		if (pid) {
			exit(0);
		}

		/*
		 * Close existing file descriptors, open "/dev/null" as
		 * standard input, output, and error, and detach from
		 * controlling terminal.
		 */
		i = getdtablesize();
		while (--i >= 0)
			(void) close(i);
		(void) open("/dev/null", O_RDONLY);
		(void) open("/dev/null", O_WRONLY);
		(void) dup(1);
		i = open("/dev/tty", O_RDWR);
		if (i >= 0) {
			(void) ioctl(i, TIOCNOTTY, (char *)0);
			(void) close(i);
		}
		(void) pmap_unset(MOUNTPROG, MOUNTVERS);
		(void) pmap_unset(MOUNTPROG, MOUNTVERS_POSIX);
		sock = RPC_ANYSOCK;
		proto = IPPROTO_UDP;
	}

	openlog("mountd", LOG_PID, LOG_DAEMON);

	/*
	 * Create UDP service
	 */
	if ((transp = svcudp_create(sock)) == NULL) {
		syslog(LOG_ERR, "couldn't create UDP transport");
		exit(1);
	}
	if (!svc_register(transp, MOUNTPROG, MOUNTVERS, mnt, proto)) {
		syslog(LOG_ERR, "couldn't register UDP MOUNTPROG_ORIG");
		exit(1);
	}
	if (!svc_register(transp, MOUNTPROG, MOUNTVERS_POSIX, mnt, proto)) {
		syslog(LOG_ERR, "couldn't register UDP MOUNTPROG");
		exit(1);
	}

	/*
	 * Create TCP service
	 */
	if ((transp = svctcp_create(RPC_ANYSOCK, 0, 0)) == NULL) {
		syslog(LOG_ERR, "couldn't create TCP transport");
		exit(1);
	}
	if (!svc_register(transp, MOUNTPROG, MOUNTVERS, mnt, 
			  IPPROTO_TCP)) {
		syslog(LOG_ERR, "couldn't register TCP MOUNTPROG_ORIG");
		exit(1);
	}
	if (!svc_register(transp, MOUNTPROG, MOUNTVERS_POSIX, mnt, 
			  IPPROTO_TCP)) {
		syslog(LOG_ERR, "couldn't register TCP MOUNTPROG");
		exit(1);
	}

	/*
	 * Initalize the world 
	 */
	(void) yp_get_default_domain(&domain);

	/*
	 * Start serving 
	 */
	rmtab_load();
	svc_run();
	syslog(LOG_ERR, "Error: svc_run shouldn't have returned");
	abort();
	/* NOTREACHED */
}


/*
 * Determine if a descriptor belongs to a socket or not 
 */
issock(fd)
	int fd;
{
	struct stat st;

	if (fstat(fd, &st) < 0) {
		return (0);
	}
	/*
	 * SunOS returns S_IFIFO for sockets, while 4.3 returns 0 and does not
	 * even have an S_IFIFO mode.  Since there is confusion about what the
	 * mode is, we check for what it is not instead of what it is. 
	 */
	switch (st.st_mode & S_IFMT) {
	case S_IFCHR:
	case S_IFREG:
	case S_IFLNK:
	case S_IFDIR:
	case S_IFBLK:
		return (0);
	default:
		return (1);
	}
}

/*
 * Server procedure switch routine 
 */
mnt(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	switch (rqstp->rq_proc) {
	case NULLPROC:
		errno = 0;
		if (!svc_sendreply(transp, xdr_void, (char *)0))
			log_cant_reply(transp);
		return;
	case MOUNTPROC_MNT:
		mount(rqstp);
		return;
	case MOUNTPROC_DUMP:
		errno = 0;
		if (!svc_sendreply(transp, xdr_mountlist, (char *)&mountlist))
			log_cant_reply(transp);
		return;
	case MOUNTPROC_UMNT:
		umount(rqstp);
		return;
	case MOUNTPROC_UMNTALL:
		umountall(rqstp);
		return;
	case MOUNTPROC_EXPORT:
	case MOUNTPROC_EXPORTALL:
		export(rqstp);
		return;
	case MOUNTPROC_PATHCONF:
		if (rqstp->rq_vers == MOUNTVERS_POSIX) {
			mnt_pathconf(rqstp);
			return;
		}
		/* else fall through to error out */
	default:
		svcerr_noproc(transp);
		return;
	}
}

struct hostent *
getclientsname(transp)
	SVCXPRT *transp;
{
	struct sockaddr_in actual;
	struct hostent *hp;
	static struct hostent h;
	static struct in_addr prev;
	static char *null_alias;

	actual = *svc_getcaller(transp);
	if (nfs_portmon) {
		if (ntohs(actual.sin_port) >= IPPORT_RESERVED) {
			return (NULL);
		}
	}
	/*
	 * Don't use the unix credentials to get the machine name,
	 * instead use the source IP address. 
	 * Used cached hostent if previous call was for the
	 * same client.
	 */

	if (bcmp(&actual.sin_addr, &prev, sizeof(struct in_addr)) == 0)
		return (&h);
	prev = actual.sin_addr;

	hp = gethostbyaddr((char *) &actual.sin_addr, sizeof(actual.sin_addr), 
			   AF_INET);
	if (hp == NULL) {			/* dummy one up */
		h.h_name = inet_ntoa(actual.sin_addr);
		h.h_aliases = &null_alias;
		h.h_addrtype = AF_INET;
		h.h_length = sizeof (u_long);
		hp = &h;
	} else {
		bcopy(hp, &h, sizeof(struct hostent));
	}

	return (hp);
}

void
log_cant_reply(transp)
	SVCXPRT *transp;
{
	int saverrno;
	struct sockaddr_in actual;
	register struct hostent *hp;
	register char *name;

	saverrno = errno;	/* save error code */
	actual = *svc_getcaller(transp);
	/*
	 * Don't use the unix credentials to get the machine name, instead use
	 * the source IP address. 
	 */
	if ((hp = gethostbyaddr(&actual.sin_addr, sizeof(actual.sin_addr),
	   AF_INET)) != NULL)
		name = hp->h_name;
	else
		name = inet_ntoa(actual.sin_addr);

	errno = saverrno;
	if (errno == 0)
		syslog(LOG_ERR, "couldn't send reply to %s", name);
	else
		syslog(LOG_ERR, "couldn't send reply to %s: %m", name);
}

/*
 * Answer pathconf questions for the mount point fs
 */
void
mnt_pathconf(rqstp)
	struct svc_req *rqstp;
{
	SVCXPRT *transp;
	struct pathcnf p;
	char *path, rpath[MAXPATHLEN];
	char *gr, *grl;
	struct exportent *xent;
	struct exportent *findentry();
	struct stat st;
	char **aliases;
	struct hostent *client;

	transp = rqstp->rq_xprt;
	path = NULL;
	bzero((caddr_t)&p, sizeof(p));
	client = getclientsname(transp);
	if (client == NULL) {
		_PC_SET(_PC_ERROR, p.pc_mask);
		goto done;	
	}
	if (!svc_getargs(transp, xdr_path, &path)) {
		svcerr_decode(transp);
		return;
	}
	if (lstat(path, &st) < 0) {
		_PC_SET(_PC_ERROR, p.pc_mask);
		goto done;	
	}
	/*
	 * Get a path without symbolic links.
	 */
	if (realpath(path, rpath) == NULL) {
		syslog(LOG_DEBUG,
			"mount request: realpath failed on %s: %m",
			path);
		_PC_SET(_PC_ERROR, p.pc_mask);
		goto done;
	}
	bzero(&p, sizeof(p));
	/*
	 * can't ask about devices over NFS
	 */
	_PC_SET(_PC_MAX_CANON, p.pc_mask);
	_PC_SET(_PC_MAX_INPUT, p.pc_mask);
	_PC_SET(_PC_PIPE_BUF, p.pc_mask);
	_PC_SET(_PC_VDISABLE, p.pc_mask);

	errno = 0;
	p.pc_link_max = pathconf(rpath, _PC_LINK_MAX);
	if (errno)
		_PC_SET(_PC_LINK_MAX, p.pc_mask);
	p.pc_name_max = pathconf(rpath, _PC_NAME_MAX);
	if (errno)
		_PC_SET(_PC_NAME_MAX, p.pc_mask);
	p.pc_path_max = pathconf(rpath, _PC_PATH_MAX);
	if (errno)
		_PC_SET(_PC_PATH_MAX, p.pc_mask);
	if (pathconf(rpath, _PC_NO_TRUNC) == 1)
		_PC_SET(_PC_NO_TRUNC, p.pc_mask);
	if (pathconf(rpath, _PC_CHOWN_RESTRICTED) == 1)
		_PC_SET(_PC_CHOWN_RESTRICTED, p.pc_mask);

done:
	errno = 0;
	if (!svc_sendreply(transp, xdr_pathcnf, (char *)&p))
		log_cant_reply(transp);
	if (path != NULL)
		svc_freeargs(transp, xdr_path, &path);
	return;
}

/*
 * Check mount requests, add to mounted list if ok 
 */
void
mount(rqstp)
	struct svc_req *rqstp;
{
	SVCXPRT *transp;
	fhandle_t fh;
	struct fhstatus fhs;
	char *path, rpath[MAXPATHLEN];
	struct mountdlist *ml;
	char *gr, *grl;
	struct exportent *xent;
	struct exportent *findentry();
	struct stat st;
	char **aliases;
	struct hostent *client;

	transp = rqstp->rq_xprt;
	path = NULL;
	fhs.fhs_status = 0;
	client = getclientsname(transp);
	if (client == NULL) {
		fhs.fhs_status = EACCES;
		goto done;	
	}
	if (!svc_getargs(transp, xdr_path, &path)) {
		svcerr_decode(transp);
		return;
	}
	if (lstat(path, &st) < 0) {
		fhs.fhs_status = EACCES;
		goto done;	
	}

	/*
	 * Get a path without symbolic links.
	 */
	if (realpath(path, rpath) == NULL) {
		syslog(LOG_DEBUG,
			"mount request: realpath failed on %s: %m",
			path);
		fhs.fhs_status = EACCES;
		goto done;
	}

	if (do_getfh(rpath, &fh) < 0) {
		fhs.fhs_status = errno == EINVAL ? EACCES : errno;
		syslog(LOG_DEBUG, "mount request: getfh failed on %s: %m",
		    path);
		goto done;
	}

	xent = findentry(rpath);
	if (xent == NULL) {
		fhs.fhs_status = EACCES;
		goto done;
	}

	/* Check access list - hostnames first */

	grl = getexportopt(xent, ACCESS_OPT);
	if (grl == NULL)
		goto done;

	while ((gr = strtok(grl, ":")) != NULL) {
		grl = NULL;
		if (strcmp(gr, client->h_name) == 0)
			goto done;
		for (aliases = client->h_aliases; *aliases != NULL;
		     aliases++) {
			if (strcmp(gr, *aliases) == 0)
				goto done;
		}
	}
	
	/* no hostname match - try netgroups */

	grl = getexportopt(xent, ACCESS_OPT);
	if (grl == NULL)
		goto done;

	while ((gr = strtok(grl, ":")) != NULL) {
		grl = NULL;
		if (in_netgroup(gr, client->h_name, domain))
			goto done;
		for (aliases = client->h_aliases; *aliases != NULL;
		     aliases++) {
			if (in_netgroup(gr, *aliases, domain))
				goto done;
		}
	}

	/* Check root and rw lists */

	grl = getexportopt(xent, ROOT_OPT);
	if (grl != NULL) {
		while ((gr = strtok(grl, ":")) != NULL) {
			grl = NULL;
			if (strcmp(gr, client->h_name) == 0)
				goto done;
		}
	}
	grl = getexportopt(xent, RW_OPT);
	if (grl != NULL) {
		while ((gr = strtok(grl, ":")) != NULL) {
			grl = NULL;
			if (strcmp(gr, client->h_name) == 0)
				goto done;
		}
	}
	fhs.fhs_status = EACCES;

done:
	if (fhs.fhs_status == 0)
		fhs.fhs_fh = fh;
	errno = 0;
	if (!svc_sendreply(transp, xdr_fhstatus, (char *)&fhs))
		log_cant_reply(transp);
	if (path != NULL)
		svc_freeargs(transp, xdr_path, &path);
	if (fhs.fhs_status)
		return;

	/*
	 *  Add an entry for this mount to the mount list.
	 *  First check whether it's there already - the client
	 *  may have crashed and be rebooting.
	 */
	for (ml = mountlist; ml != NULL ; ml = ml->ml_nxt) {
		if (strcmp(ml->ml_path, rpath) == 0) {
			if (strcmp(ml->ml_name, client->h_name) == 0) {
				return;
			}
			for (aliases = client->h_aliases; *aliases != NULL;
			     aliases++) {
				if (strcmp(ml->ml_name, *aliases) == 0) {
					return;
				}
			}
		}
	}

	/*
	 * Add this mount to the mount list.
	 */
	ml = (struct mountdlist *) exmalloc(sizeof(struct mountdlist));
	ml->ml_path = (char *) exmalloc(strlen(rpath) + 1);
	(void) strcpy(ml->ml_path, rpath);
	ml->ml_name = (char *) exmalloc(strlen(client->h_name) + 1);
	(void) strcpy(ml->ml_name, client->h_name);
	ml->ml_nxt = mountlist;
	ml->ml_pos = rmtab_insert(client->h_name, rpath);
	mountlist = ml;
	return;
}

struct exportent *
findentry(path)
	char *path;
{
	struct exportent *xent;
	struct xent_list *xp;
	register char *p1, *p2;

	check_exports();

	for (xp = xent_list ; xp ; xp = xp->x_next) {
		xent = &xp->x_xent;
		for (p1 = xent->xent_dirname, p2 = path ; *p1 == *p2 ; p1++, p2++)
			if (*p1 == '\0')
				return xent;	/* exact match */

		if ((*p1 == '\0' && *p2 == '/' ) ||
		    (*p1 == '\0' && *(p1-1) == '/') ||
		    (*p2 == '\0' && *p1 == '/' && *(p1+1) == '\0')) {
			if (issubdir(path, xent->xent_dirname))
				return xent;
		}
	}
	return ((struct exportent *)0);
}

#define MAXGRPLIST 256
/*
 * Use cached netgroup info if the previous call was
 * from the same client.  Two lists are maintained
 * here: groups the client is a member of, and groups
 * the client is not a member of.
 */
int
in_netgroup(group, hostname, domain)
	char *group, *hostname, *domain;
{
	static char prev_hostname[MAXHOSTNAMELEN+1];
	static char grplist[MAXGRPLIST+1], nogrplist[MAXGRPLIST+1];
	char key[256];
	char *ypline = NULL;
	int yplen;
	register char *gr, *p;
	static time_t last;
	time_t time();
	time_t time_now;
	static int cache_time = 30; /* sec */

	time_now = time((long *)0);
	if (time_now > last + cache_time ||
	    strcmp(hostname, prev_hostname) != 0) {
		last = time_now;
		(void) strcpy(key, hostname);
		(void) strcat(key, ".");
		(void) strcat(key, domain);
		if (yp_match(domain, "netgroup.byhost", key,
		    strlen(key), &ypline, &yplen) == 0) {
			(void) strncpy(grplist, ypline, MIN(yplen, MAXGRPLIST));
			free(ypline);
		} else {
			grplist[0] = '\0';
		}
		nogrplist[0] = '\0';
		(void) strcpy(prev_hostname, hostname);
	}

	for (gr = grplist ; *gr ; gr = p ) {
		for (p = gr ; *p && *p != ',' ; p++)
			;
		if (strncmp(group, gr, p - gr) == 0)
			return 1;
		if (*p == ',')
			p++;
	}
	for (gr = nogrplist ; *gr ; gr = p ) {
		for (p = gr ; *p && *p != ',' ; p++)
			;
		if (strncmp(group, gr, p - gr) == 0)
			return 0;
		if (*p == ',')
			p++;
	}

	if (innetgr(group, hostname, (char *)NULL, domain)) {
		if (strlen(grplist)+1+strlen(group) > MAXGRPLIST)
			return 1;
		if (*grplist)
			(void) strcat(grplist, ",");
		(void) strcat(grplist, group);
		return 1;
	} else {
		if (strlen(nogrplist)+1+strlen(group) > MAXGRPLIST)
			return 1;
		if (*nogrplist)
			(void) strcat(nogrplist, ",");
		(void) strcat(nogrplist, group);
		return 0;
	}
}

void
check_exports()
{
	FILE *f;
	struct stat st;
	static long last_xtab_time;
	struct exportent *xent;
	struct xent_list *xp, *xp_prev;
	char rpath[MAXPATHLEN];

	/*
	 *  read in /etc/xtab if it has changed 
	 */

	if (stat(XTAB, &st) != 0) {
		syslog(LOG_ERR, "Cannot stat %s: %m", XTAB);
		return;
	}
	if (st.st_mtime == last_xtab_time)
		return;				/* no change */

	xent_free(xent_list);			/* free old list */
	xent_list = NULL;
	
	f = setexportent();
	if (f == NULL)
		return;

	while (xent = getexportent(f)) {
		/*
		 * Get a path without symbolic links.
		 */
		if (realpath(xent->xent_dirname, rpath) == NULL) {
			syslog(LOG_ERR,
				"check_exports: realpath failed on %s: %m",
				xent->xent_dirname);
			continue;
		}

		xp = (struct xent_list *)malloc(sizeof(struct xent_list));
		if (xp == NULL)
			goto alloc_failed;
		if (xent_list == NULL)
			xent_list = xp;
		else
			xp_prev->x_next = xp;
		xp_prev = xp;
		bzero((char *)xp, sizeof(struct xent_list));
		xp->x_xent.xent_dirname = strdup(rpath);
		if (xp->x_xent.xent_dirname == NULL)
			goto alloc_failed;
		if (xent->xent_options) {
			xp->x_xent.xent_options = strdup(xent->xent_options);
			if (xp->x_xent.xent_options == NULL)
				goto alloc_failed;
		}
	}
	endexportent(f);
	last_xtab_time = st.st_mtime;
	return;

alloc_failed:
	syslog(LOG_ERR, "Memory allocation failed: %m");
	xent_free(xent_list);
	xent_list = NULL;
	endexportent(f);
	return;
}

void
xent_free(xp)
	struct xent_list *xp;
{
	register struct xent_list *next;

	while (xp) {
		if (xp->x_xent.xent_dirname)
			free(xp->x_xent.xent_dirname);
		if (xp->x_xent.xent_options)
			free(xp->x_xent.xent_options);
		next = xp->x_next;
		free((char *)xp);
		xp = next;
	}
}


/*
 * Remove an entry from mounted list 
 */
umount(rqstp)
	struct svc_req *rqstp;
{
	char *path;
	struct mountdlist *ml, *oldml;
	struct hostent *client;
	SVCXPRT *transp;

	transp = rqstp->rq_xprt;
	path = NULL;
	if (!svc_getargs(transp, xdr_path, &path)) {
		svcerr_decode(transp);
		return;
	}
	errno = 0;
	if (!svc_sendreply(transp, xdr_void, (char *)NULL))
		log_cant_reply(transp);

	client = getclientsname(transp);
	if (client != NULL) {
		oldml = mountlist;
		for (ml = mountlist; ml != NULL;
		     oldml = ml, ml = ml->ml_nxt) {
			if (strcmp(ml->ml_path, path) == 0 &&
			    strcmp(ml->ml_name, client->h_name) == 0) {
				if (ml == mountlist) {
					mountlist = ml->ml_nxt;
				} else {
					oldml->ml_nxt = ml->ml_nxt;
				}
				rmtab_delete(ml->ml_pos);
				free(ml->ml_path);
				free(ml->ml_name);
				free((char *)ml);
				break;
			}
		}
	}
	svc_freeargs(transp, xdr_path, &path);
}

/*
 * Remove all entries for one machine from mounted list 
 */
umountall(rqstp)
	struct svc_req *rqstp;
{
	struct mountdlist *ml, *oldml;
	struct hostent *client;
	SVCXPRT *transp;

	transp = rqstp->rq_xprt;
	if (!svc_getargs(transp, xdr_void, NULL)) {
		svcerr_decode(transp);
		return;
	}
	/*
	 * We assume that this call is asynchronous and made via the portmapper
	 * callit routine.  Therefore return control immediately. The error
	 * causes the portmapper to remain silent, as apposed to every machine
	 * on the net blasting the requester with a response. 
	 */
	svcerr_systemerr(transp);
	client = getclientsname(transp);
	if (client == NULL) {
		return;
	}
	oldml = mountlist;
	for (ml = mountlist; ml != NULL; ml = ml->ml_nxt) {
		if (strcmp(ml->ml_name, client->h_name) == 0) {
			if (ml == mountlist) {
				mountlist = ml->ml_nxt;
				oldml = mountlist;
			} else {
				oldml->ml_nxt = ml->ml_nxt;
			}
			rmtab_delete(ml->ml_pos);
			free(ml->ml_path);
			free(ml->ml_name);
			free((char *)ml);
		} else {
			oldml = ml;
		}
	}
}

/*
 * send current export list 
 */
export(rqstp)
	struct svc_req *rqstp;
{
	struct exportent *xent;
	struct exports *ex;
	struct exports **tail;
	char *grl;
	char *gr;
	struct groups *groups;
	struct groups **grtail;
	SVCXPRT *transp;
	struct xent_list *xp;

	transp = rqstp->rq_xprt;
	if (!svc_getargs(transp, xdr_void, NULL)) {
		svcerr_decode(transp);
		return;
	}

	check_exports();

	ex = NULL;
	tail = &ex;
	for (xp = xent_list ; xp ; xp = xp->x_next) {
		xent = &xp->x_xent;

		grl = getexportopt(xent, ACCESS_OPT);
		groups = NULL;
		if (grl != NULL) {
			grtail = &groups;
			while ((gr = strtok(grl, ":")) != NULL) {
				grl = NULL;
				grtail = newgroup(gr, grtail);
			}
		}
		tail = newexport(xent->xent_dirname, groups, tail);
	}

	errno = 0;
	if (!svc_sendreply(transp, xdr_exports, (char *)&ex))
		log_cant_reply(transp);
	freeexports(ex);
}


freeexports(ex)
	struct exports *ex;
{
	struct groups *groups, *tmpgroups;
	struct exports *tmpex;

	while (ex) {
		groups = ex->ex_groups;
		while (groups) {
			tmpgroups = groups->g_next;
			free(groups->g_name);
			free((char *)groups);
			groups = tmpgroups;
		}
		tmpex = ex->ex_next;
		free(ex->ex_name);
		free((char *)ex);
		ex = tmpex;
	}
}


struct groups **
newgroup(name, tail)
	char *name;
	struct groups **tail;
{
	struct groups *new;
	char *newname;

	new = (struct groups *) exmalloc(sizeof(*new));
	newname = (char *) exmalloc(strlen(name) + 1);
	(void) strcpy(newname, name);

	new->g_name = newname;
	new->g_next = NULL;
	*tail = new;
	return (&new->g_next);
}


struct exports **
newexport(name, groups, tail)
	char *name;
	struct groups *groups;
	struct exports **tail;
{
	struct exports *new;
	char *newname;

	new = (struct exports *) exmalloc(sizeof(*new));
	newname = (char *) exmalloc(strlen(name) + 1);
	(void) strcpy(newname, name);

	new->ex_name = newname;
	new->ex_groups = groups;
	new->ex_next = NULL;
	*tail = new;
	return (&new->ex_next);
}

char *
exmalloc(size)
	int size;
{
	char *ret;

	if ((ret = (char *) malloc((u_int)size)) == 0) {
		syslog(LOG_ERR, "Out of memory");
		exit(1);
	}
	return (ret);
}

usage()
{
	(void) fprintf(stderr, "usage: rpc.mountd [-n]\n");
	exit(1);
}

/*
 * Old geth() took a file descriptor. New getfh() takes a pathname.
 * So the the mount daemon can run on both old and new kernels, we try
 * the old version of getfh() if the new one fails.
 */
do_getfh(path, fh)
	char *path;
	fhandle_t *fh;
{
	int fd;
	int res;
	int save;

	res = getfh(path, fh);
	if (res < 0 && errno == EBADF) {	
		/*
		 * This kernel does not have the new-style getfh()
		 */
		fd = open(path, 0, 0);
		if (fd >= 0) {
			res = getfh((char *)fd, fh);
			save = errno;
			(void) close(fd);
			errno = save;
		}
	}
	return (res);
}


FILE *f;

void
rmtab_load()
{
	char *path;
	char *name;
	char *end;
	struct mountdlist *ml;
	char line[MAXRMTABLINELEN];

	f = fopen(RMTAB, "r");
	if (f != NULL) {
		while (fgets(line, sizeof(line), f) != NULL) {
			name = line;
			path = strchr(name, ':');
			if (*name != '#' && path != NULL) {
				*path = 0;
				path++;
				end = strchr(path, '\n');
				if (end != NULL) {
					*end = 0;
				}
				ml = (struct mountdlist *) 
					exmalloc(sizeof(struct mountdlist));
				ml->ml_path = (char *)
					exmalloc(strlen(path) + 1);
				(void) strcpy(ml->ml_path, path);
				ml->ml_name = (char *)
					exmalloc(strlen(name) + 1);
				(void) strcpy(ml->ml_name, name);
				ml->ml_nxt = mountlist;
				mountlist = ml;
			}
		}
		(void) fclose(f);
		(void) truncate(RMTAB, (off_t)0);
	} 
	f = fopen(RMTAB, "w+");
	if (f != NULL) {
		setlinebuf(f);
		for (ml = mountlist; ml != NULL; ml = ml->ml_nxt) {
			ml->ml_pos = rmtab_insert(ml->ml_name, ml->ml_path);
		}
	}
}


long
rmtab_insert(name, path)
	char *name;
	char *path;
{
	long pos;

	if (f == NULL || fseek(f, 0L, 2) == -1) {
		return (-1);
	}
	pos = ftell(f);
	if (fprintf(f, "%s:%s\n", name, path) == EOF) {
		return (-1);
	}
	return (pos);
}

void
rmtab_delete(pos)
	long pos;
{
	if (f != NULL && pos != -1 && fseek(f, pos, 0) == 0) {
		(void) fprintf(f, "#");
		(void) fflush(f);
	}
}

