
/*      @(#)startup.h 1.1 92/07/30 SMI   */

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * This file contains declarations pertaining to reading the data file.
 */
/*
 * The definitions are the token types that the data file parser recognizes.
 */
#define	SUP_EOF			-1		/* eof token */
#define	SUP_STRING		0		/* string token */
#define	SUP_EQL			1		/* equals token */
#define	SUP_COMMA		2		/* comma token */
#define	SUP_COLON		3		/* colon token */
#define	SUP_EOL			4		/* newline token */
#define	SUP_OR			5		/* vertical bar */
#define	SUP_AND			6		/* ampersand */
#define	SUP_TILDE		7		/* tilde */

/*
 * These definitions are flags for the legal keywords in the data file.
 * They are used to keep track of what parameters appear on the current
 * line in the file.
 */
#define	SUP_CTLR		0x00000001	/* set ctlr */
#define	SUP_DISK		0x00000002	/* set disk */
#define	SUP_NCYL		0x00000004	/* set ncyl */
#define	SUP_ACYL		0x00000008	/* set acyl */
#define	SUP_PCYL		0x00000010	/* set pcyl */
#define	SUP_NHEAD		0x00000020	/* set nhead */
#define	SUP_NSECT		0x00000040	/* set nsect */
#define	SUP_RPM			0x00000080	/* set rpm */
#define	SUP_BPT			0x00000100	/* set bytes/track */
#define	SUP_BPS			0x00000200	/* set bytes/sector */
#define	SUP_DRTYPE		0x00000400	/* set drive type */
#define	SUP_SKEW		0x00000800	/* set buf skew */
#define	SUP_PRCMP		0x00001000	/* set write precomp */
#define	SUP_TRKS_ZONE		0x00002000	/* set tracks/zone */
#define	SUP_ATRKS		0x00004000	/* set alt. tracks */
#define	SUP_ASECT		0x00008000	/* set sectors/zone */
#define	SUP_CACHE		0x00010000	/* set cache size*/
#define	SUP_PREFETCH		0x00020000	/* set prefetch threshold */
#define	SUP_CACHE_MIN		0x00040000	/* set min. prefetch */
#define	SUP_CACHE_MAX		0x00080000	/* set max. prefetch */
#define	SUP_PHEAD		0x00100000	/* set physical heads */
#define	SUP_PSECT		0x00200000	/* set physical sects */
#define	SUP_CYLSKEW		0x00400000	/* set cylinder skew */
#define	SUP_TRKSKEW		0x00800000	/* set track skew */
#define	SUP_READ_RETRIES	0x01000000	/* set read retries */
#define	SUP_WRITE_RETRIES	0x02000000	/* set write retries */

/*
 * The define the minimum set of parameters necessary to declare a disk
 * and a partition map in the data file.  Depending on the ctlr type,
 * more info than this may be necessary for declaring disks.
 */
#define	SUP_MIN_DRIVE   (SUP_CTLR | SUP_BPT | SUP_RPM | SUP_PCYL \
			 | SUP_NCYL | SUP_ACYL | SUP_NHEAD | SUP_NSECT)

#define	SUP_MIN_PART	0x0003			/* for maps */

