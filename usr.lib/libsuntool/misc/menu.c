#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)menu.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Implement the menu interface described in menu.h.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/cms_mono.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_screen.h>	/* to satisfy win_ioctl.h */
#include <sunwindow/win_ioctl.h>	/* for journaling */
#include "sunwindow/sv_malloc.h"
#include <suntool/menu.h>
#include <suntool/fullscreen.h>

extern	errno;

#define	MD(m)	((struct menudata *)(LINT_CAST((m)->m_data)))
#define	stripe_height	(stripe_v_bound-MENU_BORDER)

struct	menudata {
	short	md_width;	/* width of menu */
};

static	int _menu_windowfd;
static	struct inputevent *_menu_event;
static	struct pixfont *_menu_font;
static	struct fullscreen *_menu_fs;

static struct pr_size image_size();

int type;	/* MENU_IMAGESTRING or MENU_GRAPHIC */
/*
 * Menu global data
 */
static	short stripe_v_bound ;
static	int menu_count;
static	int items_top, cur_item, promotemenuno;
static	struct menu *menu_list;
static	struct rect menu_rect;
static	short _menu_butcode;

static	Pw_pixel_cache *pixel_cache;

/*
 * Cursor data
 */
extern	struct	cursor menu_cursor;

/*
 * journaling
 */
extern	int	sv_journal;
extern	void	win_sync();

struct	menuitem *
menu_display(menuptr, inputevent, iowindowfd)
	struct	menu **menuptr;
	struct	inputevent *inputevent;
	int	iowindowfd;
{
	int	initx = inputevent->ie_locx, inity = inputevent->ie_locy;
	struct	inputmask im;

	/*
	 * Grab all input and disable anybody but windowfd from writing
	 * to screen while we are violating window overlapping.
	 */
	_menu_windowfd = iowindowfd;
	_menu_fs = fullscreen_init(iowindowfd);
	/*
	 * Setup global data
	 */
	(void)_menu_init(*menuptr, inputevent);
	/*
	 * Set menu cursor
	 */
	(void)win_setcursor(_menu_windowfd, &menu_cursor);
	/*
	 * Make sure input mask allows menu package to track cursor motion
	 */
	im = _menu_fs->fs_cachedim;
	im.im_flags |= IM_NEGEVENT;
	win_setinputcodebit(&im, LOC_MOVEWHILEBUTDOWN);
	win_setinputcodebit(&im, MS_LEFT);
	win_setinputcodebit(&im, _menu_butcode);
	(void)win_setinputmask(_menu_windowfd, &im, (struct inputmask *)0,
	   WIN_NULLLINK);
Repaint:
	if (_menu_putup(&initx, &inity) == FALSE)
		goto Done;
	/*
	 * if journaling, send sync point.
	 */
	if (sv_journal)
		win_sync(WIN_SYNC_STACK, iowindowfd);
	(void)track_mouse_over_menu();
	(void)menu_click();
	(void)remove_menus();
	if (promotemenuno) {
		(void)promote_menu(promotemenuno);
		promotemenuno = 0;
		if (!(event_action(_menu_event) == _menu_butcode &&
		    win_inputnegevent(_menu_event)))
			goto Repaint;
	}
Done:
	*menuptr = menu_list;
	(void)fullscreen_destroy(_menu_fs);
	(void)pw_pfsysclose();
	if (cur_item==-1)
		return(0);
	else
		return(menu_list->m_items+cur_item);
}

