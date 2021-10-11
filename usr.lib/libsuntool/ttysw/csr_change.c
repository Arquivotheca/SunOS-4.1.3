#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)csr_change.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1985, 1986 and 1987 by Sun Microsystems, Inc.
 */

/*
 * Character screen operations (except size change and init).
 */
#include <sys/types.h>
#include <sys/time.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <suntool/tool_struct.h>
#include <suntool/ttysw.h>
#include <suntool/charimage.h>
#include <suntool/charscreen.h>
#undef CTRL
#include <suntool/ttyansi.h>
#include <stdio.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>

extern	wfd;
extern	struct	pixwin *csr_pixwin;
extern	int	tty_cursor;	/* NOCURSOR, UNDERCURSOR, BLOCKCURSOR */
extern void	pos();
extern void	win_sync();
static int	ttysw_send_sync = FALSE;
extern int	sv_journal;
extern char	*shell_prompt;
extern int	prompt_len;

static	int caretx, carety;
static	short charcursx, charcursy;

static	int boldstyle, inverse_mode, underline_mode;

extern struct timeval ttysw_bell_tv; /* initialized to 1/10 second */

ttysw_setboldstyle(new_boldstyle)
	int	new_boldstyle;
{
	if (new_boldstyle > TTYSW_BOLD_MAX
	    || new_boldstyle < TTYSW_BOLD_NONE)
		boldstyle = TTYSW_BOLD_NONE;
	else
		boldstyle = new_boldstyle;
	return boldstyle;
}

ttysw_set_inverse_mode(new_inverse_mode)
	int	new_inverse_mode;
{
	inverse_mode = new_inverse_mode;
}

ttysw_set_underline_mode(new_underline_mode)
	int	new_underline_mode;
{
	underline_mode = new_underline_mode;
}

ttysw_getboldstyle()
{
	return boldstyle;
}

ttysw_get_inverse_mode()
{
	return inverse_mode;
}

ttysw_get_underline_mode()
{
	return underline_mode;
}


ttysw_setleftmargin(left_margin)
{
	chrleftmargin = left_margin > 0 ? left_margin : 0;
}

ttysw_getleftmargin()
{
	return chrleftmargin;
}

fixup_display_mode(mode)
	char 	*mode;
{

	if ((*mode & MODE_INVERT) && (inverse_mode != TTYSW_ENABLE)) {
		*mode &= ~MODE_INVERT;
	        if (inverse_mode == TTYSW_SAME_AS_BOLD)
	            *mode |= MODE_BOLD;
	}

	if ((*mode & MODE_UNDERSCORE) && (underline_mode != TTYSW_ENABLE)) {
	        *mode &= ~MODE_UNDERSCORE;
	        if (underline_mode == TTYSW_SAME_AS_BOLD)
	            *mode |= MODE_BOLD;
	}

	if ((*mode & MODE_BOLD) && (boldstyle & TTYSW_BOLD_INVERT)) {
		*mode &= ~MODE_BOLD;
		*mode |= MODE_INVERT;
	}
}

/* Looks for the shell prompt at the end of the buffer s, used only 
   in synchronized journaling mode. */
int
svj_strcmp(s)
        register unsigned char *s;
{
	int	buf_len;
	char	*str_ptr;

	buf_len = strlen(s);
	if (buf_len >= prompt_len) {
		return (strcmp(s+buf_len-prompt_len, shell_prompt));
	}
	else
		return (-1);
}

