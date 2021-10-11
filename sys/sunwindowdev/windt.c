#ifndef lint
static  char sccsid[] = "@(#)windt.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * SunWindows Desktop driver with multiple screens support.
 */

#include <sunwindowdev/wintree.h>
#include <sunwindow/cursor_impl.h>	/* for cursor access macros */
#include <pixrect/pr_util.h>		/* for mpr support */
#include <pixrect/pr_dblbuf.h>		/* for double buffering support */
#include <pixrect/pr_planegroups.h>	/* for plane groups */
#include <sys/file.h>			/* for FREAD and FWRITE */
#include <sys/kmem_alloc.h>
#include <sun/fbio.h>
#include <sunwindow/pw_dblbuf.h>
#include <sunwindow/win_ioctl.h>

caddr_t getpages();
void freepages();

extern	struct window *wt_intersected();
extern	int wt_notifychanged();
extern	bool wt_isindisplaytree();

void	dtop_putcolormap();

static void	dtop_init_data_lock();
static void	dtop_init_display_lock();
static void	dtop_init_mutex_lock();
static void	dtop_update_cursor();

static int	dtop_check_lock();

int wincursordisable;	/* Temp: Turns off displaying of the cursor */
int win_do_hw_cursor = 1;	/* Controls hardware cursor stuff */
int fbio_kcopy;			/* See FBIOSCUROR call below */

#ifdef WINDEVDEBUG
int win_waiting_debug;	/* to printf when wakeup waiter */
int win_sharedq_debug;	/* to printf when messing with shared queue */
#endif

void	dtop_change_loc_dtop();
void	dtop_update_enable();
void	dtop_set_enable();
void	dtop_flip_display();	/* Double buffering flip */
void	dtop_choose_dblbuf();
void	dtop_changedisplay();
int	dtop_isdblwin();
int	dtop_dblset();
void	dtop_copy_dblbuffer();

/*
 * The following should only be call when the display lock is off
 * or when you are not sure.
 */
dtop_cursorup(dtop)
	register Desktop *dtop;
{
	if (dtop && !(cursor_up(dtop) ||
	    horiz_hair_up(dtop) || vert_hair_up(dtop)) &&
	    (!win_lock_display_locked(dtop->shared_info))) {
		dtop_drawcursor(dtop);
	}
}

dtop_cursordown(dtop)
	register Desktop *dtop;
{
	if ((cursor_up(dtop) || horiz_hair_up(dtop) || vert_hair_up(dtop)) &&
	    (!win_lock_display_locked(dtop->shared_info))) {
		dtop_drawcursor(dtop);
	}
}

int
dtop_cursor_ioctl(dtop, cmd, data, surpress_err_msg)
	register Desktop *dtop;
	int cmd;
	caddr_t data;
	int surpress_err_msg;
{
	register struct file *fp = dtop->dt_fbfp;
	int err;

	/*
	 * Can't just do ioctls like when setting up the framebuffer
	 * due to the lack of a u area when called from a timer.
	 * This call not suitable for streams.  Might be OK to from
	 * some source other than an timer.
	 */
	err = VOP_IOCTL((struct vnode *)fp->f_data, cmd, data, fp->f_flag,
	    fp->f_cred);
	if (err) {
		/* On error, disable hardware cursor tracking */
		dtop->dt_flags &= ~DTF_HWCURSORAVAILABLE;
		/* Notify user of error */
		if (!surpress_err_msg) {
			if (err == ENOTTY)
				printf(
	  "Tried to use hardware cursor on %s but it is not available.\n",
				    dtop->dt_screen.scr_fbname);
			else
				printf(
  "Disabled hardware cursor due to dtop cursor ioctl err == %D, cmd == %X\n",
				    err, cmd);
		}
		/*
		 * Used to call dtop_set_cursor or win_shared_update_cursor
		 * to show a normal cursor.  However, I was uncomfortable with
		 * the possible circularity situations that could pose.
		 * So, we are just letting some user action (motion
		 * across window boundary, bringing up a menu, etc.)
		 * to cause a new cursor image to be shown in this
		 * rare case (most likely due to user error).
		 */
	}
	return err;
}

void
dtop_hw_cursor_pos_update(dtop)
	register Desktop *dtop;
{
	/* See if doing using hardware cursor tracking */
	if (dtop->shared_info->cursor_info.hardware_cursor) {
		struct fbcurpos cp;

		/* Initialize the cursor position */
		cp.x = dtop->dt_rt_x;
		cp.y = dtop->dt_rt_y;
		/* Update the cursor position */
		(void) dtop_cursor_ioctl(dtop, FBIOSCURPOS, (caddr_t)&cp, 0);
	}
}

int
dtop_cursor_ok_for_hw(dtop)
	register Desktop	*dtop;
{
	struct cursor *cursor = &dtop->dt_cursorwin->w_cursor;
	struct fbcurpos size;

	/* Determine if op is suitable for hardware cursor */
	switch (cursor->cur_function & PIX_SET) {
	case PIX_SRC:
	case PIX_DST:
	case PIX_SRC|PIX_DST:
	case PIX_SRC&PIX_DST:
		/* See if size is OK */
		if (!dtop_cursor_ioctl(dtop, FBIOGCURMAX, (caddr_t)&size, 0) &&
		    (size.x < cursor->cur_shape->pr_width ||
		     size.y < cursor->cur_shape->pr_height))
			return 0;
		return 1;
	default:
		return 0;
	}
}

int
dtop_load_hw_cursor(dtop)
	register Desktop	*dtop;
{
	extern struct pixrectops mem_ops;
	register Window *cursorwin = dtop->dt_cursorwin;
	register struct cursor *cursor = &cursorwin->w_cursor;
	struct fbcursor fbc;
#define	CUR_IMAGE_BYTES	32*4	/* REMIND: should be dynamic but what the heck,
				 * SunWindows doesn't support large cursors */
#define	CUR_CMAP_LEN	2
#define	CUR_CMAP_BG	0
#define	CUR_CMAP_FG	1
	unsigned char red[CUR_CMAP_LEN];
	unsigned char green[CUR_CMAP_LEN];
	unsigned char blue[CUR_CMAP_LEN];
	char image[CUR_IMAGE_BYTES];
	char mask[CUR_IMAGE_BYTES];
	struct mpr_data md;
	struct pixrect pr;
	int cursor_fg_index;
	int err;

	/* Clear struct */
	bzero((caddr_t)&fbc, sizeof (struct fbcursor));
	/* Tell fb to pay attention to all fields */
	fbc.set = FB_CUR_SETALL;
	/* Enable hardware cursor, even if already on */
	fbc.enable = 1;
	/* Set pos in this call so done atomically */
	fbc.pos.x = dtop->dt_rt_x;
	fbc.pos.y = dtop->dt_rt_y;
	/* Set hot spot */
	fbc.hot.x = cursor->cur_xhot;
	fbc.hot.y = cursor->cur_yhot;
	/* Initialize colormap */
	bzero((caddr_t)red, CUR_CMAP_LEN);
	bzero((caddr_t)green, CUR_CMAP_LEN);
	bzero((caddr_t)blue, CUR_CMAP_LEN);
	fbc.cmap.index = 0;
	fbc.cmap.count = CUR_CMAP_LEN;
	fbc.cmap.red = red;
	fbc.cmap.green = green;
	fbc.cmap.blue = blue;
	/* Initialize planes */
	bzero(image, CUR_IMAGE_BYTES);
	bzero(mask, CUR_IMAGE_BYTES);
	bzero((caddr_t)&pr, sizeof(struct pixrect));
	bzero((caddr_t)&md, sizeof(struct mpr_data));
	pr.pr_ops = &mem_ops;
	pr.pr_depth = 1;
	pr.pr_width = 32;		/* Needs to be padded to 4 bytes */
	pr.pr_height = 32;
	pr.pr_data = (caddr_t)&md;
	md.md_image = (short *)image;
	md.md_linebytes = mpr_linebytes(32, 1);
	fbc.image = image;	/* Mask set below */
	fbc.size.x = cursor->cur_shape->pr_width;
	fbc.size.y = cursor->cur_shape->pr_height;
	/* I don't believe that line bytes are set anywhere else */
	cursorwin->w_cursordata.md_linebytes = mpr_linebytes(
	    cursor->cur_shape->pr_width, 1);
	/* Transfer the image */
	if (pr_rop(&pr, 0, 0, fbc.size.x, fbc.size.y,
	    PIX_SRC, &cursorwin->w_cursormpr, 0, 0))
		printf("HW cursor rop err 1\n");
	/*
	 * The cursor foreground color index is either the specified color or
	 * the window's foreground color.
	 */
	cursor_fg_index = PIX_OPCOLOR(cursor->cur_function);
	if (cursorwin->w_cms.cms_size == 0) {
		/* Pick default colors if not set on window yet */
		red[CUR_CMAP_BG] = dtop->dt_screen.scr_background.red;
		green[CUR_CMAP_BG] = dtop->dt_screen.scr_background.green;
		blue[CUR_CMAP_BG] = dtop->dt_screen.scr_background.blue;
		red[CUR_CMAP_FG] = dtop->dt_screen.scr_foreground.red;
		green[CUR_CMAP_FG] = dtop->dt_screen.scr_foreground.green;
		blue[CUR_CMAP_FG] = dtop->dt_screen.scr_foreground.blue;
	} else {
		int cmap_offset;
		struct	cms_map cmap;

		/* Set the foreground color if not explicitly set */
		if ((cursor_fg_index == 0) ||
		    (cursor_fg_index >= cursorwin->w_cms.cms_size))
			cursor_fg_index = cursorwin->w_cms.cms_size-1;
		/* Window color map information can come from one of 2 places */
		if (cursorwin->w_flags & WF_CMSHOG)
			cmap = cursorwin->w_cmap;
		else
			cmap = dtop->dt_cmap;
		/* Window cms addr applies to either shared or private cmaps */
		cmap_offset = cursorwin->w_cms.cms_addr;
		/* Setup the colormap (will be don't cares for some ops) */
		red[CUR_CMAP_BG] = cmap.cm_red[cmap_offset];
		green[CUR_CMAP_BG] = cmap.cm_green[cmap_offset];
		blue[CUR_CMAP_BG] = cmap.cm_blue[cmap_offset];
		red[CUR_CMAP_FG] = cmap.cm_red[cmap_offset+cursor_fg_index];
		green[CUR_CMAP_FG] =cmap.cm_green[cmap_offset+cursor_fg_index];
		blue[CUR_CMAP_FG] = cmap.cm_blue[cmap_offset+cursor_fg_index];
	}
	/*
	 * Set up the mask according to what the op is.
	 * The decisions here should be reflected in dtop_cursor_op_ok_for_hw.
	 *
	 * (image, mask):
	 *	(x,0) means show framebuffer (transparent).
	 *	(0,1) means show image background.
	 *	(1,1) means show image foreground.
	 */
	md.md_image = (short *)mask;
	switch (cursor->cur_function & PIX_SET) {
	case PIX_SRC:
		/*
		 * Mask shape: all ones the size of the image.
		 * Image background: window background.
		 * Image foreground: cursor foreground color.
		 */
		if (pr_rop(&pr, 0, 0, fbc.size.x, fbc.size.y,
		    PIX_SET, (struct pixrect *)0, 0, 0))
			printf("HW cursor rop err 2\n");
		fbc.mask = mask;
		break;
	case PIX_DST:
		/*
		 * Mask shape: all zeros.
		 * Image background: don't care.
		 * Image foreground: don't care.
		 */
		if (pr_rop(&pr, 0, 0, fbc.size.x, fbc.size.y,
		    PIX_CLR, (struct pixrect *)0, 0, 0))
			printf("HW cursor rop err 3\n");
		fbc.mask = mask;
		break;
	case PIX_SRC|PIX_DST:
		/*
		 * Mask shape: same shape as image.
		 * Image background: don't care.
		 * Image foreground: cursor foreground color.
		 */
		fbc.mask = image;
		break;
	case PIX_SRC&PIX_DST:
		/*
		 * Mask shape: all ones except where image has ones.
		 * Image background: window background
		 * Image foreground: don't care.
		 */
		if (pr_rop(&pr, 0, 0, fbc.size.x, fbc.size.y,
		    PIX_SET, (struct pixrect *)0, 0, 0))
			printf("HW cursor rop err 4\n");
		if (pr_rop(&pr, 0, 0, fbc.size.x, fbc.size.y,
		    PIX_SRC^PIX_DST, &cursorwin->w_cursormpr, 0, 0))
			printf("HW cursor rop err 5\n");
		fbc.mask = mask;
		break;
	case PIX_NOT(PIX_SRC)&PIX_DST:
		/* TBD */
	case PIX_NOT(PIX_SRC)|PIX_DST:
		/* TBD */
	default:
		/* Do in software */
		return (EINVAL);
	}
	/*
	 * In order to avoid a security problem with having the
	 * FBIOSCURSOR implementor use kcopy to copy stuff, we go
	 * into a mode that temporarily allows kcopy to be used.
	 */
	fbio_kcopy = 1;
	err = dtop_cursor_ioctl(dtop, FBIOSCURSOR, (caddr_t)&fbc, 0);
	fbio_kcopy = 0;
	return err;
}

void
dtop_remove_hw_cursor(dtop)
	register Desktop	*dtop;
{
	struct fbcursor fbc;

	/* Clear struct, including critical enable flag */
	bzero((caddr_t)&fbc, sizeof (struct fbcursor));
	fbc.set = FB_CUR_SETCUR | FB_CUR_SETHOT;
	(void) dtop_cursor_ioctl(dtop, FBIOSCURSOR, (caddr_t)&fbc, 1);
}

