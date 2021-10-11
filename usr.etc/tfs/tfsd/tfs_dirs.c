#ifndef lint
static char sccsid[] = "@(#)tfs_dirs.c 1.1 92/07/30 Copyr 1988 Sun Micro";
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
#include <nse/hash.h>
#include "tfs_lists.h"
#include <nse/stdio.h>

/*
 * Routines to read in directories.
 *
 * TFS_BACKFILES
 *
 * Also included here are routines which operate on .tfs_backfiles files.
 * These files contain the names of all directories and files visible
 * in back (read-only) file systems.  There is a .tfs_backfiles for
 * each sub-layer, and the .tfs_backfiles for each sub-layer contains
 * the read-only files showing through from back instances of that
 * sub-layer.
 *
 * The format of the file is:
 *	<filename>  <layer #>  [<mtime>]
 *
 * <filename> is either the name of a file or is an absolute pathname
 * to a directory (the directory which contains all the files at the
 * given layer.)  There is an entry for each read-only directory that
 * has visible files.
 *
 * <layer #> is the absolute layer # of the directory.  (So the layer
 * #s aren't necessarily consecutive.)  Layers are numbered starting
 * at 1.
 *
 * <mtime> is the modify time of the file, if the file's mtime in
 * this layer is different from the file's mtime in the read-only
 * layer.  (This is done so that we don't have to copy-on-write a
 * file just to change its mtime.)
 *
 * The first read-only layer is always written into .tfs_backfiles
 * so that the file can be checked for validity -- when the searchlink
 * into the first read-only directory changes we'll be able to detect it.
 */

#define NSE_TFS_BACKF_VERSION 3

#define HASH_SIZE	100
#define HASH_VALUE	1
#define INFINITY	1000000000

#define IS_DIRNAME(entry)		(entry->fname[0] == '/')

/*
 * Structure to help in iterating through lists of Filelist entries.
 */
typedef struct Flist_iter {
	Nse_listelem	elem;		/* Current element */
	Nse_listelem	end;		/* End of list */
	Filelist	entry;		/* Data of current element */
} *Flist_iter, Flist_iter_rec;

#define END_OF_LIST(flist)	(flist.elem == flist.end)


extern int	free();

int		tfs_readdir();
static bool_t	add_entry();

int		validate_directory();
static int	readdir_vnodes();
static struct vnode *matching_vnode();
static void	copy_vnodes_and_pnodes();

static bool_t	readdir_vnodes_one_layer();

static bool_t	read_and_verify_backlists();
static int	build_backlists();
static bool_t	build_backlist_one_layer();
static void	add_dir_to_backlist();
static bool_t	have_all_ro_backlists();
static void	copy_backlists();
static void	copy_backlist_entry();
static bool_t	add_to_backlist_unique();

static void	readdir_backlists();
static void	get_next_entry();
static void	destroy_backlists();

static struct vnode *create_child_vnode();
static void	create_whiteout_vnodes();
static void	insert_whiteout_into_hash();

int		change_backlist_mtime();
static void	change_mtime();
int		change_backlist_name();
static void	change_name();
static int	modify_backlist();

static Nse_list	read_backlist();
static int	write_backlist();
static Nse_err	read_backlist_entry();
static Nse_err	write_backlist_entry();
static void	add_to_backlist();

static Hash	init_dir_hash();
static bool_t	good_name();
static bool_t	backlist_name_eq();
static bool_t	backlist_is_dir();

/*
 * Variables shared between tfs_readdir() and add_entry()
 */

static char	*buf_base;
static int	buf_size;
static int	buf_maxcount;
static long	last_offset;


/*
 * Procedure 16.  Read directory entries.
 *
 * There are some weird things to look out for here.  rda->rda_offset is
 * either 0 or it is the offset returned from a previous readdir.  It is an
 * opaque value used by the server to find the correct directory block to
 * read.  In this implementation, the offset of a directory entry is the
 * index into the list of child vnodes for the directory.
 */
int
tfs_readdir(pvp, rda, rd)
	struct vnode   *pvp;
	struct nfsrddirargs *rda;
	struct nfsreaddirres *rd;
{
	static struct vnode *last_pvp;
	static struct vnode *last_vp;
	struct vnode	*vp;
	struct vnode	*next_vp;
	int		result;
	long		offset;

	/*
	 * No longer test for cd access to directory here.  If the
	 * directory is readable, then we should be able to read it,
	 * regardless of the search perms of the directory.  (This is the
	 * way the UFS behaves.)
	 */

