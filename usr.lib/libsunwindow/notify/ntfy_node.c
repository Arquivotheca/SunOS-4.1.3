#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ntfy_node.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ntfy_node.c - Storage management for the notifier.
 */

#include <sunwindow/ntfy.h>

pkg_private_data int	ntfy_nodes_avail = 0;	/* Count of nodes in ntfy_node_free */
pkg_private_data int	ntfy_node_blocks = 0;	/* Count of trips to heap for nodes
					   (used for statistics & possibly
					   run away process detection) */

static	NTFY_NODE *ntfy_node_free;	/* List of free nodes */

/*
 * Caller must initialize data returned from ntfy_alloc_node.
 * NTFY_NODE_NULL is possible.
 */
pkg_private NTFY_NODE *
ntfy_alloc_node()
{
	NTFY_NODE *node;

	if (ntfy_node_free == NTFY_NODE_NULL) {
		if (NTFY_IN_INTERRUPT)
			return(NTFY_NODE_NULL);
		else
			ntfy_replenish_nodes();
	}
	NTFY_BEGIN_CRITICAL;	/* Protect node pool */
	if (ntfy_node_free == NTFY_NODE_NULL) {
		ntfy_set_errno(NOTIFY_NOMEM);
		NTFY_END_CRITICAL;
		return(NTFY_NODE_NULL);
	}
	ntfy_assert(ntfy_nodes_avail > 0, "Node count wrong");
	node = ntfy_node_free;
	ntfy_node_free = ntfy_node_free->n.next;
	ntfy_nodes_avail--;
	NTFY_END_CRITICAL;
	return(node);
}

pkg_private void
ntfy_replenish_nodes()
{
	extern	char *calloc();
	register NTFY_NODE *new_nodes, *node;

	ntfy_assert(!NTFY_IN_INTERRUPT, "Interrupt access to heap");
	ntfy_assert(ntfy_nodes_avail <= NTFY_PRE_ALLOCED,
	    "Unnecessary node replenishment");
	new_nodes = (NTFY_NODE *) calloc(
	    1, NTFY_NODES_PER_BLOCK*sizeof(NTFY_NODE));
	for (node = new_nodes;node < new_nodes+NTFY_NODES_PER_BLOCK;node++)
		ntfy_free_node(node);
	ntfy_node_blocks++;
}

pkg_private void
ntfy_free_node(node)
	register NTFY_NODE *node;
{
	NTFY_BEGIN_CRITICAL;	/* Protect node pool */
	node->n.next = ntfy_node_free;
	ntfy_node_free = node;
	ntfy_nodes_avail++;
	NTFY_END_CRITICAL;
}