dtop_openfb(dtop, fd)
	register Desktop *dtop;
	int	fd;
{
	register int	err = 0;
	struct	fbpixrect fbpr;
	struct	singlecolor *bkgnd = &dtop->dt_screen.scr_background;
	struct	singlecolor *frgnd = &dtop->dt_screen.scr_foreground;
	struct	singlecolor *tmpgnd;
	register struct	dtopcursor *dc;
	caddr_t dtop_alloccmapdata();
	struct	fbgattr fbgattr;
	int	(*ioctl_op)();
	register int	i;
	int plane_group_count;
	int	max_depth = 1;
	u_int lock_size;
	struct fbcurpos cp;

	if (fd == WSID_DEVNONE)
		return (0);
	/*
	 * Dup file descriptor for frame buffer.
	 */
	err = kern_dupfd(fd, &dtop->dt_fbfp);
	if (err)
		return (err);
	ioctl_op = dtop->dt_fbfp->f_ops->fo_ioctl;
	/* Determine colormap size */
	err = (*ioctl_op)(dtop->dt_fbfp, FBIOGATTR, &fbgattr);
	if (err) {
		if (err != ENOTTY) {
#ifdef WINDEVDEBUG
			printf("ioctl err FBIOGATTR\n");
#endif
			return (err);
		}
		/* Use older ioctl if FBIOGATTR not implemented */
		err = (*ioctl_op)(dtop->dt_fbfp, FBIOGTYPE, &fbgattr.fbtype);
		if (err) {
#ifdef WINDEVDEBUG
			printf("ioctl err FBIOGTYPE\n");
#endif
			return (err);
		}
	}
	/*
	 * Colormap may not be less then 2 long (2 long is monochrome case).
	 * Will allow user programs to allocate colormap segments bigger
	 * than dt_cmsize.  However, colormap entries greater than the
	 * length of the map will be ignored.  This convention enables
	 * programs written for color displays to run without errors on
	 * a monochrome display.
	 */
	if ((dtop->dt_cmsize = fbgattr.fbtype.fb_cmsize) < 2) {
#ifdef WINDEVDEBUG
		printf("illegal colormap size (%D)\n", fbgattr.fbtype.fb_cmsize);
#endif
		return (EINVAL);
	}

	/*
	 * Get pixrect for framebuffer.
	 */
	err = (*ioctl_op)(dtop->dt_fbfp, FBIOGPIXRECT, &fbpr);
	if (err) {
#ifdef WINDEVDEBUG
		printf("ioctl err FBIOGPIXRECT\n");
#endif
		return (err);
	}
	dtop->dt_pixrect = fbpr.fbpr_pixrect;
	if (dtop->dt_pixrect==(struct pixrect *)0)
		return (EBUSY);
	if (dtop->dt_pixrect==(struct pixrect *)-1)
		return (EINVAL);


	/* Determine double buffering capabilities */
	err = pr_dbl_get(dtop->dt_pixrect, PR_DBL_AVAIL);
	if (err == PR_DBL_EXISTS) {
		dtop->dt_flags |= DTF_DBLBUFFER;
		dtop->dt_curdbl = WINDOW_NULL;
		dtop->dt_dbl_bkgnd = PR_DBL_B;
		dtop->dt_dbl_frgnd = PR_DBL_A;
		dtop->dt_curdbl = 0;
	}

	/* Allocate colormap resource map */
	dtop->dt_cmapdata = dtop_alloccmapdata(dtop, &dtop->dt_cmap,
	    dtop->dt_cmsize);
	if (dtop->dt_cmapdata == 0)
		return (ENXIO);
	dtop->dt_rmp = (struct mapent *)new_kmem_zalloc(
	    (u_int)(sizeof (struct mapent) * CRM_SIZE(dtop->dt_cmsize)),
	    KMEM_SLEEP);
	rminit((struct map *)dtop->dt_rmp,
	    (long)(CRM_NSLOTS(dtop->dt_cmsize)*CRM_SLOTSIZE),
	    (u_long)(0+CRM_ADDROFFSET), "colormap", CRM_SIZE(dtop->dt_cmsize));

	/* Determine foreground and background of color map */
	if (dtop->dt_screen.scr_flags & SCR_SWITCHBKGRDFRGRD) {
		tmpgnd = bkgnd;
		bkgnd = frgnd;
		frgnd = tmpgnd;
	}

	/* Determine available plane groups */
	(void) pr_available_plane_groups(dtop->dt_pixrect, DTOP_MAX_PLANE_GROUPS,
	    dtop->dt_plane_groups_available);
	/* Determine if multiple plane groups */
	plane_group_count = 0;
	for (i = 0; i < DTOP_MAX_PLANE_GROUPS; i++) {
		/* Restrict frame buffer plane group access */
		if (((dtop->dt_screen.scr_flags & SCR_8BITCOLORONLY) &&
		    (i != PIXPG_8BIT_COLOR) && (i != PIXPG_WID)) ||
		    ((dtop->dt_screen.scr_flags & SCR_OVERLAYONLY) &&
		    (i != PIXPG_OVERLAY)))
			dtop->dt_plane_groups_available[i] = 0;
		if (dtop->dt_plane_groups_available[i]) {
			int end;

			plane_group_count++;
			/* Don't set e & background for enable plane */
			if ((i != PIXPG_OVERLAY_ENABLE) &&
			    (i != PIXPG_VIDEO_ENABLE) &&
			    (i != PIXPG_CURSOR_ENABLE)) {
				/* Set fore & background in each plane group */
				pr_set_plane_group(dtop->dt_pixrect, i);
				pr_putcolormap(dtop->dt_pixrect, 0, 1,
				    &bkgnd->red, &bkgnd->green, &bkgnd->blue);
				switch (dtop->dt_pixrect->pr_depth) {
				case 1: end = 1; break;
				case 8: end = 255; max_depth = 8; break;
				case 24:
				case 32: end = 255; max_depth = 32; break;
#ifdef WINDEVDEBUG
				default: printf("Unknown depth %D\n",
				    dtop->dt_pixrect->pr_depth);
#endif
				}
				pr_putcolormap(dtop->dt_pixrect, end, 1,
				    &frgnd->red, &frgnd->green, &frgnd->blue);
			}
		}
	}
	if (dtop->dt_plane_groups_available[PIXPG_WID])
	    plane_group_count--;
	if (plane_group_count > 1)
		dtop->dt_flags |= DTF_MULTI_PLANE_GROUPS;
	/* Set initial plane group explicitly */
	if ((dtop->dt_screen.scr_flags & SCR_8BITCOLORONLY) &&
	    dtop->dt_plane_groups_available[PIXPG_8BIT_COLOR])
		pr_set_plane_group(dtop->dt_pixrect, PIXPG_8BIT_COLOR);
	else if ((dtop->dt_screen.scr_flags & SCR_OVERLAYONLY) &&
	    dtop->dt_plane_groups_available[PIXPG_OVERLAY])
		pr_set_plane_group(dtop->dt_pixrect, PIXPG_OVERLAY);
	else if (dtop->dt_plane_groups_available[PIXPG_OVERLAY])
		/*
		 * PIXPG_OVERLAY takes precedence over PIXPG_8BIT_COLOR
		 * for backwards compatibility reasons (see
		 * win_pick_plane_group).
		 */
		pr_set_plane_group(dtop->dt_pixrect, PIXPG_OVERLAY);
	else if (dtop->dt_plane_groups_available[PIXPG_8BIT_COLOR])
		pr_set_plane_group(dtop->dt_pixrect, PIXPG_8BIT_COLOR);

	/*
	 * Set up the cross hair pixrects, but do not allocate image space.
	 */

	dc = &dtop->dt_cursor;

	dc->horiz_hair_mpr.pr_depth = max_depth;
	dc->horiz_hair_mpr.pr_width = dtop->dt_screen.scr_rect.r_width;
	dc->horiz_hair_mpr.pr_height = CURSOR_MAX_HAIR_THICKNESS;

	dc->horiz_hair_size =
		-(dc->horiz_hair_mpr.pr_height *
		dtop_set_linebytes(&dc->horiz_hair_mpr));
	dc->horiz_hair_data.md_image = 0;
	horiz_hair_set_up(dtop, FALSE);

	dc->vert_hair_mpr.pr_depth = max_depth;
	dc->vert_hair_mpr.pr_width = CURSOR_MAX_HAIR_THICKNESS;
	dc->vert_hair_mpr.pr_height = dtop->dt_screen.scr_rect.r_height;

	dc->vert_hair_size =
		-(dc->vert_hair_mpr.pr_height *
		dtop_set_linebytes(&dc->vert_hair_mpr));
	dc->vert_hair_data.md_image = 0;
	vert_hair_set_up(dtop, FALSE);

	/* See if hardware cursor is part of this framebuffer */
	if (win_do_hw_cursor &&
	    (dtop_cursor_ioctl(dtop, FBIOGCURPOS, (caddr_t)&cp, 1) == 0))
		dtop->dt_flags |= DTF_HWCURSORAVAILABLE;

	/* No need to check for null return - we are not at interrupt level */
	lock_size = roundup(sizeof (Win_lock_block), PAGESIZE);

	/*
	 * Allocate the shared_info only once, because we are not going to
	 * free it ever.  This is because there are sleeps on this address
	 * that we can't make sure are all gone before releasing this storage.
	 */
	if (dtop->shared_info == 0)
		dtop->shared_info =
		  (Win_lock_block *)getpages(btopr(sizeof (Win_lock_block)), 0);

	bzero((caddr_t)dtop->shared_info, sizeof (Win_lock_block));
	dtop->display_waiting = 0;

	dtop->shared_info->version = WIN_LOCK_VERSION;
	/* NOTE: maybe RISC instead */
	dtop->shared_info->cpu_type = WIN_LOCK_68000;
	dtop->dt_sharedq = (struct inputevent *)
		((caddr_t) dtop->shared_info + sizeof (Win_lock_block));
	dtop->dt_sharedq_size =
	    (lock_size - sizeof (Win_lock_block)) / sizeof (struct inputevent);
	dtop->shared_info->shared_eventq.size = dtop->dt_sharedq_size;
	return (0);
}

dtop_close(dtop)
	register Desktop *dtop;
{
	register Desktop *dt;
	register int p;
	Desktop *dtopneighbor = NULL;
	Desktop **dt_prev_ptr;
	register Workstation *ws = dtop->dt_ws;
	Win_lock_block *shared_info_tmp;
	int waiting_tmp;

	/*
	 * Flush the notify timeout queue
	 */
	wt_sigwnch_untimeout();

	/*
	 * Remove references to this desktop from other desktops
	 */
	for (dt = desktops; dt < &desktops[ndesktops]; dt++)
		if (dt->dt_flags&DTF_PRESENT) {
			if (dt != dtop && dt->dt_ws == dtop->dt_ws)
				dtopneighbor = dt;
			for (p = 0; p < SCR_POSITIONS; p++)
				if (dt->dt_neighbors[p] == dtop)
					dt->dt_neighbors[p] = NULL;
		}
	/* Handle null workstation */
	if (ws == WORKSTATION_NULL)
		goto NoWs;
	/*
	 * Remove locator from this desktop
	 */
	dtop_change_loc_dtop(dtop, dtopneighbor, 0, 0);
	/* Remove desktop from workstation */
	dt_prev_ptr = &ws->ws_dtop;
	for (dt = ws->ws_dtop; dt; dt = dt->dt_next) {
		if (dt == dtop) {
			*dt_prev_ptr = dt->dt_next;
			break;
		}
		dt_prev_ptr = &dt->dt_next;
	}
	/* Close workstation if no more desktops using it */
	if (ws->ws_dtop == DESKTOP_NULL)
		ws_close(ws);

NoWs:
	/* Reset double buffering states */
	if (dtop->dt_flags & DTF_DBLBUFFER) {
		dtop_changedisplay(dtop, PR_DBL_A, PR_DBL_B, FALSE);
		if (pr_dbl_set(dtop->dt_pixrect, PR_DBL_WRITE, PR_DBL_BOTH, 0))
#ifdef WINDEVDEBUG
			printf("Error pr_dbl_set display  in dtop_close\n");
#else
			;
#endif
	}

	/*
	 * Close frame buffer.
	 */
	if (dtop->dt_fbfp) {
		/*
		 * DON'T clear screen because at this point have no way to
		 * arbitrate kernel access to the screen.  In the case of the
		 * sun1bw frame buffer could clobber some important register.
		 */
		/*
		 * Closing frame buffer will release dtop->dt_pixrect.
		 */
		closef(dtop->dt_fbfp);
	}
	/*
	 * Remove allocated resources
	 */
	rl_free(&dtop->dt_rlinvalid);
	cms_freecmapdata(dtop->dt_cmapdata, &dtop->dt_cmap,
	    dtop->dt_cmsize);
	if (dtop->dt_rmp)
		kmem_free((caddr_t)dtop->dt_rmp,
		    (u_int)(sizeof (struct mapent) * CRM_SIZE(dtop->dt_cmsize)));

	/* free the cross hair storage */
	if (dtop->dt_cursor.horiz_hair_data.md_image)
		kmem_free((caddr_t) dtop->dt_cursor.horiz_hair_data.md_image,
		    (u_int)dtop->dt_cursor.horiz_hair_size);
	if (dtop->dt_cursor.vert_hair_data.md_image)
		kmem_free((caddr_t) dtop->dt_cursor.vert_hair_data.md_image,
		    (u_int)dtop->dt_cursor.vert_hair_size);
	rl_free(&dtop->dt_cursor.horiz_hair_rectlist);
	rl_free(&dtop->dt_cursor.vert_hair_rectlist);

	/*
	 * Never free the shared lock block page(s), because can't be sure
	 * when wakeup will take affect.
	 */
	shared_info_tmp = dtop->shared_info;
	waiting_tmp = dtop->display_waiting;
	bzero((caddr_t)dtop, sizeof (*dtop));
	dtop->shared_info = shared_info_tmp;
	dtop->display_waiting = waiting_tmp;
	/* Wake up to give a chance to restart in win_ioctl */
	wakeup((caddr_t)shared_info_tmp);
}

