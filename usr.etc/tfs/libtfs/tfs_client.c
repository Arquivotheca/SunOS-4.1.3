#ifndef lint
static char sccsid[] = "@(#)tfs_client.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

/*
 * Library routines which make RPC calls to the TFS server to obtain their
 * results.
 */

#include <nse/param.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/dir.h>
#include <stdio.h>
#include <strings.h>
#include <nse/util.h>
#include <nse_impl/tfs_user.h>
#include <nse/tfs_calls.h>

Nse_err		nse_tfs_flush();
Nse_err		nse_tfs_sync();
Nse_err		nse_tfs_getname();
Nse_err		nse_tfs_push();
Nse_err		nse_tfs_pull();
Nse_err		nse_unwhiteout();
struct direct	*nse_readdir_whiteout();
static int	get_wo_entries();
Nse_err		nse_tfs_set_searchlink();
Nse_err		nse_tfs_set_cond_searchlink();
static Nse_err	set_searchlink();
Nse_err		nse_tfs_clear_front_fs();
Nse_err		nse_tfs_wo_readonly_files();
static Nse_err	tfs_rpc_call();
static Nse_err	get_parent_nodeid();
static Nse_err	get_nodeid();

/*
 * Tell the TFS to flush any cached information it might be keeping for the
 * current branch.
 */
Nse_err
nse_tfs_flush(branch)
	char		*branch;
{
#ifdef lint
	if (branch[0]) {}
#endif lint
#ifdef notdef
	char		branchid[NSE_MAXLONGBRANCHNAMELEN];
	char		*cp = branchid;
	enum nfsstat	status;
	Nse_err		err;

	if (branch == NULL) {
		branch = getenv(NSE_BRANCH_ENV);
		if (branch == NULL) {
			return nse_err_format(NSE_BRERR_NONAME);
		}
	}
	if (err = nse_branch_to_ascii_id(branch, branchid)) {
		return err;
	}
	err = nse_tfs_rpc_call(branch, TFS_FLUSH,
					xdr_wrapstring, (char *) &cp,
					xdr_enum, (char *) &status);
	if (err == NULL && (int) status != 0) {
		err = nse_err_format((int) status);
	}
	return err;
#endif notdef
}


/*
 * Tell the TFS to synchronize its write cache with disk.
 */
Nse_err
nse_tfs_sync(branch)
	char		*branch;
{
	return (nse_tfs_rpc_call(branch, TFS_SYNC, xdr_void, (char *)NULL,
				 xdr_void, (char *) NULL));
}


/*
 * Get a translation for the virtual name 'virt_path', and return it in
 * 'phys_path'.
 */
Nse_err
nse_tfs_getname(branch, virt_path, phys_path)
	char		*branch;
	char		*virt_path;
	char		*phys_path;
{
	Tfs_getname_res_rec gr;
	Tfs_fhandle_rec	fhandle;

	gr.path = phys_path;
	return tfs_rpc_call(branch, TFS_GETNAME, "getname", virt_path, FALSE,
			    xdr_tfs_fhandle, (char *) &fhandle,
			    xdr_tfs_getname_res, (char *) &gr);
}


Nse_err
nse_tfs_push(branch, path)
	char		*branch;
	char		*path;
{
	Tfs_fhandle_rec	fhandle;
	enum nfsstat	status;

	return tfs_rpc_call(branch, TFS_PUSH, "push", path, FALSE,
			    xdr_tfs_fhandle, (char *) &fhandle,
			    xdr_enum, (char *) &status);
}


Nse_err
nse_tfs_pull(branch, path)
	char		*branch;
	char		*path;
{
	Tfs_fhandle_rec	fhandle;
	enum nfsstat	status;

	return tfs_rpc_call(branch, TFS_PULL, "pull", path, FALSE,
			    xdr_tfs_fhandle, (char *) &fhandle,
			    xdr_enum, (char *) &status);
}


/*
 * Unwhiteout the file referenced by 'path'.  Returns 0 if all goes well, or
 * a non-zero error code if an error occurs.
 */
Nse_err
nse_unwhiteout(branch, path)
	char		*branch;
	char		*path;
{
	Tfs_name_args_rec ua;
	char		dir_name[MAXPATHLEN];
	char		name[MAXNAMLEN];
	enum nfsstat	status;

	_nse_parse_filename(path, dir_name, name);
	ua.name = name;
	return tfs_rpc_call(branch, TFS_UNWHITEOUT, "unwhiteout", dir_name,
			    TRUE, xdr_tfs_name_args, (char *) &ua,
			    xdr_enum, (char *) &status);
}


/*
 * Get next whited-out entry in a directory.  (Copy of readdir(3) code.)
 * 'dirp' should be initialized with opendir(3), and closed with
 * closedir(3).
 */
