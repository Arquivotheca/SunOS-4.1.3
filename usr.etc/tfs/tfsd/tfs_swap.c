#ifndef lint
static char sccsid[] = "@(#)tfs_swap.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

#include <nse/util.h>
#include "headers.h"
#include "vnode.h"
#include "tfs.h"
#include "subr.h"
#include "tfs_lists.h"
#include <nse/param.h>
#include <nse/stdio.h>

/*
 * Once the TFS assigns a nodeid for a file, it has to remember that nodeid
 * forever until the mount is no longer active.  The TFS can't forget about
 * any nodeids because the kernel can keep a vnode open for a file/directory
 * indefinitely, and there is no way for the TFS to query the kernel to
 * determine which files it is currently referencing.
 *
 * Since the TFS has to remember the nodeids of all files that have been
 * referenced, the TFS can grow without bound, and can grow very large
 * indeed if a large filesystem is referenced.  Consequently, we need to
 * save the nodeids to files to save memory.
 *
 * The TFS's fhandle for a file contains the nodeid of the file and
 * the nodeid of the parent directory.  Therefore, if we are careful
 * to never swap out the nodeid of a directory that contains files, we
 * will always be able to figure out the file that an fhandle refers to.
 * When the TFS receives a request to operate on a file which is in a
 * swapped-out directory, then the fhandle lookup for the file will fail.
 * Since the file's fhandle also contains the nodeid of the parent
 * directory, we will be able to look up the directory by its nodeid,
 * swap in the directory, then redo the fhandle lookup for the file.
 * (The fhtovp() routine implements this logic.)
 *
 * While we're saving nodeids, we might as well save all the cached
 * information for a directory so that we won't have to revalidate
 * the directory when it is swapped back in.  So we save all the read-only
 * pnodes of the directory, and state information for the child vnodes.
 * Note that we keep all writeable pnodes in memory because it is possible
 * for one of the writeable layers of a swapped-out directory to be updated
 * through another environment, in which case we need to swap in the directory
 * to update it.  When we swap out a directory pnode in the frontmost
 * filesystem, we swap the nodeids of all vnodes which have that pnode as
 * their frontmost pnode (there can be a maximum of MAX_NAMES such vnodes.)
 *
 * Format of the swap file:

<tfsd's pid> <timestamp>
<dir nodeid #1> <dir nodeid #2>
<directory name> <sub-layer>
   .....
<name> <nodeid #1> <nodeid #2> <layer> <w-o> <back-layer> <back w-o> <mtime>
   .....
  (the last three fields are written out only if they need to be)

 */


/*
 * Maximum # of front filesystem pnodes allowed to have all read-only pnodes
 * and child vnodes in memory.
 */
#define MAX_RESIDENT_PNODES	30


typedef struct Tfs_swap {
	long		dir_nodeids[MAX_NAMES];
	Nse_list	directories;
	Nse_list	children;
} *Tfs_swap, Tfs_swap_rec;


/*
 * LRU queue of pnodes.  The LRU pnode is at the head of the queue.
 */
static Nse_list	pnode_queue;
static struct pnode *tail_pnode;

#ifdef TFSDEBUG
int		nswapin;
int		nswapout;
int		nswapouterr;
#endif TFSDEBUG
static int	my_pid;
static long	my_timestamp;

static void	swap_out_directory();
void		swap_in_directory();
static void	save_readonly_pnodes();
static void	release_readonly_pnodes();
static void	restore_readonly_pnodes();
static struct pnode *get_pnode_if_exists();
static void	save_child_vnodes();
static void	release_child_vnodes();
static void	restore_child_vnodes();
static void	restore_vnode();
static Tfs_swap	read_swapfile();
static bool_t	write_swapfile();
static Nse_err	read_dirpnode_entry();
static Nse_err	write_dirpnode_entry();
static Nse_err	read_vnode_entry();
static Nse_err	write_vnode_entry();
static Tfs_swap	swap_rec_create();
static void	swap_rec_destroy();
static bool_t	vnodelist_name_eq();

/*
 * Routines to implement the LRU pnode queue.
 */
void		swqueue_use_pnode();
void		swqueue_add_pnode();
static void	swqueue_enqueue();
void		swqueue_dequeue();
#ifdef TFSDEBUG
void		swqueue_print();
static void	print_enqueued_pnode();
#endif TFSDEBUG

void		init_swap();


static void
swap_out_directory(pp)
	struct pnode	*pp;
{
	struct vnode	*vnodes[MAX_NAMES];
	char		pn[MAXPATHLEN];
	FILE		*file;
	Tfs_swap	swap;
	int		i;
	Nse_err		err;
	struct stat	statb;

