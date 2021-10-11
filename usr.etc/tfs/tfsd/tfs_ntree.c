#ifndef lint
static char sccsid[] = "@(#)tfs_ntree.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <nse/param.h>
#include <nse/util.h>
#include "headers.h"
#include "tfs.h"
#include "vnode.h"
#include "subr.h"

/*
 * Routines to operate on pnode and vnode trees.  Included here are
 * routines which allocate and free pnodes & vnodes, routines which
 * traverse the trees, and routines which implement the general N-trees
 * which are used to implement pnode and vnode trees.  (These trees are
 * called N-trees because each node can have any number (N) of children.)
 */

#ifdef TFSDEBUG
static int      vnode_count;
static int	dir_vnode_count;	/* XXX */
static int      pnode_count;
static int	swapped_count;
static int      fd_count;
#endif TFSDEBUG

struct pnode	*root_pnode;
long		next_fhandle;
int		num_vnodes;
int		num_pnodes;

/*
 * Routines which traverse pnode and vnode trees.
 */
void		vnode_tree_release();
void		vnode_tree_flush();
static void	vnode_tree_flush_aux();
#ifdef TFSDEBUG
void		vnode_tree_print();
void		pnode_tree_print();
#endif TFSDEBUG

/*
 * Vnode routines
 */
struct vnode	*create_vnode();
void            free_vnode();
#ifdef TFSDEBUG
static void     print_vnode();
#endif TFSDEBUG

/*
 * Pnode routines
 */
struct pnode	*alloc_pnode();
void		rename_pnode();
void		release_back_pnodes();
void		release_pnodes();
void		free_pnode();
static void	free_pnode_aux();
#ifdef TFSDEBUG
static void	print_pnode();
#endif TFSDEBUG

/*
 * Routines which operate on general directory trees.
 */
static void	ntree_insert();
static void	ntree_remove();
static void	ntree_preorder();
static void	ntree_preorder_aux();
static void	ntree_postorder();
static void	ntree_postorder_aux();
struct tnode	*ntree_find_child();
void		ntree_pathname();


/*
 * Routines which traverse vnode and pnode trees.
 */

/*
 * Release all vnodes contained in a mount point
 */
void
vnode_tree_release(root_vp)
	struct vnode   *root_vp;
{
	ntree_postorder((struct tnode *) root_vp, free_vnode);
}


/*
 * Release all pnodes and invalidate all cached information
 */
void
vnode_tree_flush(root_vp)
	struct vnode	*root_vp;
{
	ntree_postorder((struct tnode *) root_vp, vnode_tree_flush_aux);
}


static void
vnode_tree_flush_aux(vp)
	struct vnode	*vp;
{
	if (vp->v_pnode != NULL) {
		if (vp->v_pnode->p_swapped || vp->v_is_mount_pt) {
			/*
			 * If the vnode has had its contents swapped out, save
			 * the frontmost pnode.  Note that it isn't necessary
			 * to save the pnodes of all the writeable layers of
			 * this directory for tfs_update(), since this
			 * directory will be revalidated the next time it is
			 * visited.  Also have to save frontmost pnode for
			 * a mountpoint.
			 */
			release_back_pnodes(vp);
			if (vp->v_pnode->p_fd > 0) {
				close_fd(vp->v_pnode);
			}
		} else {
			release_pnodes(vp);
		}
	}
	vp->v_dir_valid = FALSE;
}


#ifdef TFSDEBUG
/*
 * Print all vnodes in tree.
 */
void
vnode_tree_print(root_vp)
	struct vnode   *root_vp;
{
	vnode_count = 0;
	dir_vnode_count = 0;
	pnode_count = 0;
	fd_count = 0;
	dprint(0, 0, "VNODE TREE\n");
	ntree_preorder((struct tnode *) root_vp, print_vnode);
	dprint(0, 0, "%d vnodes, %d dir vnodes, %d pnodes, %d open files\n",
	       vnode_count, dir_vnode_count, pnode_count, fd_count);
}


/*
 * Print all pnodes in tree.
 */
void
pnode_tree_print()
{
	if (root_pnode != NULL) {
		pnode_count = 0;
		swapped_count = 0;
		dprint(0, 0, "%-27s", "PNODE TREE");
		dprint(0, 0, "refcnt  sub-layer\n");
		ntree_preorder((struct tnode *) root_pnode, print_pnode);
		dprint(0, 0, "%d pnodes  %d swapped\n", pnode_count,
			swapped_count);
	}
}
#endif TFSDEBUG


