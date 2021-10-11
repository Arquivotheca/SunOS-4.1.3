#ifndef lint
static char sccsid[] = "@(#)tfs_subr.c 1.1 92/07/30 Copyr 1988 Sun Micro";
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
#include <nse/hash.h>
#include <nse/tfs_calls.h>
#include <nse/stdio.h>

#ifdef TFSDEBUG
char		debug_pn[100];			/* for use in dbxtool */
#endif TFSDEBUG
int		num_linknodes;

/*
 * Copy-on-write routines
 */
int		promote_file();
static int	copy_to_front();
static int	copy_file_with_retry();
static int	copy_to_front_directory();
static int	promote_directory();

/*
 * Routines which operate on linknodes.
 */
struct pnode	*get_front_parent_pnode();
struct pnode	*get_pnode_at_layer();
struct pnode	*get_next_pnode();
void		set_next_pnode();
struct pnode	*release_linknode();
bool_t		has_linknode();
void		set_linknode_layers();
void		get_vnodes();

/*
 * Pathname reconstruction routines.
 */
void		ptoname();
void		ptoname_or_pn();
void		ptopn();
void		vtovn();
char		*prepend();

/*
 * Alarm routines
 */
long		get_current_time();
void		alarm_handler();

/*
 * Routines which keep track of the process's user credentials.
 */
void		change_user_id();
int		current_user_id();
int		set_user_id();
int		has_perm_to_setattr();
bool_t		has_access();
static bool_t	in_group_list();

/*
 * Wrapper routines for NSE library routines which manipulate .tfs_info
 */
int		tfsd_set_searchlink();
int		tfsd_set_whiteout();
int		tfsd_clear_whiteout();
int		tfsd_get_backlink();
int		tfsd_set_backlink();
int		tfsd_get_tfs_info();
int		tfsd_set_tfs_info();

/*
 * Miscellaneous
 */
int		modify_file();
int		do_utimes();
static int	get_server_time();
void		update_ctime();
int		utimes_as_owner();
void		set_expire_time();
int		handle_rename_sub_layers();
bool_t		expand_searchlink();
static bool_t	create_variant_searchlink();
void		fill_diropres();
static void	sattr_null();
void		panic();
void		kill_me();
bool_t		is_tfs_special_file();
void		print_warning_msg();
void		print_pnode_path();
void		tfsd_perror();
void		tfsd_err_print();
int		hash_string();
void            makefh();
FILE		*open_tfs_file();
struct vnode	*next_file();
struct vnode	*next_whiteout();


/*
 * Copy-on-write routines
 */

/*
 * Postcondition of this function:  the file with vnode 'vp' exists in a
 * writeable layer.  Directories must exist in the frontmost layer.
 */
int
promote_file(vp)
	struct vnode	*vp;
{
	switch (vp->v_pnode->p_type) {
	case PTYPE_REG:
		if (IS_WRITEABLE(vp)) {
			return (0);
		} else {
			return (copy_to_front(vp));
		}
	case PTYPE_DIR:
		if (vp->v_layer == 0) {
			return (0);
		} else if (vp->v_is_mount_pt) {
			/*
			 * This case can occur if mount_tfs has TFS-mounted
			 * a directory on a subdir of another TFS mount.
			 */
			vp->v_layer = 0;
		} else {
			return (copy_to_front_directory(vp));
		}
	}
	return (0);
}


/*
 * Copy the file with vnode 'vp' to a writeable file system.  Assumes that
 * the parent directory exists in the frontmost layer.  If this weren't
 * true, it would be necessary to recursively promote all the parent
 * directories.
 */
static int
copy_to_front(vp)
	struct vnode	*vp;
{
	struct pnode	*parentp;
	struct pnode	*newp;
	int		result;

	parentp = get_front_parent_pnode(PARENT_VNODE(vp), vp->v_layer);
	newp = alloc_pnode(parentp, vp->v_name);
	newp->p_type = PTYPE_REG;
	newp->p_refcnt = 1;
	if (copy_file_with_retry(vp, newp) == -1) {
		free_pnode(newp);
		return (errno);
	}
	if ((parentp->p_link && parentp->p_link->l_next) ||
	    environ_has_multiple_clients(vp)) {
		update_ctime(vp, FALSE);
	}
	release_pnodes(vp);
	vp->v_pnode = newp;
	vp->v_dont_flush = TRUE;
	result = update_directory(parentp, vp->v_name, DIR_ADD_FILE);
	vp->v_dont_flush = FALSE;
	newp->p_refcnt = 1;
	if (result == 0 && vp->v_mtime != 0) {
		result = do_utimes(vp, vp->v_mtime, (long) -1, FALSE);
	}
	return (result);
}


/*
 * Copy the the file with vnode 'vp' to a writeable filesystem, if the
 * user has permission to do so.  (The user can copy the file if he owns
 * the file, or if he doesn't own the file but has write access to it.)
 * The new pnode of the file is 'newp'.  If the file copy fails because we
 * don't have read access to the file, temporarily chmod the file in back
 * so that we can copy it.
 */
static int
copy_file_with_retry(vp, newp)
	struct vnode	*vp;
	struct pnode	*newp;
{
	char		src_path[MAXPATHLEN];
	char		dest_path[MAXNAMLEN];
	struct nfsfattr	attrs;
	struct stat	statb;
	int		result = 0;

