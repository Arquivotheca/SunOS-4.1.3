#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)rectlist.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * 	Overview:	Implements the interface to the data structure called 
 *			a rectlist which is a list of rectangles.
 */

#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>

#ifndef KERNEL
#include <stdio.h>

#define	RNTABLEINC	30

#else
#include <sys/param.h>

extern	caddr_t kmem_zalloc();
#define	RNTABLEINC	NBPG/(sizeof(struct rectnode))
#endif

int	rnTableIndex;
int	rnTableOverflowed;
struct	rectnode *rnTable;	/* Array of alloced but not used nodes */
struct	rectnode *rnFree;	/* List of free nodes */

/*
 * rectlist constants
 */
extern        struct rectlist rl_null;

#define	rnptr_null	(struct	rectnode *)0

struct	rectnode *_rl_getrectnode(), **_rl_removerectnode();
bool	rl_boundintersectsrect();

#ifndef KERNEL
/*
 * rectlist geometry functions
 */
bool
rl_includespoint(rl, x, y)
	register struct	rectlist *rl;
	coord	x, y;
{
	register struct	rectnode *rn;

	rl_coordoffset(rl, &x, &y);
	if (rect_includespoint(&rl->rl_bound, x, y)) 
		for (rn = rl->rl_head; rn; rn = rn->rn_next) {
			if (rect_includespoint(&rn->rn_rect, x, y))
				return (TRUE);
		}
	return (FALSE);
}
#endif !KERNEL

rl_intersection(rl1, rl2, rl)
	register struct	rectlist *rl1, *rl2, *rl;
{
	struct	rect r;
	register struct	rectnode *rn;
	struct	rectlist rltemp;
	struct	rectlist rlresult;
	rltemp = rl_null;
	rlresult = rl_null;
	
	rl_rectoffset(rl1, &rl1->rl_bound, &r);
	if (rl_boundintersectsrect(&r, rl2))
		/*
		 * For each Rect in rl1 intersect with each Rect in rl2
		 */
		for (rn = rl1->rl_head; rn; rn = rn->rn_next) {
			rl_rectoffset(rl1, &rn->rn_rect, &r);
			(void)rl_rectintersection(&r, rl2, &rltemp);
			_rl_append(&rlresult, &rltemp);
			rltemp = rl_null; /* because appRLs consumed rltemp */
		}
	(void)rl_free(rl);
	*rl = rlresult;
}

#ifndef KERNEL
rl_sort(rl1, rl, sortorder)
	register struct	rectlist *rl1, *rl;
	int	sortorder;
{
	struct	rectlist rltemp;	/* Extract 'most' rect one at a time */
	struct	rectlist rlresult; /* Put 'most' rect here as pull off rltemp */
	struct	rectnode *rnTemp;	/* Source index */
	struct	rectnode *rnResult;	/* Result index */
	struct	rectnode *rnMost;	/* Temp result index */
	struct	rect rMost, rtemp;
	rltemp = rl_null;
	rlresult = rl_null;
	
	(void)rl_copy(rl1, &rlresult);	/* Result same as input */
	if (rl1 == rl)
		rltemp = *rl;		/* Cause will overwrite anyway */
	else
		(void)rl_copy(rl1, &rltemp);	/* Don't damage input */
	for (rnResult = rlresult.rl_head; rnResult;
	    rnResult = rnResult->rn_next) {
		rnMost = NULL;
		rMost = rect_null;
		for (rnTemp = rltemp.rl_head; rnTemp; rnTemp = rnTemp->rn_next) {
			if (rect_equal(&rnTemp->rn_rect, &rect_null))
				continue; /* Already extracted this item */
			if (rnMost == NULL) { /* Something to compare with */
				rnMost = rnTemp;
				rMost = rnMost->rn_rect;
				continue;
			}
			rtemp = rnTemp->rn_rect;
			if (rect_order(&rtemp, &rMost, sortorder)==TRUE) {
				rnMost = rnTemp;
				rMost = rtemp;
			}
		}
		if (rnMost == NULL)
			break; /* sorted all items */
		rnResult->rn_rect = rMost;
		rnMost->rn_rect = rect_null; /* Don't compare this item again */
	}
	(void)rl_free(rl);
	*rl = rlresult;
}
#endif !KERNEL

rl_union(rl1, rl2, rl)
	register struct	rectlist *rl1, *rl2, *rl;
{
	if (rl1 == rl)
		_rl_union(rl, rl2);
	else if (rl2 == rl)
		_rl_union(rl, rl1);
	else {
		(void)rl_copy(rl1, rl);
		_rl_union(rl, rl2);
	}
}

