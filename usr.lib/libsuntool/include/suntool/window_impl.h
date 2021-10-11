/*	@(#)window_impl.h 1.1 92/07/30 SMI	*/

/***********************************************************************/
/*	                      window_impl.h			       */
/*          Copyright (c) 1985 by Sun Microsystems, Inc.               */
/***********************************************************************/

#ifndef window_impl_DEFINED
#define window_impl_DEFINED

#include <sunwindow/sun.h>
#include <suntool/window.h>

#define Pkg_private
#define Pkg_extern extern
#define Private static

#define window_attr_next(attr) (Window_attribute *)attr_next((caddr_t *)attr)


/***********************************************************************/
/*	        	Structures 				       */
/***********************************************************************/

struct window {
    
    Attr_pkg		 type;
    caddr_t		(*get_proc)();
    int			(*set_proc)();
    int			(*preset_proc)();
    int			(*postset_proc)();
    int			(*layout_proc)();
    int			 fd;
    struct pixwin	*pixwin;
    caddr_t		 object;
    struct window	*owner;
    char 		*name;
    caddr_t		 menu;
    caddr_t		 set_private;    
    void		(*event_proc)();
    void		(*default_event_proc)();
    caddr_t		 client_data; 
    struct pixfont	*font;
    int			 top_margin;
    int			 bottom_margin;
    int			 left_margin;
    int			 right_margin;
    int			 row_height;
    int			 column_width;
    int			 row_gap;
    int			 column_gap;
    short		 loop_depth;

    /* Flags */

    unsigned		 show:1;
    unsigned		 show_updates:1;
    unsigned		 well_behaved:1;
    unsigned		 registered:1;
    unsigned		 created:1;

 /* Should be moved up with client data for next build */
    caddr_t		 impl_data;
};


/* 
 * Package private
 */

#define client_to_win(client) (struct window *) \
 (LINT_CAST((((caddr_t)client == win_last_client) ? win_last_window \
  	: win_from_client(client))))
Pkg_private Window win_from_client();
Pkg_private caddr_t win_last_window, win_last_client;

#define	actual_row_height(win)		\
    (win->row_height ? win->row_height : win->font->pf_defaultsize.y)

#define	actual_column_width(win)	\
    (win->column_width ? win->column_width : win->font->pf_defaultsize.x)



#endif ~window_impl_DEFINED
