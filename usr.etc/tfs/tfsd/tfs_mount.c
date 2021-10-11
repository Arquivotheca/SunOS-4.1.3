#ifndef lint
static char sccsid[] = "@(#)tfs_mount.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <nse/types.h>
#include "headers.h"
#include "tfs.h"
#include "vnode.h"
#include "subr.h"
#include "tfs_lists.h"
#include <nse/param.h>
#include <nse/util.h>
#include <nse/list.h>
#include <sys/socket.h>
#include <nse/tfs_calls.h>
#include <nse/rev_tree.h>


#define TFS_MOUNT_NAME	"tfs_mount"	/* Environment name given to the
					 * tfsd by mount_tfs */

static Nse_list	viewlist;


extern SVCXPRT	*tfsd_xprt;

bool_t		svc_finished;

int		tfs_mount_1();
int		tfs_mount_2();
int		tfs_unmount_1();
int		tfs_unmount_2();
int		tfs_getname();
int		tfs_flush();
int		create_tmp_view();
void		delete_tmp_view();
static int	create_view();
static char	*branch_to_var_name();
#ifdef TFSDEBUG
void		print_vnode_trees();
#endif TFSDEBUG

static void	add_client_entry();
static bool_t	remove_client_entry();

static Nse_list	get_covered_mounts();
static void	restore_covered_mounts();

char		*environ_var_name();
char		*environ_default_name();
bool_t		environ_has_multiple_clients();
static Viewlist	get_environ_entry();
static int	select_environ_id();
static void	release_environ_id();

static bool_t	vl_env_name_eq();
static bool_t	vl_environ_id_eq();
static void	vl_flush_all_vnodes();

static bool_t	ml_path_eq();
static bool_t	ml_physical_eq();
static void	ml_flush_all_vnodes();

static bool_t	cl_equal();
static bool_t	cl_name_neq();


/*
 * Add a new mount, version 1
 */
int
tfs_mount_1(vp, ma, mr)
	struct vnode	*vp;
	Tfs_old_mount_args ma;
	Tfs_mount_res	mr;
{
	Tfs_mount_args_rec new_ma;
	int		result;

	bzero((char *)&new_ma, sizeof new_ma);
	new_ma.environ = ma->environ;
	new_ma.tfs_mount_pt = ma->tfs_mount_pt;
	new_ma.directory = ma->directory;
	new_ma.hostname = ma->hostname;
	new_ma.pid = ma->pid;
	if (ma->writeable_layers == 0) {
		new_ma.writeable_layers = 1;
		new_ma.back_read_only = FALSE;
		new_ma.conditional_view = NSE_STRDUP("");
		new_ma.default_view = NSE_STRDUP("");
	} else {
		new_ma.writeable_layers = ma->writeable_layers;
		new_ma.back_read_only = TRUE;
		new_ma.conditional_view = branch_to_var_name(ma->environ);
		if (NSE_STREQ(new_ma.conditional_view, NSE_RT_SOURCE_NAME)) {
			new_ma.default_view = new_ma.conditional_view;
			new_ma.conditional_view = NSE_STRDUP("");
		} else {
			new_ma.default_view = NSE_STRDUP(NSE_RT_SOURCE_NAME);
		}
	}
	result = tfs_mount_2(vp, &new_ma, mr);
	free(new_ma.conditional_view);
	free(new_ma.default_view);
	return result;
}


/*
 * Add a new mount, version 2
 */
