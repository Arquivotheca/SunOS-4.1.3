/*	"@(#)defaults_impl.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985, 1988  by Sun Microsystems, Inc.
 */

#define	MAX_NAME	100	/* Maximum name length */
#define	MAX_STRING	255	/* Maximum string length */
#define	MAX_ERRORS	10	/* Maximum number of defaults errors */
#define	VERSION_NUMBER	2	/* Version number */
#define	VERSION_TAG	"SunDefaults_Version"	/* Version number tag */
#define	SEP_CHR		'/'	/* Separator character */
#define	SEP_STR		"/"	/* Separator string */


typedef char *Symbol;		/* A null-terminated string */
typedef Symbol *Name;		/* A null-terminated list of symbols */
typedef struct _node *Node;	/* Node object */
typedef struct _defaults *Defaults;	/* Defaults object */

/* Data structure defintions: */
struct _defaults {		/* Defaults data structure */
	char	*database_dir;	/* Database directory; NULL: .defaults only */
	int	errors;		/* Number of errors encountered */
	Hash	nodes;		/* Nodes hash table */
	Name	prefix;		/* Prefix symbol */
	char	*private_dir;	/* Private directory database */
	Node	root;		/* Root node of entire tree */
	Hash	symbols;	/* Symbols hash table */
	Bool	test_mode;	/* TRUE => database inaccessable */
};

struct _node {			/* Node object */
	Node	child;		/* Eldest child of parent */
	char	*default_value;	/* Default value associated with node */
	Name	name;		/* Full name */
	Node	next;		/* Next node on same level (brother/sister) */
	Node	parent;		/* Parent node */
	char	*value;		/* Value associated with node */
	Bool	private : 1;/* TRUE => Private database node */
	Bool	deleted : 1;/* TRUE => node does not really exist */
	Bool	file    : 1;/* TRUE => file associated with node */
};