void
dtop_change_loc_dtop(dtop, dtop_new, x, y)
	register Desktop *dtop;
	register Desktop *dtop_new;
	int x, y;
{
	register Workstation *ws = dtop->dt_ws;

	if (dtop == ws->ws_loc_dtop) {
		dtop_cursordown(dtop);
		win_shared_update_cursor_active(dtop->shared_info, FALSE);
		ws->ws_loc_dtop = dtop_new;
		if (dtop_new) {
			dtop_new->dt_rt_x = x;
			dtop_new->dt_rt_y = y;
			dtop_new->shared_info->status.new_cursor = TRUE;
			/* update the shared memory */
			win_shared_update_mouse_xy(dtop_new->shared_info, 0, 0);
			win_shared_update_cursor_active(
			    dtop_new->shared_info, TRUE);
			dtop_update_enable(dtop_new, dtop, 0);
		}
	}
	/* Remove pick focus from this desktop */
	if (dtop == ws->ws_pick_dtop) {
		ws->ws_pick_dtop = dtop_new;
		if (dtop_new) {
			dtop_new->dt_ut_x = x;
			dtop_new->dt_ut_y = y;
		}
	}
}

void
dtop_update_enable(dtop_new, dtop, doit)
	register Desktop *dtop_new;
	Desktop *dtop;
	int doit;
{
	char groups[DTOP_MAX_PLANE_GROUPS];
	register struct pixrect *pr;

	if (dtop_new == DESKTOP_NULL)
		return;
	/* Find a pixrect that is capable of setting the enable plane */
	pr = dtop_new->dt_pixrect;
	(void) pr_available_plane_groups(pr, DTOP_MAX_PLANE_GROUPS, groups);
	if (!groups[PIXPG_OVERLAY_ENABLE]) {
		if (dtop == DESKTOP_NULL)
			return;
		pr = dtop->dt_pixrect;
		(void) pr_available_plane_groups(pr, DTOP_MAX_PLANE_GROUPS,
		    groups);
		if (!groups[PIXPG_OVERLAY_ENABLE])
			return;
	}
	/* See if SCR_TOGGLEENABLE set anywhere */
	if ((dtop_new->dt_screen.scr_flags & SCR_TOGGLEENABLE) ||
	    (dtop && (dtop->dt_screen.scr_flags & SCR_TOGGLEENABLE)) || doit)
		dtop_set_enable(dtop_new, pr);
}

void
dtop_set_enable(dtop, pr)
	Desktop *dtop;
	register struct pixrect *pr;
{
	int original_plane_group;

	original_plane_group = pr_get_plane_group(pr);
	pr_set_planes(pr, PIXPG_OVERLAY_ENABLE, PIX_ALL_PLANES);
	if (dtop->dt_plane_groups_available[PIXPG_OVERLAY] ||
	    dtop->dt_plane_groups_available[PIXPG_MONO])
		pr_rop(pr, 0, 0, pr->pr_width, pr->pr_height,
		    PIX_SET, 0, 0, 0);
	if (dtop->dt_plane_groups_available[PIXPG_8BIT_COLOR])
		pr_rop(pr, 0, 0, pr->pr_width, pr->pr_height,
		    PIX_CLR, 0, 0, 0);
	pr_set_planes(pr, original_plane_group, PIX_ALL_PLANES);
}

int
dtop_removewin(dtop, w)
	Desktop *dtop;
	register struct window *w;
{
	int	error;
	bool	fixclipping;

	if (w->w_link[WL_PARENT] == NULL)
		return (EINVAL);
	fixclipping = wt_isindisplaytree(w);
	if ((error = wt_remove(w)) < 0)
		return (error);
	if (fixclipping)
		dtop_invalidateclipping(dtop, w->w_link[WL_PARENT], &w->w_rect);
	return (0);
}

dtop_interrupt(dtop, poll_rate)
	register Desktop *dtop;
	int poll_rate;
{
	register Win_lock_block	*shared_info = dtop->shared_info;

	/* Do lock timeout resolution */
	if (win_lock_data_locked(shared_info))
		wlok_lockcheck(&dtop->dt_datalock, poll_rate, 0);

	/* checked the shared memory locks */

	dtop_check_lock(dtop, &dtop->dt_mutexlock, &shared_info->mutex,
	    poll_rate, dtop_init_mutex_lock);

	dtop_check_lock(dtop, &dtop->dt_displaylock, &shared_info->display,
	    poll_rate, dtop_init_display_lock);

	/* See if moved or new cursor that should be put up now */
	if (shared_info->status.new_cursor) {
		dtop_cursordown(dtop);
		/*
		dtop->dt_flags &= ~DTF_NEWCURSOR;
		*/
		dtop_cursorup(dtop);
	}

	/* See if overflow from shared q may be shifted */
	if (dtop->dt_sharedq_owner)
		win_sharedq_shift(dtop->dt_sharedq_owner);

	/*
	 * see if a process is waiting for an already released lock.
	 * We need to do this here because of the race condition that
	 * exists between when a user process checks the waiting count
	 * and when the kernel sets it before sleeping.
	 */
	if ((dtop->display_waiting) && !win_lock_display_locked(shared_info)) {
#ifdef WINDEVDEBUG
		if (win_waiting_debug)
			printf("Waking up waiting process because display lock is free\n");
#endif
		wakeup((caddr_t)shared_info);
	}

	/* see if shared locking info needs updating */
	win_shared_update(dtop);
}


dtop_validate_shared_lock(dtop, wlock)
	register Desktop 	*dtop;
	Winlock			*wlock;
{
	register Win_lock_block	*shared_info = dtop->shared_info;

	if (wlock == &dtop->dt_displaylock)
	    dtop_check_lock(dtop, wlock, &shared_info->display,
	    0, dtop_init_display_lock);
	else
	    dtop_check_lock(dtop, wlock, &shared_info->mutex,
	    0, dtop_init_mutex_lock);

	/* see if shared locking info needs updating */
	win_shared_update(dtop);
}


static int
dtop_check_lock(dtop, wlock, shared_lock, poll_rate, init_lock)
	Desktop		*dtop;
	Winlock		*wlock;
	Win_lock	*shared_lock;
	int		poll_rate;
	void		(*init_lock)();

{
	int	orig_count;

	if (!win_lock_locked(shared_lock)) {
	    if (wlock->lok_pid) {
		/*
		 * Here the user has unlocked the mutex lock.
		 * So we clear the kernel lock.
		 * First set the lock bit so we can unlock it with
		 * wlok_unlock().
		 */
		win_lock_set(shared_lock, TRUE);
		shared_lock->count = 1;
		wlok_unlock(wlock);
	    }
	    return;
	}

	/* the shared lock is locked */

	if (wlock->lok_id && (wlock->lok_id != shared_lock->id)) {
	    /*
	     * Here a user process has reacquired the lock.
	     * So we forget about the old lock.
	     * Don't do the usual unlock action, since the lock
	     * is still locked.
	     * Also, be sure to preserve the lock count.
	     */
	    orig_count = shared_lock->count;
	    shared_lock->count = 1;
	    wlock->lok_unlock_action = 0;
	    wlok_unlock(wlock);
	    /* reset the lock, since wlok_unlock() zero'ed it */
	    win_lock_set(shared_lock, TRUE);
	    shared_lock->count = orig_count;
	}

	if (wlock->lok_pid) {
	    /*
	     * Here the lock is set and the timer is
	     * running, so we check the lock.
	     */
	    if (poll_rate)
		wlok_lockcheck(wlock, poll_rate, 0);
	    return;
	}
	/*
	 * Here the lock was set by a user process,
	 * so we need to initialize our internal lock.
	 */
	if (shared_lock->pid) {
	    extern struct proc 	*pfind();
	    struct proc		*process = pfind(shared_lock->pid);

	    orig_count = shared_lock->count;
	    wlok_clear(wlock);
	    (init_lock)(dtop, shared_lock->id, process);
	    shared_lock->count = orig_count;
	    if (!process) {
		/*
		 * Here a user process has set the lock,
		 * and either the process has died, or the pid was
		 * not set correctly.  So we clear the lock.
		 */
		if (!(wlock->lok_options & WLOK_SILENT))
		    printf("Window %s lock broken after process %D went away\n",
				wlock->lok_string, shared_lock->pid);
		wlok_forceunlock(wlock);
	    }
	} else {
	    /*
	     * Here the lock bit is set, but the process has
	     * not set the lock.pid yet.
	     * So complain after a while.
	     */
	    if (wlock->lok_time == 0) {
		/* initialize the lock */
		orig_count = shared_lock->count;
		wlok_clear(wlock);
		(init_lock)(dtop, shared_lock->id, 0);
		shared_lock->count = orig_count;
	    }
	    if (poll_rate)
		wlok_lockcheck(wlock, poll_rate, 0);
	}
}


/*
 * Constrains to be on desktop and moves to another if adjacent.
 */
dtop_track_locator(dtop_ptr, x_ptr, y_ptr)
	Desktop	**dtop_ptr;
	int	*x_ptr;
	int	*y_ptr;
{
	register Desktop *dtop = *dtop_ptr;
	register Desktop *dtop_old = *dtop_ptr;
	register int x = *x_ptr;
	register int y = *y_ptr;

	if (x < dtop->dt_screen.scr_rect.r_left)
		if (dtop->dt_neighbors[SCR_WEST]) {
			dtop = dtop->dt_neighbors[SCR_WEST];
			x = dtop->dt_screen.scr_rect.r_left+
			    dtop->dt_screen.scr_rect.r_width-
			    (dtop_old->dt_screen.scr_rect.r_left-x);
			y = (dtop->dt_screen.scr_rect.r_height*
			    y)/dtop_old->dt_screen.scr_rect.r_height;
		} else
			x = dtop->dt_screen.scr_rect.r_left;
	else if (x > dtop->dt_screen.scr_rect.r_width-1)
		if (dtop->dt_neighbors[SCR_EAST]) {
			dtop = dtop->dt_neighbors[SCR_EAST];
			x = dtop->dt_screen.scr_rect.r_left+
			    (x-dtop_old->dt_screen.scr_rect.r_width);
			y = (dtop->dt_screen.scr_rect.r_height*
			    y)/dtop_old->dt_screen.scr_rect.r_height;
		} else
			x = rect_right(&dtop->dt_screen.scr_rect)-1;
	if (y < dtop->dt_screen.scr_rect.r_top)
		if (dtop->dt_neighbors[SCR_NORTH]) {
			dtop = dtop->dt_neighbors[SCR_NORTH];
			y = dtop->dt_screen.scr_rect.r_top+
			    dtop->dt_screen.scr_rect.r_height-
			    (dtop_old->dt_screen.scr_rect.r_top-y);
			x = (dtop->dt_screen.scr_rect.r_width*
			    x)/dtop_old->dt_screen.scr_rect.r_width;
		} else
			y = dtop->dt_screen.scr_rect.r_top;
	else if (y > dtop->dt_screen.scr_rect.r_height-1)
		if (dtop->dt_neighbors[SCR_SOUTH]) {
			dtop = dtop->dt_neighbors[SCR_SOUTH];
			y = dtop->dt_screen.scr_rect.r_top+
			    (y-dtop_old->dt_screen.scr_rect.r_height);
			x = (dtop->dt_screen.scr_rect.r_width*
			    x)/dtop_old->dt_screen.scr_rect.r_width;
		} else
			y = rect_bottom(&dtop->dt_screen.scr_rect)-1;
	*dtop_ptr = dtop;
	*x_ptr = x;
	*y_ptr = y;
}