	bzero((char *) vnodes, sizeof vnodes);
	get_vnodes(pp, vnodes);
	if (vnodes[0] == NULL) {
		/*
		 * Can happen if all the views of this pnode were
		 * unmounted or flushed, but a reference still remains
		 * for the pnode.  (Because this pnode is for a root
		 * vnode or is not the frontmost pnode for some other
		 * vnode.)
		 */
		return;
	}
	ptopn(pp, pn);
	nse_pathcat(pn, NSE_TFS_SWAP_FILE);
	file = open_tfs_file(pn, "w", &err);
	if (file == NULL) {
		/*
		 * Print a warning message iff the directory still exists.
		 * (It's possible that we are trying to swap out a directory
		 * in an environment which has been deleted, in which case
		 * the mountd will unmount this directory soon.)
		 */
		*rindex(pn, '/') = '\0';
		if (stat(pn, &statb) == 0) {
			nse_log_err_print(err);
		}
		return;
	}
	swap = swap_rec_create();
	save_readonly_pnodes(vnodes[0], swap->directories);
	for (i = 0; i < MAX_NAMES && vnodes[i]; i++) {
#ifdef TFSDEBUG
		dprint(tfsdebug, 4, "Swapping out [%d %d]\n",
			PARENT_VNODE(vnodes[i])->v_fhid, vnodes[i]->v_fhid);
#endif TFSDEBUG
		swap->dir_nodeids[i] = vnodes[i]->v_fhid;
		save_child_vnodes(vnodes[i], swap->children, i);
	}
	/*
	 * Don't swap the directory out if there was an error writing
	 * the swap file (if, for example, the filesystem was full.)  Note
	 * that in such a case, the directory's pnodes and child vnodes
	 * will stay in memory forever.  Too bad.
	 */
	if (write_swapfile(pn, file, swap)) {
		for (i = 0; i < MAX_NAMES && vnodes[i]; i++) {
			release_child_vnodes(vnodes[i]);
			release_readonly_pnodes(vnodes[i]);
		}
		pp->p_swapped = TRUE;
	} else {
#ifdef TFSDEBUG
		nswapouterr++;
#endif TFSDEBUG
	}
	swap_rec_destroy(swap);
#ifdef TFSDEBUG
	nswapout++;
#endif TFSDEBUG
}


void
swap_in_directory(pp)
	struct pnode	*pp;
{
	Tfs_swap	swap;
	struct vnode	*vp;
	int		i;

	swap = read_swapfile(pp);
	if (swap == NULL) {
		struct vnode	*vnodes[MAX_NAMES];

		/*
		 * This should never happen.  If it does happen, then it
		 * will probably be because another process besides the
		 * tfsd has been removing .tfs_swap files.
		 */
		nse_log_message("warning: can't read swapfile");
		print_pnode_path(pp);
		/*
		 * Mark the dirs as invalid so that they will be revalidated.
		 */
		bzero((char *) vnodes, sizeof vnodes);
		get_vnodes(pp, vnodes);
		for (i = 0; i < MAX_NAMES && vnodes[i]; i++) {
			vnodes[i]->v_dir_valid = FALSE;
		}
		pp->p_swapped = FALSE;
		return;
	}
	for (i = 0; i < MAX_NAMES && swap->dir_nodeids[i] != 0; i++) {
		vp = fhandle_find((unsigned long)swap->dir_nodeids[i]);
		if (vp == NULL) {
			/*
			 * Can happen if one view on a directory was
			 * unmounted after being swapped out.
			 */
			continue;
		}
#ifdef TFSDEBUG
		dprint(tfsdebug, 4, "Swapping in [%d %d]\n",
			PARENT_VNODE(vp)->v_fhid, vp->v_fhid);
#endif TFSDEBUG
		if (nse_list_nelems(swap->directories) > 0) {
			restore_readonly_pnodes(vp, swap->directories);
		}
		restore_child_vnodes(vp, swap->children, i);
		if (!vp->v_is_mount_pt && PARENT_VNODE(vp)->v_pnode == NULL) {
			(void) recurse_lookup_pnode(PARENT_VNODE(vp));
		}
	}
	swap_rec_destroy(swap);
	swqueue_add_pnode(pp);
	/*
	 * It's important that we wait to clear the swapped flag until
	 * after adding the pnode to the swap queue.  It's possible that
	 * enqueueing this pnode has forced this pnode's parent to be
	 * swapped out.
	 */
	pp->p_swapped = FALSE;
#ifdef TFSDEBUG
	nswapin++;
#endif TFSDEBUG
}


