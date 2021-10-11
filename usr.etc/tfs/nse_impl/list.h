/*	@(#)list.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef _IMPL_LIST_H
#define _IMPL_LIST_H

#include <nse/types.h>

#define	NSE_LIST_TYPES
typedef	struct	Nse_list	*Nse_list;
typedef	struct	Nse_list	Nse_list_rec;
typedef	struct	Nse_listelem	*Nse_listelem;
typedef	struct	Nse_listelem	Nse_listelem_rec;

#include <nse/list.h>

/*
 * List element.  This is a doubly linked list that also contains
 * an opaque pointer to data.
 */
struct	Nse_listelem {
	Nse_listelem	next;		/* Forward pointer */
	Nse_listelem	prev;		/* Backward pointer */
	Nse_opaque	data;		/* Real data */
};

/*
 * A list header.  This is the handle on a list.
 */
struct	Nse_list {
	Nse_listops	ops;		/* Generic operations */
	Nse_listelem_rec anchor;	/* Starting point */
	int		nelems;		/* Number of elements */
};

#endif