/*
 * Vnode routines
 */

/*
 * create_vnode() is called when a vnode needs to be created.  It
 * constructs a vnode and assigns an fhandle for the file.
 */
struct vnode *
create_vnode(pvp, pc, nodeid)
	struct vnode   *pvp;		/* ptr to parent vnode */
	char           *pc;		/* pathname component */
	long		nodeid;
{
	struct vnode	*vp;

	vp = (struct vnode *) nse_malloc(sizeof(struct vnode));
	BZERO((caddr_t) vp, sizeof(struct vnode));
	if (nodeid == 0) {
		vp->v_fhid = ++next_fhandle;
	} else {
		vp->v_fhid = nodeid;
	}
	vp->v_layer = INVALID_LAYER;
	vp->v_back_layer = INVALID_LAYER;
	ntree_insert((struct tnode *) pvp, (struct tnode *) vp, pc);
	if (pvp != NULL) {
		vp->v_environ_id = pvp->v_environ_id;
		vp->v_writeable = pvp->v_writeable;
		vp->v_back_owner = pvp->v_back_owner;
	} else {
		vp->v_back_owner = TRUE;
	}
	fhandle_insert(vp);
	comp_insert((struct tnode *) vp);
	num_vnodes++;
	return (vp);
}


/*
 * Free a vnode -- remove it from the vnode tree and fhandle hash table
 * and free up its storage.
 */
void
free_vnode(vp)
	struct vnode   *vp;
{
#ifdef TFSDEBUG
	dprint(tfsdebug, 3, "  free_vnode: removing vnode for fh %d\n",
	       vp->v_fhid);
#endif TFSDEBUG
	fhandle_remove(vp);
	comp_remove((struct tnode *) vp);
	ntree_remove((struct tnode *) vp);
	if (vp->v_pnode != NULL) {
		release_pnodes(vp);
	}
	free((caddr_t) vp);
	num_vnodes--;
}


#ifdef TFSDEBUG
static void
print_vnode(vp)
	struct vnode	*vp;
{
	struct vnode	*pvp;
	struct pnode	*pp;

	vnode_count++;
	if (CHILD_VNODE(vp) != NULL) {
		dir_vnode_count++;
	}
	pvp = PARENT_VNODE(vp);
	if (pvp != NULL) {
		dprint(0, 0, "%-15s ", pvp->v_name);
	} else {
		dprint(0, 0, "%-15s ", "(no parent)");
	}
	dprint(0, 0, "%-15s ", vp->v_name);
	dprint(0, 0, "%5d", vp->v_fhid);
	if (vp->v_layer == INVALID_LAYER) {
		dprint(0, 0, " INVAL");
	} else {
		dprint(0, 0, "%5d", vp->v_layer);
	}
	if (vp->v_whited_out) {
		dprint(0, 0, " (wo)");
	}
	switch (vp->v_back_layer) {
	case INVALID_LAYER:
		dprint(0, 0, " INVAL ");
		break;
	case UNKNOWN_LAYER:
		dprint(0, 0, " UNKNOWN");
		break;
	default:
		dprint(0, 0, "%6d", vp->v_back_layer);
		break;
	}
	if (vp->v_back_whited_out) {
		dprint(0, 0, " (wo in back)");
	}
	if (vp->v_dir_valid) {
		dprint(0, 0, " (dir valid)");
	}
	if (vp->v_mtime != 0) {
		dprint(0, 0, " mtime %d", vp->v_mtime);
	}
	if (vp->v_pnode != NULL) {
		pp = vp->v_pnode;
		if (pp->p_type == PTYPE_DIR) {
			while (pp != NULL) {
				pnode_count++;
				dprint(0, 0, " *");
				pp = get_next_pnode(vp, pp);
			}
		} else {
			pnode_count++;
			dprint(0, 0, " *");
		}
	}
	if ((vp->v_pnode != NULL) && (vp->v_pnode->p_fd > 0)) {
		fd_count++;
		dprint(0, 0, "%5d", vp->v_pnode->p_fd);
	}
	dprint(0, 0, "\n");
}
#endif TFSDEBUG


/*
 * Pnode routines.
 */

