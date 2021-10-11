#ifndef lint
static        char sccsid[] = "@(#)heap_kmem.c 1.1 90/03/23 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */


/* define DEBUG ON */
#undef	DEBUG

/*
 * Conditions on use:
 * kmem_alloc and kmem_free must not be called from interrupt level,
 * except from software interrupt level.  This is because they are
 * not reentrant, and only block out software interrupts.  They take
 * too long to block any real devices.  There is a routine
 * kmem_free_intr that can be used to free blocks at interrupt level,
 * but only up to splimp, not higher.  This is because kmem_free_intr
 * only spl's to splimp.
 *
 * Also, these routines are not that fast, so they should not be used
 * in very frequent operations (e.g. operations that happen more often
 * than, say, once every few seconds).
 */

/*
 * description:
 *	Yet another memory allocator, this one based on a method
 *	described in C.J. Stephenson, "Fast Fits", IBM Sys. Journal
 *
 *	The basic data structure is a "Cartesian" binary tree, in which
 *	nodes are ordered by ascending addresses (thus minimizing free
 *	list insertion time) and block sizes decrease with depth in the
 *	tree (thus minimizing search time for a block of a given size).
 *
 *	In other words, for any node s, letting D(s) denote
 *	the set of descendents of s, we have:
 *
 *	a. addr(D(left(s))) <  addr(s) <  addr(D(right(s)))
 *	b. len(D(left(s)))  <= len(s)  >= len(D(right(s)))
 */

#include <machine/pte.h>
#include <sys/param.h>
#include <sys/map.h>
#include <sys/time.h>
#include <sys/proc.h>
#include "boot/cmap.h"
#include <sys/kernel.h>
#include <sys/vm.h>
#include <stand/saio.h>
#include <mon/sunromvec.h>

#ifdef	NFS_BOOT
/*
 * Local changes for boot.
 */

struct map *kernelmap;

#undef CLBYTES
#ifdef	sun2
#define	CLBYTES 2048
#endif	 /* sun2 */
#ifdef	 sun3
#define	CLBYTES	8192
#endif	/* sun3 */
#ifdef	 sun3x
#define	CLBYTES	8192
#endif	/* sun3x */
#ifdef	sun4
#define	CLBYTES 8192
#endif	 /* sun4 */
#ifdef	sun4c
#define	CLBYTES 2048
#endif	 /* sun4c */
/* XXX - what is the significance of this? */
#ifdef	sun4m
#define	CLBYTES 2048
#endif	 /* sun4c */
#endif	/* NFS_BOOT */


/*
 * The node header structure.
 * 
 * To reduce storage consumption, a header block is associated with
 * free blocks only, not allocated blocks.
 * When a free block is allocated, its header block is put on 
 * a free header block list.
 *
 * This creates a header space and a free block space.
 * The left pointer of a header blocks is used to chain free header
 * blocks together.
 */

typedef enum {false,true} bool;
typedef struct	freehdr	*Freehdr;
typedef struct	dblk	*Dblk;

/*
 * Description of a header for a free block
 * Only free blocks have such headers.
 */
struct 	freehdr	{
	Freehdr	left;			/* Left tree pointer */
	Freehdr	right;			/* Right tree pointer */
	Dblk	block;			/* Ptr to the data block */
	u_int	size;			/* Size of the data block */
};

#define NIL		((Freehdr) 0)
#define WORDSIZE	sizeof (int)
#define	SMALLEST_BLK	1	 	/* Size of smallest block */

/*
 * Description of a data block.  
 */
struct	dblk	{
	char	data[1];		/* Addr returned to the caller */
};

/*
 * weight(x) is the size of a block, in bytes; or 0 if and only if x
 *	is a null pointer. It is the responsibility of kmem_alloc() and
 *	kmem_free() to keep zero-length blocks out of the arena.
 */

#define	weight(x)	((x) == NIL? 0: (x->size))
#define	nextblk(p, size) ((Dblk) ((char *) (p) + (size)))
#define	max(a, b)	((a) < (b)? (b): (a))

Freehdr	getfreehdr();
bool	morecore();
#ifdef	NFS_BOOT
#else
caddr_t	getpages();
#endif	/* NFS_BOOT */
caddr_t	kmem_alloc();

/*
 * Structure containing various info about allocated memory.
 */
