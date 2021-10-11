#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)pw_dbl.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Pw_dbl.c: Implement double buffering 
 */
#include <varargs.h>
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
#include <sunwindow/pw_dblbuf.h>
#include <pixrect/pr_dblbuf.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_lock.h>
#include <sun/fbio.h>

int pw_dbl_get();
void pw_dbl_access();
void pw_dbl_release();
void pw_dbl_flip();
int pw_dbl_set();


/*
 * This contains the rect object of the double buffering window if active
 * If not, its value is NULL.  It is also set to NULL in fullscreen_destroy()
 * used in fullscreen_init(), fullscreen_destroy() and in this file.
 */
struct rect *pw_fullscreen_dbl_rect;

/* Request for access to the double buffering */
void 
pw_dbl_access(pw)
	Pixwin *pw;
{
	/*
	 * window id support of double buffering means multiple window
	 * support of double buffering and we can avoid the painful
	 * board-state and window-state management that the kernel goes
	 * through attempting to keep only one window actively double
	 * buffering.
	 */

	if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL) {

	    if (pw->pw_clipdata->pwcd_flags & PWCD_WID_DBL &&
	       (pw->pw_pixrect->pr_depth==8 || pw->pw_pixrect->pr_depth==32)){

		register struct	pixwin_prlist *prl;
		register struct pixwin_clipdata *pwcd = pw->pw_clipdata;
		/*
		 * if this canvas already has a unique wid, then avoid
		 * asking again and aviod the wid plane group preparation.
		 */

		if (pw->pw_clipdata->pwcd_flags & PWCD_WID_ALLOCATED) {
		    pw_dbl_set(pw, PW_DBL_WRITE, PW_DBL_BACK,
			PW_DBL_READ, PW_DBL_BACK, /*PW_DBL_ACCESS, 1,*/ 0);
		} else {
		    struct fb_wid_dbl_info wid_dbl_info;
		    int                 orig_p, orig_pg;

		    /* let pixrect know we're a sunview canvas */
		    pr_ioctl(pw->pw_pixrect, FBIOSWINFD,
			&pw->pw_clipdata->pwcd_windowfd);
		    if (pw->pw_clipdata->pwcd_prmulti)
			pr_ioctl(pw->pw_clipdata->pwcd_prmulti, FBIOSWINFD,
			    &pw->pw_clipdata->pwcd_windowfd);
/* ###
		    if (pw->pw_clipdata->pwcd_prsingle)
			pr_ioctl(pw->pw_clipdata->pwcd_prsingle, FBIOSWINFD,
			    &pw->pw_clipdata->pwcd_windowfd);
*/
		    for (prl = pwcd->pwcd_prl;prl;prl = prl->prl_next)
			pr_ioctl(prl->prl_pixrect, FBIOSWINFD, 
				&pw->pw_clipdata->pwcd_windowfd);

		    /* allocate a unique wid */
		    ioctl(pw->pw_windowfd, WINWIDGET, &wid_dbl_info);
		    wid_dbl_info.dbl_wid.wa_type =
			(pw->pw_pixrect->pr_depth == 8)
			    ? FB_WID_DBL_8 : FB_WID_DBL_24;
		    wid_dbl_info.dbl_wid.wa_index = -1;
		    wid_dbl_info.dbl_wid.wa_count = 1;

		    /*
		     * if window-ids are used up or not supported then punt
		     * and don't double buffer the application
		     */

		    if (pr_ioctl(pw->pw_pixrect, FBIO_WID_ALLOC,
			&wid_dbl_info.dbl_wid) < 0) {
			/*
			 * inefficient but nicer looking on 80 wide window.
			 * style in everything we do?
			 */
			fprintf(stderr, "pw_dbl_access: frame buffer out of ");
			fprintf(stderr, "unique window identifiers or double");
			fprintf(stderr, " buffering not supported for the cu");
			fprintf(stderr, "rrent plane group.\n");
			return;
		    }

		    pw->pw_clipdata->pwcd_flags |= PWCD_WID_ALLOCATED;
		    wid_dbl_info.dbl_read_state = PW_DBL_BACK;
		    wid_dbl_info.dbl_write_state = PW_DBL_BACK;
		    ioctl(pw->pw_windowfd, WINWIDSET, &wid_dbl_info);
		    pw_dbl_display(pw, PR_DBL_A, PR_DBL_B);

		    /* reprepare the wid planegroup surface */
		    pr_getattributes(pw->pw_pixrect, &orig_p);
		    orig_pg = pr_get_plane_group(pw->pw_pixrect);
		    pw_set_planes_directly(pw, PIXPG_WID, PIX_ALL_PLANES);
		    pw_rop(pw, 0, 0, pw->pw_pixrect->pr_width,
			pw->pw_pixrect->pr_height,
			PIX_SRC | PIX_COLOR(wid_dbl_info.dbl_wid.wa_index),
			(Pixrect *) 0, 0, 0);
		    pw_set_planes_directly(pw, orig_pg, orig_p);
		}
	    } else
		(void) werror(ioctl(pw->pw_windowfd, WINDBLACCESS, 0),
		    WINDBLACCESS);

	    pw->pw_clipdata->pwcd_flags |= PWCD_DBLACCESSED;
	}
}