	if (!pvp->v_dir_valid) {
		last_pvp = NULL;
	}
	if (result = validate_directory(pvp)) {
		return (result);
	}
	if ((pvp == last_pvp) && (rda->rda_offset == last_offset)) {
		vp = last_vp;
	} else {
		last_pvp = pvp;
		last_offset = rda->rda_offset;
		vp = CHILD_VNODE(pvp);
		if (vp != NULL && vp->v_whited_out) {
			vp = next_file(vp);
		}
		/*
		 * '.' and '..' are at offsets 1 and 2.
		 */
		offset = 3;
		while ((vp != NULL) && (offset <= last_offset)) {
			vp = next_file(vp);
			offset++;
		}
	}
	rd->rd_bufsize = rda->rda_count;
#ifdef SUN_OS_4
	rd->rd_entries = (struct dirent *) read_result_buffer;
#else
	rd->rd_entries = (struct direct *) read_result_buffer;
#endif
	buf_base = read_result_buffer;
	buf_size = 0;
	/*
	 * Leave 1000 bytes free in result buffer so that UDP buffer won't
	 * overflow when XDR adds stuff later.
	 */
	buf_maxcount = rd->rd_bufsize - 1000;
	if (last_offset == 0) {
		(void) add_entry(pvp->v_fhid, ".");
		(void) add_entry((PARENT_VNODE(pvp))->v_fhid, "..");
	}
	while (vp != NULL) {
		if (vp->v_layer != INVALID_LAYER) {
			if (!add_entry(vp->v_fhid, vp->v_name)) {
				break;
			}
			vp = next_file(vp);
		} else {
			next_vp = next_file(vp);
			vnode_tree_release(vp);
			vp = next_vp;
		}
	}
	rd->rd_size = buf_size;
	rd->rd_offset = last_offset;
	rd->rd_eof = (vp == NULL);
	last_vp = vp;
	return (0);
}


/*
 * Add one entry to the readdir result buffer with the specified file # and
 * name.  Returns TRUE if the entry fits in the result buffer.
 */
static bool_t
add_entry(nodeid, name)
	long		nodeid;
	char		*name;
{
	struct udirect	*udp;
	int		len;
	int		reclen;

	len = strlen(name);
	reclen = UENTRYSIZ(len);
	if (buf_size + reclen >= buf_maxcount) {
		return (FALSE);
	}
	udp = (struct udirect *) buf_base;
	udp->d_fileno = nodeid;
	strcpy(udp->d_name, name);
	udp->d_namlen = len;
	udp->d_reclen = reclen;
	udp->d_offset = ++last_offset;
	buf_size += reclen;
	buf_base += reclen;
	return (TRUE);
}


/*
 * Validate the directory with vnode 'vp'.  Returns 0 on success, a positive
 * error code if an error occurred.
 */
int
validate_directory(vp)
	struct vnode	*vp;
{
	int	result = 0;
	
	if (vp->v_pnode->p_swapped) {
		swap_in_directory(vp->v_pnode);
	} else if (vp->v_pnode->p_in_queue) {
		swqueue_use_pnode(vp->v_pnode);
	}
	if (!vp->v_back_owner && vp->v_dir_valid) {
		struct pnode	*pp;
		long		time;

		time = get_current_time(FALSE);
		for (pp = vp->v_pnode; pp != NULL;
		     pp = get_next_pnode(vp, pp)) {
			if (pp->p_expire <= time) {
				char		name[MAXNAMLEN];
				struct stat	statb;

				ptoname_or_pn(pp, name);
				if (stat(name, &statb) < 0) {
					return (errno);
				}
				if (pp->p_mtime != statb.st_mtime) {
					if (pp != vp->v_pnode) {
						update_ctime(vp, FALSE);
					}
					vp->v_dir_valid = FALSE;
					break;
				} else {
					set_expire_time(pp);
				}
			}
		}
	}
	if (!vp->v_dir_valid) {
		result = readdir_vnodes(vp);
		clear_visited_pnodes();
		return result;
	} else {
		return (0);
	}
}


/*
 * Build child vnodes and directory pnodes for directory 'pvp'.
 */
static int
readdir_vnodes(pvp)
	struct vnode	*pvp;
{
	struct vnode	*vp;
	struct pnode	*pp;
	struct pnode	*prev_pp = NULL;
	struct pnode	*front_pnodes[MAX_SUB_LAYER + 1];
	Nse_list	backlists[MAX_SUB_LAYER + 1];
	Nse_whiteout	wp;
	int		layer = 0;
	bool_t		rename_recovered = FALSE;
	int		result = 0;

