#ident "@(#)heap_kmem.c 1.1 92/07/30 SMI";

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
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

#include <sys/param.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/stream.h>		/* for setqsched */
#include <sys/systm.h>
#include <sys/vm.h>
#include <vm/seg.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/seg_kmem.h>
#include <sys/vmsystm.h>

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

/*
 * Description of a header for a free block
 * Only free blocks have such headers.
 */
struct 	freehdr	{
	struct freehdr *left;		/* Left tree pointer */
	struct freehdr *right;		/* Right tree pointer */
	caddr_t	block;			/* Ptr to the data block */
	u_int	size;			/* Size of the data block */
};

/*
 * weight(x) is the size of a block, in bytes; or 0 if and only if x
 *	is a null pointer.  It is the responsibility of kmem_alloc()
 *	and kmem_free() to keep zero-length blocks out of the arena.
 */

#define	weight(x)		((x) == NULL? 0 : (x->size))

static struct freehdr *getfreehdr();
static struct freehdr *freehdr_alloc();
static void freehdr();
static void check_need_to_free();
static int morecore();
caddr_t	getpages();
void	freepages();
#ifdef RECORD_USAGE
extern func_t caller();
#endif RECORD_USAGE
extern int servicing_interrupt();

/*
 * ALIGN specifies the maximum alignment requirement for allocated blocks.
 * It must be a power of two.  All blocks returned by kmem_alloc have this
 * alignment.  This is guaranteed by rounding the sizes of all requests up
 * to a multiple of ALIGN.
 *
 * ALIGN is set to sizeof (double) to allow doubleword accesses on Sun-4's.
 * Note that we assume below that sizeof (struct need_to_free) <= ALIGN.
 * This dependency could be eliminated if necessary by adding an extra
 * test to _kmem_alloc.
 */
#define	ALIGN		sizeof (double)
#define	ALIGNMASK	(ALIGN - 1)

/*
 * Unit of allocation when we obtain pages from the system
 */
#define KMEM_ALLOCUNIT	(16 * 1024)

/*
 * Structure containing various info about allocated memory.
 */
struct kmem_info {
	struct freehdr *free_root;	/* Root of free tree */
	struct freehdr *free_hdr_list;	/* Pool of available freehdr's */
	struct map *map;	/* Resource map for address space */
	caddr_t	vaddr;		/* Start address of mapped region */
	u_int updates;		/* Number of updates to free tree */
	u_int retries;		/* # of retries on interrupts */
	u_int failures;		/* # of failed allocations */
	u_int heap_size;	/* Total bytes in heap */
	u_int allocated;	/* # bytes currently allocated */
	u_int free_size;	/* # bytes in free tree */
	u_int hdr_size;		/* # bytes allocated for freehdrs */
	int keep_pages;		/* Keep whole pages in free tree? */
	u_int alloc_size;	/* Minimum allocation unit */
	struct need_to_free {	/* XXX sizeof must be <= ALIGN */
		struct need_to_free *next;
		u_int nbytes;
	} *need_to_free_list;
} kmem_info;

/*
 * Initialize kernel memory allocator
 */
kmem_init()
{
	kmem_info.free_root = NULL;
	kmem_info.free_hdr_list = NULL;
	kmem_info.map = heapmap;
	kmem_info.vaddr = Heapbase;
	kmem_info.updates = 0;
	kmem_info.retries = 0;
	kmem_info.failures = 0;
	kmem_info.heap_size = 0;
	kmem_info.allocated = 0;
	kmem_info.free_size = 0;
	kmem_info.hdr_size = 0;
	kmem_info.keep_pages = 0;
	kmem_info.alloc_size = KMEM_ALLOCUNIT;
	kmem_info.need_to_free_list = NULL;
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

static void
insert(p, len, tree, newhdr)
	register caddr_t p;		/* Ptr to the block to insert */
	register u_int len;		/* Length of new node */
	register struct freehdr **tree;	/* Address of ptr to root */
	struct freehdr *newhdr;
{
	register struct freehdr *x;
	register struct freehdr **left_hook;	/* Temp for insertion */
	register struct freehdr **right_hook;	/* Temp for insertion */

	kmem_info.updates++;
	x = *tree;
	/*
	 * Search for the first node which has a weight less
	 *	than that of the new node; this will be the
	 *	point at which we insert the new node.
	 */

	while (weight(x) >= len) {
		tree = (p < x->block) ? &x->left : &x->right;
		x = *tree;
	}

	/*
	 * Perform root insertion. The variable x traces a path through
	 *	the tree, and with the help of left_hook and right_hook,
	 *	rewrites all links that cross the territory occupied
	 *	by p.  Note that this improves performance under
	 *	paging.
	 */
	*tree = newhdr;
	left_hook = &newhdr->left;
	right_hook = &newhdr->right;

	newhdr->left = NULL;
	newhdr->right = NULL;
	newhdr->block = p;
	newhdr->size = len;

	while (x != NULL) {
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
		}
	}

	*left_hook = *right_hook = NULL;	/* clear remaining hooks */

} /* insert */


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
static void
delete(p)
	register struct freehdr **p;
{
	register struct freehdr *x;
	register struct freehdr *left_branch;	/* left & right subtrees */
	register struct freehdr *right_branch;	/* ... of deleted node */

	kmem_info.updates++;
	x = *p;
	left_branch = x->left;
	right_branch = x->right;

	while (left_branch != right_branch) {
		/*
		 * iterate until left branch and right branch are
		 * both NULL.
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
		}
	}
	*p = NULL;
	freehdr(x);
}


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
static void
demote(p)
	register struct freehdr **p;
{
	register struct freehdr *x;	/* addr of node to be demoted */
	register struct freehdr *left_branch;
	register struct freehdr *right_branch;
	register u_int    wx;

	kmem_info.updates++;
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
		}
	}

	*p = x;				/* attach demoted node here */
	x->left = left_branch;
	x->right = right_branch;
}

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
#ifdef RECORD_USAGE
caddr_t	_kmem_alloc();
caddr_t kmem_ralloc();

