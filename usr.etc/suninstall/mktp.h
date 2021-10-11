/*
 *
 * @(#)mktp.h 1.1 92/07/30 Copyr 1987 Sun Micro
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 *
 *
 */

/*
 * mktp.h - include file for mktp program
 */
#include "toc.h"

extern distribution_info dinfo;	/* distribution information             */
extern volume_info vinfo;	/* volume info - not used until actually*/
				/* making the distribution              */
#define	NENTRIES	100
extern toc_entry entries[NENTRIES];	/* pointer to first toc entry           */
extern toc_entry *ep;		/* pointer to one above last toc entry  */


/*
 * Progress of stuff. This is tracked thru dinfo.dstate.
 *
 */

#define	PARSED	0x01	/* input just been parsed	*/
#define	SIZED	0x02	/* sizes calculated		*/
#define	FITTED	0x04	/* files have had initial fitting into volumes*/
#define	EDITTED	0x08	/* files have been editted	*/

#define	READY_TO_GO	(PARSED|SIZED|FITTED)
#define	GONE	0	/* when it goes to the actual media,	*/
			/* dinfo.dstate is marked GONE		*/

/*
 * helpful defines
 *
 */


#define	IS_TOC(p) ((p)->Type == TOC && \
			(p)->Info.kind == CONTENTS \
			&& strcmp((p)->Name,"XDRTOC") == 0)

/*
 * Function declarations
 */

extern char *newstring();
extern void remove_toc_copies(), remove_entry(), destroy_entry(),
	destroy_all_entries(), dup_entry(), destroy_device();
extern void bell(), errprint(), infoprint();

extern char *malloc();
extern char *realloc();
