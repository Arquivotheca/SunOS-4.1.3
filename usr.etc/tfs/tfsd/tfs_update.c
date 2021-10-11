#ifndef lint
static char sccsid[] = "@(#)tfs_update.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <nse/param.h>
#include "headers.h"
#include "vnode.h"
#include "tfs.h"
#include "subr.h"

/*
 * Whiteout entries should be set/unset only once if multiple
 * vnodes have a pointers to a pnode.
 */
typedef enum {
	WO_UNKNOWN, WO_SET, WO_NOT_SET, WO_FAILED, WO_CLEARED
} Whiteout_status;

/*
 * Routines which update directories.  These routines maintain the
 * v_layer and v_back_layer fields in vnodes, which let us know if a
 * whiteout entry needs to be created when a file is removed.
 */
int		update_directory();
static int	dir_rename_file();
static int	dir_add_file();
static int	dir_remove_file();
static int	dir_clear_file();
static int	remove_or_whiteout_file();
static void	dir_forward();
static void	move_pnodes();
static struct vnode *vnode_of_pnode();
static struct vnode *root_vnode_of();
static void	discard_child_vnodes();
static void	clear_update_flags();


/*
 * Routines which update directories.
 */

/*
 * The directory with pnode 'pp' has changed state.  Make sure that all
 * vnodes with pointers to this pnode are updated.  'name' is the name of
 * the file added/removed/cleared, and 'type' is the type of
 * modification that occurred.
 */
/* VARARGS3 */
int
update_directory(pp, name, type, arg1, arg2, arg3, arg4)
	struct pnode	*pp;
	char		*name;
	int		type;
	char		*arg1;
	char		*arg2;
	char		*arg3;
	char		*arg4;
{
	struct linknode	*lp;
	struct linknode	*lp_next;
	struct vnode	*pvp;
	struct vnode	*vp;
	int		result;
	int		err = 0;
	Whiteout_status	wo_status = WO_UNKNOWN;
	Whiteout_status	wo_status2 = WO_UNKNOWN;
	char		nam[MAXNAMLEN];

	/*
	 * 'name' may be freed.
	 */
	if (name) {
		strcpy(nam, name);
	}
	for (lp = pp->p_link; lp != NULL; lp = lp_next) {
		/*
		 * lp may be freed, so determine lp_next now
		 */
		lp_next = lp->l_next;
		pvp = lp->l_vnode;
		if (pvp->v_pnode->p_swapped && pvp->v_dir_valid &&
		    type != DIR_FLUSH) {
			/*
			 * Swapping in a directory changes the linknodes,
			 * so iterate through the linknode list again.
			 */
#ifdef TFSDEBUG
			dprint(tfsdebug, 4,
			       "update_directory: calling swap_in\n");
#endif TFSDEBUG
			swap_in_directory(pvp->v_pnode);
			lp_next = pp->p_link;
			continue;
		}
		if (name) {
			vp = find_vnode(pvp, nam);
		}
		result = 0;
		switch (type) {
		case DIR_ADD_FILE:
			if (vp == NULL) {
				vp = create_vnode(pvp, nam, (long) 0);
			}
			result = dir_add_file(pvp, vp, lp->l_layer,
					      &wo_status);
			/*
			 * If we just added a file to a previously empty
			 * directory, put the dir. in the swap queue.
			 * XXX should do the same for rename
			 */
			if (result == 0 && !pvp->v_pnode->p_in_queue &&
			    pvp->v_back_owner) {
				swqueue_add_pnode(pvp->v_pnode);
				lp_next = pp->p_link;
				continue;
			}
			break;
		case DIR_REMOVE_FILE:
			if (vp != NULL) {
				result = dir_remove_file(pvp, vp, lp->l_layer,
							 (bool_t) arg1,
							 (bool_t) arg2,
							 (Nse_whiteout *) arg3,
							 &wo_status);
			}
			break;
		case DIR_CLEAR_FILE:
			if (vp != NULL) {
				result = dir_clear_file(pvp, vp, lp->l_layer,
							&wo_status);
			}
			break;
		case DIR_RENAME_FILE:
			result = dir_rename_file(pvp, vp, lp->l_layer, pp,
						 (struct pnode *) arg1, arg2,
						 (bool_t) arg3, (bool_t) arg4,
						 &wo_status, &wo_status2);
			break;
		case DIR_UTIME_FILE:
			if (vp != NULL) {
				vp->v_mtime = (long) arg1;
			}
			break;
		case DIR_FLUSH:
			update_ctime(pvp, TRUE);
			vnode_tree_flush(pvp);
			break;
		case DIR_FORWARD:
			dir_forward(pvp, lp->l_layer, (struct pnode *) arg1);
			break;
		}
		if (result != 0 && err == 0) {
			err = result;
		}
	}
	if (type == DIR_REMOVE_FILE || type == DIR_CLEAR_FILE ||
	    type == DIR_RENAME_FILE) {
		clear_update_flags(pp);
	}
	return (err);
}


