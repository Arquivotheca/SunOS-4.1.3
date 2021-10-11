#ifndef lint
static	char sccsid[] = "@(#)winshared.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * SunWindows Desktop shared memory update module.
 */

#include <sunwindowdev/wintree.h>
#include <sunwindow/cursor_impl.h>	/* for cursor access macros */
#include <sunwindow/cms_mono.h>	/* for cursor access macros */
#include <pixrect/pr_util.h>		/* for mpr support */
#include <pixrect/pr_planegroups.h>	/* for plane groups */

void win_shared_update_cursor_image();

void
win_shared_update_cursor(dtop)
	register Desktop	*dtop;
{
	Win_lock_block		*shared_info = dtop->shared_info;
	register Window		*cursorwin = dtop->dt_cursorwin;
	register Win_shared_cursor *shared_cursor = &shared_info->cursor_info;
	int hw_cursor_start;

	if (win_lock_mutex_locked(shared_info)) {
		shared_info->status.update_cursor = TRUE;
		return;
	}

	hw_cursor_start = shared_info->cursor_info.hardware_cursor;
	/* When real time cursor, don't want normal cursor being displayed */
	if (!cursorwin) {
		/* no cursor window, so zero the cursor info */
		shared_cursor->cursor.flags |= DONT_SHOW_CURSOR;
		shared_cursor->pr.pr_width = shared_cursor->pr.pr_height = 0;
		shared_info->cursor_info.hardware_cursor = 0;
		dtop->dt_flags &= ~DTF_HWCURNOTSHOWN;
	} else if (dtop->dt_flags & DTF_HWCURSORAVAILABLE && /*Available on fb*/
	    !(cursorwin->w_cursor.flags & NOT_IN_HW) &&     /* Hw cursor OK */
	    dtop_cursor_ok_for_hw(dtop)) {	    /* Op and size suitable */
		/* Do hardware cursor tracking */
		shared_info->cursor_info.hardware_cursor = 1;
		/* make sure pw_shared.c code knows about any crosshairs! */
		win_shared_update_cursor_image(dtop);
		/* Don't update image if client owns control of it */
		if (!(cursorwin->w_cursor.flags & CLIENT_OWNS_IMAGE))
			/*
			 * If "don't show" then remove, once.  This is ment to
			 * implicitly detect and handle old "CLIENT_OWNS_IMAGE"
			 * clients, like 1.* versions of xnews.
			 */
			if (cursorwin->w_cursor.flags & DONT_SHOW_CURSOR) {
				if (!(dtop->dt_flags & DTF_HWCURNOTSHOWN)) {
					dtop_remove_hw_cursor(dtop);
					dtop->dt_flags |= DTF_HWCURNOTSHOWN;
				}
			} else {
				/* Update the hw cursor shape & set position */
				if (dtop_load_hw_cursor(dtop)) {
					/* Something wrong so revert to sw */
					win_shared_update_cursor_image(dtop);
					shared_info->cursor_info.hardware_cursor
					     = 0;
				}
				dtop->dt_flags &= ~DTF_HWCURNOTSHOWN;
			}
	} else {

		/*
		 * The 24-bit plane group is too large an area to mmap
		 * into the kernel (open to debate?).  We leave the
		 * cursor in the overlay for our 24-bit framebuffers.
		 * (currently cg8 and cg9).
		 */

		if (cursorwin->w_plane_group == PIXPG_24BIT_COLOR)
		{
		    unsigned	char	r[1],g[1],b[1];

		    /*
		     * If the subwindow is color, make the cursor the
		     * foreground color of the subwindow.  This looks
		     * great until something like a reverse-video
		     * namestripe is encountered.  Unfortunately, since
		     * the subwindow and the cursor are in different
		     * planes, it becomes next to impossible to properly
		     * operate most cursor OPs.
		     */

		    if (strncmp(cursorwin->w_cms.cms_name, CMS_MONOCHROME,
			CMS_NAMESIZE))
		    {
			int	i;

			i = cursorwin->w_cms.cms_addr +
			    cursorwin->w_cms.cms_size - 1;
			r[0] = dtop->dt_cmap.cm_red[i];
			g[0] = dtop->dt_cmap.cm_green[i];
			b[0] = dtop->dt_cmap.cm_blue[i];
		    }
		    else
			r[0] = g[0] = b[0] = 255;

		    pr_putcolormap(dtop->dt_pixrect,
			2 | PIX_GROUP(PIXPG_OVERLAY) | PR_FORCE_UPDATE,
			1, r, g, b);
		}
		win_shared_update_cursor_image(dtop);
		shared_info->cursor_info.hardware_cursor = 0;
		dtop->dt_flags &= ~DTF_HWCURNOTSHOWN;
	}
	
	shared_info->mouse_plane_group = (!dtop->dt_cursorwin)?
	    PIXPG_CURRENT: dtop->dt_cursorwin->w_plane_group;
#ifndef PRE_FLAMINGO
        /* Redirect cursor to be in enable plane */
	if (shared_info->mouse_plane_group == PIXPG_VIDEO) {
	    shared_info->mouse_plane_group = PIXPG_VIDEO_ENABLE;
	    cursorwin->w_cursor.cur_function = PIX_NOT(PIX_SRC) & PIX_DST;
	}
#endif
	if (shared_info->mouse_plane_group == PIXPG_24BIT_COLOR)
	    shared_info->mouse_plane_group = PIXPG_OVERLAY;

	/*
	 * Reset cursor_active from cursor active cache if transitioning from
	 * hardware to software cursor.
	 */
	if (hw_cursor_start && !shared_info->cursor_info.hardware_cursor) {
		shared_info->cursor_info.cursor_active =
		    shared_info->cursor_info.cursor_active_cache;
		/* Remove the hardware cursor from screen */
		dtop_remove_hw_cursor(dtop);
	}
}

