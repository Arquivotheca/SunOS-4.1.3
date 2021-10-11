#ifndef lint
static char sccsid[] = "@(#)tfs_vnodeops.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <nse/types.h>
#include "headers.h"
#include "vnode.h"
#include "tfs.h"
#include "subr.h"
#include <nse/param.h>
#include <nse/util.h>
#include <nse/searchlink.h>

/*
 * Routines to implement NFS ops.  (Except for readdir, which is in
 * tfs_dirs.c.)
 */

extern int	chmod();
extern int	fchmod();
extern int	chown();
extern int	fchown();

int		tfs_error();
int		tfs_null();
int		tfs_getattr();
int		tfs_setattr_1();
int		tfs_setattr_2();
static int	do_setattr();
int		tfs_lookup_1();
int		tfs_lookup_2();
static int	do_lookup();
int		tfs_readlink();
int		tfs_read();
int		tfs_write();
int		tfs_create_1();
int		tfs_create_2();
static int	do_create();
int		tfs_remove();
int		tfs_rename();
int		tfs_link();
int		tfs_symlink();
int		tfs_mkdir_1();
int		tfs_mkdir_2();
static int	do_mkdir();
int		tfs_rmdir();
int		tfs_statfs();
int		tfs_translate();

/*
 * Error procedure invoked when an obsolete NFS procedure is called.
 * (i.e. nfs_root & nfs_writecache)
 */
int
tfs_error()
{
	return (EOPNOTSUPP);
}


/*
 * Procedure 0.  Null procedure available for ping-ing.
 */
int
tfs_null()
{
#ifdef TFSDEBUG
	trace_reset();
	print_vnode_trees();
	pnode_tree_print();
	comp_print();
	print_fd_cache();
	swqueue_print();
#endif TFSDEBUG
	/* do nothing */
	return (0);
}


/*
 * Procedure 1.  Get file attributes.
 * Returns the current attributes of the file with the given fhandle.
 */
/*ARGSUSED*/
int
tfs_getattr(vp, fhp, ns)
	struct vnode   *vp;
	fhandle_t	*fhp;
	struct nfsattrstat *ns;
{
	if (!attr_cache_on) {
		init_attr_cache();
	}
	if (getattrs_of(vp, &ns->ns_attr) < 0) {
		return (errno);
	}
	return (0);
}


/*
 * Procedure 2.  Set file attributes.
 * Sets the attributes of the file with the given fhandle.  Returns
 * the new attributes.
 */
int
tfs_setattr_1(vp, args, ns)
	struct vnode	*vp;
	struct nfssaargs *args;
	struct nfsattrstat *ns;
{
	int		result;

	if (result = do_setattr(vp, args)) {
		return (result);
	}
	if (args->saa_sa.sa_size != (u_long) -1) {
		/*
		 * Truncate changes the mtime of the file, so make sure that
		 * the updated mtime is correct.
		 */
		(void) write_wcache(vp->v_pnode, (char *) NULL, 0, 0L);
	}
	if (getattrs_of(vp, &ns->ns_attr) < 0) {
		return (errno);
	}
	return (0);
}


int
tfs_setattr_2(vp, args, dr)
	struct vnode	*vp;
	struct nfssaargs *args;
	struct tfsdiropres *dr;
{
	int		result;

	if (result = do_setattr(vp, args)) {
		return (result);
	}
	fill_diropres(vp, dr);
	return (0);
}


static int
do_setattr(vp, args)
	struct vnode	*vp;
	struct nfssaargs *args;
{
	struct pnode	*pp;
	char		name[MAXNAMLEN];
	struct nfssattr *sa = &args->saa_sa;
	bool_t		set_to_now;
	int		result;

