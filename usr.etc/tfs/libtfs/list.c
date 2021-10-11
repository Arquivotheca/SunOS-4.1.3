#ifndef lint
static char sccsid[] = "@(#)list.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <nse/types.h>
#include <nse/param.h>
#include "nse_impl/list.h"
#include <nse/util.h>

static	Nse_list	alloc_listhdr();

/*
 * For stat gathering.
 */
static  int             n_create;
static  int             n_copy;
static  int             n_destroy;

/*
 * Allocate and return a new element of a list
 */
Nse_listelem
nse_list_create_elem(list)
	Nse_list	list;
{
	Nse_listelem	elem;
	Nse_opaquefunc	create;

	create = list->ops->create;
	if (create != NULL) {
		elem = NSE_NEW(Nse_listelem);
		elem->data = create();
	} else {
		elem = NULL;
	}
	return elem;
}

/*
 * Allocate and return a new element of a list.
 * Set its data to that given.
 */
/* VARARGS1 */
Nse_listelem
nse_list_add_new_data(list, data)
	Nse_list	list;
	Nse_opaque	data;
{
	Nse_listelem	elem;

	elem = NSE_NEW(Nse_listelem);
	elem->data = data;
	nse_list_insert_tail(list, elem);
	return elem;
}

/*
 * Allocate and return a new element of a list.
 * Set its data to a copy of that given.
 */
/* VARARGS1 */
Nse_listelem
nse_list_add_new_copy(list, data)
	Nse_list	list;
	Nse_opaque	data;
{
	Nse_listelem	elem;
	Nse_opaquefunc	copy_func;

	copy_func = list->ops->copy;
	elem = NSE_NEW(Nse_listelem);
	if (copy_func != NULL) {
		elem->data = copy_func(data);
	}
	nse_list_insert_tail(list, elem);
	return elem;
}


/*
 * Allocate a new element of a list.
 * Insert it at the tail of the list.
 * Return a pointer to the data, not the listelem.
 */
Nse_opaque
nse_list_add_new_elem(list)
	Nse_list	list;
{
	Nse_listelem	listelem;
	Nse_opaque	data;

	data = NULL;
	listelem = nse_list_create_elem(list);
	if (listelem != NULL) {
		nse_list_insert_tail(list, listelem);
		data = listelem->data;
	}
	return data;
}

/*
 * Copy a list.
 */
Nse_list
nse_list_copy(list)
	Nse_list	list;
{
	Nse_list	newlist;
	Nse_listelem	newelem;
	Nse_listelem	elem;

	if (!list) {
		return NULL;
	}
	n_copy++;
	newlist = nse_list_alloc(list->ops);
	for (elem = list->anchor.next; elem != &list->anchor; 
	    elem = elem->next) {
		newelem = nse_listelem_copy(newlist, elem);
		nse_list_insert_tail(newlist, newelem);
	}
	return(newlist);
}

/*
 * destroy all of the elements of a list.
 */
void
nse_list_clear(list)
	Nse_list	list;
{
	Nse_listelem	elem;
	Nse_listelem	next;

	if (!list) {
		return;
	}
	for (elem = list->anchor.next; elem != &list->anchor; elem = next) {
		next = elem->next;
		nse_listelem_delete(list, elem);
	}
}

/*
 * Destroy and free the memory associated with a list.
 */
void
nse_list_destroy(list)
	Nse_list	list;
{
	Nse_listelem	elem;
	Nse_listelem	next;

	if (!list) {
		return;
	}
	n_destroy++;
	for (elem = list->anchor.next; elem != &list->anchor; elem = next) {
		next = elem->next;
		nse_listelem_destroy(list, elem);
	}
	NSE_DISPOSE(list);
}

/*
 * Destroy and free the memory associated with the structure wrapped around
 * the data elements of a list
 */
void
nse_list_destroy_wrapper(list)
	Nse_list	list;
{
	Nse_listelem	elem;
	Nse_listelem	next;

	if (!list) {
		return;
	}
	for (elem = list->anchor.next; elem != &list->anchor; elem = next) {
		next = elem->next;
		nse_listelem_destroy_wrapper(list, elem);
	}
	NSE_DISPOSE(list);
}

/*
 * Print each element of a list.
 */
void
nse_list_print(list)
	Nse_list	list;
{
	Nse_listelem	elem;

	if (!list) {
		printf("(nil)\n");
		return;
	}
	for (elem = list->anchor.next; elem != &list->anchor; 
	    elem = elem->next) {
		nse_listelem_print(list, elem);
	}
}

