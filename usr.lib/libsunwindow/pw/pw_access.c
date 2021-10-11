#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)pw_access.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Pw_access.c: Implement the clipping list and locking aspects of
 *		the pixwin.h interface.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_planegroups.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/cms_mono.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/pw_util.h>
#include <pixrect/pr_dblbuf.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_lock.h>
#include <sunwindow/sv_malloc.h>

/*
 * When not zero will immediately release lock after WINLOCKSCREEN ioctl
 * so that the debugger is continually getting hung because of the lock.
 * USE ONLY DURING DEBUGGING WHEN YOU KNOW WHAT YOUR DOING!
 */
int	pixwindebug;

/*
 * Buffer used when copying rl from kernel
 */
#define	MAXRLBUFBYTES	(100*(sizeof(struct rectnode))+sizeof(struct rectlist))
#define	RLBUFBYTESINC	(10*(sizeof(struct rectnode)))
static	rlbufbytes;
static	char *rlbuf;

static	struct pixwin *pw_open_internal();

/*
 * Pixwin standard operations vector.
 */
extern	pwo_get(), pwo_put(), pwo_rop(), pwo_vector(), pwo_putcolormap(),
	    pwo_getcolormap(), pwo_putattributes(), pwo_getattributes(),
	    pwo_stencil(), pwo_batchrop();
int	pw_close();
struct	pixrect *pwo_region();

extern	struct pixrectops pw_opsstd;

extern	struct pixrectops pw_opsstd_null_lock;

/*
 * Pixwin standard clipping and locking operations.
 */
int	pwco_lockstd(), pwco_unlockstd(),
	    pwco_resetstd(), pwco_getclippingstd();

struct	pixrect *
pwo_region(pwsrc, x, y, w, h)
	struct	pixwin *pwsrc;
	int	x, y, w, h;
{
	register struct	pixwin *pw;
	register struct	rect *srcrect = pwsrc->pw_clipdata->pwcd_regionrect;
	register struct	rect *regrect;

	if (pw = pw_open_internal(pwsrc->pw_clipdata->pwcd_windowfd)) {
		regrect = pw->pw_clipdata->pwcd_regionrect = (struct rect *)
		    sv_calloc(1, sizeof(struct rect));
		rect_construct(regrect, x, y, w, h);
		/*
		 * Clip region request if subregion.
		 */
		if (srcrect) {
			rect_passtoparent(
			    srcrect->r_left, srcrect->r_top, regrect);
			(void)rect_intersection(regrect, srcrect, regrect);
		}
		if (pwsrc->pw_prretained) {
			pw->pw_prretained = pr_region(pwsrc->pw_prretained,
			    x, y, regrect->r_width, regrect->r_height);
		}
		/* Inherit offsets */
		pw->pw_clipdata->pwcd_x_offset =
		    pwsrc->pw_clipdata->pwcd_x_offset;
		pw->pw_clipdata->pwcd_y_offset =
		    pwsrc->pw_clipdata->pwcd_y_offset;
		pw->pw_clipdata->pwcd_flags = pwsrc->pw_clipdata->pwcd_flags;
		/* Inherit cms stuff (will figure new clipping too) */
		(void)pw_initcms(pw);
		{
		    /* Inherit the window-id double buffer state */
		    int pg, save_planes;
		    struct fb_wid_dbl_info wid_dbl_info;

		    ioctl(pwsrc->pw_windowfd, WINWIDGET, &wid_dbl_info);
		    ioctl(pw->pw_windowfd, WINWIDSET, &wid_dbl_info);
		    pr_ioctl(pw->pw_pixrect, FBIOSWINFD, &pw->pw_windowfd);
		    pg = pr_get_plane_group(pw->pw_pixrect);
		    pr_getattributes(pw->pw_pixrect, &save_planes);
		    pw_set_planes_directly(pw, pg, save_planes);
		}
	}
	return((struct pixrect *)pw);
}

/*
 * Pixwin structure operations
 */