	if (result = has_perm_to_setattr(vp, sa)) {
		return (result);
	}
	pp = vp->v_pnode;
	if (pp->p_type == PTYPE_REG && pp->p_needs_write) {
		sync_wcache(pp);
	}
	/*
	 * For all operations other than changing the file's access or
	 * modify times, copy-on-write the file before changing its attrs.
	 */
	if ((sa->sa_mode != (u_short) -1 && sa->sa_mode != -1) ||
	    sa->sa_uid != -1 || sa->sa_gid != -1 ||
	    sa->sa_size != (u_long) -1) {
		if (result = promote_file(vp)) {
			return (result);
		}
	}
	pp = vp->v_pnode;

	if (sa->sa_mode != (u_short) -1 && sa->sa_mode != -1) {
		if (result = modify_file(pp, chmod, fchmod,
					 (int) sa->sa_mode)) {
			return (result);
		}
	}
	if (sa->sa_uid != -1 || sa->sa_gid != -1) {
		if (result = modify_file(pp, chown, fchown,
					 (int) sa->sa_uid, (int) sa->sa_gid)) {
			return (result);
		}
	}
	if (sa->sa_size != (u_long) -1) {
		/*
		 * ftruncate() needs an fd opened for writing, so just use
		 * truncate().
		 */
		ptoname(pp, name);
		if (truncate(name, (long) sa->sa_size) < 0) {
			return (errno);
		}
	}
	/*
	 * Change file access or modified times.
	 */
	set_to_now = FALSE;
	if (sa->sa_atime.tv_sec != -1 || sa->sa_mtime.tv_sec != -1) {
#ifdef SUN_OS_4
		/*
		 * Allow SysV-compatible option to set access and
		 * modified times.
		 * XXX - va_mtime.tv_usec == -1 flags this.
		 */
		if (sa->sa_mtime.tv_sec != -1 && sa->sa_mtime.tv_usec == -1) {
			set_to_now = TRUE;
		}
#endif
		if (result = do_utimes(vp, sa->sa_mtime.tv_sec,
				       sa->sa_atime.tv_sec, set_to_now)) {
			return (result);
		}
	}
	flush_cached_attrs(pp);
	return (0);
}


/*
 * Procedure 4.  Directory lookup.
 * Returns an fhandle and file attributes for file name in a directory.
 */
int
tfs_lookup_1(pvp, da, dr)
	struct vnode	*pvp;
	struct nfsdiropargs *da;
	struct nfsdiropres *dr;
{
	struct vnode	*vp;
	int		result;

	if (result = do_lookup(pvp, da, &vp, &dr->dr_attr)) {
		return (result);
	}
	makefh(&dr->dr_fhandle, vp);
	return (0);
}


int
tfs_lookup_2(pvp, da, dr)
	struct vnode	*pvp;
	struct nfsdiropargs *da;
	struct tfsdiropres *dr;
{
	struct vnode	*vp;
	int		result;

	if (result = do_lookup(pvp, da, &vp, (struct nfsfattr *) NULL)) {
		return (result);
	}
	fill_diropres(vp, dr);
	return (0);
}


static int
do_lookup(pvp, da, vpp, attrs)
	struct vnode	*pvp;
	struct nfsdiropargs *da;
	struct vnode	**vpp;
	struct nfsfattr *attrs;
{
	struct vnode	*vp;
	char		*name = da->da_name;

	if (is_tfs_special_file(name)) {
		return (ENOENT);
	} else if (NSE_STREQ(name, ".")) {
		vp = pvp;
	} else if (NSE_STREQ(name, "..")) {
		vp = PARENT_VNODE(pvp);
		if (vp == NULL || pvp->v_is_mount_pt) {
			vp = pvp;
		}
	} else {
		vp = lookup_vnode(pvp, name, attrs, FALSE);
		if (vp == NULL) {
			return (ENOENT);
		}
		goto got_attrs;
	}

	if (attrs && getattrs_of(vp, attrs) < 0) {
		return (errno);
	}
got_attrs:
	*vpp = vp;
	return (0);
}


/*
 * Procedure 5.  Read symbolic link.
 * Returns the string in the symbolic link at the given fhandle.
 */
