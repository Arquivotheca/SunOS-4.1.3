/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 * @(#)htable.h 1.1 92/07/30 SMI from UCB 5.1 5/28/85
 */

#include <sys/types.h>
#include <netinet/in.h>

/*
 * common definitions for htable
 */

struct addr {
	struct	in_addr addr_val;
	struct	addr *addr_link;
};

struct name {
	char	*name_val;
	struct	name *name_link;
};

struct gateway {
	struct	gateway *g_link;
	struct	gateway *g_dst;		/* connected gateway if metric > 0 */
	struct	gateway *g_firstent;	/* first entry for this gateway */
	struct	name	*g_name;
	int	g_net;
	struct in_addr	g_addr;		/* address on g_net */
	int	g_metric;		/* hops to this net */
};

#define	NOADDR			((struct addr *)0)
#define	NONAME			((struct name *)0)

#define	KW_NET		1
#define	KW_GATEWAY	2
#define	KW_HOST		3

struct name *newname();
char *malloc();

char *infile;			/* Input file name */