_menu_putup(initx, inity)
	int	*initx, *inity;
{
	struct	rect stripe_rect;
	register int i;
	register struct menuitem *mip;
	register struct menu *wmp;

	/*
	 * lay out menu space, adjusted for screen boundaries
	 */
	if (_menu_computerect(&menu_rect, initx, inity) == FALSE) {
		return(FALSE);
	}

	if ((pixel_cache = pw_save_pixels(fsglobal->fs_pixwin, &menu_rect)) ==
	    PW_PIXEL_CACHE_NULL)
		return(FALSE);

	/*
	 * Lock at outer level so every write operation doesn't have too.
	 */
	(void)pw_lock(_menu_fs->fs_pixwin, &menu_rect);
	/*
	 * paint the items for the topmost menu
	 */
	stripe_rect.r_left = menu_rect.r_left;
	stripe_rect.r_top = items_top;
	stripe_rect.r_width = MD(menu_list)->md_width;
	stripe_rect.r_height = stripe_v_bound;

	for (mip = menu_list->m_items,i=0;i < menu_list->m_itemcount;i++) {
		image_stripe(mip[i].mi_imagetype, mip[i].mi_imagedata,
			     &stripe_rect, FALSE);
		stripe_rect.r_top += stripe_height;
	}

	/*
	 * add header for topmost menu
	 */
	stripe_rect.r_height = stripe_height;
	stripe_rect.r_top = items_top - stripe_height;
	wmp = menu_list;
	image_stripe(wmp->m_imagetype, wmp->m_imagedata, &stripe_rect, TRUE);

	/*
	 * add the headers for each remaining menu on menu_list
	 */
	while (wmp = wmp->m_next) {
		stripe_rect.r_top -= stripe_height;
		stripe_rect.r_left += 4*MENU_BORDER;
		image_stripe(wmp->m_imagetype, wmp->m_imagedata,
			     &stripe_rect, TRUE);
		(void)header_separator(&stripe_rect);
		(void)fake_RL_edge(stripe_rect.r_left + stripe_rect.r_width,
			     stripe_rect.r_top + stripe_height);
	}

	(void)pw_unlock(_menu_fs->fs_pixwin);
	return(TRUE);
}

_menu_computerect(new, initx, inity)
	register struct rect  *new;
	register int *initx, *inity;
{
	register int	s_w = _menu_fs->fs_screenrect.r_width,
			s_h = _menu_fs->fs_screenrect.r_height;
	int	XINITOFFSET = 2;

	new->r_width = MD(menu_list)->md_width +
		4*MENU_BORDER * (menu_count - 1);
	new->r_height = ( (menu_list->m_itemcount + menu_count)
			* stripe_height) + MENU_BORDER;
	if (new->r_width > s_w  || new->r_height > s_h) {
		(void)fprintf(stderr, "win_menu: menu doesn't fit!\n");
		return(FALSE);
	}
	items_top = stripe_height * menu_count;

	new->r_left = *initx + XINITOFFSET;
	new->r_top = *inity - items_top;
	/*
	 * Constrain to be on screen. Note: need utility rect_constrain.
	 */
	(void)wmgr_constrainrect(new, &_menu_fs->fs_screenrect, 0, 0);
	/*
	 * Change inity so when click thru menu stack the top of the top
	 * menu doesn't appear to migrate unless needed.
	 */
	*inity = new->r_top + items_top;
	items_top += new->r_top;
	return(TRUE);
}

fake_RL_edge(x, y)
	register x, y;
{
	register w = 4*MENU_BORDER;
	register h = menu_list->m_itemcount * stripe_height;
	struct	rect r;


	x -= w;
	rect_construct(&r, x, y, w, h);
	(void)pw_preparesurface(_menu_fs->fs_pixwin, &r);
	(void)pw_writebackground(_menu_fs->fs_pixwin, x, y, w, h, PIX_SET);
	w -= MENU_BORDER; h -= MENU_BORDER;
	(void)pw_writebackground(_menu_fs->fs_pixwin, x, y, w, h, PIX_CLR);
}

remove_menus()		    /*	restore bitmap under  */
{
	pw_restore_pixels(_menu_fs->fs_pixwin, pixel_cache);
	pixel_cache = PW_PIXEL_CACHE_NULL;
}

_menu_init(menustart, initevent)
	struct	menu *menustart;
	struct	inputevent *initevent;
{
	int	i;
	struct	menu *menu;
	struct	menuitem *mi;
	struct	pr_size size;
	extern	struct pixfont *pw_pfsysopen();