/*
 * 'vp' is the old vnode of a file that was renamed from directory 'pvp'
 * at layer 'layer' into the directory 'new_parentp' with new name
 * 'new_name'.  We preserve the nodeid of the file on rename, and also
 * preserve the parent-child relationship of vnodes if we are renaming
 * a directory.
 */
static int
dir_rename_file(pvp, vp, layer, old_parentp, new_parentp, new_name,
		must_set_wo, is_directory, wo_status, wo_status2)
	struct vnode	*pvp;
	struct vnode	*vp;
	int		layer;
	struct pnode	*old_parentp;
	struct pnode	*new_parentp;
	char		*new_name;
	bool_t		must_set_wo;
	bool_t		is_directory;
	Whiteout_status	*wo_status;
	Whiteout_status	*wo_status2;
{
	struct vnode	*newpvp;
	struct vnode	*nvp;
	struct vnode	*childvp = NULL;
	long		nodeid = 0;
	struct pnode	*pp = NULL;
	bool_t		dir_valid;
	int		old_dir_layer = INVALID_LAYER;
	int		result;

	if (new_parentp == old_parentp) {
		newpvp = pvp;
	} else {
		newpvp = vnode_of_pnode(new_parentp, pvp);
	}
	if (vp != NULL) {
		if (!vp->v_whited_out) {
			/*
			 * Assign a new fhandle for the vnode, so that if it
			 * becomes a whiteout vnode and is later unwhited-out,
			 * two vnodes won't end up with the same fhandle.
			 */
			nodeid = vp->v_fhid;
			fhandle_remove(vp);
			vp->v_fhid = ++next_fhandle;
			fhandle_insert(vp);
			if (is_directory) {
				old_dir_layer = vp->v_layer;
			}
		}
		if (newpvp != NULL) {
			childvp = CHILD_VNODE(vp);
			CHILD_VNODE(vp) = NULL;
			pp = vp->v_pnode;
			vp->v_pnode = NULL;
			dir_valid = vp->v_dir_valid;
			if (is_directory) {
				new_parentp = newpvp->v_pnode;
			}
		}
		if (result = dir_remove_file(pvp, vp, layer, must_set_wo,
					is_directory, (Nse_whiteout *) NULL,
					wo_status)) {
			if (pp != NULL) {
				vp->v_pnode = pp;
			}
			if (childvp != NULL) {
				CHILD_VNODE(vp) = childvp;
			}
			return (result);
		}
	}
	if (newpvp == NULL) {
		return (0);
	}
	if (newpvp->v_pnode->p_swapped) {
		swap_in_directory(newpvp->v_pnode);
	}
	nvp = find_vnode(newpvp, new_name);
	if (nvp == NULL) {
		nvp = create_vnode(newpvp, new_name, nodeid);
	} else if (nodeid != 0) {
		fhandle_remove(nvp);
		nvp->v_fhid = nodeid;
		fhandle_insert(nvp);
	}
	if (pp != NULL) {
		if (PARENT_PNODE(pp) != new_parentp ||
		    !NSE_STREQ(pp->p_name, new_name)) {
			rename_pnode(pp, new_parentp, new_name);
		}
		if (nvp->v_pnode != NULL) {
			release_pnodes(nvp);
		}
		if (pp->p_type == PTYPE_DIR) {
			move_pnodes(pp, vp, nvp);
		}
		nvp->v_pnode = pp;
	}
	nvp->v_dont_flush = TRUE;
	result = dir_add_file(newpvp, nvp, layer, wo_status2);
	if (result == 0 && is_directory && old_dir_layer < layer) {
		result = dir_add_file(newpvp, nvp, old_dir_layer, wo_status2);
	}
	nvp->v_dont_flush = FALSE;
	if (childvp != NULL) {
		CHILD_VNODE(nvp) = childvp;
		while (childvp != NULL) {
			PARENT_VNODE(childvp) = nvp;
			childvp = NEXT_VNODE(childvp);
		}
		if (dir_valid && nvp->v_pnode != NULL) {
			nvp->v_dir_valid = TRUE;
		}
	}
	if (result != 0) {
		vnode_tree_flush(nvp);
	}
	return (result);
}


