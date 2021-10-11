#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)pw_shared.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Pw_shared.c: Use shared memory for faster locking.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_cursor.h>	
#include <sunwindow/win_lock.h>
#include <sunwindow/cursor_impl.h>	/* for crosshairs active check */


static void		pw_do_shared_lock(), pw_do_shared_unlock();


#define	pw_release_lock(which_lock)	\
	{ \
    		(which_lock)->count = 0; \
    		(which_lock)->pid = 0; \
    		(which_lock)->lock = 0; \
	}


typedef struct pw_dtop_entry {
	char		dtop_name[SCR_NAMESIZE];
	int		ref_count;
	Win_lock_block	*shared_info;
} Pw_dtop_entry;

/* set this to TRUE to turn off the shared locking */
/* initialize pw_disable_shared_locking to 0. */
int			pw_disable_shared_locking = 0;

static Pw_dtop_entry	pw_dtop_table[WIN_LOCK_MAX_DTOP];
static int		pw_lock_block_bytes;
static int		pw_mypid;


/* return TRUE if lock already taken */
#define	pw_get_lock(which_lock)		\
    ((pw_test_set(&(which_lock)->lock)) ? \
	TRUE : ((which_lock)->id++, (which_lock)->count = 1, \
		(which_lock)->pid = pw_mypid, FALSE))


pw_shared_lock(pw, rect_paint)
	register Pixwin *pw;
	Rect *rect_paint;
{
	register Win_lock_block 	*wl = pw->pw_clipdata->pwcd_wl;

	/*
	 * Cursors, 24-bit planegroups, and shared locking don't currently
	 * mix too well.
	 */
	if (pw->pw_clipdata->pwcd_plane_groups_available[PIXPG_24BIT_COLOR])
	    return(FALSE);

	/*
	 * if double buffering is on, we do not want to use the shared
	 * memory lock or else pw_rop and friends will behave abnormally
	 * (for cg{3,5} only)
	 */
	if ((pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL) &&
	    !pw->pw_clipdata->pwcd_plane_groups_available[PIXPG_24BIT_COLOR] &&
	    pw_dbl_rect(pw))
	  return(FALSE);

	/*
	 * See if locked already.  Assume that don't get here if this
	 * pixwin has the display lock already.
	 * Grabio and window data locks take precedence over display lock.
	 */
	if (!wl || pw_get_lock(&wl->mutex))
		return FALSE;

	/*
	 * Don't do own locking if io is grabbed.  This is because we
	 * aren't prepared to adequately track the cursor when the
	 * pixwin has multiple plane groups available.  We could beef
	 * this up to check to see if multiple available planes are
	 * present before we go to pw_release_lock.
	 * Also, bail out if going to to be taking cursor down
	 * that was put up over another plane group.
	 */
	if (wl->waiting || wl->go_to_kernel || win_lock_grabio_locked(wl) ||
	    (win_lock_data_locked(wl) && wl->data.pid != pw_mypid) ||
	    (win_lock_display_locked(wl) && wl->display.pid != pw_mypid) ||
	    (pw->pw_clipdata->pwcd_plane_group != wl->cursor_info.plane_group)){
		pw_release_lock(&wl->mutex);
		return FALSE;
	}

	/* See if pixwin clip data out-of-date or xhairs involved */
	if ((pw->pw_clipdata->pwcd_clipid !=
	    wl->clip_ids[pw->pw_clipdata->pwcd_winnum]) ||
	    show_horiz_hair(&wl->cursor_info.cursor) || 
	    show_vert_hair(&wl->cursor_info.cursor)) {
		/* Since have to call kernel to get clipping ... */
		pw_release_lock(&wl->mutex);
		return FALSE;
	}

	/* acquire display lock */
	if (win_lock_display_locked(wl)) {
		/* already locked, so bump the count */
		wl->display.count++;
	} else if (pw_get_lock(&wl->display)) {
		/* locked already (somehow) */
		pw_release_lock(&wl->mutex);
		return FALSE;
	}

	pw_do_shared_lock(pw, rect_paint);
	pw_release_lock(&wl->mutex);
	return TRUE;
}

