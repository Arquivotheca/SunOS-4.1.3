#ifndef lint
static char sccsid[] = "@(#)tfs_lookup.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <nse/param.h>
#include <nse/util.h>
#include "headers.h"
#include "vnode.h"
#include "tfs.h"
#include "subr.h"
#include <signal.h>
#include <nse/searchlink.h>

Nse_list	ptrlist_create();

/*
 * Vnode & pnode lookup routines.
 */

struct vnode	*mntpntovp();
struct vnode	*lookup_vnode();
bool_t		recurse_lookup_pnode();
bool_t		lookup_pnode();
static bool_t	forward_pnode();
struct pnode	*create_pnode();
void		create_root_pnode();
struct pnode	*follow_searchlink();
static struct pnode *expand_slink_to_pnode();
static struct pnode *lookup_dir_pnode();
struct pnode	*path_to_pnode();
static struct pnode *get_pnode();
static int	set_visited_pnode();
void		clear_visited_pnodes();
static void	clear_pnode();

/*
 * Routines which operate on fhandle hash table.
 */
struct vnode	*fhtovp();
struct vnode	*fhandle_find();
void		fhandle_insert();
void		fhandle_remove();

/*
 * Routines which operate on pathname component hash table.
 */
struct vnode	*find_vnode();
static struct tnode *comp_find();
void		comp_insert();
void		comp_remove();


/*
 * Vnode & pnode lookup routines.
 */

/*
 * Given a mount pathname, allocate a new fhandle and vnode for the root of
 * the sub-hierarchy.  'virtpn' is the virtual pathname (mountpoint) and
 * 'realpn' is the directory being mounted
 */
struct vnode *
mntpntovp(root_vnode, virtpn, realpn)
	struct vnode	*root_vnode;
	char		*virtpn;
	char		*realpn;
{
	struct vnode	*pvp;
	struct vnode	*vp;
	char		compname[MAXNAMLEN];
	char		*cp = virtpn;
	bool_t		new = FALSE;

	vp = root_vnode;
	cp++;
	while (cp[0] != '\0') {
		cp = _nse_extract_name(cp, compname);
		pvp = vp;
		vp = find_vnode(pvp, compname);
		if (vp == NULL) {
			vp = create_vnode(pvp, compname, (long) 0);
			new = TRUE;
		}
	}
	if (!new) {
		/*
		 * Make sure that vp's old pnode is released (it won't be
		 * if the vnode is swapped out or is a mountpt vnode.)
		 */
		vp->v_is_mount_pt = FALSE;
		if (vp->v_pnode && vp->v_pnode->p_swapped) {
			swap_in_directory(vp->v_pnode);
		}
		vnode_tree_flush(vp);
	}
	vp->v_is_mount_pt = TRUE;
	create_root_pnode(vp, realpn);
	vp->v_layer = 0;
	return (vp);
}


/*
 * Follow the searchlink from the directory with pnode 'pp', and return
 * a pointer to the directory the searchlink points to.  While we're reading
 * the .tfs_info file, read in the white-out entries also.
 * 'first_sub_layer' is the # of the frontmost sub-layer for the current
 * view.
 */
struct pnode *
follow_searchlink(pp, environ_id, first_sub_layer, wop, rename_recovered)
	struct pnode	*pp;
	unsigned	environ_id;
	unsigned	first_sub_layer;
	Nse_whiteout	*wop;
	bool_t		*rename_recovered;
{
	char		new_pn[MAXPATHLEN];
	char		searchlink[MAXPATHLEN];
	struct pnode	*next_pp = NULL;
	Nse_whiteout	bl = NULL;
	Nse_whiteout	bl_rename;

	if (wop) {
		*wop = NULL;
	}
	if (rename_recovered) {
		*rename_recovered = FALSE;
	}
	searchlink[0] = '\0';
	
	if (set_visited_pnode(pp) != 0) {
		return (NULL);
	}
	ptoname_or_pn(pp, new_pn);
	
	if (tfsd_get_tfs_info(new_pn, searchlink, &bl, wop, FALSE,
				       pp, TRUE)) {
		return (NULL);
	}
	if (bl && (bl_rename = bl_list_get_rename_elem(bl))) {
		(void) finish_rename_directory(new_pn, bl_rename->name);
		if (rename_recovered) {
			*rename_recovered = TRUE;
			nse_dispose_whiteout(bl);
			return (NULL);
		}
	}
	nse_dispose_whiteout(bl);
	if (searchlink[0] == '\0') {
		return (NULL);
	}
	if (searchlink[0] == '/') {
		next_pp = expand_slink_to_pnode(searchlink, environ_id,
						first_sub_layer);
	}
	if (next_pp == NULL) {
		nse_log_message("warning: Bad searchlink %s", searchlink);
		print_pnode_path(pp);
	} else 	if (next_pp->p_visited) {
		nse_log_message("warning: Searchlink loop through %s to ",
				searchlink);
		print_pnode_path(pp);
		return (NULL);
	}
	return (next_pp);
}