struct pnode *
alloc_pnode(parentp, name)
	struct pnode	*parentp;
	char		*name;
{
	struct pnode	*pp;

	pp = (struct pnode *) nse_malloc(sizeof(struct pnode));
	BZERO((caddr_t) pp, sizeof(struct pnode));
	ntree_insert((struct tnode *) parentp, (struct tnode *) pp, name);
	if (parentp != NULL) {
		pp->p_sub_layer = parentp->p_sub_layer;
	}
	num_pnodes++;
	return (pp);
}


/*
 * Change pnode 'pp's position in the pnode tree.
 */
void
rename_pnode(pp, newparent, newname)
	struct pnode	*pp;
	struct pnode	*newparent;
	char		*newname;
{
	struct pnode	*childp;

	childp = CHILD_PNODE(pp);
	CHILD_PNODE(pp) = NULL;
	ntree_remove((struct tnode *) pp);
	pp->p_tnode.t_sibling = NULL;
	ntree_insert((struct tnode *) newparent, (struct tnode *) pp,
		     newname);
	CHILD_PNODE(pp) = childp;
}


/*
 * Release all but the frontmost pnode for 'vp'.
 */
void
release_back_pnodes(vp)
	struct vnode	*vp;
{
	struct pnode	*pp;

	pp = vp->v_pnode;
	pp->p_refcnt++;
	release_pnodes(vp);
	vp->v_pnode = pp;
}


void
release_pnodes(vp)
	struct vnode	*vp;
{
	struct pnode	*pp;
	struct pnode	*pp_next = NULL;

	pp = vp->v_pnode;
	if (pp->p_type == PTYPE_DIR) {
		while (pp != NULL) {
			pp_next = release_linknode(vp, pp);
			free_pnode(pp);
			pp = pp_next;
		}
	} else {
		free_pnode(pp);
	}
	vp->v_pnode = NULL;
}


/*
 * Release the storage used by pnode 'pp'.
 */
void
free_pnode(pp)
	struct pnode	*pp;
{
	pp->p_refcnt--;
	free_pnode_aux(pp);
}


static void
free_pnode_aux(pp)
	struct pnode	*pp;
{
	struct pnode	*parentp;

	if (pp->p_refcnt == 0 && CHILD_PNODE(pp) == NULL) {
		if (pp->p_in_queue) {
			swqueue_dequeue(pp);
		}
		if (pp->p_fd > 0) {
			close_fd(pp);
		}
		flush_cached_attrs(pp);
		parentp = PARENT_PNODE(pp);
		ntree_remove((struct tnode *) pp);
		free((caddr_t) pp);
		num_pnodes--;
		free_pnode_aux(parentp);
	}
}


#ifdef TFSDEBUG
static void
print_pnode(pp)
	struct pnode	*pp;
{
	struct pnode	*parentp;
	struct linknode	*lp;

	pnode_count++;
	parentp = PARENT_PNODE(pp);
	if (parentp != NULL) {
		dprint(0, 0, "%-15s ", parentp->p_name);
	} else {
		dprint(0, 0, "%-15s ", "(no parent)");
	}
	dprint(0, 0, "%-15s ", pp->p_name);
	dprint(0, 0, "%5u ", pp->p_refcnt);
	dprint(0, 0, "%d ", pp->p_sub_layer);
	if (pp->p_swapped) {
		swapped_count++;
		dprint(0, 0, "(swapped) ");
	}
	if (pp->p_in_queue) {
		dprint(0, 0, "(in queue) ");
	}
	if (pp->p_fd > 0) {
		dprint(0, 0, "(fd %d) ", pp->p_fd);
	}
	switch (pp->p_type) {
	case PTYPE_DIR:
		if (pp->p_link == NULL) {
			break;
		}
		dprint(0, 0, "vnodes:  ");
		for (lp = pp->p_link; lp != NULL; lp = lp->l_next) {
			dprint(0, 0, "%d(%d)  ", lp->l_vnode->v_fhid,
				lp->l_layer);
		}
		break;
	case PTYPE_REG:
		dprint(0, 0, "reg");
		break;
	case PTYPE_LINK:
		dprint(0, 0, "link");
		break;
	case PTYPE_OTHER:
		dprint(0, 0, "other");
		break;
	default:
		break;
	}
	dprint(0, 0, "\n");
}
#endif TFSDEBUG


/*
 * Routines which operate on general directory trees.
 */

/*
 * Insert node 'tp' into a tree.  'parent' is the parent node of the new node.
 */
