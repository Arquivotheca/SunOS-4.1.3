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
static char sccsid[] = "@(#)inprims.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#include <sys/time.h>
#define NULL 0

extern char backspace_character;

static struct timeval systime, newtime;
static struct timezone zone;

pickstr *_core_check_pick();
keybstr *_core_check_keyboard();
strokstr *_core_check_stroke();
locatstr *_core_check_locator();
valstr *_core_check_valuator();
butnstr *_core_check_button();

get_mouse_state(devclass, devnum, x, y, buttons)
    int devclass, devnum;
    float *x, *y;
    int *buttons;

{
    static char funcname[] = "get_mouse_state";
    pickstr *pickptr;
    locatstr *locatptr;
    strokstr *strokptr;
    valstr *valptr;
    butnstr *butnptr;
    ddargtype ddstruct;
    int (*dev) ();

    dev = 0;
    switch (devclass) {
	case PICK:
	    if ((pickptr = _core_check_pick(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (pickptr->subpick.echosurfp != NULL) {
		dev = pickptr->subpick.devdrive;
		ddstruct.instance = pickptr->subpick.instance;
	    }
	    break;
	case KEYBOARD:
	    _core_errhand(funcname, 105);
	    return (1);
	    break;
	case STROKE:
	    if ((strokptr = _core_check_stroke(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (strokptr->substroke.echosurfp != NULL) {
		dev = strokptr->substroke.devdrive;
		ddstruct.instance = strokptr->substroke.instance;
	    }
	    break;
	case LOCATOR:
	    if ((locatptr = _core_check_locator(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (locatptr->subloc.echosurfp != NULL) {
		dev = locatptr->subloc.devdrive;
		ddstruct.instance = locatptr->subloc.instance;
	    }
	    break;
	case VALUATOR:
	    if ((valptr = _core_check_valuator(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (valptr->subval.echosurfp != NULL) {
		dev = valptr->subval.devdrive;
		ddstruct.instance = valptr->subval.instance;
	    }
	    break;
	case BUTTON:
	    if ((butnptr = _core_check_button(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (butnptr->subbut.echosurfp != NULL) {
		dev = butnptr->subbut.devdrive;
		ddstruct.instance = butnptr->subbut.instance;
	    }
	    break;
	default:
	    _core_errhand(funcname, 105);
	    return (1);
	    break;
    }
    if (dev == 0) {
	*x = 0.0;
	*y = 0.0;
	*buttons = 0;
	return (0);
    }
    ddstruct.opcode = XYREAD;	/* get x,y in ndcspace */
    (*dev) (&ddstruct);
    *x = (float) ddstruct.int1 / MAX_NDC_COORD;
    *y = (float) ddstruct.int2 / MAX_NDC_COORD;
    *buttons = ddstruct.int3;	/* bit 0 = R button, 1 M, 2 L. 1=down, 0 up. */
    return (0);
}

await_any_button(tim, butnum)
    int tim;
    int *butnum;
{
    static char funcname[] = "await_any_button";
    int i, someinit;
    ddargtype ddstruct;
    long elapsed_time;
    butnstr *butnptr;
    locatstr *locatptr;
    pickstr *pickptr;
    strokstr *strokptr;
    valstr *valptr;

    (void) gettimeofday(&systime, &zone);
    *butnum = 0;
    ddstruct.opcode = BUTREAD;
    ddstruct.logical = FALSE;
    someinit = FALSE;
    do {
	for (i = 0; i < BUTTONS; i++) {
	    butnptr = &_core_button[i];
	    if (!(butnptr->subbut.enable & 1))
		continue;
	    someinit = TRUE;
	    ddstruct.int1 = 4 >> i;
	    if (butnptr->subbut.echosurfp != NULL) {
		ddstruct.instance = butnptr->subbut.instance;
		(*butnptr->subbut.devdrive) (&ddstruct);
		if (ddstruct.logical) {
		    *butnum = i + 1;
		    break;
		}
	    } else {
		for (locatptr = &_core_locator[0];
		     locatptr < &_core_locator[LOCATORS] &&
		     ddstruct.logical == FALSE; locatptr++)
		    if ((locatptr->subloc.enable & 1) &&
			(locatptr->subloc.echosurfp != NULL)) {
			ddstruct.instance = locatptr->subloc.instance;
			(*locatptr->subloc.devdrive) (&ddstruct);
		    }
		if (ddstruct.logical) {
		    *butnum = i + 1;
		    break;
		}
		for (pickptr = &_core_pick[0]; pickptr < &_core_pick[PICKS] &&
		     ddstruct.logical == FALSE; pickptr++)
		    if ((pickptr->subpick.enable & 1) && (pickptr->subpick.echosurfp != NULL)) {
			ddstruct.instance = pickptr->subpick.instance;
			(*pickptr->subpick.devdrive) (&ddstruct);
		    }
		if (ddstruct.logical) {
		    *butnum = i + 1;
		    break;
		}
		for (strokptr = &_core_stroker[0];
		     strokptr < &_core_stroker[STROKES] &&
		     ddstruct.logical == FALSE; strokptr++)
		    if ((strokptr->substroke.enable & 1) &&
			(strokptr->substroke.echosurfp != NULL)) {
			ddstruct.instance = strokptr->substroke.instance;
			(*strokptr->substroke.devdrive) (&ddstruct);
		    }
		if (ddstruct.logical) {
		    *butnum = i + 1;
		    break;
		}
		for (valptr = &_core_valuatr[0];
		     valptr < &_core_valuatr[VALUATRS] &&
		     ddstruct.logical == FALSE; valptr++)
		    if ((valptr->subval.enable & 1) &&
			(valptr->subval.echosurfp != NULL)) {
			ddstruct.instance = valptr->subval.instance;
			(*valptr->subval.devdrive) (&ddstruct);
		    }
		if (ddstruct.logical) {
		    *butnum = i + 1;
		    break;
		}
	    }
	}
	if (ddstruct.logical)
	    break;
	if (!someinit) {
	    _core_errhand(funcname, 113);
	    return (1);
	}
	(void) gettimeofday(&newtime, &zone);
	elapsed_time = (newtime.tv_sec - systime.tv_sec) * 1000000 +
	  (newtime.tv_usec - systime.tv_usec);
    } while (elapsed_time < tim);
    return (0);
}

#define PICKBUTTON 0

await_pick(tim, picknum, segnam, pickid)
    int tim;
    int picknum, *segnam, *pickid;
{
/*  await mouse button 1 then return segment and pickid pointed to.       */
    static char funcname[] = "await_pick";
    int newpickid;		/* pickid candidate */
    int x, y;			/* mouse pointer position  */
    int offset, seg;		/* index into segment list */
    int oldhilite;
    int i, j, k;
    int try_for_hit;
    short n, sdopcode, *dummy;
    short highest_detectability;
    long elapsed_time;
    ipt_type p[200], p1;	/* extent boundary pts NDC */
    ipt_type aw[2];		/* aperture window LL UR */
    ipt_type cur_point;		/* current point ... used for polylines */
    ddargtype ddstruct;
    pickstr *pickptr, *_core_check_pick();
    segstruc *hit_segment;

    if ((pickptr = _core_check_pick(funcname, picknum, TRUE)) == 0)
	return (1);
    if (pickptr->subpick.echosurfp == NULL) {
	*segnam = 0;
	*pickid = 0;
	return (0);
    }
    if (!(_core_button[PICKBUTTON].subbut.enable & 1)) {
	ddstruct.opcode = INITBUT;
	ddstruct.int1 = 4 >> PICKBUTTON;
	(*pickptr->subpick.devdrive) (&ddstruct);
    }

/*
 *   get x,y in NDC from pick device
 */
    (void) gettimeofday(&systime, &zone);
    ddstruct.instance = pickptr->subpick.instance;
    do {			/* until timeout, check pickbutton */
	ddstruct.opcode = BUTREAD;	/* has pickbutton been hit? */
	ddstruct.int1 = 4 >> PICKBUTTON;
	(*pickptr->subpick.devdrive) (&ddstruct);
	if (ddstruct.logical)
	    break;
	(void) gettimeofday(&newtime, &zone);
	elapsed_time = (newtime.tv_sec - systime.tv_sec) * 1000000 +
	  (newtime.tv_usec - systime.tv_usec);
    } while (elapsed_time < tim);

    if (!(_core_button[PICKBUTTON].subbut.enable & 1)) {
	ddstruct.opcode = TERMBUT;
	ddstruct.int1 = 4 >> PICKBUTTON;
	(*pickptr->subpick.devdrive) (&ddstruct);
    }
    *segnam = 0;
    *pickid = 0;
    if (!ddstruct.logical)
	return (0);

    ddstruct.opcode = XYREAD;	/* pickbutton hit,get x,y in ndcsp */
    (*pickptr->subpick.devdrive) (&ddstruct);
    x = ddstruct.int1;
    y = ddstruct.int2;

/* setup aperture window */
    aw[0].x = x - pickptr->aperture;
    if (aw[0].x < 0)
	aw[0].x = 0;
    aw[0].y = y - pickptr->aperture;
    if (aw[0].y < 0)
	aw[0].y = 0;
    aw[1].x = x + pickptr->aperture;
    if (aw[1].x > MAX_NDC_COORD)
	aw[1].x = MAX_NDC_COORD;
    aw[1].y = y + pickptr->aperture;
    if (aw[1].y > MAX_NDC_COORD)
	aw[1].y = MAX_NDC_COORD;

/*
 * Find those detectable segments and pickids closest to the mouse ptr 
 */
    _core_critflag++;
    highest_detectability = 0;
    for (seg = 0; seg < SEGNUM && _core_segment[seg].type != EMPTY; seg++) {
	/* for all segs */
	if (_core_segment[seg].type >= RETAIN &&
	    _core_segment[seg].segats.detectbl &&	/* if detectable */
	    _core_segment[seg].segats.visbilty) {	/* and visible   */

	    j = FALSE;
	    for (i = 0; i < _core_segment[seg].vsurfnum; i++)
		if (_core_segment[seg].vsurfptr[i] == pickptr->subpick.echosurfp) {
		    j = TRUE;
		    break;
		}
	    if (!j)
		continue;

	    /* if no xform ... take seg bounding square as is */
	    if (_core_segment[seg].idenflag) {
		p[0].x = _core_segment[seg].bndbox_min.x;
		p[0].y = _core_segment[seg].bndbox_min.y;
		p[1].x = _core_segment[seg].bndbox_min.x;
		p[1].y = _core_segment[seg].bndbox_max.y;
		p[2].x = _core_segment[seg].bndbox_max.x;
		p[2].y = _core_segment[seg].bndbox_max.y;
		p[3].x = _core_segment[seg].bndbox_max.x;
		p[3].y = _core_segment[seg].bndbox_min.y;
	    }
	    /* else calculate bounding square from transformed box */
	    else {
		int minx = MAX_NDC_COORD;
		int miny = MAX_NDC_COORD;
		int maxx = -MAX_NDC_COORD;
		int maxy = -MAX_NDC_COORD;

		p[0].x = _core_segment[seg].bndbox_min.x;
		p[0].y = _core_segment[seg].bndbox_min.y;
		p[0].z = _core_segment[seg].bndbox_min.z;
		p[1].x = _core_segment[seg].bndbox_min.x;
		p[1].y = _core_segment[seg].bndbox_min.y;
		p[1].z = _core_segment[seg].bndbox_max.z;
		p[2].x = _core_segment[seg].bndbox_min.x;
		p[2].y = _core_segment[seg].bndbox_max.y;
		p[2].z = _core_segment[seg].bndbox_min.z;
		p[3].x = _core_segment[seg].bndbox_min.x;
		p[3].y = _core_segment[seg].bndbox_max.y;
		p[3].z = _core_segment[seg].bndbox_max.z;
		p[4].x = _core_segment[seg].bndbox_max.x;
		p[4].y = _core_segment[seg].bndbox_min.y;
		p[4].z = _core_segment[seg].bndbox_min.z;
		p[5].x = _core_segment[seg].bndbox_max.x;
		p[5].y = _core_segment[seg].bndbox_min.y;
		p[5].z = _core_segment[seg].bndbox_max.z;
		p[6].x = _core_segment[seg].bndbox_max.x;
		p[6].y = _core_segment[seg].bndbox_max.y;
		p[6].z = _core_segment[seg].bndbox_min.z;
		p[7].x = _core_segment[seg].bndbox_max.x;
		p[7].y = _core_segment[seg].bndbox_max.y;
		p[7].z = _core_segment[seg].bndbox_max.z;

		for (i = 0; i < 8; i++) {
		    _core_imxfrm3(&_core_segment[seg], &p[i], &p1);
		    if (p1.x < minx)
			minx = p1.x;
		    if (p1.x > maxx)
			maxx = p1.x;
		    if (p1.y < miny)
			miny = p1.y;
		    if (p1.y > maxy)
			maxy = p1.y;
		}
		p[0].x = minx;
		p[0].y = miny;
		p[1].x = minx;
		p[1].y = maxy;
		p[2].x = maxx;
		p[2].y = maxy;
		p[3].x = maxx;
		p[3].y = miny;
	    }

	    if (_core_test_for_hit(x, y, aw, 4, p)) {

		offset = _core_segment[seg].pdfptr;
		(void)_core_pdfseek(offset, 0, &dummy);
		while (TRUE) {
		    (void)_core_pdfread(SHORT, &sdopcode);
		    if (sdopcode == PDFENDSEGMENT)
			break;
		    try_for_hit = FALSE;
		    switch (sdopcode) {
			case PDFMOVE:
			    (void)_core_pdfread(FLOAT * 3, (short *) &cur_point);
			    break;
			case PDFLINE:
			    (void)_core_pdfread(FLOAT * 3, (short *) &p[1]);
			    p[0] = cur_point;
			    cur_point = p[1];
			    n = 2;
			    try_for_hit = TRUE;
			    break;
			case PDFTEXT:
			    (void)_core_pdfread(SHORT, &n);
			    (void)_core_pdfskip((n+1)>>1);
			    break;
			case PDFMARKER:
			    (void)_core_pdfskip(FLOAT);
			    break;
			case PDFBITMAP:
			    (void)_core_pdfread(FLOAT, (short *) &i);	/* width  */
			    (void)_core_pdfread(FLOAT, (short *) &j);	/* height */
			    (void)_core_pdfread(FLOAT, (short *) &k);	/* depth  */
			    if (k == 1)
				j = ((i + 15) >> 4) * j * SHORT;
			    else if (k == 8)
				j = i * j;
			    else
				j = i * j * 3;
			    (void)_core_pdfskip(j);
			    break;
			case PDFPOL2:
			    (void)_core_pdfread(SHORT, &n);
			    (void)_core_pdfskip(FLOAT * n * 4);
			    break;
			case PDFPOL3:
			    (void)_core_pdfread(SHORT, &n);
			    (void)_core_pdfskip(FLOAT * n * 8);
			    break;
			case PDFLCOLOR:
			    (void)_core_pdfskip(FLOAT);
			    break;
			case PDFFCOLOR:
			    (void)_core_pdfskip(FLOAT);
			    break;
			case PDFTCOLOR:
			    (void)_core_pdfskip(FLOAT);
			    break;
			case PDFLINESTYLE:
			    (void)_core_pdfskip(FLOAT);
			    break;
			case PDFPISTYLE:
			    (void)_core_pdfskip(FLOAT);
			    break;
			case PDFPESTYLE:
			    (void)_core_pdfskip(FLOAT);
			    break;
			case PDFLINEWIDTH:
			    (void)_core_pdfskip(FLOAT);
			    break;
			case PDFFONT:
			    (void)_core_pdfskip(FLOAT);
			    break;
			case PDFPEN:
			    (void)_core_pdfskip(FLOAT);
			    break;
			case PDFSPACE:
			    (void)_core_pdfskip(FLOAT * 3);
			    break;
			case PDFPATH:
			    (void)_core_pdfskip(FLOAT * 3);
			    break;
			case PDFUP:
			    (void)_core_pdfskip(FLOAT * 3);
			    break;
			case PDFCHARJUST:
			    (void)_core_pdfskip(FLOAT);
			    break;
			case PDFCHARQUALITY:
			    (void)_core_pdfskip(FLOAT);
			    break;
			case PDFVWPORT:
			    (void)_core_pdfskip(FLOAT * 6);
			    break;
			case PDFROP:
			    (void)_core_pdfskip(FLOAT);
			    break;
			case PDFPICKID:
			    (void)_core_pdfread(FLOAT, (short *) &newpickid);
			    /* get pick extents */
			    (void)_core_pdfread(SHORT, &n);
			    for (i = 0; i < n; i++)
				/* read extent bounds */
				(void)_core_pdfread(FLOAT * 3, (short *) &p[i]);
			    if (n)
				try_for_hit = TRUE;
			    break;
			default:
			    break;
		    }

		    if (try_for_hit) {
			if (!_core_segment[seg].idenflag)
			    for (i = 0; i < n; i++) {
				/* apply image transform */
				_core_imxfrm3(&_core_segment[seg], &p[i], &p1);
				p[i] = p1;
			    }
			if (_core_test_for_hit(x, y, aw, n, p)) {
			    if ((&_core_segment[seg])->segats.detectbl >=
				highest_detectability) {
				highest_detectability = (&_core_segment[seg])->segats.detectbl;
				hit_segment = &_core_segment[seg];
				*segnam = hit_segment->segname;
				*pickid = newpickid;
			    }
			}
		    }
		}
	    }
	}
    }

    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle) ();

    if (*segnam != 0) {
	if (pickptr->subpick.echo == 1) {	/* echo by blinking segment */
	    oldhilite = hit_segment->segats.highlght;
	    set_segment_highlighting(*segnam, TRUE);
	    set_segment_highlighting(*segnam, oldhilite);
	}
    }
    return (0);
}

_core_test_for_hit(x, y, aw, n, p)
    int x, y, n;
    ipt_type aw[2], p[200];

{
    int i, j, testval, under, count;

    for (i = 0; i < n; i++) {	/* check for any point inside window */
	if ((p[i].x >= aw[0].x) && (p[i].x <= aw[1].x) &&
	    (p[i].y >= aw[0].y) && (p[i].y <= aw[1].y)) {
	    return 1;
	}
    }

    for (i = 0; i < n; i++) {	/* for each point in bounding polygon */
	j = i + 1;
	if (j == n)
	    j = 0;

	if ((p[i].x >= aw[0].x) && (p[i].x <= aw[1].x) &&
	    (p[i].y >= aw[0].y) && (p[i].y <= aw[1].y)) {
	    return 1;
	}
	if ((p[i].x < aw[0].x) ^ (p[j].x < aw[0].x) &&
	    (testval = p[i].y + (p[j].y - p[i].y) * (aw[0].x - p[i].x) /
	     (p[j].x - p[i].x)) >= aw[0].y && testval <= aw[1].y) {
	    return 1;
	}
	if ((p[i].x < aw[1].x) ^ (p[j].x < aw[1].x) &&
	    (testval = p[i].y + (p[j].y - p[i].y) * (aw[1].x - p[i].x) /
	     (p[j].x - p[i].x)) >= aw[0].y && testval <= aw[1].y) {
	    return 1;
	}
	if ((p[i].y < aw[0].y) ^ (p[j].y < aw[0].y) &&
	    (testval = p[i].x + (p[j].x - p[i].x) * (aw[0].y - p[i].y) /
	     (p[j].y - p[i].y)) >= aw[0].x && testval <= aw[1].x) {
	    return 1;
	}
	if ((p[i].y < aw[1].y) ^ (p[j].y < aw[1].y) &&
	    (testval = p[i].x + (p[j].x - p[i].x) * (aw[1].y - p[i].y) /
	     (p[j].y - p[i].y)) >= aw[0].x && testval <= aw[1].x) {
	    return 1;
	}
    }

    under = FALSE;		/* last try ... see if center point in
				 * polygon */
    count = 0;
    for (i = 0; i < n; i++) {	/* test for hit */
	j = i + 1;
	if (j == n)
	    j = 0;
	if (p[j].x > p[i].x)
	    under = x <= p[j].x && x >= p[i].x &&
	      y <= (p[i].y + (p[j].y - p[i].y) *
		    (x - p[i].x) / (p[j].x - p[i].x));
	else if (p[j].x < p[i].x)
	    under = x <= p[i].x && x >= p[j].x &&
	      y <= (p[i].y + (p[j].y - p[i].y) *
		    (p[i].x - x) / (p[i].x - p[j].x));
	else
	    under = FALSE;
	if (under)
	    count++;
    }
    if (count & 1) {
	return 1;
    }
    return 0;			/* not hit */
}

await_keyboard(tim, keynum, string, length)
    int tim, keynum;
    char *string;
    int *length;
{
    static char funcname[] = "await_keyboard";
    int count, lastcount, ipos;
    char ch;
    viewsurf *surfp;
    ddargtype ddstruct;
    long elapsed_time;
    keybstr *keybptr, *_core_check_keyboard();

    if ((keybptr = _core_check_keyboard(funcname, keynum, TRUE)) == 0)
	return (1);
    keynum--;
    (void) gettimeofday(&systime, &zone);
    *string = '\0';
    count = 0;
    lastcount = 0;
    if (keybptr->subkey.echo > 0)
	surfp = keybptr->subkey.echosurfp;
    else
	surfp = 0;
    if (surfp) {
	_core_critflag++;
	ddstruct.instance = surfp->vsurf.instance;
	ddstruct.opcode = SETTCOL;	/* set xor on */
	ddstruct.int1 = 255;
	ddstruct.int2 = XORROP;
	ddstruct.int3 = FALSE;
	(*surfp->vsurf.dd) (&ddstruct);
    }
    ipos = keybptr->initpos;
    _core_devecho(KEYBOARD, keynum, 0, 0, keybptr->initstring, 0.0);
    ddstruct.opcode = KEYREAD;
    do {			/* read any chars */
	ddstruct.int1 = 1;	/* one at a time */
	ddstruct.ptr1 = string + count;	/* accumul in string */
	(*keybptr->subkey.devdrive) (&ddstruct);
	count += ddstruct.int1;

	ch = *(string + (count - 1));
	if (count && ch == backspace_character) { /* back space to erase char */
	    count--;
	    lastcount--;
	    *(string + count) = '\0';
	    if (lastcount < count)
		_core_devecho(KEYBOARD, keynum, 0, 0, string + lastcount,
			      (float) (ipos + lastcount));
	    count = lastcount;
	}
	if (count && (ch == '\r' || ch == '\n'	/* CR or LF to quit */
		      || count >= keybptr->bufsize)) {
	    *(string + (count - 1)) = '\n';
	    *(string + count) = '\0';
	    if (lastcount < count)
		_core_devecho(KEYBOARD, keynum, 0, 0, string + lastcount,
			      (float) (ipos + lastcount));
	    break;
	}
	*(string + count) = '\0';	/* write new chars */
	if (lastcount < count)
	    _core_devecho(KEYBOARD, keynum, 0, 0, string + lastcount,
			  (float) (ipos + lastcount));
	lastcount = count;

	(void) gettimeofday(&newtime, &zone);
	elapsed_time = (newtime.tv_sec - systime.tv_sec) * 1000000 +
	  (newtime.tv_usec - systime.tv_usec);
    } while (elapsed_time < tim);
    /* erase prompt */
    _core_devecho(KEYBOARD, keynum, 0, 0, keybptr->initstring, 0.0);
    /* erase string */
    _core_devecho(KEYBOARD, keynum, 0, 0, string, (float) ipos);

    if (surfp) {
	ddstruct.instance = surfp->vsurf.instance;
	ddstruct.opcode = SETTCOL;	/* restore xor to original */
	ddstruct.int1 = _core_current.textindx;
	ddstruct.int2 = (_core_xorflag) ? XORROP : _core_current.rasterop;
	ddstruct.int3 = FALSE;
	(*surfp->vsurf.dd) (&ddstruct);
	if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	    (*_core_sighandle) ();
    }
    *length = count;
    return (0);
}

#define STKBUTTON 2
await_stroke_2(tim, strokenum, arrsize, xarray, yarray, numxy)
    int tim, strokenum, arrsize, *numxy;
    float xarray[], yarray[];

{
    static char funcname[] = "await_stroke_2";
    int oldx, oldy, cx, cy;
    ddargtype ddstruct;
    long elapsed_time;
    register strokstr *strokptr;
    strokstr *_core_check_stroke();

    if ((strokptr = _core_check_stroke(funcname, strokenum, TRUE)) == 0)
	return (1);
    *numxy = 0;
    if (strokptr->substroke.echosurfp == NULL)
	return (0);
    if (!(_core_button[STKBUTTON].subbut.enable & 1)) {
	ddstruct.opcode = INITBUT;
	ddstruct.int1 = 4 >> STKBUTTON;
	(*strokptr->substroke.devdrive) (&ddstruct);
    }
    (void) gettimeofday(&systime, &zone);
    ddstruct.opcode = XYREAD;	/* get x,y in ndcspace */
    ddstruct.instance = strokptr->substroke.instance;
    (*strokptr->substroke.devdrive) (&ddstruct);
    oldx = ddstruct.int1;
    oldy = ddstruct.int2;
    do {			/* await button, and echo */
	ddstruct.opcode = XYREAD;	/* if pickbutton hit,get x,y in ndcsp */
	(*strokptr->substroke.devdrive) (&ddstruct);
	cx = ddstruct.int1;
	cy = ddstruct.int2;
	ddstruct.opcode = BUTREAD;
	ddstruct.int1 = 4 >> STKBUTTON;
	(*strokptr->substroke.devdrive) (&ddstruct);
	if (ddstruct.logical)
	    break;
	if (abs(oldx - cx) >= strokptr->distance
	    || abs(oldy - cy) >= strokptr->distance) {
	    if (*numxy < arrsize) {
		xarray[*numxy] = (float) cx / MAX_NDC_COORD;
		yarray[*numxy] = (float) cy / MAX_NDC_COORD;
		(*numxy)++;
		oldx = cx;
		oldy = cy;
	    } else
		break;
	}
	(void) gettimeofday(&newtime, &zone);
	elapsed_time = (newtime.tv_sec - systime.tv_sec) * 1000000 +
	  (newtime.tv_usec - systime.tv_usec);
    } while (elapsed_time < tim);
    if (!(_core_button[STKBUTTON].subbut.enable & 1)) {
	ddstruct.opcode = TERMBUT;
	ddstruct.int1 = 4 >> STKBUTTON;
	(*strokptr->substroke.devdrive) (&ddstruct);
    }
    return (0);
}

await_any_button_get_locator_2(tim, locnum, butnum, x, y)
    int tim, locnum, *butnum;
    float *x, *y;
{
    static char funcname[] = "await_any_button_get_locator_2";
    int i;
    int oldx, oldy, cx, cy;
    ddargtype ddstruct;
    long elapsed_time;
    register locatstr *locatptr;
    locatstr *_core_check_locator();
    butnstr *butnptr;

    if ((locatptr = _core_check_locator(funcname, locnum, TRUE)) == 0)
	return (1);
    locnum--;
    i = FALSE;
    for (butnptr = &_core_button[0]; butnptr < &_core_button[BUTTONS]; butnptr++)
	if (butnptr->subbut.enable & 1) {
	    i = TRUE;
	    break;
	}
    if (!i) {
	_core_errhand(funcname, 113);
	return (1);
    }
    if (locatptr->subloc.echosurfp == NULL) {
	*butnum = 0;
	*x = 0;
	*y = 0;
	return (0);
    }
    (void) gettimeofday(&systime, &zone);
    *butnum = 0;
    if (locatptr->subloc.echo > 1)
	_core_critflag++;
    ddstruct.opcode = XYREAD;	/* get x,y in ndcspace */
    ddstruct.instance = locatptr->subloc.instance;
    (*locatptr->subloc.devdrive) (&ddstruct);
    oldx = ddstruct.int1;
    oldy = ddstruct.int2;
    _core_devecho(LOCATOR, locnum, oldx, oldy, " ", 0.);
    do {			/* await button, and echo */
	ddstruct.opcode = BUTREAD;
	for (i = 0; i < BUTTONS; i++) {
	    ddstruct.int1 = 4 >> i;
	    (*locatptr->subloc.devdrive) (&ddstruct);
	    if (ddstruct.logical) {
		*butnum = i + 1;
	    }
	}
	ddstruct.opcode = XYREAD;	/* get x,y in ndcspace */
	(*locatptr->subloc.devdrive) (&ddstruct);
	cx = ddstruct.int1;
	cy = ddstruct.int2;
	if (oldx != cx || oldy != cy) {
	    _core_devecho(LOCATOR, locnum, oldx, oldy, " ", 0.);
	    _core_devecho(LOCATOR, locnum, cx, cy, " ", 0.);
	}
	oldx = cx;
	oldy = cy;
	if (*butnum)
	    break;
	(void) gettimeofday(&newtime, &zone);
	elapsed_time = (newtime.tv_sec - systime.tv_sec) * 1000000 +
	  (newtime.tv_usec - systime.tv_usec);
    } while (elapsed_time < tim);
    _core_devecho(LOCATOR, locnum, oldx, oldy, " ", 0.);
    if (locatptr->subloc.echo > 1)
	if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	    (*_core_sighandle) ();
    *x = (float) cx / MAX_NDC_COORD;
    *y = (float) cy / MAX_NDC_COORD;
    return (0);
}

await_any_button_get_valuator(tim, valnum, butnum, val)
    int tim, valnum, *butnum;
    float *val;
{
    static char funcname[] = "await_any_button_get_valuator";
    int i;
    float mm, bb;
    ddargtype ddstruct;
    long elapsed_time;
    register valstr *valptr;
    valstr *_core_check_valuator();
    butnstr *butnptr;

    if ((valptr = _core_check_valuator(funcname, valnum, TRUE)) == 0)
	return (1);
    valnum--;
    i = FALSE;
    for (butnptr = &_core_button[0]; butnptr < &_core_button[BUTTONS]; butnptr++)
	if (butnptr->subbut.enable & 1) {
	    i = TRUE;
	    break;
	}
    if (!i) {
	_core_errhand(funcname, 113);
	return (1);
    }
    if (valptr->subval.echosurfp == NULL) {
	*butnum = 0;
	*val = 0.0;
	return (0);
    }
    (void) gettimeofday(&systime, &zone);
    *butnum = 0;
    *val = valptr->vlinit;
    mm = (valptr->vlmax - valptr->vlmin) / (float) _core_ndcspace[0];
    bb = valptr->vlmin;
    if (valptr->subval.echo > 0)
	_core_critflag++;
    ddstruct.instance = valptr->subval.instance;
    do {			/* await button, and echo */
	for (i = 0; i < BUTTONS; i++) {
	    ddstruct.opcode = BUTREAD;
	    ddstruct.int1 = 4 >> i;
	    (*valptr->subval.devdrive) (&ddstruct);
	    if (ddstruct.logical)
		*butnum = i + 1;
	}
	if (*butnum) {
	    ddstruct.opcode = XYREAD;
	    (*valptr->subval.devdrive) (&ddstruct);
	    *val = mm * (float) ddstruct.int1 + bb;
	    _core_devecho(VALUATOR, valnum, 0, 0, " ", *val);
	    break;
	}
	(void) gettimeofday(&newtime, &zone);
	elapsed_time = (newtime.tv_sec - systime.tv_sec) * 1000000 +
	  (newtime.tv_usec - systime.tv_usec);
    } while (elapsed_time < tim);
    _core_devecho(VALUATOR, valnum, 0, 0, " ", *val);
    if (valptr->subval.echo > 0)
	if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	    (*_core_sighandle) ();
    return (0);
}