void
win_shared_update_cursor_image(dtop)
	register Desktop	*dtop;
{
	register Window		   *cursorwin = dtop->dt_cursorwin;
	register Win_shared_cursor *shared_cursor =
	    &dtop->shared_info->cursor_info;

	shared_cursor->cursor = cursorwin->w_cursor;
	shared_cursor->pr = cursorwin->w_cursormpr;
	shared_cursor->pr_data = cursorwin->w_cursordata;
	bcopy((caddr_t)cursorwin->w_cursorimage,
	    (caddr_t)shared_cursor->image, CUR_MAXIMAGEBYTES);
}

void
win_shared_update_cursor_active(shared_info, active)
	Win_lock_block		*shared_info;
	int			active;
{
	if (win_lock_mutex_locked(shared_info)) {
		shared_info->status.update_cursor_active = TRUE;
		return;
	}
	/*
	 * The cursor_active field is set by the kernel as the locator
	 * transitions from one desktop to the next.  It is never read by the
	 * kernel and is only read by the functions in
	 * libsunwindow/pw/pw_shared.c.  These functions use it to determine
	 * whether to bother thinking about putting up or taking down the
	 * cursor.
	 *
	 * We are using the cursor_active_cache bit in the win_shared_cursor
	 * struct as a cache of whether the kernel thinks that the cursor is
	 * on the desktop or not.  This cache is there because we are slightly
	 * permuting the cursor_active field to deal with hardware cursors.
	 * The cursor_active field is now going to be simply a communications
	 * to clients whether they should bother thinking about putting up or
	 * taking down the cursor.  This field may change on a window by
	 * window basis depending upon the usage of a hardware cursor or not.
	 * The cache is around to that as the cursor switches from
	 * hardware to software, the cache can be used to update cursor_active.
	 *
	 * Part of this is predicated upon the fact that
	 * win_shared_update_cursor_active is called with a TRUE value while
	 * the cursor is down and is then followed by a call to dtop_setcursor
	 * which ends up calling win_shared_update_cursor when necessary.
	 * win_shared_update_cursor then perturbs cursor_active, or restores
	 * cursor_active with cursor_active_cache.
	 */
	shared_info->cursor_info.cursor_active =
	    shared_info->cursor_info.cursor_active_cache = active;
}

void
win_shared_update_mouse_xy(shared_info, x, y)
	Win_lock_block	*shared_info;
	int		x, y;
{
	if (win_lock_mutex_locked(shared_info)) {
		shared_info->status.update_mouse_xy = TRUE;
		return;
	}

	shared_info->mouse_x = x;
	shared_info->mouse_y = y;
}


void
win_shared_update(dtop)
	register Desktop	*dtop;
{
	register Win_lock_block		*shared_info = dtop->shared_info;
	register Win_lock_status	*status = &shared_info->status;

	if (win_lock_mutex_locked(shared_info))
		return;

	if (status->update_cursor) {
		if (dtop->dt_cursorwin)
			win_shared_update_cursor(dtop);
		status->update_cursor = FALSE;
	}

	if (status->update_cursor_active) {
		win_shared_update_cursor_active(shared_info, 
		    dtop->dt_ws->ws_loc_dtop == dtop);
		status->update_cursor_active = FALSE;
	}

	if (status->update_mouse_xy) {
		win_shared_update_mouse_xy(shared_info, dtop->dt_rt_x, 
		    dtop->dt_rt_y);
		status->update_mouse_xy = FALSE;
	}
}