static void
save_readonly_pnodes(vp, list)
	struct vnode	*vp;
	Nse_list	list;
{
	struct pnode	*pp;
	Filelist	fl;
	char		pn[MAXPATHLEN];

	pp = get_pnode_if_exists(vp, vp->v_writeable + 1);
	while (pp) {
		fl = NSE_NEW(Filelist);
		ptopn(pp, pn);
		fl->fname = NSE_STRDUP(pn);
		fl->layer = pp->p_sub_layer;
		(void) nse_list_add_new_data(list, (Nse_opaque) fl);
		pp = get_next_pnode(vp, pp);
	}
}


static void
release_readonly_pnodes(vp)
	struct vnode	*vp;
{
	struct pnode	*pp;
	struct pnode	*next_pp;
	struct pnode	*front_pp;

	front_pp = get_pnode_if_exists(vp, vp->v_writeable);
	if (front_pp == NULL) {
		return;
	}
	pp = release_linknode(vp, front_pp);
	while (pp) {
		next_pp = release_linknode(vp, pp);
		free_pnode(pp);
		pp = next_pp;
	}
	set_next_pnode(vp, front_pp, (struct pnode *) NULL,
		       (int) vp->v_writeable);
}


static void
restore_readonly_pnodes(vp, list)
	struct vnode	*vp;
	Nse_list	list;
{
	struct pnode	*front_pp = vp->v_pnode;
	struct pnode	*pp;
	struct pnode	*next_pp = NULL;
	Filelist	fl;
	Nse_listelem	elem;
	Nse_listelem	end;

	front_pp = get_pnode_if_exists(vp, vp->v_writeable);
	if (front_pp == NULL) {
		/*
		 * Can happen if a directory is flushed after being swapped
		 * out.  When we get to this point, the directory is being
		 * swapped in just before it is going to be revalidated, so
		 * don't bother reconstructing the read-only pnodes as
		 * they will be rebuilt when the dir. is revalidated.
		 */
		return;
	}
	(void) release_linknode(vp, front_pp);
	NSE_LIST_ITERATE_REV(list, Filelist, fl, elem, end) {
		pp = path_to_pnode(fl->fname, (unsigned) fl->layer);
		set_next_pnode(vp, pp, next_pp, 0);
		next_pp = pp;
	}
	set_next_pnode(vp, front_pp, next_pp, 0);
	set_linknode_layers(vp);
}


/*
 * If a pnode exists for vnode 'vp' at layer 'layer', return it.
 */
static struct pnode *
get_pnode_if_exists(vp, layer)
	struct vnode	*vp;
	unsigned	layer;
{
	struct pnode	*pp = vp->v_pnode;
	int		i = 0;

	while (i < layer) {
		pp = get_next_pnode(vp, pp);
		if (pp == NULL) {
			return NULL;
		}
		i++;
	}
	return pp;
}


static void
save_child_vnodes(pvp, list, index)
	struct vnode	*pvp;
	Nse_list	list;
	int		index;
{
	struct vnode	*vp;
	Vnodelist	vl;

	for (vp = CHILD_VNODE(pvp); vp != NULL; vp = NEXT_VNODE(vp)) {
		if (CHILD_VNODE(vp) != NULL ||
		    (vp->v_pnode != NULL && vp->v_pnode->p_swapped)) {
			continue;
		}
		if (index == 0) {
			vl = NULL;
		} else {
			vl = (Vnodelist) nse_list_search(list,
							 vnodelist_name_eq,
							 vp->v_name);
		}
		if (vl != NULL) {
			vl->nodeids[index] = vp->v_fhid;
		} else {
			vl = NSE_NEW(Vnodelist);
			vl->name = NSE_STRDUP(vp->v_name);
			vl->nodeids[index] = vp->v_fhid;
			vl->layer = vp->v_layer;
			vl->back_layer = vp->v_back_layer;
			vl->whited_out = vp->v_whited_out;
			vl->back_whited_out = vp->v_back_whited_out;
			vl->mtime = vp->v_mtime;
			(void) nse_list_add_new_data(list, (Nse_opaque) vl);
		}
	}
}


static void
release_child_vnodes(pvp)
	struct vnode	*pvp;
{
	struct vnode	*vp;
	struct vnode	*next_vp;

	for (vp = CHILD_VNODE(pvp); vp != NULL; vp = next_vp) {
		next_vp = NEXT_VNODE(vp);
		if (CHILD_VNODE(vp) != NULL ||
		    (vp->v_pnode != NULL && vp->v_pnode->p_swapped)) {
			continue;
		}
		free_vnode(vp);
	}
}


