#ifndef lint
static char sccsid[] = "@(#)tfs_ops.c 1.1 92/07/30 Copyr 1988 Sun Micro";
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
#include <nse/hash.h>
#include <nse/searchlink.h>

/*
 * This file contains all the TFS protocol routines, except for tfs_mount(),
 * tfs_umount(), and tfs_flush(), which live in tfs_mount.c.  For these
 * routines, we assume that the parent directory always exists in the
 * frontmost writeable layer.
 */

int		tfs_unwhiteout();
int		tfs_get_wo_entries();
int		tfs_sync();
int		tfs_push();
int		tfs_pull();
int		tfs_set_searchlink();
int		tfs_clear_front();
int		tfs_wo_readonly_files();

/*
 * Utility routines
 */
int		set_whiteout();
int		unwhiteout();
static struct vnode *get_wo_entries();
int		clear_file();
int		push_to_sub_layer();
static int	move_file();

/*
 * Procedure 22.  Unwhiteout.
 */
/* ARGSUSED */
int
tfs_unwhiteout(pvp, args, status)
	struct vnode	*pvp;
	Tfs_name_args	args;
	enum nfsstat	*status;
{
	struct vnode	*vp;
	struct pnode	*pp;

	vp = lookup_vnode(pvp, args->name, (struct nfsfattr *) NULL, TRUE);
	if (vp == NULL || !vp->v_whited_out || !IS_WRITEABLE(vp)) {
		return (0);
	}
	pp = get_pnode_at_layer(pvp, vp->v_layer);
	return (update_directory(pp, args->name, DIR_CLEAR_FILE));
}


/*
 * Procedure 23.  Get white-out entries for a directory.
 */
int
tfs_get_wo_entries(pvp, args, res)
	struct vnode	*pvp;
	Tfs_get_wo_args	args;
	Tfs_get_wo_res	res;
{
	/*
	 * Maintain a one-entry cache, so that when a get_wo_entries
	 * request comes in for the same directory at the current offset,
	 * we can use 'last_vp' instead of seeking to the correct position
	 * in the directory.
	 */
	static struct vnode *last_pvp = NULL;
	static int	last_offset = 0;
	static struct vnode *last_vp = NULL;

	struct vnode	*vp;
	int		offset;
	int		result;

	res->buf = read_result_buffer;
	res->offset = args->offset;
	res->count = 0;
	res->eof = 0;
	if (result = validate_directory(pvp)) {
		return (result);
	}
	if ((pvp == last_pvp) && (args->offset == last_offset)) {
		vp = last_vp;
	} else {
		vp = CHILD_VNODE(pvp);
		if (vp != NULL && (!vp->v_whited_out || !IS_WRITEABLE(vp))) {
			vp = next_whiteout(vp);
		}
		if (args->offset > 0) {
			offset = 0;
			while ((vp != NULL) && (offset < args->offset)) {
				/*
				 * Seek to correct offset within directory
				 */
				offset++;
				vp = next_whiteout(vp);
			}
		}
	}
	last_vp = get_wo_entries(vp, res, args->nbytes);
	if (res->eof) {
		last_pvp = NULL;
	} else {
		last_offset = res->offset;
		last_pvp = pvp;
	}
	return (0);
}


/*
 * Procedure 21.  Synchronize the TFS's write cache with disk.
 */
int
tfs_sync()
{
	sync_entire_wcache();
	flush_all_attrs();
}


/*
 * Procedure 25.
 * Push the file with vnode 'vp' from the front sub-layer to the next
 * sub-layer back.  Needed for variants.
 */
int
tfs_push(vp)
	struct vnode	*vp;
{
	struct pnode	*front_pp;
	struct pnode	*back_pp;
	char		pn[MAXPATHLEN];
	char		*name = vp->v_name;
	int		result;