#ifdef	NFS_BOOT
#define	NEED_TO_FREE_SIZE	5
#else
#define	NEED_TO_FREE_SIZE	10
#endif	/* NFS_BOOT */
struct kmem_info {
	Freehdr	free_root;
	Freehdr	free_hdr_list;
	struct map *map;
	struct pte *pte;
	caddr_t	vaddr;
	struct need_to_free {
		caddr_t addr;
		u_int	nbytes;
	} need_to_free_list, need_to_free[NEED_TO_FREE_SIZE];
} kmem_info;


/*
 * Initialize kernel memory allocator
 */
kmem_init()
{
	register int i;
	register struct need_to_free *ntf;

#ifdef DEBUG
printf("kmem_init entered\n");
#endif
	kmem_info.free_root = NIL;
	kmem_info.free_hdr_list = NULL;
	kmem_info.map = kernelmap;
#ifdef	NFS_BOOT
#else
	kmem_info.pte = Usrptmap;
	kmem_info.vaddr = (caddr_t) usrpt;
#endif	/* NFS_BOOT */
	kmem_info.need_to_free_list.addr = 0;
	ntf = kmem_info.need_to_free;
	for (i = 0; i < NEED_TO_FREE_SIZE; i++) {
		ntf[i].addr = 0;
	}
#ifdef DEBUG
printf("kmem_init returning\n");
prtree(kmem_info.free_root, "kmem_init");
#endif
}

/*
 * Insert a new node in a cartesian tree or subtree, placing it
 *	in the correct position with respect to the existing nodes.
 *
 * algorithm:
 *	Starting from the root, a binary search is made for the new
 *	node. If this search were allowed to continue, it would
 *	eventually fail (since there cannot already be a node at the
 *	given address); but in fact it stops when it reaches a node in
 *	the tree which has a length less than that of the new node (or
 *	when it reaches a null tree pointer).  The new node is then
 *	inserted at the root of the subtree for which the shorter node
 *	forms the old root (or in place of the null pointer).
 */


insert(p, len, tree)
register Dblk p;		/* Ptr to the block to insert */
register u_int len;		/* Length of new node */
register Freehdr *tree;		/* Address of ptr to root */
{
	register Freehdr x;
	register Freehdr *left_hook;	/* Temp for insertion */
	register Freehdr *right_hook;	/* Temp for insertion */
	register Freehdr newhdr;

	x = *tree;
	/*
	 * Search for the first node which has a weight less
	 *	than that of the new node; this will be the
	 *	point at which we insert the new node.
	 */

	while (weight(x) >= len) {	
		if (p < x->block)
			tree = &x->left;
		else
			tree = &x->right;
		x = *tree;
	}

	/*
	 * Perform root insertion. The variable x traces a path through
	 *	the tree, and with the help of left_hook and right_hook,
	 *	rewrites all links that cross the territory occupied
	 *	by p.  Note that this improves performance under
	 *	paging.
	 */ 

	newhdr = getfreehdr();
	*tree = newhdr;
	left_hook = &newhdr->left;
	right_hook = &newhdr->right;

	newhdr->left = NIL;
	newhdr->right = NIL;
	newhdr->block = p;
	newhdr->size = len;

	while (x != NIL) {
		/*
		 * Remark:
		 *	The name 'left_hook' is somewhat confusing, since
		 *	it is always set to the address of a .right link
		 *	field.  However, its value is always an address
		 *	below (i.e., to the left of) p. Similarly
		 *	for right_hook. The values of left_hook and
		 *	right_hook converge toward the value of p,
		 *	as in a classical binary search.
		 */
		if (x->block < p) {
			/*
			 * rewrite link crossing from the left
			 */
			*left_hook = x;
			left_hook = &x->right;
			x = x->right;
		} else {
			/*
			 * rewrite link crossing from the right
			 */
			*right_hook = x;
			right_hook = &x->left;
			x = x->left;
		} /*else*/
	} /*while*/

	*left_hook = *right_hook = NIL;		/* clear remaining hooks */

} /*insert*/


/*
 * Delete a node from a cartesian tree. p is the address of
 *	a pointer to the node which is to be deleted.
 *
 * algorithm:
 *	The left and right sons of the node to be deleted define two
 *	subtrees which are to be merged and attached in place of the
 *	deleted node.  Each node on the inside edges of these two
 *	subtrees is examined and longer nodes are placed above the
 *	shorter ones.
 *
 * On entry:
 *	*p is assumed to be non-null.
 */