/* ARGSUSED */
int
tfs_mount_2(vp, ma, mr)
	struct vnode	*vp;
	Tfs_mount_args	ma;
	Tfs_mount_res	mr;
{
	Viewlist	vl;
	Mountlist	ml = NULL;
	Mountstack	sp;
	int		result = 0;
	struct stat	statb;

	if (viewlist == NULL) {
		viewlist = viewlist_create();
	}
	if (current_user_id() != 0) {
		result = EPERM;
		goto error;
	}
	if ((ma->directory[0] != '/') ||
	    (ma->tfs_mount_pt[0] != '/' || ma->tfs_mount_pt[1] == '\0')) {
		result = EINVAL;
		goto error;
	}
	if (stat(ma->directory, &statb) == -1) {
		result = NSE_TFS_NO_DIRECTORY;
		goto error;
	}
	vl = (Viewlist) nse_list_search(viewlist, vl_env_name_eq,
					ma->environ);
	if (vl) {
		ml = (Mountlist) nse_list_search(vl->vl_mountlist,
						 ml_path_eq, ma->tfs_mount_pt);
	} else {
		if (result = create_view(ma, &vl)) {
			goto error;
		}
		ml = NULL;
	}
	if (ml == NULL) {
		/*
		 * Don't allow the client to mount the same physical
		 * directory onto a different mount point.
		 */
		if (nse_list_search(vl->vl_mountlist, ml_physical_eq,
				    ma->directory)) {
			result = EBUSY;
			goto error;
		}
		ml = (Mountlist) nse_list_add_new_elem(vl->vl_mountlist);
		ml->ml_virtual = NSE_STRDUP(ma->tfs_mount_pt);
		ml->ml_physical = NSE_STRDUP(ma->directory);
		if (NSE_STREQ(ma->environ, TFS_MOUNT_NAME)) {
			ml->ml_covered = get_covered_mounts(vl->vl_mountlist,
							    ma->tfs_mount_pt);
		}
		ml->ml_root_vnode = mntpntovp(vl->vl_root_vnode,
					      ma->tfs_mount_pt, ma->directory);
		ml->ml_refcnt = 1;
	} else {
		/*
		 * Don't allow the client to mount a directory onto
		 * a mount point that already has a different directory
		 * mounted on it, unless mount_tfs is the client.
		 */
		if (!NSE_STREQ(ma->directory, ml->ml_physical)) {
			if (!NSE_STREQ(ma->environ, TFS_MOUNT_NAME)) {
				result = EBUSY;
				goto error;
			}
			sp = NSE_NEW(Mountstack);
			sp->name = ml->ml_physical;
			sp->covered = ml->ml_covered;
			sp->next = ml->ml_dir_stack;
			ml->ml_dir_stack = sp;
			ml->ml_physical = NSE_STRDUP(ma->directory);
			ml->ml_covered = get_covered_mounts(vl->vl_mountlist,
							    ma->tfs_mount_pt);
			ml->ml_root_vnode = mntpntovp(vl->vl_root_vnode,
						      ma->tfs_mount_pt,
						      ma->directory);
		}
		ml->ml_refcnt++;
	}
	mr->port = tfsd_xprt->xp_port;
	makefh(&mr->fh, ml->ml_root_vnode);
	mr->pid = getpid();
	add_client_entry(vl->vl_clientlist, ma->hostname, ma->pid);
error:
	if (nse_list_nelems(viewlist) == 0) {
		svc_finished = 1;
	}
	return (result);
}


/*
 * Remove a mount, version 1
 */
int
tfs_unmount_1(vp, ua, status)
	struct vnode	*vp;
	Tfs_old_mount_args ua;
	enum nfsstat	*status;
{
	Tfs_unmount_args_rec new_ua;

	new_ua.environ = ua->environ;
	new_ua.tfs_mount_pt = ua->tfs_mount_pt;
	new_ua.hostname = ua->hostname;
	new_ua.pid = ua->pid;
	return tfs_unmount_2(vp, &new_ua, status);
}


/*
 * Remove a mount, version 2
 */
/* ARGSUSED */
int
tfs_unmount_2(vp, ua, status)
	struct vnode	*vp;
	Tfs_unmount_args ua;
	enum nfsstat	*status;
{
	Viewlist	vl;
	Mountlist	ml;
	Mountstack	sp;
	int		result = 0;

