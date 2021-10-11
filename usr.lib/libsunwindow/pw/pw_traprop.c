#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)pw_traprop.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * pw_traprop.c traprop scan routine for pixwins
 *    built on top of pr_traprop
 *    see pr_traprop.c for algorithmic details
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/traprop.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/pw_util.h>

pw_traprop(pw, dx, dy, trap, op, spr, sx, sy)
    register struct pixwin *pw;
    register int dx, dy;
    struct pr_trap trap;
    int op;
    struct pixrect *spr;
    int sx, sy;
{
    register struct	pixwin_prlist *prl;
    struct	rect rdest;

    /* Translate dx, dy */
    dx = PW_X_OFFSET(pw, dx);
    dy = PW_Y_OFFSET(pw, dy);
    /* Do std setup */
    PW_SETUP(pw, rdest, DoDraw, 0, 0, pw->pw_pixrect->pr_width, 
	pw->pw_pixrect->pr_height);
    /*
     * See if user wants to bypass clipping.
     */
    if (op & PIX_DONTCLIP) {
        (void)pr_traprop(pw->pw_clipdata->pwcd_prmulti, dx,dy, trap, op, spr, sx,sy);
    } else
	    for (prl = pw->pw_clipdata->pwcd_prl;prl;prl = prl->prl_next) {
                (void)pr_traprop(prl->prl_pixrect, dx-prl->prl_x, dy-prl->prl_y,
			trap, op, spr, sx, sy);
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
        (void)pr_traprop(pw->pw_prretained, PW_RETAIN_X_OFFSET(pw, dx),
	    PW_RETAIN_Y_OFFSET(pw, dy), trap, op, spr, sx, sy);
    }

    return;
}