static void
restore_child_vnodes(pvp, list, index)
	struct vnode	*pvp;
	Nse_list	list;
	int		index;
{
	nse_list_iterate(list, restore_vnode, pvp, index);
}


static void
restore_vnode(vl, pvp, index)
	Vnodelist	vl;
	struct vnode	*pvp;
	int		index;
{
	struct vnode	*vp;

	if (vl->nodeids[index] == 0) {
		return;
	}
	vp = create_vnode(pvp, vl->name, vl->nodeids[index]);
	vp->v_layer = vl->layer;
	vp->v_back_layer = vl->back_layer;
	vp->v_whited_out = vl->whited_out;
	vp->v_back_whited_out = vl->back_whited_out;
	vp->v_mtime = vl->mtime;
}


/*
 * Note that we don't write a version number into .tfs_swap as we do with
 * other .tfs_ files because .tfs_swap is only read by the same instance
 * of the TFS.
 */
static Tfs_swap
read_swapfile(pp)
	struct pnode	*pp;
{
	char		path[MAXPATHLEN];
	FILE		*file;
	Tfs_swap	swap = NULL;
	int		pid;
	long		timestamp;
	int		ch;
	Filelist	fl;
	Vnodelist	vl;
	int		count;
	Nse_err		err;

	ptoname_or_pn(pp, path);
	nse_pathcat(path, NSE_TFS_SWAP_FILE);
	file = open_tfs_file(path, "r", (Nse_err *) NULL);
	if (file == NULL) {
		return NULL;
	}
	err = nse_fscanf(path, &count, file, "%d %ld\n", &pid, &timestamp);
	if (err || count != 2 ||
	    pid != my_pid || timestamp != my_timestamp) {
		goto error;
	}
	swap = swap_rec_create();
	err = nse_fscanf(path, &count, file, "%ld %ld\n",
			 &swap->dir_nodeids[0], &swap->dir_nodeids[1]);
	if (err || count != 2) {
		goto error;
	}
	do {
		err = nse_fgetc(path, file, &ch);
		if (err) {
			goto error;
		}
		ungetc(ch, file);
		if (ch != '/' || ch == EOF) {
			break;
		}
		if (err = read_dirpnode_entry(file, path, &fl)) {
			goto error;
		}
		if (fl) {
			(void) nse_list_add_new_data(swap->directories,
						     (Nse_opaque) fl);
		}
	} while (1);
	do {
		if (err = read_vnode_entry(file, path, &vl)) {
			goto error;
		}
		if (vl == NULL) {
			break;
		}
		(void) nse_list_add_new_data(swap->children, (Nse_opaque) vl);
	} while (1);
	if (err = nse_fclose(path, file)) {
		goto error;
	}
	(void) unlink(path);
	return swap;
error:
	if (err) {
		nse_log_err_print(err);
		fclose(file);
	}
	if (swap) {
		swap_rec_destroy(swap);
	}
	fclose(file);
	return NULL;
}


static bool_t
write_swapfile(path, file, swap)
	char		*path;
	FILE		*file;
	Tfs_swap	swap;
{
	Nse_err		err;

	if ((err = nse_fprintf(path, file, "%d %ld\n",
			       my_pid, my_timestamp)) ||
	    (err = nse_fprintf(path, file, "%ld %ld\n",
			       swap->dir_nodeids[0], swap->dir_nodeids[1])) ||
	    (err = (Nse_err) nse_list_iterate_or(swap->directories,
					write_dirpnode_entry, file, path)) ||
	    (err = (Nse_err) nse_list_iterate_or(swap->children,
					write_vnode_entry, file, path))) {
		fclose(file);
	} else {
		err = nse_fclose(path, file);
	}
	if (err) {
		nse_log_err_print(err);
		(void) unlink(path);
	}
	return (err == NULL);
}


#define DIR_PNODE_FORMAT	"%s %d\n"


static Nse_err
read_dirpnode_entry(file, path, entryp)
	FILE		*file;
	char		*path;
	Filelist	*entryp;
{
	Filelist	fl;
	char		name[MAXPATHLEN];
	int		sub_layer;
	int		count;
	Nse_err		err;

	err = nse_fscanf(path, &count, file, DIR_PNODE_FORMAT, name,
			 &sub_layer);
	if (err || count != 2) {
		*entryp = NULL;
		return err;
	}
	fl = NSE_NEW(Filelist);
	fl->fname = NSE_STRDUP(name);
	fl->layer = sub_layer;
	*entryp = fl;
	return NULL;
}


static Nse_err
write_dirpnode_entry(fl, file, path)
	Filelist	fl;
	FILE		*file;
	char		*path;
{
	return nse_fprintf(path, file, DIR_PNODE_FORMAT, fl->fname, fl->layer);
}