struct	pixwin *
pw_open(windowfd)
	int windowfd;
{
	register struct	pixwin *pw;

	if ((pw = pw_open_internal(windowfd))) {
		/* Initialize cms (will figure new clipping too) */
		(void)pw_initcms(pw);
		/* if it is a cgnine board, use fast mono */
		if (pw->pw_clipdata->pwcd_plane_group == PIXPG_24BIT_COLOR &&
		    strncmp(pw->pw_cmsname, CMS_MONOCHROME, CMS_NAMESIZE) == 0 &&
		    pw->pw_clipdata->pwcd_plane_groups_available [PIXPG_OVERLAY])
		   pw_use_fast_monochrome(pw);
	}
	return (pw);
}

/* open for fullscreen operations without monochrome problems */
struct pixwin  *
pw_open_fs(windowfd)
int             windowfd;
{
    register struct pixwin *pw;

    if ((pw = pw_open_internal(windowfd)))
	/* Initialize cms (will figure new clipping too) */
	(void) pw_initcms(pw);

    {

	/*
	 * Some planes are inappropriate for fullscreen operations.
	 * Remove them from the list to be copied.
	 */

	int                 i;
	unsigned char       elim_groups[PIX_MAX_PLANE_GROUPS];

	/* clear all entries in case the FBIO is not implemented for this fb */

	(void) pr_ioctl(pw->pw_pixrect, FBIOSWINFD, &pw->pw_windowfd);
	(void) bzero(elim_groups, PIX_MAX_PLANE_GROUPS);

	(void) pr_ioctl(pw->pw_pixrect, FBIO_FULLSCREEN_ELIMINATION_GROUPS,
			elim_groups);

	for (i = 0; i < PIX_MAX_PLANE_GROUPS; i++)
	    if (elim_groups[i])
		pw->pw_clipdata->pwcd_plane_groups_available[i] = 0;
    }

    return (pw);
}

/*
 *  "pw_open_sb"
 *
 *  Open a pixwin for use with a scrollbar. If the pixwin is for a 24-bit
 *  color surface, initialize the surface so that the plane mask will be
 *  PIX_ALL_PLANES rather than a 1. (ACG 22 June 90)
 */

struct pixwin      *
pw_open_sb(windowfd)
int                 windowfd;
{
    struct pixwin      *pw;

    if ((pw = pw_open_internal(windowfd)))
	/* Initialize cms (will figure new clipping too) */
	(void) pw_initcms(pw);

    if (pw->pw_clipdata->pwcd_plane_group == PIXPG_24BIT_COLOR)
	pw_use_color24(pw);

    return (pw);
}