	if (!vp->v_back_owner) {
		if (getattrs_of(vp, &attrs) < 0) {
			return (-1);
		}
		if (attrs.na_uid != current_user_id() &&
		    !has_access(&attrs, W_OK)) {
			errno = EACCES;
			return (-1);
		}
	}
	ptoname(newp, dest_path);
	ptopn(vp->v_pnode, src_path);
#ifdef TFSDEBUG
	dprint(tfsdebug, 3, "copy_to_front_file(%s -> %s)\n", src_path,
	       dest_path);
#endif TFSDEBUG
	if (_nse_copy_file(dest_path, src_path, 1) == NULL) {
		return (0);
	}
	if (errno != EACCES) {
		return (-1);
	}
	/*
	 * File copy failed because we don't have read access to the
	 * source file -- chmod the source file to allow read access by
	 * all and retry.
	 */
	if (LSTAT(src_path, &statb) < 0) {
		return (-1);
	}
	change_user_id((int) statb.st_uid);
	if (chmod(src_path, (int) statb.st_mode | 0444) < 0) {
		result = -1;
		goto error;
	}
	change_user_id(current_user_id());
	if (_nse_copy_file(dest_path, src_path, 1)) {
		result = -1;
	} else {
		(void) chmod(dest_path, (int) statb.st_mode);
	}
	change_user_id((int) statb.st_uid);
	(void) chmod(src_path, (int) statb.st_mode);
error:
	change_user_id(current_user_id());
	return (result);
}


/*
 * Copy the directory with vnode 'vp' to all the writeable file systems
 * in front of the frontmost instance of the directory.  We create a new
 * pnode for each new instance of the directory and set searchlinks in
 * each new instance of the directory.
 */
static int
copy_to_front_directory(vp)
	struct vnode	*vp;
{
	struct vnode	*pvp;
	struct pnode	*newp;
	struct nfsfattr	attrs;
	int		result;

	if (getattrs_of(vp, &attrs) < 0) {
		return (errno);
	}
	pvp = PARENT_VNODE(vp);
	vp->v_dont_flush = TRUE;
	result = promote_directory(pvp->v_pnode, pvp, vp, 0,
				   (int) attrs.na_mode, &newp);
	vp->v_dont_flush = FALSE;
	if (result) {
		return (result);
	}
	vp->v_pnode = newp;
	set_linknode_layers(vp);
/*
	This is necessary if we don't assume that the parent directory is
	always in the front fs

	for (childvp = CHILD_VNODE(vp); childvp != NULL;
	     childvp = NEXT_VNODE(childvp)) {
		if (childvp->v_layer != INVALID_LAYER) {
			childvp->v_layer += old_layer;
			if (childvp->v_back_layer != INVALID_LAYER) {
				childvp->v_back_layer += old_layer;
			}
		}
	}
 */
	return (0);
}


/*
 * Recursively promote the directory with vnode 'vp'.  'pvp' is the vnode
 * of the parent directory.  'parentp' is the pnode for 'pvp' at layer
 * 'layer'.  We mkdir the directory at the current layer, and create a
 * pnode 'new_pp' for the new directory.
 */
static int
promote_directory(parentp, pvp, vp, layer, mode, new_pp)
	struct pnode	*parentp;
	struct vnode	*pvp;
	struct vnode	*vp;
	int		layer;
	int		mode;
	struct pnode	**new_pp;
{
	struct pnode	*next_parentp;
	struct pnode	*next_pp;
	struct pnode	*newp;
	char		path[MAXPATHLEN];
	char		searchlink[MAXPATHLEN];
	char		backlink[MAXPATHLEN];
	char		*name;
	char		*nam;
	int		result;

	if (layer == vp->v_layer || layer > pvp->v_writeable) {
		*new_pp = vp->v_pnode;
		return (0);
	}
	next_parentp = get_next_pnode(pvp, parentp);
	if ((result = promote_directory(next_parentp, pvp, vp, layer + 1,
					mode, &next_pp)) ||
	    (result = change_to_dir(parentp))) {
		return (result);
	}
	name = vp->v_name;
	if (mkdir(name, mode) < 0 && errno != EEXIST) {
		return (errno);
	}
	ptopn(parentp, path);
	if ((nam = environ_default_name(vp->v_environ_id)) &&
	    nse_find_substring(path, nam)) {
		ptopn(next_pp, path);
		if (!create_variant_searchlink(path, vp->v_environ_id,
					       searchlink)) {
			strcpy(searchlink, path);
		}
	} else {
		ptopn(next_pp, searchlink);
		if (nam = environ_var_name(vp->v_environ_id)) {
			strcpy(backlink, nam);
			strcat(backlink, path);
			nse_pathcat(backlink, name);
			if (result = tfsd_set_backlink(searchlink, backlink)) {
				return (result);
			}
		}
	}
	if ((result = tfsd_set_searchlink(name, searchlink, parentp)) ||
	    (result = update_directory(parentp, name, DIR_ADD_FILE))) {
		return (result);
	}
	newp = alloc_pnode(parentp, name);
	newp->p_refcnt = 1;
	set_next_pnode(vp, newp, next_pp, 0);
	*new_pp = newp;
	return (0);
}


/*
 * Routines which operate on linknodes.
 */

/*
 * Gets the parent pnode at the correct sub-layer in the front file
 * system for a file at layer 'layer'.
 */
struct pnode *
get_front_parent_pnode(pvp, layer)
	struct vnode	*pvp;
	unsigned	layer;
{
	struct pnode	*pp;

	pp = get_pnode_at_layer(pvp, layer);
	if (layer <= pvp->v_writeable) {
		return (pp);
	} else {
		return (get_pnode_at_layer(pvp, SUBL_TO_LAYER(pvp, pp)));
	}
}


struct pnode *
get_pnode_at_layer(vp, layer)
	struct vnode	*vp;
	unsigned	layer;
{
	struct pnode	*pp = vp->v_pnode;
	int		i = 0;

	while (i < layer) {
		pp = get_next_pnode(vp, pp);
		if (pp == NULL) {
			nse_log_message("panic: get_pnode_at_layer(%d)",
				layer);
			panic(vp->v_pnode);
		}
		i++;
	}
	return (pp);
}