#define VNODE_FORMAT	"%s %ld %ld %d %d %d %d %ld\n"
#define VNODE_SHORT_FORMAT "%s %ld %ld %d %d\n"


static Nse_err
read_vnode_entry(file, path, entryp)
	FILE		*file;
	char		*path;
	Vnodelist	*entryp;
{
	Vnodelist	vl;
	char		line[MAXPATHLEN];
	char		name[MAXPATHLEN];
	int		nitems;
	bool_t		eof;
	Nse_err		err;

	err = nse_fgets(path, line, MAXPATHLEN, file, &eof);
	if (eof) {
		*entryp = NULL;
		return err;
	}
	vl = NSE_NEW(Vnodelist);
	nitems = sscanf(line, VNODE_FORMAT, name, &vl->nodeids[0],
			&vl->nodeids[1], &vl->layer, &vl->whited_out,
			&vl->back_layer, &vl->back_whited_out, &vl->mtime);
	if (nitems == 5) {
		vl->back_layer = INVALID_LAYER;
	} else if (nitems != 8) {
		free((char *) vl);
		*entryp = NULL;
		return NULL;
	}
	vl->name = NSE_STRDUP(name);
	*entryp = vl;
	return NULL;
}


static Nse_err
write_vnode_entry(vl, file, path)
	Vnodelist	vl;
	FILE		*file;
	char		*path;
{
	if (vl->back_layer != INVALID_LAYER || vl->mtime != 0) {
		return nse_fprintf(path, file, VNODE_FORMAT,
			vl->name, vl->nodeids[0],
			vl->nodeids[1], vl->layer, vl->whited_out,
			vl->back_layer, vl->back_whited_out, vl->mtime);
	} else {
		return nse_fprintf(path, file,
			VNODE_SHORT_FORMAT, vl->name, vl->nodeids[0],
			vl->nodeids[1], vl->layer, vl->whited_out);
	}
}


static Tfs_swap
swap_rec_create()
{
	Tfs_swap	swap;

	swap = NSE_NEW(Tfs_swap);
	swap->directories = filelist_create();
	swap->children = vnodelist_create();
	return swap;
}


static void
swap_rec_destroy(swap)
	Tfs_swap	swap;
{
	nse_list_destroy(swap->directories);
	nse_list_destroy(swap->children);
	free((char *) swap);
}


static bool_t
vnodelist_name_eq(vl, name)
	Vnodelist	vl;
	char		*name;
{
	return NSE_STREQ(vl->name, name);
}


/*
 * Routines to implement the LRU pnode queue.
 */

void
swqueue_use_pnode(pp)
	struct pnode	*pp;
{
	if (pp == tail_pnode) {
		return;
	}
	swqueue_dequeue(pp);
	swqueue_enqueue(pp);
}


void
swqueue_add_pnode(pp)
	struct pnode	*pp;
{
	swqueue_enqueue(pp);
	if (nse_list_nelems(pnode_queue) > MAX_RESIDENT_PNODES) {
		pp = (struct pnode *) nse_listelem_data(
					nse_list_first_elem(pnode_queue));
		swqueue_dequeue(pp);
		swap_out_directory(pp);
	}
}


static void
swqueue_enqueue(pp)
	struct pnode	*pp;
{
	(void) nse_list_add_new_data(pnode_queue, (Nse_opaque) pp);
	pp->p_in_queue = TRUE;
	tail_pnode = pp;
}


void
swqueue_dequeue(pp)
	struct pnode	*pp;
{
	nse_listelem_delete(pnode_queue,
			    nse_list_find_elem(pnode_queue, (Nse_opaque) pp));
	pp->p_in_queue = FALSE;
	if (pp == tail_pnode) {
		tail_pnode = NULL;
	}
}


#ifdef TFSDEBUG
void
swqueue_print()
{
	dprint(0, 0, "Swap queue:  (LRU is first elem)\n");
	nse_list_iterate(pnode_queue, print_enqueued_pnode);
	dprint(0, 0,
	    "%d directories swapped out, %d swapped in, %d swap-out errors\n",
		nswapout, nswapin, nswapouterr);
}


static void
print_enqueued_pnode(pp)
	struct pnode	*pp;
{
	char		pn[MAXPATHLEN];

	ptopn(pp, pn);
	dprint(0, 0, "  %s\n", pn);
}
#endif TFSDEBUG


void
init_swap()
{
	struct timeval	tv;

	my_pid = getpid();
	gettimeofday(&tv, (struct timezone *) NULL);
	my_timestamp = tv.tv_sec;
	pnode_queue = ptrlist_create();
}