	_menu_butcode = event_action(initevent);
	/*
	 * Open font
	 */
	_menu_font = pw_pfsysopen();
	/*
	 * Setup global menu invocation data
	 */
	menu_count = 0;
	/*
	stripe_v_bound = _menu_font->pf_defaultsize.y+8;
	*/
	stripe_v_bound = 0;
	items_top = 0;
	cur_item = -1;
	promotemenuno = 0;
	menu_list = menustart;
	menu_rect = rect_null;
	_menu_event = initevent;
	/*
	 * Do per menu initialization
	 */
	for (menu=(menu_list);menu;menu=menu->m_next) {
		/*
		 * Allocate implementation private/individual menu specific data
		 */
		if (menu->m_data==0) {
			menu->m_data =  (caddr_t) (LINT_CAST(
				sv_calloc(1, sizeof (struct menudata))));
		}
		/*
		 * Find width & height of menu
		 */
		menu_count++;
		size = image_size(menu->m_imagetype, menu->m_imagedata);
		stripe_v_bound = max(stripe_v_bound, size.y);
		MD(menu)->md_width = size.x;
		for (i=0;i<menu->m_itemcount;i++) {
			mi = menu->m_items+i;
			size = image_size(mi->mi_imagetype, mi->mi_imagedata);
			stripe_v_bound = max(stripe_v_bound, size.y);
			MD(menu)->md_width = max(size.x, MD(menu)->md_width);
		}
		MD(menu)->md_width += (2*MENU_BORDER)+2;
	}
	stripe_v_bound += 8;
}


get_mouse_raw()
{
	for (;;) {
		if (input_readevent(_menu_windowfd, _menu_event)==-1) {
			perror("menu: error reading input");
			return;
		}
		switch(event_action(_menu_event)) {
		case MS_LEFT: break;
		case LOC_MOVEWHILEBUTDOWN:
			if (win_inputnegevent(_menu_event))
				continue;
			else
				break;
		default:
			if (event_action(_menu_event) == _menu_butcode)
				break;
			else
				continue;
		}
		break;
	}
	return;
}

track_mouse_over_menu()	/*	compute new item:
			 *		outside item list ?   -1
			 *		else y-offset in item list
			 *			mod stripe r_height
			 */
{
	register int	new_item, loop = 1;
	int	firsttime = 1;

	do {
		if (firsttime)
			goto MaybeMoved;
		/*
		 * Get and choose only inputevent interested in
		 */
		(void)get_mouse_raw();
		switch(event_action(_menu_event)) {
		case MS_LEFT:
			if (win_inputnegevent(_menu_event)) {
				loop = 0;
				break;
			}
			continue;
		case LOC_MOVEWHILEBUTDOWN:
			if (win_inputposevent(_menu_event))
				break;
			continue;
		default:
			if (event_action(_menu_event) == _menu_butcode &&
			    win_inputnegevent(_menu_event)) {
				loop = 0;
				break;
			} else
				continue;
		}
MaybeMoved:
		firsttime = 0;
		/*
		 * See if over menu item
		 */
		if (menu_rect.r_left <= _menu_event->ie_locx &&
		    _menu_event->ie_locx <
		      menu_rect.r_left+MD(menu_list)->md_width  &&
		    items_top <= _menu_event->ie_locy &&
		    _menu_event->ie_locy <
		      menu_rect.r_top + menu_rect.r_height)  {
		      new_item = (_menu_event->ie_locy - items_top)/
			stripe_height;
		      if (new_item == menu_list->m_itemcount) {
			   new_item -= 1; /*  in bottom border  */
		      }
		} else {
			new_item = -1;
		}
		/*
		 * invert appropriate stripes if over new item
		 */
		if (new_item != cur_item) {
			if (cur_item != -1) (void)invert_stripe(cur_item);
			cur_item = new_item;
			if (new_item != -1) (void)invert_stripe(new_item);
		}
	} while (loop);
	return;
}