dtop_set_cursor(dtop)
	register Desktop *dtop;
{
	Workstation *ws = dtop->dt_ws;
	register struct	window *oldcursorwin = dtop->dt_cursorwin;
	register struct	window *newcursorwin;
	int new_cmap = dtop->dt_flags & DTF_NEWCMAP;

	dtop->dt_cursorwin = NULL;
	if ((ws == WORKSTATION_NULL) || (dtop->dt_rootwin == NULL) ||
	    (ws->ws_loc_dtop != dtop))
		return;
	if (ws->ws_inputgrabber) {
		/* The cursor reflects the input grabber */
		dtop->dt_cursorwin = ws->ws_inputgrabber;
	} else {
		/* The cursor reflects underlaying window */
		dtop->dt_cursorwin = wt_intersected(dtop->dt_rootwin,
		    dtop->dt_rt_x, dtop->dt_rt_y);
	}
	newcursorwin = dtop->dt_cursorwin;
	/* Might need new cursor up because of change of underlaying window */
	/*
	dtop->dt_flags |= DTF_NEWCURSOR;
	*/
	dtop->shared_info->status.new_cursor = TRUE;

	/* Change colormap if need to */
	if ((new_cmap ||
	    (oldcursorwin != newcursorwin))) {
		if (newcursorwin && (newcursorwin->w_flags & WF_CMSHOG)) {
			dtop_putcolormap(dtop, newcursorwin->w_cms.cms_size,
			    &newcursorwin->w_cmap);
		} else if (new_cmap ||
		    (oldcursorwin && (oldcursorwin->w_flags & WF_CMSHOG))) {
			dtop_putcolormap(dtop, dtop->dt_cmsize, &dtop->dt_cmap);
		}
		dtop->dt_flags &= ~DTF_NEWCMAP;
	}
	/*
	 * Update the shared cursor if the cursor window or mouse window
	 * has changed.  For hardware cursor, need to do this if the color
	 * map has changed, too.
	 */
	if ((oldcursorwin != newcursorwin) ||
	    (new_cmap && dtop->shared_info->cursor_info.hardware_cursor))
		win_shared_update_cursor(dtop);
	/* Try to update hardware cursor position with real time coordinates */
	dtop_hw_cursor_pos_update(dtop);
}

int
dtop_lockdata(dev)
	dev_t	dev;
{
	Window *w = winfromdev(dev);
	register Desktop *dtop = w->w_desktop;
	register Win_lock_block	*shared_info = dtop->shared_info;

	if (!win_lock_data_locked(dtop->shared_info)) {
		/*
		 * mark the shared memory to show that
		 * someone is wating for a lock.
		 */
		shared_info->waiting++;
		dtop->display_waiting++;

		while (
		    win_lock_mutex_locked(shared_info) ||
		    win_lock_data_locked(shared_info) ||
		    (win_lock_display_locked(shared_info) &&
		    (shared_info->display.pid!=u.u_procp->p_pid)))
			if (sleep((caddr_t)dtop->shared_info, LOCKPRI|PCATCH)) {
				shared_info->waiting--;
				dtop->display_waiting--;
				return (-1);
			}

		/* unmark the shared memory lock */
		shared_info->waiting--;
		dtop->display_waiting--;

		dtop_init_data_lock(dtop, u.u_procp);
		shared_info->data.pid = dtop->dt_datalock.lok_pid;
	} else {
		/*
		 * Wouldn't get into this routine unless
		 * calling pid == dt_datalock.lok_pid
		 */
		*(dtop->dt_datalock.lok_count) += 1;
	}
	return (0);
}

void
dtop_timedout_data(wlock)
	Winlock *wlock;
{
	extern	struct proc *pfind();
	register struct proc *process = pfind(wlock->lok_pid);

	/* Send SIGXCPU if process still exists */
	if (process != (struct proc *)0) {
		psignal(process, SIGXCPU);
		printf("The offending process was sent SIGXCPU\n");
	}
}

void
dtop_unlockdata(wlock)
	Winlock *wlock;
{
	Desktop	*dtop = (Desktop *)wlock->lok_client;

	dtop_computeclipping(dtop, TRUE);
	dtop->shared_info->data.pid = 0;
}

dtop_computeclipping(dtop, notify)
	register Desktop *dtop;
	bool	notify;
{
	extern wt_add_to_notifyqueue();
	extern winnotify_inorder;

	if (dtop->dt_parentinvalidwin == NULL)
		return;
	wt_setclipping(dtop->dt_parentinvalidwin, &dtop->dt_rlinvalid);
	ws_set_focus(dtop->dt_ws, FF_PICK_CHANGE);
	dtop_set_cursor(dtop);
	rl_free(&dtop->dt_rlinvalid);
	if (notify) {
	    if (winnotify_inorder) {
		(void) wt_enumerate(wt_add_to_notifyqueue,  WT_TOPTOBOTTOM,
		    dtop->dt_parentinvalidwin);
	    } else {
		(void) wt_enumeratechildren(wt_notifychanged,
		    dtop->dt_parentinvalidwin, (struct rect *)0);
	    }
	    dtop->dt_parentinvalidwin = NULL;
	}
}

dtop_invalidateclipping(dtop, parent, rect)
	register Desktop *dtop;
	register struct window *parent;
	struct	rect *rect;
	/*
	 * The parent relative rect has invalid clipping and
	 * parent is the bottom most window affected.
	 */
{
	struct	rectlist rlparent;
	struct	window *root = parent->w_desktop->dt_rootwin;

	rl_initwithrect(rect, &rlparent);
	/*
	 * Make invalid stuff relative to root.
	 * Need to do this because eventual call to wt_setclipping must be
	 * called with root as the initial window.
	 */
	while (parent != root) {
		wt_passrltoancestor(&rlparent, parent,
		    parent->w_link[WL_PARENT]);
		parent = parent->w_link[WL_PARENT];
	}
	dtop->dt_parentinvalidwin = root;
	rl_union(&rlparent, &dtop->dt_rlinvalid, &dtop->dt_rlinvalid);
	rl_free(&rlparent);
	if (!win_lock_data_locked(dtop->shared_info))
		dtop_computeclipping(dtop, TRUE);
}


/*
 * Display access control
 */
int
dtop_lockdisplay(dev, rect)
    dev_t	dev;
    struct	rect *rect;
{
    register struct window 	*w = winfromdev(dev);
    register Desktop		*dtop = w->w_desktop;
    register Win_lock_block	*shared_info = dtop->shared_info;
    struct rect 		rectcursor, screen_rect;
    short			take_down_cursor = FALSE;
    Win_shared_cursor		*shared_cur;
    /*
     * Currently, locking a dev you have locked increments a ref count.
     */
    if (!win_lock_display_locked(shared_info)) {
	/*
	 * mark the shared memory to show that
	 * someone is wating for a lock.
	 */
	shared_info->waiting++;
	dtop->display_waiting++;

	/* loop until the lock is free */
	while (win_lock_mutex_locked(shared_info) ||
		win_lock_display_locked(shared_info) ||
		((dtop->dt_displaygrabber != NULL) &&
		(dtop->dt_displaygrabber->w_pid != w->w_pid)) ||
		(win_lock_data_locked(shared_info) &&
		(dtop->dt_datalock.lok_pid != u.u_procp->p_pid))) {
		if (sleep((caddr_t)dtop->shared_info, LOCKPRI|PCATCH)) {
			shared_info->waiting--;
			dtop->display_waiting--;
			return (-1);
		}
	}

	/* unmark the shared memory lock */
	shared_info->waiting--;
	dtop->display_waiting--;
	shared_cur = &dtop->shared_info->cursor_info;
	if (!(shared_cur->spare3 )) {
	    if (cursor_up(dtop)) {
		int cursor_in;	/* Whether the cursor is in w ? */

		 rect_construct(&rectcursor,
			    cursor_x(dtop) - w->w_screenx,
			    cursor_y(dtop) - w->w_screeny,
			    cursor_screen_width(dtop),
			    cursor_screen_height(dtop));

		cursor_in = rl_rectintersects(&rectcursor, &w->w_rlexposed);
		/*
		* Don't take down the cursor if the cursor is in the
		* currently active double buffering window.  And we are
		* just accessing the background (the cursor is only in the
		* foreground).
		*/
		if ((dtop->dt_curdbl == w) && 
			(w->w_dbl_wrstate == PW_DBL_BACK) &&
			(w->w_dbl_rdstate == PW_DBL_BACK) && cursor_in)
			take_down_cursor = FALSE;
		else {
		/*
		* Can't restrict cursor removal if
		* displaygrabbed because user may violate
		* w_rlexposed if wants, i.e., to do menus.
		*/
		    take_down_cursor =
			rect_intersectsrect(rect, &rectcursor) &&
			(dtop->dt_displaygrabber == w || cursor_in);
		}
	    }
	}

	/* convert locked area to screen coordinates */
	screen_rect = *rect;
	rect_passtoparent(w->w_screenx, w->w_screeny, &screen_rect);

	/* check for intersection with cross hairs */
	if (horiz_hair_up(dtop)) {
	    take_down_cursor = take_down_cursor ||
		rl_rectintersects(&screen_rect,
				&dtop->dt_cursor.horiz_hair_rectlist);
	}

	if (vert_hair_up(dtop)) {
	    take_down_cursor = take_down_cursor ||
		rl_rectintersects(&screen_rect,
				&dtop->dt_cursor.vert_hair_rectlist);
	}

	if (take_down_cursor) {
	    dtop_cursordown(dtop);
	}

	shared_info->display.pid = u.u_procp->p_pid;
	shared_info->display.id++;
	dtop_init_display_lock(dtop, shared_info->display.id, u.u_procp);

	/*
	 * Double buffer support
	 * Flip the write control bits  if current window is the
	 *  double bufferer else set write to both
	 */
	if (dtop->dt_dblcount) {
		if (dtop->dt_curdbl == w) {
			(void)dtop_dblset(dtop, PR_DBL_WRITE, w->w_dbl_wrstate);
			(void)dtop_dblset(dtop, PR_DBL_READ, w->w_dbl_rdstate);
		} else if (!(dtop->dt_displaygrabber == w)) {
			if (pr_dbl_set(dtop->dt_pixrect, PR_DBL_WRITE,
			    PR_DBL_BOTH, PR_DBL_READ, dtop->dt_dbl_bkgnd, 0))
#ifdef WINDEVDEBUG
				printf("Error dbl_set lock display not",
				" a cur dbl %d\n", dtop->dt_dbl_frgnd);
#else
				;
#endif
		}
	}
    } else if (dtop->dt_displaylock.lok_count)
	*(dtop->dt_displaylock.lok_count) += 1;
#ifdef WINDEVDEBUG
    else
	printf("Warning: display lock count pointer is NULL\n");
#endif
    /*
     * Note: Should check to see if going to write outside of
     * original rect affected.
     */
    return (w->w_clippingid);
}

/* ARGSUSED */
void
dtop_timedout_display(wlock)
	Winlock *wlock;
{
#ifdef WINDEVDEBUG
	Desktop		*dtop = (Desktop *) wlock->lok_client;
	Win_lock	*shared_lock = &dtop->shared_info->display;
#endif

	printf("You may see display garbage because of this action\n");

#ifdef WINDEVDEBUG
	/* debug info */
	if ((wlock->lok_pid != shared_lock->pid) ||
	    (wlock->lok_id != shared_lock->id)) {
		printf("Internal lock: pid = %d, id = %d, time = %d\n",
			wlock->lok_pid, wlock->lok_id, wlock->lok_time);
		printf("Shared lock: pid = %d, id = %d\n",
			shared_lock->pid, shared_lock->id);
	}
#endif
}

static void
dtop_update_cursor(dtop)
	register Desktop *dtop;
{

	if (cursor_up(dtop) || horiz_hair_up(dtop) || vert_hair_up(dtop)) {
		if (dtop != dtop->dt_ws->ws_loc_dtop)
			/*
			 * take down current cursor ms not on dtop anymore.
			 */
			dtop_cursordown(dtop);
		if (dtop->dt_cursorwin &&
		    ((cursor_x(dtop) != dtop->dt_rt_x -
			dtop->dt_cursorwin->w_cursor.cur_xhot) ||
		    (cursor_y(dtop) != dtop->dt_rt_y -
			dtop->dt_cursorwin->w_cursor.cur_yhot))) {
			/*
			 * take down current cursor cause in wrong position
			 */
			dtop_cursordown(dtop);
		}
	}
}

void
dtop_unlockdisplay(wlock)
	Winlock *wlock;
{
	register Desktop *dtop = (Desktop *)wlock->lok_client;

	dtop_update_cursor(dtop);
	dtop->shared_info->display.pid = 0;
	/*
	 * put up current cursor now
	 */
	dtop_cursorup(dtop);
}

void
dtop_unlockmutex(wlock)
	Winlock *wlock;
{
	register Desktop *dtop = (Desktop *)wlock->lok_client;

	dtop->shared_info->mutex.pid = 0;
}