	if (vp->v_pnode->p_sub_layer == MAX_SUB_LAYER) {
		return (0);
	}
	if (vp->v_pnode->p_type == PTYPE_DIR) {
		return (EISDIR);
	}
	if (result = promote_file(vp)) {
		return (result);
	}
	if (vp->v_pnode->p_fd != 0) {
		close_fd(vp->v_pnode);
	}
	front_pp = PARENT_PNODE(vp->v_pnode);
	back_pp = get_next_pnode(PARENT_VNODE(vp), front_pp);
	if (back_pp == NULL) {
		return (0);
	}
	if (result = change_to_dir(back_pp)) {
		return (result);
	}
	ptopn(vp->v_pnode, pn);
	if ((result = move_file(pn, name)) ||
	    (result = update_directory(front_pp, name, DIR_CLEAR_FILE)) ||
	    (result = update_directory(back_pp, name, DIR_ADD_FILE))) {
		return (result);
	}
	return (0);
}


/*
 * Pull the file with vnode 'vp' from the file system just in back of
 * the front file system to the front file system .  Needed for variants.
 */
int
tfs_pull(vp)
	struct vnode	*vp;
{
	struct pnode	*front_pp;
	struct pnode	*back_pp;
	char		pn[MAXPATHLEN];
	char		*name = vp->v_name;
	int		result;

	if (vp->v_pnode->p_sub_layer == FIRST_SUBL(vp)) {
		return (0);
	}
	if (vp->v_pnode->p_type == PTYPE_DIR) {
		return (EISDIR);
	}
	if (result = promote_file(vp)) {
		return (result);
	}
	if (vp->v_pnode->p_fd != 0) {
		close_fd(vp->v_pnode);
	}
	back_pp = PARENT_PNODE(vp->v_pnode);
	front_pp = PARENT_VNODE(vp)->v_pnode;
	if (result = change_to_dir(front_pp)) {
		return (result);
	}
	ptopn(vp->v_pnode, pn);
	if ((result = move_file(pn, name)) ||
	    (result = update_directory(back_pp, name, DIR_CLEAR_FILE)) ||
	    (result = update_directory(front_pp, name, DIR_ADD_FILE)) ||
	    (result = validate_directory(PARENT_VNODE(vp)))) {
		return (result);
	}
	if (vp->v_back_layer != INVALID_LAYER &&
	    (result = whiteout_other_dirs(back_pp, name, front_pp))) {
		return result;
	}
	return (0);
}


/*
 * Procedure 26.
 * Set the searchlink of the directory with vnode 'vp' to the value of
 * args->value.
 */
int
tfs_set_searchlink(vp, args)
	struct vnode	*vp;
	Tfs_searchlink_args args;
{
	char		*name = args->value;
	struct pnode	*back_pp;
	char		pn[MAXPATHLEN];
	char		full_name[MAXPATHLEN];
	char		*cp;
	int		result;

	if (result = validate_directory(vp)) {
		return (result);
	}
	back_pp = get_next_pnode(vp, vp->v_pnode);
	if (back_pp != NULL) {
		ptopn(back_pp, pn);
		if (NSE_STREQ(name, pn)) {
			/*
			 * Directory already has a searchlink set to
			 * the correct value.
			 */
			return args->conditional ? NSE_SL_ALREADY_SET_SAME : 0;
		}
		cp = nse_find_substring(name, NSE_TFS_WILDCARD);
		if (cp != NULL) {
			/*
			 * The searchlink has the wildcard string in it, so
			 * the existing searchlink may still match.  (This
			 * match will succeed incorrectly if the existing
			 * searchlink doesn't contain the wildcard string.)
			 */
			if (expand_searchlink(name, vp->v_environ_id, cp,
					      full_name, TRUE) &&
			    NSE_STREQ(full_name, pn)) {
				return args->conditional ?
						NSE_SL_ALREADY_SET_SAME : 0;
			}
		}
		if (args->conditional) {
			return (NSE_SL_ALREADY_SET_DIFF);
		}
	}
	if ((result = change_to_dir(vp->v_pnode)) ||
	    (result = tfsd_set_searchlink(".", name, vp->v_pnode)) ||
	    (result = update_directory(vp->v_pnode, (char *) NULL,
				       DIR_FLUSH))) {
		return (result);
	}
	return (0);
}