static void
pw_do_shared_lock(pw, rect_paint)
	register Pixwin *pw;
	Rect *rect_paint;
{
	Win_lock_block 	*wl = pw->pw_clipdata->pwcd_wl;
	register Win_shared_cursor	*shared_cursor = &wl->cursor_info;
	int				original_planes;
	int				planes;
#ifdef ecd.cursor
        short                           olddata;
#endif ecd.cursor
	
	/*
	 * See if cursor is active, up on the screen and intersects
	 * area that are going to paint in. If a cursor plane exists -
	 * i.e spare3 is set, you dont have to paint it in
	 */
	if (shared_cursor->cursor_active && shared_cursor->cursor_is_up &&
	    pw_cursor_intersect_exposed(pw, rect_paint) &&
		(!shared_cursor->spare3)) {
		/* Take cursor off of the screen */
		if (pw->pw_clipdata->pwcd_flags & PWCD_CURSOR_INVERTED)
		    (void)pr_reversevideo(pw->pw_pixrect, 0, 1);
		win_lock_prepare_screen_pr(shared_cursor);
		/* Set up full planes */
		(void)pr_getattributes(pw->pw_pixrect, &original_planes);
		planes = PIX_ALL_PLANES;
#ifdef  planes_fully_implemented 
		pr_putattributes(pw->pw_pixrect, &planes);
#else
		(void)pw_full_putattributes(pw->pw_pixrect, &planes);
#endif  planes_fully_implemented

#ifdef ecd.cusror
                /* This fix is mainly for MS windows which runs in
                 * reverse video.  When we rop from the MS win pixrect
                 * we would use a reverse rop. However, the kernel uses
                 * the shared memory pixrect when it moves the cursor
                 * and doesn't know we rop'd from a reverse video window.
                 * The kernel just uses the display pixrect which has the
                 * attributes that were used when sunview was started.
                 * The fix is to clear the window pixrect MP_REVERSEVIDEO
                 * flag while we rop into the memory pixrect.  The kernel
                 * also has some similar code to fix the situation where
                 * sunview was started with -i (inverse). (See sys/
                 * sunwindowdev/windt.c)
                 */
        
 
                olddata = ((struct mpr_data *)(pw->pw_pixrect->pr_data))->md_flags;
                ((struct mpr_data *)(pw->pw_pixrect->pr_data))->md_flags &= ~MP_REVERSEVIDEO;
#endif ecd.cursor

		(void)pr_rop(pw->pw_pixrect, shared_cursor->x, shared_cursor->y,
		    shared_cursor->screen_pr.pr_width,
		    shared_cursor->screen_pr.pr_height,
		    PIX_SRC, &shared_cursor->screen_pr, 0, 0);

#ifdef ecd.cursor
                ((struct mpr_data *)(pw->pw_pixrect->pr_data))->md_flags = olddata;
#endif ecd.cursor

		/* Reset planes */
#ifdef  planes_fully_implemented 
		pr_putattributes(pw->pw_pixrect, &original_planes);
#else
		(void)pw_full_putattributes(pw->pw_pixrect, &original_planes);
#endif  planes_fully_implemented
		if (pw->pw_clipdata->pwcd_flags & PWCD_CURSOR_INVERTED)
		    (void)pr_reversevideo(pw->pw_pixrect, 0, 1);
		shared_cursor->cursor_is_up = FALSE;
	}
#ifdef DEBUG
	printf("locked display\n");
#endif
}