/* ARGSUSED */
int
tfs_readlink(vp, fhp, rl)
	struct vnode	*vp;
	fhandle_t	*fhp;
	struct nfsrdlnres *rl;
{
	char            name[MAXNAMLEN];
	int             count;

	rl->rl_data = read_result_buffer;
	ptoname(vp->v_pnode, name);
	count = readlink(name, rl->rl_data, MAXPATHLEN);
	if (count < 0) {
		rl->rl_data = NULL;
		rl->rl_count = 0;
		return (errno);
	}
	rl->rl_data[count] = '\0';
	rl->rl_count = count;
	return (0);
}


/*
 * Procedure 6.  Read data.
 * Returns some data read from the file at the given fhandle.
 */
int
tfs_read(vp, ra, rr)
	struct vnode   *vp;
	struct nfsreadargs *ra;
	struct nfsrdresult *rr;
{
	struct pnode	*pp;
	long		offset;
	int		length;
	int		count;

	/*
	 * Check the transfer size restriction.  If we try to send more than
	 * NFS_MAXDATA, we will hang the server.  This is only a problem if the
	 * client dosn't listen to our transfer size in statfs. 
	 */
	if (ra->ra_count > NFS_MAXDATA) {
		return (EMSGSIZE);
	}
	rr->rr_data = read_result_buffer;

	/*
	 * for now we assume no append mode and ignore totcount (read ahead) 
	 */
	offset = (long) ra->ra_offset;
	length = (int) ra->ra_count;
	count = 0;
	pp = vp->v_pnode;
	if (offset >= pp->p_size) {
		/*
		 * Reading past EOF (happens when the buffer cache
		 * reads-ahead before writing.)
		 */
		goto rdwr_finished;
	}
	if (get_rw_fd(vp, UIO_READ) < 0) {
		return (errno);
	}
	if (pp->p_needs_write) {
		/*
		 * Read data from write cache if it's there.
		 */
		count = read_wcache(pp, rr->rr_data, length, offset);
	}
	if (count <= 0) {
		count = rdwr_fd(pp, rr->rr_data, length, offset, UIO_READ);
	}
rdwr_finished:
	if ((count < 0) || getattrs_of(vp, &rr->rr_attr) < 0) {
		return (errno);
	}
	rr->rr_count = count;
	return (0);
}


/*
 * Procedure 8.  Write data to file.
 * Returns attributes of a file after writing some data to it.
 */
int
tfs_write(vp, wa, ns)
	struct vnode   *vp;
	struct nfswriteargs *wa;
	struct nfsattrstat *ns;
{
	struct pnode	*pp;
	int		result;
	long		offset;
	int		length;

	/*
	 * for now we assume no append mode 
	 */
	if (result = promote_file(vp)) {
		return (result);
	}
	pp = vp->v_pnode;
	if (pp->p_write_error != 0) {
		/*
		 * Return error code that was returned when an earlier
		 * delayed write failed.
		 */
		return (pp->p_write_error);
	}
	if (get_rw_fd(vp, UIO_WRITE) < 0) {
		return (errno);
	}
	offset = (long) wa->wa_offset;
	length = (int) wa->wa_count;
	if (offset + length >= pp->p_size) {
		pp->p_size = offset + length;
	}
	result = write_wcache(pp, wa->wa_data, length, offset);
	if ((result < 0) || getattrs_of(vp, &ns->ns_attr) < 0) {
		return (errno);
	}
	return (0);
}


/*
 * Procedure 9.  Create a file.
 * Creates a file with given attributes and returns those attributes
 * and an fhandle for the new file.
 */
int
tfs_create_1(pvp, args, dr)
	struct vnode	*pvp;
	struct nfscreatargs *args;
	struct nfsdiropres *dr;
{
	struct vnode	*vp;
	int		old_size = -1;
	int		result;