static	struct	pixwin *
pw_open_internal(windowfd)
	register int windowfd;
{
	register struct	pixwin *pw;
	struct	screen screen;
	register int i;
	extern	struct pixrect *pr_open();
	extern	Win_lock_block *pw_get_lock_block();
	struct pwset list;

	pw = (struct pixwin *) sv_calloc(1, sizeof(struct pixwin));
	/*
	 * Initialization of data alterred by pwco_reset
	 */
	pw->pw_fixup = rl_null;
	pw->pw_clipdata = (struct pixwin_clipdata *)
	    sv_calloc(1, sizeof(struct pixwin_clipdata));
	pw->pw_clipdata->pwcd_clipping = rl_null;
	for (i=0;i<RECTS_SORTS;i++)
		pw->pw_clipdata->pwcd_clippingsorted[i] = rl_null;
	pw->pw_clipdata->pwcd_lockcount = 0;
	pw->pw_clipdata->pwcd_clipid = 0;
	pw->pw_clipdata->pwcd_damagedid = 0;
	pw->pw_clipdata->pwcd_state = PWCD_NULL;
	pw->pw_clipdata->pwcd_prsingle = (struct pixrect *) 0;
	pw->pw_clipdata->pwcd_prmulti = (struct pixrect *) 0;
	pw->pw_clipdata->pwcd_prl = (struct pixwin_prlist *) 0;
		/* Initialized when first used.  */
	pw->pw_clipdata->pwcd_x_offset = 0;
	pw->pw_clipdata->pwcd_y_offset = 0;
	(void)pw_reset_batch(pw);
	/*
	 * Initialization of data that is static over life of pixwin
	 */
	pw->pw_clipops = (struct pixwin_clipops *)
	    sv_calloc(1, sizeof(struct pixwin_clipops));
	pw->pw_clipops->pwco_lock = (int (*)()) pwco_lockstd;
	pw->pw_clipops->pwco_unlock = (int (*)()) pwco_unlockstd;
	pw->pw_clipops->pwco_reset = (int (*)()) pwco_resetstd;
	pw->pw_clipops->pwco_getclipping = (int (*)()) pwco_getclippingstd;
	pw->pw_clipdata->pwcd_windowfd = windowfd;

	/* set up the shared memory info */
	pw->pw_clipdata->pwcd_winnum = win_fdtonumber(windowfd);
	pw->pw_clipdata->pwcd_wl = 0;
	pw->pw_clipdata->pwcd_client = 
            (caddr_t) pw_get_lock_block(pw->pw_windowfd);

	/*
	 * Initialization of data that is changed by in pw_region.
	 */
	pw->pw_clipdata->pwcd_regionrect = (struct rect *)0;
	/*
	 * Initialization of data that is changed by user.
	 */
	pw->pw_prretained = 0;
	/*
	 * Initialization of data that is determined by pwcd_state when lock
	 * and reset in pwco_reset.
	 */
	pw->pw_ops = &pw_opsstd;
	pw->pw_opshandle = (caddr_t) pw;
	pw->pw_opsx = pw->pw_clipdata->pwcd_x_offset;
	pw->pw_opsy = pw->pw_clipdata->pwcd_y_offset;
	/*
	 * Initialization of data that changes when size|position changes.
	 * The r_left & r_top fields are set up when get clipping.
	 */
	(void)win_screenget(windowfd, &screen);
	if ((pw->pw_pixrect = pr_open(screen.scr_fbname))==0) {
		(void)pw_close(pw);
		return((struct pixwin *)0);
	}
	/*
	 * Check if double buffering exists.
	 * Ioctl WINDBLGET returns TRUE/FALSE in the value field.
	 */
	list.attribute = PR_DBL_AVAIL;
	if (ioctl(pw->pw_windowfd, WINDBLGET, &list) == -1) {
		if (errno == ENOTTY)
			/*
			 * For backwards compatibility reasons, ignore
			 * fact that kernel not prepared for this call.
			 * If we are running on an old kernel, then we
			 * don't support double buffering.
			 * There was no notion of dbl buf before 3.3.
			 */
			{}
		else
			(void)werror(-1, WINDBLGET);
	} else {
		if (list.value) {
		    pw->pw_clipdata->pwcd_flags |= PWCD_DBL_AVAIL;	
		    /*
		     * find out if there are window ids and if they
		     * support multiple window double buffering.
		     * see pw_dbl.c for the significance of these flags.
		     */
		    if (pr_dbl_get(pw->pw_pixrect, PR_DBL_WID) == 1)
		    {
			pw->pw_clipdata->pwcd_flags |= PWCD_WID_DBL;
			if (getenv("PW_COPY_ON_DBL_RELEASE"))
			    pw->pw_clipdata->pwcd_flags |=
				PWCD_COPY_ON_DBL_RELEASE;
		    }
		}
	}
	/*
	 * Caller is responsible for setting up the cms stuff.
	 * Also, the caller needs to set pw->pw_clipops->pwco_getclipping
	 * to pw_exposed.  Both of which are done in pw_initcms.
	 */
	return(pw);
}