dtop_drawcursor(dtop)
    register Desktop *dtop;
{
#define	ROPERR(tag) { rop_err_id = (tag); goto Roperr; }
    register struct dtopcursor 	*dc;
    register struct cursor 	*cursor;
    register struct mpr_data 	*scrmpr_data;
    struct mpr_data 		*data;
    struct pixrect		*shape;
    int				planes;	/* Can't be register */
    struct rect 		hair_rect;
    short			want_cursor;
    Workstation			*ws;
    Win_shared_cursor		*shared;
    struct window		*cursorwin;
    struct pixrect		*pr;
    register int		full_planes = PIX_ALL_PLANES;
    register int		original_plane_group;
    register int		rop_err_id = 0;
    int				readstate;
    int				writestate;
    int				cursor_offset_x, cursor_offset_y;
    char			pgroups[PIXPG_INVALID+1];


    if (dtop == NULL || dtop->dt_pixrect == NULL || wincursordisable)
	return;

    /* can't do anything if the shared structure is locked down */
    if (win_lock_mutex_locked(dtop->shared_info))
	return;
    (void) pr_available_plane_groups(dtop->dt_pixrect, PIXPG_INVALID,
					pgroups);

    /* For FBs with dedicated cursor planes */
    if ((pgroups[PIXPG_CURSOR]) && (pgroups[PIXPG_CURSOR_ENABLE])) {

	shared = &dtop->shared_info->cursor_info;
	 shared->spare3 = TRUE;
	dc = &dtop->dt_cursor;
	pr = dtop->dt_pixrect;
	original_plane_group = pr_get_plane_group(pr);
	if (cursor_up(dtop) || 	horiz_hair_up(dtop) || vert_hair_up(dtop)) {
	    /* Set up plane group to be cursor image plane */
	    if (cursor_up(dtop)) {
		pr_set_planes(pr, PIXPG_CURSOR, full_planes);
		/* take down existing cursor by clearing the bits in *
		* image  planes */
		win_lock_prepare_screen_pr(shared);
		cursor_set_up(dtop, FALSE);
		if (pr_rop(pr, shared->x, shared->y,
		    shared->screen_pr.pr_width, shared->screen_pr.pr_height,
			PIX_CLR, NULL, 0, 0))
		    ROPERR(1)
		/* Now do the enable plane */
		pr_set_planes(pr, PIXPG_CURSOR_ENABLE,full_planes);
		if (pr_rop(pr, shared->x-1, shared->y-1,
		    shared->screen_pr.pr_width+2, 
		    shared->screen_pr.pr_height+2,
		    PIX_CLR, NULL, 0, 0))
		    ROPERR(2)
	    }
	    /* Now take the cross hairs down*/
	    if (horiz_hair_up(dtop)) {
		horiz_hair_set_up(dtop, FALSE);
	    	pr_set_planes(pr, PIXPG_CURSOR, full_planes);
		if (dtop_rl_rop(pr, 0, 0,
				&dc->horiz_hair_rectlist,
				PIX_CLR, (Pixrect *) 0,
				0, 0))
		    ROPERR(3)
		pr_set_planes(pr, PIXPG_CURSOR_ENABLE, full_planes);
		if (dtop_rl_rop_enable(pr, 0, 0,
				&dc->horiz_hair_rectlist,
				PIX_CLR, (Pixrect *)0,
				0, 0))
		    ROPERR(4)
		rl_free(&dc->horiz_hair_rectlist);
	    }
	    if (vert_hair_up(dtop)) {
		vert_hair_set_up(dtop, FALSE);
		pr_set_planes(pr, PIXPG_CURSOR, full_planes);
		if (dtop_rl_rop(pr, 0, 0,
				&dc->vert_hair_rectlist,
				PIX_CLR, (Pixrect *)0,
				0, 0))
		    ROPERR(5)
		pr_set_planes(pr, PIXPG_CURSOR_ENABLE, full_planes);
		if (dtop_rl_rop_enable(pr, 0, 0,
				&dc->vert_hair_rectlist,
				PIX_CLR, (Pixrect *)0,
				0, 0))
		    ROPERR(6)
		rl_free(&dc->vert_hair_rectlist);
	    }
	    goto Cursor_Done;
	}
	if (dtop->dt_cursorwin && !(dtop->dt_cursorwin->w_flags & WF_OPEN)) {
		dtop->dt_cursorwin = WINDOW_NULL;
		goto Cursor_Done;
	}
	/* Putup  the new cursor */
	cursorwin = dtop->dt_cursorwin;
	if (!cursorwin || !(ws = dtop->dt_ws) || (ws->ws_loc_dtop != dtop))
	    goto Cursor_Done;

	/* we are dealing with the latest cursor */
	dtop->shared_info->status.new_cursor = FALSE;
	cursor = &cursorwin->w_cursor;
	data = &cursorwin->w_cursordata;
	shape = cursor->cur_shape;
	want_cursor = shape->pr_width && shape->pr_height &&
			shape->pr_depth && data->md_image &&
			show_cursor(cursor);

	if (!(want_cursor || show_horiz_hair(cursor) 
					|| show_vert_hair(cursor)))
	    goto Cursor_Done;

	/*
	* Figure out where on screen to draw cursor.
	*/
	shared->x = dtop->dt_rt_x - cursor->cur_xhot;
	shared->y = dtop->dt_rt_y - cursor->cur_yhot;
	shared->screen_pr.pr_height = shape->pr_height;
	shared->screen_pr.pr_width = shape->pr_width;
	shared->spare3 = TRUE;		

	 win_lock_prepare_screen_pr(shared);
	/* The rectlist of the cross hairs has to be
	 * generated. So we will do the same as before 
	 * to save the image below cross hairs.

        if (show_horiz_hair(cursor) || show_vert_hair(cursor)) {
	/* prepare cross hair storage */
	/* get the horizontal & vertical info from the input window */
    	
	pr_set_planes(pr, PIXPG_CURSOR,full_planes);
	dc->horiz_hair_mpr.pr_depth = 1;
	dc->horiz_hair_mpr.pr_height = cursor->horiz_hair_thickness;
	(void) dtop_set_linebytes(&dc->horiz_hair_mpr);

	dc->vert_hair_mpr.pr_depth = 1;
	dc->vert_hair_mpr.pr_width = cursor->vert_hair_thickness;
	(void) dtop_set_linebytes(&dc->vert_hair_mpr);

	dc->hair_x = dtop->dt_rt_x - (dc->vert_hair_mpr.pr_width / 2);
	dc->hair_y = dtop->dt_rt_y - (dc->horiz_hair_mpr.pr_height / 2);

	if (show_horiz_hair(cursor)) {
	    /* construct the horizontal hair window/fullscreen rect */
	    rect_construct(&hair_rect, 0,
				fullscreen(cursor) ? dc->hair_y :
				dc->hair_y - cursorwin->w_screeny,
				dc->horiz_hair_mpr.pr_width,
				dc->horiz_hair_mpr.pr_height);

	    /*
	     * determine the intersection with the exposed area of
	     *  the window.
	     */
	    dc->horiz_hair_rectlist = rl_null;
	    if (fullscreen(cursor))
		rl_initwithrect(&hair_rect, &dc->horiz_hair_rectlist);
	    else {
		rl_rectintersection(&hair_rect, &cursorwin->w_rlexposed,
				    &dc->horiz_hair_rectlist);
		/* convert to screen coordinates */
		dc->horiz_hair_rectlist.rl_x = cursorwin->w_screenx;
		dc->horiz_hair_rectlist.rl_y = cursorwin->w_screeny;
		rl_normalize(&dc->horiz_hair_rectlist);
		/* convert hair_rect top to screen coordinates */
		hair_rect.r_top = dc->hair_y;
	    }

	    /* if length is not full, cut out excess */
	    if (cursor->horiz_hair_length != CURSOR_TO_EDGE) {
		int	max_length;
		int	x_left;

		if (horiz_border_gravity(cursor)) {
		    max_length = fullscreen(cursor)?
			dc->horiz_hair_mpr.pr_width : cursorwin->w_rect.r_width;
		    x_left = fullscreen(cursor) ? cursor->horiz_hair_length :
			cursorwin->w_screenx + cursor->horiz_hair_length;

		    hair_rect.r_left = x_left;
		    hair_rect.r_width = max_length-2*cursor->horiz_hair_length;
		    rl_rectdifference(&hair_rect, &dc->horiz_hair_rectlist,
					&dc->horiz_hair_rectlist);
		} else {
		    /* length is from center */
		    x_left = fullscreen(cursor) ? 0 : cursorwin->w_screenx;

		    hair_rect.r_left = x_left;
		    hair_rect.r_width =
			dc->hair_x - cursor->horiz_hair_length - x_left;
		    rl_rectdifference(&hair_rect, &dc->horiz_hair_rectlist,
					&dc->horiz_hair_rectlist);

		    hair_rect.r_left = dc->hair_x + cursor->horiz_hair_length;
		    hair_rect.r_width = dc->horiz_hair_mpr.pr_width;
		    rl_rectdifference(&hair_rect, &dc->horiz_hair_rectlist,
					&dc->horiz_hair_rectlist);
		}
	    }

	    if (cursor->horiz_hair_gap) {
		/* cut out a gap the width of the cursor or as specified */
		if (cursor->horiz_hair_gap == CURSOR_TO_EDGE) {
		    hair_rect.r_width = shape->pr_width;
		    hair_rect.r_left = shared->x;
		} else {
		    hair_rect.r_width = cursor->horiz_hair_gap;
		    hair_rect.r_left = dc->hair_x - hair_rect.r_width / 2;
		}
		rl_rectdifference(&hair_rect, &dc->horiz_hair_rectlist,
					&dc->horiz_hair_rectlist);
	    }
	}
	if (show_vert_hair(cursor)) {
	   /* construct the vertical hair window/fullscreen rect */
	    rect_construct(&hair_rect,
		fullscreen(cursor) ? dc->hair_x :
			dc->hair_x - cursorwin->w_screenx,
			0,
			dc->vert_hair_mpr.pr_width,
			dc->vert_hair_mpr.pr_height);

	    /*
	     * determine the intersection with the exposed area of
	     *  the window.
	     */
	    dc->vert_hair_rectlist = rl_null;
	    if (fullscreen(cursor))
		rl_initwithrect(&hair_rect, &dc->vert_hair_rectlist);
	    else {
		rl_rectintersection(&hair_rect, &cursorwin->w_rlexposed,
				    &dc->vert_hair_rectlist);
		/* convert to screen coordinates */
		dc->vert_hair_rectlist.rl_x = cursorwin->w_screenx;
		dc->vert_hair_rectlist.rl_y = cursorwin->w_screeny;
		rl_normalize(&dc->vert_hair_rectlist);
		/* convert hair_rect left to screen coordinates */
		hair_rect.r_left = dc->hair_x;
	    }

	    /* if length is not full, cut out excess */
	    if (cursor->vert_hair_length != CURSOR_TO_EDGE) {
		int	max_length;
		int	y_top;

		if (vert_border_gravity(cursor)) {
		    max_length = fullscreen(cursor)?
			dc->vert_hair_mpr.pr_height:cursorwin->w_rect.r_height;
		    y_top = fullscreen(cursor) ? cursor->vert_hair_length :
			cursorwin->w_screeny + cursor->vert_hair_length;

		    hair_rect.r_top = y_top;
		    hair_rect.r_height = max_length-2*cursor->vert_hair_length;
		    rl_rectdifference(&hair_rect, &dc->vert_hair_rectlist,
					&dc->vert_hair_rectlist);
		} else {
		    /* length is from center */
		    y_top = fullscreen(cursor) ? 0 : cursorwin->w_screeny;

		    hair_rect.r_top = y_top;
		    hair_rect.r_height =
			dc->hair_y - cursor->vert_hair_length - y_top;
		    rl_rectdifference(&hair_rect, &dc->vert_hair_rectlist,
					&dc->vert_hair_rectlist);

		    hair_rect.r_top = dc->hair_y + cursor->vert_hair_length;
		    hair_rect.r_height = dc->vert_hair_mpr.pr_height;
		    rl_rectdifference(&hair_rect, &dc->vert_hair_rectlist,
					&dc->vert_hair_rectlist);
		}
	    }

	    if (cursor->vert_hair_gap) {
		/* cut out a gap the height of the cursor or as specified */
		if (cursor->vert_hair_gap == CURSOR_TO_EDGE) {
		    hair_rect.r_height = shape->pr_height;
		    hair_rect.r_top = shared->y;
		} else {
		    hair_rect.r_height = cursor->vert_hair_gap;
		    hair_rect.r_top = dc->hair_y - hair_rect.r_height / 2;
		}
		rl_rectdifference(&hair_rect, &dc->vert_hair_rectlist,
				&dc->vert_hair_rectlist);
	    }
	}
	/*
	 * Need to add something here to change colormap of 
         * of cursor plane so that cursor has correct colors
         */
	/* Write to display */

	if (want_cursor) {
	    /* write the cursor */
	    if (show_cursor(cursor)) {
		cursor_set_up(dtop, TRUE);

		pr_set_planes(pr, PIXPG_CURSOR, full_planes);
		if (pr_rop(pr, shared->x, shared->y,
		    shared->screen_pr.pr_width, shared->screen_pr.pr_height,
		    PIX_SRC, shape, 0, 0))
		ROPERR(10)
	    	pr_set_planes(pr, PIXPG_CURSOR_ENABLE, full_planes);
		if (pr_rop(pr, shared->x, shared->y, 
		    shared->screen_pr.pr_width, shared->screen_pr.pr_height,
		    PIX_SRC, (Pixrect *)shape, 0, 0))
		    ROPERR(11)

		/* smear */
		if (pr_rop(pr, shared->x-1, shared->y-1, 
		    shared->screen_pr.pr_width, 
		    shared->screen_pr.pr_height,
		    PIX_SRC|PIX_DST, shape, 0, 0))
		    ROPERR(11)	/*U-L*/
		if (pr_rop(pr, shared->x, shared->y-1, 
		    shared->screen_pr.pr_width, 
		    shared->screen_pr.pr_height,
		    PIX_SRC|PIX_DST, shape, 0, 0))
		    ROPERR(11) /*U */
		if (pr_rop(pr, shared->x+1, shared->y-1, 
		    shared->screen_pr.pr_width, 
		    shared->screen_pr.pr_height,
		    PIX_SRC|PIX_DST, shape, 0, 0))
		    ROPERR(11)/*U-R */
		if (pr_rop(pr, shared->x+1, shared->y, 
		    shared->screen_pr.pr_width, 
		    shared->screen_pr.pr_height,
		    PIX_SRC|PIX_DST, shape, 0, 0))
		    ROPERR(11) /* R */
		if (pr_rop(pr, shared->x+1, shared->y+1, 
		    shared->screen_pr.pr_width, 
		    shared->screen_pr.pr_height,
		    PIX_SRC|PIX_DST, shape, 0, 0))
		    ROPERR(11) /*L - R */
		if (pr_rop(pr, shared->x, shared->y+1, 
		    shared->screen_pr.pr_width, 
		    shared->screen_pr.pr_height,
		    PIX_SRC|PIX_DST, shape, 0, 0))
		    ROPERR(11) /*L */
		if (pr_rop(pr, shared->x-1, shared->y+1, 
		    shared->screen_pr.pr_width, 
		    shared->screen_pr.pr_height,
		    PIX_SRC|PIX_DST, shape, 0, 0))
		    ROPERR(11) /*L-L */
		if (pr_rop(pr, shared->x-1, shared->y, 
		    shared->screen_pr.pr_width,
		     shared->screen_pr.pr_height,
		    PIX_SRC|PIX_DST, shape, 0, 0))
		    ROPERR(11) /* L */
	    }	 
	}
	if (show_horiz_hair(cursor) && !rl_empty(&dc->horiz_hair_rectlist)) {
	    horiz_hair_set_up(dtop, TRUE);
	    pr_set_planes(pr, PIXPG_CURSOR, full_planes);
	    if (dtop_rl_rop(pr, 0, 0, &dc->horiz_hair_rectlist,
	            PIX_SET , (struct pixrect *)0, 0, 0))
		ROPERR(9)
	    pr_set_planes(pr, PIXPG_CURSOR_ENABLE, full_planes);
	    if (dtop_rl_rop_enable(pr, 0, 0, &dc->horiz_hair_rectlist,
		    PIX_SET, (struct pixrect *)0, 0, 0))
	    ROPERR(10)
	}
	/* write the vertical cross hair */
	if (show_vert_hair(cursor) && !rl_empty(&dc->vert_hair_rectlist)) {
	    vert_hair_set_up(dtop, TRUE);
	    pr_set_planes(pr, PIXPG_CURSOR, full_planes);
	    if (dtop_rl_rop(pr, 0, 0, &dc->vert_hair_rectlist,
	            PIX_SET, (struct pixrect *)0, 0, 0))
		ROPERR(11)
	    pr_set_planes(pr, PIXPG_CURSOR_ENABLE, full_planes);
	    if (dtop_rl_rop_enable(pr, 0, 0, &dc->vert_hair_rectlist,
		    PIX_SET, (struct pixrect *)0, 0, 0))
		ROPERR(12)
	}

Cursor_Done:
    pr_set_planes(pr, original_plane_group, full_planes);
    return;
    }

    /*
     *  A Hack of Sorts ...
     *  we don't need to mess with the double registers on the
     *  Crane (or other 24 bit fb's, as the kernel can't access
     *  them, and therefore can't draw a cursor in them.  This
     *  means that we don't need a sync in pr_dbl_set associated
     *  with cursor drawing, which is a big performance plus.
     *
     *  So, don't mess with the double register if we've got
     *  24 bit color.
     *
     * (Dwight Wilcox - 10 May 89)
     */

    /* Save the read/write control states */
    /* Set read/write control bits to read/write from the foreground */
    if (dtop->dt_dblcount &&
	!(dtop->dt_plane_groups_available[PIXPG_24BIT_COLOR])) {
	readstate = pr_dbl_get(dtop->dt_pixrect, PR_DBL_READ);
	writestate = pr_dbl_get(dtop->dt_pixrect, PR_DBL_WRITE);
	if (pr_dbl_set(dtop->dt_pixrect, PR_DBL_WRITE, dtop->dt_dbl_frgnd,
			PR_DBL_READ, dtop->dt_dbl_frgnd, 0))
#ifdef WINDEVDEBUG
		printf("Error dbl_set draw cursor\n");
#else
		;
#endif
    }

    /*
     * Keep 24-bit cursors and fullscreen cursors (such as wmgr move/strech,
     * menu cursors, confirm cursors, or fullscreen crosshairs) in the overlay.
     * Fullscreen crosshairs (in general but especially on a multiple plane
     * group framebuffers) are very suspect.
     */

    if (dtop->dt_plane_groups_available[PIXPG_OVERLAY] &&
	(dtop->shared_info->mouse_plane_group == PIXPG_24BIT_COLOR ||
	(dtop->dt_cursorwin && fullscreen(&dtop->dt_cursorwin->w_cursor))))
    {
	dtop->dt_cursor.enable_color = 1;
	dtop->shared_info->mouse_plane_group = PIXPG_OVERLAY;
    }

    shared = &dtop->shared_info->cursor_info;
    dc = &dtop->dt_cursor;
    pr = dtop->dt_pixrect;
    original_plane_group = pr_get_plane_group(pr);

    if (cursor_up(dtop) ||
	enable_plane_cursor_up(dtop) || videnb_plane_cursor_up(dtop) ||
	horiz_hair_up(dtop) || vert_hair_up(dtop)) {

	/* Set up plane group to be the one that holds the screen image */
	pr_set_planes(pr, shared->plane_group, full_planes);

	/*
	 * Take existing cursor down by putting old screen image there
	 */
	if (cursor_up(dtop)) {
	    /* prepare the screen pixrect for use */
	    win_lock_prepare_screen_pr(shared);
	    cursor_set_up(dtop, FALSE);
	    /* rop up the saved bits */
	    if (pr_rop(pr, shared->x, shared->y,
			shared->screen_pr.pr_width, shared->screen_pr.pr_height,
			PIX_SRC, &shared->screen_pr, 0, 0))
		ROPERR(1)
	}
	/* replace bits in enable plane */
	if (enable_plane_cursor_up(dtop)) {
	    enable_plane_cursor_set_up(dtop, FALSE);
	    pr_set_planes(pr, PIXPG_OVERLAY_ENABLE, full_planes);
	    if (pr_rop(pr, shared->x, shared->y,
		dc->enable_mpr.pr_width, dc->enable_mpr.pr_height,
		PIX_SRC, &dc->enable_mpr, 0, 0))
		ROPERR(2)
	    pr_set_planes(pr, original_plane_group, full_planes);
	}

	/* replace bits in video enable plane */
	if (videnb_plane_cursor_up(dtop)) {
	    videnb_plane_cursor_set_up(dtop, FALSE);
	    pr_set_planes(pr, PIXPG_VIDEO_ENABLE, full_planes);
	    if (pr_rop(pr, shared->x, shared->y,
		dc->videnb_mpr.pr_width, dc->videnb_mpr.pr_height,
		PIX_SRC, &dc->videnb_mpr, 0, 0))
		ROPERR(2)
	    pr_set_planes(pr, original_plane_group, full_planes);
	}

	/* replace image under cross hairs */
	if (horiz_hair_up(dtop)) {
	    horiz_hair_set_up(dtop, FALSE);
	    if (dtop_rl_rop(pr, 0, 0,
				&dc->horiz_hair_rectlist,
				PIX_SRC, &dc->horiz_hair_mpr,
				0, dc->hair_y))
		ROPERR(3)
	    rl_free(&dc->horiz_hair_rectlist);
	}

	if (vert_hair_up(dtop)) {
	    vert_hair_set_up(dtop, FALSE);
	    if (dtop_rl_rop(pr, 0, 0,
				&dc->vert_hair_rectlist,
				PIX_SRC, &dc->vert_hair_mpr,
				dc->hair_x, 0))
		ROPERR(4)
	    rl_free(&dc->vert_hair_rectlist);
	}

	goto Done;
    }

    shared = &dtop->shared_info->cursor_info;

    if (shared->hardware_cursor) {	/* SEVANS */
	/*
	 * Mark cursor as inactive if in hardware now that we have the software
	 * cursor off the screen.  This is for client program's locking code
	 * sake.
	 */
	shared->cursor_active = 0;
	/*
	 * For hardware cursors, should have the latest image by now.
	 * Update new_cursor here so that we don't keep tripping through this
	 * code (when cursorwin == NULL) until something interesting changes.
	 * Didn't clear new_cursor before in case software cursor needed to
	 * be removed.
	 */
        dtop->shared_info->status.new_cursor = FALSE;
    }

    /*
     * Setting of shared->plane_group was moved up from just above the next
     * pr_set_planes call so that is would always be set, even in the case of
     * using a hardware cursor, so that client locking would not be forced into
     * the kernel (see libsunwindow/pw/pw_shared.c).
     */
    shared->plane_group = dtop->shared_info->mouse_plane_group;	/* SEVANS */

    /*
     * if dtop_cursorwin is not open then zero out and return;
     * this shouldn't really be necessary since the cursor window
     * should be zeroed out when the window is closed
     */

    if (dtop->dt_cursorwin && !(dtop->dt_cursorwin->w_flags & WF_OPEN)) {
		dtop->dt_cursorwin = WINDOW_NULL;
		goto Done;
    }

    /*
     * Put up new cursor by saving old screen image then writing
     * new cursor.
     */
    cursorwin = dtop->dt_cursorwin;
    if (!cursorwin || !(ws = dtop->dt_ws) || (ws->ws_loc_dtop != dtop))
	goto Done;

    /*
     * We are dealing with the latest cursor.
     * Doesn't hurt to mark this again if hardware cursor.
     */
    dtop->shared_info->status.new_cursor = FALSE;

    cursor = &cursorwin->w_cursor;
    data = &cursorwin->w_cursordata;
    shape = cursor->cur_shape;

    want_cursor = shape->pr_width && shape->pr_height &&
			shape->pr_depth && data->md_image &&
			show_cursor(cursor) && !shared->hardware_cursor;

    if (!(want_cursor || show_horiz_hair(cursor) || show_vert_hair(cursor)))
	goto Done;

    /*
     * Figure out where on screen to draw cursor.
     */
    shared->x = dtop->dt_rt_x - cursor->cur_xhot;
    shared->y = dtop->dt_rt_y - cursor->cur_yhot;

    /* Set plane group of plane group to write into */
    pr_set_planes(pr, shared->plane_group, full_planes);

    if (want_cursor) {
	if (show_cursor(cursor)) {
	    /*
	     * Prepare pixrect in which will store screen image.
	     */
	    win_lock_prepare_screen_pr(shared);

	    scrmpr_data = (struct mpr_data *)(shared->screen_pr.pr_data);
	    shared->screen_pr = *shape;
	    shared->screen_pr.pr_depth = pr->pr_depth;
	    shared->screen_pr.pr_data = (caddr_t) scrmpr_data;
	    *scrmpr_data = *data;
	    scrmpr_data->md_image = shared->screen_image;
	    (void) dtop_set_linebytes(&shared->screen_pr);

	    cursor_offset_x = shared->x;
	    cursor_offset_y = shared->y;

	    if (dtop->dt_plane_groups_available[PIXPG_24BIT_COLOR] &&
		shared->plane_group == PIXPG_8BIT_COLOR)
	    {
		struct rect         cursor_rect;
		struct rectlist     cursor_rect_list;

		rect_construct(&cursor_rect,
		    shared->x - cursorwin->w_screenx,
		    shared->y - cursorwin->w_screeny,
		    shared->screen_pr.pr_width, shared->screen_pr.pr_height);
		cursor_rect_list = rl_null;
		rl_rectintersection(&cursor_rect, &cursorwin->w_rlexposed,
		    &cursor_rect_list);

		shared->x=cursorwin->w_screenx+cursor_rect_list.rl_bound.r_left;
		shared->y=cursorwin->w_screeny+cursor_rect_list.rl_bound.r_top;
		shared->screen_pr.pr_width = cursor_rect_list.rl_bound.r_width;
		shared->screen_pr.pr_height=cursor_rect_list.rl_bound.r_height;
		rl_free(&cursor_rect_list);
	    }

	    cursor_offset_x = shared->x - cursor_offset_x;
	    cursor_offset_y = shared->y - cursor_offset_y;

	    /* Copy from display to memory */
	    if (pr_rop(&shared->screen_pr, 0, 0,
			shared->screen_pr.pr_width, shared->screen_pr.pr_height,
			PIX_SRC, pr, shared->x, shared->y))
		ROPERR(5)
	}
	if (show_enable_plane_cursor(dtop)) {
	    /* Prepare pixrect in which will store enable plane image */
	    dc->enable_mpr.pr_width = shape->pr_width;
	    dc->enable_mpr.pr_height = shape->pr_height;
	    (void) dtop_set_linebytes(&dc->enable_mpr);
	    /* Copy from display to memory */
	    pr_set_planes(pr, PIXPG_OVERLAY_ENABLE, full_planes);
	    if (pr_rop(&dc->enable_mpr, 0, 0, shape->pr_width, shape->pr_height,
			PIX_SRC, pr, shared->x, shared->y))
		ROPERR(6)
	    pr_set_planes(pr, shared->plane_group, full_planes);
	}

	if (show_videnb_plane_cursor(dtop)) {
	    /* Prepare pixrect in which will store video enable plane image */
	    dc->videnb_mpr.pr_width = shape->pr_width;
	    dc->videnb_mpr.pr_height = shape->pr_height;
	    ((struct mpr_data *)dc->videnb_mpr.pr_data)->md_linebytes =
		mpr_linebytes(dc->videnb_mpr.pr_width, 1);
	    /* Copy from display to memory */
	    pr_set_planes(pr, PIXPG_VIDEO_ENABLE, full_planes);
	    if (pr_rop(&dc->videnb_mpr, 0, 0, shape->pr_width, shape->pr_height,
		PIX_SRC, pr, shared->x, shared->y))
		ROPERR(601)
	    pr_set_planes(pr, shared->plane_group, full_planes);
	}
    }

    if (show_horiz_hair(cursor) || show_vert_hair(cursor)) {
	/* prepare cross hair storage */
	/* get the horizontal & vertical info from the input window */

	dc->horiz_hair_mpr.pr_depth = pr->pr_depth;
	dc->horiz_hair_mpr.pr_height = cursor->horiz_hair_thickness;
	(void) dtop_set_linebytes(&dc->horiz_hair_mpr);

	dc->vert_hair_mpr.pr_depth = pr->pr_depth;
	dc->vert_hair_mpr.pr_width = cursor->vert_hair_thickness;
	(void) dtop_set_linebytes(&dc->vert_hair_mpr);

	dc->hair_x = dtop->dt_rt_x - (dc->vert_hair_mpr.pr_width / 2);
	dc->hair_y = dtop->dt_rt_y - (dc->horiz_hair_mpr.pr_height / 2);

	if (show_horiz_hair(cursor)) {
	    /* construct the horizontal hair window/fullscreen rect */
	    rect_construct(&hair_rect, 0,
				fullscreen(cursor) ? dc->hair_y :
				dc->hair_y - cursorwin->w_screeny,
				dc->horiz_hair_mpr.pr_width,
				dc->horiz_hair_mpr.pr_height);

	    /*
	     * determine the intersection with the exposed area of
	     *  the window.
	     */
	    dc->horiz_hair_rectlist = rl_null;
	    if (fullscreen(cursor))
		rl_initwithrect(&hair_rect, &dc->horiz_hair_rectlist);
	    else {
		rl_rectintersection(&hair_rect, &cursorwin->w_rlexposed,
				    &dc->horiz_hair_rectlist);
		/* convert to screen coordinates */
		dc->horiz_hair_rectlist.rl_x = cursorwin->w_screenx;
		dc->horiz_hair_rectlist.rl_y = cursorwin->w_screeny;
		rl_normalize(&dc->horiz_hair_rectlist);
		/* convert hair_rect top to screen coordinates */
		hair_rect.r_top = dc->hair_y;
	    }

	    /* if length is not full, cut out excess */
	    if (cursor->horiz_hair_length != CURSOR_TO_EDGE) {
		int	max_length;
		int	x_left;

		if (horiz_border_gravity(cursor)) {
		    max_length = fullscreen(cursor)?
			dc->horiz_hair_mpr.pr_width : cursorwin->w_rect.r_width;
		    x_left = fullscreen(cursor) ? cursor->horiz_hair_length :
			cursorwin->w_screenx + cursor->horiz_hair_length;

		    hair_rect.r_left = x_left;
		    hair_rect.r_width = max_length-2*cursor->horiz_hair_length;
		    rl_rectdifference(&hair_rect, &dc->horiz_hair_rectlist,
					&dc->horiz_hair_rectlist);
		} else {
		    /* length is from center */
		    x_left = fullscreen(cursor) ? 0 : cursorwin->w_screenx;

		    hair_rect.r_left = x_left;
		    hair_rect.r_width =
			dc->hair_x - cursor->horiz_hair_length - x_left;
		    rl_rectdifference(&hair_rect, &dc->horiz_hair_rectlist,
					&dc->horiz_hair_rectlist);

		    hair_rect.r_left = dc->hair_x + cursor->horiz_hair_length;
		    hair_rect.r_width = dc->horiz_hair_mpr.pr_width;
		    rl_rectdifference(&hair_rect, &dc->horiz_hair_rectlist,
					&dc->horiz_hair_rectlist);
		}
	    }

	    if (cursor->horiz_hair_gap) {
		/* cut out a gap the width of the cursor or as specified */
		if (cursor->horiz_hair_gap == CURSOR_TO_EDGE) {
		    hair_rect.r_width = shape->pr_width;
		    hair_rect.r_left = shared->x;
		} else {
		    hair_rect.r_width = cursor->horiz_hair_gap;
		    hair_rect.r_left = dc->hair_x - hair_rect.r_width / 2;
		}
		rl_rectdifference(&hair_rect, &dc->horiz_hair_rectlist,
					&dc->horiz_hair_rectlist);
	    }

	    /* copy the image under the exposed area of the cross hair */
	    if (!rl_empty(&dc->horiz_hair_rectlist)) {
		if (dtop_rl_rop(&dc->horiz_hair_mpr, 0, dc->hair_y,
				&dc->horiz_hair_rectlist,
				PIX_SRC, pr, 0, 0))
		    ROPERR(7)
		}
	}

	if (show_vert_hair(cursor)) {
	    /* construct the vertical hair window/fullscreen rect */
	    rect_construct(&hair_rect,
		fullscreen(cursor) ? dc->hair_x :
			dc->hair_x - cursorwin->w_screenx,
			0,
			dc->vert_hair_mpr.pr_width,
			dc->vert_hair_mpr.pr_height);

	    /*
	     * determine the intersection with the exposed area of
	     *  the window.
	     */
	    dc->vert_hair_rectlist = rl_null;
	    if (fullscreen(cursor))
		rl_initwithrect(&hair_rect, &dc->vert_hair_rectlist);
	    else {
		rl_rectintersection(&hair_rect, &cursorwin->w_rlexposed,
				    &dc->vert_hair_rectlist);
		/* convert to screen coordinates */
		dc->vert_hair_rectlist.rl_x = cursorwin->w_screenx;
		dc->vert_hair_rectlist.rl_y = cursorwin->w_screeny;
		rl_normalize(&dc->vert_hair_rectlist);
		/* convert hair_rect left to screen coordinates */
		hair_rect.r_left = dc->hair_x;
	    }

	    /* if length is not full, cut out excess */
	    if (cursor->vert_hair_length != CURSOR_TO_EDGE) {
		int	max_length;
		int	y_top;

		if (vert_border_gravity(cursor)) {
		    max_length = fullscreen(cursor)?
			dc->vert_hair_mpr.pr_height:cursorwin->w_rect.r_height;
		    y_top = fullscreen(cursor) ? cursor->vert_hair_length :
			cursorwin->w_screeny + cursor->vert_hair_length;

		    hair_rect.r_top = y_top;
		    hair_rect.r_height = max_length-2*cursor->vert_hair_length;
		    rl_rectdifference(&hair_rect, &dc->vert_hair_rectlist,
					&dc->vert_hair_rectlist);
		} else {
		    /* length is from center */
		    y_top = fullscreen(cursor) ? 0 : cursorwin->w_screeny;

		    hair_rect.r_top = y_top;
		    hair_rect.r_height =
			dc->hair_y - cursor->vert_hair_length - y_top;
		    rl_rectdifference(&hair_rect, &dc->vert_hair_rectlist,
					&dc->vert_hair_rectlist);

		    hair_rect.r_top = dc->hair_y + cursor->vert_hair_length;
		    hair_rect.r_height = dc->vert_hair_mpr.pr_height;
		    rl_rectdifference(&hair_rect, &dc->vert_hair_rectlist,
					&dc->vert_hair_rectlist);
		}
	    }

	    if (cursor->vert_hair_gap) {
		/* cut out a gap the height of the cursor or as specified */
		if (cursor->vert_hair_gap == CURSOR_TO_EDGE) {
		    hair_rect.r_height = shape->pr_height;
		    hair_rect.r_top = shared->y;
		} else {
		    hair_rect.r_height = cursor->vert_hair_gap;
		    hair_rect.r_top = dc->hair_y - hair_rect.r_height / 2;
		}
		rl_rectdifference(&hair_rect, &dc->vert_hair_rectlist,
				&dc->vert_hair_rectlist);
	    }

	    /* copy the image under the exposed area of the cross hair */
	    if (!rl_empty(&dc->vert_hair_rectlist)) {
		if (dtop_rl_rop(&dc->vert_hair_mpr, dc->hair_x, 0,
				&dc->vert_hair_rectlist,
				PIX_SRC, pr, 0, 0))
		    ROPERR(8)
	    }
	}
    }

    /*
     * Set planes such that will write to foreground and background
     * of current colormap segment.
     */
    planes = cursorwin->w_cms.cms_size - 1;
    if( (planes & cursorwin->w_cms.cms_size) != 0 )
    {
      /* colormap wasn't a power of 2 in length,
	 compute planes a different way */
      register int i = planes ;
      planes = 1 ;
      while( i != 0 )
      {  
	planes <<= 1 ;
	i >>= 1 ;
      }  
      --planes ;
    }
#define	planes_fully_implemented
#ifdef	planes_fully_implemented
    pr_putattributes(pr, &planes);
#else
    pr_set_planes(pr, shared->plane_group, planes);
#endif	planes_fully_implemented

    /*
     * Write to display
     */
    /* write the horizontal cross hair */
    if (show_horiz_hair(cursor) && !rl_empty(&dc->horiz_hair_rectlist)) {
	horiz_hair_set_up(dtop, TRUE);
	if (dtop_rl_rop(pr, 0, 0, &dc->horiz_hair_rectlist,
		    cursor->horiz_hair_op | PIX_COLOR(cursor->horiz_hair_color),
		    (struct pixrect *)0, 0, 0))
	    ROPERR(9)
    }

    /* write the vertical cross hair */
    if (show_vert_hair(cursor) && !rl_empty(&dc->vert_hair_rectlist)) {
	vert_hair_set_up(dtop, TRUE);
	if (dtop_rl_rop(pr, 0, 0, &dc->vert_hair_rectlist,
		    cursor->vert_hair_op | PIX_COLOR(cursor->vert_hair_color),
		    (struct pixrect *)0, 0, 0))
	    ROPERR(10)
    }

    if (want_cursor) {
	int op;

	/* write the cursor */
	if (show_cursor(cursor)) {
	    cursor_set_up(dtop, TRUE);
	    if (pr_rop(pr, shared->x, shared->y,
			shared->screen_pr.pr_width, shared->screen_pr.pr_height,
			cursor->cur_function, shape,
			cursor_offset_x, cursor_offset_y))
		ROPERR(11)
	}
	/* write the enable plane cursor */
	if (show_enable_plane_cursor(dtop)) {
	    op = (dc->enable_color)? PIX_SRC | PIX_DST:
		PIX_NOT(PIX_SRC) & PIX_DST;
	    enable_plane_cursor_set_up(dtop, TRUE);
	    pr_set_planes(pr, PIXPG_OVERLAY_ENABLE, full_planes);
	    if (pr_rop(pr, shared->x, shared->y, dc->enable_mpr.pr_width,
			dc->enable_mpr.pr_height, op, shape, 0, 0))
		    ROPERR(12)
	    pr_set_planes(pr, original_plane_group, full_planes);
	}

	/* write the video enable plane cursor */
	if (show_videnb_plane_cursor(dtop)) {
	    op = PIX_NOT(PIX_SRC) & PIX_DST;
	    videnb_plane_cursor_set_up(dtop, TRUE);
	    pr_set_planes(pr, PIXPG_VIDEO_ENABLE, full_planes);
	    if (pr_rop(pr, shared->x, shared->y, dc->videnb_mpr.pr_width,
			dc->videnb_mpr.pr_height, op, shape, 0, 0))
		    ROPERR(12)
	    pr_set_planes(pr, shared->plane_group, full_planes);
	}
    }

Done:
    if (dtop->dt_dblcount &&
	!(dtop->dt_plane_groups_available[PIXPG_24BIT_COLOR])) {
	if (pr_dbl_set(dtop->dt_pixrect, PR_DBL_READ, readstate, PR_DBL_WRITE,
	    writestate, 0))
#ifdef WINDEVDEBUG
		printf("Error in drawcursor: pr_dbl_set\n");
#else
		;
#endif
    }
    pr_set_planes(pr, original_plane_group, full_planes);
    return;

Roperr:
    pr_set_planes(pr, original_plane_group, full_planes);
#ifdef WINDEVDEBUG
    printf("Kernel cursor Roperr %D\n", rop_err_id);
#endif
#ifdef lint
    printf("Kernel cursor Roperr %D\n", rop_err_id);
#endif
}