struct pnode *
get_next_pnode(vp, pp)
	struct vnode	*vp;
	struct pnode	*pp;
{
	struct linknode *lp;

	for (lp = pp->p_link; lp != NULL; lp = lp->l_next) {
		if (lp->l_vnode == vp) {
			return (lp->l_next_pnode);
		}
	}
	return (NULL);
}


void
set_next_pnode(vp, pp, nextp, layer)
	struct vnode	*vp;
	struct pnode	*pp;
	struct pnode	*nextp;
	int		layer;
{
	struct linknode	*lp;

	lp = (struct linknode *) nse_malloc(sizeof(struct linknode));
	num_linknodes++;
	lp->l_next_pnode = nextp;
	lp->l_vnode = vp;
	lp->l_layer = layer;
	lp->l_next = pp->p_link;
	pp->p_link = lp;
}


struct pnode *
release_linknode(vp, pp)
	struct vnode	*vp;
	struct pnode	*pp;
{
	struct linknode	*lp;
	struct linknode	*lp_prev = NULL;
	struct pnode	*next_pp;

	for (lp = pp->p_link; lp != NULL; lp = lp->l_next) {
		if (lp->l_vnode == vp) {
			break;
		}
		lp_prev = lp;
	}
	if (lp == NULL) {
		return (NULL);
	}
	if (lp_prev == NULL) {
		pp->p_link = lp->l_next;
	} else {
		lp_prev->l_next = lp->l_next;
	}
	next_pp = lp->l_next_pnode;
	free((char *) lp);
	num_linknodes--;
	return (next_pp);
}


bool_t
has_linknode(vp, pp)
	struct vnode	*vp;
	struct pnode	*pp;
{
	struct linknode *lp;

	for (lp = pp->p_link; lp != NULL; lp = lp->l_next) {
		if (lp->l_vnode == vp) {
			return (TRUE);
		}
	}
	return (FALSE);
}


void
set_linknode_layers(vp)
	struct vnode	*vp;
{
	struct pnode	*pp = vp->v_pnode;
	struct linknode	*lp;
	int		layer = 0;

	while (pp != NULL) {
		for (lp = pp->p_link; lp != NULL; lp = lp->l_next) {
			if (lp->l_vnode == vp) {
				break;
			}
		}
		if (lp == NULL) {
			break;
		}
		lp->l_layer = layer++;
		pp = lp->l_next_pnode;
	}
}


/*
 * Returns all vnodes with 'pp' as their frontmost pnode.  'vnodes' should
 * be a bzero'd array with MAX_NAMES entries.
 */
void
get_vnodes(pp, vnodes)
	struct pnode	*pp;
	struct vnode	**vnodes;
{
	struct linknode	*lp;
	int		i = 0;

	for (lp = pp->p_link; lp != NULL; lp = lp->l_next) {
		if (lp->l_layer == 0) {
			if (i == MAX_NAMES) {
				/*
				 * This is bad.  Panic now before the stack
				 * is corrupted.
				 */
				nse_log_message("panic: get_vnodes");
				panic(pp);
			}
			vnodes[i] = lp->l_vnode;
			i++;
		}
	}
}


/*
 * Pathname reconstruction routines.
 */

/*
 * Return the name of pnode 'pp', and fchdir to the parent directory, so
 * that the relative name can be used in system calls.
 */
void
ptoname(pp, name)
	struct pnode	*pp;
	char		*name;
{
	if (change_to_dir(PARENT_PNODE(pp)) == 0) {
		strcpy(name, pp->p_name);
	} else {
		ptopn(pp, name);
	}
}


/*
 * Construct a pathname for a pnode.  If a parent directory has an fd
 * open, fchdir to it and return the partial name; otherwise construct the
 * full pathname of the file.  If a parent directory has a long name, then
 * fchdir to that directory (this saves on NFS remote lookup calls, since
 * the kernel directory name lookup cache only caches names of a small
 * length.)
 */
void
ptoname_or_pn(pp, pn)
	struct pnode	*pp;
	char		*pn;
{
	char            tpn[MAXPATHLEN];
	char		*pnptr = &tpn[MAXPATHLEN - 1];
	int             pathsize = 0;

	if ((pp->p_fd != 0) && (pp->p_type == PTYPE_DIR) &&
	    (change_to_dir(pp) == 0)) {
		strcpy(pn, ".");
		return;
	}
	*pnptr = '\0';
	do {
		pnptr = prepend(pp->p_name, pnptr, &pathsize);
		pp = PARENT_PNODE(pp);
		if (pp->p_fd != 0 || strlen(pp->p_name) >= 20) {
			if (change_to_dir(pp) == 0) {
				break;
			}
		}
		pnptr = prepend("/", pnptr, &pathsize);
	} while (pp->p_name[0] != '/');
	strcpy(pn, pnptr);
}


/*
 * Convert pnode to full pathname.
 */
void
ptopn(pp, pn)
	struct pnode	*pp;
	char		*pn;
{
	ntree_pathname((struct tnode *) pp, pn);
}


/*
 * Convert vnode to virtual pathname.
 */
void
vtovn(vp, pn)
	struct vnode	*vp;
	char		*pn;
{
	ntree_pathname((struct tnode *) vp, pn);
}


/*
 * prepend() tacks a directory name onto the front of a pathname.
 * The maximum length of the pathname is MAXPATHLEN.
 */
char *
prepend(dirname, pathname, pathsize)
	char		*dirname;
	char		*pathname;
	int		*pathsize;
{
	int		i;		/* directory name size counter */

	for (i = 0; *dirname != '\0'; i++, dirname++)
		continue;
	if ((*pathsize += i) < MAXPATHLEN)
		while (i-- > 0)
			*--pathname = *--dirname;
	return (pathname);
}