delete(p)
register Freehdr *p;
{
	register Freehdr x;
	register Freehdr left_branch;	/* left subtree of deleted node */
	register Freehdr right_branch;	/* right subtree of deleted node */

	x = *p;
	left_branch = x->left;
	right_branch = x->right;

	while (left_branch != right_branch) {	
		/*
		 * iterate until left branch and right branch are
		 * both NIL.
		 */
		if (weight(left_branch) >= weight(right_branch)) {
			/*
			 * promote the left branch
			 */
			*p = left_branch;
			p = &left_branch->right;
			left_branch = left_branch->right;
		} else {
			/*
			 * promote the right branch
			 */
			*p = right_branch;
			p = &right_branch->left;
			right_branch = right_branch->left;
		}/*else*/
	}/*while*/
	*p = NIL;
	freehdr(x);
} /*delete*/


/*
 * Demote a node in a cartesian tree, if necessary, to establish
 *	the required vertical ordering.
 *
 * algorithm:
 *	The left and right subtrees of the node to be demoted are to
 *	be partially merged and attached in place of the demoted node.
 *	The nodes on the inside edges of these two subtrees are
 *	examined and the longer nodes are placed above the shorter
 *	ones, until a node is reached which has a length no greater
 *	than that of the node being demoted (or until a null pointer
 *	is reached).  The node is then attached at this point, and
 *	the remaining subtrees (if any) become its descendants.
 *
 * on entry:
 *   a. All the nodes in the tree, including the one to be demoted,
 *	must be correctly ordered horizontally;
 *   b. All the nodes except the one to be demoted must also be
 *	correctly positioned vertically.  The node to be demoted
 *	may be already correctly positioned vertically, or it may
 *	have a length which is less than that of one or both of
 *	its progeny.
 *   c. *p is non-null
 */


demote(p)
register Freehdr *p;
{
	register Freehdr x;		/* addr of node to be demoted */
	register Freehdr left_branch;
	register Freehdr right_branch;
	register u_int    wx;

	x = *p;
	left_branch = x->left;
	right_branch = x->right;
	wx = weight(x);

	while (weight(left_branch) > wx || weight(right_branch) > wx) {
		/*
		 * select a descendant branch for promotion
		 */
		if (weight(left_branch) >= weight(right_branch)) {
			/*
			 * promote the left branch
			 */
			*p = left_branch;
			p = &left_branch->right;
			left_branch = *p;
		} else {
			/*
			 * promote the right branch
			 */
			*p = right_branch;
			p = &right_branch->left;
			right_branch = *p;
		} /*else*/
	} /*while*/

	*p = x;				/* attach demoted node here */
	x->left = left_branch;
	x->right = right_branch;
} /*demote*/

/*
 * Allocate a block of storage
 *
 * algorithm:
 *	The freelist is searched by descending the tree from the root
 *	so that at each decision point the "better fitting" child node
 *	is chosen (i.e., the shorter one, if it is long enough, or
 *	the longer one, otherwise).  The descent stops when both
 *	child nodes are too short.
 *
 * function result:
 *	kmem_alloc returns a pointer to the allocated block; a null
 *	pointer indicates storage could not be allocated.
 */
/*
 * We need to return blocks that are on word boundaries so that callers
 * that are putting int's into the area will work.  Since we allow
 * arbitrary free'ing, we need a weight function that considers
 * free blocks starting on an odd boundary special.  Allocation is
 * aligned to 4 byte boundaries (ALIGN).
 */
#define	ALIGN		sizeof(int)
#define	ALIGNMASK	(ALIGN-1)
#define	ALIGNMORE(addr)	(ALIGN - ((int)(addr) & ALIGNMASK))

#define	mweight(x) ((x) == NIL ? 0 : \
    ((((int)(x)->block) & ALIGNMASK) && ((x)->size > ALIGNMORE((x)->block)))\
    ? (x)->size - ALIGNMORE((x)->block) : (x)->size)