	if (current_user_id() != 0) {
		result = EPERM;
		goto error;
	}
	if (viewlist == NULL) {
		return (EINVAL);
	}
	vl = (Viewlist) nse_list_search(viewlist, vl_env_name_eq, ua->environ);
	if (vl == NULL) {
		result = EINVAL;
		goto error;
	}
	ml = (Mountlist) nse_list_search(vl->vl_mountlist, ml_path_eq,
					 ua->tfs_mount_pt);
	if (ml == NULL ||
	    !remove_client_entry(vl->vl_clientlist, ua->hostname, ua->pid)) {
		result = EINVAL;
		goto error;
	}
	ml->ml_refcnt--;
	if (ml->ml_covered) {
		restore_covered_mounts(ml->ml_covered, vl->vl_mountlist,
				       vl->vl_root_vnode);
	}
	if (ml->ml_refcnt == 0) {
		/*
		 * Free mountlist entry, and viewlist entry also, if this
		 * was the last active mount for this view.
		 */
		remove_from_list(vl->vl_mountlist, (char *) ml);
		if (nse_list_nelems(vl->vl_mountlist) == 0) {
			release_environ_id(vl->vl_environ_id);
			remove_from_list(viewlist, (char *) vl);
		}
	} else if (ml->ml_dir_stack) {
		free(ml->ml_physical);
		sp = ml->ml_dir_stack;
		ml->ml_dir_stack = sp->next;
		ml->ml_physical = sp->name;
		ml->ml_covered = sp->covered;
		free(sp);
		ml->ml_root_vnode = mntpntovp(vl->vl_root_vnode,
					      ml->ml_virtual, ml->ml_physical);
	} else {
		ml->ml_covered = NULL;
	}
error:
	if (nse_list_nelems(viewlist) == 0) {
		svc_finished = 1;
	}
	return (result);
}


/* ARGSUSED */
int
tfs_getname(vp, dummy, gr)
	struct vnode	*vp;
	int		dummy;
	Tfs_getname_res	gr;
{
	static char	path[MAXPATHLEN];

	ptopn(vp->v_pnode, path);
	gr->path = path;
	return (0);
}


/*
 * Flush all cached information for the branch specified by 'branch'.
 * Releases all pnodes, then creates new pnodes for the mountpoint vnodes.
 */
/* ARGSUSED */
int
tfs_flush(dummy, branch)
	int		dummy;
	char		**branch;
{
	if (viewlist) {
		nse_list_iterate(viewlist, vl_flush_all_vnodes, *branch);
	}
	return (0);
}


int
create_tmp_view(vp, varname)
	struct vnode	*vp;
	char		*varname;
{
	Viewlist	vl;

	vl = (Viewlist) nse_list_add_new_elem(viewlist);
	vl->vl_environ_id = select_environ_id();
	if (vl->vl_environ_id == -1) {
		remove_from_list(viewlist, (char *) vl);
		return NSE_TFS_TOO_MANY_ENVS;
	}
	vl->vl_env_name = NSE_STRDUP("tmp");
	vl->vl_var_name = NSE_STRDUP(varname);
	vl->vl_root_vnode = vp;
	vp->v_environ_id = vl->vl_environ_id;
	return 0;
}


void
delete_tmp_view(vp)
	struct vnode	*vp;
{
	Viewlist	vl;

	vl = get_environ_entry(vp->v_environ_id);
	release_environ_id(vl->vl_environ_id);
	remove_from_list(viewlist, (char *) vl);
}


static int
create_view(ma, vlp)
	Tfs_mount_args	ma;
	Viewlist	*vlp;
{
	Viewlist	vl;

	vl = (Viewlist) nse_list_add_new_elem(viewlist);
	vl->vl_environ_id = select_environ_id();
	if (vl->vl_environ_id == -1) {
		remove_from_list(viewlist, (char *) vl);
		return NSE_TFS_TOO_MANY_ENVS;
	}
	vl->vl_env_name = NSE_STRDUP(ma->environ);
	if (ma->conditional_view[0] != '\0') {
		vl->vl_var_name = NSE_STRDUP(ma->conditional_view);
	}
	if (ma->default_view[0] != '\0') {
		vl->vl_default_name = NSE_STRDUP(ma->default_view);
	}
	vl->vl_root_vnode = create_vnode((struct vnode *) NULL, "/",
					 (long) 0);
	vl->vl_root_vnode->v_environ_id = vl->vl_environ_id;
	if (!ma->back_read_only) {
		vl->vl_root_vnode->v_back_owner = FALSE;
	}
	vl->vl_root_vnode->v_writeable = ma->writeable_layers - 1;
	*vlp = vl;
	return 0;
}