/* rl should be normalized and in screen coordinates */
dtop_rl_rop(dest_pixrect, dest_dx, dest_dy, rl, op, src_pixrect,
	    src_dx, src_dy)
	struct pixrect	*dest_pixrect;
	int		dest_dx, dest_dy;
	struct rectlist	*rl;
	int		op;
	struct pixrect	*src_pixrect;
	int		src_dx, src_dy;
/*
 * dtop_rl_rop does a pr_rop from src_pixrect to dst_pixrect for each
 * area described in rl.
 */
{
    register struct rectnode	*rectnode;
    register struct rect	*r;

    for (rectnode = rl->rl_head; rectnode;
	rectnode = rectnode->rn_next) {
	r = &rectnode->rn_rect;
	if (pr_rop(dest_pixrect, r->r_left - dest_dx, r->r_top - dest_dy,
			r->r_width, r->r_height, op, src_pixrect,
			r->r_left - src_dx, r->r_top - src_dy))
		return (1);
    }
    return (0);
} /* dtop_rl_rop*/

/*  This "smears" the enable plane a pixel in each each dir 
    Used only if cursor planes are available - mostly for cross
    hairs. rl should be normalized and in screen coordinates */
dtop_rl_rop_enable(dest_pixrect, dest_dx, dest_dy, rl, op, src_pixrect,
	    src_dx, src_dy)
	struct pixrect	*dest_pixrect;
	int		dest_dx, dest_dy;
	struct rectlist	*rl;
	int		op;
	struct pixrect	*src_pixrect;
	int		src_dx, src_dy;