rl_difference(rl1, rl2, rl)
	register struct	rectlist *rl1, *rl2, *rl;
{
	struct	rect r;
	register struct	rectnode *rn;

	(void)rl_copy(rl1, rl);
	rl_rectoffset(rl, &rl->rl_bound, &r);
	if (rl_boundintersectsrect(&r, rl2)) {
		/*
		 * For each Rect in rl2 remove from rl
		 */
		for (rn = rl2->rl_head; rn; rn = rn->rn_next) {
			rl_rectoffset(rl2, &rn->rn_rect, &r);
			_rl_removerect(&r, rl);
		}
		_rl_makebound(rl);
	}
}

bool
rl_empty(rl)
	register struct	rectlist *rl;
{
	register struct	rectnode *rn;

	if (rect_isnull(&(rl->rl_bound)))
		return (TRUE);
	for (rn = rl->rl_head; rn; rn = rn->rn_next) {
		if (!rect_isnull(&(rn->rn_rect)))
			return (FALSE);
	}
	return (TRUE);
}

bool
rl_equal(rl1, rl2)
	register struct	rectlist *rl1, *rl2;
{	register struct	rectnode *rn1, *rn2;

	if (!rect_equal(&rl1->rl_bound, &rl2->rl_bound) ||
	    rl1->rl_x!=rl2->rl_x || rl1->rl_y!=rl2->rl_y)
		return (FALSE);
	rn2 = rl2->rl_head;
	for (rn1 = rl1->rl_head; ; rn1 = rn1->rn_next) {
		if (rn1==NULL && rn2==NULL)
			return (TRUE);
		if (rn1==NULL || rn2==NULL)
			return (FALSE);
		if (!rect_equal(&rn1->rn_rect, &rn2->rn_rect))
			return (FALSE);
		rn2 = rn2->rn_next;
	}
}

#ifndef KERNEL
/*
 * rectlist with Rect geometry functions
 */
bool
rl_equalrect(r, rl)
	register struct	rect *r;
	register struct	rectlist *rl;
{
	struct	rect rtemp;

	rl_rectoffset(rl, &rl->rl_bound, &rtemp);
	if (rect_equal(r, &rtemp) && rl->rl_head == rl->rl_tail)
		return (TRUE);
	else
		return (FALSE);
}
#endif !KERNEL

bool
rl_boundintersectsrect(r, rl)
	register struct	rect *r;
	register struct	rectlist *rl;
{
	struct	rect rtemp;

	rl_rectoffset(rl, &rl->rl_bound, &rtemp);
	return (rect_intersectsrect(&rtemp, r));
}

bool
rl_rectintersects(r, rl) 
	register struct	rect *r; 
	register struct	rectlist *rl;
{
	struct	rectnode *rn;
	struct	rect r1;

	if (rl_boundintersectsrect(r, rl)) 
		/* For each Rect in rl intersect with r */
		for (rn = rl->rl_head; rn; rn = rn->rn_next) {
			rl_rectoffset(rl, &rn->rn_rect, &r1);
			if (rect_intersectsrect(r, &r1))
				return (TRUE);
		}
	return (FALSE);
}

rl_rectintersection(r, rl1, rl) 
	register struct	rect *r; 
	register struct	rectlist *rl1, *rl;
{
	struct	rectnode *rn;
	struct	rect r1, rtemp;
	struct	rectlist rlresult;
	rlresult = rl_null;

	if (rl_boundintersectsrect(r, rl1)) 
		/* For each Rect in rl1 intersect with r */
		for (rn = rl1->rl_head; rn; rn = rn->rn_next) {
			rl_rectoffset(rl1, &rn->rn_rect, &r1);
			(void)rect_intersection(r, &r1, &rtemp);
			_rl_appendrect(&rtemp, &rlresult);
		}
	(void)rl_free(rl);
	*rl = rlresult;
}

rl_rectunion(r, rl1, rl)
	register struct	rect *r; 
	register struct	rectlist *rl1, *rl;
{
	struct	rectlist rlresult;
	struct	rect rtemp;
	register struct	rectnode *rn;
	rlresult = rl_null;

	_rl_appendrect(r, &rlresult);
	if (rl_boundintersectsrect(r, rl1)) 
		for (rn = rl1->rl_head; rn; rn = rn->rn_next) {
			rl_rectoffset(rl1, &rn->rn_rect, &rtemp);
			_rl_removerect(&rtemp, &rlresult);
		}
	(void)rl_copy(rl1, rl);
	_rl_append(rl, &rlresult);
}