	if (result = do_create(pvp, args, &vp, &old_size)) {
		return (result);
	}
	if (getattrs_of(vp, &dr->dr_attr) < 0) {
		return (errno);
	}
	if (old_size == -1 || args->ca_sa.sa_size == 0) {
		/*
		 * If the file was just created or truncated, we have to
		 * update the mtime of the file, even if no writes are
		 * done to the file later.
		 */
		(void) write_wcache(vp->v_pnode, (char *) NULL, 0, 0L);
		if (getattrs_of(vp, &dr->dr_attr) < 0) {
			return (errno);
		}
	}
	makefh(&dr->dr_fhandle, vp);
	return (0);
}


int
tfs_create_2(pvp, args, dr)
	struct vnode	*pvp;
	struct nfscreatargs *args;
	struct tfsdiropres *dr;
{
	struct vnode	*vp;
	int		result;

	if (result = do_create(pvp, args, &vp, (int *) NULL)) {
		return (result);
	}
	fill_diropres(vp, dr);
	return (0);
}


/*
 * XXX The sa_uid field of the args is used as a transaction ID to check
 * for duplicate requests.  We really should add a transaction ID field
 * to the create args.
 */
static int
do_create(pvp, args, vpp, sizep)
	struct vnode	*pvp;
	struct nfscreatargs *args;
	struct vnode	**vpp;
	int		*sizep;
{
	struct vnode	*vp;
	struct pnode	*parentp;
	struct pnode	*pp;
	struct nfsfattr	attrs;
	char		*name = args->ca_da.da_name;
	int		mode;
	int		flags;
	int		result;

	/*
	 * XXX Should get exclusive flag and pass it on here.
	 */
	if (is_tfs_special_file(name)) {
		return (EACCES);
	}
	vp = lookup_vnode(pvp, name, &attrs, TRUE);
	if (vp != NULL && !vp->v_whited_out) {
		if (attrs.na_size == 0 && args->ca_sa.sa_size == 0 &&
		    dupreq_is_duplicate(TFS_CREATE, args->ca_sa.sa_uid)) {
			*vpp = vp;
			if (sizep) {
				*sizep = attrs.na_size;
			}
			return (0);
		}
		/*
		 * If file exists in a back file system, create the file in
		 * the front file system with the mode of the existing file
		 * in the back fs (if we have the proper access to the file.)
		 */
		if (!has_access(&attrs, W_OK)) {
			return (EACCES);
		}
		if (sizep) {
			*sizep = attrs.na_size;
		}
		parentp = get_front_parent_pnode(pvp, vp->v_layer);
		mode = (int) attrs.na_mode;
		if (!IS_WRITEABLE(vp)) {
			if ((parentp->p_link && parentp->p_link->l_next) ||
			    environ_has_multiple_clients(vp)) {
				update_ctime(vp, FALSE);
			}
			release_pnodes(vp);
		} else if (args->ca_sa.sa_size == -1) {
			/*
			 * File exists in writeable layer & its size
			 * doesn't need to be changed.
			 */
			goto ok;
		} else {	/* args->ca_sa.sa_size == 0 */
			/*
			 * File exists in writeable layer & has to be
			 * truncated.  If we have an open fd, ftruncate
			 * the file (which will update its mtime)
			 */
			pp = vp->v_pnode;
			if (sizep && *sizep == 0) {
				/*
				 * (Version 1) File doesn't need to be
				 * truncated.  Let the write cache
				 * update the mtime.
				 */
				goto ok;
			}
			if (pp->p_needs_write) {
				flush_wcache(pp);
			}
			flush_cached_attrs(pp);
			if (pp->p_fd) {
				if (get_open_fd_flags((int) pp->p_fd) ==
				    O_RDONLY) {
					close_fd(pp);
				} else if (ftruncate((int)pp->p_fd, 0L) < 0) {
					return (errno);
				} else {
					goto ok;
				}
			}
		}
	} else {
		/*
		 * If the file is whited-out, create the file at the
		 * sub-layer the whiteout entry was found at; otherwise
		 * create the file in the frontmost layer.
		 */
		if (vp != NULL) {
			parentp = get_front_parent_pnode(pvp, vp->v_layer);
		} else {
			parentp = pvp->v_pnode;
		}
		mode = (int) args->ca_sa.sa_mode;
		if (vp == NULL) {
			vp = create_vnode(pvp, name, (long) 0);
		}
	}
	if (vp->v_pnode == NULL) {
		vp->v_pnode = create_pnode(parentp, name, PTYPE_REG);
	}
	if (args->ca_sa.sa_size == 0) {
		flags = O_RDWR | O_CREAT | O_TRUNC;
	} else {
		/*
		 * sa_size == -1 if the client didn't set the O_TRUNC flag
		 */
		flags = O_RDWR | O_CREAT;
	}
	if (open_fd(vp->v_pnode, flags, mode) < 0) {
		if (vp->v_layer == INVALID_LAYER) {
			free_vnode(vp);
		} else {
			release_pnodes(vp);
		}
		return (errno);
	}
	vp->v_dont_flush = TRUE;
	result = update_directory(parentp, name, DIR_ADD_FILE);
	vp->v_dont_flush = FALSE;
	if (result != 0) {
		return (result);
	}
	vp->v_created = TRUE;
ok:
	*vpp = vp;
	dupreq_save(TFS_CREATE, args->ca_sa.sa_uid);
	return (0);
}


