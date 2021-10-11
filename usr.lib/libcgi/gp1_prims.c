#ifndef lint
static char	sccsid[] = "@(#)gp1_prims.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
#endif

/*
 * Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc. 
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

#include "cgipriv.h"
#include <pixrect/gp1cmds.h>

extern Outatt  *_cgi_att;

static int      old_resetcnt;
static short   *gp1_addr, *shmvecbase;
static int      pairs_left;
static int      fd;
static struct gp1pr *dmd;
static Gp1_attr *g_gpptr;

static short   *_cgi_gp1_snd_lines();

/* SPARC requires diff' alignment for int's, so get as two shorts.
 *   NOTE: Can't do anything tricky with param' inside of the GetGPInt macro!
 *	   (Like "Getint(x++);" is a NO-NO!)
 */
#ifdef	sparc
#define	GetGPInt(x)	*shmptr++ = (short) ((x)>>16), \
			*shmptr++ = (x) & 0xffff
#else	!sparc
#define	GetGPInt(x)	*((int *) shmptr)++ = (x)
#endif	sparc

_cgi_gp1_pw_polyline(pw, gpptr, npts, pts, mvlist)
Pixwin         *pw;
Gp1_attr       *gpptr;
int             npts;
Ccoor          *pts;
u_char         *mvlist;
{
    register short *shmptr;
    register Ccoor *coordptr;
    register u_char *mvP;
    register int    vertex_pairs_left, nlines;
    int             words_used, roomleft;

Restart:
    g_gpptr = gpptr;
    fd = ((struct gp1pr *) (pw->pw_pixrect->pr_data))->ioctl_fd;
    coordptr = pts;
    switch ((int)mvlist)
    {
    case (int)POLY_DISJOINT:
	nlines = npts >> 1;
	break;
    case (int)POLY_CLOSE:
	nlines = npts;
	break;
    case (int)POLY_DONTCLOSE:
    default:
	nlines = npts - 1;
	break;
    }
    old_resetcnt = gpptr->resetcnt;
    if (gpptr->clpcnt != gpptr->resetcnt)
	/* Offset update now handled by _cgi_gp1_pw_updclplst */
	(void) _cgi_gp1_pw_updclplst(pw, gpptr);
    dmd = gp1_d(pw->pw_pixrect);
    gp1_addr = (short *) dmd->gp_shmem;
    /*
     * This assumes that any GP buffer left allocated was never used, so we can
     * start at the base offset without syncronizing with the GP. 
     */
    while (gpptr->shm_base == 0)
	gpptr->shm_base = gp1_alloc(gp1_addr, 1, &gpptr->bitvec, dmd->minordev, fd);

    gpptr->offset = gpptr->shm_base;
    if (gpptr->attrchg | (gpptr->attrcnt != gpptr->resetcnt))
    {
	if ((words_used = _cgi_gp1_snd_attr(gp1_addr, gpptr->offset, gpptr, fd))
	    < 0)
	    goto Restart;
	gpptr->offset += words_used;
	shmptr = &gp1_addr[gpptr->offset];
	roomleft = 512 - words_used;
    }
    else
    {
	shmptr = &gp1_addr[gpptr->offset];
	roomleft = 512;
    }
    shmptr += 3;		/* skip words for 2 commands and a count */
    shmvecbase = shmptr;
    vertex_pairs_left = (roomleft - 6) >> 3;

    /* Separate loops for efficiency */
    switch ((int)mvlist)
    {
    case (int)POLY_CLOSE:	/* closed polyline */
    case (int)POLY_DONTCLOSE:	/* normal polyline */
	while (--nlines >= 0)
	{
	    GetGPInt(coordptr->x);
	    GetGPInt(coordptr->y);
	    coordptr++;
	    GetGPInt(coordptr->x);
	    GetGPInt(coordptr->y);
	    if (--vertex_pairs_left <= 0)
	    {
		if ((shmptr = _cgi_gp1_snd_lines(shmptr)) == (short *) 0)
		    goto Restart;
		vertex_pairs_left = pairs_left;
	    }
	}
	if (mvlist == POLY_CLOSE)
	{
	    GetGPInt(coordptr->x);
	    GetGPInt(coordptr->y);
	    GetGPInt(pts->x);
	    GetGPInt(pts->y);
	}
	break;
    case (int)POLY_DISJOINT:	/* disjoint polyline */
	while (--nlines >= 0)
	{
	    GetGPInt(coordptr->x);
	    GetGPInt(coordptr->y);
	    coordptr++;
	    GetGPInt(coordptr->x);
	    GetGPInt(coordptr->y);
	    coordptr++;
	    if (--vertex_pairs_left <= 0)
	    {
		if ((shmptr = _cgi_gp1_snd_lines(shmptr)) == (short *) 0)
		    goto Restart;
		vertex_pairs_left = pairs_left;
	    }
	}
	break;
    default:			/* polyline with arbitrary move list */
	/*
	 * This doesn't yet implement the pixwin functionality of closing each
	 * distinct subpolygon (when element 0 of mvlist is non-zero).  This
	 * isn't used by any current callers within CGI. 
	 */
	mvP = mvlist;

	while (--nlines >= 0)
	{
	    if (*++mvP == 0)
	    {
		GetGPInt(coordptr->x);
		GetGPInt(coordptr->y);
		coordptr++;
		GetGPInt(coordptr->x);
		GetGPInt(coordptr->y);
		if (--vertex_pairs_left <= 0)
		{
		    if ((shmptr = _cgi_gp1_snd_lines(shmptr)) == (short *) 0)
			goto Restart;
		    vertex_pairs_left = pairs_left;
		}
	    }
	    else
		coordptr++;
	}
	break;
    }

    /* Send remaining vectors in gp buffer */
    if (shmptr != shmvecbase)
    {
	if (_cgi_gp1_snd_lines(shmptr) == (short *) 0)
	    goto Restart;
    }
    if (old_resetcnt != gpptr->resetcnt)
	goto Restart;
}

static short   *_cgi_gp1_snd_lines(endptr)
short          *endptr;
{
    register short *shmptr;

    /*
     * First, fill in header information before the vectors 
     */
    shmptr = shmvecbase - 3;
    *shmptr++ = GP1_USEFRAME | g_gpptr->statblkindx;	/* frame command */
    if (g_gpptr->cmdver[GP1_CGI_LINE >> 8])
	*shmptr++ = GP1_CGI_LINE;	/* line drawing command */
    else
	*shmptr++ = GP1_CGIVEC;	/* line drawing command */
    *shmptr = (endptr - shmvecbase) >> 3;	/* # of lines */

    shmptr = endptr;
    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
    *shmptr++ = (g_gpptr->bitvec >> 16) & 0xffff;
    *shmptr = g_gpptr->bitvec & 0xffff;
    if (gp1_post(gp1_addr, g_gpptr->offset, fd))
    {
	while (old_resetcnt == g_gpptr->resetcnt)
	    ;
	g_gpptr->offset = g_gpptr->shm_base = 0;
	return ((short *) 0);
    }
    while ((g_gpptr->shm_base = gp1_alloc(gp1_addr, 1, &g_gpptr->bitvec,
					  dmd->minordev, fd)) == 0)
	;
    g_gpptr->offset = g_gpptr->shm_base;
    shmptr = &gp1_addr[g_gpptr->offset];
    pairs_left = (512 - 6) >> 3;
    shmvecbase = shmptr + 3;
    return (shmvecbase);
}