pw_shared_unlock(pw)
	register Pixwin *pw;
{
	register Win_lock_block 	*wl = pw->pw_clipdata->pwcd_wl;
	register Win_shared_cursor	*shared_cursor;
	

	/*
	 * Cursors, 24-bit planegroups, and shared locking don't currently
	 * mix too well.
	 */
	if (pw->pw_clipdata->pwcd_plane_groups_available[PIXPG_24BIT_COLOR])
	    return(FALSE);

	/*
	 * if double buffering is on, we do not want to use the shared
	 * memory unlock or else pw_rop and friends will behave abnormally
	 * (for cg{3,5} only)
	 */
	if ((pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL) &&
	    !pw->pw_clipdata->pwcd_plane_groups_available[PIXPG_24BIT_COLOR] &&
	    pw_dbl_rect(pw))
	  return(FALSE);

	if (!wl || pw_get_lock(&wl->mutex))
		return FALSE;

	shared_cursor = &wl->cursor_info;

	/* See if we have it locked, no one waiting for 
	 * lock and no xhairs.  Also, bail out if going to
	 * to be putting cursor up over another plane group.
	 *
	 * Also, don't do own locking if io is grabbed.  This is because we
	 * aren't prepared to adequately track the cursor when the
	 * pixwin has multiple plane groups available.
	 */
	if (wl->waiting || wl->go_to_kernel || win_lock_grabio_locked(wl) ||
	    wl->status.new_cursor ||
	    !win_lock_display_locked(wl) || (wl->display.pid != pw_mypid) ||
	    (pw->pw_clipdata->pwcd_plane_group != wl->mouse_plane_group) ||
	    show_horiz_hair(&shared_cursor->cursor) || 
	    show_vert_hair(&shared_cursor->cursor)) {
	    pw_release_lock(&wl->mutex);
	    return FALSE;
	}

	wl->display.count--;

	if (wl->display.count <= 0) {
	    /* Release the lock */
	    pw_do_shared_unlock(pw);
	    pw_release_lock(&wl->display);
	}

	/* release the mutex lock */
	pw_release_lock(&wl->mutex);
	return TRUE;
}