pw_close(pw)
	struct	pixwin *pw;
{
	extern void pw_free_lock_block();
	(void)pw_reset(pw);
	if (pw->pw_pixrect)
		(void)pr_destroy(pw->pw_pixrect);
	if (pw->pw_prretained)
		(void)pr_destroy(pw->pw_prretained);
	if (pw->pw_clipdata->pwcd_regionrect)
		free((caddr_t)pw->pw_clipdata->pwcd_regionrect);

	/* free the shared lock block if ref count is zero */
        pw_free_lock_block(pw->pw_clipdata->pwcd_wl);
        /* free copy or real lock data */
	pw_free_lock_block((Win_lock_block *) pw->pw_clipdata->pwcd_client);

	free((caddr_t)pw->pw_clipdata);
	free((caddr_t)pw->pw_clipops);
	free((caddr_t)pw);
	return;
}

struct win_shared_eventqueue *
pw_share_queue(pw)
	struct	pixwin *pw;
{
	register struct win_lock_block	*wl;

	if (!pw || !pw->pw_clipdata || !(wl = pw->pw_clipdata->pwcd_wl))
		return (NULL);
	if (ioctl(pw->pw_windowfd, WINSHAREQUEUE, 0) < 0)
		return (NULL);
	wl->shared_eventq.events =
		(struct inputevent *) ((caddr_t) wl + sizeof (* wl));
	return (&wl->shared_eventq);
}

pw_exposed(pw)
	struct	pixwin *pw;
{
	pw->pw_clipops->pwco_getclipping = pw_exposed;
	(void)_pw_getclipping(pw, FALSE);
	pw->pw_clipdata->pwcd_damagedid = win_getdamagedid(
	    pw->pw_clipdata->pwcd_windowfd);
	    /* See comments above this procedure's implementation (below) */
	return;
}

pw_damaged(pw)
	struct	pixwin *pw;
{
	int	originalcall = (pw->pw_clipops->pwco_getclipping != pw_damaged);
	int	startingdid = pw->pw_clipdata->pwcd_damagedid;

	pw->pw_clipops->pwco_getclipping = pw_damaged;
	(void)_pw_getclipping(pw, TRUE);
	/*
	 * Don't change damagedid until pw_donedamaged called so that we know
	 * exactly which damaged area was repaired.
	 * We are noticing that this subroutine is called from pw_lock when the
	 * clipping changes during damage repair.
	 */
	pw->pw_clipdata->pwcd_damagedid = (originalcall)?
	    pw->pw_clipdata->pwcd_clipid: startingdid;
	(void)pw_preparesurface_full(pw, RECT_NULL, 1);
	return;
}

pw_donedamaged(pw)
	struct	pixwin *pw;
{
	(void)werror(ioctl(pw->pw_clipdata->pwcd_windowfd, WINDONEDAMAGED, 
	    &pw->pw_clipdata->pwcd_damagedid), WINDONEDAMAGED);
	/*
	 * free existing clipping
	 */
	pwco_reinitclipping(pw);
	(void)pw_exposed(pw);
	return;
}

/*
 * Pixwin_ops standard implementations
 */
pwco_lockstd(pw, affected)
	register struct pixwin *pw;
	struct	rect *affected;
{
#include <pixrect/gp1var.h>

	struct	winlock winlock;

	/*
	 * Bump lock count if already locked
	 */
	if (pw->pw_clipdata->pwcd_lockcount) {
		pw->pw_clipdata->pwcd_lockcount++;
		return;
	}
	pw->pw_clipdata->pwcd_lockcount = 1;
	/* Don't need screen access if batching */
	if (pw->pw_clipdata->pwcd_batch_type != PW_NONE)
		return;

	/*
	 * Make sure gp2 has correct colormap for 8-bit on 24-bit
	 * frame buffer.  This is a no-op for all other cases.
	 * Sorta ugly but gets full binary backward compatibilty.
	 */
	
	pr_ioctl(pw->pw_pixrect, GP1IO_SCMAP, 0);

