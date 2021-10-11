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
static char sccsid[] = "@(#)savesegment.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"
#include <sys/file.h>
#include <stdio.h>
#define NULL 0
#define OLDSEGMENT 0
#define NEWSEGMENT 1

/*
 * These routines save and restore a SunCore segment on disk file.
 */

/* need this in case we restore an old segment */
typedef struct {		/* 1.1 Segment descriptor */
    int segname;
    short type;			/* noretain,retain,xlate,xform */
    segattr segats;
    int vsurfnum;		/* numbr of view surfs to display */
    viewsurf *vsurfptr[MAXVSURF];
    int pdfptr;			/* byte ptr into pseudo display file */
    int redraw;			/* true if segment needs redraw */
    int segsize;
} oldsegstruc;

#define CHARSEGBUFSZ	1024	/* Number of chars in buffer */
#define SHORTSEGBUFSZ	512	/* Number of shorts in buffer */

save_segment(segnum, filename)
    int segnum;
    char *filename;
{
    int segfid, i;
    segstruc *sptr;
    int errnum;
    char *funcname;
    short *buf;
    short *dummy;

    errnum = 0;
    if ((segfid = open(filename, O_WRONLY | O_CREAT | O_TRUNC, PMODE)) == -1) {
	(void)fprintf(stderr, "save_segment: can't open %s\n", filename);
	return (1);
    }
    _core_critflag++;
    /* copy the segment */
    for (sptr = &_core_segment[0]; sptr < &_core_segment[SEGNUM]; sptr++) {
	if ((sptr->type != DELETED) && (segnum == sptr->segname)) {
	    if (sptr == _core_openseg) {	/* seg must be closed */
		errnum = 1;
		break;
	    }
	    (void)write(segfid, "SEGM", 4);	/* write file id */
	    /* 
	     * Write segment header in pieces to avoid architectural 
	     * differeces in structure sizes.  Must be able to read and
	     * segments on different machines.
	     */
	    (void)write(segfid, (char *)&sptr->segname, sizeof(int));
	    (void)write(segfid, (char *)&sptr->type, sizeof(short));
	    (void)write(segfid, (char *)&sptr->segats, sizeof(segattr));
	    (void)write(segfid, (char *)&sptr->vsurfnum, sizeof(int));
	    (void)write(segfid, (char *)&sptr->vsurfptr[0],MAXVSURF*sizeof(int));
	    (void)write(segfid, (char *)&sptr->pdfptr, sizeof(int));
	    (void)write(segfid, (char *)&sptr->redraw, sizeof(int));
	    (void)write(segfid, (char *)&sptr->segsize, sizeof(int));
	    (void)write(segfid, (char *)&sptr->imxform[0][0], 16*sizeof(int));
	    (void)write(segfid, (char *)&sptr->idenflag, sizeof(int));
	    (void)write(segfid, (char *)&sptr->bndbox_min, sizeof(ipt_type));
	    (void)write(segfid, (char *)&sptr->bndbox_max, sizeof(ipt_type));
	    buf = (short *) malloc(CHARSEGBUFSZ);
	    (void)_core_pdfseek(sptr->pdfptr, 0, &dummy);	/* write seg data */
	    for (i = sptr->segsize; i >= SHORTSEGBUFSZ; i -= SHORTSEGBUFSZ) {
		(void)_core_pdfread(SHORTSEGBUFSZ, buf);
		(void)write(segfid, (char *)buf, CHARSEGBUFSZ);
	    }
	    (void)_core_pdfread(i, buf);
	    (void)write(segfid, (char *)buf, i<<1);
	    free((char *)buf);
	    break;
	}
	if (sptr->type == EMPTY) {
	    funcname = "save_segment";
	    errnum = 29;
	    _core_errhand(funcname, errnum);
	    break;
	}
    }
    if (sptr == &_core_segment[SEGNUM]) {
	funcname = "save_segment";
	errnum = 29;
	_core_errhand(funcname, errnum);
    }
    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle) ();
    (void)close(segfid);		/* close disk file */
    return (errnum);
}

