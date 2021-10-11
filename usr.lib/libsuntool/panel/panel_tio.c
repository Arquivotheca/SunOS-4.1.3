#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)panel_tio.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*****************************************************************************/
/*                          panel_tio.c                                      */
/*            Copyright (c) 1985 by Sun Microsystems, Inc.                   */
/*****************************************************************************/

#include <varargs.h>
#include <suntool/panel_impl.h>
#include <sunwindow/sun.h>

static panel_selected(), panel_handle_sigwinch();


/******************* timer for blinking caret ********************************/

#define	TIMER_EXPIRED(timer)	 ((*(timer)) && ((*(timer))->tv_sec == 0) &&\
				  ((*(timer))->tv_usec == 0))
				  
#define	TIMER_OFF(timer)	*(timer) = (struct timeval *) NULL

#define	TIMER_ON(timer, def)	*(timer) = &(def)

#define	TIMER_SET(timer, init)	**(timer) = init

/*****************************************************************************/
/* panel_create                                                              */
/*****************************************************************************/

/* VARARGS1 */
Toolsw *
panel_create(tool, va_alist)
Tool 		*tool;
va_dcl
{
   caddr_t		avlist[ATTR_STANDARD_SIZE];
   va_list		valist;
   register Toolsw 	*tsw;

   va_start(valist);
   if (!attr_make(avlist, ATTR_STANDARD_SIZE, valist)) {
      va_end(valist);
      return NULL;
   }

   va_end(valist);
   if (tsw = panel_create_sw(tool, FALSE, avlist)) {
      /* setup the subwindow procs */
      tsw->ts_io.tio_handlesigwinch = panel_handle_sigwinch;
      tsw->ts_io.tio_selected       = panel_selected;
      tsw->ts_destroy               = panel_free;
   }
   return tsw;
}

/*****************************************************************************/
/* panel_selected                                                            */
/*****************************************************************************/

static
panel_selected(panel, ibits, obits, ebits, timer)
panel_handle	 panel;
int		*ibits, *obits, *ebits;
struct timeval **timer;
{
   int                	fd;
   Event  		event;

   fd = panel->windowfd;
   *ibits = *obits = *ebits = 0;

   if (TIMER_EXPIRED(timer)) 
   {
      if (blinking(panel))
	 panel_caret_invert(panel);

      /* notify the client about the timer */
      if (panel->timer_proc)
	 (*panel->timer_proc)(panel);

      TIMER_SET(timer, panel->timer_full);
      return;
   }

   if (input_readevent(fd, &event) == -1) {
      perror("panel input failed");
      abort();
   }

   switch (event_action(&event)) {
      case KBD_USE:
	 (void)tool_kbd_use(panel->tool, (char *)(LINT_CAST(panel)));
	 if (timing(panel)) {
	    TIMER_ON(timer, panel->timer);
	    TIMER_SET(timer, panel->timer_full);
	 }
	 panel_caret_on(panel, FALSE);
	 panel->caret_pr = &panel_caret_pr;
	 panel_caret_on(panel, TRUE);
	 break;

      case KBD_DONE:
	 (void)tool_kbd_done(panel->tool, (caddr_t)(LINT_CAST(panel)));
	 panel_caret_on(panel, FALSE);
	 TIMER_OFF(timer);
	 panel->caret_pr = &panel_ghost_caret_pr;
         panel_caret_on(panel, TRUE);
	 break;

      default:
         break;
   }

   (void) panel_use_event(panel, &event, NULL);
}

/*****************************************************************************/
/* panel_handle_sigwinch                                                     */
/*****************************************************************************/

static
panel_handle_sigwinch(panel)
panel_handle panel;
{
   Pixwin       *pw = panel->pixwin;
   Rect         r, region_rect, cur_region_rect;

   win_getsize(panel->windowfd, &r);
   /* get the current view region rect and
    * compute what the view region rect should be.
    */
   (void) pw_get_region_rect(panel->view_pixwin, &cur_region_rect);
   panel_compute_region_rect(panel, &region_rect);
   pw_damaged(pw);
   /* if the panel rect or the region rect has changed,
    * then this is a WIN_RESIZE.
    */
   if (!rect_equal(&panel->rect, &r) ||
       !rect_equal(&cur_region_rect, &region_rect))  {
      panel_resize(panel);
      (void) pw_donedamaged(pw);
      panel_display(panel, PANEL_CLEAR);
   } else {
      panel_display(panel, PANEL_CLEAR);
      /* show the initial caret */
      panel_caret_on(panel, TRUE);
      (void) pw_donedamaged(pw);
   }

}