	winlock.wl_rect = *affected;
	/* Transform from pixwin to window coordinates */
	winlock.wl_rect.r_left = PW_X_TO_WIN(pw, winlock.wl_rect.r_left);
	winlock.wl_rect.r_top = PW_Y_TO_WIN(pw, winlock.wl_rect.r_top);

	/* try to used the shared memory first */
	if (!pw_shared_lock(pw, &winlock.wl_rect)) {
	    /*
	     * Transform coordinate systems if a region.
	     * i.e. transform from region space to window space.
	     */
	    if (pw->pw_clipdata->pwcd_regionrect)
		    rect_passtoparent(pw->pw_clipdata->pwcd_regionrect->r_left,
			pw->pw_clipdata->pwcd_regionrect->r_top, &winlock.wl_rect);

	    /*
	     * Set lockcount before actually get lock so that reset knows that
	     * it should unlock if called during the time that this ioctl is
	     * happening.  This is a race condition.
	     * Extra unlocks are fairly harmless (unless you block).
	     */
	    (void)werror(ioctl(pw->pw_clipdata->pwcd_windowfd,
		WINLOCKSCREEN, &winlock), WINLOCKSCREEN);
	    if (pixwindebug)
		    (void)werror(ioctl(pw->pw_clipdata->pwcd_windowfd,
			WINUNLOCKSCREEN, 0), WINUNLOCKSCREEN);
	    if (pw->pw_clipdata->pwcd_clipid!=winlock.wl_clipid) {
		    /*
		     * Get new version of the clipping if out of date
		     */
		    (void)pw_getclipping(pw);
	    }
	} else {
		if (pixwindebug)
			(void)pw_shared_unlock(pw);
	}
	if (pw->pw_clipdata->pwcd_state == PWCD_SINGLERECT) {
		pw->pw_opshandle = (caddr_t) pw->pw_clipdata->pwcd_prsingle;
		pw->pw_ops = pw->pw_pixrect->pr_ops;
		pw->pw_opsx += pw->pw_clipdata->pwcd_clipping.rl_bound.r_left;
		pw->pw_opsy += pw->pw_clipdata->pwcd_clipping.rl_bound.r_top;
	} else if (pw->pw_clipdata->pwcd_state == PWCD_NULL) {
		pw->pw_ops = &pw_opsstd_null_lock;
	}
	return;
}

pwco_unlockstd(pw)
	register struct pixwin *pw;
{
	if (pw->pw_clipdata->pwcd_lockcount==0)
		return;
	if (pw->pw_clipdata->pwcd_lockcount==1) {
		/* Didn't get screen access if batching */
		if (pw->pw_clipdata->pwcd_batch_type != PW_NONE)
			goto Done;

		/* try to used the shared memory first */
		if (!pixwindebug && !pw_shared_unlock(pw))
			(void)werror(ioctl(pw->pw_clipdata->pwcd_windowfd,
			    WINUNLOCKSCREEN, 0), WINUNLOCKSCREEN);

		if ((pw->pw_clipdata->pwcd_state == PWCD_SINGLERECT) ||
		   (pw->pw_clipdata->pwcd_state == PWCD_NULL)) {
			pw->pw_opshandle = (caddr_t) pw;
			pw->pw_ops = &pw_opsstd;
			pw->pw_opsx = pw->pw_clipdata->pwcd_x_offset;
			pw->pw_opsy = pw->pw_clipdata->pwcd_y_offset;
		}
	}
Done:
	pw->pw_clipdata->pwcd_lockcount--;
	return;
}

pwco_resetstd(pw)
	struct	pixwin *pw;
{
	pwco_reinitclipping(pw);
	/*
	 * Unlock
	 */
	while (pw->pw_clipdata->pwcd_lockcount) (void)pw_unlock(pw);
	return;
}

/*ARGSUSED*/
pwco_getclippingstd(pw)
	struct	pixwin *pw;
{
	/*
	 * Standard is nop
	 */
}

/*
 * Pixwin structure operations utilities
 */