rl_rectdifference(r, rl1, rl)
	register struct	rect *r;
	register struct	rectlist *rl1, *rl;
{
	struct	rect rtemp;
	struct	rectlist rltemp;
	register struct	rectnode *rn;

	if (rect_isnull(r))
		(void)rl_copy(rl1, rl);
	else if (_rl_equal(rl1, &rl_null))
		(void)rl_free(rl);
	else {
		if (rl1 == rl)
			_rl_removerect(r, rl);
		else {
			(void)rl_free(rl);
			for (rn = rl1->rl_head; rn; rn = rn->rn_next) {
				rl_rectoffset(rl1,&rn->rn_rect, &rtemp);
				rltemp = rl_null;
				_rl_difrects(&rtemp, r, &rltemp);
				_rl_append(rl, &rltemp);
			}
		}
		_rl_makebound(rl);
	}
}

/*
 * rectlist initialization functions
 */

rl_initwithrect(r, rl)
	register struct	rect *r;
	struct	rectlist *rl;
{

	*rl = rl_null;
	_rl_appendrect(r, rl);
}

/*
 * rectlist List Memory Management functions
 */
rl_copy(rl1, rl)
	register struct	rectlist *rl1, *rl;
{
	register struct	rectnode *rn;
	
	if (rl1 != rl) {
		(void)rl_free(rl);
		*rl= *rl1;
		rl->rl_head = 0;
		rl->rl_tail = 0;
		for (rn = rl1->rl_head; rn; rn = rn->rn_next)
			_rl_appendrect(&rn->rn_rect, rl);
	}
}

rl_free(rl)
	register struct	rectlist *rl;
{
	register struct	rectnode *rn, *rn_next, *rn_last = (struct rectnode *)0;
	
	for (rn = rl->rl_head; rn; rn = rn_next) {
		rn_next = rn->rn_next;
		_rl_freerectnode(rn);
		rn_last = rn;
	}
	if (rn_last != rl->rl_tail) {
#ifdef	KERNEL
		(void)printf("Malformed rl in rl_free in KERNEL\n");
#else
		(void)fprintf(stderr, "Malformed rl in rl_free\n");
#endif
	}
	*rl = rl_null;
}

rl_coalesce(rl)
	register struct	rectlist *rl;
{
	struct	rectnode *rn;
	struct	rect rbound;
	register int area = 0;
	
	/*
	 * If already one rect in list then can't do any better.
	 */
	if (rl->rl_head == rl->rl_tail)
		return;
	/*
	 * Sum area in rect list
	 */
	for (rn = rl->rl_head; rn; rn = rn->rn_next) {
		area += rn->rn_rect.r_width*rn->rn_rect.r_height;
	}
	/*
	 * If area of list = area of rl_bound then make a single entry list
	 */
	rl_rectoffset(rl, &rl->rl_bound, &rbound);
	if (area == (rbound.r_width*rbound.r_height)) {
		(void)rl_free(rl);
		(void)rl_initwithrect(&rbound, rl);
	}
}

rl_normalize(rl)
	register struct	rectlist *rl;
{
	struct	rectnode *rn;
	
	if (rl->rl_x == 0 && rl->rl_y == 0)
		return;
	rl_rectoffset(rl, &rl->rl_bound, &rl->rl_bound);
	for (rn = rl->rl_head; rn; rn = rn->rn_next) {
		rl_rectoffset(rl, &rn->rn_rect, &rn->rn_rect);
	}
	rl->rl_x = 0;
	rl->rl_y = 0;
}

#ifdef notdef
/*	Debug Utilities	*/