/*
 * dtop_rl_rop does a pr_rop from src_pixrect to dst_pixrect for each
 * area described in rl.
 */
{
    register struct rectnode	*rectnode;
    register struct rect	*r;

    for (rectnode = rl->rl_head; rectnode;
	rectnode = rectnode->rn_next) {
	r = &rectnode->rn_rect;
	if (pr_rop(dest_pixrect, r->r_left - dest_dx-1, r->r_top - dest_dy-1,
			r->r_width+2, r->r_height+2, op, src_pixrect,
			r->r_left - src_dx, r->r_top - src_dy))
		return (1);
    }
    return (0);
} /* dtop_rl_rop */


/* return TRUE if the mutex, display or data locks are set. */
int
dtop_check_all_locks(lok_client)
	caddr_t	lok_client;
{
	Desktop			*dtop = (Desktop *) lok_client;
	register Win_lock_block	*shared_info = dtop->shared_info;

	return (shared_info->mutex.lock || shared_info->data.lock ||
		shared_info->display.lock);
}


static void
dtop_init_data_lock(dtop, process)
	register Desktop	*dtop;
	struct proc		*process;
{
	register Winlock	*wlock = &dtop->dt_datalock;

	wlok_setlock(wlock, &dtop->shared_info->data.lock, WLOK_SHAREDLOCK_BITS,
	    &wlock->lok_count_storage, process);
	wlock->lok_client = (caddr_t)dtop;
	wlock->lok_unlock_action = dtop_unlockdata;
	wlock->lok_timeout_action = dtop_timedout_data;
	wlock->lok_string = "data";
	wlock->lok_wakeup = (caddr_t) dtop->shared_info;
	wlock->lok_other_check = dtop_check_all_locks;
}