/*
 * Routines to prevent infinite recursion on circularities in searchlinks
 * and backlinks. set_visited_pnode and clear_visited_pnodes share
 * the 'visited_pnode_list'.
 */

static Nse_list	visited_pnode_list = 0;

/*
 * Check to see if pnode 'pp' has already been seen while following
 * a searchlink chain. If it has, log an error and return true,
 * otherwise mark it as visited and add it to the 'visited_pnode_list'.
 * We really shouldn't ever see a previously visited pnode here, but ...
 */

static int
set_visited_pnode(pp)
	struct pnode	*pp;
	
{
	char		path[MAXPATHLEN];

	if (pp->p_visited) {
		ptopn(pp, path);
		nse_log_message("warning: Searchlink loop through %s\n", path);
		return 1;
	}
	if (visited_pnode_list == 0) {
		visited_pnode_list = ptrlist_create();
	}
	pp->p_visited = TRUE;
	pp->p_refcnt++;
	nse_list_add_new_data(visited_pnode_list, (Nse_opaque)pp);
	return 0;
}

/*
 * Clean up pnodes visited while following searchlinks.
 */

void
clear_visited_pnodes()
{
	if (visited_pnode_list) {
		nse_list_iterate(visited_pnode_list, clear_pnode);
		nse_list_destroy(visited_pnode_list);
		visited_pnode_list = 0;
	}
}

/*
 * Clear the visited flag of pnode 'pp'. Call free_pnode in case the
 * last reference to this pnode disappeared after it was visited.
 */

static void
clear_pnode(pp)
	struct pnode	*pp;
{
	pp->p_visited = FALSE;
	free_pnode(pp);
}

/*
 * Return a vnode for the file 'name' in the directory with vnode 'pvp'.
 * Returns NULL if the file doesn't exist.  If the parameter 'attrs' is
 * non-NULL, the attributes of the file are returned.  If 'want_whiteout'
 * is TRUE, return the whiteout vnode for the file if it exists.
 */
struct vnode *
lookup_vnode(pvp, name, attrs, want_whiteout)
	struct vnode	*pvp;
	char		*name;
	struct nfsfattr	*attrs;
	bool_t		want_whiteout;
{
	struct vnode	*vp;

	if (validate_directory(pvp) > 0) {
		return (NULL);
	}
	vp = find_vnode(pvp, name);
	if (vp == NULL) {
		return (NULL);
	}
	if (vp->v_whited_out) {
		if (want_whiteout) {
			return (vp);
		} else {
			return (NULL);
		}
	}
	if (vp->v_layer == INVALID_LAYER) {
		vnode_tree_release(vp);
		return (NULL);
	}
	if (!lookup_pnode(pvp, vp, attrs)) {
		return (NULL);
	}
	return (vp);
}


/*
 * Lookup the pnode for vnode 'vp'.  Recursively looks up pnodes for all of
 * 'vp's ancestors, if the pnodes don't exist (because of a tfs_flush.)
 * Also ensures that all 'vp's ancestor directories are valid.
 * Returns TRUE if the pnode(s) is/are constructed, FALSE otherwise.
 */
bool_t
recurse_lookup_pnode(vp)
	struct vnode	*vp;
{
	struct vnode	*pvp;

	pvp = PARENT_VNODE(vp);
	if (pvp == NULL) {
		return (FALSE);
	}
	if (pvp->v_dir_valid) {
		if (vp->v_pnode != NULL) {
			return (TRUE);
		}
		if (pvp->v_pnode == NULL && !recurse_lookup_pnode(pvp)) {
			return (FALSE);
		}
	} else {
		if (vp->v_is_mount_pt) {
			if (vp->v_pnode == NULL) {
				nse_log_message("panic: recurse_lookup_pnode");
				panic((struct pnode *) NULL);
			}
			return (TRUE);
		}
		if (!recurse_lookup_pnode(pvp)) {
			return (FALSE);
		}
		if (validate_directory(pvp) > 0) {
			return (FALSE);
		}
	}
	return (lookup_pnode(pvp, vp, (struct nfsfattr *) NULL));
}