	for (vp = CHILD_VNODE(pvp); vp != NULL; vp = NEXT_VNODE(vp)) {
		vp->v_layer = INVALID_LAYER;
		vp->v_back_layer = INVALID_LAYER;
		vp->v_whited_out = FALSE;
		vp->v_back_whited_out = FALSE;
		vp->v_mtime = 0;
		if (vp->v_pnode != NULL && vp->v_pnode->p_type != PTYPE_DIR) {
			release_pnodes(vp);
		}
	}
	if (pvp->v_layer == 0) {
		if (vp = matching_vnode(pvp)) {
			copy_vnodes_and_pnodes(vp, pvp);
			pvp->v_dir_valid = TRUE;
			return (0);
		}
	} else if (result = promote_file(pvp)) {
		/*
		 * Always promote the directory to the front-most writeable
		 * layer.  If there is at least one read-only layer, we
		 * need to promote the directory to hold a .tfs_backfiles
		 * file.  If there is no read-only layer, we still want the
		 * directory to exist in all the writeable layers.
		 */
		return (result);
	}
	/*
	 * If there is more than one pnode for this directory (because the
	 * directory was just promoted or swapped in,) release all but the
	 * front one, as the rest will be built below.
	 */
	release_back_pnodes(pvp);
	pp = pvp->v_pnode;
	if (!pvp->v_back_owner) {
		char		name[MAXNAMLEN];
		struct stat	statb;

		ptoname_or_pn(pp, name);
		if (stat(name, &statb) < 0) {
			return (errno);
		}
		pp->p_mtime = statb.st_mtime;
		set_expire_time(pp);
	}
	bzero((char *) front_pnodes, sizeof front_pnodes);
	do {
		/*
		 * If the user doesn't own files showing through from
		 * read-only filesystems, don't use .tfs_backfiles.
		 * Otherwise, read the backfiles after reading all the
		 * writeable sub-layers.
		 */
		if (front_pnodes[pp->p_sub_layer] == NULL) {
			front_pnodes[pp->p_sub_layer] = pp;
		} else if (pvp->v_back_owner) {
			break;
		}
		if (!readdir_vnodes_one_layer(pvp, pp, layer)) {
			return (errno);
		}
		if (prev_pp != NULL) {
			set_next_pnode(pvp, prev_pp, pp, layer - 1);
		}
		prev_pp = pp;
		pp = follow_searchlink(pp, pvp->v_environ_id, FIRST_SUBL(pvp),
				       &wp, &rename_recovered);
		create_whiteout_vnodes(pvp, layer, wp);
		nse_dispose_whiteout(wp);
		if (rename_recovered) {
			return (readdir_vnodes(pvp));
		}
		layer++;
	} while (pp != NULL);
	if (pp != NULL) {
		bzero((char *) backlists, sizeof backlists);
		if (read_and_verify_backlists(front_pnodes, pp, backlists)) {
			free_pnode(pp);
		} else if (result = build_backlists(front_pnodes, pp,
					FIRST_SUBL(pvp), pvp->v_environ_id,
					backlists)) {
			destroy_backlists(backlists);
			return (result);
		}
		readdir_backlists(pvp, prev_pp, backlists, layer - 1);
		destroy_backlists(backlists);
	} else {
		set_next_pnode(pvp, prev_pp, (struct pnode *) NULL, layer - 1);
	}
	pvp->v_dir_valid = TRUE;
	/*
	 * Put the new directory in the swap queue only if it has more
	 * than one child.
	 */
	if (pvp->v_back_owner && !pvp->v_pnode->p_in_queue) {
		if ((vp = CHILD_VNODE(pvp)) && NEXT_VNODE(vp)) {
			swqueue_add_pnode(pvp->v_pnode);
		}
	}
	return (0);
}


/*
 * 'vp' is a directory vnode with only one pnode.  Determine if there
 * is another vnode for the same environment which has the same front
 * pnode as vp.  If there is, return that vnode.
 */
static struct vnode *
matching_vnode(vp)
	struct vnode	*vp;
{
	struct vnode	*vnodes[MAX_NAMES];

	bzero((char *) vnodes, sizeof vnodes);
	get_vnodes(vp->v_pnode, vnodes);
	if (vnodes[0] && vnodes[0]->v_dir_valid &&
	    vnodes[0]->v_environ_id == vp->v_environ_id) {
		/*
		 * Note that vnodes[0] will not have the correct environ_id
		 * if this directory does not exist yet in the front
		 * sub-layer.
		 */
		return (vnodes[0]);
	}
	return (NULL);
}


/*
 * Setup vnode 'new_vp' to have the same pnodes and child vnodes as the
 * vnode 'old_vp'.  The child vnodes are all created with different
 * nodeid's from those of the original child vnodes.
 */
static void
copy_vnodes_and_pnodes(old_vp, new_vp)
	struct vnode	*old_vp;
	struct vnode	*new_vp;
{
	struct pnode	*pp;
	struct pnode	*next_pp;
	int		layer = 0;
	struct vnode	*vp;
	struct vnode	*nvp;