/*
 * Per-block recorded information
 */
struct blockrec {
	caddr_t	addr;			/* Address of block */
	u_int	size;			/* Size of block */
	u_int	intpri;			/* Interrupt priority */
	struct caller_rec *caller_info;	/* Summary for calling function */
	struct blockrec *next, *last;	/* Links for hash list */
};

/*
 * Per-caller record
 */
struct caller_rec {
	struct caller_rec *next, *prev;
	func_t	caller;		/* return address to caller */
	u_int	amount;		/* # of bytes held by caller */
	u_int	total;		/* total bytes requested by caller */
	u_int	calls;		/* number of requests */
	u_int	last;		/* size of last request */
	u_int	intr;		/* # of requests at interrupt time */
	u_int	blocks;		/* # of blocks held by caller */
	u_int	peak;		/* peak # of bytes held by caller */
};

/*
 * list of lumps of blockrecs for kmemstat -b to sift through
 */
struct blockrec_rec {
	struct blockrec_rec *next;
	struct blockrec *blockrecs;
	u_int	count;
};

static struct blockrec_rec *kmem_log_blockrecs;
static void record_alloc(), record_free();
static struct blockrec *get_blockrec();
static void free_blockrec();
static struct caller_rec *alloc_summary();
static void free_summary();

#define	KMEM_HASHSZ	256
#define	KMEM_HASH(addr)	((u_long) (addr) >> 3 & (KMEM_HASHSZ - 1))

#define	BLOCKREC_MIN	8
#define	BLOCKREC_ALLOC	PAGESIZE
#define	BLOCKREC_INCR	(BLOCKREC_ALLOC / sizeof (struct blockrec))

static struct blockrec *kmem_blocktab[KMEM_HASHSZ];
static struct blockrec *blockrec_freelist;
static int blockrec_freecount, blockrec_min = BLOCKREC_MIN;

#if defined(sparc)
#define	kmempri(s)	(((s) & PSR_PIL) >> 8)
#elif defined(mc68000)
#define	kmempri(s)	(((s) & SR_INTPRI) >> 8)
#else
#define	kmempri(s)	(s)
#endif

static void
record_alloc(block, size, caller_addr, nosleep)
	caddr_t block;
	u_int size;
	func_t caller_addr;
	int nosleep;
{
	register struct blockrec *br, *h;
	register int s, is_intr;

	br = get_blockrec(nosleep);
	if (br == NULL) {
		printf(
	"kmem_alloc: can't record block info for block (%x, %d), caller %x\n",
			block, size, caller_addr);
		panic("kmem_alloc");
	}
	br->addr = block;
	br->size = size;
	br->last = NULL;
	s = splhigh();
	h = kmem_blocktab[KMEM_HASH(block)];
	br->next = h;
	if (h != NULL)
		h->last = br;
	kmem_blocktab[KMEM_HASH(block)] = br;
	(void) splx(s);
	is_intr = servicing_interrupt();
	br->intpri = is_intr ? kmempri(splr(0)) : 0;
	br->caller_info = alloc_summary(size, caller_addr, is_intr, nosleep);
	if (br->caller_info == NULL) {
		printf(
	"kmem_alloc: can't record caller info for block (%x, %d), caller %x\n",
			block, size, caller_addr);
	}
}