/* Release the double buffer accessing */
#include <pixrect/pr_planegroups.h>

void
pw_dbl_release(pw)
struct pixwin  *pw;
{
    if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL &&
			pw->pw_clipdata->pwcd_flags & PWCD_WID_DBL) {
	/*
	 * Some applications may depend on release doing a front to back
	 * copy of the buffers.  This is actually no longer necessary with
	 * window ids and causes a significant performance penalty.  Thus,
	 * by default we won't do it but allow it to be turned on if necessary.
	 * A backward compatibility hack that should go away.
	 */
	if (pw->pw_clipdata->pwcd_flags & PWCD_COPY_ON_DBL_RELEASE) {

	    pw_dbl_set(pw,PW_DBL_READ,PW_DBL_FORE,PW_DBL_WRITE,PW_DBL_BACK,0);
	    pw_rop(pw, 0, 0,
		pw->pw_pixrect->pr_width, pw->pw_pixrect->pr_height, PIX_SRC,
		pw->pw_pixrect,
		pw->pw_clipdata->pwcd_screen_x, pw->pw_clipdata->pwcd_screen_y);
	}
	pw_dbl_set(pw, PW_DBL_READ, PW_DBL_FORE, PW_DBL_WRITE, PW_DBL_FORE, 0);
	pw->pw_clipdata->pwcd_flags &= ~PWCD_DBLACCESSED;

    } else if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL) {

	if (pw->pw_clipdata->pwcd_plane_groups_available[PIXPG_24BIT_COLOR]) {

	    pw_dbl_set(pw, PW_DBL_READ, PW_DBL_FORE, PW_DBL_WRITE, PW_DBL_BACK,
		 0);
	    pw_rop(pw, 0, 0,
		pw->pw_pixrect->pr_width, pw->pw_pixrect->pr_height, PIX_SRC,
		pw->pw_pixrect,
		pw->pw_clipdata->pwcd_screen_x, pw->pw_clipdata->pwcd_screen_y);
	}

	pw->pw_clipdata->pwcd_flags &= ~PWCD_DBLACCESSED;
	(void) werror(ioctl(pw->pw_clipdata->pwcd_windowfd, WINDBLRLSE, 0),
	    WINDBLRLSE);
    }
}

/* Flip the write control bits */
void 
pw_dbl_flip(pw)
	Pixwin	*pw;
{
	Rect	rect;

	if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL &&
	    pw->pw_clipdata->pwcd_flags & PWCD_DBLACCESSED &&
	    pw->pw_clipdata->pwcd_flags & PWCD_WID_DBL)
	{
	    struct fb_wid_dbl_info wid_dbl_info;

	    ioctl(pw->pw_windowfd, WINWIDGET, &wid_dbl_info);
	    pw_dbl_display(pw, wid_dbl_info.dbl_back, wid_dbl_info.dbl_fore);
	}
	else
	{
	    rect.r_width = rect.r_height = 1;
	    pw_lock(pw, &rect);
	    if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL &&
		pw->pw_clipdata->pwcd_flags & PWCD_DBLACCESSED)
		(void)werror(ioctl(pw->pw_windowfd, WINDBLFLIP, 0), WINDBLFLIP);
	    pw_unlock(pw);
	}
}



