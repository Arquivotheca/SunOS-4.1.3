#ifndef lint
static char sccsid[] = "@(#)listelem.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <nse/types.h>
#include <nse/param.h>
#include "nse_impl/list.h"
#include <nse/util.h>

/*
 * Return a pointer to the next element of a list 
 */
Nse_listelem
nse_listelem_next(elem)
	Nse_listelem	elem;
{
	return elem->next;
}

/*
 * Return a pointer to the previous element of a list 
 */
Nse_listelem
nse_listelem_prev(elem)
	Nse_listelem	elem;
{
	return elem->prev;
}

/*
 * Return pointer to the data pointer of an element of a list.
 */
Nse_opaque
nse_listelem_data(elem)
	Nse_listelem	elem;
{
	return elem->data;
}

/* VARARGS1 */
/*
 * Set the data of an element of a list.
 */
void
nse_listelem_set_data(elem, data)
	Nse_listelem	elem;
	Nse_opaque	data;
{
	if (elem) {
		elem->data = data;
	}
}

/*
 * Destroy and free the memory associated with a list element.
 */
void
nse_listelem_destroy(list, elem)
	Nse_list	list;
	Nse_listelem	elem;
{
	Nse_voidfunc	destroy_func;

	destroy_func = list->ops->destroy;
	if (destroy_func != NULL) {
		destroy_func(elem->data);
	}
	NSE_DISPOSE(elem);
}

/*
 * Destroy and free the memory associated with the structure wrapped
 * around the data item of a list element.
 */
/* ARGSUSED */
void
nse_listelem_destroy_wrapper(list, elem)
	Nse_list	list;
	Nse_listelem	elem;
{
	NSE_DISPOSE(elem);
}

/*
 * Print an element of a list.
 */
void
nse_listelem_print(list, elem)
	Nse_list	list;
	Nse_listelem	elem;
{
	Nse_voidfunc	print_func;

	print_func = list->ops->print;
	if (print_func != NULL) {
		print_func(elem->data);
	}
}

/*
 * Delete an element of a list.
 */
void
nse_listelem_delete(list, elem)
	Nse_list	list;
	Nse_listelem	elem;
{
	nse_listelem_remove(list, elem);
	nse_listelem_destroy(list, elem);
}

/*
 * Remove an element of from a list and return it.
 */
Nse_listelem
nse_listelem_remove(list, elem)
	Nse_list	list;
	Nse_listelem	elem;
{
	elem->prev->next = elem->next;
	elem->next->prev = elem->prev;
	list->nelems--;
	return elem;
}

/*
 * Copy an element of a list.
 */
Nse_listelem
nse_listelem_copy(list, elem)
	Nse_list	list;
	Nse_listelem	elem;
{
	Nse_opaquefunc	copy_func;
	Nse_listelem	newelem;

	copy_func = list->ops->copy;
	newelem = NSE_NEW(Nse_listelem);
	if (copy_func != NULL) {
		newelem->data = copy_func(elem->data);
	}
	return newelem;
}

/*
 * Compare two elements of a list and return a boolean as to their
 * equality.
 */
bool_t
nse_listelem_equal(list, elem1, elem2)
	Nse_list	list;
	Nse_listelem	elem1;
	Nse_listelem	elem2;
{
	Nse_boolfunc	equal_func;
	bool_t		r;

	equal_func = list->ops->equal;
	if (equal_func != NULL) {
		r = equal_func(elem1->data, elem2->data);
	} else {
		r = FALSE;
	}
	return r;
}