	/*
	 * Copy directory pnodes
	 */
	release_back_pnodes(new_vp);
	pp = new_vp->v_pnode;
	next_pp = get_next_pnode(old_vp, old_vp->v_pnode);
	while (next_pp != NULL) {
		next_pp->p_refcnt++;
		set_next_pnode(new_vp, pp, next_pp, layer);
		layer++;
		pp = next_pp;
		next_pp = get_next_pnode(old_vp, next_pp);
	}
	set_next_pnode(new_vp, pp, (struct pnode *) NULL, layer);

	/*
	 * Copy child vnodes
	 */
	for (vp = CHILD_VNODE(old_vp); vp != NULL; vp = NEXT_VNODE(vp)) {
		nvp = create_child_vnode(new_vp, vp->v_name,
					 (int) vp->v_layer, vp->v_mtime);
		nvp->v_whited_out = vp->v_whited_out;
		nvp->v_back_layer = vp->v_back_layer;
		nvp->v_back_whited_out = vp->v_back_whited_out;
	}
}


/*
 * Read directory entries from one layer.  Creates vnodes for all files
 * seen in this layer.
 */
static bool_t
readdir_vnodes_one_layer(pvp, pp, layer)
	struct vnode	*pvp;
	struct pnode	*pp;
	int		layer;
{
	DIR		*dirp;
	struct direct	*dp;

	dirp = tfs_opendir(pp);
	if (dirp == NULL) {
		return (FALSE);
	}
	dp = readdir(dirp);
	while (dp != NULL) {
		if (good_name(dp->d_name)) {
			(void) create_child_vnode(pvp, dp->d_name, layer,
						  (long) 0);
		}
		dp = readdir(dirp);
	}
	tfs_closedir(dirp);
	return (TRUE);
}


/*
 * 'front_pnodes', an array indexed by sub-layer #, contains the pnodes
 * of all the writeable sub-layers.  Read in the backlist for each
 * of the writeable sub-layers into 'backlists', which is an array of
 * backlists indexed by sub-layer #.  Returns TRUE if the backlists are
 * valid.
 * 'ro_pp' is the first read-only pnode.
 */
static bool_t
read_and_verify_backlists(front_pnodes, ro_pp, backlists)
	struct pnode	**front_pnodes;
	struct pnode	*ro_pp;
	Nse_list	*backlists;
{
	char		pn[MAXPATHLEN];
	int		i;
	Filelist	fl;

	for (i = 0; i <= MAX_SUB_LAYER; i++) {
		if (front_pnodes[i]) {
			backlists[i] = read_backlist(front_pnodes[i]);
			if (backlists[i] == NULL) {
				goto error;
			}
		}
	}
	fl = (Filelist) nse_list_search(backlists[ro_pp->p_sub_layer],
					backlist_is_dir);
	ptopn(ro_pp, pn);
	if (fl != NULL && fl->layer == 1 && NSE_STREQ(pn, fl->fname)) {
		return TRUE;
	}
error:
	destroy_backlists(backlists);
	return FALSE;
}


/*
 * Build .tfs_backfiles files for all sub-layers of the directory whose
 * first read-only layer is 'pp'.  Read in the directory entries for all
 * sub-layers of the first read-only layer, and then use the backfiles
 * of these sub-layers to finish building the backfiles.  If there aren't
 * backfiles in the first read-only layer, then recursively build them.
 */