static void
record_free(block, size)
	caddr_t block;
	u_int size;
{
	register struct blockrec *b;
	register int s;

	s = splhigh();
	b = kmem_blocktab[KMEM_HASH(block)];
	while (b != NULL && (b->addr != block || b->size != size))
		b = b->next;

	if (b == NULL) {
		printf("kmem_free: no record for block (%x, %d)\n",
			block, size);
		return;
	}

	if (b->next != NULL)
		b->next->last = b->last;
	if (b->last != NULL)
		b->last->next = b->next;
	else
		kmem_blocktab[KMEM_HASH(block)] = b->next;
	(void) splx(s);

	if (b->caller_info == NULL) {
		printf("kmem_free: no caller info for block (%x, %d)\n",
			b->addr, b->size);
	} else
		free_summary(b->caller_info, size);
	free_blockrec(b);
}

static struct blockrec *
get_blockrec(nosleep)
	int nosleep;
{
	register struct blockrec *b;
	register int s, n;

	s = splhigh();
	if (blockrec_freecount <= blockrec_min) {
		b = (struct blockrec *)getpages(btop(BLOCKREC_ALLOC), nosleep);
		if (b != NULL) {
			struct blockrec_rec *brr;
			brr = (struct blockrec_rec *)kernelmap_alloc(
				sizeof(struct blockrec_rec), nosleep);
			brr->blockrecs = b;
			brr->count = BLOCKREC_INCR;
			brr->next = kmem_log_blockrecs;
			kmem_log_blockrecs = brr;
			kmem_info.hdr_size += BLOCKREC_ALLOC;
			n = BLOCKREC_INCR;
			blockrec_freecount += n;
			while (n-- != 0) {
				b->addr = 0;	/* zero means invalid */
				b->next = blockrec_freelist;
				blockrec_freelist = b;
				b++;
			}
		}
	}
	b = blockrec_freelist;
	if (b != NULL) {
		blockrec_freecount--;
		blockrec_freelist = b->next;
	}
	(void) splx(s);
	return (b);
}

static void
free_blockrec(b)
	struct blockrec *b;
{
	register int s;

	s = splhigh();
	b->addr = 0;	/* zero means invalid */
	b->next = blockrec_freelist;
	blockrec_freelist = b;
	blockrec_freecount++;
	(void) splx(s);
}

struct caller_rec kmem_log_head = {
	&kmem_log_head, &kmem_log_head
};

static struct caller_rec *
alloc_summary(size, caller_addr, is_intr, nosleep)
	register func_t caller_addr;
	register u_int size;
	int is_intr, nosleep;
{
	register struct caller_rec *cr;
	register int s;

	s = splhigh();

	cr = kmem_log_head.next;
	while (cr != &kmem_log_head && cr->caller != caller_addr)
		cr = cr->next;

	if (cr->caller == caller_addr) {
		cr->prev->next = cr->next;
		cr->next->prev = cr->prev;
	} else {
		cr = (struct caller_rec *)
			_kmem_alloc(sizeof (struct caller_rec), nosleep);
		if (cr == NULL) {
			printf("alloc_summary: can't allocate record for %x\n",
				caller_addr);
			(void) splx(s);
			return (NULL);
		}
		bzero((caddr_t) cr, sizeof (*cr));
		cr->caller = caller_addr;
		cr->last = size;
	}
	cr->next = kmem_log_head.next;
	cr->prev = &kmem_log_head;
	kmem_log_head.next->prev = cr;
	kmem_log_head.next = cr;
	(void) splx(s);

	cr->amount += size;
	cr->peak = MAX(cr->peak, cr->amount);
	cr->total += size;
	cr->calls++;
	cr->blocks++;
	cr->intr += is_intr;
	return (cr);
}

static void
free_summary(cr, size)
	register struct caller_rec *cr;
	register u_int size;
{
	register int s;

	s = splhigh();
	cr->prev->next = cr->next;
	cr->next->prev = cr->prev;
	cr->amount -= size;
	cr->blocks--;
	cr->next = kmem_log_head.next;
	cr->prev = &kmem_log_head;
	kmem_log_head.next->prev = cr;
	kmem_log_head.next = cr;
	(void) splx(s);
}

static caddr_t
kmem_ralloc(nbytes, nosleep, caller_addr)
	u_int nbytes;
	int nosleep;
	func_t caller_addr;
{
	register caddr_t block;

	block = _kmem_alloc(nbytes, nosleep);
	if (block != NULL)
		record_alloc(block, nbytes, caller_addr, nosleep);
	return (block);
}

caddr_t
kmem_alloc(nbytes)
	register u_int nbytes;
{
	return (kmem_ralloc(nbytes, servicing_interrupt(), caller()));
}

caddr_t
new_kmem_alloc(nbytes, nosleep)
	u_int nbytes;
	int nosleep;
{
	return (kmem_ralloc(nbytes, nosleep, caller()));
}

caddr_t _kmem_free();