/*
 * Procedure 10.  Remove a file.
 * Remove named file from parent directory.
 * If the file isn't found in the directory, then assume that the remove
 * request is a duplicate.  This assumption is valid as long as the client
 * kernel always tries to look up the file before sending the remove
 * request (i.e.  the kernel never sends a remove request for a
 * non-existent file.)  XXX A better solution would be to pass a unique
 * transaction ID in the remove args, so that duplicate remove requests can
 * be detected.
 */
/* ARGSUSED */
int
tfs_remove(pvp, da, status)
	struct vnode	*pvp;
	struct nfsdiropargs *da;
	enum nfsstat	*status;
{
	struct vnode	*vp;
	struct pnode	*parentp;
	char		*name = da->da_name;
	int		result;

	vp = lookup_vnode(pvp, name, (struct nfsfattr *) NULL, FALSE);
	if (vp == NULL) {
		return (0);
	}
	parentp = get_front_parent_pnode(pvp, vp->v_layer);
	if (IS_WRITEABLE(vp)) {
		if (result = change_to_dir(parentp)) {
			return (result);
		}
		if (unlink(name) < 0) {
			return (errno);
		}
	}
	if (vp->v_pnode != NULL && vp->v_pnode->p_needs_write) {
		flush_wcache(vp->v_pnode);
	}
	if (result = update_directory(parentp, name, DIR_REMOVE_FILE,
				      (parentp->p_sub_layer > FIRST_SUBL(pvp)),
				      FALSE, NULL)) {
		return (result);
	}
	return (0);
}


/*
 * Procedure 11.  Rename a file.
 * Give a file (from) a new name (to).
 * As in tfs_remove(), a request to rename a non-existent file is assumed
 * to be a duplicate request.
 */
/* ARGSUSED */
int
tfs_rename(opvp, args, status)
	struct vnode	*opvp;
	struct nfsrnmargs *args;
	enum nfsstat	*status;
{
	struct vnode	*npvp;		/* vp for destination directory */
	char		npn[MAXPATHLEN];
	struct pnode	*oparentp;
	struct pnode	*nparentp;
	char		*oname = args->rna_from.da_name;
	char		*nname = args->rna_to.da_name;
	struct vnode	*ovp;
	struct vnode	*nvp;
	int		dest_sub_layer = -1;
	int		result;

	npvp = fhtovp(TFS_FH(&args->rna_to.da_fhandle));
	if (npvp == NULL) {
		return (ESTALE);
	}