/*
 * Lookup the pnode for vnode 'vp'.  If 'vp' is for a directory, only the
 * pnode for the frontmost layer is constructed.  Returns TRUE if the pnode
 * is constructed, FALSE otherwise.  If 'attrs' is non-NULL, the attributes
 * of the file are returned.
 */
bool_t
lookup_pnode(pvp, vp, attrs)
	struct vnode	*pvp;
	struct vnode	*vp;
	struct nfsfattr	*attrs;
{
	struct pnode	*parentp;
	struct stat	statb;
	char		*name = vp->v_name;
	unsigned	type;

	if (vp->v_pnode != NULL) {
		if (attrs != NULL && getattrs_of(vp, attrs) < 0) {
			vnode_tree_release(vp);
			return (FALSE);
		}
		return (TRUE);
	}
	parentp = get_pnode_at_layer(pvp, vp->v_layer);
	if (change_to_dir(parentp)) {
		return (FALSE);
	}
	if (LSTAT(name, &statb) < 0) {
		if (!forward_pnode(pvp, &parentp) || change_to_dir(parentp) ||
		    LSTAT(name, &statb) < 0) {
			vnode_tree_release(vp);
			return (FALSE);
		}
	}
	switch (statb.st_mode & S_IFMT) {
	case S_IFREG:
		type = PTYPE_REG;
		break;
	case S_IFDIR:
		type = PTYPE_DIR;
		break;
	case S_IFLNK:
		type = PTYPE_LINK;
		break;
	default:
		type = PTYPE_OTHER;
		break;
	}
	vp->v_pnode = create_pnode(parentp, name, type);
	if (attrs != NULL) {
		fattr_to_nattr(&statb, attrs);
		if (attr_cache_on) {
			if (vp->v_pnode->p_have_attrs) {
				flush_cached_attrs(vp->v_pnode);
			}
			set_cached_attrs(vp->v_pnode, attrs);
		}
		fix_attrs(vp, attrs);
	} else if ((type == PTYPE_REG) && (!vp->v_pnode->p_needs_write)) {
		vp->v_pnode->p_size = statb.st_size;
	}
	return (TRUE);
}


/*
 * If the directory with pnode 'parentp' has been forwarded, return TRUE
 * and return the new pnode of the directory in '*parentp'.
 */
static bool_t
forward_pnode(vp, parentp)
	struct vnode	*vp;
	struct pnode	**parentp;
{
	struct pnode	*pp;
	struct pnode	*newp;
	struct pnode	*front_pp;
	char		oldpath[MAXPATHLEN];
	char		newpath[MAXPATHLEN];
	struct stat	statb;

	if (stat(NSE_TFS_FORWARD_FILE, &statb) == -1) {
		return FALSE;
	}
	if (nse_get_searchlink(".", newpath)) {
		return FALSE;
	}
	newp = expand_slink_to_pnode(newpath, vp->v_environ_id,
				     FIRST_SUBL(vp));
	if (newp == NULL) {
		return FALSE;
	}
	pp = *parentp;
	ptopn(pp, oldpath);
	if (newp->p_sub_layer != pp->p_sub_layer) {
		nse_log_message("warning: can't forward %s to %s\n",
			oldpath, newpath);
		free_pnode(newp);
		return FALSE;
	}
	front_pp = get_pnode_at_layer(vp, SUBL_TO_LAYER(vp, newp));
	newp->p_refcnt--;
	if (change_backlist_name(front_pp, oldpath, newpath) != 0 ||
	    update_directory(pp, "", DIR_FORWARD, newp) != 0) {
		return FALSE;
	}
	*parentp = newp;
	return TRUE;
}


/*
 * Create pnode for a regular file/directory.
 */
struct pnode *
create_pnode(parentp, name, type)
	struct pnode	*parentp;
	char		*name;
	unsigned	type;
{
	struct pnode	*pp;

	pp = get_pnode(parentp, name);
	pp->p_type = type;
	pp->p_refcnt++;
	return (pp);
}


/*
 * Create pnode for a root vnode.
 */
void
create_root_pnode(vp, pn)
	struct vnode	*vp;
	char		*pn;
{
	vp->v_pnode = path_to_pnode(pn, MAX_SUB_LAYER - vp->v_writeable);
	vp->v_pnode->p_type = PTYPE_DIR;
}