caddr_t
kmem_alloc(nbytes)
	register u_int	nbytes;
{
	register Freehdr a;		/* ptr to node to be allocated */
	register Freehdr *p;		/* address of ptr to node */
	register u_int	 left_weight;
	register u_int	 right_weight;
	register Freehdr left_son;
	register Freehdr right_son;
	register char	 *retblock;	/* Address returned to the user */
	int s;
#ifdef	DEBUG
	printf("kmem_alloc(nbytes 0x%x)\n", nbytes);
#endif	/* DEBUG */

	if (nbytes == 0) {
		return(NULL);
	}
	s = splnet();

	if (nbytes < SMALLEST_BLK) {
		printf("illegal kmem_alloc call for %d bytes\n", nbytes);
		panic("kmem_alloc");
	}
	check_need_to_free();

	/*
	 * ensure that at least one block is big enough to satisfy
	 *	the request.
	 */

	if (mweight(kmem_info.free_root) <= nbytes) {
		/*
		 * the largest block is not enough. 
		 */
		if (!morecore(nbytes)) {
			printf("kmem_alloc failed, nbytes %d\n", nbytes);
			panic("kmem_alloc");
		}
	}

	/*
	 * search down through the tree until a suitable block is
	 *	found.  At each decision point, select the better
	 *	fitting node.
	 */

	p = (Freehdr *) &kmem_info.free_root;
	a = *p;
	left_son = a->left;
	right_son = a->right;
	left_weight = mweight(left_son);
	right_weight = mweight(right_son);

	while (left_weight >= nbytes || right_weight >= nbytes) {
		if (left_weight <= right_weight) {
			if (left_weight >= nbytes) {
				p = &a->left;
				a = left_son;
			} else {
				p = &a->right;
				a = right_son;
			}
		} else {
			if (right_weight >= nbytes) {
				p = &a->right;
				a = right_son;
			} else {
				p = &a->left;
				a = left_son;
			}
		}
		left_son = a->left;
		right_son = a->right;
		left_weight = mweight(left_son);
		right_weight = mweight(right_son);
	} /*while*/	

	/*
	 * allocate storage from the selected node.
	 */

	if (a->size - nbytes < SMALLEST_BLK) {
		/*
		 * not big enough to split; must leave at least
		 * a dblk's worth of space.
		 */
		retblock = a->block->data;
		delete(p);
	} else {

		/*
		 * split the node, allocating nbytes from the top.
		 *	Remember we've already accounted for the
		 *	allocated node's header space.
		 */
		register Freehdr x;
		x = getfreehdr();
		if ((int) a->block->data & ALIGNMASK &&
		    a->size > ALIGNMORE(a->block->data)) {
			nbytes += ALIGNMORE(a->block->data);
			x->block = a->block;
			x->size = ALIGNMORE(a->block->data);
			x->left = a->left;
			x->right = a->right;
			/*
			 * the node pointed to by *p has become smaller;
			 *	move it down to its appropriate place in
			 *	the tree.
			 */
			*p = x;
			demote(p);
			/*
			rmlog(0, a->block, 1, caller());
			rmlog(1, a->block, 1, caller());
			*/
			retblock = a->block->data + ALIGNMORE(a->block->data);
			if (a->size > nbytes) {
				/*
				rmlog(0, nextblk(a->block, nbytes), a->size - nbytes, caller());
				*/
				kmem_free((caddr_t)nextblk(a->block, nbytes),
				    (u_int)a->size - nbytes);
			}
			freehdr(a);
			nbytes -= ALIGNMORE(a->block->data);
		} else {
			x->block = nextblk(a->block, nbytes);
			x->size = a->size - nbytes;
			x->left = a->left;
			x->right = a->right;
			/*
			 * the node pointed to by *p has become smaller;
			 *	move it down to its appropriate place in
			 *	the tree.
			 */
			*p = x;
			demote(p);
			retblock = a->block->data;
			freehdr(a);
		}
	}
#ifdef DEBUG
prtree(kmem_info.free_root, "kmem_alloc");
printf("kmem_alloc returning %x\n", retblock);
#endif
	/*
	rmlog(0, retblock, nbytes, caller());
	*/
	(void) splx(s);
#ifdef	NFS_BOOT
	bzero(retblock, nbytes);
#endif	/* NFS_BOOT */
	return (retblock);

} /*kmem_alloc*/

/*
 * Return a block to the free space tree.
 * 
 * algorithm:
 *	Starting at the root, search for and coalesce free blocks
 *	adjacent to one given.  When the appropriate place in the
 *	tree is found, insert the given block.
 *
 * Do some sanity checks to avoid total confusion in the tree.
 * If the block has already been freed, panic.
 * If the ptr is not from the arena, panic.
 */
