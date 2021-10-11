/*  @(#)admin_amcl.h 1.1 92/07/30 SMI  */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifndef _admin_amcl_h
#define _admin_amcl_h

/*
 *	This file contains the exported definitions from the Administrative
 *	Methods Client Library.  The file contains definitions for:
 *
 *	      o Administrative framework control options.
 *	      o Administrative argument types.
 *	      o Status codes.
 *	      o Miscellaneous constants.
 *	      o Structure of an administrative argument.
 *	      o Exported interfaces from the client library.
 */


#include <sys/types.h>


/* Administrative framework control options. */
/* No options are supported in Ice Cream. */

#define ADMIN_END_OPTIONS	0	/* End of option list */


/* Administrative parameter/argument value types */

#define ADMIN_STRING	(u_int) 9	/* All types must be strings for now */


/* Administrative Methods Client Library Status Codes */

#define SUCCESS		0	/* Method completed successfully */
#define FAIL_CLEAN	1	/* Method failed cleanly */
#define FAIL_DIRTY	3	/* Method failed dirty */
#define FAIL_ERROR	4	/* Framework or request error */


/* Miscellaneous constants */

#define ADMIN_LOCAL	    "# LOCAL #"	/* Special local host name keyword */


/* Administrative parameter/argument */

#define ADMIN_NAMESIZE	128

typedef struct Admin_arg Admin_arg;
struct Admin_arg {
	char name[ADMIN_NAMESIZE];	/* parameter/argument name */
	u_int type;			/* parameter/argument type */
	u_int length;			/* parameter/argument length */
	caddr_t value;			/* pointer to parameter/argument value */
	Admin_arg *next;		/* Link to next argument value */
};


int	admin_perf_method();


#endif /* !_admin_amcl_h */