	if (is_tfs_special_file(oname) || is_tfs_special_file(nname)) {
		return (EACCES);
	}
	/*
	 * Get vnode for the old name of the file, and make sure that the
	 * file exists in a writeable file system before it is renamed.
	 */
	ovp = lookup_vnode(opvp, oname, (struct nfsfattr *) NULL, FALSE);
	if (ovp == NULL) {
		/*
		 * Assume that the rename request is a duplicate.
		 */
		return (0);
	}
	/*
	 * Don't allow copy into self.
	 */
	if (ovp == npvp) {
		return (EINVAL);
	}
	if (result = promote_file(ovp)) {
		return (result);
	}
	nvp = lookup_vnode(npvp, nname, (struct nfsfattr *) NULL, TRUE);
	if (nvp != NULL && !nvp->v_whited_out) {
		/*
		 * Make sure we flush any data for the file about to be
		 * removed by the rename.
		 */
		if (nvp->v_pnode != NULL && nvp->v_pnode->p_fd != 0) {
			close_fd(nvp->v_pnode);
		}
		/*
		 * If we're renaming a directory to the name of an existing
		 * directory, we have to ensure that the destination
		 * directory is empty.
		 */
		if (nvp->v_pnode->p_type == PTYPE_DIR &&
		    ovp->v_pnode->p_type == PTYPE_DIR) {
			if ((result = remove_directory(nvp)) &&
			    result != ENOENT) {
				return (result);
			}
		}
	}
	if (ovp->v_pnode->p_type == PTYPE_DIR) {
		return rename_directory(ovp, npvp, nname);
	}
	oparentp = PARENT_PNODE(ovp->v_pnode);
	nparentp = get_front_parent_pnode(npvp, ovp->v_layer);
	if (result = change_to_dir(oparentp)) {
		return (result);
	}
	if (oparentp == nparentp) {
		strcpy(npn, nname);
	} else {
		ptopn(nparentp, npn);
		nse_pathcat(npn, nname);
	}
	if (rename(oname, npn) < 0) {
		return (errno);
	}
	if (nvp != NULL &&
	    (result = handle_rename_sub_layers(nvp, ovp->v_layer,
					       &dest_sub_layer))) {
		return (result);
	}
	result = update_directory(oparentp, oname, DIR_RENAME_FILE,
				  nparentp, nname,
				  (oparentp->p_sub_layer > FIRST_SUBL(ovp)),
				  FALSE);
	if (result == 0 && opvp != npvp) {
		/*
		 * Another view may have a vnode for the destination
		 * directory but not one for the source directory.
		 * Here we guarantee that the destination directory
		 * gets a vnode for the new (renamed) file.
		 */
		result = update_directory(nparentp, nname, DIR_ADD_FILE);
	}
	if (result == 0 && dest_sub_layer != -1) {
		/*
		 * If file was renamed to a name which existed in a sub-layer
		 * further back, push the file back to that sub-layer.
		 */
		nvp = find_vnode(npvp, nname);
		result = push_to_sub_layer(nvp, dest_sub_layer);
	}
	return (result);
}


/*
 * Procedure 12.  Link to a file.
 * Create a file (new) which is a hard link to the given file (old).
 */
/* ARGSUSED */
int
tfs_link(oldvp, args, status)
	struct vnode	*oldvp;
	struct nfslinkargs *args;
	enum nfsstat	*status;
{
	struct vnode	*newpvp;
	struct vnode	*newvp;
	struct pnode	*nparentp;
	char		oldname[MAXNAMLEN];
	char		newpn[MAXPATHLEN];
	char		*name = args->la_to.da_name;
	int		dest_sub_layer = -1;
	int		result;

