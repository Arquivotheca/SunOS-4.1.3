#ifndef lint
static char	sccsid[] = "@(#)gp1_clip.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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

#include <sunwindow/window_hs.h>
#include "cgi_gp1_pwpr.h"
#include <pixrect/gp1cmds.h>

_cgi_gp1_pw_updclplst(pw, ptr)
Pixwin         *pw;
Gp1_attr       *ptr;
{
    int             nrect;
    unsigned int    bitvec;
    short           offset, *gp1_addr, *nptr;
    register short *bufptr;
    struct pixwin_prlist *prl;
    struct gp1pr   *dmd;
    int             x, y, retcode /* , cnt */ ;
    int             fd;

    fd = ((struct gp1pr *) (pw->pw_pixrect->pr_data))->ioctl_fd;
    ptr->clipid = pw->pw_clipdata->pwcd_clipid;
    retcode = 0;		/* wds 851009: Moved above restart */
Restart:
    ptr->clpcnt = ptr->resetcnt;
    dmd = gp1_d(pw->pw_pixrect);
    gp1_addr = (short *) dmd->gp_shmem;
    while ((offset = gp1_alloc(gp1_addr, 1, &bitvec, dmd->minordev, fd)) == 0)
	;
    bufptr = &gp1_addr[offset];
    *bufptr++ = GP1_USEFRAME | (ptr->statblkindx & 0x7);
    *bufptr++ = GP1_SETCLPLST;
    nptr = bufptr++;
    if (pw->pw_clipdata->pwcd_state == PWCD_NULL)
    {
	*nptr = 1;
	*bufptr++ = 0;
	*bufptr++ = 0;
	*bufptr++ = 0;
	*bufptr++ = 0;
    }
    else
    {				/* better be PWCD_SINGLERECT or PWCD_MULTIRECTS */
	nrect = 0;
	dmd = gp1_d(pw->pw_clipdata->pwcd_prmulti);
	x = dmd->cgpr_offset.x;
	y = dmd->cgpr_offset.y;
	if (x != ptr->org_x || y != ptr->org_y)
	{
	    /* Add to new org: portion of offset not due to org */
	    ptr->xoffset = x + (ptr->xoffset - ptr->org_x);
	    ptr->yoffset = y + (ptr->yoffset - ptr->org_y);
	    ptr->attrchg = TRUE;
	    ptr->org_x = x;
	    ptr->org_y = y;
	    retcode = 1;
	}
	for (prl = pw->pw_clipdata->pwcd_prl; prl; prl = prl->prl_next)
	{
	    dmd = gp1_d(prl->prl_pixrect);
	    *bufptr++ = dmd->cgpr_offset.x;
	    *bufptr++ = dmd->cgpr_offset.y;
	    *bufptr++ = prl->prl_pixrect->pr_size.x;
	    *bufptr++ = prl->prl_pixrect->pr_size.y;
	    if (++nrect == 60)
		break;
	}
	*nptr = nrect;
    }
    *bufptr++ = GP1_EOCL | GP1_FREEBLKS;
    *bufptr++ = (bitvec >> 16) & 0xffff;
    *bufptr++ = bitvec & 0xffff;
    if (gp1_post(gp1_addr, offset, fd))
    {				/* If post failed, loop until GP signal changes
				 * resetcnt */
	while (ptr->clpcnt == ptr->resetcnt)
	    ;
	goto Restart;
    }
    ptr->clpcnt = ptr->resetcnt;
    return (retcode);
}
