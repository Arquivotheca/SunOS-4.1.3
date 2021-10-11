/*	@(#)malloc.h 1.1 92/07/30 SMI; from include/malloc.h 1.5	*/


#ifndef	__malloc_h
#define	__malloc_h

/*
 *	Constants defining mallopt operations
 */
#define	M_MXFAST	1	/* set size of 'small blocks' */
#define	M_NLBLKS	2	/* set num of small blocks in holding block */
#define	M_GRAIN		3	/* set rounding factor for small blocks */
#define	M_KEEP		4	/* (nop) retain contents of freed blocks */

/*
 *	malloc information structure
 */
struct	mallinfo  {
	int arena;	/* total space in arena */
	int ordblks;	/* number of ordinary blocks */
	int smblks;	/* number of small blocks */
	int hblks;	/* number of holding blocks */
	int hblkhd;	/* space in holding block headers */
	int usmblks;	/* space in small blocks in use */
	int fsmblks;	/* space in free small blocks */
	int uordblks;	/* space in ordinary blocks in use */
	int fordblks;	/* space in free ordinary blocks */
	int keepcost;	/* cost of enabling keep option */

	int mxfast;	/* max size of small blocks */
	int nlblks;	/* number of small blocks in a holding block */
	int grain;	/* small block rounding factor */
	int uordbytes;	/* space (including overhead) allocated in ord. blks */
	int allocated;	/* number of ordinary blocks allocated */
	int treeoverhead;	/* bytes used in maintaining the free tree */
};

typedef	void *	malloc_t;

extern	malloc_t	calloc(/* size_t nmemb, size_t size */);
extern	void		free(/* malloc_t ptr */);
extern	malloc_t	malloc(/* size_t size */);
extern	malloc_t	realloc(/* malloc_t ptr, size_t size */);
extern	int		mallopt();
extern	struct mallinfo	mallinfo();

#endif	/* !__malloc_h */
