#ifndef lint
static char sccsid[] = "@(#)tfs_backlink.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

#include <nse/param.h>
#include <nse/util.h>
#include "headers.h"
#include "vnode.h"
#include "tfs.h"
#include "subr.h"
#include "tfs_lists.h"
#include <nse/searchlink.h>

/*
 * Routines to remove and rename directories.  These routines remove/rename
 * the directory in *all* variants, using the backlinks in .tfs_info to
 * determine the front layer in each variant.  Renaming a directory can
 * potentially be a long-running operation, since all the searchlinks and
 * backlinks of sub-directories have to be renamed.  Therefore, the old and
 * new name of the directory being renamed are recorded, to allow
 * completion of the rename later should the tfsd fail to finish the rename
 * (either because the tfsd is killed or the machine crashes.)
 *
 * The following rename records are written, to allow partially completed
 * directory renames to be recovered:
 *
 * !TFS_RENAME1 old_pattern new_pattern
 *	Written into the source parent directory in the shared sub-layer.
 *	'old_pattern' and 'new_pattern' can be fed to replace_substring()
 *	to determine the new directory name from the old name.
 *
 * !TFS_RENAME2 shared_path
 *	Written into the source parent directories in the front sub-layers.
 *	Also written into the destination parent directory, if the directory
 *	is being renamed into a different parent directory.  This record
 *	points to the directory containing the !TFS_RENAME1 record.
 */

int		remove_directory();
static int	bl_verify_dir_empty();
static int	verify_dir_empty();
static int	bl_physical_rmdir();
static int	physical_rmdir();
static void	remove_tfs_file();
int		rename_directory();
int		finish_rename_directory();
static int	finish_rename_dir();
int		whiteout_other_dirs();
static int	bl_whiteout();
static int	get_backlinks();
static int	bl_rename_dir();
static int	rename_shared_dir();
static int	recursive_directory_op();
static int	rename_searchlink();
static int	rename_backlinks();
static int	create_parent_dir();
static int	clear_all_rename_records();
static int	bl_clear_rename_record();
static int	set_rename_record();
static int	clear_rename_record();
Nse_whiteout	bl_list_get_rename_elem();
static void	bl_list_remove_elem();
static void	replace_substring();
static char	*common_suffix();
static int	update_backlinks();
int		exists_in_variant();
static int	bl_verify_no_variant();

/*
 * Remove the directory with vnode 'vp'.  This routine returns 0 if the
 * directory is successfully removed, a positive error code otherwise.
 */
int
remove_directory(vp)
	struct vnode	*vp;
{
	struct pnode	*pp;
	char		searchlink[MAXPATHLEN];
	Nse_list	backlinks;
	int		result;

	if (result = validate_directory(vp)) {
		return result;
	}
	pp = vp->v_pnode;
	while (pp->p_sub_layer < MAX_SUB_LAYER) {
		pp = get_next_pnode(vp, pp);
	}
	if ((result = change_to_dir(PARENT_PNODE(pp))) ||
	    (result = get_backlinks(pp->p_name, searchlink, &backlinks, pp))) {
		return result;
	}
	if (nse_list_nelems(backlinks) == 0 && pp != vp->v_pnode) {
		vtovn(vp, searchlink);
		nse_log_message(
	    "warning: can't remove directory %s, need Version 2 .tfs_info\n",
			searchlink);
		result = EACCES;
		goto error;
	}
	if ((result = (int) nse_list_iterate_or(backlinks,
						bl_verify_dir_empty)) ||
	    (result = verify_dir_empty(pp, (char *) NULL,
				       (struct vnode **) NULL)) ||
	    (result = (int) nse_list_iterate_or(backlinks,
						bl_physical_rmdir)) ||
	    (result = physical_rmdir(pp))) {
		goto error;
	}
	result = update_directory(PARENT_PNODE(pp), pp->p_name,
				  DIR_REMOVE_FILE, FALSE, TRUE, NULL);
error:
	nse_list_destroy(backlinks);
	return result;
}


