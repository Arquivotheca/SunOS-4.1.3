/*	@(#)tfs_lists.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <nse/list.h>



/*
 * List of clients which have mounted directories in a given view.  This
 * allows us to detect when the NSE mount daemon is trying to unmount a
 * mountpoint which is not being served by this instance of the TFS (after a
 * previous instance crashed.)
 */
typedef struct Clientlist {
	char		*cl_hostname;
	int		cl_pid;
	int		cl_refcnt;
} *Clientlist, Clientlist_rec;


/*
 * Stack of directories that have been mounted on a given mount pt (by
 * mount_tfs)
 */
typedef struct Mountstack *Mountstack, Mountstack_rec;

struct Mountstack {
	char		*name;
	Nse_list	covered;
	Mountstack	next;
};


typedef struct Mountlist {
	char		*ml_virtual;		/* Mount point name */
	char		*ml_physical;		/* Dir mounted on mount pt */
	struct vnode	*ml_root_vnode;
	Mountstack	ml_dir_stack;
	Nse_list	ml_covered;		/* Mounts covered by this one
						 * (for mount_tfs) */
	int		ml_refcnt;
} *Mountlist, Mountlist_rec;

Nse_list	mountlist_create();


/*
 * List of views/environments that the TFS is serving.  Each view has a
 * unique id, and a unique string (in the NSE, "branch/variant" is used as
 * the env name.)  Each view also has its own vnode tree, and a list of
 * mounts for the view.
 */
typedef struct Viewlist {
	char		*vl_env_name;		/* name of environment */
	unsigned	vl_environ_id;		/* number stuffed into
						 * linknodes for this env.
						 */
	char		*vl_var_name;		/* string that
						 * NSE_TFS_WILDCARD expands
						 * to (for conditional
						 * searchlinks)
						 */
	char		*vl_default_name;
	struct vnode	*vl_root_vnode;
	Nse_list	vl_mountlist;
	Nse_list	vl_clientlist;
} *Viewlist, Viewlist_rec;

Nse_list	viewlist_create();


typedef struct Filelist {
	char		*fname;
	int		layer;
	long		mtime;
} *Filelist, Filelist_rec;

Nse_list	filelist_create();


typedef struct Vnodelist {
	char		*name;
	long		nodeids[MAX_NAMES];
	int		layer;
	int		back_layer;
	bool_t		whited_out;
	bool_t		back_whited_out;
	long		mtime;
} *Vnodelist, Vnodelist_rec;

Nse_list	vnodelist_create();


typedef struct Backlink {
	struct pnode	*pnode;
	struct vnode	*dummy_vnode;
	char		*path;
	char		*varname;
} *Backlink, Backlink_rec;

Nse_list	bklink_list_create();


Nse_list	ptrlist_create();
