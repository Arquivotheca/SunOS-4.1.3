#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)walkmenu_render.c 1.1 92/07/30 Copyright 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*-
	WALKING MENU PACKAGE

	walkmenu_render.c, Sun Jun 30 15:38:39 1985

		Craig Taylor,
		Sun Microsystems
 */

/* ------------------------------------------------------------------------- */

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <fcntl.h>
#include <varargs.h>

#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>

#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_ioctl.h>

#include <suntool/fullscreen.h>
#include <suntool/window.h>

#include <suntool/walkmenu_impl.h>

#define	PIXRECT_NULL	((struct pixrect *)0)
#define SCREEN_MARGIN   10		/* Minimum number of pixels away
					 * from the edge of the screen a menu's
					 * left and top sides should be.
					 * (to enable backing out of 
					 * pullright menus
					 */

/* ------------------------------------------------------------------------- */

/* 
 * Public
 */
    /* None */

/* 
 * Package private
 */
Pkg_extern	struct pixrect menu_arrow_pr;	/* in walkmenu_create.c */
Pkg_extern	struct fullscreen *menu_fs;	/* in walkmenu_public.c */

Pkg_extern	int menu_curitem();
Pkg_private	int menu_render();


/* 
 * Private
 */
Private		int compute_item_size();
Private		int compute_dimensions();
Private		void compute_rects();
Private		void render_items();
Private		int render_pullright();
Private		void paint_shadow();
Private		void paint_menu();
Private		void feedback();
Private		void constrainrect();
Private		void destroy_gen_items();

/* 
 * Private defs
 */
#define	MENU_SHADOW	6 /* Width of menu shadow */
#define MENU_BORDER	1 /* Width of menu border */
#define STANDOFF	3 /* 1/2 the number of active pixels left of the menu */

extern	int	sv_journal;
extern	void	win_sync();

static int      old_mousex, old_mousey;
int             menu_count;

/* ------------------------------------------------------------------------- */

/* 
 * Menu_render modifies the inputevent parameter iep.
 * It should contain the last active inputevent read for the fd.
 */
Pkg_private int
menu_render(menu, dynamic, parent, subrect, reenter, fd)
	struct menu *menu;
	struct menu_info *dynamic;
	struct menu_item *parent;
	struct rectnode *subrect;
	int fd;
{   
    register struct menu *m;
    register struct menu_item *mi;
    register curitem, item_width, item_height;
    register struct inputevent *iep;
    register short menu_button, locx;
    struct image *std_image;
    struct menu *(*gen_proc)();
    struct rect menurect, activerect, saverect, itemrect;
    struct pixrect *menu_image, *items_image;
    Pw_pixel_cache *pixel_cache;
    int stand_off, status, gen_items;
    int margin, right_margin, ncols, nrows;

    /* 
     *  Initial setup: Pull out dynamic information.
     */
    menu->dynamic_info = dynamic, menu->parent = parent;
    dynamic->depth++;
    iep = dynamic->last_iep;
    menu_button = iep->ie_code;
    stand_off = reenter == FALSE && menu->stand_off;
    pixel_cache = PW_PIXEL_CACHE_NULL, gen_items = FALSE, gen_proc = NULL;

    /* 
     *  Dynamically create the menu if requested.
     */
    if (gen_proc = menu->gen_proc) {
	m = (gen_proc)(menu, MENU_DISPLAY);
	if (m == NULL) {
	    (void) fprintf(stderr,
		    "menu_show: menu's gen_proc failed to generate a menu.\n");
	    goto error_exit;
	}
	m->dynamic_info = dynamic, m->parent = parent;
    } else {
	m = menu;
    }

    /* 
     *  No items, return to parent menu
     */
    if (m->nitems == 0) {
	status = 1;
	goto exit;
    }

    /*
     * Find the current menu item
     */
    if (reenter) {	/* Based on default selection */
	curitem = menu_curitem(m, m->default_selection, 1);
    } else {		/* Based on initial selection */
	curitem = menu_curitem(m, m->initial_selection, 1);
	if (!curitem) {
	    status = 1;
	    goto exit;
	}		
    }

    /* 
     * Compute the size of an item.
     */
    std_image = &m->default_image;
    gen_items = compute_item_size(m, std_image);
    item_width = std_image->width;
    item_height = std_image->height;
    margin =  std_image->margin;
    right_margin = std_image->right_margin;

    /* 
     *  Automatically compute the elided dimension.
     */
    if (!compute_dimensions(m, item_width, item_height,
			    &menurect, &ncols, &nrows))
        goto error_exit;    

    /* 
     *  Compute the rects:
     *    menurect -	represents the area of the menu including its borders.
     *    activerect -	extends the left edge of the menurect.  Cursor motion
     * 			is mapped to its associated menu with this rect.
     *    saverect - 	extended menurect with includes the menu shadow.
     */
    compute_rects(m, iep, stand_off, curitem,
		  item_width, item_height, ncols, nrows,
		  &menurect, &activerect, &saverect);

    /*
     * Save the mouse position so that after selecting, it will return
     * to the same place
     */
    if (menu_count++ == 0){
      old_mousex = win_get_vuid_value(fd, LOC_X_ABSOLUTE);
      old_mousey = win_get_vuid_value(fd, LOC_Y_ABSOLUTE);
    }
    /* 
     *  Save image under menu, generate menu and display it.
     */
    if (((pixel_cache = pw_save_pixels(menu_fs->fs_pixwin, &saverect)) ==
            PW_PIXEL_CACHE_NULL) ||
        (pw_primary_cached_mpr(menu_fs->fs_pixwin, pixel_cache) ==
	    (struct pixrect *)0))
		goto error_exit;

    /*
     *  Create pixrect w/menu image.
     *  For now assume that the menu is monochrome.
     */
    menu_image = mem_create(saverect.r_width, saverect.r_height, 1);
    items_image = pr_region(menu_image, MENU_BORDER, MENU_BORDER,
			    item_width * ncols, item_height * nrows);

    paint_shadow(menu_image, m->shadow_pr,
	pw_primary_cached_mpr(menu_fs->fs_pixwin, pixel_cache));
    render_items(m, std_image, items_image, ncols, nrows);

    /* 
     * Display the menu and provide item highlighting.
     */    
    (void) pw_lock(menu_fs->fs_pixwin, &saverect);

    paint_menu(menu_image, &saverect, (m->shadow_pr)? 1: 0);

    if (!stand_off)
	feedback(m, &menurect, curitem, ncols, nrows, MENU_PROVIDE_FEEDBACK);
    (void) pw_unlock(menu_fs->fs_pixwin);
    mem_destroy(items_image);
    mem_destroy(menu_image);

    if (sv_journal) {
	win_sync(WIN_SYNC_MENU, fd);
    }

    /* From here on any returns from this procedure should go thru exit: */

    /* 
     *  Handle initial selection expansion
     */
    if (!reenter && !m->display_one_level) {
	mi = m->item_list[curitem - 1];
	if (mi->pullright && !mi->inactive && !mi->selected &&	    
	    (mi->value || mi->gen_pullright))
	{ 
	    if (!mi->gen_pullright) { /* FIXME: Should handle gen pullright */
		struct menu *mn = (struct menu *)LINT_CAST(mi->value);
		if (!menu_curitem(mn, mn->initial_selection, 0))
		    goto enter_input_loop;
	    }

	    if (stand_off) {
		feedback(m, &menurect,
			 curitem, ncols, nrows, MENU_PROVIDE_FEEDBACK);
	    }

	    locx = menurect.r_left;
	    if (m->column_major)
		locx += ((curitem - 1) / nrows + 1) * item_width + MENU_BORDER;
	    else
		locx += ((curitem - 1) % ncols + 1) * item_width + MENU_BORDER;
	    itemrect.r_width = locx;
	    locx -= margin + right_margin + mi->image->right_pr->pr_width;
	    locx -= stand_off ? STANDOFF : -STANDOFF;
	    iep->ie_locx = locx;

	    iep->ie_locy = menurect.r_top + MENU_BORDER;
	    if (m->column_major)
		iep->ie_locy += (curitem - 1) % nrows * item_height
		    + item_height / 2;
	    else
		iep->ie_locy += (curitem - 1) / ncols * item_height
		    + item_height / 2;

	    itemrect.r_left = locx;
	    itemrect.r_width -= locx;
	    itemrect.r_top = iep->ie_locy - item_height / 2;
	    itemrect.r_height = item_height;

	    (void) win_setmouseposition(menu_fs->fs_windowfd,
				 	iep->ie_locx, iep->ie_locy);

	    event_set_id(iep, menu_button);
	    status = render_pullright(mi, subrect, &menurect, &itemrect, FALSE,fd);
	    if (status == 0 || --status != 0) goto exit;
	    stand_off = FALSE;
	}
    }

enter_input_loop:
    if (stand_off) curitem = 0;
    locx = iep->ie_locx;
   /*
    * Track the mouse.
    */
    for (;;) {
	register int itemx, itemy, newitem;
	int arrow_bdry;

	if (rect_includespoint(&activerect, iep->ie_locx, iep->ie_locy)) {
	   /*
	    * Check if cursor is in the current menu
	    */
	    itemx = iep->ie_locx - menurect.r_left - MENU_BORDER;
	    if (itemx < 0) itemx = m->nitems; /* Outside menu proper */
	    else {
		itemx = itemx / item_width;
		if (itemx >= ncols) itemx = ncols - 1;
	    }

	    itemy = (iep->ie_locy - menurect.r_top - MENU_BORDER) / item_height;
	    if (itemy < 0) itemy = 0;
	    else if (itemy >= nrows) itemy = nrows - 1;

	    newitem = m->column_major ? 
		itemx * nrows + itemy + 1 : itemy * ncols + itemx + 1;

	    if (newitem > m->nitems) newitem = 0;
	} else {
            register struct rectnode *rp;
	    if (subrect && !rect_includespoint(&subrect->rn_rect,
					       iep->ie_locx, iep->ie_locy)) {
		if ((rect_includesrect(&menurect,&subrect->rn_next->rn_rect)) &&
			iep->ie_locx < menurect.r_left)  {
		    /*
		     * cursor is to the left of pullright, and
		     * parent is obscured by the child menu, so we
		     * pop back to the parent menu.
		     */
		     status = 1;
		     goto exit;
		}
	       /*
		* Check if cursor is in a parent menu
		*/

		status = 1;
		for (rp = subrect->rn_next; rp;
		     rp = rp->rn_next ? rp->rn_next->rn_next : NULL, status++) {
		    if (rect_includespoint(&rp->rn_rect,
					   iep->ie_locx, iep->ie_locy))
		        goto exit;
		}
	    }
	    newitem = 0;
	}

       /*
	* Provide feedback for new item.
	*/
	if (newitem != curitem) { /* revert item to normal state */
	    if (curitem)
		feedback(m, &menurect,
			 curitem, ncols, nrows, MENU_REMOVE_FEEDBACK);
	    curitem = newitem;   /* invert new item */
	    if (curitem) {
		feedback(m, &menurect,
			 curitem, ncols, nrows, MENU_PROVIDE_FEEDBACK);
		locx = iep->ie_locx;
	    }
	}

	if (locx >= iep->ie_locx) locx = iep->ie_locx;

	if (newitem) {
	   /*
	    * If item is a menu, recurse.
	    */
	    mi = m->item_list[newitem - 1];
	    if (mi->pullright)
		arrow_bdry = menurect.r_left + MENU_BORDER + STANDOFF
		    + (itemx * item_width) + item_width
			- (margin + (mi->image->string?right_margin:0)
			   + mi->image->right_pr->pr_width);
	    if (mi->pullright && !mi->inactive
		&& (iep->ie_locx > arrow_bdry
		    || iep->ie_locx >= locx + m->pullright_delta)
		&& (mi->value || mi->gen_pullright))
	    {   
		if (iep->ie_locx > arrow_bdry) iep->ie_locx = arrow_bdry;

		iep->ie_locy = (menurect.r_top + MENU_BORDER
				+ itemy * item_height
				+ (item_height / 2));
		event_set_id(iep, menu_button);
		/* Recurse */
		itemrect.r_left = iep->ie_locx;
		itemrect.r_width = menurect.r_left + MENU_BORDER + STANDOFF
		    + itemx * item_width + item_width - iep->ie_locx;;
		itemrect.r_top = iep->ie_locy - item_height / 2;
		itemrect.r_height = item_height;

		status =
		    render_pullright(mi, subrect, &menurect, &itemrect, TRUE,fd);

		if (status == 0 || --status != 0) goto exit;
		locx = iep->ie_locx;
	    }
	}

       /*
	* Get next input event.
	*/
	do {
	    if (input_readevent(menu_fs->fs_windowfd, iep) == -1) {
		(void) fprintf(stderr, "menu_show: failed to track cursor.\n");
		perror("menu_show");
		status = -1;
		goto exit;
	    }

	   /*
	    * If button up is the menu button then
	    * an item has been selected,  return it.
	    */
	    if (menu_button == iep->ie_code && win_inputnegevent(iep)) {
		if (curitem == 0 || m->item_list[curitem-1]->inactive)
		    m->dynamic_info->depth = 0, curitem = NULL;
		status = 0;
		goto exit;
	    }
	    else if (event_action(iep) == ACTION_HELP &&
		     win_inputposevent(iep)) {
		if (curitem == 0 || !m->item_list[curitem-1]->help_data)
		    m->dynamic_info->help = m->help_data;
		else
		    m->dynamic_info->help = m->item_list[curitem-1]->help_data;
		m->dynamic_info->depth = 0, curitem = NULL;
		status = 0;
		goto exit;
	    }
	} while (iep->ie_code != LOC_MOVEWHILEBUTDOWN &&
		 iep->ie_code != LOC_STILL && iep->ie_code != LOC_MOVE);
    }

    /* Entry states at the next higher level:
     *   status<0 abort menu chain (error or null selection),
     *   status=0 valid selection save selected item,
     *   status>0 cursor has entered a parent menu.
     */

exit:
    if (status == 0 && curitem)
        m->selected_position = curitem;
    if (status > 0) --m->dynamic_info->depth;

    /*
     * set the mouse back to the position where it was first clicked
     */
    if(--menu_count == 0)
      win_setmouseposition(menu_fs->fs_windowfd, old_mousex, old_mousey);

    pw_restore_pixels(menu_fs->fs_pixwin, pixel_cache); /* Checks for null */
    if (gen_items) destroy_gen_items(m);
    m->dynamic_info = NULL;
    if (gen_proc) {
	(gen_proc)(m, MENU_DISPLAY_DONE);
    }

    return status;

error_exit:
    status = -1;
    m->dynamic_info->depth = 0;
    goto exit;
}


/* 
 *  Compute max item size.  Only zero sized items have to be recomputed
 */
Private int
compute_item_size(menu, std_image)
    struct menu *menu;
    struct image *std_image;
{   
    register int width, height, nitems, recompute;
    register struct menu_item *mi, **mip;
    int gen_items = FALSE;

    nitems = menu->nitems;
    width = height = 0;
    recompute = std_image->width == 0;

    /* 
     *  This causes the menu to shrink around the items.
     *  When the std_image is available at the client interface zeroing the
     *  size of std_image should be rethought.
     */
    std_image->width = std_image->height = 0;

    /* Compute max size if any of the items have changed */
    for (mip = menu->item_list;mi = *mip, nitems--; mip++) {

	mi->parent = menu;

	if (mi->gen_proc) {
	    *mip = mi = (mi->gen_proc)(mi, MENU_DISPLAY);
	    gen_items = TRUE;
	}	    

	/* This is an untested (unnecessary?) optimization */
	if (recompute) mi->image->width = 0; /* This forces the size to be
					      * recompute inside image_get() */
	if (mi->image->width == 0) {
	    width = imax(image_get(mi->image, std_image, IMAGE_WIDTH), width);
	    height = imax((int)image_get(mi->image, std_image, IMAGE_HEIGHT),
			  height);
	} else {
	    width = imax(mi->image->width, width);
	    height = imax(mi->image->height, height);
	}	
    }
    std_image->width = width;
    std_image->height = height;

    return gen_items;
}


/* 
 *  Compute the dimensions of the menu.
 */
Private int
compute_dimensions(menu, iwidth, iheight, rect, ncolp, nrowp)
	register struct menu *menu;
	int iwidth, iheight;
	register Rect *rect;
	int *ncolp, *nrowp;
{   
    register int ncols, nrows;

    ncols = menu->ncols;
    nrows = menu->nrows;
    
    /* Fix 1015167: subtract SCREEN_MARGIN everywhere */

    if (!(ncols && nrows)) {
	if (ncols) {			/* case: ncols=n, nrows=to fit */
	    rect->r_width = (ncols * iwidth) +
		2*MENU_BORDER + (menu->shadow_pr ? MENU_SHADOW : 0);
	    if (rect->r_width > menu_fs->fs_screenrect.r_width
	    -SCREEN_MARGIN) {
		ncols = (menu_fs->fs_screenrect.r_width - SCREEN_MARGIN
			 - 2*MENU_BORDER + (menu->shadow_pr ? MENU_SHADOW : 0))
		    / iwidth;
	    }
	    nrows = (menu->nitems - 1) / ncols + 1;
	} else {			/* case: nrows=n, ncols=to fit */
	    if (!nrows) nrows = menu->nitems; /* Both zero, fit cols */
	    rect->r_height = (nrows * iheight) +
		2*MENU_BORDER + (menu->shadow_pr ? MENU_SHADOW : 0);
	    if (rect->r_height > menu_fs->fs_screenrect.r_height - SCREEN_MARGIN) {
		nrows = (menu_fs->fs_screenrect.r_height - SCREEN_MARGIN
			 - 2*MENU_BORDER - (menu->shadow_pr ? MENU_SHADOW : 0))
		    / iheight;

		ncols = (menu->nitems - 1) / nrows + 1;
		nrows = (menu->nitems - 1) / ncols + 1;
	    } else {
		ncols = (menu->nitems - 1) / nrows + 1;
	    }
	}
    }
    rect->r_width = (ncols * iwidth) +
	2*MENU_BORDER + (menu->shadow_pr ? MENU_SHADOW : 0);
    rect->r_height = (nrows * iheight) +
	2*MENU_BORDER + (menu->shadow_pr ? MENU_SHADOW : 0);
    if (rect->r_width > menu_fs->fs_screenrect.r_width - SCREEN_MARGIN
	|| rect->r_height > menu_fs->fs_screenrect.r_height - SCREEN_MARGIN) {
	(void) fprintf(stderr, "menu_show: Menu too large for screen.\n");
	return FALSE;
    }

    *ncolp = ncols;
    *nrowp = nrows;

    return TRUE;    
}


/* 
 *  Compute 3 rects:
 * 	mrect = menu_image;
 * 	srect = mrect + shadow
 * 	irect = input region = menu_rect + stand_off region
 */
Private void
compute_rects(menu, iep, stand_off, curitem,
	      item_width, item_height, ncols, nrows,
	      mrect, irect, srect)
	struct menu *menu;
	struct inputevent *iep;
	int stand_off, curitem, item_width, item_height, ncols, nrows;
	Rect *mrect, *irect, *srect;
{
    int left, top;

    left = item_width * (menu->column_major ?
			 (curitem - 1) / nrows : (curitem - 1) % ncols);
    top =  item_height * (menu->column_major ?
			  (curitem - 1) % nrows : (curitem - 1) / ncols);
    mrect->r_left = iep->ie_locx - left - MENU_BORDER;
    mrect->r_top = iep->ie_locy - top - MENU_BORDER - item_height / 2;

    if (left != 0) stand_off = FALSE; /* Not on the edge */

    /* Move the menu under the cursor so that it is outside/inside the item */
    mrect->r_left += stand_off ? STANDOFF : -STANDOFF;

    left = mrect->r_left, top = mrect->r_top;

    constrainrect(mrect, &menu_fs->fs_screenrect);

    if (left != mrect->r_left || top != mrect->r_top) {
	iep->ie_locx -= left - mrect->r_left;
	iep->ie_locy -= top - mrect->r_top;
	(void) win_setmouseposition(menu_fs->fs_windowfd,
				    iep->ie_locx, iep->ie_locy);
    }

    *srect = *mrect; /* Remember total size of affected area */

    if (menu->shadow_pr) {
	mrect->r_width -= MENU_SHADOW;
	mrect->r_height -= MENU_SHADOW;
    }

    *irect = *mrect;
    if (stand_off) {
	irect->r_left -= STANDOFF;
	irect->r_width += STANDOFF;
    }
}


/* 
 *  Rop items into supplied pixrect
 */
Private void
render_items(menu, std_image, pr, ncols, nrows)
	struct menu *menu;
	struct image *std_image;
	struct pixrect *pr;
{   
    register int top, left, n, i, j;
    register struct menu_item **mip;
    int item_width, item_height;
    register total_height, total_width;

    item_width = std_image->width;
    item_height = std_image->height;
    mip = menu->item_list;

    n = 0;
    total_height = item_height * nrows;
    total_width = item_width * ncols;
    if (menu->column_major) {
	for (j = left = 0; j < ncols; j++) {
	    for (i = top = 0; i < nrows; i++) {
		if (++n > menu->nitems) break;
		if ((*mip)->inactive) {
		    image_render((*mip)->image, std_image,
				 LINT_CAST(IMAGE_REGION), pr, left, top,
				 IMAGE_INACTIVE,
				 0);
		} else {
		    image_render((*mip)->image, std_image,
				 LINT_CAST(IMAGE_REGION), pr, left, top,
				 0);
		}
		if (menu->h_line || (*mip)->h_line)  {
		    image_vector(0,top+item_height,total_width-1,top+item_height);
		}
		if (menu->v_line || (*mip)->v_line)  {
		    image_vector(left+item_width,0,left+item_width,total_height-1);
		}
		top += item_height;
		mip++;
	    }
	    left += item_width;
	}
    } else {
	for (i = top = n = 0; i < nrows; i++) {
	    for (j = left = 0; j < ncols; j++) {
		if (++n > menu->nitems) break;
		if ((*mip)->inactive)
		    image_render((*mip)->image, std_image,
				 LINT_CAST(IMAGE_REGION), pr, left, top,
				 IMAGE_INACTIVE,
				 0);
		else {
		    image_render((*mip)->image, std_image,
				 LINT_CAST(IMAGE_REGION), pr, left, top,
				 0);
		}
		if (menu->h_line || (*mip)->h_line)  {
		    image_vector(0,top+item_height,total_width-1,top+item_height);
		}
		if (menu->v_line || (*mip)->v_line)  {
		    image_vector(left+item_width,0,left+item_width,total_height-1);
		}
		left += item_width;
		mip++;
	    }
	    top += item_height;
	}
    }
}


/* 
 *  Handle recursive calls for pullright items
 */
Private int
render_pullright(mi, subrect, menurect, itemrect, reenter, fd)
	register struct menu_item *mi;
	Rectnode *subrect;
	Rect *menurect, *itemrect;
	int  fd;
{   
    register struct menu *m, *(*gen_proc)();
    struct rectnode menu_node, active_node;
    int status;


    if (gen_proc = mi->gen_pullright) {
	m = (gen_proc)(mi, MENU_DISPLAY);
	if (!m) {
	    (void) fprintf(stderr,
	      "menu_show: gen_proc failed to generate a pullright menu.\n");
	    return -1;
	}
	mi->value = (caddr_t)m;
    } else {
	m = (struct menu *)LINT_CAST(mi->value);
    }

    menu_node.rn_next = subrect, menu_node.rn_rect = *menurect;
    active_node.rn_next = &menu_node, active_node.rn_rect = *itemrect;

    status =
	menu_render(m, mi->parent->dynamic_info, mi, &active_node, reenter, fd);

    if (gen_proc)
	mi->value = (caddr_t)(gen_proc)(mi, MENU_DISPLAY_DONE);

    return status;
}


Private void
paint_shadow(pr, shadow_pr, save_pr)
	register struct pixrect *pr, *shadow_pr, *save_pr;
{
    if (shadow_pr) {
 	/* Draw borders with shadows */
	(void) pr_rop(pr, 0, 0, pr->pr_width - MENU_SHADOW, MENU_BORDER,
	       	      PIX_SET, PIXRECT_NULL, 0, 0);
	(void) pr_rop(pr, 0, MENU_BORDER,
	       	      MENU_BORDER, pr->pr_height - MENU_SHADOW - 2*MENU_BORDER,
	       	      PIX_SET, PIXRECT_NULL, 0, 0);
	(void) pr_rop(pr,
	       	      pr->pr_width - MENU_SHADOW - MENU_BORDER, MENU_BORDER,
	       	      MENU_BORDER, pr->pr_height - MENU_SHADOW - 2*MENU_BORDER,
	       	      PIX_SET, PIXRECT_NULL, 0, 0);
	(void) pr_rop(pr, 0, pr->pr_height - MENU_SHADOW - MENU_BORDER,
	       	      pr->pr_width - MENU_SHADOW, MENU_BORDER,
	       	      PIX_SET, PIXRECT_NULL, 0, 0);
	/* Fill in with background */
	(void) pr_rop(pr, pr->pr_width - MENU_SHADOW, 0,
		      MENU_SHADOW, pr->pr_height,
		      PIX_SRC, save_pr, pr->pr_width - MENU_SHADOW, 0);
	(void) pr_rop(pr, 0, pr->pr_height - MENU_SHADOW,
		      pr->pr_width - MENU_SHADOW, MENU_SHADOW,
		      PIX_SRC, save_pr, 0, pr->pr_height - MENU_SHADOW);
	/* Rop shadow over background */
	(void) pr_replrop(pr, pr->pr_width - MENU_SHADOW, MENU_SHADOW,
		   	  MENU_SHADOW, pr->pr_height - MENU_SHADOW,
		   	  PIX_SRC /* |PIX_DST opaque shadow */, shadow_pr,
		   	  pr->pr_width - MENU_SHADOW, MENU_SHADOW);
	(void) pr_replrop(pr, MENU_SHADOW, pr->pr_height - MENU_SHADOW,
		   	  pr->pr_width - 2 * MENU_SHADOW, MENU_SHADOW,
		   	  PIX_SRC /* |PIX_DST  */, shadow_pr,
		   	  MENU_SHADOW, pr->pr_height - MENU_SHADOW);
    } else {
 	/* Draw borders without shadows */
	(void) pr_rop(pr, 0, 0, pr->pr_width, MENU_BORDER, PIX_SET,
		      PIXRECT_NULL, 0, 0);
	(void) pr_rop(pr, 0, MENU_BORDER - 1,
	              MENU_BORDER, pr->pr_height - 2*MENU_BORDER,
	       	      PIX_SET, PIXRECT_NULL, 0, 0);
	(void) pr_rop(pr, pr->pr_width - MENU_BORDER - 1, MENU_BORDER - 1,
	       	      MENU_BORDER, pr->pr_height - 2*MENU_BORDER, PIX_SET,
	       	      PIXRECT_NULL, 0, 0);
	(void) pr_rop(pr, 0, pr->pr_height - MENU_BORDER - 1,
	       	      pr->pr_width, MENU_BORDER,
	       	      PIX_SET, PIXRECT_NULL, 0, 0);
    }
}


Private void
paint_menu(image, rect, shadow)
	register struct pixrect *image;
	register Rect *rect;
	register int shadow;
{
    if (menu_fs->fs_pixwin->pw_pixrect->pr_depth > 1 && shadow) {
	rect->r_width -= MENU_SHADOW;
	rect->r_height -= MENU_SHADOW;
    }

    (void) pw_preparesurface_full(menu_fs->fs_pixwin, rect, 1);
    (void) pw_write(menu_fs->fs_pixwin,
	     	    rect->r_left, rect->r_top, rect->r_width, rect->r_height,
	     	    PIX_SRC, image, 0, 0);

    if (menu_fs->fs_pixwin->pw_pixrect->pr_depth > 1 && shadow) {
	Rect shadow_rect;

	shadow_rect.r_left = rect->r_left + rect->r_width;
	shadow_rect.r_top = rect->r_top + MENU_SHADOW;
	shadow_rect.r_width = MENU_SHADOW;
	shadow_rect.r_height = rect->r_height;
	
	(void) pw_preparesurface_full(menu_fs->fs_pixwin, &shadow_rect, 1);
	(void) pw_write(menu_fs->fs_pixwin,
		 	shadow_rect.r_left,
		 	shadow_rect.r_top,
		 	MENU_SHADOW, rect->r_height,
		 	PIX_SRC /* | PIX_DST  opaque shadow */, image,
		 	image->pr_width - MENU_SHADOW, MENU_SHADOW);
 
         shadow_rect.r_left = rect->r_left + MENU_SHADOW;
	 shadow_rect.r_top = rect->r_top + rect->r_height;
	 shadow_rect.r_width = rect->r_width - MENU_SHADOW;
         shadow_rect.r_height = MENU_SHADOW;
	 (void)pw_preparesurface_full(menu_fs->fs_pixwin, &shadow_rect, 1); 
	 (void) pw_write(menu_fs->fs_pixwin,
		 	shadow_rect.r_left,
			shadow_rect.r_top,
 		 	shadow_rect.r_width, MENU_SHADOW,
		 	PIX_SRC /* | PIX_DST opaque shadow */, image,
		 	MENU_SHADOW, image->pr_height - MENU_SHADOW);
	rect->r_width += MENU_SHADOW;
	rect->r_height += MENU_SHADOW;
    }
}


/* 
 *  Provide feedback directly to the pixwin.
 *  Someday this should be a client settable option.
 */
Private void
feedback(m, r, n, ncols, nrows, state)
	register struct menu *m;
	struct rect *r;
	int n, ncols, nrows;
	Menu_feedback state;
{
    struct menu_item *mi = m->item_list[n - 1];
    int max_width = m->default_image.width;
    int max_height = m->default_image.height;

    if (!mi->no_feedback && !m->feedback_proc && !mi->inactive) {
	int left, top;
	register int margin = m->default_image.margin;

	n--; /* Convert to zero origin */
	left = (r->r_left + MENU_BORDER + margin
		+ (m->column_major ? n / nrows : n % ncols) * max_width);
	top = (r->r_top + MENU_BORDER + margin
	       + (m->column_major ? n % nrows : n / ncols) * max_height);
	(void) pw_writebackground(menu_fs->fs_pixwin, left, top,
		 max_width - 2  * margin,
		 max_height - 2 * margin,
		 PIX_NOT(PIX_DST));
    } else if (m->feedback_proc)
	(m->feedback_proc)(mi, state); /* FIXME: Pass rect or region? */
}


/* 
 *  Menu must be completely on the screen.
 */
Private void
constrainrect(rconstrain, rbound)
	register struct rect *rconstrain, *rbound;
{
   /*
    * Bias algorithm to have too big rconstrain fall off right/bottom.
    */
    if (rect_right(rconstrain) > rect_right(rbound)) {
	rconstrain->r_left = rbound->r_left + rbound->r_width
	    - rconstrain->r_width;
    }
    if (rconstrain->r_left < rbound->r_left) {
	rconstrain->r_left = rbound->r_left + SCREEN_MARGIN;
    }
    if (rect_bottom(rconstrain) > rect_bottom(rbound)) {
	rconstrain->r_top = rbound->r_top + rbound->r_height
	    - rconstrain->r_height;
    }
    if (rconstrain->r_top < rbound->r_top) {
	rconstrain->r_top = rbound->r_top + SCREEN_MARGIN;
    }
}


/*
 *  Clean up any client generated items
 */
Private void
destroy_gen_items(menu)
    struct menu *menu;
{   
    register int nitems;
    register struct menu_item *mi, **mip;

    nitems = menu->nitems;
    /* Give client a chance to clean up any generated items */
    for (mip = menu->item_list;mi = *mip, nitems--; mip++)
	if (mi->gen_proc) *mip = (mi->gen_proc)(mi, MENU_DISPLAY_DONE);
}