/*
 * Procedure 27.
 * Remove the file 'args->name' from the frontmost sub-layer of the front
 * file system (and remove the white-out entry for it, if there is one.)
 */
int
tfs_clear_front(pvp, args)
	struct vnode	*pvp;
	Tfs_name_args	args;
{
	struct vnode	*vp;

	vp = lookup_vnode(pvp, args->name, (struct nfsfattr *) NULL, TRUE);
	if (vp == NULL || vp->v_layer > 0) {
		return (0);
	}
	return (clear_file(pvp->v_pnode, vp));
}


/*
 * Procedure 28.
 * Whiteout all files in the directory with vnode 'pvp' that are showing
 * through from read-only filesystems.  (Whiteout only those files which
 * are in the first sub-layer.)
 */
int
tfs_wo_readonly_files(pvp)
	struct vnode	*pvp;
{
	struct vnode	*vp;
	struct vnode	*next_vp;
	struct pnode	*parentp;
	char		searchlink[MAXPATHLEN];
	Nse_whiteout	wo;
	Nse_whiteout	bl;
	int		err;
	int		result;

	if ((result = validate_directory(pvp)) ||
	    (result = change_to_dir(pvp->v_pnode))) {
		return result;
	}
	if (result = tfsd_get_tfs_info(".", searchlink, &bl, &wo, TRUE,
				       pvp->v_pnode, FALSE)) {
		return result;
	}
	for (vp = CHILD_VNODE(pvp); vp != NULL; vp = next_vp) {
		next_vp = NEXT_VNODE(vp);
		if (vp->v_layer > pvp->v_writeable && !vp->v_whited_out) {
			parentp = get_front_parent_pnode(pvp, vp->v_layer);
			if (parentp->p_sub_layer == FIRST_SUBL(pvp) &&
			    (result = update_directory(parentp, vp->v_name,
						       DIR_REMOVE_FILE,
						       TRUE, FALSE, &wo))) {
				goto error;
			}
		}
	}
error:
	if ((err = tfsd_set_tfs_info(".", searchlink, bl, wo, pvp->v_pnode)) &&
	    result == 0) {
		return err;
	}
	return result;
}


/*
 * Utility routines
 */

/*
 * Set whiteout entry for 'vp' in the directory with vnode 'pvp'.
 * precondition:  'vp' is a vnode in a writeable layer.
 */
int
set_whiteout(pvp, vp, listp)
	struct vnode	*pvp;
	struct vnode	*vp;
	Nse_whiteout	*listp;
{
	struct pnode	*pp;
	int		result;

	if (listp) {
		Nse_whiteout	wo;

		wo = NSE_NEW(Nse_whiteout);
		wo->name = NSE_STRDUP(vp->v_name);
		wo->next = *listp;
		*listp = wo;
		return (0);
	}
	pp = get_pnode_at_layer(pvp, vp->v_layer);
	if (result = change_to_dir(pp)) {
		return (result);
	}
	return (tfsd_set_whiteout(".", vp->v_name, pp));
}


/*
 * Remove whiteout entry for vnode 'vp' in the directory with vnode 'pvp'.
 */
int
unwhiteout(pvp, vp)
	struct vnode	*pvp;
	struct vnode	*vp;
{
	struct pnode	*pp;
	int		result;

	pp = get_pnode_at_layer(pvp, vp->v_layer);
	if ((result = change_to_dir(pp)) ||
	    (result = tfsd_clear_whiteout(".", vp->v_name, pp))) {
		return (result);
	}
	return (0);
}


