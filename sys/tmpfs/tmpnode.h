/*  @(#)tmpnode.h 1.1 92/07/30 SMI */

#include <vm/seg_vn.h>

/*
 * tmpnode is the (tmp)inode for the anonymous page based file system
 */

struct tmpnode {
	u_int		tn_flags;		/* see T flags below */
	int		tn_count;		/* links to tmpnode */
	struct tmpnode	*tn_back;		/* linked list of tmpnodes */
	struct tmpnode	*tn_forw;		/* linked list of tmpnodes */
	long		tn_owner;		/* proc/thread locking node */
	union {
		struct tdirent	*un_dir;	/* pointer to directory list */
		char 		*un_symlink;	/* pointer to symlink */
	} un_tmpnode;
	struct vnode 	tn_vnode;		/* vnode for this tmpnode */
	struct anon_map	*tn_amapp;		/* anonymous pages map */
	struct vattr	tn_attr;		/* attributes */
};
#define	tn_dir		un_tmpnode.un_dir
#define	tn_symlink	un_tmpnode.un_symlink

/* tn_flags */
#define	TLOCKED		0x1		/* tmpnode is locked */
#define	TWANTED		0x2		/* tmpnode wanted */
#define	TREF		0x4		/* tmpnode referenced */

/* modes */
#define	TFMT	0170000		/* type of file */
#define	TFIFO	0010000		/* named pipe (fifo) */
#define	TFCHR	0020000		/* character special */
#define	TFDIR	0040000		/* directory */
#define	TFBLK	0060000		/* block special */
#define	TFREG	0100000		/* regular */
#define	TFLNK	0120000		/* symbolic link */
#define	TFSOCK	0140000		/* socket */

#define	TSUID	04000		/* set user id on execution */
#define	TSGID	02000		/* set group id on execution */
#define	TSVTX	01000		/* save swapped text even after use */
#define	TREAD	0400		/* read, write, execute permissions */
#define	TWRITE	0200
#define	TEXEC	0100

#define	TMAP_ALLOC	131072		/* anon map's increment click size */

#define	TP_TO_VP(tp)	(&(tp)->tn_vnode)
#define	VP_TO_TP(vp)	((struct tmpnode *)(vp)->v_data)

#ifdef KERNEL
struct tmpnode *tmpnode_alloc();
void tmpnode_get();
void tmpnode_free();
void tmpnode_put();
#endif KERNEL