/*
 * Alarm routines, for timing out the attribute & write caches
 */

#define ALARM_DELAY	5		/* # of secs between alarms */

static struct timeval current_time;
bool_t		alarm_went_off;
extern bool_t	servicing_req;


long
get_current_time(precise)
	bool_t		precise;
{
	struct timeval	time;

	if (current_time.tv_sec == 0) {
#ifdef TFSDEBUG
		dprint(tfsdebug, 3, "Setting alarm\n");
#endif TFSDEBUG
		(void) gettimeofday(&current_time, (struct timezone *) NULL);
		alarm(ALARM_DELAY);
	} else if (precise) {
		(void) gettimeofday(&time, (struct timezone *) NULL);
		return (time.tv_sec);
	}
	return (current_time.tv_sec);
}


/* ARGSUSED */
void
alarm_handler(sig, code, scp)
	int		sig;
	int		code;
	struct sigcontext *scp;
{
	bool_t		more_attrs;
	bool_t		more_writes;

	if (servicing_req) {
		/*
		 * Handle this alarm after the current request is serviced.
		 * (This avoids race conditions between the attr and write
		 * cache timeout routines and other routines which modify
		 * the caches.)
		 */
		alarm_went_off = TRUE;
		return;
	}
	(void) gettimeofday(&current_time, (struct timezone *) NULL);
#ifdef TFSDEBUG
	dprint(tfsdebug, 3, "ALARM = %u\n", current_time.tv_sec);
#endif TFSDEBUG
	more_attrs = timeout_attr_cache(current_time.tv_sec);
	more_writes = timeout_wcache(current_time.tv_sec);
	if ((more_attrs) || (more_writes)) {
		alarm(ALARM_DELAY);
	} else {
		current_time.tv_sec = 0;
	}
	alarm_went_off = FALSE;
}


int
init_alarm()
{
	(void) signal(SIGALRM, alarm_handler);
}


/*
 * Routines which keep track of the process's user credentials.
 */

/*
 * Declare this struct rather than use 'struct ucred' because the fields
 * of ucred are shorts in 4.0.  This will cause problems if the uid or
 * gid is ever negative, because setreuid & setregid barf if given a
 * negative number other than -1.  (An example negative uid/gid is -2
 * for 'nobody'.)
 */
static struct {
	u_short		cr_uid;			/* effective user id */
	u_short		cr_gid;			/* effective group id */
	int		cr_groups[NGROUPS];	/* groups, 0 terminated */
} user_cred;

static int	num_groups;		/* Length of group list */


/*
 * Change user id
 */
void
change_user_id(uid)
	int		uid;
{
	setreuid(-1, 0);
	setreuid(-1, uid);
}


int
current_user_id()
{
	return (user_cred.cr_uid);
}


/*
 * Set uid, gid, and gids to auth params that came in over the wire.  Call
 * setreuid only if the set of auth params is different from the current set
 * of user credentials.  NOTE: any other routine which does a setreuid()
 * must make sure to setreuid() back to the effective user id, otherwise
 * chaos could result!
 */
int
set_user_id(aup)
	struct authunix_parms *aup;
{
#ifdef TFSDEBUG
	dprint(tfsdebug, 3, "(uid %d gid %d)  ", aup->aup_uid, aup->aup_gid);
#endif TFSDEBUG
	/*
	 * If the user changes groups on us, we won't catch it here.
	 */
	if (aup->aup_uid == user_cred.cr_uid) {
		return (0);
	}

	if (setreuid(-1, 0)) {
		return (-1);
	}
	user_cred.cr_gid = aup->aup_gid;
	if (setregid(-1, (int) user_cred.cr_gid)) {
		return (-1);
	}
	num_groups = aup->aup_len;
	BCOPY((caddr_t) aup->aup_gids, (caddr_t) user_cred.cr_groups,
	      num_groups * sizeof (int));
	if (setgroups((int) num_groups, (int *) aup->aup_gids)) {
		return (-1);
	}

	user_cred.cr_uid = aup->aup_uid;
	if (setreuid(-1, (int) user_cred.cr_uid)) {
		return (-1);
	}
	return (0);
}


/*
 * This routine is called by tfs_setattr() to ensure that the user has
 * the correct access to file 'vp' to set the attributes specified in
 * 'sa'.  This routine is necessary because the file may have to be
 * promoted to the front file system before the attributes are set, and
 * we don't want to copy the file if the setattr is going to fail.
 */
int
has_perm_to_setattr(vp, sa)
	struct vnode	*vp;
	struct nfssattr *sa;
{
	struct nfsfattr	attrs;

	if (getattrs_of(vp, &attrs) < 0) {
		return (errno);
	}
	/*
	 * Change file mode or time.  Must be owner of file.
	 */
	if ((sa->sa_mode != (u_short) -1 && sa->sa_mode != -1) ||
	    sa->sa_atime.tv_sec != -1 || sa->sa_mtime.tv_sec != -1) {
		if (user_cred.cr_uid != attrs.na_uid) {
			return (EPERM);
		}
	}
	/*
	 * Change file/group ownership.  Must be su.
	 */
	if (sa->sa_uid != -1 &&
	    sa->sa_uid != attrs.na_uid &&
	    user_cred.cr_uid != 0) {
		return (EPERM);
	}
	if (sa->sa_gid != -1 &&
	    sa->sa_gid != attrs.na_gid &&
	    !in_group_list((int) sa->sa_gid) &&
	    user_cred.cr_uid != 0) {
		return (EPERM);
	}
	/*
	 * Truncate file.  Must have write access to the file and the
	 * node cannot be a directory.
	 */
	if (sa->sa_size != -1) {
		if (vp->v_pnode->p_type == PTYPE_DIR ||
		    !has_access(&attrs, W_OK)) {
			return (EACCES);
		}
	}
	return (0);
}