static int
bl_verify_dir_empty(bp)
	Backlink	bp;
{
	return verify_dir_empty(bp->pnode, bp->varname, &bp->dummy_vnode);
}


static int
verify_dir_empty(pp, varname, dummy_vpp)
	struct pnode	*pp;
	char		*varname;
	struct vnode	**dummy_vpp;
{
	struct vnode	*vnodes[MAX_NAMES];
	struct vnode	*vp;
	struct vnode	*childvp;
	int		result;

	bzero((char *) vnodes, sizeof vnodes);
	get_vnodes(pp, vnodes);
	vp = vnodes[0];
	if (vp == NULL) {
		if (dummy_vpp == NULL) {
			return 0;
		}
		vp = create_vnode((struct vnode *) NULL, "dummy", 0L);
		vp->v_layer = 0;
		vp->v_pnode = pp;
		*dummy_vpp = vp;
		/*
		 * Create a tmp view so that validate_directory()
		 * can follow searchlinks.
		 */
		if (create_tmp_view(vp, varname)) {
			return ENOTEMPTY;
		}
	}
	if (result = validate_directory(vp)) {
		return result;
	}
	for (childvp = CHILD_VNODE(vp); childvp;
	     childvp = next_file(childvp)) {
		if (!childvp->v_whited_out &&
		    childvp->v_layer != INVALID_LAYER) {
			return ENOTEMPTY;
		}
	}
	return 0;
}


static int
bl_physical_rmdir(bp)
	Backlink	bp;
{
	return physical_rmdir(bp->pnode);
}


static int
physical_rmdir(pp)
	struct pnode	*pp;
{
	char		dir_name[MAXNAMLEN];

	/*
	 * Remove the .tfs_ files from the directory so that
	 * the rmdir call will succeed.
	 */
	ptoname(pp, dir_name);
	remove_tfs_file(dir_name, NSE_TFS_FILE);
	remove_tfs_file(dir_name, NSE_TFS_BACK_FILE);
	remove_tfs_file(dir_name, NSE_TFS_SWAP_FILE);
	if (rmdir(dir_name) < 0) {
		return (errno);
	}
	return (0);
}


static void
remove_tfs_file(path, name)
	char		*path;
	char		*name;
{
	char		tfs_name[MAXPATHLEN];

	strcpy(tfs_name, path);
	nse_pathcat(tfs_name, name);
	(void) unlink(tfs_name);
}


/*
 * Rename the directory with vnode 'vp' to be named 'newname' in the
 * directory 'newpvp'.
 */
int
rename_directory(vp, newpvp, newname)
	struct vnode	*vp;
	struct vnode	*newpvp;
	char		*newname;
{
	struct pnode	*pp;
	struct pnode	*new_parentp;
	char		str[MAXPATHLEN];
	char		var[MAXPATHLEN];
	Nse_list	backlinks;
	Backlink	bp;
	int		result;

	if (result = validate_directory(vp)) {
		return result;
	}
	pp = vp->v_pnode;
	while (pp->p_sub_layer < MAX_SUB_LAYER) {
		pp = get_next_pnode(vp, pp);
	}
	new_parentp = newpvp->v_pnode;
	while (new_parentp->p_sub_layer < MAX_SUB_LAYER) {
		new_parentp = get_next_pnode(newpvp, new_parentp);
	}
	if (result = change_to_dir(new_parentp)) {
		return (result);
	}
	if (result = exists_in_variant(newname, var)) {
		vtovn(newpvp, str);
		nse_pathcat(str, newname);
		nse_log_message(
			"rename: %s already exists as a file in variant %s\n",
				str, var);
		return (ENOTDIR);
	}
	if ((result = change_to_dir(PARENT_PNODE(pp))) ||
	    (result = get_backlinks(pp->p_name, str, &backlinks, pp))) {
		return result;
	}
	if (nse_list_nelems(backlinks) == 0 && pp != vp->v_pnode) {
		vtovn(vp, str);
		nse_log_message(
	"warning: can't rename directory %s, need Version 2 .tfs_info\n",
			str);
		result = EACCES;
		goto error;
	}
	bp = (Backlink) nse_listelem_data(nse_list_first_elem(backlinks));
    	if (result = update_backlinks(bp, backlinks, pp, new_parentp,
					  newname)) {
		goto error;
	}
	if ((result = update_directory(PARENT_PNODE(pp), pp->p_name,
				DIR_RENAME_FILE, new_parentp, newname,
				FALSE, TRUE)) ||
	    (result = update_directory(new_parentp, newname, DIR_ADD_FILE))) {
		/*
		 * Another view may have a vnode for the destination parent
		 * directory but not one for the source parent directory.
		 * The second call to update_directory() guarantees that
		 * the destination directory gets a vnode for the new
		 * (renamed) directory.
		 */
		goto error;
	}
	if (PARENT_PNODE(pp) != new_parentp ||
	    !NSE_STREQ(pp->p_name, newname)) {
		rename_pnode(pp, new_parentp, newname);
	}
error:
	nse_list_destroy(backlinks);
	return result;
}

