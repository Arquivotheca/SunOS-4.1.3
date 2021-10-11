/* @(#)exportent.h 1.1 92/07/30 (C) 1986 SMI */

/*
 * Exported file system table, see exportent(3)
 * Copyright (C) 1986 by Sun Microsystems, Inc.
 */ 

#ifndef _exportent_h
#define _exportent_h

#define TABFILE "/etc/xtab"		/* where the table is kept */

/*
 * Options keywords
 */
#define ACCESS_OPT	"access"	/* machines that can mount fs */
#define ROOT_OPT	"root"		/* machines with root access of fs */
#define RO_OPT		"ro"		/* export read-only */
#define RW_OPT		"rw"		/* export read-mostly */
#define ANON_OPT	"anon"		/* uid for anonymous requests */
#define SECURE_OPT	"secure"	/* require secure NFS for access */
#define WINDOW_OPT	"window"	/* expiration window for credential */

struct exportent {
	char *xent_dirname;	/* directory (or file) to export */
	char *xent_options;	/* options, as above */
};

extern FILE *setexportent();
extern void endexportent();
extern int remexportent();
extern int addexportent();
extern char *getexportopt();
extern struct exportent *getexportent();

#endif /*!_exportent_h*/