/*
 * Return a pointer to the first element of a list 
 */
Nse_listelem
nse_list_first_elem(list)
	Nse_list	list;
{
	return list->anchor.next;
}

/*
 * Return a pointer to the end of the list.
 * The end of the list is actually the anchor - not the last
 * element.
 */
Nse_listelem
nse_list_end(list)
	Nse_list	list;
{
	return &list->anchor;
}

/*
 * Iterate over a list calling a function with a pointer to each 
 * member of the list.
 */
/* VARARGS1 */
void
nse_list_iterate(list, func, data1, data2, data3, data4, data5)
	Nse_list	list;
	Nse_voidfunc	func;
	Nse_opaque	data1;
	Nse_opaque	data2;
	Nse_opaque	data3;
	Nse_opaque	data4;
	Nse_opaque	data5;
{
	Nse_listelem	elem;

	for (elem = list->anchor.next; elem != &list->anchor; 
	    elem = elem->next) {
		func(elem->data, data1, data2, data3, data4, data5);
	}
}

/*
 * Iterate over a list calling a function with a pointer to each 
 * member of the list, stopping at the first non-zero return value.
 */
/* VARARGS1 */
Nse_opaque
nse_list_iterate_or(list, func, data1, data2, data3, data4)
	Nse_list	list;
	Nse_opaquefunc	func;
	Nse_opaque	data1;
	Nse_opaque	data2;
	Nse_opaque	data3;
	Nse_opaque	data4;
{
	Nse_listelem	elem;
	Nse_opaque	ret;

	ret = (Nse_opaque) 0;
	for (elem = list->anchor.next; elem != &list->anchor; 
	    elem = elem->next) {
		if (ret = func(elem->data, data1, data2, data3, data4)) {
			break;
		}
	}
	return ret;
}

/*
 * Search a list by iterating over it and calling the list equal function on
 * each member of the list.  When the element is found that matches data, stop
 * the search and return element pointer of that list element.  If no matching
 * element is found, return NULL.
 */
Nse_listelem
nse_list_find_elem(list, data)
	Nse_list	list;
	Nse_opaque	data;
{
	Nse_boolfunc	equal_func;
	Nse_listelem	r;

	equal_func = list->ops->equal;
	if (equal_func == NULL) {
		return NULL;
	}
	r = nse_list_search_elem(list, equal_func, data);
	return r;
}

/*
 * Search a list by iterating over it and calling a function with a pointer to
 * each member of the list.  When the function returns TRUE, stop the search
 * and return data pointer of that list element.
 */
/* VARARGS1 */
Nse_opaque
nse_list_search(list, func, data1, data2, data3, data4)
	Nse_list	list;
	Nse_boolfunc	func;
	Nse_opaque	data1;
	Nse_opaque	data2;
	Nse_opaque	data3;
	Nse_opaque	data4;
{
	Nse_listelem	elem;

	elem = nse_list_search_elem(list, func, data1, data2, data3, data4);
	if (elem != NULL) {
		return elem->data;
	} else {
		return NULL;
	}
}


/* 
 * Search a list by iterating over it and calling a function
 * with a pointer to each member of the list.  When the function
 * returns TRUE, stop the search and return that list element.
 */
/* VARARGS1 */
Nse_listelem
nse_list_search_elem(list, func, data1, data2, data3, data4)
	Nse_list	list;
	Nse_boolfunc	func;
	Nse_opaque	data1;
	Nse_opaque	data2;
	Nse_opaque	data3;
	Nse_opaque	data4;
{
	Nse_listelem	elem;

	for (elem = list->anchor.next; elem != &list->anchor; 
	     elem = elem->next) {
		if (func(elem->data, data1, data2, data3, data4) == TRUE) {
			return elem;
		}
	}
	return NULL;
}


/*
 * Insert a node at the head of a list.
 */
void
nse_list_insert_head(list, elem)
	Nse_list	list;
	Nse_listelem	elem;
{
	nse_list_insert_after(list, &list->anchor, elem);
}

/*
 * Insert a node at the tail of a list.
 */
void
nse_list_insert_tail(list, elem)
	Nse_list	list;
	Nse_listelem	elem;
{
	nse_list_insert_after(list, list->anchor.prev, elem);
}

/*
 * Insert a node before another node.
 */
void
nse_list_insert_before(list, old, new)
	Nse_list	list;
	Nse_listelem	old;
	Nse_listelem	new;
{
	nse_list_insert_after(list, old->prev, new);
}