static void
ntree_insert(parent, tp, name)
	struct tnode	*parent;
	struct tnode	*tp;
	char		*name;
{
	tp->t_name = NSE_STRDUP(name);
	tp->t_parent = parent;
	if (parent != NULL) {
		if (parent->t_child != NULL) {
			tp->t_sibling = parent->t_child;
		}
		parent->t_child = tp;
	}
}


/*
 * Remove a node from a tree.
 */
static void
ntree_remove(tp)
	struct tnode	*tp;
{
	struct tnode   *parent;
	struct tnode   *tt;
	struct tnode   *ttprev;

#ifdef TFSDEBUG
	if (tp->t_child != NULL) {
		char		pn[MAXPATHLEN];

		ntree_pathname(tp, pn);
		nse_log_message(
	"warning: ntree_remove: node for file %s has children! (%s)\n",
			pn, tp->t_child->t_name);
	}
#endif TFSDEBUG
	parent = tp->t_parent;
	if (parent == NULL) {
		goto done;
	}
	tt = parent->t_child;
	ttprev = NULL;
	while (tt != NULL) {
		if (tt == tp) {
			if (ttprev == NULL) {
				parent->t_child = tt->t_sibling;
			} else {
				ttprev->t_sibling = tt->t_sibling;
			}
			break;
		}
		ttprev = tt;
		tt = tt->t_sibling;
	}
#ifdef TFSDEBUG
	if (tt != tp) {
		nse_log_message(
	"warning: ntree_remove: node for file %s not found under parent!\n", 
			tp->t_name);
	}
#endif TFSDEBUG
done:
	free(tp->t_name);
}


/*
 * Traverse a tree pre-order, applying the function 'process_node' to each
 * node visited.  Note that ntree_preorder_aux is needed because 'tp' may
 * have siblings.
 */
static void
ntree_preorder(tp, process_node)
	struct tnode	*tp;
	void		(*process_node) ();
{
	process_node(tp);
	if (tp->t_child != NULL) {
		ntree_preorder_aux(tp->t_child, process_node);
	}
}


static void
ntree_preorder_aux(tp, process_node)
	struct tnode	*tp;
	void		(*process_node) ();
{
	process_node(tp);
	if (tp->t_child != NULL) {
		ntree_preorder_aux(tp->t_child, process_node);
	}
	if (tp->t_sibling != NULL) {
		ntree_preorder_aux(tp->t_sibling, process_node);
	}
}


/*
 * Traverse a tree post-order, applying the function 'process_node' to each
 * node visited.  'tp' is the root of the tree (or subtree.)  Note that
 * ntree_postorder_aux is needed because 'tp' may have siblings.
 */
static void
ntree_postorder(tp, process_node)
	struct tnode	*tp;
	void		(*process_node) ();
{
	if (tp->t_child != NULL) {
		ntree_postorder_aux(tp->t_child, process_node);
	}
	process_node(tp);
}


static void
ntree_postorder_aux(tp, process_node)
	struct tnode	*tp;
	void		(*process_node) ();
{
	if (tp->t_child != NULL) {
		ntree_postorder_aux(tp->t_child, process_node);
	}
	if (tp->t_sibling != NULL) {
		ntree_postorder_aux(tp->t_sibling, process_node);
	}
	process_node(tp);
}


struct tnode *
ntree_find_child(parent, name)
	struct tnode	*parent;
	char		*name;
{
	struct tnode	*tp;

	tp = parent->t_child;
	while ((tp != NULL) && (!NSE_STREQ(name, tp->t_name))) {
		tp = tp->t_sibling;
	}
	return (tp);
}


/*
 * Convert a tree node into a pathname:  follow the node's parent pointer
 * to get all the components of the full pathname.  NOTE:  this routine
 * assumes that the root node of the tree has a name starting with '/'.
 */
void
ntree_pathname(tp, pn)
	struct tnode	*tp;
	char		*pn;
{
	char            tpn[MAXPATHLEN];
	char		*pnptr = &tpn[MAXPATHLEN - 1];
	int             pathsize = 0;

	*pnptr = '\0';
	do {
		pnptr = prepend(tp->t_name, pnptr, &pathsize);
		pnptr = prepend("/", pnptr, &pathsize);
		tp = tp->t_parent;
	} while (tp->t_name[0] != '/');
	strcpy(pn, pnptr);
}