static void
pw_do_shared_unlock(pw)
	register Pixwin *pw;
{
	register Win_lock_block 	*wl = pw->pw_clipdata->pwcd_wl;
	register Win_shared_cursor	*shared_cursor = &wl->cursor_info;
	int				original_planes;
	int				planes;
#ifdef ecd.cursor
        short                           olddata;
        struct mpr_data                 *mpds, *mpdd;
#endif ecd.cursor
	
#ifdef DEBUG
	printf("unlocked display\n");
#endif
	/* only draw the cursor if not up, active and want it or
	* we dont have cursor planes
	*/
	if ((shared_cursor->spare3)|| (shared_cursor->cursor_is_up) ||
	    (!shared_cursor->cursor_active) ||
	    (!(show_cursor(&shared_cursor->cursor) &&
	       shared_cursor->pr_data.md_image && 
	       shared_cursor->pr.pr_width &&
	       shared_cursor->pr.pr_depth && 
	       shared_cursor->pr.pr_height)))
		    return;
		
	/* Set up full planes */
	(void)pr_getattributes(pw->pw_pixrect, &original_planes);
	planes = PIX_ALL_PLANES;
#ifdef  planes_fully_implemented 
	pr_putattributes(pw->pw_pixrect, &planes);
#else
	(void)pw_full_putattributes(pw->pw_pixrect, &planes);
#endif  planes_fully_implemented

	/*
	 * Get current mouse position.
	 * Race with kernel but not critical.
	 */
	shared_cursor->x = wl->mouse_x;
	shared_cursor->y = wl->mouse_y;

	/* Note what plane group using */
	shared_cursor->plane_group = pw->pw_clipdata->pwcd_plane_group;

	/* adjust for the hot spot */
	shared_cursor->x -= shared_cursor->cursor.cur_xhot;
	shared_cursor->y -= shared_cursor->cursor.cur_yhot;

	if (pw->pw_clipdata->pwcd_flags & PWCD_CURSOR_INVERTED)
	    (void)pr_reversevideo(pw->pw_pixrect, 0, 1);

	/* save the bits under the cursor */
	shared_cursor->screen_pr.pr_width = shared_cursor->pr.pr_width;
	shared_cursor->screen_pr.pr_height = shared_cursor->pr.pr_height;
	win_lock_prepare_screen_pr(shared_cursor);
#ifdef ecd.cursor
        olddata = ((struct mpr_data *)(pw->pw_pixrect->pr_data))->md_flags;
        ((struct mpr_data *)(pw->pw_pixrect->pr_data))->md_flags &= ~MP_REVERSEVIDEO;
#endif ecd.cursor
	(void)pr_rop(&shared_cursor->screen_pr, 0, 0,
	    shared_cursor->screen_pr.pr_width,
	    shared_cursor->screen_pr.pr_height,
	    PIX_SRC, pw->pw_pixrect, shared_cursor->x, shared_cursor->y);
#ifdef ecd.cursor 
        ((struct mpr_data *)(pw->pw_pixrect->pr_data))->md_flags = olddata;
#endif ecd.cursor 

	/*
	 * Reset planes so that will write to foreground and background
	 * of current colormap segment.
	 */
#ifdef  planes_fully_implemented 
	pr_putattributes(pw->pw_pixrect, &original_planes);
#else
	(void)pw_full_putattributes(pw->pw_pixrect, &original_planes);
#endif  planes_fully_implemented

	/* Put cursor on the screen */
	win_lock_prepare_cursor_pr(shared_cursor);
	(void)pr_rop(pw->pw_pixrect, shared_cursor->x, shared_cursor->y,
	    shared_cursor->screen_pr.pr_width,
	    shared_cursor->screen_pr.pr_height,
	    shared_cursor->cursor.cur_function, &shared_cursor->pr, 0, 0);
	shared_cursor->cursor_is_up = TRUE;

	if (pw->pw_clipdata->pwcd_flags & PWCD_CURSOR_INVERTED)
	    (void)pr_reversevideo(pw->pw_pixrect, 0, 1);

}


static int
pw_cursor_intersect_exposed(pw, rect_paint)
	register Pixwin	*pw;
	Rect		*rect_paint;
{
    Win_lock_block 	*shared_info = pw->pw_clipdata->pwcd_wl;
    Win_shared_cursor 	*cursor_info = &shared_info->cursor_info;
    Rect		rect;
    int			have_grabio;

    /* if the data lock is ours, we can't trust the
     * screen_x, screen_Y coordinates of the window, since
     * a win_setrect() could have been done without changing
     * the clipping id.  So play it safe and take down the cursor.
     */
    if (win_lock_data_locked(shared_info) && 
        (shared_info->data.pid == pw_mypid))
	return TRUE;

    rect_construct(&rect, cursor_info->x, cursor_info->y,
	cursor_info->pr.pr_width, cursor_info->pr.pr_height);

    /* transform to window space */
    rect_passtochild(pw->pw_clipdata->pwcd_screen_x, 
	pw->pw_clipdata->pwcd_screen_y, &rect);

    /*
     * Since the clipping list is in the space of the pixwin,
     * transform the window-space cursor rect to region space.
     */
    if (pw->pw_clipdata->pwcd_regionrect)
	rect_passtochild(pw->pw_clipdata->pwcd_regionrect->r_left,
	    pw->pw_clipdata->pwcd_regionrect->r_top, &rect);

    /* cursor must be on the intended paint area to intersect.
     * Also, either this process must have the grabio lock or
     * the cursor must be in the exposed rectlist of pw.
     */
    have_grabio = win_lock_grabio_locked(shared_info) && 
	 (shared_info->grabio.pid == pw_mypid);

    return (rect_intersectsrect(&rect, rect_paint) && 
	    (have_grabio || 
	     rl_rectintersects(&rect, &pw->pw_clipdata->pwcd_clipping)));
}