/*
 * Insert a node after another node.
 */
void
nse_list_insert_after(list, old, new)
	Nse_list	list;
	Nse_listelem	old;
	Nse_listelem	new;
{
	old->next->prev = new;
	new->next = old->next;
	old->next = new;
	new->prev = old;
	list->nelems++;
}

/*
 * Return the number of elements in a list.
 */
int
nse_list_nelems(list)
	Nse_list	list;
{
	if (!list) {
		return 0;
	}
	return list->nelems;
}

/*
 * Append one list to the end of the other.
 * Make sure the lists are of the same type.
 * Modifies destlist and destroys srclist.
 */
Nse_list
nse_list_append(destlist, srclist) 
	Nse_list	destlist;
	Nse_list	srclist;
{
	if (srclist->nelems == 0) {
		return;
	}
	destlist->anchor.prev->next = srclist->anchor.next;
	srclist->anchor.next->prev = destlist->anchor.prev;
	destlist->anchor.prev = srclist->anchor.prev;
	srclist->anchor.prev->next = &destlist->anchor;
	destlist->nelems += srclist->nelems;
	srclist->anchor.prev = &srclist->anchor;
	srclist->anchor.next = &srclist->anchor;
	srclist->nelems = 0;
	nse_list_destroy(srclist);
}

/*
 * Compare two lists.  Stop when a difference is found and return FALSE.	
 * Return TRUE if there are no differences.
 */
bool_t
nse_list_cmp(list1, list2)
	Nse_list	list1;
	Nse_list	list2;
{
	Nse_listelem	le1;
	Nse_listelem	le2;
	register int	i;
	int		numelems;

	/* TRUE if the same list or both NULL, FALSE if one NULL */
	if (list1 == list2) {
		return TRUE;
	} else if ((!list1) || (!list2)) {
		return FALSE;
	}
	/* first do quick check by checking the number of elements
	 * in each list.
	 */
	if ((numelems = nse_list_nelems(list1)) != nse_list_nelems(list2)) {
		return(FALSE);
	}
	le1 = nse_list_first_elem(list1);
	le2 = nse_list_first_elem(list2);
	for (i = numelems; i ; i--) {
		if (!nse_listelem_equal(list1, le1, le2)) {
			return(FALSE);
		}
		le1 = nse_listelem_next(le1);
		le2 = nse_listelem_next(le2);
	}
	return(TRUE);
}

/*
 * Produce a list containing the members that are in list1 and not
 * in list2.
 */
Nse_list
nse_list_diff(list1, list2)
	Nse_list	list1;
	Nse_list	list2;
{
	Nse_list	diff;
	Nse_listelem	le1;
	Nse_listelem	le2;
	Nse_listelem	listelem;
	Nse_listelem	stop1;
	Nse_listelem	stop2;

	diff = nse_list_alloc(list1->ops);
	stop1 = nse_list_end(list1);
	for (le1 = nse_list_first_elem(list1); le1 != stop1; 
	    le1 = nse_listelem_next(le1)) {
		stop2 = nse_list_end(list2);
		for (le2 = nse_list_first_elem(list2); le2 != stop2;
		    le2 = nse_listelem_next(le2)) {
			if (nse_listelem_equal(list1, le1, le2)) {
				break;
			}
		}
		if (le2 == stop2) {
			listelem = nse_listelem_copy(list1, le1);
			nse_list_insert_tail(diff, listelem);
		}
	}
	return diff;
}

/*
 * Produce a list that is the union of the members of list1 and list2.
 */
Nse_list
nse_list_union(list1, list2)
	Nse_list	list1;
	Nse_list	list2;
{
	Nse_list	either;
	Nse_list	more;

	either = nse_list_copy(list1);
	more = nse_list_diff(list2, list1);
	nse_list_append(either, more);
	return either;
}

/*
 * Produce a list that is intersection union of the members of list1 and list2.
 */
Nse_list
nse_list_intersection(list1, list2)
	Nse_list	list1;
	Nse_list	list2;
{
	Nse_list	intersection;
	Nse_listelem	le1;
	Nse_listelem	le2;
	Nse_listelem	listelem;
	Nse_listelem	stop1;
	Nse_listelem	stop2;

	intersection = nse_list_alloc(list1->ops);
	stop1 = nse_list_end(list1);
	for (le1 = nse_list_first_elem(list1); le1 != stop1; 
	    le1 = nse_listelem_next(le1)) {
		stop2 = nse_list_end(list2);
		for (le2 = nse_list_first_elem(list2); le2 != stop2;
		    le2 = nse_listelem_next(le2)) {
			if (nse_listelem_equal(list1, le1, le2)) {
				listelem = nse_listelem_copy(list1, le1);
				nse_list_insert_tail(intersection, listelem);
				break;
			}
		}
	}
	return intersection;
}