/*
 * Check for accessibility of the file with attributes 'attrs', given
 * the credentials of the user.  'mode' is one of R_OK, W_OK, X_OK, or
 * an inclusive OR of them.  This function returns TRUE if the file can
 * be accessed by the user.  This function duplicates the functionality
 * of the access() system call.  We can't use the access() system call
 * directly because access() would use the tfsd's real user id and real
 * group id, and we want to test for access using the effective uid & gid.
 */
bool_t
has_access(attrs, mode)
	struct nfsfattr	*attrs;
	int		mode;
{
	/*
	 * If you're the super-user, you always get access.
	 */
	if (user_cred.cr_uid == 0) {
		return (TRUE);
	}
	/*
	 * Access check is based on only one of owner, group, public.
	 * If not owner, then check group.  If not a member of the group, then
	 * check public access.
	 */
	mode <<= 6;
	if (user_cred.cr_uid != attrs->na_uid) {
		mode >>= 3;
		if (!in_group_list((int) attrs->na_gid)) {
			mode >>= 3;
		}
	}
	return ((attrs->na_mode & mode) == mode);
}


static bool_t
in_group_list(gid)
	int		gid;
{
	int		i;

	if (gid == user_cred.cr_gid) {
		return (TRUE);
	}
	for (i = 0; i < num_groups; i++) {
		if (gid == user_cred.cr_groups[i]) {
			return (TRUE);
		}
	}
	return (FALSE);
}


/*
 * Wrapper routines for NSE library routines which manipulate .tfs_info
 * files.  These wrapper routines allow the tfsd to print an error message
 * before returning a cryptic error to the client (e.g. the tfsd may
 * return EPERM when it doesn't have permission to open .tfs_info, when the
 * client is doing an operation for which the error EPERM doesn't make sense.)
 */
int
tfsd_set_searchlink(directory, name, pp)
	char		*directory;
	char		*name;
	struct pnode	*pp;
{
	Nse_err		err;

	if (err = nse_set_searchlink(directory, name)) {
		nse_log_message("warning: can't set searchlink: ");
		tfsd_err_print(err);
		if (pp) {
			print_pnode_path(pp);
		}
		return (err->code);
	}
	return (0);
}


int
tfsd_set_whiteout(directory, name, pp)
	char		*directory;
	char		*name;
	struct pnode	*pp;
{
	Nse_err		err;

	if (err = nse_set_whiteout(directory, name)) {
		nse_log_message("warning: can't set whiteout: ");
		tfsd_err_print(err);
		if (pp) {
			print_pnode_path(pp);
		}
		return (err->code);
	}
	return (0);
}


int
tfsd_clear_whiteout(directory, name, pp)
	char		*directory;
	char		*name;
	struct pnode	*pp;
{
	Nse_err		err;

	if (err = nse_clear_whiteout(directory, name)) {
		nse_log_message("warning: can't clear whiteout: ");
		tfsd_err_print(err);
		if (pp) {
			print_pnode_path(pp);
		}
		return (err->code);
	}
	return (0);
}


int
tfsd_get_backlink(directory, blp)
	char		*directory;
	Nse_whiteout	*blp;
{
	Nse_err		err;

	if (err = nse_get_backlink(directory, blp)) {
		nse_log_message("warning: can't get backlink: ");
		tfsd_err_print(err);
		return (err->code);
	}
	return (0);
}


int
tfsd_set_backlink(directory, name)
	char		*directory;
	char		*name;
{
	Nse_err		err;

	if (err = nse_set_backlink(directory, name)) {
		nse_log_message("warning: can't set backlink: ");
		tfsd_err_print(err);
		return (err->code);
	}
	return (0);
}


int
tfsd_get_tfs_info(directory, name, blp, wop, will_write_later, pp,
		  ignore_enoent)
	char		*directory;
	char		*name;
	Nse_whiteout	*blp;
	Nse_whiteout	*wop;
	bool_t		will_write_later;
	struct pnode	*pp;
	bool_t		ignore_enoent;	/* ignore ENOENT error? */
{
	Nse_err		err;

	if (err = nse_get_tfs_info(directory, name, blp, wop,
				   will_write_later)) {
		if (ignore_enoent && err->code == ENOENT) {
			return (0);
		}
		nse_log_message("warning: nse_get_tfs_info: ");
		tfsd_err_print(err);
		if (pp) {
			print_pnode_path(pp);
		}
		return (err->code);
	}
	return (0);
}


int
tfsd_set_tfs_info(directory, name, bl_first, wo, pp)
	char		*directory;
	char		*name;
	Nse_whiteout	bl_first;
	Nse_whiteout	wo;
	struct pnode	*pp;
{
	Nse_err		err;

	if (err = nse_set_tfs_info(directory, name, bl_first, wo)) {
		nse_log_message("warning: nse_set_tfs_info: ");
		tfsd_err_print(err);
		if (pp) {
			print_pnode_path(pp);
		}
		return (err->code);
	}
	return (0);
}


/*
 * Miscellaneous
 */

/*
 * Modify the file with pnode 'pp' with the function 'ffunc' if the file
 * has a file descriptor, or with 'func' if the file doesn't have an fd.
 */
/* VARARGS4 */
int
modify_file(pp, func, ffunc, arg1, arg2)
	struct pnode	*pp;
	int		(* func)();
	int		(* ffunc)();
	int		arg1;
	int		arg2;
{
	char		name[MAXNAMLEN];

	if (pp->p_fd != 0) {
		if (ffunc(pp->p_fd, arg1, arg2) < 0) {
			return (errno);
		}
	} else {
		ptoname(pp, name);
		if (func(name, arg1, arg2) < 0) {
			return (errno);
		}
	}
	return (0);
}