kmem_free(block, nbytes)
	register caddr_t block;
	register u_int nbytes;
{
	if (_kmem_free(block, nbytes) != NULL)
		record_free(block, nbytes);
}
#else	!RECORD_USAGE

#define	_kmem_alloc	new_kmem_alloc
#define	_kmem_free	kmem_free

caddr_t _kmem_alloc();

caddr_t
kmem_alloc(nbytes)
	register u_int nbytes;
{
	return (_kmem_alloc(nbytes, servicing_interrupt()));
}
#endif	RECORD_USAGE

caddr_t
_kmem_alloc(nbytes, nosleep)
	register u_int nbytes;
	int nosleep;
{
	register struct freehdr *a;	/* ptr to node to be allocated */
	register struct freehdr *lchild, *rchild; /* Children of a */
	register struct freehdr **p;	/* address of ptr to node */
	register u_int lwt, rwt;
	register caddr_t retblock;	/* Address returned to the user */
	u_int updates;
	int s;

	if (nbytes == 0) {
		return (NULL);
	}
	nbytes = roundup(nbytes, ALIGN);

again:
	if (!nosleep && kmem_info.need_to_free_list != NULL)
		check_need_to_free();

	updates = kmem_info.updates;

	/*
	 * ensure that at least one block is big enough to satisfy
	 *	the request.
	 */

	if (weight(kmem_info.free_root) < nbytes) {
		/*
		 * the largest block is not enough.
		 */
		if (!morecore(nbytes, nosleep)) {
			kmem_info.failures++;
			return (NULL);
		}
		goto again;
	}

	/*
	 * search down through the tree until a suitable block is
	 *	found.  At each decision point, select the better
	 *	fitting node.
	 */

	p = (struct freehdr **) &kmem_info.free_root;
	a = *p;
	if (a == NULL) {
		kmem_info.retries++;
		goto again;
	}
	lchild = a->left;
	rchild = a->right;
	lwt = weight(lchild);
	rwt = weight(rchild);

	while ((lwt >= nbytes || rwt >= nbytes) &&
		updates == kmem_info.updates) {
		p = (nbytes <= lwt && (nbytes >= rwt || lwt <= rwt))
		    ? &a->left : &a->right;
		a = *p;
		if (a == NULL)
			break;
		lchild = a->left;
		rchild = a->right;
		lwt = weight(lchild);
		rwt = weight(rchild);
	}

	/*
	 * allocate storage from the selected node.
	 */
	s = splhigh();
	if (updates != kmem_info.updates) {
		kmem_info.retries++;
		(void) splx(s);
		goto again;
	}
	retblock = a->block;

	if (retblock == NULL) {
		printf("kmem_alloc: returning NULL, nbytes = %x\n", nbytes);
		panic("kmem_alloc");
	}

	if (((long) retblock & ALIGNMASK) != 0) {
		printf("kmem_alloc: misaligned block %x\n", retblock);
		panic("kmem_alloc");
	}

	if (a->size == nbytes)
		delete(p);
	else {
		/*
		 * split the node, allocating nbytes from the top.
		 *	Remember we've already accounted for the
		 *	allocated node's header space.
		 */

		a->block += nbytes;
		a->size -= nbytes;
		/*
		 * the node pointed to by *p has become smaller;
		 *	move it down to its appropriate place in
		 *	the tree.
		 */
		demote(p);
	}
	kmem_info.allocated += nbytes;
	kmem_info.free_size -= nbytes;
	(void) splx (s);
	return (retblock);
}

/*
 * Return nbytes worth of zeroed out memory.
 */
caddr_t
kmem_zalloc(nbytes)
	u_int nbytes;
{
	register caddr_t res;

#ifdef	RECORD_USAGE
	res = kmem_ralloc(nbytes, servicing_interrupt(), caller());
#else !RECORD_USAGE
	res = _kmem_alloc(nbytes, servicing_interrupt());
#endif RECORD_USAGE
	if (res != NULL)
		bzero(res, nbytes);
	return (res);
}

caddr_t
new_kmem_zalloc(nbytes, nosleep)
	u_int nbytes;
	int nosleep;
{
	register caddr_t res;

#ifdef	RECORD_USAGE
	res = kmem_ralloc(nbytes, nosleep, caller());
#else !RECORD_USAGE
	res = _kmem_alloc(nbytes, nosleep);
#endif RECORD_USAGE
	if (res != NULL)
		bzero(res, nbytes);
	return (res);
}

/*
 * Reduce the size of an allocated block.  This is currently done by
 * allocating a new block and copying
 */
