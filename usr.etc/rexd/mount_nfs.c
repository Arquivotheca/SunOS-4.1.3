# ifdef lint
static char sccsid[] = "@(#)mount_nfs.c 1.1 92/07/30 Copyr 1985 Sun Micro";
# endif lint

/*
 *  mount_nfs.c - procedural interface to the NFS mount operation
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

# define NFSCLIENT
/** # define OLDMOUNT /** for 2.0 systems only **/

#include <sys/param.h>
#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <nfs/nfs.h>
# ifndef OLDMOUNT
#include <sys/mount.h>
# endif OLDMOUNT
#include <rpcsvc/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdio.h>
#include <mntent.h>
#include <sys/vfs.h>

#define MAXSLEEP 1  /* in seconds */
char *sys_errlist[];
int sys_nerr;

/*
 * mount_nfs - mount a file system using NFS
 *
 * Returns: 0 if OK, 1 if error.  
 * 	The "error" string returns the error message.
 */
mount_nfs(fsname, dir, error)
	char *fsname;
	char *dir;
	char *error;
{
	struct sockaddr_in sin;
	struct hostent *hp;
	struct fhstatus fhs;
	char host[256];
	char *path;
	char opts[32];
	struct stat st;
	int s = -1;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
	int printed1 = 0;
	int printed2 = 0;
	unsigned winks = 0;  /* seconds of sleep time */
	extern errno;
	struct mntent mnt;
	char *index();
	FILE *mnted;
# ifndef OLDMOUNT
	struct nfs_args args;
# endif OLDMOUNT

	path = index(fsname, ':');
	if (path==NULL) {
		errprintf(error,"No host name in %s\n", fsname);
		return(1);
	}
	*path++ = '\0';
	strcpy(host,fsname);
	path[-1] = ':';
	if (*path == '\0') {
	     /*
	      * handle the special case of importing a root file system
	      */
	      strcpy(path,"/");
	}
	/*
	 * Get server's address
	 */
	if ((hp = gethostbyname(host)) == NULL) {
		errprintf(error,
		    "mount %s: %s not in hosts database\n", fsname, host);
		return (1);
	}

	/*
	 * get fhandle of remote path from server's mountd.
	 * We have to use short timeouts since otherwise our incoming
	 * RPC call will time out.
	 */
	do {
		bzero(&sin, sizeof(sin));
		bcopy(hp->h_addr, (char *) & sin.sin_addr, hp->h_length);
		sin.sin_family = AF_INET;
		timeout.tv_usec = 0;
		timeout.tv_sec = 5;
		s = -1;
		do {
		    if (sin.sin_port==0) {
		    	/*
			 * Ask portmapper for port number since
			 * the standard pmap_getport has too long a timeout
			 */
			int ps = -1;
			struct pmap parms;
			short port;

			sin.sin_port = htons(PMAPPORT);
			client = clntudp_create(&sin, PMAPPROG, PMAPVERS,
				timeout, &ps);
			if (client==NULL) {
			    if (winks < MAXSLEEP)
				winks++;
			    else {
			      errprintf(error,
			          "can not map port for %s\n", host);
			      return(1);
			    }
			    sleep(winks);
			    sin.sin_port = 0;
			    continue;
			}
			parms.pm_prog = MOUNTPROG;
			parms.pm_vers = MOUNTVERS;
			parms.pm_prot = IPPROTO_UDP;
			parms.pm_port = 0;
			if (clnt_call(client, PMAPPROC_GETPORT, xdr_pmap,
			    &parms, xdr_u_short, &port, timeout)
			       != RPC_SUCCESS) {
			    if (winks < MAXSLEEP)
				winks++;
			    else {
			      errprintf(error,
			          "%s not responding to port map request\n", 
				   host);
			      clnt_destroy(client);
			      return(1);
			    }
			    if (!printed1++) {
				fprintf(stderr,
			    "rexd portmap: %s not responding", host);
				clnt_perror(client,"");
				fprintf(stderr,"\r");
			    }
			    sleep(winks);
			    clnt_destroy(client);
			    client = NULL;
			    close(ps);
			    sin.sin_port = 0;
			    continue;
			}
			sin.sin_port = ntohs(port);
			clnt_destroy(client);
			close(ps);
		    }

		    if ((client = clntudp_create(&sin,
			    MOUNTPROG, MOUNTVERS, timeout, &s)) == NULL) {
			if (winks < MAXSLEEP)
				winks++;
			else {
			  if (rpc_createerr.cf_stat == RPC_PROGNOTREGISTERED)
			      errprintf(error,
			          "%s is not running a mount daemon\n", host);
			  else
			  	errprintf(error,
				  "%s not responding to mount request\n", 
				    host);
			  return(1);
			}
			sleep(winks);
			if (!printed1++) {
				fprintf(stderr,
			    "rexd mount: %s not responding", host);
				clnt_pcreateerror("");
				fprintf(stderr,"\r");
			}
		    }
		} while (client == NULL);
		client->cl_auth = authunix_create_default();
		timeout.tv_usec = 0;
		timeout.tv_sec = 25;
		rpc_stat = clnt_call(client, MOUNTPROC_MNT, xdr_path, &path,
		    xdr_fhstatus, &fhs, timeout);
		if (rpc_stat != RPC_SUCCESS) {
			if (!printed2++) {
				fprintf(stderr,
				    "rexd mount: %s not responding", host);
				clnt_perror(client, "");
				fprintf(stderr,"\r");
			}
		}
		close(s);
		clnt_destroy(client);
	} while (rpc_stat == RPC_TIMEDOUT || fhs.fhs_status == ETIMEDOUT);

	if (rpc_stat != RPC_SUCCESS || fhs.fhs_status) {
		errno = fhs.fhs_status;
		if (errno == EACCES) {
			errprintf(error,
			  "rexd mount: not in export list for %s\n",
			    fsname);
		}
		return (1);
	}
	if (printed1 || printed2) {
		fprintf(stderr, "rexd mount: %s OK\n", host);
		fprintf(stderr,"\r");
	}

	/*
	 * remote mount the fhandle on the local path.
	 */
# ifdef OLDMOUNT
	if (nfsmount(&sin, &fhs.fhs_fh, dir, 0, 0, 0) <0) {
# else OLDMOUNT
	sin.sin_port = htons(NFS_PORT);
	args.addr = &sin;
	args.fh = (caddr_t) &fhs.fhs_fh;
	args.flags = NFSMNT_HOSTNAME | NFSMNT_INT;
	args.hostname = host;
	if (mount("nfs", dir, M_NOSUID | M_NEWTYPE, &args) < 0) {
# endif OLDMOUNT
		errprintf(error,"unable to mount %s on %s: %s\n",
			fsname, dir, 
			errno > sys_nerr ? "unknown error" : 
			        sys_errlist[errno]);
		return (1);
	}

	/*
	 * stat the new mount and get its dev
	 */
	if (stat(dir, &st) < 0) {
		errprintf(error, "couldn't stat %s\n", dir);
		return (1);
	}
	(void) sprintf(opts, "rw,noquota,hard,intr,%s=%04x",
		MNTINFO_DEV, (st.st_dev & 0xFFFF));

	/*
	 * update /etc/mtab
	 */
	mnt.mnt_fsname = fsname;
	mnt.mnt_dir = dir;
	mnt.mnt_type = MNTTYPE_NFS;
	mnt.mnt_opts = opts;
	mnt.mnt_freq = 0;
	mnt.mnt_passno = 0;
	newmtab(&mnt, (char *)NULL);
	return (0);
}

#define UNMOUNTTRIES 6
/*
 * umount_nfs - unmount a file system when finished
 */
umount_nfs(fsname, dir)
	char *fsname, *dir;
{
	char *p, *index();
	struct sockaddr_in sin;
	struct hostent *hp;
	int s = -1;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
	int count = 0;
		
	/*
	 * Give the filesystem time to become un-busy when unmounting.  
	 * If child aborted and is takes a core dump, we may receive the
	 * SIGCHLD before the core dump is completed.
	 */
	while (unmount(dir) == -1) {
		if (errno != EBUSY) {
			perror(dir);
			return (1);
		}
		if (++count > UNMOUNTTRIES)
			return (1);
		sleep (10);
        }

	newmtab((struct mntent *)NULL, dir);
	if ((p = index(fsname, ':')) == NULL)
		return(1);
	*p++ = 0;
	if ((hp = gethostbyname(fsname)) == NULL) {
		fprintf(stderr, "%s not in hosts database\n", fsname);
		return(1);
	}
	bzero(&sin, sizeof(sin));
	bcopy(hp->h_addr, (char *) & sin.sin_addr, hp->h_length);
	sin.sin_family = AF_INET;
	timeout.tv_usec = 0;
	timeout.tv_sec = 10;
	if ((client = clntudp_create(&sin, MOUNTPROG, MOUNTVERS,
	    timeout, &s)) == NULL) {
		clnt_pcreateerror("Warning on umount create:");
		fprintf(stderr,"\r");
		return(1);
	}
	client->cl_auth = authunix_create_default();
	timeout.tv_usec = 0;
	timeout.tv_sec = 25;
	rpc_stat = clnt_call(client, MOUNTPROC_UMNT, xdr_path, &p,
	    xdr_void, NULL, timeout);
	if (rpc_stat != RPC_SUCCESS) {
		clnt_perror(client, "Warning: umount:");
		fprintf(stderr,"\r");
		return(1);
	}
	return(0);
}

#define MAXTRIES 300

newmtab(addmnt, deldir)
	struct mntent *addmnt;
	char *deldir;
{
	char *from = "/etc/omtabXXXXXX";
	char *to = "/etc/nmtabXXXXXX";
	FILE *fromp, *top;
	int count = 0;
	struct mntent *mnt;
	struct stat statbuf;
	
	mktemp(from);
	while (rename(MOUNTED, from)) {
		if (++count > MAXTRIES) {
			fprintf(stderr, "newmtab: cat get %s lock\r\n", MOUNTED);
			return;
		}
		sleep(1);
	}
	fromp = setmntent(from, "r");
	mktemp(to);
	top = setmntent(to, "w");
	if (fromp == NULL || top == NULL) {
		rename(from, MOUNTED);
		fprintf(stderr, "newmtab: can't open %s or %s\r\n", from, to);
		return;
	}
	while ((mnt = getmntent(fromp)) != NULL)
		if (deldir == NULL || strcmp(mnt->mnt_dir, deldir) != 0)
			addmntent(top, mnt);
	if (addmnt)
		addmntent(top, addmnt);
	if (!fstat (fileno (fromp), &statbuf))
		(void) fchmod (fileno (top), statbuf.st_mode);
	endmntent(fromp);
	endmntent(top);

	if (rename(to, MOUNTED)) {
		fprintf(stderr, "newmtab can not rename %s to %s\r\n", 
			to, MOUNTED);
		rename(from, MOUNTED);
	}
	unlink(from);
}

/*
 * parsefs - given a name of the form host:/path/name/for/file
 *	connect to the give host and look for the exported file system
 *	that matches.  
 * Returns: pointer to string containing the part of the pathname
 *	within the exported directory.
 *	Returns NULL on errors.
 */
char *
parsefs(fullname,error)
	char *fullname;
	char *error;
{
	char *dir, *subdir;
        struct exports *ex = NULL;
	int err;
	int bestlen = 0;
	int len, dirlen;

	dir = index(fullname, ':');
	if (dir == NULL) {
		errprintf(error,"No host name in %s\n",fullname);
		return(NULL);
	}
	*dir++ = '\0';

	if (err = callrpc(fullname, MOUNTPROG, MOUNTVERS, MOUNTPROC_EXPORT,
	    xdr_void, 0, xdr_exports, &ex)) {
	    	if (err== (int)RPC_TIMEDOUT)
			errprintf(error, "Host %s is not running mountd\n",
				fullname);
		else
			errprintf(error,"RPC error %d with host %s\n",
				err,fullname);
		return(NULL);
	}
	dirlen = strlen(dir);
	for (; ex; ex = ex->ex_next) {
		len = strlen(ex->ex_name);
		if (len > bestlen && len <= dirlen &&
		    strncmp(dir, ex->ex_name, len) == 0 &&
		    (dir[len] == '/' || dir[len] == '\0'))
			bestlen = len;
	}
	if (bestlen == 0) {
		errprintf(error, "%s not exported by %s\n", 
			dir, fullname);
		return(NULL);
	}
	if (dir[bestlen] == '\0')
		subdir = &dir[bestlen];
	else {
		dir[bestlen] = '\0';
		subdir = &dir[bestlen+1];
	}
	*--dir = ':';
	return (subdir);
}


/*
 * errprintf will print a message to standard error channel,
 * and return the same string in the given message buffer.
 * We add an extra return character in case the console is in raw mode.
 */
errprintf(s,p1,p2,p3,p4,p5,p6,p7)
	char *s;
{
	sprintf(s,p1,p2,p3,p4,p5,p6,p7);
	fprintf(stderr,"%s\r",s);
}