/*
 * Set the modify and access times of the file with vnode 'vp' to the
 * indicated values.  If 'vp' is in a writeable layer, directly modify
 * the file; otherwise set the mtime in .tfs_backfiles and set the atime
 * of the read-only file.
 */
int
do_utimes(vp, mtime, atime, set_to_now)
	struct vnode	*vp;
	long		mtime;
	long		atime;
	bool_t		set_to_now;
{
	struct pnode	*parentp;
	struct timeval	tvp[2];
	struct nfsfattr	attrs;
	char		*fname;
	char		name[MAXNAMLEN];
	int		result;

	if (vp->v_is_mount_pt) {
		parentp = vp->v_pnode;
		fname = ".";
	} else {
		parentp = get_front_parent_pnode(PARENT_VNODE(vp),
						 vp->v_layer);
		fname = vp->v_name;
	}
	if (IS_WRITEABLE(vp)) {
		if (result = change_to_dir(parentp)) {
			return (result);
		}
		if (set_to_now) {
			if (utimes(fname, (struct timeval *)0) < 0) {
				return (errno);
			}
		} else {
			tvp[0].tv_sec = atime;
			tvp[0].tv_usec = 0;
			tvp[1].tv_sec = mtime;
			tvp[1].tv_usec = 0;
			if (utimes(fname, tvp) < 0) {
				return (errno);
			}
		}
		return (0);
	}
	if (set_to_now) {
		if (result = get_server_time(parentp, &mtime)) {
			return (result);
		}
		atime = mtime;
	}
	if (mtime != -1) {
		if ((result = update_directory(parentp, fname, DIR_UTIME_FILE,
					       mtime)) ||
		    (result = change_backlist_mtime(parentp, fname, mtime))) {
			return (result);
		}
	}
	if (atime != -1) {
		/*
		 * Set the access time of the file in the read-only layer
		 */
		if (get_realattrs_of(vp, &attrs) < 0) {
			return (errno);
		}
		if (attrs.na_atime.tv_sec == atime) {
			return (0);
		}
		tvp[0].tv_sec = atime;
		tvp[0].tv_usec = 0;
		tvp[1].tv_sec = -1;
		tvp[1].tv_usec = 0;
		ptoname(vp->v_pnode, name);
		result = utimes_as_owner(name, tvp, (int) attrs.na_uid);
	}
	return (result);
}

/*
 * Create a dummy file in 'dir' whose sole purpose is to get the
 * current 'mtime' on the NFS server on which it resides. This is
 * used by utimes(x, NULL) to get the right value of 'mtime' for a
 * file 'x' which is in the read-only layers of 'dir'.
 */
static int
get_server_time(dir, mtime)
	struct pnode	*dir;
	long		*mtime;
{
	int		fd;
	struct stat	stbuf;
	int		result = 0;

	*mtime = -1;
	if (result = change_to_dir(dir)) {
		return result;
	}
	if (utimes(NSE_TFS_FILE, (struct timeval *) NULL) == -1 ||
	    stat(NSE_TFS_FILE, &stbuf) == -1) {
		if (errno != ENOENT) {
			return (errno);
		}
		fd = open(NSE_TFS_UTIMES_FILE, O_RDWR|O_CREAT, 0666);
		if (fd == -1) {
			return (errno);
		}
		if (fstat(fd, &stbuf) == -1) {
			result = errno;
		}
		if (close(fd) == -1 && result == 0) {
			result = errno;
		}
		if (unlink(NSE_TFS_UTIMES_FILE) == -1 && result == 0) {
			result = errno;
		}
	}
	*mtime = stbuf.st_mtime;
	return (result);
}

/*
 * Update the ctime of the file with vnode 'vp' by setting its atime and
 * mtime to their current values.  This is necessary so that the kernel VFS
 * will know when a directory has had an entry removed.  (The kernel VFS
 * needs to flush a directory from the directory name lookup cache (DNLC)
 * when the directory's ctime changes, because it is possible for someone
 * on another machine to remove or rename a file/dir that was in the local
 * machine's DNLC; we don't want the file to be reachable through the DNLC
 * any longer.)  The tfsd needs to explicitly update the ctime of the
 * directory in the front layer when a file is removed from a back layer,
 * or a file is removed by creating a whiteout entry.
 *
 * This routine is also called when a file in a back layer is
 * copy-on-writed, in two cases: 1) there is more than one view on the file
 * (e.g. the file can be seen in both a var-sun3 and a var-sun4 activation
 * at the same time), or 2) the environ containing the file is being
 * accessed by more than one client machine.  In such a case, it is
 * possible for a client on one machine to copy-on-write a file for which
 * there is another kernel TFS vnode.  Updating the ctime of the file
 * in the back layer is the only way to alert the client kernel that the
 * other TFS vnode's real vnode has changed.  NOTE: this unfortunately updates
 * the ctime of a file in a read-only layer.
 *
 * It is not necessary to call this routine when a directory is
 * copy-on-writed, because the kernel VFS knows that a directory is
 * copy-on-writed the first time it is read, and so will flush the real
 * vnode of a non-writeable directory the first time it is read.
 */
void
update_ctime(vp, avoid_dup_updates)
	struct vnode	*vp;
	bool_t		avoid_dup_updates;
{
	struct nfsfattr	attrs;
	struct timeval	tvp[2];
	char		name[MAXNAMLEN];
	int		result;

	if (avoid_dup_updates) {
		if (vp->v_pnode->p_updated) {
			return;
		}
		vp->v_pnode->p_updated = TRUE;
	}
	if (get_realattrs_of(vp, &attrs) < 0) {
		print_warning_msg(vp->v_pnode, "update_ctime: getattrs");
		return;
	}
	tvp[0].tv_sec = attrs.na_atime.tv_sec;
	tvp[0].tv_usec = 0;
	tvp[1].tv_sec = attrs.na_mtime.tv_sec;
	tvp[1].tv_usec = 0;
	if (vp->v_pnode->p_type == PTYPE_DIR &&
	    change_to_dir(vp->v_pnode) == 0) {
		strcpy(name, ".");
	} else {
		ptoname(vp->v_pnode, name);
	}
	result = utimes_as_owner(name, tvp, (int) attrs.na_uid);
	if (result) {
		errno = result;
		print_warning_msg(vp->v_pnode, "update_ctime: utimes");
	}
}