restore_segment(segname, filename)
    int segname;
    char *filename;
{
    int segfid, i;
    segstruc *sptr;
    viewsurf *surfp;
    char segfileid[4];
    short *buf;
    char *funcname;
    int visible_flag;
    int errnum;
    int segment_type;

    errnum = 0;
    funcname = "restore_segment";
    if (!segname) {		/* seg number can't be 0 */
	_core_errhand(funcname, 29);
	return (1);
    }
    if (_core_osexists) {	/* a segment is already open */
	_core_errhand(funcname, 8);
	return (1);
    }
    if ((segfid = open(filename, O_RDONLY, PMODE)) == -1) {
	(void)fprintf(stderr, "restore_segment: can't open %s\n", filename);
	return (1);
    }
    (void)read(segfid, segfileid, 4);	/* read file id */
    if (segfileid[0] == 'S' && segfileid[1] == 'E' &&
	segfileid[2] == 'G' && segfileid[3] == 'M') {
	segment_type = NEWSEGMENT;
    } else if (segfileid[0] == 'S' && segfileid[1] == 'U' &&
	       segfileid[2] == 'N' && segfileid[3] == 'C') {
	segment_type = OLDSEGMENT;
    } else {
	(void)fprintf(stderr, "restore_segment: %s is not a segment file\n", filename);
	(void)close(segfid);
	return (1);
    }

    /*
     * create the new segment from disk file 
     */
    for (sptr = &_core_segment[0]; sptr < &_core_segment[SEGNUM]; sptr++) {
	if (sptr->type == EMPTY) {
	    if (_core_openseg == NULL)
		_core_openseg = sptr;	/* find available segstruc */
	    break;
	} else if (sptr->type == DELETED) {
	    if (_core_openseg == NULL)
		_core_openseg = sptr;	/* find available segstruc */
	} else if (segname == sptr->segname) {
	    _core_errhand(funcname, 31);
	    _core_openseg = NULL;
	    (void)close(segfid);
	    return (2);
	}
    }
    if (_core_openseg == NULL) {
	_core_errhand(funcname, 16);
	return (16);
    }
    _core_critflag++;
    errnum = 33;		/* assume error */
    _core_openseg->vsurfnum = 0;/* zero number of selected view surfs */
    for (surfp = &_core_surface[0]; surfp < &_core_surface[MAXVSURF]; surfp++) {
	if (surfp->selected) {
	    errnum = 0;		/* no error because one is selected */
	    /* Put pointers to selected viewsurfaces in segment. */
	    _core_openseg->vsurfptr[(_core_openseg->vsurfnum)++] = surfp;
	}
    }
    if (errnum == 33) {		/* no surfaces are selected */
	_core_errhand(funcname, errnum);
	_core_openseg = NULL;
	(void)close(segfid);
	if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	    (*_core_sighandle) ();
	return (4);
    }
    _core_ndcset |= 1;
    _core_openseg->segname = segname;

    /*
     * SET SEGMENT DYNAMIC ATTRIBUTES FROM DISK FILE SEGMENT ATTRIBUTES 
     */
    buf = (short *) malloc(CHARSEGBUFSZ);
    sptr = (segstruc *) buf;
    if (segment_type == NEWSEGMENT) {
	    /* 
	     * Read new segment header in pieces to avoid architectural 
	     * differeces in structure sizes.  Must be able to read and
	     * segments on different machines.
	     */
	    (void)read(segfid, (char *)&sptr->segname, sizeof(int));
	    (void)read(segfid, (char *)&sptr->type, sizeof(short));
	    (void)read(segfid, (char *)&sptr->segats, sizeof(segattr));
	    (void)read(segfid, (char *)&sptr->vsurfnum, sizeof(int));
	    (void)read(segfid, (char *)&sptr->vsurfptr[0],MAXVSURF*sizeof(int));
	    (void)read(segfid, (char *)&sptr->pdfptr, sizeof(int));
	    (void)read(segfid, (char *)&sptr->redraw, sizeof(int));
	    (void)read(segfid, (char *)&sptr->segsize, sizeof(int));
	    (void)read(segfid, (char *)&sptr->imxform[0][0], 16*sizeof(int));
	    (void)read(segfid, (char *)&sptr->idenflag, sizeof(int));
	    (void)read(segfid, (char *)&sptr->bndbox_min, sizeof(ipt_type));
	    (void)read(segfid, (char *)&sptr->bndbox_max, sizeof(ipt_type));
    } else {
	    /* 
	     * Read old segment header in pieces to avoid architectural 
	     * differeces in structure sizes.  Must be able to read and
	     * segments on different machines.
	     */
	    (void)read(segfid, (char *)&sptr->segname, sizeof(int));
	    (void)read(segfid, (char *)&sptr->type, sizeof(short));
	    (void)read(segfid, (char *)&sptr->segats, sizeof(segattr));
	    (void)read(segfid, (char *)&sptr->vsurfnum, sizeof(int));
	    (void)read(segfid, (char *)&sptr->vsurfptr[0],MAXVSURF*sizeof(int));
	    (void)read(segfid, (char *)&sptr->pdfptr, sizeof(int));
	    (void)read(segfid, (char *)&sptr->redraw, sizeof(int));
	    (void)read(segfid, (char *)&sptr->segsize, sizeof(int));
    }

    _core_openseg->type = sptr->type;
    _core_openseg->segats.visbilty = sptr->segats.visbilty;
    if (_core_openseg->segats.visbilty)
	visible_flag = TRUE;
    else
	visible_flag = FALSE;
    _core_openseg->segats.detectbl = sptr->segats.detectbl;
    _core_openseg->segats.highlght = sptr->segats.highlght;
    _core_openseg->segats.scale[0] = sptr->segats.scale[0];
    _core_openseg->segats.scale[1] = sptr->segats.scale[1];
    _core_openseg->segats.scale[2] = sptr->segats.scale[2];
    _core_openseg->segats.translat[0] = sptr->segats.translat[0];
    _core_openseg->segats.translat[1] = sptr->segats.translat[1];
    _core_openseg->segats.translat[2] = sptr->segats.translat[2];
    _core_openseg->segats.rotate[0] = sptr->segats.rotate[0];
    _core_openseg->segats.rotate[1] = sptr->segats.rotate[1];
    _core_openseg->segats.rotate[2] = sptr->segats.rotate[2];
    _core_setmatrix(_core_openseg);
    _core_openseg->pdfptr = _core_pdfwrite(0, buf);	/* next pdf position */
    _core_openseg->redraw = TRUE;
    _core_openseg->segsize = sptr->segsize;
    if (segment_type == NEWSEGMENT) {	/* restore segment bndbox if newseg */
	_core_openseg->bndbox_min.x = sptr->bndbox_min.x;
	_core_openseg->bndbox_min.y = sptr->bndbox_min.y;
	_core_openseg->bndbox_min.z = sptr->bndbox_min.z;
	_core_openseg->bndbox_max.x = sptr->bndbox_max.x;
	_core_openseg->bndbox_max.y = sptr->bndbox_max.y;
	_core_openseg->bndbox_max.z = sptr->bndbox_max.z;
    } else {			/* set up segment bndbox to be max dimensions
				 * if od type of seg */
	_core_openseg->bndbox_min.x = -MAX_NDC_COORD;
	_core_openseg->bndbox_min.y = -MAX_NDC_COORD;
	_core_openseg->bndbox_min.z = -MAX_NDC_COORD;
	_core_openseg->bndbox_max.x = MAX_NDC_COORD;
	_core_openseg->bndbox_max.y = MAX_NDC_COORD;
	_core_openseg->bndbox_max.z = MAX_NDC_COORD;
    }
    /* transfer seg data */
    for (i = ((_core_openseg->segsize<<1) - 1); i >= CHARSEGBUFSZ; i -= CHARSEGBUFSZ) {
	(void)read(segfid, (char *)buf, CHARSEGBUFSZ);
	(void)_core_pdfwrite(SHORTSEGBUFSZ, buf);
    }
    (void)read(segfid, (char *)buf, i);
    (void)_core_pdfwrite(i>>1, buf);	/* seg remainder without PDFENDSEG */
    free((char *)buf);

    /*
     * SET SEGMENT STATE AND OUTPUT PRIMITIVE FLAGS 
     */

    _core_prevseg = TRUE;
    _core_osexists = TRUE;
    _core_segnum++;
    close_retained_segment();

    if (visible_flag) {
	set_segment_visibility(segname, FALSE);
	set_segment_visibility(segname, TRUE);
    }
    (void)close(segfid);		/* close the file */
    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle) ();
    return (0);
}
