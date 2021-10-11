#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ntfy_protect.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ntfy_protect.c - Contains routines used exclusively for the
 * protection of notifier data from signal interrupts.  Called by both
 * the detector and the dispatcher.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>	/* For ndet_send_delayed_sigs */

Notify_error	notify_errno = 0;

pkg_private_data int	ntfy_sigs_blocked = 0;   /* count of nesting of signal blocking*/
pkg_private_data int	ntfy_interrupts = 0;     /* count of interrupts handling (0 or 1) */
pkg_private_data u_int ntfy_sigs_delayed = 0; /* Bit mask of signals received while in
				        critical section */

static	NTFY_NODE *ntfy_malloc_tb_freed;/* Linked list of "nodes" to be freed
				   that were allocated with malloc.  Nodes are
				   at least 4 bytes long but are almost
				   certainly not sizeof(NTFY_NODE) long. */

pkg_private void
ntfy_end_critical()
{
	/* See if about to exit critical section while not at interrupt level */
	if (ntfy_sigs_blocked == 1 && !NTFY_IN_INTERRUPT) {
		/* See if the pre-alloced pool of nodes has fallen low */
		if( ntfy_nodes_avail < NTFY_PRE_ALLOCED)
			ntfy_replenish_nodes();
	}
	ntfy_sigs_blocked--;
	/* See if any signals arrived while in a critical section */
	if (ntfy_sigs_blocked == 0 && ntfy_sigs_delayed != 0)
		ndet_send_delayed_sigs();
}

pkg_private char *
ntfy_malloc(size)
	u_int size;
{
	extern	char *malloc();
	char *ptr;

	ntfy_assert(!NTFY_IN_INTERRUPT, "Mallocing from interrupt level");
	if ((ptr = malloc(size)) == (char *)0)
		ntfy_set_errno(NOTIFY_NOMEM);
	return(ptr);
}

pkg_private void
ntfy_free_malloc(ptr)
	NTFY_DATA ptr;
{
	NTFY_NODE *tmp;

	if (NTFY_IN_INTERRUPT) {
		NTFY_BEGIN_CRITICAL;
		tmp = ntfy_malloc_tb_freed;
		ntfy_malloc_tb_freed = (NTFY_NODE *)ptr;
		ntfy_malloc_tb_freed->n.next = tmp;
		NTFY_END_CRITICAL;
	} else {
		ntfy_flush_tb_freed();
		free((char *)ptr);
        }
}

pkg_private void
ntfy_flush_tb_freed()
{
	register NTFY_NODE *n, *next;

	ntfy_assert(!NTFY_IN_INTERRUPT, "Freeing from interrupt level");
	NTFY_BEGIN_CRITICAL;
	for (n = ntfy_malloc_tb_freed;n;n = next) {
		next = n->n.next;
		free((char *)n);
	}
	ntfy_malloc_tb_freed = NTFY_NODE_NULL;
	NTFY_END_CRITICAL;
}

