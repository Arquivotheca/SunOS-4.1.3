#ifndef lint
#ifdef sccs
static char  sccsid[] = "@(#)cursor_confirm.c 1.1 92/07/30 Sun Micro";
#endif
#endif

/*
 *  Copyright (c) 1984 by Sun Microsystems Inc.
 *
 *  cursor_confirm:
 *	sieze the screen, display a special cursor, and await a mouse
 *	button-push.  If it was left, return TRUE; middle or right, FALSE.
 *	Client should post a prompt before calling, and expect a button-up
 *	(if its mask admits such) after return.
 */

#include <stdio.h>
#include <suntool/tool_hs.h>
#include <suntool/fullscreen.h>

static unsigned	cursor_data[8] = {
			0x01F80206, 0x7FF18008, 0xB148B228, 0xB148B228,
			0xB148B228, 0xB1488008, 0x80088008, 0x8008FFF8
		};
static mpr_static(confirm_pr, 16, 16, 1, cursor_data);

/*
 * should get FULLSCREEN from <sunwindow/cursor_impl.h> but "fullscreen" macro
 * conflicts with the use of "fullscreen" struct in <suntool/fullscreen.h>
 */

#define	FULLSCREEN	0x10

static struct cursor confirm_cursor={ 8, 8, PIX_SRC, &confirm_pr, FULLSCREEN };

cursor_confirm(fd)
int	fd;
{
	struct fullscreen	*fsh;
	struct inputmask	 im;
	struct inputevent	 ie;
	int			 result;

	fsh = fullscreen_init(fd);
	(void)input_imnull(&im);
	win_setinputcodebit(&im, MS_LEFT);
	win_setinputcodebit(&im, MS_MIDDLE);
	win_setinputcodebit(&im, MS_RIGHT);
	(void)win_setinputmask(fd, &im, (struct inputmask *)NULL, WIN_NULLLINK);
	(void)win_setcursor(fd, &confirm_cursor);
	for(;;) {
		if (input_readevent(fd, &ie) == -1)  {
			perror("Cursor_confirm input failed");
			abort();
		}
		switch (ie.ie_code)  {
		  case MS_LEFT:		result = TRUE; break;

		  case MS_MIDDLE:
		  case MS_RIGHT:	result = FALSE; break;

		  default:		blink(fd); continue;
		}
		break;
	}

	(void)fullscreen_destroy(fsh);
	return result;
}


static
blink(fd)
int	fd;
{
	register int	i = 10000;

	(void)pr_rop(&confirm_pr, 0, 0, 16, 16, PIX_NOT(PIX_DST), 
		(struct pixrect *)0, 0, 0);
	(void)win_setcursor(fd, &confirm_cursor);
	while (i--);
	(void)pr_rop(&confirm_pr, 0, 0, 16, 16, PIX_NOT(PIX_DST), 
		(struct pixrect *)0, 0, 0);
	(void)win_setcursor(fd, &confirm_cursor);
}