int
utimes_as_owner(name, tvp, owner)
	char		*name;
	struct timeval	tvp[2];
	int		owner;
{
	int		result = 0;

	if (owner != current_user_id()) {
		change_user_id(owner);
	}
	if (utimes(name, tvp) < 0) {
		result = errno;
	}
	if (owner != current_user_id()) {
		change_user_id(current_user_id());
	}
	return (result);
}


/*
 * Determine when the cached mtime for pnode 'pp' will expire and need to
 * be checked.  Use the algorithm used by the NFS -- base the expire time
 * on the time since the directory last changed, but set limits on the
 * maximum and minimum expire times.
 */
void
set_expire_time(pp)
	struct pnode	*pp;
{
	long		delta;

	pp->p_expire = get_current_time(FALSE);
	delta = (pp->p_expire - pp->p_mtime) >> 4;
	if (delta < MIN_DIR_EXPIRE) {
		delta = MIN_DIR_EXPIRE;
	} else if (delta > MAX_DIR_EXPIRE) {
		delta = MAX_DIR_EXPIRE;
	}
	pp->p_expire += delta;
}


/*
 * This routine is called by tfs_rename() and tfs_link(), which create
 * new names for objects at specified sub-layers.  This routine handles
 * two complications that arise:
 * 1) If the source file occurred at a layer further back than the dest
 *    file, then the original dest file has to be removed.  (This ensures
 *    that we don't create a file underneath an existing file/whiteout entry.)
 * 2) If the source file at a layer further forward than the dest file,
 *    but dest is at a greater sub-layer, then the source file will have
 *    to be pushed to the sub-layer of dest.  In this case, 'dest_sub_layer'
 *    is set to the sub-layer that the file has to be pushed to.
 */
int
handle_rename_sub_layers(dest_vp, src_layer, dest_sub_layer)
	struct vnode	*dest_vp;
	unsigned	src_layer;
	int		*dest_sub_layer;
{
	struct pnode	*src_pp;
	struct pnode	*dest_pp;
	int		result;

	dest_pp = get_front_parent_pnode(PARENT_VNODE(dest_vp),
					 dest_vp->v_layer);
	if (dest_vp->v_layer < src_layer) {
		if (result = clear_file(dest_pp, dest_vp)) {
			return (result);
		}
	} else if (dest_vp->v_layer > src_layer) {
		src_pp = get_front_parent_pnode(PARENT_VNODE(dest_vp),
						src_layer);
		if (src_pp->p_sub_layer < dest_pp->p_sub_layer) {
			*dest_sub_layer = dest_pp->p_sub_layer;
		}
	}
	return (0);
}


/*
 * Expand the variant searchlink 'path' into 'expanded_path'.  'cp' is a
 * pointer to the position of the wildcard string in 'path', and 'environ_id'
 * is the environment ID which should be used to expand the searchlink.
 * 'use_var_name' indicates whether to use the variant name or the default
 * name of the environment.
 */
bool_t
expand_searchlink(path, environ_id, cp, expanded_path, use_var_name)
	char		*path;
	unsigned	environ_id;
	char		*cp;
	char		*expanded_path;
	bool_t		use_var_name;
{
	char		*tailp;
	char		oldc;

	tailp = cp + strlen(NSE_TFS_WILDCARD);
	if (*tailp != '/') {
		return (FALSE);
	}
	oldc = *cp;
	*cp = '\0';
	strcpy(expanded_path, path);
	*cp = oldc;
	if (use_var_name && (cp = environ_var_name(environ_id))) {
		strcat(expanded_path, cp);
	} else if (cp = environ_default_name(environ_id)) {
		strcat(expanded_path, cp);
	} else {
		return (FALSE);
	}
	strcat(expanded_path, tailp);
	return (TRUE);
}


/*
 * Convert the path 'path' into a variant searchlink.
 */
static bool_t
create_variant_searchlink(path, environ_id, unexpanded_path)
	char		*path;
	unsigned	environ_id;
	char		*unexpanded_path;
{
	char		*cp = NULL;
	char		*tailp;
	char		oldc;
	char		*varname;
	char		*default_name;

	varname = environ_var_name(environ_id);
	if (varname) {
		cp = nse_find_substring(path, varname);
	}
	if (cp) {
		tailp = cp + strlen(varname);
	} else {
		default_name = environ_default_name(environ_id);
		cp = nse_find_substring(path, default_name);
		if (cp == NULL) {
			return (FALSE);
		}
		tailp = cp + strlen(default_name);
	}
	if (*tailp != '/') {
		return (FALSE);
	}
	oldc = *cp;
	*cp = '\0';
	strcpy(unexpanded_path, path);
	*cp = oldc;
	strcat(unexpanded_path, NSE_TFS_WILDCARD);
	strcat(unexpanded_path, tailp);
	return (TRUE);
}


/*
 * Fill tfsdiropres struct to send back to kernel.  Used by TFS version 2
 * requests.  (The client kernel contains the kernel VFS.)
 */
void
fill_diropres(vp, dr)
	struct vnode	*vp;
	struct tfsdiropres *dr;
{
	static char	pn[MAXPATHLEN];