/*
 * Expand the variant searchlink in 'path'.  Returns the pnode of the
 * searchlink, if it evaluates to a valid directory.
 */
static struct pnode *
expand_slink_to_pnode(path, environ_id, first_sub_layer)
	char		*path;
	unsigned	environ_id;
	unsigned	first_sub_layer;
{
	struct pnode	*pp;
	char		*cp;
	char		*varname;
	char		fullname[MAXPATHLEN];

	cp = nse_find_substring(path, NSE_TFS_WILDCARD);
	if (cp == NULL || !expand_searchlink(path, environ_id, cp, fullname,
					     TRUE)) {
		varname = environ_var_name(environ_id);
		if (varname && nse_find_substring(path, varname)) {
			return lookup_dir_pnode(path, first_sub_layer);
		} else {
			return lookup_dir_pnode(path, MAX_SUB_LAYER);
		}
	}
	pp = lookup_dir_pnode(fullname, first_sub_layer);
	if (pp != NULL) {
		return pp;
	}
	if (!expand_searchlink(path, environ_id, cp, fullname, FALSE)) {
		strcpy(fullname, path);
	}
	return lookup_dir_pnode(fullname, MAX_SUB_LAYER);
}


static struct pnode *
lookup_dir_pnode(path, sub_layer)
	char		*path;
	unsigned	sub_layer;
{
	struct pnode	*pp;
	char		name[MAXPATHLEN];
	struct stat	statb;

	pp = path_to_pnode(path, sub_layer);
	ptoname_or_pn(pp, name);
	if ((stat(name, &statb) == 0) && NSE_ISDIR(&statb)) {
		pp->p_mtime = statb.st_mtime;
		set_expire_time(pp);
		return (pp);
	} else {
		free_pnode(pp);
		return (NULL);
	}
}


struct pnode *
path_to_pnode(pn, sub_layer)
	char		*pn;
	unsigned	sub_layer;
{
	struct pnode	*pp;
	char		compname[NFS_MAXNAMLEN + 1];

	pp = root_pnode;
	pn++;
	while (pn[0] != '\0') {
		pn = _nse_extract_name(pn, compname);
		pp = get_pnode(pp, compname);
		pp->p_sub_layer = sub_layer;
	}
	pp->p_refcnt++;
	return (pp);
}


static struct pnode *
get_pnode(parentp, name)
	struct pnode	*parentp;
	char		*name;
{
	struct tnode	*tp;

	tp = ntree_find_child((struct tnode *) parentp, name);
	if (tp != NULL) {
		return ((struct pnode *) tp);
	} else {
		return (alloc_pnode(parentp, name));
	}
}


/*
 * Hash table which maps fhandles into vnodes.  This is needed so that an
 * fhandle can be quickly resolved into a vnode when an RPC call comes into
 * the server.  Note that whiteout vnodes are also kept in this hash table,
 * because after a flush a whiteout vnode may become a regular vnode.
 * NOTE: HASH_TABLE_SIZE must be a power of 2 for FH_TABLE_HASH() to work!
 */
#define		FHTABLE_HASH(fh)	(fh & (HASH_TABLE_SIZE - 1))

static struct vnode *fhtable[HASH_TABLE_SIZE];

/*
 * Routines which operate on fhandle hash table.
 */

/*
 * Translates the fhandle 'fh' into a vnode with a pnode attached.  If
 * we don't initially find the vnode in the fhandle hash table, it may
 * be because the parent vnode has had its contents swapped out of memory.
 * If this is the case, swap in the parent directory and try the fhandle
 * lookup again.
 */
struct vnode *
fhtovp(fh)
	struct tfsfh	*fh;
{
	struct vnode	*pvp;
	struct vnode	*vp;
	extern time_t	tfsd_timestamp;

	if (fh->fh_timestamp != tfsd_timestamp) {
		return (NULL);
	}
	vp = fhandle_find(fh->fh_id);
	if (vp != NULL) {
		return (vp);
	}
	pvp = fhandle_find(fh->fh_parent_id);
	if (pvp == NULL || !pvp->v_pnode->p_swapped) {
		goto error;
	}
	swap_in_directory(pvp->v_pnode);
	vp = fhandle_find(fh->fh_id);
	if (vp != NULL) {
		return (vp);
	}
error:
#ifdef TFSDEBUG
	dprint(tfsdebug, 3, "fhtovp: can't find file for fhandle [%d %d]\n",
		fh->fh_parent_id, fh->fh_id);
#endif TFSDEBUG
	return (NULL);
}