caddr_t
kmem_resize(ptr, offset, newsize, oldsize)
	caddr_t ptr;
	u_int offset, newsize, oldsize;
{
	register caddr_t res;
#ifdef RECORD_USAGE
	res = kmem_ralloc(newsize, servicing_interrupt(), caller());
#else !RECORD_USAGE
	res = _kmem_alloc(newsize, servicing_interrupt());
#endif RECORD_USAGE
	bcopy(ptr + offset, res, newsize);
	kmem_free(ptr, oldsize);
	return (res);
}

caddr_t
new_kmem_resize(ptr, offset, newsize, oldsize, nosleep)
	caddr_t ptr;
	u_int offset, newsize, oldsize;
	int nosleep;
{
	register caddr_t res;
#ifdef RECORD_USAGE
	res = kmem_ralloc(newsize, nosleep, caller());
#else !RECORD_USAGE
	res = _kmem_alloc(newsize, nosleep);
#endif RECORD_USAGE
	bcopy(ptr + offset, res, newsize);
	kmem_free(ptr, oldsize);
	return (res);
}

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
#ifdef RECORD_USAGE
caddr_t
_kmem_free(ptr, nbytes)
#else
kmem_free(ptr, nbytes)
#endif
	caddr_t ptr;
	u_int nbytes;			/* Size of node to be released */
{
	register caddr_t p;
	register struct freehdr **np;	/* For deletion from free list */
	register struct freehdr *neighbor; /* Node to be coalesced */
	register char	*neigh_block;	/* Ptr to potential neighbor */
	register u_int	neigh_size;	/* Size of potential neighbor */
	register u_int	n;		/* Number of bytes to free */
	struct freehdr *nhdr;
	u_int kstart, kend;
	u_int xstart, xend;
	u_int updates;
	int s;
	extern char strbcwait, strbcflag;	/* XXX: see below */

	nbytes = roundup(nbytes, ALIGN);
	if (nbytes == 0)
#ifdef RECORD_USAGE
		return (NULL);
#else
		return;
#endif
	/*
	 * check bounds of pointer.
	 */
	if (ptr < Heapbase || (ptr + nbytes) > Heaplimit) {
		printf("kmem_free: illegal pointer %x\n", ptr);
		panic("kmem_free");
	}
	if (((int) ptr & ALIGNMASK) != 0) {
		printf("kmem_free: misaligned pointer %x\n", ptr);
		panic("kmem_free");
	}
#ifdef KMEMZERO
	bzero(ptr, nbytes);
#endif KMEMZERO

	nhdr = getfreehdr();
	if (nhdr == NULL) {
		kmem_free_intr(ptr, nbytes);
#ifdef RECORD_USAGE
		return (NULL);
#else
		return;
#endif
	}

	/*
	 * Search the tree for the correct insertion point for this
	 * node, coalescing adjacent free blocks along the way.
	 */
	p = ptr;
	n = nbytes;
again:
	updates = kmem_info.updates;
	np = &kmem_info.free_root;
	neighbor = *np;
	while (neighbor != NULL) {
		s = splhigh();
		neigh_block = neighbor->block;
		neigh_size = neighbor->size;
		(void) splx(s);
		if (neigh_block == NULL) {
			kmem_info.retries++;
			goto again;
		}
		if (p < neigh_block) {
			if (p + n == neigh_block) {
				s = splhigh();
				if (updates != kmem_info.updates) {
					kmem_info.retries++;
					(void) splx(s);
					goto again;
				}
				delete(np);
				n += neigh_size;
				updates = kmem_info.updates;
				(void) splx(s);
			} else if (p + n > neigh_block) {
				printf(
			"kmem_free: free block overlap %x+%d over %x\n",
				    p, n, neigh_block);
				panic("kmem_free: free block overlap");
			} else
				np = &neighbor->left;
		} else if (p > neigh_block) {
			if (neigh_block + neigh_size == p) {
				s = splhigh();
				if (updates != kmem_info.updates) {
					kmem_info.retries++;
					(void) splx(s);
					goto again;
				}
				delete(np);
				p = neigh_block;
				n += neigh_size;
				updates = kmem_info.updates;
				(void) splx(s);
			} else if (neigh_block + neigh_size > p) {
				printf(
			"kmem_free: free block overlap %x+%d over %x\n",
				    neigh_block, neigh_size, p);
				panic("kmem_free: free block overlap");
			} else
				np = &neighbor->right;
		} else {
			panic("kmem_free: block already free");
		}
		neighbor = *np;
	}

	/*
	 * Insert the new node into the free space tree
	 */
	s = splhigh();
	if (updates != kmem_info.updates) {
		kmem_info.retries++;
		(void) splx(s);
		goto again;
	}

	kmem_info.allocated -= nbytes;
	kmem_info.free_size += nbytes;

	kstart = ((u_int) p + PAGEOFFSET) & PAGEMASK;
	kend   = ((u_int) p + n) & PAGEMASK;
	xstart = kstart - (u_int) p;
	xend = (u_int) p + n - kend;

	if (!kmem_info.keep_pages && kstart < kend &&
	    kend - kstart >= kmem_info.alloc_size) {
		if (xstart != 0) {
			insert(p, xstart, &kmem_info.free_root, nhdr);
			p = (caddr_t) kstart;
			n -= xstart;
		}
		if (xend != 0) {
			if (xstart != 0)
				nhdr = getfreehdr();
			if (nhdr != NULL) {
				insert((caddr_t) kend, xend,
				    &kmem_info.free_root, nhdr);
				n -= xend;
			} else
				kmem_free_intr((caddr_t) kend, xend);
		} else if (xstart == 0) {
			freehdr(nhdr);
		}
		freepages(p, (u_int) btop(n));
		kmem_info.free_size -= n;
	} else {
		insert(p, n, &kmem_info.free_root, nhdr);
	}
	(void) splx(s);

	/*
	 * Wake up the streams code if necessary.
	 *
	 * XXX:	need more elegant way of doing this; the allocator shouldn't
	 *	know anything about its clients.
	 */
	if (strbcwait && !strbcflag) {
		setqsched();
		strbcflag = 1;
	}
#ifdef RECORD_USAGE
	return (ptr);
#endif
}