menu_click()
{
	register int	menu_no;

	/*
	 * cur_item has just been set to the item which contains
	 * the mouse (or -1 if none).
	 */
	if (cur_item != -1)
		return;
	/*
	 * in header region, button means promote a different menu to r_top
	 */
	if (_menu_event->ie_locy < menu_rect.r_top ||
	    items_top <= _menu_event->ie_locy ) {
		return;
	}
	menu_no = menu_count - ((_menu_event->ie_locy - menu_rect.r_top)
				 / stripe_height) - 1;
	if (menu_rect.r_left + (menu_no * 4*MENU_BORDER)
	    <= _menu_event->ie_locx
	 && _menu_event->ie_locx
	    < menu_rect.r_left + menu_rect.r_width
	      - ((menu_count - menu_no - 1) * 4*MENU_BORDER))
		promotemenuno = menu_no;
	return;
}

text_stripe(str, r, invert)
	register char *str;
	struct rect	*r;
{
#define	MENU_STRMAX	100
	char	buf[MENU_STRMAX+1];
	int	max_len = min((r->r_width -
		    (2 * MENU_BORDER) - 2)/_menu_font->pf_defaultsize.x,
		    MENU_STRMAX);

	/*
	 * Clip string for rear menu titles (sometimes)
	 */
	if (max_len < strlen(str)) {
		(void)strncpy(buf, str, max_len);
		buf[max_len] = '\0';
		str = buf;
	}
	(void)pw_preparesurface(_menu_fs->fs_pixwin, r);
	(void)pw_writebackground(_menu_fs->fs_pixwin, r->r_left, r->r_top,
	    r->r_width, r->r_height, PIX_SET);
	if(!invert)
		(void)pw_writebackground(_menu_fs->fs_pixwin,
		    r->r_left+MENU_BORDER, r->r_top+MENU_BORDER,
		    r->r_width-2*MENU_BORDER, r->r_height-2*MENU_BORDER,
		    PIX_CLR);
	if (str == '\0')
		return;
	(void)pw_text(_menu_fs->fs_pixwin,
	    r->r_left+MENU_BORDER+1-_menu_font->pf_char[str[0]].pc_home.x,
	    r->r_top+MENU_BORDER+1-_menu_font->pf_char[str[0]].pc_home.y,
	    (invert ? PIX_NOT(PIX_SRC) : PIX_SRC), _menu_font, str);
}

invert_stripe(n)
{
	struct rect r;

	if (n > menu_list->m_itemcount || n < 0) {
		(void)fprintf(stderr, "win_menu: invert_stripe n=%d\n", n);
		return;
	}
	r.r_left = menu_rect.r_left + MENU_BORDER;
	r.r_top = items_top + n*stripe_height + MENU_BORDER;
	r.r_width = MD(menu_list)->md_width - 2 * MENU_BORDER;
	r.r_height = stripe_height - MENU_BORDER;

	(void)pw_writebackground(_menu_fs->fs_pixwin,
	    r.r_left, r.r_top, r.r_width, r.r_height, PIX_NOT(PIX_DST));
}

/*
 * Menu promotion routine
 */
promote_menu(n)
{
	register struct menu  *new, *prev;

	if (n==0) return;
	if (n >= menu_count) {
	    (void)fprintf(stderr, "Menu Manager error: selected non-active menu.\n");
	    return;
	}
	prev = menu_list;
	while (--n) if (prev->m_next) prev = prev->m_next;
	if (prev->m_next == 0) {
	    (void)fprintf(stderr, "Menu Manager error: menu list too short.\n");
	    return;
	}
	new = prev->m_next; prev->m_next = new->m_next;
	new->m_next = menu_list; menu_list = new;

}


struct pixrect *
save_bits(pixwin, r)		/*	save underlying bits		*/
	struct pixwin *pixwin;
	struct rect  *r;
{
	struct	pixrect *mpr = mem_create(r->r_width, r->r_height, 
	    pixwin->pw_pixrect->pr_depth);