/*
 * 'vp' is the vnode for a file that was added to the directory 'pvp' at
 * layer 'layer'.
 */
static int
dir_add_file(pvp, vp, layer, wo_status)
	struct vnode	*pvp;
	struct vnode	*vp;
	int		layer;
	Whiteout_status	*wo_status;
{
	int		back_layer;
	bool_t		back_wo;
	int		result = 0;

	if (vp->v_layer < layer) {
		if (vp->v_back_layer >= layer) {
			vp->v_back_layer = layer;
			vp->v_back_whited_out = FALSE;
		}
		return (0);
	}
	if ((vp->v_layer == INVALID_LAYER) || (vp->v_layer > layer)) {
		back_layer = vp->v_layer;
		back_wo = vp->v_whited_out;
		if (!vp->v_dont_flush) {
			vnode_tree_flush(vp);
		}
		vp->v_back_layer = back_layer;
		vp->v_back_whited_out = back_wo;
		vp->v_layer = layer;
	} else {	/* vp->v_layer == layer */
		if (IS_WRITEABLE(vp) && vp->v_whited_out &&
		    *wo_status != WO_CLEARED && *wo_status != WO_FAILED) {
			if (result = unwhiteout(pvp, vp)) {
				*wo_status = WO_FAILED;
			} else {
				*wo_status = WO_CLEARED;
			}
		}
	}
	vp->v_whited_out = FALSE;
	return (result);
}


/*
 * A file with vnode 'vp' was removed from the directory 'pvp' at layer
 * 'layer'.  Update the directory accordingly.  May need to whiteout
 * the file if it would show through from a back fs.  If 'must_set_wo' is
 * TRUE, always whiteout the file that was removed, even if it doesn't
 * exist in a back fs.
 */
static int
dir_remove_file(pvp, vp, layer, must_set_wo, is_directory, wo_listp, wo_status)
	struct vnode	*pvp;
	struct vnode	*vp;
	int		layer;
	bool_t		must_set_wo;
	bool_t		is_directory;
	Nse_whiteout	*wo_listp;
	Whiteout_status	*wo_status;
{
	int		result = 0;

	if (vp->v_layer == INVALID_LAYER) {
		return (0);
	}
	if (vp->v_layer < layer) {
		/*
		 * if vp->v_back_layer < layer, do nothing;
		 * vp->v_back_layer > layer impossible
		 */
		if (vp->v_back_layer == layer) {
			vp->v_back_layer = UNKNOWN_LAYER;
			vp->v_back_whited_out = FALSE;
			if (is_directory) {
				return (remove_or_whiteout_file(pvp, vp, layer,
								must_set_wo,
								is_directory,
								wo_listp,
								wo_status));
			}
		}
		return (0);
	}
	if (CHILD_VNODE(vp) != NULL) {
		/*
		 * This can happen if rmdir removes a directory which
		 * contained whiteout vnodes, or if rename moves a
		 * directory to a different parent directory.
		 */
		discard_child_vnodes(vp);
	}
	if (vp->v_whited_out) {
		return (0);
	}
	if (vp->v_layer == layer) {
		if (vp->v_layer > 0) {
			update_ctime(pvp, TRUE);
		}
		if (vp->v_back_layer == INVALID_LAYER && !must_set_wo) {
			/*
			 * File doesn't exist in back -- throw away
			 * the reference to it.
			 */
			free_vnode(vp);
			return (0);
		}
		if (vp->v_back_layer == UNKNOWN_LAYER) {
			return (remove_or_whiteout_file(pvp, vp, layer,
							must_set_wo,
							is_directory,
							wo_listp,
							wo_status));
		} else if (vp->v_back_whited_out) {
			vp->v_layer = vp->v_back_layer;
			vp->v_back_layer = UNKNOWN_LAYER;
			vp->v_back_whited_out = FALSE;
		} else if (IS_WRITEABLE(vp) && *wo_status != WO_SET &&
			   *wo_status != WO_FAILED) {
			if (result = set_whiteout(pvp, vp, wo_listp)) {
				*wo_status = WO_FAILED;
			} else {
				*wo_status = WO_SET;
			}
		}
	} else {
		int		old_layer;

		/*
		 * vp->v_layer > layer (can happen if a file is removed which
		 * is showing through from a read-only layer.)
		 */
		update_ctime(pvp, TRUE);
		old_layer = vp->v_layer;
		vp->v_layer = layer;
		if (IS_WRITEABLE(vp) && *wo_status != WO_SET &&
		    *wo_status != WO_FAILED) {
			if (result = set_whiteout(pvp, vp, wo_listp)) {
				*wo_status = WO_FAILED;
			} else {
				*wo_status = WO_SET;
			}
		}
		if (result == 0 && *wo_status != WO_FAILED) {
			vp->v_back_layer = old_layer;
			vp->v_back_whited_out = FALSE;
		} else {
			vp->v_layer = old_layer;
		}
	}
	if (result == 0 && *wo_status != WO_FAILED) {
		vp->v_whited_out = TRUE;
		if (vp->v_pnode != NULL) {
			release_pnodes(vp);
		}
		/*
		 * Set v_dir_valid to FALSE in case this was a dir vnode
		 * (it may be unwhited-out in the future, in which case we
		 * don't want it to appear to be a valid directory.)
		 */
		vp->v_dir_valid = FALSE;
	}
	return (result);
}