_pw_getclipping(pw, damaged)
	struct	pixwin *pw;
	bool	damaged;
{
	struct	winclip winclip;

	/*
	 * free existing clipping
	 */
	pwco_reinitclipping(pw);
	/*
	 * Copy clipping data from kernel
	 */
	win_getwinclip(pw->pw_clipdata->pwcd_windowfd, damaged, &winclip,
	    &pw->pw_clipdata->pwcd_clipping);
	pw->pw_clipdata->pwcd_clipid = winclip.wc_clipid;

	/* save the (left, top) of the window for use with 
	 * shared locking.
	 */
	pw->pw_clipdata->pwcd_screen_x = winclip.wc_screenrect.r_left;
	pw->pw_clipdata->pwcd_screen_y = winclip.wc_screenrect.r_top;

	/*
	 * Set up clipping utilities
	 */
	(void)_pw_setclippers(pw, &winclip.wc_screenrect);
}

/*
 * Set up all the various clipping optimizations
 */
_pw_setclippers(pw, screenrectarg)
	register struct	pixwin *pw;
	struct	rect *screenrectarg;
{
	struct	rect bounding;
	struct	rect screenrect;
	register struct pixwin_clipdata *pwcd = pw->pw_clipdata;

	screenrect = *screenrectarg;
	if (pwcd->pwcd_regionrect) {
		struct	rect regionrect;

		/*
		 * Trim/translate clipping to regionrect
		 */
		(void)rl_rectintersection(pwcd->pwcd_regionrect,
		    &pwcd->pwcd_clipping,
		    &pwcd->pwcd_clipping);
		rl_passtochild(pwcd->pwcd_regionrect->r_left,
		    pwcd->pwcd_regionrect->r_top,
		    &pwcd->pwcd_clipping);
		(void)rl_normalize(&pwcd->pwcd_clipping);
		/*
		 * Trim screenrect to regionrect
		 */
		regionrect = *pwcd->pwcd_regionrect;
		rect_passtoparent(screenrect.r_left, screenrect.r_top,
		    &regionrect);
		(void)rect_intersection(&regionrect, &screenrect, &screenrect);
	}
	bounding = pwcd->pwcd_clipping.rl_bound;
	if (pwcd->pwcd_state == PWCD_USERDEFINE) {
	} else {
		struct	rectnode *rn;
		struct	pixwin_prlist *prlnext, *prl;

		pwcd->pwcd_state = PWCD_MULTIRECTS;
		pwcd->pwcd_prmulti = pr_region(pw->pw_pixrect,
		    screenrect.r_left, screenrect.r_top,
		    screenrect.r_width, screenrect.r_height);
		/*
		 * Setup vector clipping stuff.
		 */
		for (rn = pwcd->pwcd_clipping.rl_head;
		    rn; rn = rn->rn_next) {
			prlnext = pwcd->pwcd_prl;
			prl = (struct pixwin_prlist *)
			    sv_calloc(1, sizeof(struct pixwin_prlist));
			prl->prl_pixrect = pr_region(pw->pw_pixrect,
			    screenrect.r_left+rn->rn_rect.r_left,
			    screenrect.r_top+rn->rn_rect.r_top,
			    rn->rn_rect.r_width, rn->rn_rect.r_height);
			prl->prl_next = prlnext;
			prl->prl_x = rn->rn_rect.r_left;
			prl->prl_y = rn->rn_rect.r_top;
			pwcd->pwcd_prl = prl;
		}
		if (pw->pw_prretained) {
		} else
		if (!pwcd->pwcd_clipping.rl_head)  {
			pwcd->pwcd_state = PWCD_NULL;
			pwcd->pwcd_prsingle = pr_region(
			    pw->pw_pixrect,
			    screenrect.r_left, screenrect.r_top, 0, 0);
		} else
		if (pwcd->pwcd_clipping.rl_head ==
		    pwcd->pwcd_clipping.rl_tail) {
			pwcd->pwcd_state = PWCD_SINGLERECT;
			pwcd->pwcd_prsingle = pr_region(
			    pw->pw_pixrect,
			    screenrect.r_left+bounding.r_left,
			    screenrect.r_top+bounding.r_top,
			    bounding.r_width, bounding.r_height);
		}
	}
}