/*
 * We maintain a list of blocks that need to be freed.
 * This is because we don't want to spl the relatively long
 * routines malloc and free, but we need to be able to free
 * space at interrupt level.
 */
kmem_free_intr(ptr, nbytes)
	caddr_t ptr;
	u_int nbytes;	/* Size of node to be released */
{
	register struct need_to_free *ntf;
	register int s;

	if (nbytes == 0)
		return;
	if (roundup(nbytes, ALIGN) < sizeof (struct need_to_free)) {
		printf("kmem_free_intr: freeing %d bytes\n", nbytes);
		panic("kmem_free_intr");
	}
	if (((int) ptr & ALIGNMASK) != 0) {
		printf("kmem_free_intr: misaligned pointer %x\n", ptr);
		panic("kmem_free_intr");
	}

	ntf = (struct need_to_free *)ptr;
	ntf->nbytes = nbytes;
	s = splhigh();
	ntf->next = kmem_info.need_to_free_list;
	kmem_info.need_to_free_list = ntf;
	(void) splx(s);
	return;
}

static void
check_need_to_free()
{
	register struct need_to_free *ntf;
	register int s;

	for (;;) {
		s = splhigh();
		ntf = kmem_info.need_to_free_list;
		if (ntf != NULL)
			kmem_info.need_to_free_list = ntf->next;
		(void) splx(s);
		if (ntf == NULL)
			break;
		kmem_free((caddr_t) ntf, ntf->nbytes);
	}
}

/*
 * Add a block of at least nbytes to the free space tree.
 *
 * return value:
 *	true	if at least nbytes can be allocated
 *	false	otherwise
 *
 * remark:
 *	free space is extended by an amount determined by rounding nbytes
 *	up to a multiple of the system page size.
 */
static int
morecore(nbytes, nosleep)
	u_int nbytes;
	int nosleep;
{
	register caddr_t p;
	register int s;

	s = splhigh();
	if (nosleep && kmem_info.free_hdr_list == NULL &&
	    freehdr_alloc(nosleep) == NULL) {
		(void) splx(s);
		return (0);
	}

	nbytes = roundup(nbytes, kmem_info.alloc_size);
	p = getpages(btop(nbytes), nosleep);
	if (p != 0) {
		kmem_info.allocated += nbytes;
		kmem_info.keep_pages++;
#ifdef RECORD_USAGE
		(void) _kmem_free((caddr_t)p, nbytes);
#else
		kmem_free((caddr_t)p, nbytes);
#endif
		kmem_info.keep_pages--;
	}
	(void) splx(s);
	return (p != 0);
}

/*
 * simple kmem_alloc() hardwired to use kernelmap/Sysbase/kseg,
 * and w/o any way to free the memory once it is allocated.
 * Invented  for ethernet drivers which need to grab and keep
 * lumps of bytes from kernelmap.  Also used by fast_kmem_alloc().
 *
 * Allocates pages from kernemap, and hands out bytes from them.
 * Remembers if there are extra bytes left over, and satisfies
 * future requests with them.  If the future requests can't be
 * filled from the previous leftovers, then the leftovers are
 * discarded and new page[s] allocated.
 * 
 * Total allocated bytes and discarded leftover bytes are recorded
 * in kernelmap_signal and kernelmap_noise respectively.  If the S/N
 * is lower than 10, then we're wasting >10% and need to make this
 * allocator smarter about not wasting fragments.
 */
static u_int kernelmap_signal;
static u_int kernelmap_noise;

