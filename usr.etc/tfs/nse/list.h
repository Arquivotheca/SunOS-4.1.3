/*	@(#)list.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef _NSE_LIST_H
#define _NSE_LIST_H

#include <nse/types.h>

/*
 * A set of function pointers for generic operations on lists.
 */
struct	Nse_listops {
	Nse_opaquefunc	create;		/* Create an element */
	Nse_opaquefunc	copy;		/* Copy an element */
	Nse_voidfunc	destroy;	/* Free an element */
	Nse_voidfunc	print;		/* Debugging routine */
	Nse_boolfunc	equal;		/* Comparision routine */
};

typedef	struct	Nse_listops	*Nse_listops;
typedef	struct	Nse_listops	Nse_listops_rec;

#ifdef lint
#include <nse_impl/list.h>
#endif

#ifndef NSE_LIST_TYPES
typedef Nse_opaque	Nse_list;
typedef Nse_opaque	Nse_listelem;
#endif
typedef	Nse_listelem	(*Nse_listelemfunc)();

#define NSE_LIST_ITERATE(list, type, data, listelem, end) \
	for (listelem = nse_list_first_elem(list), end = nse_list_end(list); \
	     listelem!=end && (data = (type) nse_listelem_data(listelem), 1); \
	     listelem = nse_listelem_next(listelem))

#define NSE_LIST_ITERATE_REV(list, type, data, listelem, end) \
	for (end = nse_list_end(list), listelem = nse_listelem_prev(end); \
	     listelem!=end && (data = (type) nse_listelem_data(listelem), 1); \
	     listelem = nse_listelem_prev(listelem))

/*
 * Functions that operate on a list.
 */
Nse_listelem	nse_list_create_elem();
Nse_listelem	nse_list_add_new_data();
Nse_listelem	nse_list_add_new_copy();
Nse_opaque	nse_list_add_new_elem();
Nse_list	nse_list_copy();
void		nse_list_clear();
void		nse_list_destroy();
void		nse_list_destroy_wrapper();
void		nse_list_print();
Nse_listelem	nse_list_first_elem();
Nse_listelem	nse_list_end();
void		nse_list_iterate();
Nse_opaque	nse_list_iterate_or();
Nse_listelem	nse_list_find_elem();
Nse_opaque	nse_list_search();
Nse_listelem	nse_list_search_elem();
void		nse_list_insert_head();
void		nse_list_insert_tail();
void		nse_list_insert_before();
void		nse_list_insert_after();
int		nse_list_nelems();
Nse_list	nse_list_append();
void		nse_list_subset();
bool_t		nse_list_cmp();
Nse_list	nse_list_diff();
Nse_list	nse_list_union();
Nse_list	nse_list_intersection();
Nse_list	nse_list_add();
void		nse_list_sort();
Nse_list	nse_list_alloc();
void		nse_list_print_stats();
void		nse_list_print_cnt_stats();

/*
 * Functions that operate on a listelem
 */
Nse_listelem	nse_listelem_next();
Nse_listelem	nse_listelem_prev();
Nse_opaque	nse_listelem_data();
void		nse_listelem_set_data();
void		nse_listelem_destroy();
void		nse_listelem_destroy_wrapper();
void		nse_listelem_print();
void		nse_listelem_delete();
Nse_listelem	nse_listelem_remove();
Nse_listelem	nse_listelem_copy();
bool_t		nse_listelem_equal();

#endif