/*
 * Stuff whiteout entries from the vnode list 'vp' into the directory
 * buffer in gr->buf.  Returns pointer to the next entry in the list, if
 * the list is too long to fit in one buffer; otherwise NULL.
 */
static struct vnode *
get_wo_entries(vp, gr, maxcount)
	struct vnode	*vp;
	Tfs_get_wo_res	gr;
	int		maxcount;
{
	struct direct	*dp;
	char		*buffer = gr->buf;
	int		len;
	int		reclen;

	do {
		if (vp == NULL) {
			gr->eof = 1;
			break;
		}
		len = strlen(vp->v_name);
		reclen = ENTRYSIZ(len);
		if (gr->count + reclen >= maxcount) {
			break;
		}
		dp = (struct direct *) buffer;
		dp->d_fileno = 1;
		strcpy(dp->d_name, vp->v_name);
		dp->d_namlen = len;
		dp->d_reclen = reclen;
		gr->count += reclen;
		buffer += reclen;
		gr->offset++;
#ifdef SUN_OS_4
		dp->d_off = gr->offset;
#endif
		vp = next_whiteout(vp);
	} while (1);
	return (vp);
}


/*
 * Remove the file/whiteout entry 'vp' from the directory with pnode
 * 'parentp'.
 */
int
clear_file(parentp, vp)
	struct pnode	*parentp;
	struct vnode	*vp;
{
	int		result;

	if (!vp->v_whited_out) {
		if (vp->v_pnode->p_needs_write) {
			sync_wcache(vp->v_pnode);
		}
		if (result = change_to_dir(parentp)) {
			return (result);
		}
		if (unlink(vp->v_name) < 0) {
			return (errno);
		}
	}
	return (update_directory(parentp, vp->v_name, DIR_CLEAR_FILE));
}


/*
 * Push the file with vnode 'vp' to sub-layer 'sub_layer'.  Assumes that
 * 'vp' is currently at or in front of the specified sub-layer.
 */
int
push_to_sub_layer(vp, sub_layer)
	struct vnode	*vp;
	int		sub_layer;
{
	int		result;

	while (1) {
		if (!lookup_pnode(PARENT_VNODE(vp), vp,
		    (struct nfsfattr *) NULL)) {
			return (errno);
		}
		if (vp->v_pnode->p_sub_layer == sub_layer) {
			break;
		}
		if (result = tfs_push(vp)) {
			return (result);
		}
	}
	return (0);
}

/*
 * Try to move 'from' file to 'to' file accounting for all manner
 * of unusual error conditions.
 */

static int
move_file(from, to)
	char		*from;
	char		*to;
{
	int		result;
	struct stat	tostat;
	struct stat	fromstat;
	Nse_err		err;

	result = rename(from, to);
	if (result == -1) {
		if (errno != EXDEV) {
			return errno;
		}
		err = _nse_copy_file(to, from, 1);
		if (err) {
			if (err->code != EPERM) {
				return err->code;
			}
			/*
			 * If copy failed because we are not the owner of the
			 * destination, make sure we can read the source as
			 * anybody, change uid to the owner of the
			 * destination, and try again.
			 */
			if (lstat(to, &tostat) < 0) {
				return errno;
			}
			if ((lstat(from, &fromstat) < 0) ||
			    (chmod(from, 0444) < 0)) {
				return errno;
			}
			change_user_id((int)tostat.st_uid);
			err = _nse_copy_file(to, from, 1);
			if (!err && (chmod(to, (int)fromstat.st_mode) < 0)) {
				err = nse_err_format_errno(
					   "Can't change mode of %s\n", to);
			}
			change_user_id(current_user_id());
			if (err) {
				/*
				 * It didn't work, so try to set the modes
				 * back as if nothing had happened.
				 */
				(void)chmod(from, (int)fromstat.st_mode);
				return err->code;
			}
		}
		/*
		 * Success! get rid of the source file.
		 */
		if (unlink(from) < 0) {
			return (errno);
		}
	}
	return (0);
}
