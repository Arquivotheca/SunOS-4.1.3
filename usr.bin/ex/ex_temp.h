/*	@(#)ex_temp.h 1.1 92/07/30 SMI; from S5R3.1 1.6	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1981 Regents of the University of California */

/*
 * The editor uses a temporary file for files being edited, in a structure
 * similar to that of ed.  The first block of the file is used for a header
 * block which guides recovery after editor/system crashes.
 * Lines are represented in core by a pointer into the temporary file which
 * is packed into 16 bits (32 on VMUNIX).  All but the low bit index the temp
 * file; the last is used by global commands.  The parameters below control
 * how much the other bits are shifted left before they index the temp file.
 * Larger shifts give more slop in the temp file but allow larger files
 * to be edited.
 *
 * The editor does not garbage collect the temporary file.  When a new
 * file is edited, the temporary file is rather discarded and a new one
 * created for the new file.  Garbage collection would be rather complicated
 * in ex because of the general undo, and in any case would require more
 * work when throwing lines away because marks would have be carefully
 * checked before reallocating temporary file space.  Said another way,
 * each time you create a new line in the temporary file you get a unique
 * number back, and this is a property used by marks.
 *
 * The following temp file parameters allow 256k bytes in the temporary
 * file.  By changing to the numbers in comments you can get 512k.
 * For VMUNIX you get more than you could ever want.
 * VMUNIX uses long (32 bit) integers giving much more
 * space in the temp file and no waste.  This doubles core
 * requirements but allows files of essentially unlimited size to be edited.
 */
#ifndef VMUNIX
#define	BLKMSK	0777		/* 01777 */
#define	BNDRY	8		/* 16 */
#define	INCRMT	0200		/* 0100 */
#define	LBTMSK	0770		/* 0760 */
#define	NMBLKS	506		/* 1018 */
#define	OFFBTS	7		/* 6 */
#define	OFFMSK	0177		/* 077 */
#define	SHFT	2		/* 3 */
#else
#define	BLKMSK	077777
#define	BNDRY	2
#define	INCRMT	02000
#define	LBTMSK	01776
#define	NMBLKS	077770
#define	OFFBTS	10
#define	OFFMSK	01777
#define	SHFT	0
#endif

/*
 * The editor uses three buffers into the temporary file (ed uses two
 * and is very similar).  These are two read buffers and one write buffer.
 * Basically, the editor deals with the file as a sequence of BUFSIZ character
 * blocks.  Each block contains some number of lines (and lines
 * can run across block boundaries.
 *
 * New lines are written into the last block in the temporary file
 * which is in core as obuf.  When a line is needed which isn't in obuf,
 * then it is brought into an input buffer.  As there are two, the choice
 * is to take the buffer into which the last read (of the two) didn't go.
 * Thus this is a 2 buffer LRU replacement strategy.  Measurement
 * shows that this saves roughly 25% of the buffer reads over a one
 * input buffer strategy.  Since the editor (on our VAX over 1 week)
 * spends (spent) roughly 30% of its time in the system read routine,
 * this can be a big help.
 */
var bool	hitin2;		/* Last read hit was ibuff2 not ibuff */
var bool	ichang2;	/* Have actually changed ibuff2 */
var bool	ichanged;	/* Have actually changed ibuff */
var short	iblock;		/* Temp file block number of ibuff (or -1) */
var short	iblock2;	/* Temp file block number of ibuff2 (or -1) */
var short	ninbuf;		/* Number useful chars left in input buffer */
var short	nleft;		/* Number usable chars left in output buffer */
var short	oblock;		/* Temp file block number of obuff (or -1) */
#ifndef VMUNIX
var short	tline;		/* Current temp file ptr */
#else
var int	tline;
#endif

var char	ibuff[BUFSIZ];
var char	ibuff2[BUFSIZ];
var char	obuff[BUFSIZ];

/*
 * Structure of the descriptor block which resides
 * in the first block of the temporary file and is
 * the guiding light for crash recovery.
 *
 * As the Blocks field below implies, there are temporary file blocks
 * devoted to (some) image of the incore array of pointers into the temp
 * file.  Thus, to recover from a crash we use these indices to get the
 * line pointers back, and then use the line pointers to get the text back.
 * Except for possible lost lines due to sandbagged I/O, the entire
 * file (at the time of the last editor "sync") can be recovered from
 * the temp file.
 */

/* This definition also appears in expreserve.c... beware */
struct 	header {
	time_t	Time;			/* Time temp file last updated */
	int	Uid;
#ifndef VMUNIX
	short	Flines;			/* Number of lines in file */
#else
	int	Flines;
#endif
	char	Savedfile[FNSIZE];	/* The current file name */
	short	Blocks[LBLKS];		/* Blocks where line pointers stashed */
	short 	encrypted;		/* Encrypted temp file flag */
}; 
var struct 	header H;

#define	uid		H.Uid
#define	flines		H.Flines
#define	savedfile	H.Savedfile
#define	blocks		H.Blocks