/* pw_dbl_get returns the current value of which_attr. */
int
pw_dbl_get(pw, which_attr)
	struct pixwin		*pw;
	Pw_dbl_attribute		which_attr;
{
	struct pwset list;

	if (which_attr == PW_DBL_AVAIL)
		/* return PW_DBL_EXISTS if true else return FALSE */
		return ((pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL)? PW_DBL_EXISTS:
			FALSE);
	if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL) {

		struct fb_wid_dbl_info wid_dbl_info;

		ioctl(pw->pw_windowfd, WINWIDGET, &wid_dbl_info);
		switch (which_attr) {
		    case PW_DBL_DISPLAY: 
			if (pw->pw_clipdata->pwcd_flags & PWCD_WID_DBL)
			    return (wid_dbl_info.dbl_fore);
			list.attribute = PR_DBL_DISPLAY;
		    break;
		    case PW_DBL_WRITE: 
			if (pw->pw_clipdata->pwcd_flags & PWCD_WID_DBL) 
			    return ((wid_dbl_info.dbl_wid.wa_count == 0) ?
			    PW_DBL_BOTH : wid_dbl_info.dbl_write_state);
			list.attribute = PR_DBL_WRITE;
		    break;
		    case PW_DBL_READ:
			if (pw->pw_clipdata->pwcd_flags & PWCD_WID_DBL) 
			    return (wid_dbl_info.dbl_read_state);
		    break;
		    default:
			return(PW_DBL_ERROR);
		}
		(void)werror(ioctl(pw->pw_windowfd, WINDBLGET, &list),
			     WINDBLGET);
		return(list.value);
	} else
		return(PW_DBL_ERROR);

} /* pw_dbl_get */


/* Set double buffer control bits */
int
pw_dbl_set(pw, va_alist)
struct pixwin      *pw;
va_dcl
{
    va_list             valist;
    Attr_avlist         avlist;
    caddr_t             vlist[ATTR_STANDARD_SIZE];
    struct pwset        pw_setlist;
    Pw_dbl_attribute    which_attr;


   if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL) {

	struct fb_wid_dbl_info wid_dbl_info;
	register struct	pixwin_prlist *prl;
	register struct pixwin_clipdata *pwcd = pw->pw_clipdata;
	int wdbl_flag = -1;

	ioctl(pw->pw_windowfd, WINWIDGET, &wid_dbl_info);

	va_start(valist);
	avlist = attr_make(vlist, ATTR_STANDARD_SIZE, valist);
	va_end(valist);
	while (which_attr = (Pw_dbl_attribute) * avlist++) {

	    if (which_attr == PW_DBL_DISPLAY)
		return (PW_DBL_ERROR);
	    else {
		pw_setlist.value = (int) *avlist++;
		if (which_attr == PW_DBL_READ) {
		    pw_setlist.attribute = PR_DBL_READ;
		    if (pw->pw_clipdata->pwcd_flags & PWCD_WID_DBL) {
                      wid_dbl_info.dbl_read_state = pw_setlist.value;
		    }
		} else if (which_attr == PW_DBL_WRITE) {
		    pw_setlist.attribute = PR_DBL_WRITE;
                    if (pw->pw_clipdata->pwcd_flags & PWCD_WID_DBL) {
		      wid_dbl_info.dbl_write_state = pw_setlist.value;
		    }
		} 
		else
		    return (PW_DBL_ERROR);
		}
	}
	if (pw->pw_clipdata->pwcd_flags & PWCD_WID_DBL) {
	    ioctl(pw->pw_windowfd, WINWIDSET, &wid_dbl_info);

	    wdbl_flag = 1;
	    pr_ioctl(pw->pw_pixrect, FBIO_WID_DBL_SET, &wdbl_flag);
	    
	    wdbl_flag = 0;
	    if (pw->pw_clipdata->pwcd_prmulti)
		pr_ioctl(pw->pw_clipdata->pwcd_prmulti, 
				FBIO_WID_DBL_SET, &wdbl_flag);
/*
	    if (pw->pw_clipdata->pwcd_prsingle)
                pr_ioctl(pw->pw_clipdata->pwcd_prsingle,
                                FBIO_WID_DBL_SET, &wdbl_flag);
*/
	    for (prl = pwcd->pwcd_prl;prl;prl = prl->prl_next)
		pr_ioctl(prl->prl_pixrect, FBIO_WID_DBL_SET, &wdbl_flag); 
	} else
	    (void) werror(ioctl(pw->pw_windowfd, WINDBLSET,
				&pw_setlist), WINDBLSET);
	    
	return (TRUE);
    } else
	return (FALSE);
}



