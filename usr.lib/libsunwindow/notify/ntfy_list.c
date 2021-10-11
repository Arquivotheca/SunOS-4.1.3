#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ntfy_list.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ntfy_list.c - NTFY_CLIENT and NTFY_CONDITION list management.
 * Both structures use NTFY_NODEs and can share the same list management
 * code.
 */

#include <sunwindow/ntfy.h>

pkg_private void
ntfy_append_node(node_list, new_node)
	register NTFY_NODE **node_list;
	register NTFY_NODE *new_node;
{
	register NTFY_NODE *node;

	ntfy_assert(NTFY_IN_CRITICAL, "Unprotected list manipulations");
	new_node->n.next = NTFY_NODE_NULL;
	for (node = *node_list;node;node = node->n.next) {
		if (node->n.next == NTFY_NODE_NULL)
			break;
	}
	if (node == NTFY_NODE_NULL)
		*node_list= new_node;
	else
		node->n.next = new_node;
}

pkg_private void
ntfy_remove_node(node_list, node_axe)
	register NTFY_NODE **node_list;
	register NTFY_NODE *node_axe;
{
	register NTFY_NODE **node_ptr;
	register NTFY_NODE **node_ptr_next;

	ntfy_assert(*node_list!= NTFY_NODE_NULL,
	    "Tried to remove node from empty list");
	ntfy_assert(node_axe != NTFY_NODE_NULL, "Tried to remove null node");
	ntfy_assert(NTFY_IN_CRITICAL, "Unprotected list manipulations");
	for (node_ptr = node_list; *node_ptr; node_ptr = node_ptr_next) {
		node_ptr_next = &((*node_ptr)->n.next);
		if (*node_ptr == node_axe) {
			*node_ptr = *node_ptr_next;
			ntfy_free_node(node_axe);
			return;
		}
	}
	ntfy_fatal_error("Tried to remove node that wasn't on list");
}

