#ifndef lint
static char sccsid[] = "@(#)nse_tfs_mount.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 * Routines to mount/unmount tfs directories
 */

#include <nse/param.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <nse/util.h>
#include <stdio.h>
#include <strings.h>
#include <netdb.h>
#include <mntent.h>
#include <nse_impl/tfs_user.h>
#include <nse_impl/tfs.h>
#define NFS
#define NFSCLIENT
#define LOFS
#include <nse/mount.h>
#include <rpcsvc/mount.h>
#include <nse/nse_rpc.h>
#include <nse/tfs_calls.h>
#include <nse/util.h>
#include <nse/tfs_mount.h>

#ifdef USE_SITE
extern char	my_site[];
#endif USE_SITE
#ifdef DEBUG
extern bool_t	soft_mounts;
#endif DEBUG

extern void	nse_tfs_addr();

Nse_err		nse_tfs_mount();
Nse_err		nse_tfs_unmount();
static Nse_err	old_tfs_mount();
static Nse_err	old_tfs_unmount();
static Nse_err	mount_tfs();
static Nse_err	nfs_mount();
Nse_err		nse_mount_loopback();
static int	nopt();


/*
 * Mount the directory 'ma->directory' on the directory 'mount_pt'.
 * 'ma->tfs_mount_pt' is the tfsd's name for the mount pt.  (See comments
 * for struct Mntlist in active_list.h for example values of 'directory',
 * 'mount_pt', and 'tfs_mount_pt'.)
 */
Nse_err
nse_tfs_mount(ma, mount_pt, host, read_only, mntp, version)
	Tfs_mount_args	ma;
	char		*mount_pt;
	char		*host;
	bool_t		read_only;
	struct mntent	**mntp;
	u_long		version;
{
	char		hostname[NSE_MAXHOSTNAMELEN];
	Tfs_mount_res_rec mr;
	struct sockaddr_in *tfsd_addr;
	Nse_err		err;

	*mntp = NULL;
	if (access(mount_pt, 0) < 0) {
		return nse_err_format(NSE_TFS_NO_MOUNTPT);
	}
	if (version == TFS_VERSION) {
		err = nse_tfs_rpc_call_to_host(host, TFS_MOUNT,
					xdr_tfs_mount_args, (char *) ma,
					xdr_tfs_mount_res, (char *) &mr);
	} else {
		err = old_tfs_mount(ma, host, &mr);
	}
	if (err) {
		return err;
	}
	if (mr.status != 0) {
		if (mr.status > 0) {
			errno = mr.status;
			return nse_err_format_errno("TFS mount of \"%s\"",
						    ma->tfs_mount_pt);
		} else {
			return nse_err_format(mr.status);
		}
	}
	nse_tfs_addr(&tfsd_addr, version);
	if (version == TFS_VERSION) {
		sprintf(hostname, "%s(%d)", host, mr.pid);
	} else {
		sprintf(hostname, "%s tfs(%d)", host, mr.pid);
	}
	tfsd_addr->sin_port = htons((u_short) mr.port);
	err = mount_tfs(ma->directory, mount_pt, hostname, tfsd_addr, &mr.fh,
			read_only, mntp, version);
	if (err) {
		Tfs_unmount_args_rec ua;

		/*
		 * Don't call nse_err_save() here -- may overwrite an
		 * already-saved error.
		 */
		ua.environ = ma->environ;
		ua.tfs_mount_pt = ma->tfs_mount_pt;
		ua.hostname = _nse_hostname();
		ua.pid = getpid();
		(void) nse_tfs_unmount(&ua, host, version);
	}
	return err;
}


/*
 * Unmount the mount_pt 'tfs_mount_pt' for the env 'environ'.
 */
Nse_err
nse_tfs_unmount(ua, host, version)
	Tfs_unmount_args ua;
	char		*host;
	u_long		version;
{
	enum nfsstat	status;
	Nse_err		err;

	/*
	 * Make a TFS RPC call to the specified host.  Don't use the vanilla
	 * nse_tfs_rpc_call(), as it calls nse_branch_remote() to determine
	 * the host.  If the branch has been deleted and removed from NIS,
	 * this won't work.
	 */
	if (version == TFS_VERSION) {
		err = nse_tfs_rpc_call_to_host(host, TFS_UNMOUNT,
					xdr_tfs_unmount_args, (char *) ua,
					xdr_enum, (char *) &status);
	} else {
		err = old_tfs_unmount(ua, host, &status);
	}
	if (err) {
		return err;
	}
	if ((int) status != 0) {
		if ((int) status > 0) {
			errno = (int) status;
			return nse_err_format_errno("TFS unmount of \"%s\"",
						    ua->tfs_mount_pt);
		} else {
			return nse_err_format((int) status);
		}
	} else {
		return NULL;
	}
}


