#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)pw_polygon2.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * pw_polygon2.c polygon scan routine for pixwins
 *    built on top of pr_polygon_2
 *    see pr_polygon2.c for algorithmic details
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/pw_util.h>

pw_polygon_2(pw, dx, dy, nbds, npts, vlist, op, spr, sx, sy)
    register struct pixwin *pw;
    register int dx, dy;
    int nbds;
    int npts[];
    struct pr_pos *vlist;
    int op;
    struct pixrect *spr;
    int sx, sy;
{
    register struct	pixwin_prlist *prl;
    struct	rect rdest;

    /* Translate dx, dy */
    dx = PW_X_OFFSET(pw, dx);
    dy = PW_Y_OFFSET(pw, dy);
    /* Do standard setup */
    PW_SETUP(pw, rdest, DoDraw, dx, dy, pw->pw_pixrect->pr_width - dx, 
	pw->pw_pixrect->pr_height - dy);
    /*
     * See if user wants to bypass clipping.
     */
    if (op & PIX_DONTCLIP) {
	    (void)pr_polygon_2(pw->pw_clipdata->pwcd_prmulti, dx, dy, nbds, npts, 
                     vlist, op, spr, sx, sy);
    } else
            for (prl = pw->pw_clipdata->pwcd_prl;prl;prl = prl->prl_next) {
                (void)pr_polygon_2(prl->prl_pixrect,
			dx - prl->prl_x, dy - prl->prl_y,
			nbds,npts,vlist, op, spr, sx, sy);
            }

   /*
    * Unlock screen
    */
    (void)pw_unlock(pw);

    /*
     * Write to retained pixrect if have one.
     */
DoDraw:
    if (pw->pw_prretained) {
        (void)pr_polygon_2(pw->pw_prretained, PW_RETAIN_X_OFFSET(pw, dx), 
	    PW_RETAIN_Y_OFFSET(pw, dy), nbds, npts, vlist, op, spr, sx, sy);
    }

    return;
}