/*
 * Produce a list that is the contents of both list1 and list2.
 */
Nse_list
nse_list_add(list1, list2)
	Nse_list	list1;
	Nse_list	list2;
{
	Nse_list	add;
	Nse_list	tmp;

	add = nse_list_copy(list1);
	tmp = nse_list_copy(list2);
	nse_list_append(add, tmp);
	return add;
}

/*
 * Sort a list.  Uses qsort().  Allocate buffer large enough
 * to hold a pointer to each listelem.  Call qsort() and use
 * the given comparision function.  Then rebuild the list.
 */
void
nse_list_sort(list, compare)
	Nse_list	list;
	Nse_intfunc		compare;
{
	Nse_listelem	le;
	Nse_listelem	next;
	Nse_listelem	*argv;
	int		i, j;

	if (list == NULL) {
		return;
	}
	argv = (Nse_listelem *) malloc((unsigned)
					(sizeof(Nse_listelem) * list->nelems));
	i = 0;
	for (le = list->anchor.next; le != &list->anchor; le = next) {
		next = le->next;
		argv[i] = le;
		nse_listelem_remove(list, le);
		i++;
	}
	qsort((char *) argv, i, sizeof(Nse_listelem), compare);
	for (j = 0; j < i; j++) {
		nse_list_insert_tail(list, argv[j]);
	}
}

/*
 * Allocate the memory for a listhdr, find its type and initialized
 * the type and ops fields.
 */
Nse_list
nse_list_alloc(ops)
	Nse_listops	ops;
{
	Nse_list	list;

	n_create++;
	list = alloc_listhdr();
	list->ops = ops;
	return list;
}

/*
 * Allocate the memory for listhdr and connect the anchor to itself.
 */
static  Nse_list
alloc_listhdr()
{
	Nse_list	list;

	list = NSE_NEW(Nse_list);
	list->anchor.next = &list->anchor;
	list->anchor.prev = &list->anchor;
	return list;
}

/*
 * Generic routine for printing out stats about a list.
 * Gives total number of elements, the number of bytes occupied by
 * the elements, the number of bytes private to the data structure
 * (strings for instances), and the amount of list overhead.
 */
void
nse_list_print_stats(fp, list, sizeof_elem, func, str)
	FILE		*fp;
	Nse_list	list;
	int		sizeof_elem;
	Nse_intfunc	func;
	char		*str;
{
	Nse_listelem	le;
	Nse_listelem	stop;
	Nse_opaque	data;
	int		n;
	int		data_bytes;
	int		private_bytes;
	int		list_bytes;

	if (list == NULL) {
		return;
	}
	n = list->nelems;
	fprintf(fp, "\n");
	fprintf(fp, "%s\n", str);
	fprintf(fp, "\t%-20s %5d\n", "# elements in list", n);
	data_bytes = sizeof_elem * n;
	fprintf(fp, "\t%-20s %5d\n", "bytes in list data", data_bytes);
	private_bytes = 0;
	NSE_LIST_ITERATE(list, Nse_opaque, data, le, stop) {
		private_bytes += func(data);
	}
	fprintf(fp, "\t%-20s %5d\n", "data-specific bytes", private_bytes);
	list_bytes = n * sizeof(Nse_listelem_rec) + sizeof(Nse_list_rec);
	fprintf(fp, "\t%-20s %5d\n", "list overhead", list_bytes);
	fprintf(fp, "\t%-20s %5d\n", "total bytes", 
	    data_bytes + private_bytes + list_bytes);
}

/*
 * Print stats about the usage of lists.
 */
void
nse_list_print_cnt_stats(fp)
	FILE		*fp;
{
	fprintf(fp, "\n");
        fprintf(fp, "Nse_lists (%d bytes each):\n", sizeof(Nse_list));
        fprintf(fp, "\t%-20s %5d\n", "creates", n_create);
        fprintf(fp, "\t%-20s %5d\n", "copies", n_copy);
        fprintf(fp, "\t%-20s %5d\n", "destroys", n_destroy);
}