win_getwinclip(windowfd, damaged, winclip, rl)
	int	windowfd;
	bool	damaged;
	struct	winclip *winclip;
	struct	rectlist *rl;
{
	int	err;

	/*
	 * Copy clipping data from kernel
	 */
Again:
	winclip->wc_blockbytes = rlbufbytes;
	winclip->wc_block = rlbuf;
	if (damaged) {
		err = ioctl(windowfd, WINGETDAMAGEDRL, winclip);
	} else {
		err = ioctl(windowfd, WINGETEXPOSEDRL, winclip);
	}
	if (err) {
		extern	errno;

		if (errno == EFBIG) {
			if (rlbuf)
				free(rlbuf);
			rlbufbytes += RLBUFBYTESINC;
			rlbuf = sv_calloc(1, (unsigned)rlbufbytes);
			if (rlbufbytes > MAXRLBUFBYTES)
				(void)werror(err, (damaged)? WINGETDAMAGEDRL:
				    WINGETEXPOSEDRL);
			goto Again;
		}
		(void)werror(err, (damaged)? WINGETDAMAGEDRL: WINGETEXPOSEDRL);
	}
	(void)rl_copy((struct rectlist *)(winclip->wc_block), rl);
		/*
		 * Note this cast should be ok on machines with non-680x0
		 * alignment due to the way that the kernel lays out the
		 * structures in the byte block.
		 */

	(void)rl_sort(rl, rl, RECTS_LEFTTORIGHT);
	(void)rl_sort(rl, rl, RECTS_TOPTOBOTTOM);
}

/*
 * Note: Could change get clipping to alway return the damagedid.
 * See pw_copy and pw_read for explanation of why damagedid of non 0 is bad
 * in those situations (fixup is not guarrenteed).
 */
int
win_getdamagedid(windowfd)
	int	windowfd;
{
	struct	winclip winclip;
	struct	rectlist clipping;

	/*
	 * initialize clipping
	 */
	clipping = rl_null;
	/*
	 * Copy damaged clipping data from kernel
	 */
	win_getwinclip(windowfd, 1, &winclip, &clipping);
	/*
	 * If damage exits then return clipping id
	 */
	if (rl_empty(&clipping))
		return(0);
	else {
		(void)rl_free(&clipping);
		return(winclip.wc_clipid);
	}
}

win_getscreenposition(windowfd, x, y)
	int	windowfd;
	int	*x, *y;
{
	struct	winclip winclip;
	struct	rectlist rl;

	rl = rl_null;
	win_getwinclip(windowfd, 0, &winclip, &rl);
	(void)rl_free(&rl);
	*x = winclip.wc_screenrect.r_left;
	*y = winclip.wc_screenrect.r_top;
}

pwco_reinitclipping(pw)
	struct	pixwin *pw;
{
	int	i;
	struct	pixwin_prlist *prl, *prlnext;