static void
dtop_init_display_lock(dtop, id, process)
	register Desktop	*dtop;
	int			id;
	struct proc		*process;
{
	register Winlock	*wlock = &dtop->dt_displaylock;

	wlok_setlock(wlock, &dtop->shared_info->display.lock,
	    WLOK_SHAREDLOCK_BITS, &dtop->shared_info->display.count, process);
		/*
		 * 1 use to be used instead of WLOK_SHAREDLOCK_BITS, but,
		 * due to the fact that sparc wants to set different bits
		 * from 68K, there was a problem.  Now, setting all the bits
		 * should allow both schemes to work.
		 */
	wlock->lok_client = (caddr_t) dtop;
	wlock->lok_unlock_action = dtop_unlockdisplay;
	wlock->lok_timeout_action = dtop_timedout_display;
	wlock->lok_string = "display";
	wlock->lok_wakeup = (caddr_t) dtop->shared_info;
	wlock->lok_id = id;
	wlock->lok_other_check = dtop_check_all_locks;
}

static void
dtop_init_mutex_lock(dtop, id, process)
	register Desktop	*dtop;
	int			id;
	struct proc		*process;
{
	register Winlock	*wlock = &dtop->dt_mutexlock;

	wlok_setlock(wlock, &dtop->shared_info->mutex.lock,
	    WLOK_SHAREDLOCK_BITS, &dtop->shared_info->mutex.count, process);
	wlock->lok_client = (caddr_t) dtop;
	wlock->lok_unlock_action = dtop_unlockmutex;
	/* use the display timedout routine */
	wlock->lok_timeout_action = dtop_timedout_display;
	wlock->lok_string = "mutex";
	wlock->lok_wakeup = (caddr_t) dtop->shared_info;
	wlock->lok_id = id;
	wlock->lok_other_check = dtop_check_all_locks;
}

void
dtop_putcolormap(dtop, cms_size, cmap)
	Desktop *dtop;
	int cms_size;
	struct cms_map *cmap;
{
	register struct pixrect *pr = dtop->dt_pixrect;
	int original_plane_group;

	/*
	 * Don't change colormap of monochrome
	 * because not really a colormap under device.  This code (in
	 * combination with the cursor tracking stuff) assumes
	 * that changing the colormap doesn't changed the bits.
	 * Removing test for colormap size leaves reverse cursor images
	 * whenever you change cursor windows with different backgrounds.
	 */
	if (dtop->dt_cmsize <= 2)
		return;
	/*
	 * Don't try to set colormap of a pixrect of depth one.
	 * If such a beast is encounted, change the plane group
	 * to a color plane group.
	 */
	original_plane_group = pr_get_plane_group(pr);
	if (pr->pr_depth == 1) {
		 if (dtop->dt_plane_groups_available[PIXPG_8BIT_COLOR])
			pr_set_planes(pr, PIXPG_8BIT_COLOR, PIX_ALL_PLANES);
		else if (dtop->dt_plane_groups_available[PIXPG_24BIT_COLOR])
			pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
		else {
#ifdef WINDEVDEBUG
			if (!(dtop->dt_screen.scr_flags & SCR_OVERLAYONLY))
				printf("Tried to set 1 bit deep colormap\n");
#endif
			return;
		}
	}
	if (pr_putcolormap(pr, 0, cms_size,
	    cmap->cm_red, cmap->cm_green, cmap->cm_blue))
#ifdef WINDEVDEBUG
		printf("pr_putcolormap error\n");
#else
		;
#endif
	pr_set_planes(pr, original_plane_group, PIX_ALL_PLANES);
}



/*
 * Pick the window with the cursor. If this window is not a double
 * bufferer or cursor is not in any of the windows, choose a double
 * bufferer from the list of windows (first one from the top to the
 * the bottom). There will always be atleast one double bufferer
 * when this routine is called. Flip the display, write and read states.
 */

void
dtop_flip_display(dtop, w)
	register Desktop *dtop;
	register struct window *w;
{
	struct window *active_dbl = dtop->dt_curdbl;

	if (w == active_dbl) {
		/*
		 * Flip the display and set write to old foreground, read to old
		 * background.  Flip the dt_dbl_backgr and dt_dbl_frgnd field
		 */
		dtop_changedisplay(dtop, dtop->dt_dbl_bkgnd, dtop->dt_dbl_frgnd,
			FALSE);
		/*
		 * If the cursor is not in the active double bufferer
		 * choose another double bufferer.  The same double
		 * bufferer might get chosen again.
		 */
		if (dtop->dt_cursorwin != active_dbl) {
			/* Choose a new double bufferer */
			dtop_choose_dblbuf(dtop);
			/*
			 * If a new double bufferrer was chosen, copy from
			 * background buffer to forground buffer
			 */
			if (active_dbl != dtop->dt_curdbl)
			    dtop_copy_dblbuffer(dtop, w);
		}
	}
#if 0
	else {
		/* Pick a  window if current double bufferer is not active */
		if (curdbl process not active)
			dtop_choose_dblbuf(dtop);

	}
#endif
	return;
}


/*
 * Choose a double bufferer
 */
void
dtop_choose_dblbuf(dtop)
	register Desktop *dtop;
{
	register struct window *w = dtop->dt_cursorwin;

	dtop->dt_curdbl = WINDOW_NULL;
	if (dtop->dt_flags & DTF_DBLBUFFER) {
		if (w == WINDOW_NULL) {
			wt_enumeratechildren(dtop_isdblwin, dtop->dt_rootwin,
					(struct rect *)0);
			dtop->dt_flags &= ~DTF_DBL_FOUND;
		} else if (!(w->w_flags & WF_DBLBUF_ACCESS)) {
			wt_enumeratechildren(dtop_isdblwin, dtop->dt_rootwin,
					(struct rect *)0);
			dtop->dt_flags &= ~DTF_DBL_FOUND;
		} else
			dtop->dt_curdbl = w;
	}
}

/*
 * This routine is called by wt_enumeratechildren for each of the
 * windows determines whether the window is a double bufferer or not.
 * It sets a flag DTF_DBL_FOUND if a double bufferer is found.  The idea
 * is to find the first double bufferer (from TOP to BOTTOM).
 */

/*ARGSUSED*/
int
dtop_isdblwin(w, rect)
	register struct window *w;
	struct rect *rect;
{
	if (!(w->w_desktop->dt_flags & DTF_DBL_FOUND))
		if (w->w_flags & WF_DBLBUF_ACCESS) {
			w->w_desktop->dt_flags |= DTF_DBL_FOUND;
			w->w_desktop->dt_curdbl = w;
		}
}


/*
 * Remove the cursor, flip the display and redraw the
 * cursor in the foreground
 */
void
dtop_changedisplay(dtop, fore, back, first)
	Desktop *dtop;
	int 	fore, back;
	int	first;	/* Is it called for the first time ? */
{

	if (dtop) {
		int was_up = dtop_cursordraw(dtop);
		if (cursor_up(dtop) || horiz_hair_up(dtop) ||
		    vert_hair_up(dtop))
#ifdef WINDEVDEBUG
			printf("Warning: Cursor up dtop_changedisplay [windt.c]\n");
#else
			;
#endif
		if (first)
			dtop->dt_dblcount++;
		dtop->dt_dbl_frgnd = fore;
		dtop->dt_dbl_bkgnd = back;
		if (pr_dbl_set(dtop->dt_pixrect, PR_DBL_DISPLAY,
						dtop->dt_dbl_frgnd, 0))
#ifdef WINDEVDEBUG
			printf("Error in dtop_changedisplay \n");
#else
			;
#endif
		if (was_up)
			dtop_cursorup(dtop);
	}

}

static int
dtop_cursordraw(dtop)
	Desktop *dtop;
{
  /* cg9 doesn't need cursor updates during double buffering */
  /* don't remove: see comments in dtop_drawcursor() */
  if (dtop->dt_flags & DTF_DBLBUFFER &&
      dtop->dt_plane_groups_available[PIXPG_24BIT_COLOR])
    return(0);

	if (cursor_up(dtop) || horiz_hair_up(dtop) || vert_hair_up(dtop))
	    if (!win_lock_mutex_locked(dtop->shared_info)) {
		dtop_drawcursor(dtop);
		return (1);
	    }
     	return (0);
}



void
dtop_copy_dblbuffer(dtop, w)
	register Desktop *dtop;
	register struct window *w;
{

	int cursor_removed;

	cursor_removed = dtop_cursordraw(dtop);
	/* Copy foreground buffer into bkgnd buffer */
	if (pr_dbl_set(dtop->dt_pixrect, PR_DBL_READ, dtop->dt_dbl_frgnd,
	    PR_DBL_WRITE, dtop->dt_dbl_bkgnd, 0))
#ifdef WINDEVDEBUG
		printf("Error dbl_set flip disp #2\n");
#else
		;
#endif
	/* Screen x, y need to be normalized */
	if (dtop_rl_rop(dtop->dt_pixrect, -w->w_screenx, -w->w_screeny,
	    &w->w_rlexposed, PIX_SRC, dtop->dt_pixrect, -w->w_screenx,
	    -w->w_screeny))
#ifdef WINDEVDEBUG
		printf("roperr in flip display\n");
#else
		;
#endif
	if (cursor_removed)
		(void)dtop_cursorup(dtop);
}


dtop_event_to_sharedq(dtop, event)
	Desktop *dtop;
	struct	inputevent *event;
{
	register int	old_tail = dtop->shared_info->shared_eventq.tail;
	register int	new_tail = old_tail + 1;
	register int	head = dtop->shared_info->shared_eventq.head;
	register int	size = dtop->shared_info->shared_eventq.size;

	if (size <= 0) {
		return (sizeof (*event));
	}
	if (new_tail == size)
		new_tail = 0;
	if (new_tail == head)
		return (sizeof (*event));
	dtop->dt_sharedq[old_tail] = *event;
#ifdef WINDEVDEBUG
	if (win_sharedq_debug) {
		printf("size=%d head=%d oldtail=%d\n",
			size, head, old_tail);
	}
#endif
	dtop->shared_info->shared_eventq.tail = new_tail;

	return (0);
}


int
dtop_dblset(dtop, attribute, value)
	register Desktop *dtop;
	int	attribute;
	int	value;
{
	int pr_value;

	if (value == PW_DBL_BOTH)
		pr_value = PR_DBL_BOTH;
	else if (value == PW_DBL_FORE)
		pr_value = dtop->dt_dbl_frgnd;
	else if (value == PW_DBL_BACK)
		pr_value = dtop->dt_dbl_bkgnd;
	else return (-1);

	if (pr_dbl_set(dtop->dt_pixrect, attribute, pr_value, 0)) {
#ifdef WINDEVDEBUG
		printf("error in ioctl SET for dbl_set\n");
#endif
		return (-1);
	}
	return (0);
}


static int
dtop_set_linebytes(mpr)
	register Pixrect *mpr;
{
	register int bytes;

	bytes = mpr_linebytes(mpr->pr_size.x, mpr->pr_depth);

#ifndef mc68010
	if (bytes & 2 && bytes > 2)
		bytes += 2;
#endif

	mpr_d(mpr)->md_linebytes = bytes;
	return (bytes);
}


/*
 * Allocate image space for crosshair pixrects
 */
dtop_setup_crosshairs(dtop)
	Desktop *dtop;
{
	struct dtopcursor *dc;
	int size;

	dc = &dtop->dt_cursor;

	if ((size = -dc->horiz_hair_size) > 0) {
		dc->horiz_hair_data.md_image =
			(short *)new_kmem_zalloc((u_int) size, KMEM_SLEEP);
		dc->horiz_hair_size = size;
	}
	if ((size = -dc->vert_hair_size) > 0) {
		dc->vert_hair_data.md_image =
			(short *) new_kmem_zalloc((u_int) size, KMEM_SLEEP);
		dc->vert_hair_size = size;
	}
}