struct direct *
nse_readdir_whiteout(branch, dirname, dirp)
	char		*branch;
	char		*dirname;
	DIR		*dirp;
{
	struct direct	*dp;

#ifdef SUN_OS_4
	int saveloc = 0;
next:
        if (dirp->dd_size != 0) {
                dp = (struct direct *)&dirp->dd_buf[dirp->dd_loc];
                saveloc = dirp->dd_loc;   /* save for possible EOF */
                dirp->dd_loc += dp->d_reclen;
        }
        if (dirp->dd_loc >= dirp->dd_size)
                dirp->dd_loc = dirp->dd_size = 0;

        if (dirp->dd_size == 0  /* refill buffer */
          && (dirp->dd_size = get_wo_entries(branch, dirname, dirp->dd_buf,
					     (int) dirp->dd_bsize)
             ) <= 0
           ) {
                if (dirp->dd_size == 0) /* This means EOF */
                        dirp->dd_loc = saveloc;  /* EOF so save for telldir */
                return (NULL);    /* error or EOF */
        }

        dp = (struct direct *)&dirp->dd_buf[dirp->dd_loc];
        if (dp->d_reclen <= 0)
                return (NULL);
        if (dp->d_fileno == 0)
                goto next;
        dirp->dd_off = dp->d_off;
        return(dp);
#else
	for (;;) {
		if (dirp->dd_loc == 0) {
			dirp->dd_size = get_wo_entries(branch, dirname,
							dirp->dd_buf,
							(int) dirp->dd_bsize);
			if (dirp->dd_size <= 0)
				return (NULL);
			dirp->dd_entno = 0;
		}
		if (dirp->dd_loc >= dirp->dd_size) {
			dirp->dd_loc = 0;
			continue;
		}
		dp = (struct direct *)(dirp->dd_buf + dirp->dd_loc);
		if (dp->d_reclen <= 0)
			return (NULL);
		dirp->dd_loc += dp->d_reclen;
		dirp->dd_entno++;
		if (dp->d_fileno == 0)
			continue;
		return (dp);
	}
#endif
}


/*
 * Get the whited-out files in the directory with name 'path' (which can be a
 * relative pathname.)  'buf' is filled with the whited-out entries of the
 * front file system only.  This function returns the number of bytes in
 * 'buf', 0 if the end of the directory has been reached.
 */
static int
get_wo_entries(branch, path, buf, nbytes)
	char		*branch;
	char		*path;
	char		*buf;
	int		nbytes;
{
	/*
	 * One-entry cache to keep track of the current offset within the
	 * directory, and whether or not we've reached eof in the directory
	 * yet.
	 */
	static char	last_path[MAXPATHLEN];
	static int	last_offset;
	static int	eof = 0;

	Tfs_get_wo_args_rec args;
	Tfs_get_wo_res_rec result;
	Nse_err		err;

	if (NSE_STREQ(path, last_path)) {
		if (eof) {
			last_path[0] = '\0';
			return (0);
		}
		args.offset = last_offset;
	} else {
		strcpy(last_path, path);
		last_offset = 0;
		eof = 0;
	}
	args.nbytes = nbytes;
	args.offset = last_offset;
	result.buf = buf;
	result.count = nbytes;
	err = tfs_rpc_call(branch, TFS_GET_WO, "get_whiteout_entries",
			   path, TRUE, xdr_tfs_get_wo_args, (char *) &args,
			   xdr_tfs_get_wo_res, (char *) &result);
	if (err) {
		nse_err_print(err);
		last_path[0] = '\0';
		return (-1);
	} else {
		last_offset = result.offset;
		eof = result.eof;
		return (result.count);
	}
}


Nse_err
nse_tfs_set_searchlink(branch, directory, name)
	char		*branch;
	char		*directory;
	char		*name;
{
	return (set_searchlink(branch, directory, name, FALSE));
}


Nse_err
nse_tfs_set_cond_searchlink(branch, directory, name)
	char		*branch;
	char		*directory;
	char		*name;
{
	return set_searchlink(branch, directory, name, TRUE);
}


static Nse_err
set_searchlink(branch, directory, name, conditional_set)
	char		*branch;
	char		*directory;
	char		*name;
	bool_t		conditional_set;
{
	Tfs_searchlink_args_rec sa;
	enum nfsstat	status;

	sa.value = name;
	sa.conditional = conditional_set;
	return tfs_rpc_call(branch, TFS_SET_SEARCHLINK, "set_searchlink",
			    directory, TRUE,
			    xdr_tfs_searchlink_args, (char *) &sa,
			    xdr_enum, (char *) &status);
}


Nse_err
nse_tfs_clear_front_fs(branch, path)
	char		*branch;
	char		*path;
{
	Tfs_name_args_rec pa;
	char		dir_name[MAXPATHLEN];
	char		name[MAXPATHLEN];
	enum nfsstat	status;

	_nse_parse_filename(path, dir_name, name);
	pa.name = name;
	return tfs_rpc_call(branch, TFS_CLEAR_FRONT, "clear_front_fs",
			    dir_name, TRUE, xdr_tfs_name_args, (char *) &pa,
			    xdr_enum, (char *) &status);
}