	/*
	 * Reset clipping data
	 */
	(void)rl_free(&pw->pw_clipdata->pwcd_clipping);
	for (i=0;i<RECTS_SORTS;i++)
		(void)rl_free(&pw->pw_clipdata->pwcd_clippingsorted[i]);
	for (prl=pw->pw_clipdata->pwcd_prl;prl;prl = prlnext) {
		(void)pr_destroy(prl->prl_pixrect);
		prlnext = prl->prl_next;
		free((caddr_t)prl);
	}
	pw->pw_clipdata->pwcd_prl = (struct pixwin_prlist *) 0;
	if (pw->pw_clipdata->pwcd_prsingle) {
		(void)pr_destroy(pw->pw_clipdata->pwcd_prsingle);
		pw->pw_clipdata->pwcd_prsingle = (struct pixrect *) 0;
	}
	if (pw->pw_clipdata->pwcd_prmulti) {
		(void)pr_destroy(pw->pw_clipdata->pwcd_prmulti);
		pw->pw_clipdata->pwcd_prmulti = (struct pixrect *) 0;
	}
	(void)rl_free(&pw->pw_fixup);
	pw->pw_clipdata->pwcd_clipid = 0;
	pw->pw_clipdata->pwcd_state = PWCD_NULL;
	pw->pw_opshandle = (caddr_t) pw;
	pw->pw_ops = &pw_opsstd;
	pw->pw_opsx = pw->pw_clipdata->pwcd_x_offset;
	pw->pw_opsy = pw->pw_clipdata->pwcd_y_offset;
	pw->pw_clipdata->pwcd_damagedid = 0;
	return;
}

pw_repairretained(pw)
	struct	pixwin *pw;
{
	struct	pixrect *pr = pw->pw_prretained;
	Rect	*r = &pw->pw_clipdata->pwcd_clipping.rl_bound;

	if (pr == (struct pixrect *)0)
		return;
	/* Preload memory pixrect if swapped out by double XOR on left edge */
	(void)pr_rop(pr, r->r_left, r->r_top, 1, r->r_height,
	    PIX_NOT(PIX_DST), (Pixrect *)0, 0, 0);
	(void)pr_rop(pr, r->r_left, r->r_top, 1, r->r_height,
	    PIX_NOT(PIX_DST), (Pixrect *)0, 0, 0);
	/* Do the repair */
	(void)pw_repair(pw, r);
}

/* Called with r in window coordinate space */
pw_repair(pw, r)
	register Pixwin *pw;
	register Rect *r;
{
	extern	int	pw_dontclipflag;
	int		pw_dontclipflagsaved = pw_dontclipflag;
	Pw_batch_type	typesaved = pw->pw_clipdata->pwcd_batch_type;
	struct	pixrect *prsaved = pw->pw_prretained;
	Rect		r_pw, r_pr;

	if (prsaved == (struct pixrect *)0 || typesaved != PW_NONE)
		return;

	/* Refresh screen from retained pixwin based on rect */
	pw->pw_prretained = (struct pixrect *)0;
	/* Be sure to clip source */
	pw_dontclipflag = 0;
	/* Restore from pixrect */
	rect_construct(&r_pw, PW_X_FROM_WIN(pw, r->r_left),
	    PW_Y_FROM_WIN(pw, r->r_top), r->r_width, r->r_height);
	(void)pw_write(pw, r_pw.r_left, r_pw.r_top, r_pw.r_width, r_pw.r_height,
	    PIX_SRC, prsaved, r_pw.r_left, r_pw.r_top);
	/* Reset clipping state */
	pw_dontclipflag = pw_dontclipflagsaved;
	pw->pw_prretained = prsaved;
	pw->pw_clipdata->pwcd_batch_type = typesaved;

	/* Clear part repaired that didn't have pixrect under it */
	rect_construct(&r_pr, 0, 0, prsaved->pr_width, prsaved->pr_height);
	/* Do quick test to see if anything needs to by cleared */
	if (!rect_includesrect(&r_pr, &r_pw)) {
		Rectlist rl_dif;
		register struct rectnode *rn;

		/* Need to determine exactly what to clear */
		(void)rl_initwithrect(&r_pw, &rl_dif);
		(void)rl_rectdifference(&r_pr, &rl_dif, &rl_dif);
		/* Clear each rect in rl_dif */
		for (rn = rl_dif.rl_head; rn; rn = rn->rn_next) {
			(void)pw_writebackground(pw, rn->rn_rect.r_left,
			    rn->rn_rect.r_top, rn->rn_rect.r_width,
			    rn->rn_rect.r_height, PIX_CLR);
		}
		(void)rl_free(&rl_dif);
	}
}

int
pwo_nop()
{
	return (0);
}