static char *
branch_to_var_name(fullbrname)
	char		*fullbrname;
{
	char		tmpname[MAXPATHLEN];
	char		variant[MAXNAMLEN];

	nse_parse_branch_name(fullbrname, (char *) NULL, variant);
	if (NSE_STREQ(variant, NSE_RT_SOURCE_NAME)) {
		strcpy(tmpname, variant);
	} else {
		strcpy(tmpname, NSE_RT_VAR_PREFIX);
		strcat(tmpname, variant);
	}
	return (NSE_STRDUP(tmpname));
}


#ifdef TFSDEBUG
static void
pr_vnode_tree(vl)
	Viewlist	vl;
{
	dprint(0, 0, "Branch: %s\n", vl->vl_env_name);
	nse_list_print(vl->vl_mountlist);
	nse_list_print(vl->vl_clientlist);
	vnode_tree_print(vl->vl_root_vnode);
}


void
print_vnode_trees()
{
	if (viewlist) {
		nse_list_iterate(viewlist, pr_vnode_tree);
	}
}
#endif TFSDEBUG


/*
 * Routines to add/remove client entries from clientlists.
 */
static void
add_client_entry(clientlist, hostname, pid)
	Nse_list	clientlist;
	char		*hostname;
	int		pid;
{
	Clientlist	cl;

	cl = (Clientlist) nse_list_search(clientlist, cl_equal, hostname, pid);
	if (cl == NULL) {
		cl = (Clientlist) nse_list_add_new_elem(clientlist);
		cl->cl_hostname = NSE_STRDUP(hostname);
		cl->cl_pid = pid;
		cl->cl_refcnt = 1;
	} else {
		cl->cl_refcnt++;
	}
}


static bool_t
remove_client_entry(clientlist, hostname, pid)
	Nse_list	clientlist;
	char		*hostname;
	int		pid;
{
	Nse_listelem	cl_elem;
	Clientlist	cl;

	cl_elem = nse_list_search_elem(clientlist, cl_equal, hostname, pid);
	if (cl_elem == NULL) {
		return (FALSE);
	}
	cl = (Clientlist) nse_listelem_data(cl_elem);
	cl->cl_refcnt--;
	if (cl->cl_refcnt == 0) {
		nse_listelem_delete(clientlist, cl_elem);
	}
	return (TRUE);
}


/*
 * Removes mountpoints from 'mlist' that will be covered by a mount on
 * 'path'.  A list of the removed mountpoints is returned.
 */
static Nse_list
get_covered_mounts(mlist, path)
	Nse_list	mlist;
	char		*path;
{
	Mountlist	ml;
	Nse_listelem	elem;
	Nse_listelem	end;
	Nse_listelem	next;
	int		pathlen = strlen(path);
	Nse_list	covered_list = NULL;

	elem = nse_list_first_elem(mlist);
	end = nse_list_end(mlist);
	while (elem != end) {
		next = nse_listelem_next(elem);
		ml = (Mountlist) nse_listelem_data(elem);
		if (ml && strncmp(ml->ml_virtual, path, pathlen) == 0 &&
		    ml->ml_virtual[pathlen] == '/') {
			if (!covered_list) {
				covered_list = mountlist_create();
			}
			nse_list_add_new_data(covered_list, ml);
			nse_listelem_remove(mlist, elem);
			nse_listelem_destroy_wrapper(elem, (Nse_opaque) NULL);
			ml->ml_root_vnode->v_is_mount_pt = FALSE;
		}
		elem = next;
	}
	return covered_list;
}


/*
 * Put covered mountpoints back into 'mlist'.
 */
static void
restore_covered_mounts(covered_list, mlist, root_vnode)
	Nse_list	covered_list;
	Nse_list	mlist;
	struct vnode	*root_vnode;
{
	Mountlist	ml;
	Nse_listelem	elem;
	Nse_listelem	end;

	NSE_LIST_ITERATE(covered_list, Mountlist, ml, elem, end) {
		ml->ml_root_vnode = mntpntovp(root_vnode, ml->ml_virtual,
					      ml->ml_physical);
		nse_list_add_new_data(mlist, ml);
		nse_listelem_set_data(elem, (Nse_opaque) NULL);
	}
	nse_list_destroy(covered_list);
}


/*
 * Given the ID of the environment, returns the variant directory string
 * (to be used in following conditional searchlinks.)
 */
