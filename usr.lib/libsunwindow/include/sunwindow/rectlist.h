/*	@(#)rectlist.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Defines the interface to the data structure called 
 * a rectlist which is a list of rectangles.
 *
 * A rectlist has an offset (rl_x, rl_y) assoicated with it that is used
 * to efficiently pass this data structure from one embedded coordinate
 * system to another without changing the offsets of all the rectangles in
 * the list.
 *
 * Rl_bound is the rectangle that bounds all the rectangles in the list
 * and is maintained to facilitate efficient rectangle algebra.
 */

#ifndef _rectlist_defined
#define _rectlist_defined

typedef	struct	rectnode {
	struct	rectnode *rn_next;	/* Pointer to next rectnode */
	struct	rect rn_rect;
} Rectnode;

#define	RECTNODE_NULL	((Rectnode *)0)

typedef	struct	rectlist {
	coord	rl_x, rl_y;		/* Offset to apply to each rect
					   in list including bound */
	struct	rectnode *rl_head;	/* Pointer to first rectnode */
	struct	rectnode *rl_tail;	/* Pointer to last rectnode */
	struct	rect rl_bound;		/* Describes bounding rect of 
					   all rects in list */
} Rectlist;

#define	RECTLIST_NULL	((Rectlist *)0)

extern	struct rectlist rl_null;

/*
 * Rectlist Transformation macros used for passing rectlists up/down embedded
 * coordinate systems.
 */
#define	rl_passtoparent(x,y,rl) \
	{(rl)->rl_x=(rl)->rl_x+(x); (rl)->rl_y=(rl)->rl_y+(y);}

#define	rl_passtochild(x,y,rl) \
	{(rl)->rl_x=(rl)->rl_x-(x); (rl)->rl_y=(rl)->rl_y-(y);}

/*
 * Rectlist Offset Adjustment macros
 */
#define	rl_rectoffset(rl,r1,r) \
	{*(r)= *(r1); (r)->r_left+=(rl)->rl_x; (r)->r_top+=(rl)->rl_y;}

#define	rl_coordoffset(rl,x,y) {*(x)-=(rl)->rl_x;*(y)-=(rl)->rl_y;}
	
extern	bool rl_empty(), rl_equal(), rl_boundintersectsrect(),
		rl_rectintersects();

#ifndef KERNEL
extern	bool rl_equalrect(), rl_includespoint();
#endif !KERNEL

#ifdef cplus
/*
 * C Library routine specifications related to rectlist functions.
 */

/*
 * Rectlist geometry predicates and functions
 */
bool	rl_empty(struct rectlist *rl);
bool	rl_equal(struct rectlist *rl1, *rl2);
void	rl_intersection(struct rectlist *rl1, *rl2, *rl);
void	rl_sort(struct rectlist *rl1, *rl, int sortorder);
void	rl_secondarysort(struct rectlist *rl1, *rl, int sortorder);
void	rl_union(struct rectlist *rl1, *rl2, *rl);
void	rl_difference(struct rectlist *rl1, *rl2, *rl);
void	rl_coalesce(struct rectlist rl);
#ifndef KERNEL
bool	rl_includespoint(struct rectlist *rl, coord x, y);
#endif !KERNEL

/*
 * Rectlist with Rect geometry predicates and functions
 */
#ifndef KERNEL
bool	rl_equalrect(struct rect *r, struct rectlist *rl);
#endif !KERNEL
bool	rl_boundintersectsrect(struct rect *r,  struct rectlist *rl);
bool	rl_rectintersects(struct rect *r,  struct rectlist *rl);
void	rl_rectintersection(struct rect *r, struct rectlist *rl1, *rl);
void	rl_rectunion(struct rect *r, struct rectlist *rl1, *rl);
void	rl_rectdifference(struct rect *r, struct rectlist *rl1, *rl);

/*
 * Rectlist initialization functions
 */
void	rl_initwithrect(struct rect *r, struct rectlist *rl);

/*
 * Rectlist List Memory Management functions
 */
void	rl_copy(struct rectlist *rl1, *rl);
void	rl_free(struct rectlist *rl);

/*
 * Rectlist Offset Adjustment functions
 */
void	rl_normalize(struct rectlist *rl);

#endif cplus
#endif /*!_rectlist_defined*/
