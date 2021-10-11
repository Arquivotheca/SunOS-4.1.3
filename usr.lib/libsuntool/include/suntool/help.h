/* @(#)help.h 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#ifndef help_DEFINED
#define help_DEFINED

#include <sunwindow/attr.h>

#define HELPDIRECTORY "/usr/lib/help"
#define HELPINDEX "/usr/lib/help/helpindex"

/*
 * Attributes.
 */

#define HELP_ATTR(type, ordinal)	ATTR(ATTR_PKG_HELP, type, ordinal)

/* first attribute used */
#define HELP_FIRST_ATTR			32

typedef enum {
	HELP_DATA	= HELP_ATTR(ATTR_OPAQUE, HELP_FIRST_ATTR + 1),
} Help_attribute;

/* new attributes should start at HELP_LAST_ATTR + 1 */
#define HELP_LAST_ATTR			(HELP_FIRST_ATTR + 1)

#define event_is_help(event) (event_id(event)==ACTION_HELP)

/*
 * Public interface.
 */

extern void help_request();
extern void help_more();
extern void help_set_request_func();
extern void help_set_more_func();
extern char *help_get_arg();
extern char *help_get_text();

/*
 * Default help RPC interface.
 */

#define HELPREQUEST 1

#define HELPDEFAULTSERVER "help_viewer"
#define HELPDEFAULTPROG 100040
#define HELPVERS 1
#define HELPDATAMAX 512

typedef struct {
    char *data;		/* client data */
    int  x, y;		/* screen x, y */
    int  pid;		/* requesting process id */
    int  seq;		/* request sequence number */
} Help_request;

extern int xdr_help();		/* XDR routine for help args */
extern int help_rpc_register(); /* register rpc handler with Notifier */

#endif not help_DEFINED


