#ifndef lint
static char	sccsid[] = "@(#)gp1_attr.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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

Gstate _cgi_state;

_cgi_gp1_attr_init(pw, attrptr)
Pixwin         *pw;
Gp1_attr      **attrptr;
{
    struct gp1pr   *dmd;
    struct pixrect *pr;
    int             statblk;

    pr = pw->pw_pixrect;
    dmd = gp1_d(pr);
    if (*attrptr != 0)
	_cgi_gp1_attr_close(pw, attrptr);
    *attrptr = (Gp1_attr *) malloc(sizeof(Gp1_attr));
    if (*attrptr == (Gp1_attr *) 0)
	return (-1);
    else
    {
	statblk = _cgi_gp1_stblk_alloc(dmd->cgpr_fd);
	if (statblk < 0)
	    return (-1);
	(*attrptr)->attrchg = TRUE;
	(*attrptr)->attrcnt = (*attrptr)->clpcnt = _cgi_state.gp1resetcnt - 1;
	(*attrptr)->org_x = (*attrptr)->org_y = 0;
	(*attrptr)->statblkindx = statblk;
	(*attrptr)->fbindx = dmd->cg2_index;
	(*attrptr)->gp1_addr = (short *) 0;
	(*attrptr)->shm_base = 0;
	(*attrptr)->xscale = 0.0;
	(*attrptr)->xoffset = 0.0;
	(*attrptr)->yscale = 0.0;
	(*attrptr)->yoffset = 0.0;
	(*attrptr)->op = 0;
	(*attrptr)->color = 0;
	(*attrptr)->pixplanes = 0;
	(*attrptr)->width = 0;
	(*attrptr)->tex_pattern = (short *) 0;
	(*attrptr)->tex_offset = 0;
	(*attrptr)->tex_options = 0;
	(*attrptr)->gp1_addr = (short *) dmd->gp_shmem;
	/* By default, assume it has at least width-1 solid cgi lines */
#ifdef	GP1_VERSION_QUERY
	(*attrptr)->ncmd = dmd->ncmd;
	(*attrptr)->cmdver = dmd->cmdver;
#else	GP1_VERSION_QUERY
	/*
	 * Create fake values for ncmd and cmdver to look like GP1_CGIVEC
	 * exists.  Remember that the GP's cmdver is guaranteed to be at least
	 * 64 elements. 
	 */
	(*attrptr)->ncmd = 64;
	(*attrptr)->cmdver = (u_char *) calloc(64, sizeof(u_char));
	if ((*attrptr)->cmdver == (u_char *) 0)
	    return (-1);
	(*attrptr)->cmdver[GP1_CGIVEC >> 8] = 1;
#endif	GP1_VERSION_QUERY
	return (0);
    }
}


_cgi_gp1_attr_close(pw, attrptr)
Pixwin         *pw;
Gp1_attr      **attrptr;
{
    struct gp1pr   *dmd;

    dmd = gp1_d(pw->pw_pixrect);
    /* ## what if this fails? */
    (void) _cgi_gp1_stblk_free(dmd->cgpr_fd, (*attrptr)->statblkindx);
    free((char *) *attrptr);
    *attrptr = 0;
}


_cgi_gp1_set_scale(xscl, yscl, ptr)
float          *xscl, *yscl;
Gp1_attr       *ptr;
{
    ptr->xscale = *xscl;
    ptr->yscale = *yscl;
    ptr->attrchg = TRUE;
}


_cgi_gp1_set_offset(xoff, yoff, ptr)
float          *xoff, *yoff;
Gp1_attr       *ptr;
{
    ptr->xoffset = *xoff;
    ptr->yoffset = *yoff;
    ptr->attrchg = TRUE;
}


_cgi_gp1_set_op(op, ptr)
int             op;
Gp1_attr       *ptr;
{
    if (ptr->op != op)
    {
	ptr->op = op;
	ptr->attrchg = TRUE;
    }
}


_cgi_gp1_set_color(color, ptr)
int             color;
Gp1_attr       *ptr;
{
    if (ptr->color != color)
    {
	ptr->color = color;
	ptr->attrchg = TRUE;
    }
}


_cgi_gp1_set_pixplanes(pixplanes, ptr)
int             pixplanes;
Gp1_attr       *ptr;
{
    if (ptr->pixplanes != pixplanes)
    {
	ptr->pixplanes = pixplanes;
	ptr->attrchg = TRUE;
    }
}