/*
 * Clear the entry for file with vnode 'vp' in the directory 'pvp' at layer
 * 'layer', and allow the back instance of the file to show through.  If
 * there is a whiteout entry for the file, clear it.
 */
static int
dir_clear_file(pvp, vp, layer, wo_status)
	struct vnode	*pvp;
	struct vnode	*vp;
	int		layer;
	Whiteout_status	*wo_status;
{
	int		result = 0;

	if (*wo_status == WO_FAILED) {
		return (0);
	}
	if (vp->v_layer == layer) {
		if (!vp->v_whited_out && vp->v_layer > 0) {
			update_ctime(pvp, TRUE);
		}
		if (IS_WRITEABLE(vp) && vp->v_whited_out &&
		    *wo_status != WO_CLEARED && *wo_status != WO_FAILED) {
			if (result = unwhiteout(pvp, vp)) {
				*wo_status = WO_FAILED;
			} else {
				*wo_status = WO_CLEARED;
			}
		}
		if (*wo_status == WO_FAILED) {
			return (result);
		}
		if (vp->v_back_layer == UNKNOWN_LAYER) {
			pvp->v_dir_valid = FALSE;
		} else {
			/*
			 * v_back_layer may be INVALID_LAYER here -- don't
			 * release the vnode so that tfs_push will work
			 */
			vp->v_layer = vp->v_back_layer;
			vp->v_whited_out = vp->v_back_whited_out;
			vp->v_back_layer = UNKNOWN_LAYER;
			vp->v_back_whited_out = FALSE;
		}
		vp->v_dir_valid = FALSE;
		if (vp->v_pnode != NULL && result == 0) {
			release_pnodes(vp);
		}
	} else if (vp->v_layer < layer) {
		/*
		 * if vp->v_back_layer < layer, do nothing;
		 * vp->v_back_layer > layer impossible
		 */
		if (vp->v_back_layer == layer) {
			vp->v_back_layer = UNKNOWN_LAYER;
			vp->v_back_whited_out = FALSE;
		}
	}
	/*
	 * vp->v_layer > layer impossible, unless this routine called twice
	 * on same file
	 */
	return (result);
}


/*
 * This routine is called when a file has been removed and we don't know
 * whether or not the name exists in another layer further back.  This can
 * happen when a directory is removed, because more than one layer is
 * removed.  We can only keep track of two layers for a file with v_layer &
 * v_back_layer, so when the second layer is removed, the parent directory
 * needs to be revalidated before we can determine whether or not a
 * whiteout entry needs to be created.
 */