/*
 * See if 'name' exists in any variant
 */
int
exists_in_variant(name, var)
	char	*name;
	char	*var;
{
	Nse_list	backlinks;
	char		str[MAXPATHLEN];
	int		result;
	
	if (result = get_backlinks(".", str, &backlinks, (struct pnode *)0)) {
		return result;
	}
	result = (int) nse_list_iterate_or(backlinks, bl_verify_no_variant,
					   name, var);
	nse_list_destroy(backlinks);
	return result;
}

/*
 * Make sure that file 'name' does not exist in variant pointed
 * to by backlink 'bp'.
 */

static int
bl_verify_no_variant(bp, name, var)
	Backlink	bp;
	char		*name;
	char		*var;
{
	char		path[MAXPATHLEN];
	struct stat	statb;

	if (bp) {
		(void)strcpy(path, bp->path);
		nse_pathcat(path, name);
		if (lstat(path, &statb) == 0) {
			strcpy(var, bp->varname);
			return EEXIST;
		}
	}
	return 0;
}

/*
 * Update all tfs_info files with search links that point to 'pp'
 * using backlink information from 'bp' and 'backlinks' and physically
 * rename 'pp' to 'new_parentp'/'newname'.
 */

static int
update_backlinks(bp, backlinks, pp, new_parentp, newname)
	Backlink	bp;
	Nse_list	backlinks;
	struct pnode	*pp;
	struct pnode	*new_parentp;
	char		*newname;
{
	char		old_path[MAXPATHLEN];
	char		new_path[MAXPATHLEN];
	char		*old_pattern;
	char		*new_pattern;
	char		str[MAXPATHLEN];
	char		*cp;
	bool_t		same_dir;
	int		result;

	ptopn(pp, old_path);
	ptopn(new_parentp, new_path);
	nse_pathcat(new_path, newname);
	/*
	 * If there was no .tfs_info file, and hence no backlink information
	 * just do the rename.
	 */
	if (bp == NULL) {
		if (rename(old_path, new_path) == -1) {
			return errno;
		}
		return 0;
	}
	old_pattern = common_suffix(old_path, bp->path);
	new_pattern = &new_path[nse_find_substring(old_path, old_pattern)
				- old_path];
	sprintf(str, "%s1 %s %s", NSE_TFS_RENAME_STR, old_pattern,
		new_pattern);
	if (result = set_rename_record(".", str)) {
		return result;
	}
	same_dir = (new_parentp == PARENT_PNODE(pp));
	if (!same_dir) {
		sprintf(str, "%s2 %s", NSE_TFS_RENAME_STR, old_path);
		*rindex(str, '/') = '\0';
		cp = rindex(new_path, '/');
		*cp = '\0';
		result = set_rename_record(new_path, str);
		*cp = '/';
		if (result) {
			return result;
		}
	}
	if ((result = (int) nse_list_iterate_or(backlinks, bl_rename_dir,
						old_path, old_pattern, new_pattern,
						same_dir)) ||
	    (result = rename_shared_dir(pp, new_path, old_pattern,
					new_pattern, same_dir)) ||
	    (result = clear_all_rename_records(PARENT_PNODE(pp), new_parentp,
					       old_pattern, new_pattern, backlinks,
					       FALSE))) {
		return result;
	}
	return 0;
}