kmem_free(ptr, nbytes)
	caddr_t ptr;
	register u_int 	 nbytes;	/* Size of node to be released */
{
	register Freehdr *np;		/* For deletion from free list */
	register Freehdr neighbor;	/* Node to be coalesced */
	register char	 *neigh_block;	/* Ptr to potential neighbor */
	register u_int	 neigh_size;	/* Size of potential neighbor */
	int s;

#ifdef DEBUG
printf("kmem_free (ptr %x nbytes %d)\n", ptr, nbytes);
prtree(kmem_info.free_root, "kmem_free");
#endif
	if (nbytes == 0) {
		return;
	}

#ifdef	NFS_BOOT
#else
	/*
	 * check bounds of pointer.
	 */
	if (ptr < (caddr_t) usrpt ||
	    ptr > (caddr_t) usrpt + USRPTSIZE*NBPG) {
		printf("kmem_free: illegal pointer %x\n",ptr);
		panic("kmem_free");
		return;
	}
#endif	/* NFS_BOOT */
	s = splnet();

	/*
	bzero(ptr, nbytes);
	rmlog(1, ptr, nbytes, caller());
	*/
	/*
	 * Search the tree for the correct insertion point for this
	 *	node, coalescing adjacent free blocks along the way.
	 */
	np = &kmem_info.free_root;
	neighbor = *np;
	while (neighbor != NIL) {
		neigh_block = (char *) neighbor->block;
		neigh_size = neighbor->size;
		if (ptr < neigh_block) {
			if (ptr + nbytes == neigh_block) {
				/*
				 * Absorb and delete right neighbor
				 */
				nbytes += neigh_size;
				delete(np);
			} else if (ptr + nbytes > neigh_block) {
				/*
				 * The block being freed overlaps
				 * another block in the tree.  This
				 * is bad news.
				 */
				 printf("kmem_free: free block overlap %x+%d over %x\n", ptr, nbytes, neigh_block);
				 panic("kmem_free: free block overlap");
			} else {
				/*
				 * Search to the left
			 	*/
				np = &neighbor->left;
			}
		} else if (ptr > neigh_block) {
			if (neigh_block + neigh_size == ptr) {
				/*
				 * Absorb and delete left neighbor
				 */
				ptr = neigh_block;
				nbytes += neigh_size;
				delete(np);
			} else if (neigh_block + neigh_size > ptr) {
				/*
				 * This block has already been freed
				 */
				panic("kmem_free block already free");
			} else {
				/*
				 * search to the right
				 */
				np = &neighbor->right;
			}
		} else {
			/*
			 * This block has already been freed
			 * as "ptr == neigh_block"
			 */
			panic("kmem_free: block already free as neighbor");
			return;
		} /*else*/
		neighbor = *np;
	} /*while*/

	/*
	 * Insert the new node into the free space tree
	 */
	insert((Dblk) ptr, nbytes, &kmem_info.free_root);
#ifdef DEBUG
printf("exiting kmem_free\n");
prtree(kmem_info.free_root, "kmem_free");
#endif
	(void) splx(s);
} /*kmem_free*/

#ifdef	NFS_BOOT
#else
/*
 * We maintain a list of blocks that need to be freed.
 * This is because we don't want to spl the relatively long
 * routines malloc and free, but we need to be able to free
 * space at interrupt level.
 */
kmem_free_intr(ptr, nbytes)
	caddr_t ptr;
	register u_int nbytes;	/* Size of node to be released */
{
	register u_int i;
	register struct need_to_free *ntf;
	int s;

	s = splimp();
	if (nbytes >= sizeof (struct need_to_free)) {
		if ((int)ptr & ALIGNMASK) {
			i = ALIGNMORE(ptr);
			kmem_free_intr(ptr, i);
			kmem_free_intr(ptr + i, nbytes - i);
			return;
		}
		ntf = &kmem_info.need_to_free_list;
		*(struct need_to_free *)ptr = *ntf;
		ntf->addr = ptr;
		ntf->nbytes = nbytes;
		(void) splx(s);
		return;
	}
	ntf = kmem_info.need_to_free;
	for (i = 0; i < NEED_TO_FREE_SIZE; i++) {
		if (ntf[i].addr == 0) {
			ntf[i].addr = ptr;
			ntf[i].nbytes = nbytes;
			(void) splx(s);
			return;
		}
	}
	panic("kmem_free_intr");
}
#endif	/* NFS_BOOT */

static
check_need_to_free()
{
	register int i;
	register struct need_to_free *ntf;
	caddr_t addr;
	u_int nbytes;
	int s;

again:
	s = splimp();
	ntf = &kmem_info.need_to_free_list;
	if (ntf->addr) {
		addr = ntf->addr;
		nbytes = ntf->nbytes;
		*ntf = *(struct need_to_free *) ntf->addr;
		(void) splx(s);
		kmem_free(addr, nbytes);
		goto again;
	}
	ntf = kmem_info.need_to_free;
	for (i = 0; i < NEED_TO_FREE_SIZE; i++) {
		if (ntf[i].addr) {
			addr = ntf[i].addr;
			nbytes = ntf[i].nbytes;
			ntf[i].addr = 0;
			(void) splx(s);
			kmem_free(addr, nbytes);
			goto again;
		}
	}
	(void) splx(s);
}

