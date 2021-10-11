#ifndef lint
static char sccsid[] = "@(#)tfs_lists.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

#include <nse/param.h>
#include "vnode.h"
#include "tfs_lists.h"

Nse_list	viewlist_create();
static Nse_list	clientlist_create();
static bool_t	is_sub_mount_pt();
Nse_list	mountlist_create();
Nse_list	filelist_create();
Nse_list	vnodelist_create();
Nse_list	bklink_list_create();
Nse_list	ptrlist_create();
void		remove_from_list();


/*
 * Viewlist routines.
 */

static Nse_opaque
vl_create()
{
	Viewlist	vl;

	vl = NSE_NEW(Viewlist);
	vl->vl_mountlist = mountlist_create();
	vl->vl_clientlist = clientlist_create();
	return ((Nse_opaque) vl);
}


static void
vl_destroy(vl)
	Viewlist	vl;
{
	free(vl->vl_env_name);
	NSE_DISPOSE(vl->vl_var_name);
	NSE_DISPOSE(vl->vl_default_name);
	vnode_tree_release(vl->vl_root_vnode);
	nse_list_destroy(vl->vl_mountlist);
	nse_list_destroy(vl->vl_clientlist);
	free((caddr_t) vl);
}


static Nse_listops_rec viewlist_ops = {
	vl_create,
	NULL,
	vl_destroy,
	NULL,
	NULL,
};


Nse_list
viewlist_create()
{
	return nse_list_alloc(&viewlist_ops);
}


/*
 * Clientlist routines.
 */

static Nse_opaque
cl_create()
{
	Clientlist	cl;

	cl = NSE_NEW(Clientlist);
	return ((Nse_opaque) cl);
}


static void
cl_destroy(cl)
	Clientlist	cl;
{
	free(cl->cl_hostname);
	free((caddr_t) cl);
}


static void
cl_print(cl)
	Clientlist	cl;
{
	dprint(0, 0, "  Machine %s  pid %d  (refcnt %d)\n",
		cl->cl_hostname, cl->cl_pid, cl->cl_refcnt);
}


static Nse_listops_rec clientlist_ops = {
	cl_create,
	NULL,
	cl_destroy,
	cl_print,
	NULL,
};


static Nse_list
clientlist_create()
{
	return nse_list_alloc(&clientlist_ops);
}


/*
 * Mountlist routines.
 */

static Nse_opaque
ml_create()
{
	Mountlist	ml;

	ml = NSE_NEW(Mountlist);
	return ((Nse_opaque) ml);
}


static void
ml_destroy(ml)
	Mountlist	ml;
{
	if (ml == NULL) {
		return;
	}
	if (is_sub_mount_pt(ml->ml_root_vnode) || ml->ml_covered) {
		ml->ml_root_vnode->v_is_mount_pt = FALSE;
		vnode_tree_flush(ml->ml_root_vnode);
		PARENT_VNODE(ml->ml_root_vnode)->v_dir_valid = FALSE;
	} else {
		vnode_tree_release(ml->ml_root_vnode);
	}
	free(ml->ml_virtual);
	free(ml->ml_physical);
	free((caddr_t) ml);
}


static bool_t
is_sub_mount_pt(vp)
	struct vnode	*vp;
{
	for (vp = PARENT_VNODE(vp); vp; vp = PARENT_VNODE(vp)) {
		if (vp->v_is_mount_pt) {
			return TRUE;
		}
	}
	return FALSE;
}


static void
ml_print(ml)
	Mountlist	ml;
{
	dprint(0, 0, "%s mounted on %s\n", ml->ml_physical, ml->ml_virtual);
}


static Nse_listops_rec mountlist_ops = {
	ml_create,
	NULL,
	ml_destroy,
	ml_print,
	NULL,
};


Nse_list
mountlist_create()
{
	return nse_list_alloc(&mountlist_ops);
}


/*
 * Filelist routines
 */

static Filelist
fl_create()
{
	Filelist	fl;

	fl = NSE_NEW(Filelist);
	return fl;
}


static void
fl_destroy(fl)
	Filelist	fl;
{
	free(fl->fname);
	free((char *) fl);
}


static void
fl_print(fl)
	Filelist	fl;
{
	if (fl->mtime > 0) {
		printf("%s  %d  %d\n", fl->fname, fl->layer, fl->mtime);
	} else {
		printf("%s  %d\n", fl->fname, fl->layer);
	}
}


static Nse_listops_rec filelist_ops = {
	(Nse_opaquefunc) fl_create,
	NULL,
	fl_destroy,
	fl_print,
	NULL,
};


Nse_list
filelist_create()
{
	return nse_list_alloc(&filelist_ops);
}


/*
 * Vnodelist routines
 */

static Vnodelist
vnl_create()
{
	Vnodelist	vl;

	vl = NSE_NEW(Vnodelist);
	return (vl);
}


static void
vnl_destroy(vl)
	Vnodelist	vl;
{
	free(vl->name);
	free((char *) vl);
}


static void
vnl_print(vl)
	Vnodelist	vl;
{
	printf("%s  nodeids %d  %d  layer %d  back-layer %d\n",
		vl->name, vl->nodeids[0], vl->nodeids[1], vl->layer,
		vl->back_layer);
	printf("  wo %d  back-wo %d  mtime %d\n",
		vl->whited_out, vl->back_whited_out, vl->mtime);
}


static Nse_listops_rec vnodelist_ops = {
	(Nse_opaquefunc) vnl_create,
	NULL,
	vnl_destroy,
	vnl_print,
	NULL,
};


Nse_list
vnodelist_create()
{
	return nse_list_alloc(&vnodelist_ops);
}


/*
 * Backlink list routines
 */
static Nse_opaque
bl_create()
{
	return ((Nse_opaque) NSE_NEW(Backlink));
}


static void
bl_destroy(bl)
	Backlink	bl;
{
	free(bl->path);
	free(bl->varname);
	if (bl->dummy_vnode) {
		if (bl->dummy_vnode->v_environ_id) {
			delete_tmp_view(bl->dummy_vnode);
		} else {
			free_vnode(bl->dummy_vnode);
		}
	} else {
		free_pnode(bl->pnode);
	}
	free((char *) bl);
}


static Nse_listops_rec backlinks_ops = {
	bl_create,
	NULL,
	bl_destroy,
	NULL,
	NULL,
};


Nse_list
bklink_list_create()
{
	return nse_list_alloc(&backlinks_ops);
}


/*
 * Ptrlist routines.
 */

static bool_t
ptr_equal(x, y)
	Nse_opaque	x;
	Nse_opaque	y;
{
	return (x == y);
}


static Nse_listops_rec ptrlist_ops = {
	NULL,
	NULL,
	NULL,
	NULL,
	ptr_equal,
};


Nse_list
ptrlist_create()
{
	return nse_list_alloc(&ptrlist_ops);
}


void
remove_from_list(list, entry)
	Nse_list	list;
	char		*entry;
{
	Nse_listelem	elem;

	elem = nse_list_search_elem(list, ptr_equal, entry);
	if (elem) {
		nse_listelem_delete(list, elem);
	}
}