/*
 * Finish a partially-completed directory rename.  (The rename wasn't
 * completed because the machine went down in the middle of it, for
 * example.)  'path' is the name of the parent directory of the directory
 * being renamed.
 */
int
finish_rename_directory(path, rename_entry)
	char		*path;
	char		*rename_entry;
{
	char		*cp;
	char		type;
	Nse_whiteout	bl;
	Nse_whiteout	bl_rename;
	int		result = 0;

	cp = rename_entry + strlen(NSE_TFS_RENAME_STR);
	type = *cp++;
	switch (type) {
	case '1':
		result = finish_rename_dir(path, cp);
		break;
	case '2':
		cp++;
		if (result = tfsd_get_backlink(cp, &bl)) {
			return result;
		}
		bl_rename = bl_list_get_rename_elem(bl);
		if (bl_rename) {
			result = finish_rename_directory(cp, bl_rename->name);
		} else {
			result = clear_rename_record(path);
		}
		nse_dispose_whiteout(bl);
		break;
	}
	return result;
}


static int
finish_rename_dir(path, rename_entry)
	char		*path;
	char		*rename_entry;
{
	struct pnode	*pp;
	struct pnode	*new_parentp;
	char		old_path[MAXPATHLEN];
	char		new_path[MAXPATHLEN];
	char		old_pattern[MAXNAMLEN];
	char		new_pattern[MAXNAMLEN];
	char		searchlink[MAXPATHLEN];
	char		*cp;
	bool_t		same_dir;
	Nse_list	backlinks = NULL;
	int		result;
	struct stat	statb;

	sscanf(rename_entry, "%s %s", old_pattern, new_pattern);
	strcpy(old_path, path);
	nse_pathcat(old_path, rindex(old_pattern, '/') + 1);
	pp = path_to_pnode(old_path, MAX_SUB_LAYER);
	replace_substring(old_path, old_pattern, new_pattern, new_path);
	cp = rindex(new_path, '/');
	*cp = '\0';
	same_dir = NSE_STREQ(path, new_path);
	if (!same_dir) {
		new_parentp = path_to_pnode(new_path, MAX_SUB_LAYER);
	} else {
		new_parentp = PARENT_PNODE(pp);
	}
	*cp = '/';
	if (stat(old_path, &statb) == 0) {
		/*
		 * If the old name still exists, complete the rename
		 */
		if (result = get_backlinks(old_path, searchlink, &backlinks,
					   (struct pnode *) NULL)) {
			goto error;
		}
		if ((result = (int) nse_list_iterate_or(backlinks,
					bl_rename_dir, old_path, old_pattern,
					new_pattern, same_dir)) ||
		    (result = rename_shared_dir(pp, new_path, old_pattern,
						new_pattern, same_dir))) {
			goto error;
		}
		nse_list_destroy(backlinks);
		backlinks = NULL;
	}
	/*
	 * Necessary to get the backlinks again even if they've already
	 * been read, because the backlinks may have pointed to invalid
	 * directories if the tfsd crashed after some or all of the front
	 * layers were renamed but before the backlinks were renamed.
	 */
	if (result = get_backlinks(new_path, searchlink, &backlinks,
				   (struct pnode *) NULL)) {
		goto error;
	}
	result = clear_all_rename_records(PARENT_PNODE(pp), new_parentp,
					  old_pattern, new_pattern,
					  backlinks, TRUE);
error:
	if (backlinks) {
		nse_list_destroy(backlinks);
	}
	if (!same_dir) {
		free_pnode(new_parentp);
	}
	free_pnode(pp);
	return result;
}