char *
environ_var_name(environ_id)
	unsigned	environ_id;
{
	Viewlist	vl;

	vl = get_environ_entry(environ_id);
	if (vl == NULL) {
		return (NULL);
	} else {
		return (vl->vl_var_name);
	}
}


char *
environ_default_name(environ_id)
	unsigned	environ_id;
{
	Viewlist	vl;

	vl = get_environ_entry(environ_id);
	if (vl == NULL) {
		return (NULL);
	} else {
		return (vl->vl_default_name);
	}
}


bool_t
environ_has_multiple_clients(vp)
	struct vnode	*vp;
{
	Viewlist	vl;
	Clientlist	cl;

	vl = get_environ_entry(vp->v_environ_id);
	cl = (Clientlist) nse_listelem_data(
				nse_list_first_elem(vl->vl_clientlist));
	if (nse_list_search(vl->vl_clientlist, cl_name_neq, cl->cl_hostname)) {
		return TRUE;
	} else {
		return FALSE;
	}
}


/*
 * Used by get_environ_entry() so that we don't have to search viewlist every
 * time.  Set to 0 by release_environ_id().
 */
static int	last_environ_id;

/*
 * Returns viewlist entry for the environment with id 'environ_id'.
 */
static Viewlist
get_environ_entry(environ_id)
	unsigned	environ_id;
{
	static Viewlist	vl;

	if (environ_id != last_environ_id) {
		vl = (Viewlist) nse_list_search(viewlist, vl_environ_id_eq,
						environ_id);
		if (vl == NULL) {
			last_environ_id = 0;
		} else {
			last_environ_id = environ_id;
		}
	}
	return (vl);
}


/*
 * Routines to manage environment id's, to ensure that no more than
 * MAX_ENVIRON id's are handed out.  (Restricted by the width of the
 * v_environ_id field in the vnode.)
 */

static int	next_environ_id;
static long	used_environs;


static int
select_environ_id()
{
	long		mask;
	int		i;

	for (i = 0; i <= MAX_ENVIRON; i++) {
		next_environ_id++;
		if (next_environ_id > MAX_ENVIRON) {
			next_environ_id = 1;
		}
		mask = 1 << next_environ_id;
		if ((used_environs & mask) == 0) {
			used_environs |= mask;
			return (next_environ_id);
		}
	}
	return (-1);
}


static void
release_environ_id(id)
	unsigned	id;
{
	used_environs &= ~(1 << id);
	next_environ_id = id;
	last_environ_id = 0;
}


/*
 * Routines for viewlists.
 */

static bool_t
vl_env_name_eq(vl, name)
	Viewlist	vl;
	char		*name;
{
	return (NSE_STREQ(vl->vl_env_name, name));
}


static bool_t
vl_environ_id_eq(vl, num)
	Viewlist	vl;
	int		num;
{
	return (vl->vl_environ_id == num);
}


static void
vl_flush_all_vnodes(vl, name)
	Viewlist	vl;
	char		*name;
{
	if (strncmp(vl->vl_env_name, name, strlen(name)) == 0) {
		nse_list_iterate(vl->vl_mountlist, ml_flush_all_vnodes);
	}
}


/*
 * Routines for mountlists.
 */

static bool_t
ml_path_eq(ml, name)
	Mountlist	ml;
	char		*name;
{
	return (NSE_STREQ(ml->ml_virtual, name));
}


static bool_t
ml_physical_eq(ml, name)
	Mountlist	ml;
	char		*name;
{
	return (NSE_STREQ(ml->ml_physical, name));
}


static void
ml_flush_all_vnodes(ml)
	Mountlist	ml;
{
	(void) update_directory(ml->ml_root_vnode->v_pnode, (char *) NULL,
				DIR_FLUSH);
}


/*
 * Routines for clientlists.
 */

static bool_t
cl_equal(cl, hostname, pid)
	Clientlist	cl;
	char		*hostname;
	int		pid;
{
	return (NSE_STREQ(cl->cl_hostname, hostname) && (cl->cl_pid == pid));
}


static bool_t
cl_name_neq(cl, hostname)
	Clientlist	cl;
	char		*hostname;
{
	return !NSE_STREQ(cl->cl_hostname, hostname);
}