static Nse_err
old_tfs_mount(ma, host, mr)
	Tfs_mount_args	ma;
	char		*host;
	Tfs_mount_res	mr;
{
	Tfs_old_mount_args_rec old_ma;

	old_ma.environ = ma->environ;
	old_ma.tfs_mount_pt = ma->tfs_mount_pt;
	old_ma.directory = ma->directory;
	old_ma.hostname = ma->hostname;
	old_ma.pid = ma->pid;
	if (ma->back_read_only) {
		old_ma.writeable_layers = ma->writeable_layers;
	} else {
		old_ma.writeable_layers = 0;
	}
	return nse_tfs_rpc_call_to_host_1(host, TFS_OLD_MOUNT,
				xdr_tfs_old_mount_args, (char *) &old_ma,
				xdr_tfs_mount_res, (char *) mr);
}


static Nse_err
old_tfs_unmount(ua, host, statusp)
	Tfs_unmount_args ua;
	char		*host;
	enum nfsstat	*statusp;
{
	Tfs_old_mount_args_rec old_ua;

	old_ua.environ = ua->environ;
	old_ua.tfs_mount_pt = ua->tfs_mount_pt;
	old_ua.directory = "";
	old_ua.hostname = ua->hostname;
	old_ua.pid = ua->pid;
	return nse_tfs_rpc_call_to_host_1(host, TFS_OLD_UNMOUNT,
				xdr_tfs_old_mount_args, (char *) &old_ua,
				xdr_enum, (char *) statusp);
}


static Nse_err
mount_tfs(directory, tfs_mount_pt, server_name, server_addr, root_fhandle,
	  read_only, mntp, version)
	char		*directory;
	char		*tfs_mount_pt;
	char		*server_name;
	struct sockaddr_in *server_addr;
	fhandle_t	*root_fhandle;
	bool_t		read_only;
	struct mntent	**mntp;
	u_long		version;
{
	struct mntent	*mnt;
	char		*opts;
	Nse_err		err;

	mnt = (struct mntent *) nse_calloc(sizeof(struct mntent), 1);
	mnt->mnt_fsname = NSE_STRDUP(directory);
	mnt->mnt_dir = NSE_STRDUP(tfs_mount_pt);
	mnt->mnt_type = NSE_STRDUP("tfs");
	mnt->mnt_freq = 0;
	mnt->mnt_passno = 0;
	if (version == TFS_VERSION) {
		opts = "timeo=50";
	} else {
		opts = "timeo=50,wsize=2048";
	}
	err = nfs_mount(server_addr, root_fhandle, tfs_mount_pt, read_only,
			server_name, opts, mnt, version);
	if (!err) {
		*mntp = mnt;
	} else {
		nse_mntent_destroy(mnt);
	}
	return err;
}


/*
 * TFS mount the fhandle 'root_fhandle' served by the TFS server at
 * address 'server_addr' onto the directory 'mntpnt'.  (Do an NFS mount
 * if we're talking to a version 1 tfsd.)
 * Reads the NIS maps site.bynet and mountopts.bysitepair to determine
 * if there are any special mount options to use in mounting from the
 * remote machine.  'default_opts' contains any special mount options
 * which should be taken as the default values if the NIS maps don't
 * specify otherwise.
 * Fills in the options field for the /etc/mtab entry 'mnt'.
 */