/*
 * Maps the fhandle 'nodeid' into the corresponding vnode, and returns NULL if
 * there is no vnode for the fhandle.  Performs a search in the fhandle
 * hash table.
 */
struct vnode *
fhandle_find(nodeid)
	unsigned long	nodeid;
{
	struct vnode	*vp;

	vp = fhtable[FHTABLE_HASH(nodeid)];
	while (vp != NULL) {
		if (nodeid == vp->v_fhid) {
			if (vp->v_layer == INVALID_LAYER) {
				vnode_tree_release(vp);
				return (NULL);
			}
			if (!vp->v_whited_out && recurse_lookup_pnode(vp)) {
				return (vp);
			} else {
				return (NULL);
			}
		}
		vp = vp->v_fnext;
	}
	return (NULL);
}


/*
 * Put a vnode in the fhandle hash table
 */
void
fhandle_insert(vp)
	struct vnode	*vp;
{
	long		index;

	index = FHTABLE_HASH(vp->v_fhid);
	vp->v_fnext = fhtable[index];
	fhtable[index] = vp;
}


/*
 * Remove a vnode from the fhandle hash table
 */
void
fhandle_remove(vp)
	struct vnode	*vp;
{
	struct vnode	*vt;
	struct vnode	*vtprev = NULL;
	long		index;

	index = FHTABLE_HASH(vp->v_fhid);
	vt = fhtable[index];
	while (vt != NULL) {
		if (vt == vp) {
			if (vtprev == NULL) {
				fhtable[index] = vt->v_fnext;
			} else {
				vtprev->v_fnext = vt->v_fnext;
			}
			return;
		}
		vtprev = vt;
		vt = vt->v_fnext;
	}
}


/*
 * Hash table of vnode component names.  This is needed so that we can
 * quickly determine if a child vnode with a specified name exists under a
 * given parent vnode.
 */
static struct tnode *comptable[HASH_TABLE_SIZE];

/*
 * Routines which operate on pathname component hash table.
 */

#ifdef TFSDEBUG
comp_print()
{
	struct tnode   *tp;
	int		i;
	int		full_buckets = 0;
	int		vnode_count = 0;

	for (i = 0 ; i < HASH_TABLE_SIZE ; i++) {
		tp = comptable[i];
		if (tp) {
			full_buckets++;
			vnode_count++;
			while (tp->t_cnext != NULL) {
				vnode_count++;
				tp = tp->t_cnext;
			}
		}
	}
	dprint(0, 0,
		"comptable: %d buckets total, %d vnodes, %d buckets full\n",
		HASH_TABLE_SIZE, vnode_count, full_buckets);
}
#endif TFSDEBUG


/*
 * Try to find vnode for file 'name' with pvp vnode 'pvp'.
 * Performs a search in the pathname component hash table.
 */
struct vnode   *
find_vnode(pvp, name)
	struct vnode   *pvp;
	char           *name;
{
	struct tnode   *tp;

	tp = comp_find((struct tnode *) pvp, name);
	if (tp == NULL) {
		return (NULL);
	} else {
		return ((struct vnode *) tp);
	}
}


static struct tnode *
comp_find(ptp, name)
	struct tnode   *ptp;
	char		*name;
{
	struct tnode   *tp;

	tp = comptable[hash_string(name)];
	while (tp != NULL) {
		if ((ptp == tp->t_parent) && (NSE_STREQ(name, tp->t_name))) {
			return (tp);
		}
		tp = tp->t_cnext;
	}
	return (NULL);
}


/*
 * Put a tnode in the pathname component hash table
 */
void
comp_insert(tp)
	struct tnode   *tp;
{
	long            index;

	index = hash_string(tp->t_name);
	tp->t_cnext = comptable[index];
	comptable[index] = tp;
}


/*
 * Remove a tnode from the pathname component hash table
 */
void
comp_remove(tp)
	struct tnode   *tp;
{
	struct tnode   *tt;
	struct tnode   *ttprev = NULL;
	long            index;

	index = hash_string(tp->t_name);
	tt = comptable[index];
	while (tt != NULL) {
		if (tt == tp) {
			if (ttprev == NULL) {
				comptable[index] = tt->t_cnext;
			} else {
				ttprev->t_cnext = tt->t_cnext;
			}
			return;
		}
		ttprev = tt;
		tt = tt->t_cnext;
	}
}
