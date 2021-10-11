/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)pdfio.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#include <stdio.h>

#define	PDFCHUNK	32768	/* words in PDF chunk */

static short *pdfstart, *pdfend;/* pointers to start and end of PDF */
static int pdfptr, pdfnext;	/* PDF read ptr and write ptr */

extern char *malloc();
extern char *realloc();

_core_pdfread(shortcnt, data)
    int shortcnt;
    register short *data;
{				/* read count words from the PDF */
    register short *ptr;
    register int i;

    ptr = &(pdfstart[pdfptr]);
    if (pdfptr + shortcnt > pdfnext) {
	*data = PDFENDSEGMENT;
	return (pdfptr);
    }
    for (i = 0; i < shortcnt; i++)
	*data++ = *ptr++;
    pdfptr += shortcnt;
    return (pdfptr);
}

_core_pdfwrite(shortcnt, data)
    int shortcnt;
    register short *data;
{				/* append count words to the PDF, count must
				 * be <PDFCHUNK */

    register short *ptr;
    register int i;

    ptr = &(pdfstart[pdfnext]);
    if (ptr + shortcnt >= pdfend) {
	short *newstart;
	int size;

	newstart = (short *)realloc((char *)pdfstart,
			 (unsigned int)((pdfend - pdfstart + shortcnt + PDFCHUNK + 1) << 1));
	if (newstart) {
	    size = pdfend - pdfstart + shortcnt + PDFCHUNK;
	    pdfstart = newstart;
	    pdfend = pdfstart + size;
	    ptr = &(pdfstart[pdfnext]);
	} else {
	    (void)fprintf(stderr,
	    "Display list overflow; delete segments before adding more!\n");
	    return (1);
	}
    }
    for (i = 0; i < shortcnt; i++)
	*ptr++ = *data++;
    pdfnext += shortcnt;
    return (pdfnext);
}

_core_pdfskip(shortnum)
    int shortnum;
{
    pdfptr += shortnum;
    return (pdfptr);
}

_core_pdfseek(wordnum, mode, dataptr)
    int wordnum, mode;
    short **dataptr;
{				/* read data into buffers and return ptr into
				 * buffer    */
    if (mode) {
	*dataptr = &(pdfstart[pdfptr + wordnum]);
	pdfptr += wordnum;
    } else {
	*dataptr = &(pdfstart[wordnum]);
	pdfptr = wordnum;
    }
    return (pdfptr);
}

_core_pdfmarkend()
{				/* mark temporary end of segment and backup */
    short ptype = PDFENDSEGMENT;

    (void)_core_pdfwrite(SHORT, &ptype);
    pdfnext--;
}

_core_PDFinit()
{
    int i, words;

    pdfptr = 0;
    pdfnext = 0;
    words = PDFCHUNK;
    for (i = 0; i < 3; i++) {
	pdfstart = (short *) malloc((unsigned int)(words * sizeof(short)));	/* word  array */
	pdfend = pdfstart + words;
	if (pdfstart)
	    break;
	else {
	    (void)fprintf(stderr,
	      "Insufficient disk pages for %D word virtual display list.\n",
		    words);
	    words >>= 1;
	}
    }
    return ((int) pdfstart);
}

_core_PDFclose()
{
    if (pdfstart)
	free((char *)pdfstart);
}

_core_PDFcompress(segptr)	/* compress the PDF */
    segstruc *segptr;
{
    int i;
    short *ptr1, *ptr2;
    segstruc *sptr;

    if (segptr == (segstruc *) 0) {
	pdfnext = 0;
	return (0);
    }
    ptr1 = &(pdfstart[segptr->pdfptr]);
    ptr2 = ptr1 + segptr->segsize;
    for (i = 0; i < (pdfnext - (segptr->pdfptr + segptr->segsize)); i++)
	*ptr1++ = *ptr2++;
    pdfnext -= segptr->segsize;
    for (sptr = &_core_segment[0]; sptr < &_core_segment[SEGNUM]; sptr++) {
	if (sptr->type == EMPTY)
	    break;
	if (sptr->pdfptr > segptr->pdfptr)
	    sptr->pdfptr -= segptr->segsize;
    }
    return (0);
}