static int
build_backlists(front_pnodes, pp, first_sub_layer, environ_id, backlists)
	struct pnode	**front_pnodes;
	struct pnode	*pp;
	unsigned	first_sub_layer;
	unsigned	environ_id;
	Nse_list	*backlists;
{
	struct pnode	*ro_pnodes[MAX_SUB_LAYER + 1];
	Hash		backhashs[MAX_SUB_LAYER + 1];
	Nse_list	ro_backlists[MAX_SUB_LAYER + 1];
	Nse_whiteout	wp;
	int		sub_layer;
	int		last_sub_layer = -1;
	int		i;
	int		result = 0;

#ifdef TFSDEBUG
	dprint(tfsdebug, 3, "build_backlists()\n");
#endif TFSDEBUG
	bzero((char *) ro_pnodes, sizeof ro_pnodes);
	bzero((char *) ro_backlists, sizeof ro_backlists);
	for (i = 0; i <= MAX_SUB_LAYER; i++) {
		backhashs[i] = init_dir_hash();
		backlists[i] = filelist_create();
	}
	do {
		sub_layer = pp->p_sub_layer;
		if (ro_pnodes[sub_layer] == NULL &&
		    sub_layer > last_sub_layer) {
			ro_pnodes[sub_layer] = pp;
		} else {
			break;
		}
		last_sub_layer = sub_layer;
		ro_backlists[sub_layer] = read_backlist(pp);
		if (!build_backlist_one_layer(pp, sub_layer, backhashs,
					      backlists[sub_layer])) {
			result = errno;
			goto error;
		}
		add_dir_to_backlist(pp, backlists[sub_layer]);
		pp = follow_searchlink(pp, environ_id, first_sub_layer, &wp,
				       (bool_t *) NULL);
		insert_whiteout_into_hash(backhashs[sub_layer], wp);
		nse_dispose_whiteout(wp);
	} while (pp != NULL);
	if (pp != NULL) {
		if (!have_all_ro_backlists(ro_pnodes, ro_backlists,
					   first_sub_layer)) {
			destroy_backlists(ro_backlists);
			bzero((char *) ro_backlists, sizeof ro_backlists);
			if (result = build_backlists(ro_pnodes, pp,
						first_sub_layer, environ_id,
						ro_backlists)) {
				goto error;
			}
		} else {
			free_pnode(pp);
		}
		copy_backlists(backlists, backhashs, ro_backlists,
			       first_sub_layer);
	}
	for (i = first_sub_layer; i <= MAX_SUB_LAYER; i++) {
		if (front_pnodes[i] == NULL) {
			continue;
		}
		if (write_backlist(front_pnodes[i], backlists[i])) {
			nse_log_message(
				"warning: can't write .tfs_backfiles");
			print_pnode_path(front_pnodes[i]);
		}
	}
error:
	for (i = 0; i <= MAX_SUB_LAYER; i++) {
		nse_hash_destroy(backhashs[i], (Nse_voidfunc) free,
				 (Nse_voidfunc) NULL);
		if (ro_pnodes[i] != NULL) {
			free_pnode(ro_pnodes[i]);
		}
	}
	destroy_backlists(ro_backlists);
	return (result);
}

static bool_t
build_backlist_one_layer(pp, sub_layer, backhashs, backlist)
	struct pnode	*pp;
	int		sub_layer;
	Hash		*backhashs;
	Nse_list	backlist;
{
	DIR		*dirp;
	struct direct	*dp;

	dirp = tfs_opendir(pp);
	if (dirp == NULL) {
		return (FALSE);
	}
	dp = readdir(dirp);
	while (dp != NULL) {
		if (good_name(dp->d_name)) {
			(void) add_to_backlist_unique(backlist, backhashs,
						      dp->d_name, 1,
						      sub_layer, 0L);
		}
		dp = readdir(dirp);
	}
	tfs_closedir(dirp);
	return (TRUE);
}


/*
 * Add the directory with pnode 'pp' to 'backlist'.  This routine exists
 * solely to prevent the runtime stack of build_backlists() from getting
 * too large in case we have to recurse a number of times.  Note that
 * we always put the directory names of the first read-only layer into the
 * backlist.  These directory names are needed by read_and_verify_backlists()
 * to determine whether or not a backlist is valid; and by
 * tfs_set_searchlink() to determine whether or not a new searchlink
 * is different from the one already set.
 */
static void
add_dir_to_backlist(pp, backlist)
	struct pnode	*pp;
	Nse_list	backlist;
{
	char		pn[MAXPATHLEN];

	ptopn(pp, pn);
	add_to_backlist(backlist, pn, 1, 0L);
}


static bool_t
have_all_ro_backlists(ro_pnodes, ro_backlists, first_sub_layer)
	struct pnode	**ro_pnodes;
	Nse_list	*ro_backlists;
	unsigned	first_sub_layer;
{
	int		i;

	for (i = first_sub_layer; i <= MAX_SUB_LAYER; i++) {
		if (ro_pnodes[i] == NULL || ro_backlists[i] == NULL) {
			return (FALSE);
		}
	}
	return (TRUE);
}


/*
 * Copy entries from backlists in sub-layers of the first read-only layer.
 */
static void
copy_backlists(backlists, backhashs, ro_backlists, first_sub_layer)
	Nse_list	*backlists;
	Hash		*backhashs;
	Nse_list	*ro_backlists;
	unsigned	first_sub_layer;
{
	bool_t		file_added;
	int		i;

	file_added = FALSE;
	for (i = first_sub_layer; i <= MAX_SUB_LAYER; i++) {
		nse_list_iterate(ro_backlists[i], copy_backlist_entry,
				 backlists[i], backhashs, i, &file_added);
	}
}