static int
remove_or_whiteout_file(pvp, vp, layer, must_set_wo, is_directory, wo_listp,
			wo_status)
	struct vnode	*pvp;
	struct vnode	*vp;
	int		layer;
	bool_t		must_set_wo;
	bool_t		is_directory;
	Nse_whiteout	*wo_listp;
	Whiteout_status	*wo_status;
{
	struct pnode	*pp;
	int		result;

	switch (*wo_status) {
	case WO_SET:
		vp->v_layer = layer;
		vp->v_whited_out = TRUE;
		if (vp->v_pnode != NULL) {
			release_pnodes(vp);
		}
		if (CHILD_VNODE(vp) != NULL) {
			discard_child_vnodes(vp);
		}
		return (0);
	case WO_NOT_SET:
		vnode_tree_release(vp);
		return (0);
	case WO_FAILED:
		return (0);
	default:
		/*
		 * Make sure that the pnode at 'layer' is not freed
		 * by validate_directory as it is being held by
		 * update_directory.
		 */
		pp = get_pnode_at_layer(pvp, layer);
		pp->p_refcnt++;
		pvp->v_dir_valid = FALSE;
		result = validate_directory(pvp);
		pp->p_refcnt--;
		if (result) {
			return (result);
		}
		if (vp->v_layer == INVALID_LAYER) {
			vnode_tree_release(vp);
			*wo_status = WO_NOT_SET;
			return (0);
		} else {
			/* vp->v_layer > layer */
			return (dir_remove_file(pvp, vp, layer, must_set_wo,
						is_directory, wo_listp,
						wo_status));
		}
	}
}


/*
 * The pnode for 'pvp' at layer 'layer' has been changed to 'newp'.
 */
static void
dir_forward(pvp, layer, newp)
	struct vnode	*pvp;
	int		layer;
	struct pnode	*newp;
{
	struct pnode	*pp;
	struct pnode	*prev_pp;
	struct pnode	*next_pp;

	prev_pp = get_pnode_at_layer(pvp, (unsigned) layer - 1);
	pp = release_linknode(pvp, prev_pp);
	set_next_pnode(pvp, prev_pp, newp, layer - 1);
	next_pp = release_linknode(pvp, pp);
	free_pnode(pp);
	set_next_pnode(pvp, newp, next_pp, layer);
	newp->p_refcnt++;
}


/*
 * Transfer directory pnodes from vnode 'old_vp' to vnode 'new_vp'.
 * ('old_vp' may have been freed.)
 */
static void
move_pnodes(pp, old_vp, new_vp)
	struct pnode	*pp;
	struct vnode	*old_vp;
	struct vnode	*new_vp;
{
	struct pnode	*next_pp;
	int		layer = 0;

	if (!has_linknode(old_vp, pp)) {
		return;
	}
	while (pp) {
		next_pp = release_linknode(old_vp, pp);
		set_next_pnode(new_vp, pp, next_pp, layer);
		pp = next_pp;
		layer++;
	}
}


/*
 * Returns vnode pointing to pnode pp which is under the same mountpt as vnode
 * pvp.  Needed for rename when the source and destination directories are
 * not the same.
 */
static struct vnode *
vnode_of_pnode(pp, pvp)
	struct pnode	*pp;
	struct vnode	*pvp;
{
	struct vnode	*rootvp;
	struct vnode	*vp2;
	struct linknode	*lp;

	rootvp = root_vnode_of(pvp);
	lp = pp->p_link;
	while (lp != NULL) {
		vp2 = lp->l_vnode;
		if (vp2->v_environ_id == pvp->v_environ_id &&
		    root_vnode_of(vp2) == rootvp) {
			return (vp2);
		}
		lp = lp->l_next;
	}
	return (NULL);
}


static struct vnode *
root_vnode_of(vp)
	struct vnode	*vp;
{
	while (vp != NULL && !vp->v_is_mount_pt) {
		vp = PARENT_VNODE(vp);
	}
	return (vp);
}


static void
discard_child_vnodes(pvp)
	struct vnode	*pvp;
{
	struct vnode	*vp;
	struct vnode	*nextvp;

	for (vp = CHILD_VNODE(pvp); vp != NULL; vp = nextvp) {
		nextvp = NEXT_VNODE(vp);
		vnode_tree_release(vp);
	}
}


static void
clear_update_flags(pp)
	struct pnode	*pp;
{
	struct linknode	*lp;

	for (lp = pp->p_link; lp != NULL; lp = lp->l_next) {
		lp->l_vnode->v_pnode->p_updated = FALSE;
	}
}