rl_print(rl, tag)
	struct	rectlist *rl;
	char *tag;
{
	struct	rectnode *rn;

#ifdef KERNEL
	(void)printf(
#else
	(void)fprintf(stderr,
#endif
		"%s: Bounding ", tag);
	rect_print(&rl->rl_bound);
#ifdef KERNEL
	(void)printf(
#else
	(void)fprintf(stderr,
#endif
		"\n\t");
	for (rn = rl->rl_head; rn; rn = rn->rn_next) {
		rect_print(&rn->rn_rect);
#ifdef KERNEL
		(void)printf(
#else
		(void)fprintf(stderr,
#endif
			"\n\t");
	}
#ifdef KERNEL
	(void)printf(
#else
	(void)fprintf(stderr,
#endif
		"using these offsets: x=%d, y=%d \n", rl->rl_x, rl->rl_y);
}
#endif /* notdef */

/*
 * Private Routines
 */

/*
 * Create node for r and app to rl
 */
_rl_appendrect(r, rl)
	register struct	rect *r;
	register struct	rectlist *rl;
{
	register struct	rectnode *rn;
	
	if (rect_isnull(r))
		return;
	rn = _rl_getrectnode(r);

	_rl_appendrectnode(rl, rn);
}

_rl_append(rlBase, rlAdd)
	struct	rectlist *rlBase, *rlAdd;
{
	struct	rect r;
	struct	rectnode *rn, *rn_next;
	
	for (rn = rlAdd->rl_head; rn; rn = rn_next) {
		rl_rectoffset(rlAdd, &rn->rn_rect, &r);
		rl_coordoffset(rlBase, &r.r_left, &r.r_top);
		rn->rn_rect.r_left = r.r_left;
		rn->rn_rect.r_top = r.r_top;
		rn_next = rn->rn_next;
		_rl_appendrectnode(rlBase, rn); /* Modifies rn->rn_next */
	}
}

struct	rectnode *
_rl_getrectnode(r)
	register struct	rect *r;
{
	register struct	rectnode *rn;

	if (!rnFree) {
		/* free list empty, alloc from table	*/
		if (!rnTable || rnTableIndex == RNTABLEINC) { /* table empty */
#ifdef	KERNEL
			/*
			 * Alloc only scheme (no freeing yet)
			 */
			rnTable = (struct rectnode *)kmem_zalloc(
			    RNTABLEINC * sizeof (struct rectnode));
			if (rnTable == 0)
				(void)printf("Kernel: couldn't allocate rnTable!\n");
#ifdef	notdef /* Old non-dynamic stuff (RNTABLEINC was 1000) */
			if (!rnTable)
				rnTable = &rnKernTable[0];
			else
			    (void)printf("rnTable overflow in _rl_getrectnode!");
			/* Note: Add dynamic memory allocation here	*/
#endif	notdef
#else
			/*
			 * Alloc only scheme (no freeing yet)
			 */

			rnTable = (struct rectnode *) sv_calloc(
			    1, RNTABLEINC*sizeof(struct rectnode));
#endif	KERNEL
			rnTableOverflowed++;
			rnTableIndex = 0;
		}
		rn = rnTable+rnTableIndex;
		rnTableIndex++;
	} else {
		rn = rnFree;
		rnFree = rnFree->rn_next;
	}
	rn->rn_next = rnptr_null;
	rn->rn_rect = *r;
	return (rn);
}

_rl_appendrectnode(rl, rn)
	register struct	rectlist *rl;
	register struct	rectnode *rn;
{

	/* Fix up list pointers */
	if (!rl->rl_head)
		rl->rl_head = rn;
	if (rl->rl_tail)
		rl->rl_tail->rn_next = rn;
	rl->rl_tail = rn;
	rn->rn_next = 0;

	/* Fix up rl->rl_bound */
	rl->rl_bound = rect_bounding(&rn->rn_rect, &rl->rl_bound);
}

_rl_makebound(rl)
	struct	rectlist *rl;
{
	struct	rectnode *rn;

	rl->rl_bound = rect_null;
	for (rn = rl->rl_head; rn; rn = rn->rn_next)
		rl->rl_bound = rect_bounding(&rl->rl_bound, &rn->rn_rect);
	rl_coordoffset(rl, &rl->rl_bound.r_left, &rl->rl_bound.r_top);
}

_rl_difrects(r1, r2, rl)
	register struct	rect *r1, *r2;
	register struct	rectlist *rl;
	/*
	 * Assuming this r1 and r2 intersect, place r1-r2 in rl.
	 * Doesn't free rl.
	 * rl may be unaffected if r1 is completely contained in r2.
	 */
{
	struct	rect r;
	struct	rect r1L;
	r1L = *r1;

	/* Identify an included upper rect	*/
	if (r1L.r_top < r2->r_top) {
		rect_construct(&r, r1L.r_left, r1L.r_top, r1L.r_width,
		    (r2->r_top - r1L.r_top));
		_rl_appendrect(&r, rl);
		r1L.r_height -= (r2->r_top - r1L.r_top);
		r1L.r_top = r2->r_top;
	}

	/* Identify an included lower rect	*/
	if (rect_bottom(&r1L) > rect_bottom(r2)) {
		rect_construct(&r, r1L.r_left, rect_bottom(r2)+1, r1L.r_width,
		    (rect_bottom(&r1L)-rect_bottom(r2)));
		_rl_appendrect(&r, rl);
		r1L.r_height -= (rect_bottom(&r1L) - rect_bottom(r2));
	}

	/* Identify an included r_left rect	*/
	if (r1L.r_left < r2->r_left) {
		rect_construct(&r, r1L.r_left, r1L.r_top,
		    r2->r_left - r1L.r_left, r1L.r_height);
		_rl_appendrect(&r, rl);
	}

	/* Identify an included right rect	*/
	if (rect_right(&r1L) > rect_right(r2)) {
		rect_construct(&r, rect_right(r2)+1, r1L.r_top,
		    (rect_right(&r1L)-rect_right(r2)), r1L.r_height);
		_rl_appendrect(&r, rl);
	}
}

_rl_equal(rl1, rl2)
	register struct	rectlist *rl1, *rl2;
{

	if (rl1->rl_x == rl2->rl_x && rl1->rl_y == rl2->rl_y &&
	    rl1->rl_head == rl2->rl_head && rl1->rl_tail == rl2->rl_tail &&
	    rect_equal(&rl1->rl_bound, &rl2->rl_bound))
		return (TRUE);
	else
		return (FALSE);
}

_rl_freerectnode(rn)
	register struct	rectnode *rn;
{

	rn->rn_next = rnFree;
	rnFree = rn;
}

_rl_removerect(r, rl)
	register struct	rect *r;
	register struct	rectlist *rl;
{
	struct	rectnode **rnPtr, **rn_nextPtr;
	struct	rectlist rltemp;
	struct	rect rtemp;

	if (rl->rl_head == 0)
		return;
	for (rnPtr = &(rl->rl_head); *rnPtr; rnPtr = rn_nextPtr) {
		rl_rectoffset(rl, &((*rnPtr)->rn_rect), &rtemp);
		rn_nextPtr = &((*rnPtr)->rn_next);
		if (rect_intersectsrect(r, &rtemp)) {
			rltemp = rl_null;
			_rl_difrects(&rtemp, r, &rltemp);
			if (rltemp.rl_head != rnptr_null)
				_rl_replacernbyrl(rl, *rnPtr, &rltemp);
			else {
				/* Remove rn from rl	*/
				rn_nextPtr = _rl_removerectnode(rl, rnPtr);
				if (rn_nextPtr == 0)
					break;
			}
		}
	}
}

_rl_union(rlBase, rlAdd)
	register struct	rectlist *rlBase, *rlAdd;
{
	register struct	rectnode *rn;
	struct	rect r;

	for (rn = rlAdd->rl_head; rn; rn = rn->rn_next) {
		rl_rectoffset(rlAdd, &rn->rn_rect, &r);
		(void)rl_rectunion(&r, rlBase, rlBase);
	}
}

struct	rectnode **
_rl_removerectnode(rl, rnPtr)
	struct	rectlist *rl;
	struct	rectnode **rnPtr;
{
	struct	rectnode *rnAxe, **rn_nextPtr;

	if (rl->rl_head == rl->rl_tail) {
		(void)rl_free(rl);
		return (0);
	} else {
		rnAxe = *rnPtr;
		rn_nextPtr = rnPtr;
		*rn_nextPtr = rnAxe->rn_next;
		if (rl->rl_tail == rnAxe)
			rl->rl_tail = (struct rectnode *)rn_nextPtr;
			/*
			 * Note: Can get away with the above cast only because
			 * rn_next pointers are the first fields in
			 * struct rectnode and the case where *rn_nextPtr is the
			 * header is handled in the first test in this function.
			 */
		_rl_freerectnode(rnAxe);
		return (rn_nextPtr);
	}
}
		
_rl_replacernbyrl(rlBase, rnAxe, rlAdd)
	register struct	rectlist *rlBase;
	register struct	rectnode *rnAxe;
	register struct	rectlist *rlAdd;
	/*
	 * WARNING: rlBase.rl_bound remains unchanged. 
	 * If replaced data possibly invalidates rlBase.rl_bound then
	 * need to eventually explicitely bound rlBase
	 */
{
	register struct	rectnode *rn;

	/* Adjust offsets of rlAdd	*/
	if (rlAdd->rl_x != rlBase->rl_x ||
	    rlAdd->rl_y != rlBase->rl_y)
		/* Modify values of rlAdd to be compatible with rlBase offset */
		for (rn = rlAdd->rl_head; rn; rn = rn->rn_next) {
			rl_rectoffset(rlAdd, &rn->rn_rect, &rn->rn_rect);
			rl_coordoffset(rlBase,
			    &rn->rn_rect.r_left,&rn->rn_rect.r_top);
		}

	/* Insert rlAdd in place of rnAxe in rlBase */
	if ((rlBase->rl_tail == rnAxe) && (rlAdd->rl_tail != rlAdd->rl_head))
		rlBase->rl_tail = rlAdd->rl_tail;
	rlAdd->rl_tail->rn_next = rnAxe->rn_next;
	*rnAxe = *rlAdd->rl_head;
	_rl_freerectnode(rlAdd->rl_head);
}
