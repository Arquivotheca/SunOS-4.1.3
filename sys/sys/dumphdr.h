/*	@(#)dumphdr.h 1.1 92/07/30 SMI */

#ifndef _SYS_DUMPHDR_
#define _SYS_DUMPHDR_

#include <sys/time.h>

/*
 * The dump header describes the contents of the crash dump.
 * Only "interesting" chunks are written to the dump area; they are
 * written in physical address order and the bitmap indicates which
 * chunks are present.
 * Savecore uses the bitmap to create a sparse file out of the chunks.
 *
 * Layout of crash dump at end of swap partition:
 *  ----------------------------------     end of swap partition
 * |         duplicate header         |
 * |----------------------------------|    page boundary
 * |       "interesting" chunks       |
 * |----------------------------------|    page boundary
 * |   bitmap of interesting chunks   |
 * |----------------------------------|    page boundary
 * |        header information        |
 * |----------------------------------|    page boundary
 * |              unused              |
 *  ----------------------------------     start of swap partition
 *
 * The size of a "chunk" is determined by balancing the following factors:
 *   a desire to keep the bitmap small
 *   a desire to keep the dump small
 * A small chunksize will have a large, sparse bitmap and many small
 * chunks, but only a fraction of physical memory will be represented
 * (the limit is chunksize == pagesize).
 * A large chunksize will have a smaller, denser bitmap and a few large
 * chunks, but much more of physical memory will be represented (the
 * limit is chunksize = physical memory size).
 *
 * "Interesting" is defined as follows:
 * Most interesting:
 *	msgbuf
 *	interrupt stack
 *	user struct of current process (including kernel stack)
 *	kernel data (etext to econtig)
 *	Sysbase to Syslimit (kmem_alloc space)
 * Still interesting:
 *	other user structures
 *	segkmap
 * Least interesting (but may be useful):
 *	anonymous pages (user stacks and modified data)
 *	non-anonymous pages (user text and unmodified data)
 * Not interesting:
 *	"gone" pages
 *
 * A "complete" dump includes all the "interesting" items above.
 */

#define	DUMP_MAGIC	0x8FCA0102	/* magic number for savecore(8) */
#define	DUMP_VERSION	1		/* version of this dumphdr*/
struct	dumphdr {
	long	dump_magic;		/* magic number */
	long	dump_version;		/* version number */
	long	dump_flags;		/* flags; see below */
	long	dump_pagesize;		/* size of page in bytes */
	long	dump_chunksize;		/* size of chunk in pages */
	long	dump_bitmapsize;	/* size of bitmap in bytes */
	long	dump_nchunks;		/* number of chunks */
	long	dump_dumpsize;		/* size of dump in bytes */
	struct timeval	dump_crashtime;	/* time of crash */
	long	dump_versionoff;	/* offset to version string */
	long	dump_panicstringoff;	/* offset to panic string */
	long	dump_headersize;	/* size of header, including strings */
	/* char	dump_versionstr[]; */	/* copy of version string */
	/* char	dump_panicstring[]; */	/* copy of panicstring string */
};

#define dump_versionstr(dhp)	((char *)(dhp) + (dhp)->dump_versionoff)
#define dump_panicstring(dhp)	((char *)(dhp) + (dhp)->dump_panicstringoff)

/* Flags in dump header */
#define DF_VALID	0x00000001	/* Dump is valid (savecore clears) */
#define DF_COMPLETE	0x00000002	/* All "interesting" chunks present */

/*
 * Use the setbit, clrbit, isset, and isclr macros defined in param.h
 * for manipulating the bit map.
 */
#endif _SYS_DUMPHDR_