	if (mpr == 0) {
		(void)fprintf(stderr, "Couldn't allocate memory in save_bits\n");
		return((struct pixrect *) 0);
	}
	/*
	 * Subvert the pixwin interface, because pw_read requires
	 * a PIX_DONTCLIP|PIX_SRC to not clip to the src (the window)
	 * but the PIX_DONTCLIP flags makes the call unsafe if try to read
	 * from off the screen.
	 */
	(void)pw_lock(pixwin, r);
	(void)pr_rop(mpr, 0, 0, r->r_width, r->r_height, PIX_SRC, pixwin->pw_pixrect,
	    r->r_left-fsglobal->fs_screenrect.r_left,
	    r->r_top-fsglobal->fs_screenrect.r_top);
	(void)pw_unlock(pixwin);
	/*
	 * Ignore pixwin->pw_rlfixup
	 */
	return(mpr);
}

restore_bits(pixwin, r, mpr)	/*	restore saved underlying bits	*/
	struct pixwin *pixwin;
	struct rect *r;
	struct pixrect *mpr;
{
	(void)pw_write(pixwin, r->r_left, r->r_top, r->r_width, r->r_height,
	    PIX_SRC, mpr, 0, 0);
	mem_destroy(mpr);
}

header_separator(r)
	register struct rect *r;
{
	(void)pw_writebackground(_menu_fs->fs_pixwin,
	    r->r_left+MENU_BORDER, r->r_top+r->r_height-MENU_BORDER,
	    r->r_width-2*MENU_BORDER, MENU_BORDER, PIX_CLR);
}


static struct pr_size
image_size(menu_type, data)
int menu_type;	/* MENU_IMAGESTRING or MENU_GRAPHIC */
caddr_t data;	/* char * or pixrect * */
/* image_size returns the size of data given its type.  Global _menu_font
   is used if type is MENU_IMAGESTRING.
*/
{
   extern struct pr_size pf_textwidth();

   switch (menu_type) {
      case MENU_IMAGESTRING:
	 return (pf_textwidth(strlen((char *)data), _menu_font, (char *)data));

      case MENU_GRAPHIC:
	 return (((struct pixrect *)LINT_CAST(data))->pr_size);
   }
   /*NOTREACHED*/
} /* image_size */


static
image_stripe(menu_type, data, r, invert)
int menu_type;
register caddr_t data;
struct rect	*r;
int invert;
/* image_stripe stripes data within the rect r.
*/
{
   switch (menu_type) {
      case MENU_IMAGESTRING:
	 (void)text_stripe((char *)data, r, invert);
	 break;

      case MENU_GRAPHIC:
	 (void)pw_preparesurface(_menu_fs->fs_pixwin, r);
	 (void)pw_writebackground(_menu_fs->fs_pixwin, r->r_left, r->r_top,
	                    r->r_width, r->r_height, PIX_SET);
	 if(!invert)
	    (void)pw_writebackground(_menu_fs->fs_pixwin,
		    r->r_left+MENU_BORDER, r->r_top+MENU_BORDER,
		    r->r_width-2*MENU_BORDER, r->r_height-2*MENU_BORDER,
		    PIX_CLR);
	 /* center the pixrect vertically in the enclosing rect */
	 (void)pw_write(_menu_fs->fs_pixwin,
		  r->r_left+MENU_BORDER + 1,
		  r->r_top + MENU_BORDER + 1 + 
		  (r->r_height - ((struct pixrect *)(LINT_CAST(data)))->pr_height -
		  2 * MENU_BORDER) / 2,
		  min(r->r_width, ((struct pixrect *)(LINT_CAST(data)))->pr_width),
		  r->r_height,
		  (invert ? PIX_NOT(PIX_SRC) : PIX_SRC), 
		  (struct pixrect *)(LINT_CAST(data)), 0, 0);
	 break;
   }
} /* image_stripe */