/*
 * Whiteout the file named 'name' in all directories, except directory
 * 'exclude_pp', which have searchlinks pointing to directory 'shared_pp'.
 * Needed for tfs_pull().
 */
int
whiteout_other_dirs(shared_pp, name, exclude_pp)
	struct pnode	*shared_pp;
	char		*name;
	struct pnode	*exclude_pp;
{
	char		searchlink[MAXPATHLEN];
	Nse_list	backlinks;
	int		result;

	if ((result = change_to_dir(PARENT_PNODE(shared_pp))) ||
	    (result = get_backlinks(shared_pp->p_name, searchlink,
				    &backlinks, shared_pp))) {
		return result;
	}
	result = (int) nse_list_iterate_or(backlinks, bl_whiteout, name,
					   exclude_pp);
	nse_list_destroy(backlinks);
	return result;
}


static int
bl_whiteout(bp, name, exclude_pp)
	Backlink	bp;
	char		*name;
	struct pnode	*exclude_pp;
{
	struct vnode	*vnodes[MAX_NAMES];
	struct vnode	*vp;
	struct vnode	*childvp;
	int		result;

	if (bp->pnode == exclude_pp) {
		return 0;
	}
	bzero((char *) vnodes, sizeof vnodes);
	get_vnodes(bp->pnode, vnodes);
	vp = vnodes[0];
	if (vp == NULL) {
		return tfsd_set_whiteout(bp->path, name,
					 (struct pnode *) NULL);
	}
	if (result = validate_directory(vp)) {
		return result;
	}
	childvp = find_vnode(vp, name);
	if (childvp) {
		return update_directory(vp->v_pnode, name, DIR_REMOVE_FILE,
					TRUE, FALSE, NULL);
	}
	return 0;
}


static int
get_backlinks(dir_path, searchlink, backlinks, pp)
	char		*dir_path;
	char		*searchlink;
	Nse_list	*backlinks;
	struct pnode	*pp;
{
	Nse_whiteout	first_bl;
	Nse_whiteout	bl;
	Backlink	bp;
	char		*path;
	int		result;
	struct stat	statb;

	if (result = tfsd_get_tfs_info(dir_path, searchlink, &first_bl,
			(Nse_whiteout *) NULL, FALSE, pp, TRUE)) {
		return result;
	}
	*backlinks = bklink_list_create();
	for (bl = first_bl; bl != NULL; bl = bl->next) {
		path = index(bl->name, '/');
		pp = path_to_pnode(path, MAX_SUB_LAYER - 1);
		if (pp->p_refcnt == 1 && stat(path, &statb) == -1) {
			free_pnode(pp);
			continue;
		}
		bp = (Backlink) nse_list_add_new_elem(*backlinks);
		bp->pnode = pp;
		bp->path = NSE_STRDUP(path);
		*path = '\0';
		bp->varname = NSE_STRDUP(bl->name);
		*path = '/';
	}
	nse_dispose_whiteout(first_bl);
	return 0;
}