Nse_err
nse_tfs_wo_readonly_files(branch, path)
	char		*branch;
	char		*path;
{
	Tfs_fhandle_rec	fhandle;
	enum nfsstat	status;

	return tfs_rpc_call(branch, TFS_WO_READONLY_FILES, "wo_readonly_files",
			    path, TRUE, xdr_tfs_fhandle, (char *) &fhandle,
			    xdr_enum, (char *) &status);
}


/*
 * Make the TFS RPC call 'procnum' on the file named 'path'.  'follow_symlink'
 * determines whether a symlink should be followed when getting the fhandle
 * for the file.  If an error code is returned by the TFS server, format
 * an error struct and return.  Note that the TFS server can't format the
 * error for the NSE_TFS_NOT_UNDER_MOUNTPT error because the TFS doesn't
 * know the name of the file in this case while the client does.
 * This routine relies on the fhandle being the first word of args, and on
 * the return status being the first word of res.
 */
static Nse_err
tfs_rpc_call(branch, procnum, procname, path, follow_symlink, xdr_args, args,
	     xdr_res, res)
	char		*branch;
	int		procnum;
	char		*procname;
	char		*path;
	bool_t		follow_symlink;
	xdrproc_t	xdr_args;
	char		*args;
	xdrproc_t	xdr_res;
	char		*res;
{
	Tfs_fhandle	fhp;
	int		ntries = 0;
	int		status;
	Nse_err		err;

	fhp = (Tfs_fhandle) args;
	if (err = get_nodeid(path, follow_symlink, &fhp->nodeid)) {
		return err;
	}
	fhp->parent_id = 0;
retry:
	err = nse_tfs_rpc_call(branch, procnum, xdr_args, args, xdr_res, res);
	if (err) {
		return err;
	}
	status = *(int *) res;
	if (status == 0) {
		return NULL;
	} else if (status > 0) {
		errno = status;
		if (procnum == TFS_UNWHITEOUT || procnum == TFS_CLEAR_FRONT) {
			return nse_err_format_errno("TFS %s of %s/%s",
						    procname, path,
						  ((Tfs_name_args)args)->name);
		} else {
			return nse_err_format_errno("TFS %s of %s", procname,
						    path);
		}
	} else if (status == NSE_TFS_NOT_UNDER_MOUNTPT && ntries == 0) {
		/*
		 * get nodeid of parent directory and try again
		 */

		ntries++;
		if (err = get_parent_nodeid(path, follow_symlink,
					    &fhp->parent_id)) {
			return err;
		}
		goto retry;
	} else {
		/*
		 * NSE_TFS_NOT_UNDER_MOUNTPT takes a path argument; other
		 * TFS error returns which could be seen here take no
		 * arguments.
		 */
		return nse_err_format((int) status, path);
	}
}


static Nse_err
get_parent_nodeid(path, follow_symlink, nodeidp)
	char		*path;
	bool_t		follow_symlink;
	long		*nodeidp;
{
	struct stat	statb;
	char		abs_path[MAXPATHLEN];
	char		abs_dir[MAXPATHLEN];
	char		dir[MAXPATHLEN];
	char		*cp;

	if (follow_symlink && lstat(path, &statb) == 0 &&
	    (statb.st_mode & S_IFMT) == S_IFLNK) {
		/*
		 * If we're following symlinks and this file is a symlink,
		 * we want to stat the directory that is the parent of the
		 * file referenced by the symlink.
		 */
		if (nse_abspath(path, abs_path) == NULL) {
			strcpy(abs_path, path);
		} else {
			/* XXX cover up for bug in nse_abspath()!
			 */
			cp = &abs_path[strlen(abs_path) - 1];
			if (*cp == '/') {
				*cp = '\0';
			}
		}
		_nse_parse_filename(abs_path, abs_dir, (char *) NULL);
	} else {
		/*
		 * Could be any # of symlinks above the file
		 */
		_nse_parse_filename(path, dir, (char *) NULL);
		if (nse_abspath(dir, abs_dir) == NULL) {
			strcpy(abs_dir, dir);
		}
	}
	return get_nodeid(abs_dir, FALSE, nodeidp); 
}


static Nse_err
get_nodeid(path, follow_symlink, nodeidp)
	char		*path;
	bool_t		follow_symlink;
	long		*nodeidp;
{
	struct stat	statb;
	static char *last_name = NULL;
	static bool_t last_symlink;
	static long last_ino;

	if (last_name && NSE_STREQ(path, last_name) &&
	    follow_symlink == last_symlink) {
		*nodeidp = last_ino;
		return NSE_OK;
	}
	if (follow_symlink) {
		if (stat(path, &statb) < 0) {
			return nse_err_format_errno("Stat of \"%s\"", path);
		}
	} else {
		if (lstat(path, &statb) < 0) {
			return nse_err_format_errno("Stat of \"%s\"", path);
		}
	}
	NSE_DISPOSE(last_name);
	last_ino = *nodeidp = statb.st_ino;
	last_name = NSE_STRDUP(path);
	last_symlink = follow_symlink;
	return NULL;
}