static void
copy_backlist_entry(fl, backlist, backhashs, sub_layer, new_file)
	Filelist	fl;
	Nse_list	backlist;
	Hash		*backhashs;
	int		sub_layer;
	bool_t		*new_file;
{
	if (!IS_DIRNAME(fl)) {
		if (add_to_backlist_unique(backlist, backhashs, fl->fname,
					   fl->layer + 1, sub_layer,
					   fl->mtime)) {
			*new_file = TRUE;
		}
	} else {
		if (*new_file) {
			add_to_backlist(backlist, fl->fname, fl->layer + 1,
					fl->mtime);
		}
		*new_file = FALSE;
	}
}


/*
 * Returns TRUE if 'fname' is added to the backlist; FALSE if the name
 * is already in the backlist.  Note that if 'fname' is in a variant
 * sub-layer, we must also make sure that 'fname' hasn't been added to
 * the shared-source backlist.
 */
static bool_t
add_to_backlist_unique(backlist, backhashs, fname, layer, sub_layer, mtime)
	Nse_list	backlist;
	Hash		*backhashs;
	char		*fname;
	int		layer;
	int		sub_layer;
	long		mtime;
{
	char		*file_name;
	int		i;

	if (sub_layer < MAX_SUB_LAYER) {
		for (i = MAX_SUB_LAYER; i > sub_layer; i--) {
			if (nse_hash_lookup(backhashs[i], fname)) {
				return (FALSE);
			}
		}
	}
	file_name = NSE_STRDUP(fname);
	if (nse_hash_insert(backhashs[sub_layer], file_name, HASH_VALUE)) {
		free(file_name);
		return (FALSE);
	}
	add_to_backlist(backlist, fname, layer, mtime);
	return (TRUE);
}


/*
 * 'backlists' is a list containing backlists for each of the sub-layers.
 * Convert the file and directory names in the backlists into child vnodes
 * and directory pnodes for the directory vnode 'pvp'.
 * 'pp' is the pnode of the last writeable layer.
 */
static void
readdir_backlists(pvp, pp, backlists, layer)
	struct vnode	*pvp;
	struct pnode	*pp;
	Nse_list	*backlists;
	int		layer;
{
	Flist_iter_rec	flist[MAX_SUB_LAYER + 1];
	struct pnode	*next_pp;
	unsigned	first_sub_layer = FIRST_SUBL(pvp);
	int		max_sub_layer = MAX_SUB_LAYER;
	int		min_layer;
	unsigned	sub_layer;
	int		i;

	for (i = first_sub_layer; i <= MAX_SUB_LAYER; i++) {
		if (backlists[i] == NULL) {
			max_sub_layer = i - 1;
			break;
		}
		flist[i].end = nse_list_end(backlists[i]);
		flist[i].elem = nse_list_first_elem(backlists[i]);
		flist[i].entry = (Filelist) nse_listelem_data(flist[i].elem);
	}
	while (1) {
		layer++;
		min_layer = INFINITY;
		/*
		 * Select the sub-layer with the minimum absolute layer #.
		 */
		for (i = first_sub_layer; i <= max_sub_layer; i++) {
			if (!END_OF_LIST(flist[i]) &&
			    flist[i].entry->layer < min_layer) {
				min_layer = flist[i].entry->layer;
				sub_layer = i;
			}
		}
		if (min_layer == INFINITY) {
			break;
		}
		/*
		 * flist[sub_layer] now points to a series of file entries
		 * followed by an entry for the directory in which the files
		 * can be found.
		 */
		while (!IS_DIRNAME(flist[sub_layer].entry)) {
			(void) create_child_vnode(pvp,
					flist[sub_layer].entry->fname,
					layer, flist[sub_layer].entry->mtime);
			get_next_entry(&flist[sub_layer]);
		}
		/*
		 * flist[sub_layer] now points to a directory entry.  If
		 * layer == 0, then this is an extra directory name for
		 * the first layer.  (Extra directory names used to exist
		 * in the .tfs_backfiles of the frontmost sub-layer in
		 * rev 3 backfiles.)
		 * XXX remove this check when BACKF_VERSION changed.
		 */
		if (flist[sub_layer].entry->layer > 0) {
			next_pp = path_to_pnode(flist[sub_layer].entry->fname,
						sub_layer);
			set_next_pnode(pvp, pp, next_pp, layer - 1);
			pp = next_pp;
		} else {
			layer--;
		}
		get_next_entry(&flist[sub_layer]);
	}
	set_next_pnode(pvp, pp, (struct pnode *) NULL, layer - 1);
}


static void
get_next_entry(flist)
	Flist_iter	flist;
{
	flist->elem = nse_listelem_next(flist->elem);
	flist->entry = (Filelist) nse_listelem_data(flist->elem);
}