static int
bl_rename_dir(bp, shared_path, old_pattern, new_pattern, same_directory)
	Backlink	bp;
	char		*shared_path;
	char		*old_pattern;
	char		*new_pattern;
	bool_t		same_directory;
{
	char		path[MAXPATHLEN];
	int		result;

	sprintf(path, "%s2 %s", NSE_TFS_RENAME_STR, shared_path);
	*rindex(path, '/') = '\0';
	if ((result = change_to_dir(PARENT_PNODE(bp->pnode))) ||
	    (result = set_rename_record(".", path))) {
		return result;
	}
	strcpy(path, bp->pnode->p_name);
	if (result = recursive_directory_op(path, rename_searchlink,
					    old_pattern, new_pattern)) {
		return result;
	}
	replace_substring(bp->path, old_pattern, new_pattern, path);
	if (same_directory) {
		char		*newname;

		newname = rindex(path, '/') + 1;
		if (rename(bp->pnode->p_name, newname) < 0) {
			return errno;
		}
	} else {
		if (rename(bp->pnode->p_name, path) < 0) {
			if (errno == ENOENT) {
				char		searchlink[MAXPATHLEN];

				/*
				 * The destination parent directory doesn't
				 * exist. Since create_parent_dir may change
				 * the current directory, use the full
				 * pathname.
				 */

				replace_substring(shared_path, old_pattern,
						  new_pattern, searchlink);
				if (result = create_parent_dir(path,
							       searchlink,
							       bp->varname)) {
					return result;
				}
				if (rename(bp->path, path) < 0) {
					return errno;
				}
			} else {
				return errno;
			}
		}
	}
	return 0;
}

static int
rename_shared_dir(pp, new_path, old_pattern, new_pattern, same_directory)
	struct pnode	*pp;
	char		*new_path;
	char		*old_pattern;
	char		*new_pattern;
	bool_t		same_directory;
{
	char		path[MAXPATHLEN];
	int		result;

	if (result = change_to_dir(PARENT_PNODE(pp))) {
		return result;
	}
	strcpy(path, pp->p_name);
	if (result = recursive_directory_op(path, rename_backlinks,
					    old_pattern, new_pattern)) {
		return result;
	}
	if (same_directory) {
		char		*newname;

		newname = rindex(new_path, '/') + 1;
		if (rename(pp->p_name, newname) < 0) {
			return errno;
		}
	} else {
		if (rename(pp->p_name, new_path) < 0) {
			return errno;
		}
	}
	return 0;
}


static int
recursive_directory_op(path, func, arg1, arg2)
	char		*path;
	int		(*func)();
	char		*arg1;
	char		*arg2;
{
	DIR		*dirp;
	struct direct	*dp;
	struct stat	statb;
	char		curname[MAXNAMLEN];
	int		result;

	dirp = opendir(path);
	if (dirp == NULL) {
		nse_log_message("warning: can't read directory %s\n",
			path);
		return 0;
	}
	while (dp = readdir(dirp)) {
		if (NSE_STREQ(dp->d_name, ".") ||
		    NSE_STREQ(dp->d_name, "..") ||
		    is_tfs_special_file(dp->d_name)) {
			continue;
		}
		nse_pathcat(path, dp->d_name);
		if (lstat(path, &statb) < 0) {
			closedir(dirp);
			return errno;
		}
		if (NSE_ISDIR(&statb)) {
			strcpy(curname, dp->d_name);
			closedir(dirp);
			if (result = recursive_directory_op(path, func, arg1,
							    arg2)) {
				return result;
			}
			*rindex(path, '/') = '\0';
			dirp = opendir(path);
			if (dirp == NULL) {
				nse_log_message(
					"warning: can't read directory %s\n",
					path);
				return 0;
			}
			/*
			 * Pick up where we left off.
			 */
			while (dp = readdir(dirp)) {
				if (NSE_STREQ(curname, dp->d_name)) {
					break;
				}
			}
		} else {
			*rindex(path, '/') = '\0';
		}
	}
	closedir(dirp);
	return func(path, arg1, arg2);
}


static int
rename_searchlink(path, old_pattern, new_pattern)
	char		*path;
	char		*old_pattern;
	char		*new_pattern;
{
	char		searchlink[MAXPATHLEN];
	char		new_searchlink[MAXPATHLEN];
	Nse_whiteout	wo;
	Nse_whiteout	bl;
	int		result;

	if (result = tfsd_get_tfs_info(path, searchlink, &bl, &wo, TRUE,
				       (struct pnode *) NULL, FALSE)) {
		return result;
	}
	replace_substring(searchlink, old_pattern, new_pattern,
			  new_searchlink);
	return tfsd_set_tfs_info(path, new_searchlink, bl, wo,
				 (struct pnode *) NULL);
}