_cgi_gp1_set_linewidth(width, ptr)
int             width;
Gp1_attr       *ptr;
{
#ifndef NO_PIXWIN_POLYLINE
    if (ptr->width != width)
    {
	ptr->width = width;
	ptr->attrchg = TRUE;
    }
#endif NO_PIXWIN_POLYLINE
}


/*
 * WARNING: this routine assumes that CGI patterns are STATIC,
 * and uses the pointer value to determine equality.
 */
_cgi_gp1_set_linetexture(texP, gpptr)
struct pr_texture *texP;
Gp1_attr       *gpptr;
{
#ifndef NO_PIXWIN_POLYLINE
    /*
     * If you figure out how to do this with only one 'if' clause, AND prevent
     * an access through a null pointer, let me know. 
     */
    if (texP == (struct pr_texture *) 0)
    {
	if (gpptr->tex_pattern != (short *) 0)
	{
	    gpptr->tex_offset = 0;
	    gpptr->tex_options = 0;
	    gpptr->tex_pattern = 0;
	    gpptr->attrchg = TRUE;
	}
    }
    else
    {
	short           options_word, *p;

	p = (short *) &(texP->options);	/* this keeps lint from complaining */
	options_word = *p;
	if (gpptr->tex_offset != texP->offset
	    || gpptr->tex_options != options_word
	    || gpptr->tex_pattern != texP->pattern)
	{
	    gpptr->tex_offset = texP->offset;
	    gpptr->tex_options = options_word;
	    gpptr->tex_pattern = texP->pattern;
	    gpptr->attrchg = TRUE;
	}
    }
#endif NO_PIXWIN_POLYLINE
}


_cgi_gp1_snd_attr(gp1_addr, offset, ptr, fd)
short          *gp1_addr;
int             offset, fd;
register Gp1_attr *ptr;
{
    register short *bufptr, *patptr;
    register int    n;
    int             cnt;

    cnt = ptr->resetcnt;
    bufptr = &gp1_addr[offset];
    *bufptr++ = GP1_USEFRAME | ptr->statblkindx;
    *bufptr++ = GP1_SETFBINDX | ptr->fbindx;
    *bufptr++ = GP1_SETVWP_2D;
    *((float *) bufptr)++ = ptr->xscale;
    *((float *) bufptr)++ = ptr->xoffset;
    *((float *) bufptr)++ = ptr->yscale;
    *((float *) bufptr)++ = ptr->yoffset;
    *bufptr++ = GP1_SETROP;
    *bufptr++ = ptr->op;
    *bufptr++ = GP1_SETCOLOR | ptr->color;
    *bufptr++ = GP1_SETPIXPLANES | ptr->pixplanes;
#ifndef NO_PIXWIN_POLYLINE
    /* size of cmdver guaranteed to be >= 64 */
    if (ptr->cmdver[GP1_SET_LINE_WIDTH >> 8])
    {
	*bufptr++ = GP1_SET_LINE_WIDTH;
	*bufptr++ = ptr->width;
	*bufptr++ = 0;
    }
    /* size of cmdver guaranteed to be >= 64 */
    if (ptr->cmdver[GP1_SET_LINE_TEX >> 8])
    {
	*bufptr++ = GP1_SET_LINE_TEX;
	patptr = ptr->tex_pattern;
	if (patptr != 0)
	{
	    for (n = 16; --n >= 0 && *patptr != 0;)
		*bufptr++ = *patptr++;
	}
	*bufptr++ = 0;
	*bufptr++ = ptr->tex_offset;
	*bufptr++ = ptr->tex_options;
    }
#endif NO_PIXWIN_POLYLINE
    *bufptr++ = GP1_SETCLIPPLANES | 0x3F;	/* | ptr->clipplanes; */
    *bufptr++ = GP1_EOCL;
    if (gp1_post(gp1_addr, offset, fd))
    {				/* If post failed, loop until GP signal changes
				 * resetcnt */
	while (cnt == ptr->resetcnt)
	    ;
	return (-1);
    }
    n = bufptr - &gp1_addr[offset];
    ptr->attrcnt = cnt;
    ptr->attrchg = FALSE;
    ptr->offset += n;
    return (n);			/* NOTE: up to 39 words of shared mem used */
}
