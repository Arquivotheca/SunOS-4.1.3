#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)panel_sw.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*            Copyright (c) 1986 by Sun Microsystems, Inc.                   */

/* This module contains routines called by pre-notifier and
 * pre-wrapper code.
 */

#include <varargs.h>
#include <suntool/panel_impl.h>

#ifdef KEYMAP_DEBUG
#include "../../libsunwindow/win/win_keymap.h"
#else
#include <sunwindow/win_keymap.h>
#endif

static Panel panel_init_sw();


/* VARARGS1 */
Panel
panel_begin(tool, va_alist)
Tool 		*tool;
va_dcl
{
   caddr_t			avlist[ATTR_STANDARD_SIZE];
   va_list			valist;
   Toolsw	       		*tsw;

   va_start(valist);
   if (!attr_make(avlist, ATTR_STANDARD_SIZE, valist))  {
      va_end(valist);
      return NULL;
   }

   va_end(valist);
   if (tsw = panel_create_sw(tool, TRUE, avlist))
      return (Panel)tsw->ts_data;
   else
      return NULL;
}

Toolsw *
panel_create_sw(tool, notifier_based, avlist)
Tool		*tool;
int              notifier_based;
Attr_avlist	 avlist;
{
   register Panel_attribute	attr;
   char				*name		= "";
   int             		width		= -1;
   int             		height		= -1;
   int		   		y_offset	= ITEM_Y_GAP;
   Pixfont			*font		= (Pixfont *) NULL;
   Toolsw			*tsw;
   Attr_avlist			orig_avlist	= avlist;

   while(attr = (Panel_attribute) *avlist++) {
      switch (attr) {
	 case PANEL_ITEM_Y_GAP:
	    y_offset = (int) *avlist++;
	    break;

	 case PANEL_FONT :
	    font = (Pixfont *) LINT_CAST(*avlist++);
	    break;

	 default: 
	    avlist = attr_skip(attr,avlist);
	    break;
      }
   }

   /* use the default font if none specified */
   if (!font)
      font = pw_pfsysopen();

   /* convert any character units to pixel units */
   attr_replace_cu(orig_avlist, font, PANEL_ITEM_X_START, y_offset, y_offset);

   avlist = orig_avlist;
   while(attr = (Panel_attribute) *avlist++) {
      switch (attr) {
	 case PANEL_NAME:
	    name = (char *) *avlist++;
	    break;

	  case PANEL_WIDTH:
            width = (int) *avlist++;
            break;

	 case PANEL_HEIGHT:
	    height = (int) *avlist++;
	    break;

	 default: 
	    avlist = attr_skip(attr,avlist);
	    break;
      }
   }

   /* create the tool subwindow */
   tsw = tool_createsubwindow(tool, name, width, height);

   if (!tsw)
      return((Toolsw *)NULL);

   /* set the desired attributes */
   tsw->ts_data = panel_init_sw(tsw->ts_windowfd, notifier_based, orig_avlist);

   if (tsw->ts_data == NULL)
      return((Toolsw *)NULL);

   /* set the pointer from the panel to the tool subwindow */
   ((panel_handle)LINT_CAST(tsw->ts_data))->toolsw = tsw;

   /* set the pointer from the panel to the tool */
   ((panel_handle)LINT_CAST(tsw->ts_data))->tool = tool;

   return tsw;
}

/*****************************************************************************/
/* panel_init_sw                                                              */
/* allocates panel and initializes panel struct, calls set_panel to process  */
/* attribute list, opens pixwin.  If 'notifier_based' is true, then          */
/* registers the panel as a notifier client.                                 */
/*****************************************************************************/

static Panel
panel_init_sw(windowfd, notifier_based, avlist) 
int 		windowfd;
int     	notifier_based;
Attr_avlist	avlist;
{
   register panel_handle		panel;

   if (!(panel = panel_create_panel_struct((caddr_t) 0, windowfd)))
      return NULL;

   if (!(panel->pixwin = pw_open_monochrome(windowfd))) 
      return NULL;

   panel->view_pixwin = pw_region(panel->pixwin, 0, 0,
                                  panel->rect.r_width,
                                  panel->rect.r_height);

   if (notifier_based) {
      if (win_register((caddr_t)(LINT_CAST(panel)), panel->pixwin, 
                       panel_notify_event, panel_destroy,
		       PW_FIXED_IMAGE | PW_INPUT_DEFAULT))
         return NULL;
      panel->status |= USING_NOTIFIER;
   }

   /* set the specified attributes */
   (*panel->ops->set_attr)(panel, avlist);
   
   (void)panel_set_inputmask(panel, windowfd);

   return (Panel) panel;
}