caddr_t
kernelmap_alloc(nbytes, nosleep)
	u_int nbytes;
	int nosleep;
{
	register caddr_t va;
	register u_long x;
	register int s;
	u_int npages;
	static u_int n_leftovers;
	static caddr_t p_leftovers;

	if (nbytes == 0) {
		return (NULL);
	}
	nbytes = roundup(nbytes, ALIGN);

	s = splhigh();
	if(nbytes <= n_leftovers)
	  {
	    va = p_leftovers;
	    p_leftovers = p_leftovers + nbytes;
	    n_leftovers -= nbytes;
	    (void) splx(s);
	    return(va);
	  }

	/*
	 * else nbytes > n_leftovers
	 * get enough new pages to satisfy request.
	 */
	npages = btopr(nbytes);
	while ((x = rmalloc(kernelmap, (long)npages)) == 0) {
		if (nosleep) {
			(void) splx(s);
			return (NULL);
		}
		mapwant(kernelmap)++;
		(void) sleep((caddr_t)kernelmap, PZERO - 1);
	}
	va = Sysbase + ptob(x);
	if (segkmem_alloc(&kseg, (addr_t)va, ptob(npages),
	    !nosleep, SEGKMEM_HEAP) == 0) {
		if (nosleep) {
			rmfree(kernelmap, (long)npages, x);
			(void) splx(s);
			return (NULL);
		}
		panic("getpages: segkmem_alloc failed");
	}
	/*
	 * if new chunk of memory is adjacent to old, then
	 * coalesce them and use previous leftovers, else
	 * discard previous leftovers.
	 */
	if(va == p_leftovers + n_leftovers)
	  {
	    va = p_leftovers;
	    p_leftovers = p_leftovers + nbytes;
	    n_leftovers = n_leftovers + ptob(npages) - nbytes;
	  }
	else
	  {
	    kernelmap_noise += n_leftovers;
	    n_leftovers = ptob(npages) - nbytes;
	    p_leftovers = va + nbytes;
	  }
	kernelmap_signal += nbytes;
	(void) splx(s);
	return (va);
}

/*
 * Return nbytes worth of zeroed out memory from kernelmap
 */

caddr_t
kernelmap_zalloc(nbytes, nosleep)
	u_int nbytes;
	int nosleep;
{
	register caddr_t res;

	res = kernelmap_alloc(nbytes, nosleep);
	if (res != NULL)
		bzero(res, nbytes);
	return (res);
}

/*
 * get npages pages from the system
 */
caddr_t
getpages(npages, nosleep)
	u_int npages;
	int nosleep;
{
	register caddr_t va;
	register u_long x;
	register int s;

	s = splhigh();
	while ((x = rmalloc(kmem_info.map, (long)npages)) == 0) {
		if (nosleep) {
			(void) splx(s);
			return (NULL);
		}
		mapwant(kmem_info.map)++;
		(void) sleep((caddr_t)kmem_info.map, PZERO - 1);
	}
	va = kmem_info.vaddr + ptob(x);
	if (segkmem_alloc(&bufheapseg, (addr_t)va, ptob(npages),
	    !nosleep, SEGKMEM_HEAP) == 0) {
		if (nosleep) {
			rmfree(kmem_info.map, (long)npages, x);
			(void) splx(s);
			return (NULL);
		}
		panic("getpages: segkmem_alloc failed");
	}
	kmem_info.heap_size += ptob(npages);
	(void) splx(s);
	return (va);
}

void
freepages(pages, npages)
	caddr_t	pages;
	u_int	npages;
{
	register u_int nbytes = ptob(npages);
	register int s;

	s = splhigh();
	segkmem_free(&bufheapseg, (addr_t)pages, nbytes);
	rmfree(kmem_info.map, (long)npages,
	    (u_long)btop(pages - kmem_info.vaddr));
	kmem_info.heap_size -= nbytes;
	(void) splx(s);
}

/*
 * Get a free block header
 * There is a list of available free block headers.
 * When the list is empty, allocate another pageful.
 */
static struct freehdr *
getfreehdr()
{
	register struct freehdr *r;
	register int s;

	s = splhigh();
	if (kmem_info.free_hdr_list == NULL &&
	    freehdr_alloc(servicing_interrupt()) == NULL) {
		(void) splx(s);
		return (NULL);
	}
	r = kmem_info.free_hdr_list;
	kmem_info.free_hdr_list = kmem_info.free_hdr_list->left;
	(void) splx(s);
	return (r);
}

/*
 * Allocate a page of freehdr's
 * Must be called at splhigh()!
 */
static struct freehdr *
freehdr_alloc(nosleep)
	int nosleep;
{
	register struct freehdr *r;
	register int n;

	r = (struct freehdr *) getpages(btop(PAGESIZE), nosleep);
	if (r != NULL) {
		kmem_info.hdr_size += PAGESIZE;
		for (n = 0; n < PAGESIZE / sizeof (*r); n++) {
			freehdr(&r[n]);
		}
	}
	return (r);
}

