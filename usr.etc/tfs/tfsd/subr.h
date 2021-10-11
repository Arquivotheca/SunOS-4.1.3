/*	@(#)subr.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

/*
 * tfs_backlink.c
 */
Nse_whiteout	bl_list_get_rename_elem();

/*
 * tfs_attrs.c
 */
extern bool_t	attr_cache_on;

/*
 * tfs_fd.c
 */
DIR            *tfs_opendir();

/*
 * tfs_lookup.c
 */
struct vnode	*mntpntovp();
struct vnode	*lookup_vnode();
struct pnode	*create_pnode();
struct pnode	*follow_searchlink();
struct pnode	*path_to_pnode();
struct vnode	*fhtovp();
struct vnode	*fhandle_find();
struct vnode	*find_vnode();

/*
 * tfs_mount.c
 */
char		*environ_var_name();
char		*environ_default_name();

/*
 * tfs_ntree.c
 */
extern struct pnode *root_pnode;
extern long	next_fhandle;

struct vnode	*create_vnode();
struct pnode	*alloc_pnode();
struct tnode	*ntree_find_child();

/*
 * tfs_printf.c
 */
#ifdef TFSDEBUG
extern int	tfsdebug;
#endif TFSDEBUG

/*
 * tfs_server.c
 */
extern char	*read_result_buffer;
extern char	*readdir_buffer;

void		tfs_dispatch();

/*
 * tfs_subr.c
 */
struct pnode	*release_linknode();
struct pnode	*get_front_parent_pnode();
struct pnode	*get_pnode_at_layer();
struct pnode	*get_next_pnode();
char		*prepend();
long		get_current_time();
FILE		*open_tfs_file();
struct vnode	*next_file();
struct vnode	*next_whiteout();

/*
 * other
 */
long		lseek();