/* Defining MAP_FIXED is for compiling under 3.4 environment */
#ifndef MAP_FIXED
#define MAP_FIXED	0
#endif

Win_lock_block *
pw_get_lock_block(windowfd)
	int	windowfd;
{
    struct screen		screen;
    register char		*dtop_name = screen.scr_fbname;
    register int		i;
    register Pw_dtop_entry	*entry;
    Pw_dtop_entry		*free_entry = 0;
    Win_lock_block		*shared_info;
    static Win_lock_block	*last_valloc; /* = 0 */
    static int			wrong_version; /* = 0 */
    char *strcpy();

    if (wrong_version || pw_disable_shared_locking)
	return 0;

    pw_mypid = getpid();

    (void)win_screenget(windowfd, &screen);

    /* see if this dtop is already mapped in */

    for (i = 0, entry = &pw_dtop_table[i]; i < WIN_LOCK_MAX_DTOP; 
	 i++, entry = &pw_dtop_table[i])
	if (!entry->shared_info) {
	    if (!free_entry)
		free_entry = entry;
	} else if (strcmp(dtop_name, entry->dtop_name) == 0) {
	    entry->ref_count++;
	    return entry->shared_info;
	}
    
    /* not already in the table, so there better be a
     * free slot.
     */
    if (!free_entry)
	return 0;

    /* there is a free slot, so mmap the shared info */

    if (!pw_lock_block_bytes)
	pw_lock_block_bytes = round_to_page(sizeof(Win_lock_block));    

    /* Note that here we mmap() using the windowfd and that
     * other windows will share this mmaped memory.
     * If close(windowfd) is changed to munmap() the storage,
     * the other windows will be left pointing to invalid storage.
     */
    if (last_valloc)
	shared_info = last_valloc;
    else {
        extern char *valloc();
	
	shared_info = (Win_lock_block *) valloc((u_int)pw_lock_block_bytes);
    }
    if ((int)mmap((caddr_t)shared_info, pw_lock_block_bytes,
	PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED, windowfd, 0) == -1) {
	last_valloc = shared_info;
	return 0;
    }

    /* if the kernel has a different version number,
     * don't use the shared memory.
     */
    if (shared_info->version != WIN_LOCK_VERSION) {
	wrong_version = TRUE;
	return 0;
    }

    last_valloc = 0;

    entry = free_entry;
    entry->shared_info = shared_info;
    (void) strcpy(entry->dtop_name, dtop_name);
    entry->ref_count = 1;

    return shared_info;
}


void
pw_free_lock_block(shared_info)
	register Win_lock_block	*shared_info;
{

    if (!shared_info)
	return;

/* Don't free up the shared memory, since this is probably
 * no big win (and a waste of cycles).

    for (i = 0, entry = &pw_dtop_table[i]; i < WIN_LOCK_MAX_DTOP; 
	 i++, entry = &pw_dtop_table[i])
	if (shared_info == entry->shared_info) {
	    entry->ref_count--;
	    if (entry->ref_count <= 0) {
		munmap(shared_info, pw_lock_block_bytes);
		free(shared_info);
	    }
	    return;
	}
*/
}

/* Update process specific data */
pw_fork()
{
    pw_mypid = getpid();
}

static int
round_to_page(bytes)
        register int    bytes;
{
        register int    pagesize = getpagesize();
        register int    roundup = pagesize-(bytes%pagesize);

        if (roundup != pagesize)
                bytes += roundup;
        return(bytes);
}


pw_reset_lock(pw)
	Pixwin	*pw;
{
	Win_lock_block	*wl = pw->pw_clipdata->pwcd_wl;

	if (!wl)
		return;

	pw_release_lock(&wl->mutex);
	pw_release_lock(&wl->display);
}


/* turn shared locking on or off */
pw_set_shared_locking(on)
	int	on;
{
	/* pw_disable_shared_locking = !on; */
}
