/* @(#)cache.h 1.1 92/07/30 SMI */

/*
 * ld.so directory caching
 */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Shared object lookup performance in the run-time link editor is 
 * enhanced through the use of caches for directories that the editor
 * searches.  A given "cache" describes the contents of a single directory,
 * and each cache entry contains the canonical name for a shared object
 * as well as its absolute pathname.
 *
 * Within a cache, "pointers" are really relative addresses to some absolute
 * address (often the base address of the containing database).
 */

#define	CACHE_FILE	"/etc/ld.so.cache"	/* file where it is stored */

/*
 * Relative pointer macros.
 */
#define	RELPTR(base, absptr) ((long)(absptr) - (long)(base))
#define	AP(base) ((caddr_t)base)

/*
 * Definitions for cache structures.
 */
#define DB_HASH		11		/* number of hash buckets in caches */
#define LD_CACHE_MAGIC 	0x041155	/* cookie to identify data structure */
#define LD_CACHE_VERSION 0		/* version number of cache structure*/

struct	dbe	{			/* element of a directory cache */
	long	dbe_next;		/* (rp) next element on this list */
	long	dbe_lop;		/* (rp) canonical name for object */
	long	dbe_name;		/* (rp) absolute name */
};

struct	db	{			/* directory cache database */
	long	db_name;		/* (rp) directory contained here */
	struct	dbe db_hash[DB_HASH];	/* hash buckets */
	caddr_t	db_chain;		/* private to database mapping */
};

struct dbf 	{			/* cache file image */
	long dbf_magic;			/* identifying cookie */
	long dbf_version;		/* version no. of these dbs */
	long dbf_machtype;		/* machine type */
	long dbf_db;		/* directory cache dbs */
};

/*
 * Structures used to describe and access a database.
 */
struct	dbd	{			/* data base descriptor */
	struct	dbd *dbd_next;		/* next one on this list */
	struct	db *dbd_db;		/* data base described by this */
};

struct	dd	{			/* directory descriptor */
	struct	dd *dd_next;		/* next one on this list */
	struct	db *dd_db;		/* data base described by this */
};

/*
 * Interfaces imported/exported by the lookup code.
 */
caddr_t (*db_malloc)();			/* allocator for relative objects */
caddr_t (*heap_malloc)();		/* allocator for general objects */
int (*is_secure)();			/* tells whether censorship in force */
extern	int use_cache;			/* use existing cache? */
extern caddr_t db_base;

char	*lo_lookup(/* lop, lmp */);	/* name for link_object */
struct	db *lo_cache(/* cp */);		/* obtain cache for directory name */
void	lo_flush();			/* foreach dir get a new database */
void	dbd_flush();			/* delete the dbs that came from 
					   ld.so.cache */