	newpvp = fhtovp(TFS_FH(&args->la_to.da_fhandle));
	if (newpvp == NULL) {
		return (ESTALE);
	}
	if (is_tfs_special_file(name)) {
		return (EACCES);
	}
	newvp = lookup_vnode(newpvp, name, (struct nfsfattr *) NULL, TRUE);
	if (newvp != NULL && !newvp->v_whited_out) {
		return (EEXIST);
	}
	if (result = promote_file(oldvp)) {
		return (result);
	}
	if (newvp != NULL &&
	    (result = handle_rename_sub_layers(newvp, oldvp->v_layer,
					       &dest_sub_layer))) {
		return (result);
	}
	nparentp = get_front_parent_pnode(newpvp, oldvp->v_layer);
	ptoname(oldvp->v_pnode, oldname);
	if (oldname[0] != '/' && nparentp == PARENT_PNODE(oldvp->v_pnode)) {
		strcpy(newpn, name);
	} else {
		ptopn(nparentp, newpn);
		nse_pathcat(newpn, name);
	}
	if (link(oldname, newpn) < 0) {
		return (errno);
	}
	flush_cached_attrs(oldvp->v_pnode);
	if (oldvp->v_pnode->p_needs_write) {
		sync_wcache(oldvp->v_pnode);
	}
	if (result = update_directory(nparentp, name, DIR_ADD_FILE)) {
		return (result);
	}
	if (dest_sub_layer != -1) {
		newvp = find_vnode(newpvp, name);
		result = push_to_sub_layer(newvp, dest_sub_layer);
	}
	return (result);
}


/*
 * Procedure 13.  Symbolicly link to a file.
 * Create a file (named newpn) which is a symbolic link pointing to name
 * args->sla_tnm.  NOTE: the attributes in args are not used, since
 * symlinks are always created with mode 777.
 */
/* ARGSUSED */
int
tfs_symlink(pvp, args, status)
	struct vnode	*pvp;
	struct nfsslargs *args;
	enum nfsstat	*status;
{
	struct vnode	*vp;
	struct pnode	*parentp;
	char		*name = args->sla_from.da_name;
	int		result;

	if (is_tfs_special_file(name)) {
		return (EACCES);
	}
	vp = lookup_vnode(pvp, name, (struct nfsfattr *) NULL, TRUE);
	if (vp != NULL) {
		if (!vp->v_whited_out) {
			return (EEXIST);
		}
		parentp = get_front_parent_pnode(pvp, vp->v_layer);
	} else {
		parentp = pvp->v_pnode;
	}
	if (result = change_to_dir(parentp)) {
		return (result);
	}
	if (symlink(args->sla_tnm, name) < 0) {
		struct stat	statb;

		/*
		 * Possible for symlink create to fail but leave a symlink
		 * pointing at nothing.
		 */
		result = errno;
		if (lstat(name, &statb) == 0) {
			(void) update_directory(parentp, name, DIR_ADD_FILE);
		}
		return (errno);
	}
	return (update_directory(parentp, name, DIR_ADD_FILE));
}


/*
 * Procedure 14.  Make a directory.
 * Create a directory with the given name, parent directory, and attributes.
 * Returns a file handle and attributes for the new directory.
 */
int
tfs_mkdir_1(pvp, args, dr)
	struct vnode	*pvp;
	struct nfscreatargs *args;
	struct nfsdiropres *dr;
{
	struct vnode	*vp;
	int		result;

	if (result = do_mkdir(pvp, args, &vp, &dr->dr_attr)) {
		return (result);
	}
	makefh(&dr->dr_fhandle, vp);
	return (0);
}


int
tfs_mkdir_2(pvp, args, dr)
	struct vnode	*pvp;
	struct nfscreatargs *args;
	struct tfsdiropres *dr;
{
	struct vnode	*vp;
	int		result;

	if (result = do_mkdir(pvp, args, &vp, (struct nfsfattr *) NULL)) {
		return (result);
	}
	fill_diropres(vp, dr);
	return (0);
}