static Nse_err
nfs_mount(server_addr, root_fhandle, mntpnt, read_only, server_name,
	  default_opts, mnt, version)
	struct sockaddr_in *server_addr;
	fhandle_t	*root_fhandle;
	char		*mntpnt;
	bool_t		read_only;
	char		*server_name;
	char		*default_opts;
	struct mntent	*mnt;
	u_long		version;
{
	struct nfs_args	args;
	int		flags = 0;
	char		opts[100];
#ifdef USE_SITE
	char		hissite[NSE_MAXSITENAMELEN];
	char		siteopts[100];
#endif USE_SITE

	if (read_only) {
		flags |= M_RDONLY;
		strcpy(opts, "ro");
	} else {
		strcpy(opts, "rw");
	}
#ifdef USE_SITE
	/*
	 * Get location-dependent mount options by using site information
	 */
	getsite(server_addr->sin_addr, hissite);
	getoptsbysites(my_site, hissite, siteopts);
	if (siteopts[0]) {
		strcat(opts, ",");
		strcat(opts, siteopts);
	}
#endif USE_SITE
	if (default_opts) {
		strcat(opts, ",");
		strcat(opts, default_opts);
	}
	bzero((char *)&args, sizeof args);
	args.addr = server_addr;
#ifdef SUN_OS_4
	args.fh = (caddr_t) root_fhandle;
#else
	args.fh = root_fhandle;
#endif SUN_OS_4
#ifdef DEBUG
	if (soft_mounts) {
		args.flags |= NFSMNT_SOFT;
		strcat(opts, ",soft");
	}
#endif DEBUG
	args.hostname = server_name;
	args.flags |= (NFSMNT_HOSTNAME | NFSMNT_INT);
	strcat(opts, ",intr");
	mnt->mnt_opts = opts;
	if (args.rsize = nopt(mnt, "rsize")) {
		args.flags |= NFSMNT_RSIZE;
	}
	if (args.wsize = nopt(mnt, "wsize")) {
		args.flags |= NFSMNT_WSIZE;
	}
	if (args.timeo = nopt(mnt, "timeo")) {
		args.flags |= NFSMNT_TIMEO;
	}
	if (args.retrans = nopt(mnt, "retrans")) {
		args.flags |= NFSMNT_RETRANS;
	}
	mnt->mnt_opts = NSE_STRDUP(opts);

	if (version == TFS_VERSION) {
#ifdef SUN_OS_4
		if (mount("tfs", mntpnt, flags | M_NEWTYPE, (char *) &args)) {
#else
		if (mount(MOUNT_TFS, mntpnt, flags, (char *) &args)) {
#endif SUN_OS_4
			return nse_err_format_errno("Mount (TFS) of \"%s\"",
						    mntpnt);
		} else {
			return NULL;
		}
	} else {
#ifdef SUN_OS_4
		if (mount("nfs", mntpnt, flags | M_NEWTYPE, (char *) &args)) {
#else
		if (mount(MOUNT_NFS, mntpnt, flags, (char *) &args)) {
#endif SUN_OS_4
			return nse_err_format_errno("NFS mount of \"%s\"",
						    mntpnt);
		} else {
			return NULL;
		}
	}
}


/*
 * Loopback mount 'dir' on the directory 'mount_pt'.
 */
Nse_err
nse_mount_loopback(dir, mount_pt, read_only, mntp)
	char		*dir;
	char		*mount_pt;
	bool_t		read_only;
	struct mntent	**mntp;
{
	struct lo_args	args;
	int		flags = 0;
	struct mntent	*mnt;

	args.fsdir = dir;
	if (read_only) {
		flags |= M_RDONLY;
	}
#ifdef SUN_OS_4
	if (mount("lo", mount_pt, flags | M_NEWTYPE, (char *) &args) == -1) {
#else
	if (mount(MOUNT_LO, mount_pt, flags, (char *) &args) == -1) {
#endif SUN_OS_4
		return nse_err_format_errno("Loopback mount of \"%s\"",
					    mount_pt);
	}
	mnt = (struct mntent *) nse_calloc(sizeof(struct mntent), 1);
	mnt->mnt_fsname = NSE_STRDUP(dir);
	mnt->mnt_dir = NSE_STRDUP(mount_pt);
	mnt->mnt_type = NSE_STRDUP("lo");
	if (read_only) {
		mnt->mnt_opts = NSE_STRDUP("ro");
	} else {
		mnt->mnt_opts = NSE_STRDUP("rw");
	}
	mnt->mnt_freq = 0;
	mnt->mnt_passno = 0;
	*mntp = mnt;
	return NULL;
}


/*
 * Return the value of a numeric option of the form foo=x.  If
 * option is not found or is malformed, return 0.
 */
static int
nopt(mnt, opt)
	struct mntent	*mnt;
	char		*opt;
{
	int		val = 0;
	char		*equal;
	char		*str;

	if (str = hasmntopt(mnt, opt)) {
		if (equal = index(str, '=')) {
			val = atoi(&equal[1]);
		}
	}
	return (val);
}