/* Note: whole string will be diplayed with mode. */
pstring(s, mode, col, row, op)
	register unsigned char *s;
	char mode;
	register int col, row;
	int op; /* PIX_SRC | PIX_DST (faster), or PIX_SRC (safer) */
{
	register unsigned char *cp;

	if ((ttysw_send_sync) && (svj_strcmp(s) == 0)) {
			win_sync(WIN_SYNC_TTY,-1);
			ttysw_send_sync = FALSE;
		}
	if (delaypainting) {
		if (row == bottom)
			/*
			 * Reached bottom of screen so end delaypainting.
			 */
			(void)pdisplayscreen(1);
		return;
	}
	if (s==0)
		return;
	fixup_display_mode(&mode);
	if (mode & MODE_BOLD) {
	        (void) pclearline(col, col+strlen(s), row);  /* Clean up first */
		(void)pw_text(csr_pixwin,
		   col_to_x(col)-pixfont->pf_char[*s].pc_home.x,
		   row_to_y(row)-pixfont->pf_char[*s].pc_home.y,
		   (mode & MODE_INVERT)? PIX_NOT(PIX_SRC) : op, pixfont, s);

		if (boldstyle & TTYSW_BOLD_OFFSET_X)
			(void)pw_text(csr_pixwin,
			   col_to_x(col)-pixfont->pf_char[*s].pc_home.x+1,
			   row_to_y(row)-pixfont->pf_char[*s].pc_home.y,
			   (mode & MODE_INVERT)? PIX_NOT(PIX_SRC) & PIX_DST :
			   PIX_SRC|PIX_DST, pixfont, s);
		if (boldstyle & TTYSW_BOLD_OFFSET_Y)
			(void)pw_text(csr_pixwin,
			   col_to_x(col)-pixfont->pf_char[*s].pc_home.x,
			   row_to_y(row)-pixfont->pf_char[*s].pc_home.y+1,
			   (mode & MODE_INVERT)? PIX_NOT(PIX_SRC) & PIX_DST :
			   PIX_SRC|PIX_DST, pixfont, s);
		if (boldstyle & TTYSW_BOLD_OFFSET_XY)
			(void)pw_text(csr_pixwin,
			   col_to_x(col)-pixfont->pf_char[*s].pc_home.x+1,
			   row_to_y(row)-pixfont->pf_char[*s].pc_home.y+1,
			   (mode & MODE_INVERT)? PIX_NOT(PIX_SRC) & PIX_DST :
			   PIX_SRC|PIX_DST, pixfont, s);
	} else {
		(void)pw_text(csr_pixwin,
		   col_to_x(col)-pixfont->pf_char[*s].pc_home.x,
		   row_to_y(row)-pixfont->pf_char[*s].pc_home.y,
		   (mode & MODE_INVERT)? PIX_NOT(PIX_SRC): op, pixfont, s);
	}
	if (mode & MODE_UNDERSCORE) {
		pw_writebackground(csr_pixwin,
		   col_to_x(col), row_to_y(row) + chrheight - 1,
		   strlen(s) * chrwidth, 1,
		   (mode & MODE_INVERT) ? PIX_SRC : PIX_NOT(PIX_SRC));
	}
}

pclearline(fromcol, tocol, row)
	int fromcol, tocol, row;
{
	if (delaypainting)
		return;
	(void)pw_writebackground(csr_pixwin, col_to_x(fromcol), row_to_y(row),
	    col_to_x(tocol)-col_to_x(fromcol), chrheight, PIX_CLR);
}

pcopyline(tocol, fromcol, count, row)
	int fromcol, tocol, count, row;
{
        int	pix_width = (count * chrwidth);
	if (delaypainting)
		return;
	(void)pw_copy(csr_pixwin, col_to_x(tocol), row_to_y(row),
	    pix_width, chrheight, PIX_SRC,
	    csr_pixwin, col_to_x(fromcol), row_to_y(row));
	(void)prepair(wfd, &csr_pixwin->pw_fixup);
	(void)rl_free(&csr_pixwin->pw_fixup);
}

pclearscreen(fromrow, torow)
	int fromrow, torow;
{
	if (delaypainting)
		return;
	(void)pw_writebackground(csr_pixwin, col_to_x(left), row_to_y(fromrow),
	    winwidthp, row_to_y(torow-fromrow), PIX_CLR);
}

pcopyscreen(fromrow, torow, count)
	int fromrow, torow, count;
{
	if (delaypainting)
		return;
	(void)pw_copy(csr_pixwin, col_to_x(left), row_to_y(torow), winwidthp,
	    row_to_y(count), PIX_SRC,
	    csr_pixwin, col_to_x(left), row_to_y(fromrow));
	(void)prepair(wfd, &csr_pixwin->pw_fixup);
	(void)rl_free(&csr_pixwin->pw_fixup);
}

pdisplayscreen(dontrestorecursor)
	int	dontrestorecursor;
{
	delaypainting = 0;
	/*
	 * refresh the entire image.
	 */
	if (csr_pixwin->pw_prretained) {
		Rectlist rl;
		Rect r;

		rect_construct(&r, 0, 0, winwidthp, winheightp);
		(void)rl_initwithrect(&r, &rl);
		(void)prepair(wfd, &rl);
		(void)rl_free(&rl);
	} else
		(void)prepair(wfd, &csr_pixwin->pw_clipdata->pwcd_clipping);
	if (!dontrestorecursor)
		/*
		 * The following has effect of restoring cursor.
		 */
		(void)removeCursor();
}