static int
do_mkdir(pvp, args, vpp, attrs)
	struct vnode	*pvp;
	struct nfscreatargs *args;
	struct vnode	**vpp;
	struct nfsfattr	*attrs;
{
	struct vnode	*vp;
	struct pnode	*parentp;
	char		*name = args->ca_da.da_name;
	char		path[MAXPATHLEN];
	char		variant[MAXPATHLEN];
	int		result;

	if (is_tfs_special_file(name)) {
		return (EACCES);
	}
	vp = lookup_vnode(pvp, name, (struct nfsfattr *) NULL, TRUE);
	if (vp != NULL && !vp->v_whited_out) {
		if (dupreq_is_duplicate(TFS_MKDIR, args->ca_sa.sa_uid)) {
			if (attrs && getattrs_of(vp, attrs) < 0) {
				return (errno);
			}
			*vpp = vp;
			return (0);
		} else {
			return (EEXIST);
		}
	}
	parentp = get_pnode_at_layer(pvp, pvp->v_writeable);
	if (result = change_to_dir(parentp)) {
		return (result);
	}
	if (result = exists_in_variant(name, variant)) {
		vtovn(pvp, path);
		nse_pathcat(path, name);
		nse_log_message(
			"mkdir: %s already exists as a file in variant %s\n",
				path, variant);
		return (result);
	}
	if (mkdir(name, (int) args->ca_sa.sa_mode) < 0) {
		return (errno);
	}
	if (result = update_directory(parentp, name, DIR_ADD_FILE)) {
		return (result);
	}
	vp = lookup_vnode(pvp, name, attrs, TRUE);
	if (vp != NULL && vp->v_whited_out) {
		parentp = get_pnode_at_layer(pvp, vp->v_layer);
		if (result = update_directory(parentp, name,
					      DIR_CLEAR_FILE)) {
			return (result);
		}
		vp = lookup_vnode(pvp, name, attrs, FALSE);
	}
	if (vp == NULL) {
		return (errno);
	}
	*vpp = vp;
	dupreq_save(TFS_MKDIR, args->ca_sa.sa_uid);
	return (0);
}


/*
 * Procedure 15.  Remove a directory.
 * Remove the given directory name from the given parent directory.
 * As in tfs_remove(), a request to remove a non-existent directory is
 * assumed to be a duplicate request.
 */
/* ARGSUSED */
int
tfs_rmdir(pvp, da, status)
	struct vnode	*pvp;
	struct nfsdiropargs *da;
	enum nfsstat	*status;
{
	struct vnode	*vp;

	vp = lookup_vnode(pvp, da->da_name, (struct nfsfattr *) NULL, FALSE);
	if (vp == NULL) {
		return (0);
	}
	return (remove_directory(vp));
}


/*
 * Procedure 17.  Get file system statistics.
 */
/* ARGSUSED */
int
tfs_statfs(vp, fh, fs)
	struct vnode	*vp;
	fhandle_t	*fh;
	struct nfsstatfs *fs;
{
	char		name[MAXNAMLEN];
	struct statfs	sb;
	int		fd;

	fd = vp->v_pnode->p_fd;
	if (fd != 0) {
		if (fstatfs(fd, &sb) < 0) {
			return (errno);
		}
	} else {
		ptoname(vp->v_pnode, name);
		if (statfs(name, &sb) < 0) {
			return (errno);
		}
	}
	fs->fs_tsize = NFS_MAXDATA;
	fs->fs_bsize = sb.f_bsize;
	fs->fs_blocks = sb.f_blocks;
	fs->fs_bfree = sb.f_bfree;
	fs->fs_bavail = sb.f_bavail;
	return (0);
}


/*
 * Procedure 39.  Provide TFS translation for a virtual file.
 */
int
tfs_translate(vp, args, dr)
	struct vnode	*vp;
	struct tfstransargs *args;
	struct tfsdiropres *dr;
{
	int		result;

	if (args->tr_writeable && (result = promote_file(vp))) {
		return (result);
	}
	fill_diropres(vp, dr);
	return (0);
}