/*
 * returns the rect of the window currently doing double buffering
 */

struct rect *pw_dbl_rect(pw)
	struct pixwin	*pw;
{
  static struct rect pw_dbl_rect_object;

  bzero(&pw_dbl_rect_object, sizeof pw_dbl_rect_object);
  if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL) {
    (void)werror(ioctl(pw->pw_windowfd, WINDBLCURRENT,
		       &pw_dbl_rect_object), WINDBLCURRENT);
  }
  return (pw_dbl_rect_object.r_width || pw_dbl_rect_object.r_height
	  ?  &pw_dbl_rect_object : (struct rect *) NULL);
}


/*
 * This function is used to check wether a line/vector intersects the
 * rect performing double buffering  (used in pw_cms.c only).
 */

bool
pw_dbl_intersects_vector(pw,x0,y0,x1,y1)
     struct pixwin *pw;
     int x0,y0,x1,y1;
{
  if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL
      && pw_fullscreen_dbl_rect)
    return (rect_clipvector(pw_fullscreen_dbl_rect,&x0,&y0,&x1,&y1));
  return FALSE;
}


/*
 * This function is used to check wether a region/rect intersects the
 * rect performing double buffering (used in pw_cms.c only).
 */

bool
pw_dbl_intersects_region(pw,x,y,w,h)
     struct pixwin *pw;
     int x,y,w,h;
{
  struct rect rct;

  if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL
      && pw_fullscreen_dbl_rect){
    rect_construct(&rct,x,y,w,h);
    return(rect_intersectsrect(pw_fullscreen_dbl_rect, &rct));
  }
  return FALSE;
}

pw_dbl_display(pw, fore, back)
struct pixwin      *pw;
unsigned int        fore, back;
{
    struct fb_wid_dbl_info wid_dbl_info;
    register struct	pixwin_prlist *prl;
    register struct pixwin_clipdata *pwcd = pw->pw_clipdata;
    int	    wdbl_flag;

    ioctl(pw->pw_windowfd, WINWIDGET, &wid_dbl_info);
    wid_dbl_info.dbl_fore = fore;
    wid_dbl_info.dbl_back = back;
    ioctl(pw->pw_windowfd, WINWIDSET, &wid_dbl_info);

    wdbl_flag = 1;
    pr_ioctl(pw->pw_pixrect, FBIO_WID_DBL_SET, &wdbl_flag);
    
    wdbl_flag = 0;
    if (pw->pw_clipdata->pwcd_prmulti)     
	      pr_ioctl(pw->pw_clipdata->pwcd_prmulti, FBIO_WID_DBL_SET, 
						    &wdbl_flag);
    for (prl = pwcd->pwcd_prl;prl;prl = prl->prl_next)
	pr_ioctl(prl->prl_pixrect, FBIO_WID_DBL_SET, &wdbl_flag);
 
    pr_dbl_set(pw->pw_pixrect, PR_DBL_DISPLAY, fore, 0);
}
