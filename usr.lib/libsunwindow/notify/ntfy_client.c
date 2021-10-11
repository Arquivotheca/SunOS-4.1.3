#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ntfy_client.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif
 
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ntfy_client.c - NTFY_CLIENT specific operations that both the detector
 * and dispatcher share.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndis.h>	/* For ndis_default_prioritizer */

/* Variables used in paranoid enumerator (see ntfy_condition) */
pkg_private_data NTFY_CLIENT *ntfy_enum_client = 0;
pkg_private_data NTFY_CLIENT *ntfy_enum_client_next = 0;

pkg_private NTFY_CLIENT *
ntfy_find_nclient(client_list, nclient, client_latest)
	NTFY_CLIENT *client_list;
	Notify_client nclient;
	register NTFY_CLIENT **client_latest;
{
	register NTFY_CLIENT *client;
	NTFY_CLIENT *next;
	
	ntfy_assert(NTFY_IN_CRITICAL, "Unprotected list search");
	/* See if hint matches */
	if (*client_latest && (*client_latest)->nclient == nclient)
		return(*client_latest);
	/* Search entire list */
	for (client = client_list; client; client = next) {
		next = client->next;
		if (client->nclient == nclient) {
			/* Set up hint for next time */
			*client_latest = client;
			return(client);
		}
	}
	return(NTFY_CLIENT_NULL);
}

/*
 * Find/create client that corresponds to nclient
 */
pkg_private NTFY_CLIENT *
ntfy_new_nclient(client_list, nclient, client_latest)
	NTFY_CLIENT **client_list;
	Notify_client nclient;
	NTFY_CLIENT **client_latest;
{
	register NTFY_CLIENT *client;

	if ((client = ntfy_find_nclient(*client_list, nclient,
	    client_latest)) == NTFY_CLIENT_NULL) {
		/* Allocate client */
		if ((client = ntfy_alloc_client()) == NTFY_CLIENT_NULL)
			return(NTFY_CLIENT_NULL);
		/* Initialize client */
		client->next = NTFY_CLIENT_NULL;
		client->conditions = NTFY_CONDITION_NULL;
		client->condition_latest = NTFY_CONDITION_NULL;
		client->nclient = nclient;
		client->prioritizer = ndis_default_prioritizer;
		client->flags = 0;
		/* Append to client list */
		ntfy_append_client(client_list, client);
		/* Set up client hint */
		*client_latest = client;
	}
	return(client);
}

pkg_private void
ntfy_remove_client(client_list, client, client_latest, who)
	NTFY_CLIENT **client_list;
	NTFY_CLIENT *client;
	NTFY_CLIENT **client_latest;
	NTFY_WHO who;
{
	register NTFY_CONDITION *condition;
	NTFY_CONDITION *next;

	/* Fixup enumeration variables if client matches one of them */
	if (client == ntfy_enum_client)
		ntfy_enum_client = NTFY_CLIENT_NULL;
	if (client == ntfy_enum_client_next)
		ntfy_enum_client_next = ntfy_enum_client_next->next;
	/* Make sure that remove all conditions */
	for (condition = client->conditions; condition; condition = next) {
		next = condition->next;
		ntfy_remove_condition(client, condition, who);
	}
	/* Remove & free client from client_list */
	ntfy_remove_node((NTFY_NODE **)client_list, (NTFY_NODE *)client);
	/* Invalidate condition hint */
	*client_latest = NTFY_CLIENT_NULL;
}