static int
rename_backlinks(path, old_pattern, new_pattern)
	char		*path;
	char		*old_pattern;
	char		*new_pattern;
{
	char		searchlink[MAXPATHLEN];
	char		new_backlink[MAXPATHLEN];
	Nse_whiteout	wo;
	Nse_whiteout	bl_first;
	Nse_whiteout	bl;
	int		result;

	if (result = tfsd_get_tfs_info(path, searchlink, &bl_first, &wo, TRUE,
				       (struct pnode *) NULL, FALSE)) {
		return (result);
	}
	for (bl = bl_first; bl != NULL; bl = bl->next) {
		replace_substring(bl->name, old_pattern, new_pattern,
				  new_backlink);
		free(bl->name);
		bl->name = NSE_STRDUP(new_backlink);
	}
	return tfsd_set_tfs_info(path, searchlink, bl_first, wo,
				 (struct pnode *) NULL);
}


/*
 * Create the parent directory of the directory named 'path'.
 * 'searchlink' is the value of the searchlink from the directory 'path'.
 */
static int
create_parent_dir(path, searchlink, varname)
	char		*path;
	char		*searchlink;
	char		*varname;
{
	char		*p;
	char		*s;
	int		result = 0;
	struct stat	statb;

	p = rindex(path, '/');
	*p = '\0';
	s = rindex(searchlink, '/');
	*s = '\0';
	if (stat(path, &statb) == -1) {
		if (result = create_parent_dir(path, searchlink, varname)) {
			goto error;
		}
		if (mkdir(path, 0777) < 0) {
			result = errno;
			goto error;
		}
		result = tfsd_set_searchlink(path, searchlink,
					     (struct pnode *) NULL);
		if (result == 0) {
			char		backlink[MAXPATHLEN];

			strcpy(backlink, varname);
			strcat(backlink, path);
			result = tfsd_set_backlink(searchlink, backlink);
		}
	} else {
		struct pnode	*pp;

		/*
		 * There may be a invalid directory vnode whose only pnode
		 * will be covered by the directory we are creating here.
		 */
		pp = path_to_pnode(path, MAX_SUB_LAYER - 1);
		result = update_directory(pp, (char *) NULL, DIR_FLUSH);
		free_pnode(pp);
	}
error:
	*p = '/';
	*s = '/';
	return result;
}


static int
clear_all_rename_records(old_parentp, new_parentp, old_pattern, new_pattern,
			 backlinks, recover)
	struct pnode	*old_parentp;
	struct pnode	*new_parentp;
	char		*old_pattern;
	char		*new_pattern;
	Nse_list	backlinks;
	bool_t		recover;
{
	int		result;

	if (result = (int) nse_list_iterate_or(backlinks,
					bl_clear_rename_record, old_pattern,
					new_pattern, recover)) {
		return result;
	}
	if (new_parentp != old_parentp) {
		if ((result = change_to_dir(new_parentp)) ||
		    (result = clear_rename_record("."))) {
			return result;
		}
	}
	if (result = change_to_dir(old_parentp)) {
		return result;
	}
	return clear_rename_record(".");
}


static int
bl_clear_rename_record(bp, old_pattern, new_pattern, recover)
	Backlink	bp;
	char		*old_pattern;
	char		*new_pattern;
	bool_t		recover;
{
	if (recover) {
		char		old_backlink[MAXPATHLEN];

		/*
		 * In the recovery case, the backlinks contain the new
		 * directory names of the front layers.
		 */
		replace_substring(bp->path, new_pattern, old_pattern,
				  old_backlink);
		*rindex(old_backlink, '/') = '\0';
		return clear_rename_record(old_backlink);
	} else {
		int		result;

		if (result = change_to_dir(PARENT_PNODE(bp->pnode))) {
			return result;
		}
		return clear_rename_record(".");
	}
}