/* ARGSUSED */
prepair(windowfd, rlp)
	int	windowfd;
	struct	rectlist *rlp;
{
	struct	rectnode *rnp;
	struct	rect rect;

	rl_rectoffset(rlp, &rlp->rl_bound, &rect);
	(void)pw_lock(csr_pixwin, &rect);
	for (rnp = rlp->rl_head; rnp; rnp = rnp->rn_next) {
		register int row, colstart, blanks, colfirst;
		register unsigned char *strstart, *strfirst; 
		register char modefirst;
		register char *modestart;
		int	toprow, botrow, leftcol;
		char	csave;

		colfirst = 0;	/* make LINT shut up */
		rl_rectoffset(rlp, &rnp->rn_rect, &rect);
		(void)pw_writebackground(csr_pixwin, rect.r_left, rect.r_top,
		    rect.r_width, rect.r_height, PIX_CLR);
		toprow = y_to_row(rect.r_top);
		botrow = min(bottom, y_to_row(rect_bottom(&rect))+1);
		leftcol = x_to_col(rect.r_left);
		for (row = toprow; row < botrow; row++) {
		    colstart = leftcol;
		    if (length(image[row]) > leftcol) {
			strfirst = 0;
			modefirst = MODE_CLEAR;
			blanks = 1;
			for (strstart = image[row]+leftcol,
			    modestart = screenmode[row]+leftcol; *strstart;
			    strstart++, modestart++, colstart++) {
				/*
				 * Find beginning of bold string
				 */
				if (*modestart != modefirst) {
					goto Flush;
				/*
				 * Find first non-blank char
				 */
				} else if (blanks && (*strstart != ' '))
					goto Flush;
				else
					continue;
Flush:
				if (strfirst != 0) {
					csave = *strstart;
					*strstart = '\0';
					(void)pstring(strfirst, modefirst,
					    colfirst, row, PIX_SRC | PIX_DST);
					*strstart = csave;
				}
				colfirst = colstart;
				strfirst = strstart;
				modefirst = *modestart;
				blanks = 0;
			}
			if (strfirst != 0)
				(void)pstring(strfirst, modefirst, colfirst,
				    row, PIX_SRC | PIX_DST);
		    }
		}
	}
	(void)pw_unlock(csr_pixwin);
}

drawCursor(yChar, xChar)
{

	charcursx = xChar;
	charcursy = yChar;
	caretx = col_to_x(xChar);
	carety = row_to_y(yChar);
	if (delaypainting || tty_cursor == NOCURSOR)
		return;
	(void)pw_writebackground(csr_pixwin,
	    caretx, carety, chrwidth, chrheight, PIX_NOT(PIX_DST));
	if (tty_cursor & LIGHTCURSOR) {
		(void)pw_writebackground(csr_pixwin,
		    caretx-1, carety-1, chrwidth+2, chrheight+2, PIX_NOT(PIX_DST));
		(void) pos(xChar, yChar);
	}
	if ((sv_journal) && (tty_cursor & BLOCKCURSOR)) 
			ttysw_send_sync = TRUE;
}

removeCursor()
{
	if (delaypainting || tty_cursor == NOCURSOR)
		return;
	(void)pw_writebackground(csr_pixwin,
	    caretx, carety, chrwidth, chrheight, PIX_NOT(PIX_DST));
	if (tty_cursor & LIGHTCURSOR) {
		(void)pw_writebackground(csr_pixwin,
		    caretx-1, carety-1, chrwidth+2, chrheight+2, PIX_NOT(PIX_DST));
	}
}

saveCursor()
{
	(void)removeCursor();
}

restoreCursor()
{
	(void)removeCursor();
}

screencomp()
{
}

blinkscreen()
{
	struct timeval	now;
static	struct timeval	lastblink;

	(void)gettimeofday(&now, (struct timezone *) 0);
	if (now.tv_sec - lastblink.tv_sec > 1) {
	    (void)win_bell(csr_pixwin->pw_windowfd, 
	                   ttysw_bell_tv, csr_pixwin);
	    lastblink = now;
	}
}

pselectionhilite(r)
	struct	rect *r;
{
	struct rect rectlock;

	rectlock = *r;
	rect_marginadjust(&rectlock, 1);
	(void)pw_lock(csr_pixwin, &rectlock);
	(void)pw_writebackground(csr_pixwin, r->r_left, r->r_top,
	    r->r_width, r->r_height, PIX_NOT(PIX_DST));
	(void)pw_unlock(csr_pixwin);
}