	makefh(&dr->dr_fh, vp);
	dr->dr_nodeid = vp->v_fhid;
	ptopn(vp->v_pnode, pn);
	if (vp->v_pnode->p_type == PTYPE_DIR) {
		/*
		 * A mountpoint vnode may have a pnode which is actually
		 * a symlink, if a branch mount has been made of a relocated
		 * variant.  In this case, we want to return the pathname
		 * of the real directory, not the symlink, to the kernel.
		 * Note that the pnodes at mountpoints are the only ones
		 * which can be assigned a p_type which does not match
		 * the type of the real file.
		 */
		if (vp->v_is_mount_pt) {
			struct stat	statb;

			while (LSTAT(pn, &statb) == 0 &&
			       (statb.st_mode & S_IFMT) == S_IFLNK) {
				int		count;

				count = readlink(pn, pn, MAXPATHLEN);
				if (count < 0) {
					break;
				}
				pn[count] = '\0';
			}
		}
		dr->dr_writeable = (vp->v_layer == 0);
	} else {
		dr->dr_writeable = IS_WRITEABLE(vp);
	}
	dr->dr_path = pn;
	dr->dr_pathlen = strlen(pn);
	sattr_null(&dr->dr_sattrs);
	if (!IS_WRITEABLE(vp)) {
		if (vp->v_back_owner) {
			dr->dr_sattrs.sa_uid = current_user_id();
		}
		if (vp->v_mtime != 0) {
			dr->dr_sattrs.sa_mtime.tv_sec = vp->v_mtime;
			dr->dr_sattrs.sa_mtime.tv_usec = 0;
		}
	}
}


/*
 * Set sattr structure to a null value.
 * XXX machine dependent?
 */
static void
sattr_null(sap)
	struct nfssattr	*sap;
{
	int		n;
	char		*cp;

	n = sizeof(struct nfssattr);
	cp = (char *) sap;
	while (n--) {
		*cp++ = -1;
	}
}


/*
 * Chdir to /tmp and dump core there.  Called from various places when
 * something strange happens.
 */
void
panic(pp)
	struct pnode	*pp;
{
	if (pp != NULL) {
		print_pnode_path(pp);
	}
	kill_me();
}


void
kill_me()
{
	/*
	 * Core dump will be successful only if (uid == euid && gid == egid)
	 */
	(void) setreuid(-1, 0);
	(void) setregid(-1, getgid());
	(void) chdir("/tmp");
	abort();
}


bool_t
is_tfs_special_file(name)
	char		*name;
{
	return (strncmp(name, NSE_TFS_FILE_PREFIX, NSE_TFS_FILE_PREFIX_LEN)
		== 0);
}


void
print_warning_msg(pp, str)
	struct pnode	*pp;
	char		*str;
{
	nse_log_message("warning: ");
	tfsd_perror(str);
	print_pnode_path(pp);
}


void
print_pnode_path(pp)
	struct pnode	*pp;
{
	char		pn[MAXPATHLEN];

	ptopn(pp, pn);
	fprintf(stderr, " (dir %s)\n", pn);
	fprintf(stdout, " (dir %s)\n", pn);
}


extern int	sys_nerr;
extern char	*sys_errlist[];

void
tfsd_perror(str)
	char		*str;
{
	char		string[MAXPATHLEN];

	strcpy(string, str);
	strcat(string, ": ");
	if (errno < sys_nerr && errno > 0) {
		strcat(string, sys_errlist[errno]);
	} else {
		str = string + strlen(string);
		sprintf(str, "Unknown error #%d", errno);
	}
	fprintf(stderr, string);
	fprintf(stdout, string);
}


/*
 * Print the error string to stdout and stderr without the leading
 * 'tfsd: '.
 */
void
tfsd_err_print(err)
	Nse_err		err;
{
	char		*str;

	if (err) {
		str = index(err->str, ' ');
		str++;
		fprintf(stderr, "%s\n", str);
		fprintf(stdout, "%s\n", str);
	}
}


int
hash_string(compname)
	char	       *compname;
{
	int		sum;

	for (sum = 0; *compname; sum += *compname++)
		;
	return (sum & (HASH_TABLE_SIZE - 1));
}

/*
 * get the fhandle from a given vnode.
 */
void
makefh(fh, vp)
	fhandle_t	*fh;
	struct vnode	*vp;
{
	extern time_t	tfsd_timestamp;
	
	BZERO((caddr_t) fh, NFS_FHSIZE);
	TFS_FH(fh)->fh_id = vp->v_fhid;
	TFS_FH(fh)->fh_parent_id = PARENT_VNODE(vp)->v_fhid;
	TFS_FH(fh)->fh_timestamp = tfsd_timestamp;
}

FILE *
open_tfs_file(path, mode, errp)
	char		*path;
	char		*mode;
	Nse_err		*errp;
{
	FILE		*file;
	Nse_err		err;

	if (err = nse_fopen(path, mode, &file)) {
		if (errp) {
			*errp = err;
		} else {
			nse_log_err_print(err);
		}
		return (NULL);
	}
	if (file != NULL) {
		setbuffer(file, read_result_buffer, NFS_MAXDATA);
	}
	return (file);
}


/*
 * Returns the next vnode in the directory (which is not a whiteout vnode.)
 */
struct vnode *
next_file(vp)
	struct vnode	*vp;
{
	do {
		vp = NEXT_VNODE(vp);
	} while (vp != NULL && vp->v_whited_out);
	return (vp);
}


/*
 * Returns the next whiteout vnode in the directory.
 */
struct vnode *
next_whiteout(vp)
	struct vnode	*vp;
{
	do {
		vp = NEXT_VNODE(vp);
	} while (vp != NULL && (!vp->v_whited_out || !IS_WRITEABLE(vp)));
	return (vp);
}