static int
set_rename_record(path, entry)
	char		*path;
	char		*entry;
{
	char		searchlink[MAXPATHLEN];
	Nse_whiteout	wo;
	Nse_whiteout	bl;
	Nse_whiteout	bl_rename;
	int		result;

	if (result = tfsd_get_tfs_info(path, searchlink, &bl, &wo, TRUE,
				       (struct pnode *) NULL, FALSE)) {
		return result;
	}
	if (bl_rename = bl_list_get_rename_elem(bl)) {
		free(bl_rename->name);
		bl_rename->name = NSE_STRDUP(entry);
	} else {
		bl_rename = NSE_NEW(Nse_whiteout);
		bl_rename->name = NSE_STRDUP(entry);
		bl_rename->next = bl;
		bl = bl_rename;
	}
	return tfsd_set_tfs_info(path, searchlink, bl, wo,
				 (struct pnode *) NULL);
}


static int
clear_rename_record(path)
	char		*path;
{
	char		searchlink[MAXPATHLEN];
	Nse_whiteout	wo;
	Nse_whiteout	bl;
	Nse_whiteout	bl_rename;
	int		result;

	if (result = tfsd_get_tfs_info(path, searchlink, &bl, &wo, TRUE,
				       (struct pnode *) NULL, FALSE)) {
		return result;
	}
	if (bl_rename = bl_list_get_rename_elem(bl)) {
		bl_list_remove_elem(&bl, bl_rename);
	}
	return tfsd_set_tfs_info(path, searchlink, bl, wo,
				 (struct pnode *) NULL);
}


Nse_whiteout
bl_list_get_rename_elem(bl_first)
	Nse_whiteout	bl_first;
{
	Nse_whiteout	bl;
	int		rename_str_len = strlen(NSE_TFS_RENAME_STR);

	for (bl = bl_first; bl != NULL; bl = bl->next) {
		if (0 == strncmp(bl->name, NSE_TFS_RENAME_STR,
				 rename_str_len)) {
			return bl;
		}
	}
	return NULL;
}


static void
bl_list_remove_elem(bl_first, bl_to_delete)
	Nse_whiteout	*bl_first;
	Nse_whiteout	bl_to_delete;
{
	Nse_whiteout	bl;
	Nse_whiteout	bl_prev = NULL;

	for (bl = *bl_first; bl != NULL; bl = bl->next) {
		if (bl == bl_to_delete) {
			if (bl_prev) {
				bl_prev->next = bl->next;
			} else {
				*bl_first = bl->next;
			}
			bl->next = NULL;
			nse_dispose_whiteout(bl);
			return;
		}
		bl_prev = bl;
	}
}


static void
replace_substring(orig_path, old_pattern, new_pattern, new_path)
	char		*orig_path;
	char		*old_pattern;
	char		*new_pattern;
	char		*new_path;
{
	char		*cp;
	char		c;

	cp = nse_find_substring(orig_path, old_pattern);
	if (cp == NULL) {
		strcpy(new_path, orig_path);
		return;
	}
	c = *cp;
	*cp = '\0';
	strcpy(new_path, orig_path);
	*cp = c;
	strcat(new_path, new_pattern);
	cp += strlen(old_pattern);
	if (*cp != '/' && *cp != '\0') {
		/*
		 * Sanity check to ensure that 'old_pattern' exactly
		 * matched a portion of the path 'orig_path'.
		 */
		strcpy(new_path, orig_path);
	} else {
		strcat(new_path, cp);
	}
}


static char *
common_suffix(name1, name2)
	char		*name1;
	char		*name2;
{
	char		*cp1 = &name1[strlen(name1) - 1];
	char		*cp2 = &name2[strlen(name2) - 1];

	for ( ; *cp1 == *cp2; cp1--, cp2--)
		;
	return ++cp1;
}