static void
destroy_backlists(backlists)
	Nse_list	*backlists;
{
	int		i;

	for (i = 0; i <= MAX_SUB_LAYER; i++) {
		if (backlists[i] != NULL) {
			nse_list_destroy(backlists[i]);
		}
	}
}


/*
 * File 'name' has been seen in directory 'pvp' at layer # 'layer'.  Create
 * a vnode for the file if it hasn't been seen yet.
 */
static struct vnode *
create_child_vnode(pvp, name, layer, mtime)
	struct vnode	*pvp;
	char		*name;
	int		layer;
	long		mtime;
{
	struct vnode	*vp;

	vp = find_vnode(pvp, name);
	if (vp == NULL) {
		vp = create_vnode(pvp, name, (long) 0);
	}
	if (vp->v_layer == INVALID_LAYER) {
		vp->v_layer = layer;
	} else if (vp->v_back_layer == INVALID_LAYER) {
		vp->v_back_layer = layer;
	}
	vp->v_mtime = mtime;
	return (vp);
}


/*
 * Create white-out vnodes for each of the entries in the whiteout list
 * 'wp'.
 */
static void
create_whiteout_vnodes(pvp, layer, wp)
	struct vnode	*pvp;
	int		layer;
	Nse_whiteout	wp;
{
	struct vnode	*vp;

	while (wp != NULL) {
		vp = find_vnode(pvp, wp->name);
		if (vp == NULL) {
			vp = create_vnode(pvp, wp->name, (long) 0);
		}
		if (vp->v_layer == INVALID_LAYER) {
			vp->v_whited_out = TRUE;
			vp->v_layer = layer;
		} else if (vp->v_back_layer == INVALID_LAYER) {
			vp->v_back_whited_out = TRUE;
			vp->v_back_layer = layer;
		}
		wp = wp->next;
	}
}


/*
 * Insert the entries in the whiteout list 'wp' into the readdir hash table.
 */
static void
insert_whiteout_into_hash(readdir_hash, wp)
	Hash		readdir_hash;
	Nse_whiteout	wp;
{
	char		*file_name;

	while (wp != NULL) {
		file_name = NSE_STRDUP(wp->name);
		if (nse_hash_insert(readdir_hash, file_name, HASH_VALUE)) {
			free(file_name);
		}
		wp = wp->next;
	}
}


int
change_backlist_mtime(pp, name, mtime)
	struct pnode	*pp;
	char		*name;
	long		mtime;
{
	return modify_backlist(pp, name, change_mtime, (char *) mtime);
}


/* ARGSUSED */
static void
change_mtime(list, fl, mtime)
	Nse_list	list;
	Filelist	fl;
	long		mtime;
{
	fl->mtime = mtime;
}


/*
 * Change the name of the entry named 'name' to 'newname' in the backlist
 * of directory 'pp'.  If 'name' was the first directory entry in the
 * backlist, then insert the old name at the front of the backlist.
 */
int
change_backlist_name(pp, name, newname)
	struct pnode	*pp;
	char		*name;
	char		*newname;
{
	return modify_backlist(pp, name, change_name, newname);
}


static void
change_name(list, fl, newname)
	Nse_list	list;
	Filelist	fl;
	char		*newname;
{
	Nse_listelem	elem;
	Filelist	fl_new;

	if (fl->layer == 1 && IS_DIRNAME(fl)) {
		elem = nse_list_create_elem(list);
		fl_new = (Filelist) nse_listelem_data(elem);
		fl_new->fname = NSE_STRDUP(fl->fname);
		fl_new->layer = 1;
		fl_new->mtime = fl->mtime;
		nse_list_insert_head(list, elem);
	}
	free(fl->fname);
	fl->fname = NSE_STRDUP(newname);
}


/*
 * Modify the backlist entry for 'name' in the backlist for directory 'pp'.
 * 'modify_func' is the function to be used to change the entry, and 'arg'
 * is the argument to 'modify_func'.
 */
static int
modify_backlist(pp, name, modify_func, arg)
	struct pnode	*pp;
	char		*name;
	void		(*modify_func)();
	char		*arg;
{
	Nse_list	backlist;
	Filelist	fl;
	int		result = 0;

	backlist = read_backlist(pp);
	if (backlist == NULL) {
		return ENOENT;
	}
	fl = (Filelist) nse_list_search(backlist, backlist_name_eq, name);
	if (fl == NULL) {
		result = ENOENT;
		goto error;
	}
	modify_func(backlist, fl, arg);
	result = write_backlist(pp, backlist);
error:
	nse_list_destroy(backlist);
	return result;
}


static Nse_list
read_backlist(pp)
	struct pnode	*pp;
{
	char		path[MAXPATHLEN];
	FILE		*file;
	Nse_list	list;
	Filelist	fl;
	int		count;
	int		version;
	bool_t		eof = FALSE;
	Nse_err		err;