/*
 * Free a free block header
 * Add it to the list of available headers.
 */
static void
freehdr(p)
	struct freehdr *p;
{
	p->left = kmem_info.free_hdr_list;
	p->right = NULL;
	p->block = NULL;
	kmem_info.free_hdr_list = p;
}

/*
 * These routines are for quickly allocating and freeing memory in
 * some commonly used size.  The chunks argument is used to reduce the
 * number of calls to kmem_alloc, and to reduce memory fragmentation.
 * The base argument is a caller allocated caddr_t * which is the base
 * of the free list of pieces of memory.  None of this memory is ever
 * freed, so these routines should be used only for structures that
 * will be reused often.  These routines can only be called at process
 * level.
 */

caddr_t
kmem_fast_alloc(base, size, chunks)
	caddr_t *base;	/* holds linked list of freed items */
	int	size;	/* size of each item - constant for given base */
	int	chunks;	/* number of items to alloc when needed */
{
	register u_int total_size = size * chunks;
	register caddr_t p;
	register int s;

retry:
	if (*base == 0) {	/* no free chunks */
		p = kernelmap_alloc(total_size, servicing_interrupt());
		if (p == NULL)
			return (NULL);

		while (--chunks >= 0) {
			kmem_fast_free(base, p + chunks * size);
		}
	}
	s = splhigh();
	p = *base;
	if (p == NULL && !servicing_interrupt()) {
		(void)splx(s);
		goto retry;
	}
	*base = *(caddr_t *)p;
	(void)splx(s);
	return (p);
}

caddr_t
new_kmem_fast_alloc(base, size, chunks, nosleep)
	caddr_t *base;	/* holds linked list of freed items */
	int	size;	/* size of each item - constant for given base */
	int	chunks;	/* number of items to alloc when needed */
	int	nosleep;
{
	register u_int total_size = size * chunks;
	register caddr_t p;
	register int s;

retry:
	if (*base == NULL) {	/* no free chunks */
		p = kernelmap_alloc(total_size, nosleep);
		if (p == NULL)
			return (NULL);

		while (--chunks >= 0) {
			kmem_fast_free(base, p + chunks * size);
		}
	}
	s = splhigh();
	p = *base;
	if (p == NULL && !nosleep) {
		(void)splx(s);
		goto retry;
	}
	*base = *(caddr_t *)p;
	(void)splx(s);
	return (p);
}


kmem_fast_free(base, p)
	caddr_t *base, p;
{
	register int s = splhigh();
	*(caddr_t *)p = *base;
	*base = p;
	(void)splx(s);
}

/*
 * Like kmem_fast_alloc, but use kmem_zalloc instead of
 * kmem_alloc to bzero the memory upon allocation.
 */
caddr_t
kmem_fast_zalloc(base, size, chunks)
	caddr_t *base;	/* holds linked list of freed items */
	int	size;	/* size of each item - constant for given base */
	int	chunks;	/* number of items to alloc when needed */
{
	register u_int total_size = size * chunks;
	register caddr_t p;
	register int s;

retry:
	if (*base == 0) {	/* no free chunks */
		p = kernelmap_alloc(total_size, servicing_interrupt());
		if (p == NULL)
			return (NULL);

		while (--chunks >= 0) {
			kmem_fast_free(base, p + chunks * size);
		}
	}
	s = splhigh();
	p = *base;
	if (p == NULL && !servicing_interrupt()) {
		(void)splx(s);
		goto retry;
	}
	*base = *(caddr_t *)p;
	(void)splx(s);
	bzero(p, (u_int)size);
	return (p);
}

caddr_t
new_kmem_fast_zalloc(base, size, chunks, nosleep)
	caddr_t *base;	/* holds linked list of freed items */
	int	size;	/* size of each item - constant for given base */
	int	chunks;	/* number of items to alloc when needed */
	int	nosleep;
{
	register u_int total_size = size * chunks;
	register caddr_t p;
	register int s;

retry:
	if (*base == 0) {	/* no free chunks */
		p = kernelmap_alloc(total_size, nosleep);
		if (p == NULL)
			return (NULL);

		while (--chunks >= 0) {
			kmem_fast_free(base, p + chunks * size);
		}
	}
	s = splhigh();
	p = *base;
	if (p == NULL && !nosleep) {
		(void)splx(s);
		goto retry;
	}
	*base = *(caddr_t *)p;
	(void)splx(s);
	bzero(p, (u_int)size);
	return (p);
}

int
kmem_avail()
{
	register int as_avail = rm_avail(kmem_info.map);
	return (ptob(MIN(freemem, as_avail)));
}

