/*	@(#)vnode.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#ifndef _VNODE_H
#define _VNODE_H

#include <nse/searchlink.h>

/*
 * Node in tree (of pnodes or vnodes).
 */
struct tnode {
	char		*t_name;	/* pathname component */
	struct tnode	*t_parent;
	struct tnode	*t_child;
	struct tnode	*t_sibling;
	struct tnode	*t_cnext;	/* next tnode in comp table */
};


typedef struct vnode *Vnode;
typedef struct pnode *Pnode;

/*
 * If a directory contains a conditional searchlink, then the searchlink
 * may evaluate differently for different variants, and so the pnode for
 * that directory will have more than one 'next' pnode.  The linknode
 * implements a list of next pnodes.  The linknode is also used by
 * update_directory() to figure out which directory vnodes have pointers
 * to a given directory pnode, so that if a directory is modified
 * (e.g. a file is added) through one name, the change can be propagated
 * to all views of the directory.
 */
struct linknode {
	struct linknode	*l_next;
	Vnode		l_vnode;	/* vnode that the link is
					 * valid for */
	Pnode		l_next_pnode;	/* next pnode */
	int		l_layer;	/* layer # of this pnode in the
					 * directory with nodeid 'l_nodeid'
					 */
};


/*
 * Physical node.  A physical node exists for each physical file or directory
 * visited.  This information can be flushed, unlike the vnode information.
 */
struct pnode {
	struct tnode	p_tnode;	/* node in pnode tree */
	union {
		struct {
			struct linknode	*pu_link;  /* dir in next layer */
			long		pu_mtime;  /* modify time of dir */
			long		pu_expire; /* when does mtime need to
						      be checked? */
		} pu_dirstuff;
		struct {
			long		pu_offset; /* offset for rdwr */
			int		pu_size;   /* size of file */
			long		pu_write_time; /* time of last write */
		} pu_filestuff;
	} p_union;
	int		p_refcnt;	/* # of references */
	unsigned	p_sub_layer	: 2; /* sub-layer */
	unsigned	p_fd		: 8; /* file descriptor, or 0 */
	unsigned	p_write_error	: 8; /* error code for delayed write */
	unsigned	p_type		: 2; /* type of the file */
	unsigned	p_have_attrs	: 1; /* Have attrs for this pnode? */
	unsigned	p_needs_write	: 1; /*	Does this file have data in
					      * write cache? */
	unsigned	p_swapped	: 1; /* Has been swapped out? */
	unsigned	p_in_queue	: 1; /* In the LRU pnode queue? */
	unsigned	p_updated	: 1; /* Used by update_ctime() */
	unsigned	p_visited	: 1; /* Have we seen this node before
					      *	while following searchlinks? */
};

#define p_link		p_union.pu_dirstuff.pu_link
#define p_mtime		p_union.pu_dirstuff.pu_mtime
#define p_expire	p_union.pu_dirstuff.pu_expire
#define p_offset	p_union.pu_filestuff.pu_offset
#define p_size		p_union.pu_filestuff.pu_size
#define p_write_time	p_union.pu_filestuff.pu_write_time

/* File types */
#define PTYPE_DIR		0
#define PTYPE_REG		1
#define PTYPE_LINK		2
#define PTYPE_OTHER		3

#define p_name		p_tnode.t_name

#define PARENT_PNODE(pp)	((struct pnode *) (pp)->p_tnode.t_parent)
#define CHILD_PNODE(pp)		((struct pnode *) (pp)->p_tnode.t_child)

#define MIN_DIR_EXPIRE	30	/* Min secs before checking cached dir mtime */
#define MAX_DIR_EXPIRE	60	/* Max secs before checking cached dir mtime */

/*
 * Virtual node.  A virtual node is allocated for each file or directory
 * ever visited in the lifetime of the TFS server.
 */
struct vnode {
	struct tnode	v_tnode;	/* node in vnode tree */
	struct vnode	*v_fnext;	/* next vnode in fhandle table */
	long		v_fhid;		/* file handle/ nodeid */
	struct pnode	*v_pnode;	/* physical node for the file/dir */
	long		v_mtime;	/* modify time of file */
	unsigned	v_environ_id	: 5; /* Used in following search-
					      * links */
	unsigned	v_writeable	: 2; /* # of last writeable layer */
	unsigned	v_layer		: 8; /* Layer # of file/dir */
	unsigned	v_back_layer	: 8; /* Layer # file found in back */
	unsigned	v_whited_out	: 1; /* Is whited-out? */
	unsigned	v_back_whited_out : 1; /* Is whited-out in back? */
	unsigned	v_dir_valid	: 1; /*	Are vnodes of this dir
					      * up-to-date? */
	unsigned	v_created	: 1; /*	file created by TFS?  */
	unsigned	v_dont_flush	: 1; /* Used by update_directory() */
	unsigned	v_is_mount_pt	: 1; /* Is a mountpt vnode? */
	unsigned	v_back_owner	: 1; /* Own files showing through? */
};

#define v_name		v_tnode.t_name

#define PARENT_VNODE(vp)	((struct vnode *) (vp)->v_tnode.t_parent)
#define CHILD_VNODE(vp)		((struct vnode *) (vp)->v_tnode.t_child)
#define NEXT_VNODE(vp)		((struct vnode *) (vp)->v_tnode.t_sibling)


#define INVALID_LAYER	0xff
#define UNKNOWN_LAYER	0xfe
#define MAX_LAYERS	0xfe

/*
 * Is this vnode in one of the writeable layers?  We assume that
 * the parent directory always exists in the frontmost writeable
 * layer.  (We require any directory to exist in all writeable layers,
 * and if there are any back read-only layers, the directory must exist
 * to hold .tfs_backfiles.)  Without this assumption, we would have to
 * recursively promote all parent directories before doing any write
 * operation (including setting whiteout entries.)
 */
#define IS_WRITEABLE(vp)	(vp->v_layer <= vp->v_writeable)

/*
 * Maximum possible sub-layer.  This should be one less than the maximum
 * number of writeable layers.
 */
#define MAX_SUB_LAYER	1

/*
 * # of the frontmost sub-layer.  This guarantees that a given pnode
 * will always have the same sub-layer, regardless of the view it was
 * reached through (and the # of writeable layers for that view.)
 */
#define FIRST_SUBL(vp)		(MAX_SUB_LAYER - vp->v_writeable)

/*
 * Translate sub-layer # to internal layer #.
 */
#define SUBL_TO_LAYER(vp, pp)	(pp->p_sub_layer - FIRST_SUBL(vp))

/*
 * Max # of environments (branch/variants) the TFS can serve at once.
 * Restricted by the width of the v_environ_id field in the vnode.
 */
#define MAX_ENVIRON 31

/*
 * Max. # of names (vnodes) within one environment which can have the same
 * directory in the frontmost filesystem.  (Is equal to two because we can
 * view a file in the frontmost file system through an activated name or
 * through a branch-mount name.)
 */
#define MAX_NAMES	2

#endif _VNODE_H