/*
 * Add a block of at least nbytes to the free space tree.
 *
 * return value:
 *	true	if at least nbytes can be allocated
 *	false	otherwise
 *
 * remark:
 *	free space (delimited by the static variable ubound) is 
 *	extended by an amount determined by rounding nbytes up to
 *	a multiple of the system page size.
 */

bool
morecore(nbytes)
	u_int nbytes;
{
	Dblk p;
#ifdef	DEBUG
	printf("morecore(nbytes 0x%x)\n", nbytes);
#endif	/* DEBUG */


	nbytes = roundup(nbytes, CLBYTES);
#ifdef	NFS_BOOT
	p = (Dblk) resalloc(RES_MAINMEM, nbytes);
#else
	p = (Dblk) getpages(nbytes / NBPG);
#endif	/* NFS_BOOT */
	if (p == 0) {
		return (false);
	}
	kmem_free((caddr_t) p, nbytes);
	return (true);

} /*morecore*/

#ifdef	NFS_BOOT
#else
/*
 * get npages pages from the system
 */
caddr_t
getpages(npages)
	u_int npages;
{
	register caddr_t va;
	register long x;
	int s;

	s = splimp();
	while ((x = rmalloc(kmem_info.map, (long)npages)) == 0) {
		mapwant(kmem_info.map)++;
		if (intsvc()) {
			panic("getpages called at interrupt level");
		}
		(void) sleep((caddr_t)kmem_info.map, PZERO - 1);
	}
	(void) vmemall(&(kmem_info.pte)[x], (int)npages, &proc[0], CSYS);
	va = kmem_info.vaddr + (x << PGSHIFT);
	vmaccess(&(kmem_info.pte)[x], va, (int)npages);
#ifdef DEBUG
printf("getpages returning 0x%x, %d pages, x was %d\n", va, npages, x);
#endif
	(void) splx(s);
	return (va);
}
#endif	/* NFS_BOOT */


/*
 * Get a free block header
 * There is a list of available free block headers.
 * When the list is empty, allocate another pagefull.
 */
Freehdr
getfreehdr()
{
	Freehdr	r;
	int	n;
#ifdef	DEBUG
	printf("getfreehdr()\n");
#endif	/* DEBUG */

	if (kmem_info.free_hdr_list != NIL) {
		r = kmem_info.free_hdr_list;
		kmem_info.free_hdr_list = kmem_info.free_hdr_list->left;
	} else {
#ifdef	NFS_BOOT
		r = (Freehdr) resalloc(RES_MAINMEM, CLSIZE);
#else
		r = (Freehdr) getpages(CLSIZE);
#endif	/* NFS_BOOT */
		if (r == 0) {
			panic("getfreehdr");
		}
		for (n = 1; n < CLBYTES / sizeof (*r); n++) {
			freehdr(&r[n]);
		}
#ifdef	DEBUG
	printf("getfreehdr: freed %d headers\n", n);
#endif	/* DEBUG */
	}
	return (r);
}

/*
 * Free a free block header
 * Add it to the list of available headers.
 */

freehdr(p)
Freehdr	p;
{
	p->left = kmem_info.free_hdr_list;
	p->right = NIL;
	p->block = NULL;
	kmem_info.free_hdr_list = p;
}

#ifdef DEBUG
/*
 * Diagnostic routines
 */
static depth = 0;

prtree(p, cp)
Freehdr p;
char *cp;
{
	int n;
	if (depth == 0) {
		printf("prtree(p %x cp %s)\n", p, cp);
/*
	} else {
		printf("prtree. p %x depth %d\n", p, depth);
*/
	}
	if (p != NIL){
		depth++;
		prtree(p->left, (char *)NULL);
		depth--;

		for (n = 0; n < depth; n++) {
			printf("   ");
		}
		printf(
		     "(%x): (left = %x, right = %x, block = %x, size = %d)\n",
			p, p->left, p->right, p->block, p->size);

		depth++;
		prtree(p->right, (char *)NULL);
		depth--;
	}
}
#endif DEBUG