	ptoname_or_pn(pp, path);
	nse_pathcat(path, NSE_TFS_BACK_FILE);
	file = open_tfs_file(path, "r", (Nse_err *) NULL);
	if (file == NULL) {
		return (NULL);
	}
	/*
	 * Make sure that the version # in the file is correct.
	 */
	err = nse_fscanf(path, &count, file, NSE_TFS_VERSION_FORMAT, &version);
	if (err || count != 1 || version != NSE_TFS_BACKF_VERSION) {
		if (err) {
			nse_log_err_print(err);
		}
		fclose(file);
		return (NULL);
	}
	list = filelist_create();
	err = read_backlist_entry(file, path, &fl, &eof);
	while (!eof) {
		(void) nse_list_add_new_data(list, (Nse_opaque) fl);
		err = read_backlist_entry(file, path, &fl, &eof);
	}
	if (err) {
		nse_log_err_print(err);
	}
	fclose(file);
	return (list);
}


static int
write_backlist(pp, list)
	struct pnode	*pp;
	Nse_list	list;
{
	char		path[MAXPATHLEN];
	FILE		*file;
	Nse_err		err;

	ptoname_or_pn(pp, path);
	nse_pathcat(path, NSE_TFS_BACK_FILE);
	file = open_tfs_file(path, "w", (Nse_err *) NULL);
	if (file == NULL) {
		return (errno);
	}
	if ((err = nse_fprintf(path, file, NSE_TFS_VERSION_FORMAT,
				NSE_TFS_BACKF_VERSION)) ||
	    (err = (Nse_err) nse_list_iterate_or(list, write_backlist_entry,
						 file, path)) ||
	    (err = nse_fclose(path, file))) {
		nse_log_err_print(err);
		fclose(file);
		(void) unlink(path);
		return (err->code);
	}
	return (0);
}


#define BACK_FILE_FORMAT	"%s %d %ld\n"


static Nse_err
read_backlist_entry(file, path, entryp, eofp)
	FILE		*file;
	char		*path;
	Filelist	*entryp;
	bool_t		*eofp;
{
	Filelist	fl;
	char		line[MAXPATHLEN];
	char		name[MAXPATHLEN];
	int		layer;
	long		mtime;
	int		nitems;
	Nse_err		err;

	/*
	 * Can't use fscanf() because if there is no mtime on the current
	 * line and the name of the file on the next line is a number,
	 * fscanf() would read the next filename in as the mtime.
	 */
	err = nse_fgets(path, line, MAXPATHLEN, file, eofp);
	if (*eofp) {
		return err;
	}
	nitems = sscanf(line, BACK_FILE_FORMAT, name, &layer, &mtime);
	if (nitems < 2) {
		*eofp = TRUE;
		return NULL;
	}
	fl = NSE_NEW(Filelist);
	fl->fname = NSE_STRDUP(name);
	fl->layer = layer;
	if (nitems == 3) {
		fl->mtime = mtime;
	}
	*entryp = fl;
	return NULL;
}


static Nse_err
write_backlist_entry(fl, file, path)
	Filelist	fl;
	FILE		*file;
	char		*path;
{
	if (fl->mtime != 0) {
		return nse_fprintf(path, file, BACK_FILE_FORMAT,
				   fl->fname, fl->layer, fl->mtime);
	} else {
		return nse_fprintf(path, file, "%s %d\n",
				   fl->fname, fl->layer);
	}
}


static void
add_to_backlist(list, name, layer, mtime)
	Nse_list	list;
	char		*name;
	int		layer;
	long		mtime;
{
	Filelist	fl;

	fl = (Filelist) nse_list_add_new_elem(list);
	fl->fname = NSE_STRDUP(name);
	fl->layer = layer;
	fl->mtime = mtime;
}


static Hash
init_dir_hash()
{
	return (nse_hash_create(HASH_SIZE, NULL,
				(Nse_boolfunc) _nse_streq_func,
				_nse_hash_string, (Nse_intfunc) NULL, NULL,
				(Nse_intfunc) NULL, 7));
}


static bool_t
good_name(name)
	char		*name;
{
	return (!NSE_STREQ(name, ".") && !NSE_STREQ(name, "..") &&
		!is_tfs_special_file(name));
}


/*
 * Filelist routines
 */

static bool_t
backlist_is_dir(fl)
	Filelist	fl;
{
	return (IS_DIRNAME(fl));
}


static bool_t
backlist_name_eq(fl, name)
	Filelist	fl;
	char		*name;
{
	return (NSE_STREQ(fl->fname, name));
}
