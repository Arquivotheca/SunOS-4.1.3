#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)pw_text.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Pw_text.c: Implement the pw_char & pw_text functions
 *	of the pixwin.h interface.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/pw_util.h>

PIXFONT	*pf_sys;
int	pf_syscount;

extern	pf_ttext(), pf_text();
static	int (*pf_textop)() = pf_text;	/* toggle between pf_text& pf_ttext */

pw_char(pw, xw, yw, op, pixfont, c)
	struct	pixwin *pw;
	int	op, xw, yw;
	char	c;
	struct	pixfont *pixfont;
{
	char	s[2];

	s[0] = c;
	s[1] = 0;
	(void)pw_text(pw, xw, yw, op, pixfont, s);
}

pw_ttext(pw, xbasew, ybasew, op, pixfont, s)
	struct	pixwin *pw;
	int	op, xbasew, ybasew;
	char	*s;
	struct	pixfont *pixfont;
{
	pf_textop = pf_ttext;
	(void)pw_text(pw, xbasew, ybasew, op, pixfont, s);
	pf_textop = pf_text;
}

pw_text(pw, xbasew, ybasew, op, pixfont, s)
	register struct	pixwin *pw;
	int	op;
	register int	xbasew, ybasew;
	char	*s;
	struct	pixfont *pixfont;
{
	struct	rect rclipstruct, rdeststruct;
	register struct	rect *rclip = &rclipstruct, *rdest = &rdeststruct;
	register int	yhomew, xhomew;
	register struct	pixwin_prlist *prl;
	int	len = strlen(s);
	struct	pr_size strsize;
	extern	struct pr_size pf_textwidth();
	struct pr_prpos prpos;

	if (len == 0) return;

	/*
	 * NULL pixfont mean use pf_sys.
	 */
	if (pixfont == 0) {
		PIXFONT *pw_pfnull();

		pixfont = pw_pfnull(pixfont);
	}
	/* Translate destination */
	xbasew = PW_X_OFFSET(pw, xbasew);
	ybasew = PW_Y_OFFSET(pw, ybasew);
	/*
	 * Construct window relative rectangle that will be written to
	 */
	yhomew = ybasew+pixfont->pf_char[(unsigned char)s[0]].pc_home.y;
	xhomew = xbasew+pixfont->pf_char[(unsigned char)s[0]].pc_home.x;
	strsize = pf_textwidth(len, pixfont, s);
	PW_SETUP(pw, rdeststruct, DoDraw,
	    xhomew, yhomew, strsize.x, strsize.y);
	if (op & PIX_DONTCLIP) {
		prpos.pr = pw->pw_clipdata->pwcd_prmulti;
		prpos.pos.x = xbasew;
		prpos.pos.y = ybasew;
		pf_textop(prpos,op,pixfont,s);
		goto TryRetained;
	}
	for (prl = pw->pw_clipdata->pwcd_prl;prl;prl = prl->prl_next) {
		rect_construct(rclip, prl->prl_x, prl->prl_y,
		    prl->prl_pixrect->pr_width, prl->prl_pixrect->pr_height);
		if (rect_includesrect(rclip, rdest)) {
			prpos.pr = prl->prl_pixrect;
			prpos.pos.x = xbasew-prl->prl_x;
			prpos.pos.y = ybasew-prl->prl_y;
			pf_textop(prpos,
			    op|PIX_DONTCLIP, pixfont, s);
			break;
		} else  {
			prpos.pr = prl->prl_pixrect;
			prpos.pos.x = xbasew-prl->prl_x;
			prpos.pos.y = ybasew-prl->prl_y;
			pf_textop(prpos,
			    op&(~PIX_DONTCLIP), pixfont, s);
		}
	}
TryRetained:
	(void)pw_unlock(pw);
DoDraw:
	if (pw->pw_prretained) {
		prpos.pr = pw->pw_prretained;
		prpos.pos.x = PW_RETAIN_X_OFFSET(pw,xbasew);
		prpos.pos.y = PW_RETAIN_Y_OFFSET(pw,ybasew);
		pf_textop(prpos,op,pixfont,s);
	}
	return;
}

/*
 * Shared system pixfont.
 */
PIXFONT *
pw_pfsysopen()
{
	if (pf_sys == 0) {
		pf_sys = pf_open((char *)0);
		if (pf_sys == 0)
			return(0);
		pf_syscount = 1;
	} else
		pf_syscount++;
	return(pf_sys);
}

pw_pfsysclose()
{
	pf_syscount--;
	if (pf_syscount <= 0 && pf_sys) {
		(void)pf_close(pf_sys);
		pf_syscount = 0;
		pf_sys = (PIXFONT *) 0;
	}
}

/*
 * Convert font handle into default font handle if original is
 * null.  Will increment ref count on default font handle only
 * if not already open.  This is meant as a handy utility for
 * handling null font handles passed at you.
 *
 * One should really call pw_pfsysopen directly if you want
 * to get and hold onto the default font handle so that the
 * reference counts are maintained correctly.
 */
PIXFONT *
pw_pfnull(font)
	PIXFONT *font;
{
	if (font == (PIXFONT *)0) {
		if (pf_sys == 0 && pw_pfsysopen() == (PIXFONT *)0)
			return((PIXFONT *)0);
		font = pf_sys;
	}
	return(font);
}

