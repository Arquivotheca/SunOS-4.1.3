#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)ttysw_menu.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1985, 1986 and 1987 by Sun Microsystems, Inc.
 */

/*
 * Ttysw menu initialization and call-back procedures
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>

#include <stdio.h>
#include <ctype.h>

#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/win_input.h>
#include <suntool/ttysw.h>
#include <suntool/textsw.h>
#include <suntool/walkmenu.h>
#include <suntool/selection.h>
#include <suntool/selection_svc.h>
#include <suntool/ttysw_impl.h>

#define HELP_INFO(s) HELP_DATA,s,

extern void ttygetselection();
extern void ttysw_do_put_get();
extern struct pixrect *mem_create();
extern Textsw_status	textsw_set();

static int	ttysw_defaults_checked;
static int	ttysw_use_new_menu;
static int	ttysw_use_new_menu_style();

/* generally applicable ttysw menu definitions */

static char *ttysw_copy_then_paste = "Copy then Paste";
static char *ttysw_paste_only      = "          Paste";
static char *ttysw_copy_then       = "Copy then";
static char *ttysw_put_then_get = "Put then Get";
static char *ttysw_get_only     = "         Get";
static char *ttysw_put_then =     "Put then";
#define TTYSW_SCROLLING_ENABLESTRING "Enable Scrolling"
static char *ttysw_scrolling_enable	= TTYSW_SCROLLING_ENABLESTRING;

/* ttysw old non-walking menu definitions */

#define TTYSW_STUFF	 (caddr_t)1
#define TTYSW_PAGE	 (caddr_t)2
#define TTYSW_HIST	 (caddr_t)3
#define TTYSW_FLUSH	 (caddr_t)4
#define TTYSW_PUT_GET	 (caddr_t)5
#define TTYSW_ENABLE_SCROLLING	 (caddr_t)6

/* shorthand */
#define	iwbp	ttysw->ttysw_ibuf.cb_wbp
#define	irbp	ttysw->ttysw_ibuf.cb_rbp

/* ttysw non-walking menu definitions */

/* static void		 ttysw_do_enable_scrolling(); */

/* ttysw walking menu definitions */

static Menu_item	 ttysw_menu_page_state();
static Menu_item	 ttysw_menu_put_get_state();
static void		 ttysw_show_walkmenu();

static void		 ttysw_menu_stuff();
static void		 ttysw_enable_scrolling();
static void		 ttysw_disable_scrolling();
static void		 ttysw_menu_page();
static void		 ttysw_menu_flush();
static void		 ttysw_menu_history();
static void		 ttysw_menu_put_get();

static struct pixrect	*ttysw_get_only_pr;

/* cmdsw walking menu definitions */

extern Cmdsw	*cmdsw;

int	scroll_disabled_from_menu = 0;


typedef enum {
	TTYSW_APPEND_ONLY		= TEXTSW_MENU_LAST_CMD + 1,
	TTYSW_LAST_CMD			= TEXTSW_MENU_LAST_CMD + 2
}	Ttysw_menu_cmd;

static char	*ttysw_menu_edit_on	= "Enable Editing";
static char	*ttysw_menu_edit_off	= "Disable Editing";
static char	*ttysw_scrolling_disable = "Disable Scrolling";

/* ttysw old non-walking menu utilities */

int
ttysw_process_menu(ttysw, ie)
    register struct ttysubwindow *ttysw;
    register struct inputevent *ie;
{
    int                 show_flush;

    if (win_inputnegevent(ie)) {
	return TTY_OK;
    }

    show_flush = (iwbp > irbp);

    /* use the walking menu if present */
    ttysw_show_walkmenu(ttysw, ie, show_flush);
    return (TTY_DONE);
}


/* ttysw walking menu utilities */

Menu
ttysw_walkmenu(ttysw)
	register Ttysw *ttysw;
{   
    Menu	ttysw_menu;
    Menu_item	put_get_item,enable_scrolling_item;
    int		new_wording = ttysw_use_new_menu_style();

    ttysw_menu = menu_create(
	MENU_ITEM,
	  MENU_STRING, "Stuff",
	  MENU_ACTION, ttysw_menu_stuff,
	  MENU_CLIENT_DATA, ttysw,
	  HELP_INFO("ttysw:mstuff")
	  0, 
	MENU_ITEM,
	  MENU_STRING, "Disable Page Mode",
	  MENU_ACTION, ttysw_menu_page,
	  MENU_GEN_PROC, ttysw_menu_page_state,
	  MENU_CLIENT_DATA, ttysw,
	  HELP_INFO("ttysw:mdsbpage")
	  0,
	HELP_INFO("ttysw:menu")
	0);
    if (ttysw_get_only_pr == 0) {
	struct pixfont		*menu_font;
	struct pr_subregion	 get_only_bound;
	struct pr_prpos		 get_only_prpos;
	int			 menu_left_margin;
	
	/* compute the size of the pixrect and allocate it */
	menu_font = (struct pixfont *)LINT_CAST(menu_get(
	    ttysw_menu, MENU_FONT));
	if (new_wording) { 
	    (void)pf_textbound(&get_only_bound,
		strlen(ttysw_copy_then_paste),
		menu_font, ttysw_copy_then_paste);
	} else (void)pf_textbound(&get_only_bound,
	    strlen(ttysw_put_then_get),
	    menu_font, ttysw_put_then_get);
	menu_left_margin = (int)menu_get(ttysw_menu, MENU_LEFT_MARGIN);
	ttysw_get_only_pr = mem_create(
	    get_only_bound.size.x + menu_left_margin +
	    (int)menu_get(ttysw_menu, MENU_RIGHT_MARGIN),
	    get_only_bound.size.y, 1);
	/* write the text into the pixrect */
	get_only_prpos.pr = ttysw_get_only_pr;
	get_only_prpos.pos.x = (new_wording) ?
	    menu_left_margin - menu_font->pf_char[
		ttysw_copy_then_paste[0]].pc_home.x
	  : menu_left_margin - menu_font->pf_char[
		ttysw_put_then_get[0]].pc_home.x;
	get_only_prpos.pos.y = (new_wording) ?
	       - menu_font->pf_char[ttysw_copy_then_paste[0]].pc_home.y
	    :  - menu_font->pf_char[ttysw_put_then_get[0]].pc_home.y;
	(void)pf_text(get_only_prpos, PIX_SRC, menu_font,
	    (new_wording) ? ttysw_copy_then_paste : ttysw_put_then_get);
	/* gray out "Put, then" or "Cut then" */
	if (new_wording) {
	    (void)pf_textbound(&get_only_bound,
		strlen(ttysw_copy_then), menu_font, ttysw_copy_then);
	} else
	    (void)pf_textbound(&get_only_bound,
		strlen(ttysw_put_then), menu_font, ttysw_put_then);
	(void)pr_replrop(ttysw_get_only_pr, menu_left_margin, 0,
	    get_only_bound.size.x, get_only_bound.size.y, 
	    PIX_SRC & PIX_DST, &menu_gray50_pr, 0, 0);
    }
    put_get_item = menu_create_item(
	MENU_STRING, (new_wording) ?
	    ttysw_copy_then_paste : ttysw_put_then_get,
	MENU_ACTION, ttysw_menu_put_get,
	MENU_GEN_PROC, ttysw_menu_put_get_state,
	MENU_CLIENT_DATA, ttysw,
	HELP_INFO("ttysw:mputget")
	0); 
    (void)menu_set(ttysw_menu, MENU_APPEND_ITEM, put_get_item, 0);
    
    if (ttysw->ttysw_hist) {
    	enable_scrolling_item = menu_create_item(
		MENU_STRING, ttysw_scrolling_enable,
		MENU_ACTION, ttysw_enable_scrolling,
		MENU_CLIENT_DATA, ttysw,
 		HELP_INFO("ttysw:menbscroll")
		MENU_INACTIVE, TRUE, /* newly added here */
		0); 
    	(void)menu_set(ttysw_menu, MENU_APPEND_ITEM, enable_scrolling_item, 0);
    };

    return ttysw_menu;
}


static void
ttysw_show_walkmenu(ttysw, event, show_flush)
	Ttysw	*ttysw;
	Event	*event;
	int	show_flush;
{
    Menu_item	flush_item;

    if (show_flush) {
	flush_item = menu_create_item(
	    MENU_STRING, "Flush",
	    MENU_ACTION, ttysw_menu_flush,
	    MENU_CLIENT_DATA, ttysw,
	    HELP_INFO("ttysw:mflush")
	    0); 
	(void)menu_set(ttysw->ttysw_menu, MENU_APPEND_ITEM, flush_item, 0);
    }

    (void)menu_show_using_fd(ttysw->ttysw_menu, ttysw->ttysw_wfd, event);

    if (show_flush) {
	(void)menu_set(ttysw->ttysw_menu, MENU_REMOVE_ITEM, flush_item, 0);
	menu_destroy(flush_item);
    }
}


/* 
 *  Menu item gen procs
 */
static Menu_item
ttysw_menu_page_state(mi, op)
	Menu_item mi;
	Menu_generate op;
{
    Ttysw *ttysw;
    
    if (op == MENU_DESTROY)
	return mi;

    ttysw = (Ttysw *) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));

    if (ttysw->ttysw_frozen)
	(void)menu_set(mi, MENU_STRING, "Continue",
		       HELP_INFO("ttysw:mcont")
		       0);
    else if (ttysw_getopt((caddr_t) ttysw, TTYOPT_PAGEMODE))
	(void)menu_set(mi, MENU_STRING, "Disable Page Mode",
		       HELP_INFO("ttysw:mdsbpage")
		       0);
    else
	(void)menu_set(mi, MENU_STRING, "Enable Page Mode ",
		       HELP_INFO("ttysw:menbpage")
		       0);
    return mi;
}

static Menu_item
ttysw_menu_put_get_state(mi, op)
	Menu_item mi;
	Menu_generate op;
{
    Ttysw *ttysw;
    int	   new_wording = ttysw_use_new_menu_style();
    
    if (op == MENU_DESTROY)
	return mi;

    ttysw = (Ttysw *) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));

    if (ttysw_is_seln_nonzero(ttysw, SELN_PRIMARY)) {
        (void)menu_set(mi,
	    MENU_IMAGE,		0,
	    MENU_STRING,	(new_wording) ?
			ttysw_copy_then_paste : ttysw_put_then_get,
 	    HELP_INFO("ttysw:mputget")
	    MENU_INACTIVE,	FALSE,
	    0);
    } else if (ttysw_is_seln_nonzero(ttysw, SELN_SHELF)) {
        (void)menu_set(mi,
	    MENU_STRING,	0,
	    MENU_IMAGE,		ttysw_get_only_pr,
	    HELP_INFO("ttysw:mgetonly")
	    MENU_INACTIVE,	FALSE,
	    0);
    } else {
        (void)menu_set(mi,
	    MENU_IMAGE,		0,
	    MENU_STRING,	(new_wording) ?
			ttysw_copy_then_paste : ttysw_put_then_get,
	    HELP_INFO("ttysw:mputget")
	    MENU_INACTIVE,	TRUE,
	    0);
    }

    return mi;
}

/*
 *  Callout functions
 */



/* ARGSUSED */
static void
ttysw_menu_stuff(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    Ttysw	*ttysw = (Ttysw *) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));
    
    ttygetselection(ttysw);
}

/* ARGSUSED */
static void
ttysw_enable_scrolling(menu, mi)
    Menu	menu;
    Menu_item	mi;
{
    Ttysw	*ttysw = (Ttysw *) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));
    
    ttysw_setopt(ttysw, TTYOPT_TEXT, 1);
    scroll_disabled_from_menu = FALSE;
}

extern Menu_item
ttysw_get_scroll_cmd_from_menu_for_ttysw(ttysw_menu)
    Menu	ttysw_menu;
{
    /* The reason for using menu_get() is menu_find() has problem. */
    return (Menu_item)menu_get(
	ttysw_menu,
	MENU_NTH_ITEM,
	menu_get(ttysw_menu, MENU_NITEMS, 0));
}

extern Menu_item
ttysw_get_scroll_cmd_from_menu_for_textsw(textsw_menu)
    Menu	textsw_menu;
{
    /* The reason for using menu_get() is menu_find() has problem. */
    Menu_item cmd_modes_item = (Menu_item)menu_get(
	textsw_menu,
	MENU_NTH_ITEM,
	menu_get(textsw_menu, MENU_NITEMS, 0));
    Menu cmd_modes_menu = (Menu)menu_get( /* get the pullright menu */
	cmd_modes_item,
	MENU_PULLRIGHT,
	0);

    return (Menu_item)menu_get(
	cmd_modes_menu,
	MENU_NTH_ITEM,
	menu_get(cmd_modes_menu, MENU_NITEMS, 0));
}

void
ttysw_set_scrolling_cmds(disable_scroll, enable_scroll, allow_enable)
     Menu_item	disable_scroll, enable_scroll;
     int	allow_enable;
{
    if (allow_enable) {
	(void)menu_set(enable_scroll, MENU_INACTIVE, FALSE, 0); 
	(void)menu_set(disable_scroll, MENU_INACTIVE, TRUE, 0);
    } else {
	(void)menu_set(enable_scroll, MENU_INACTIVE, TRUE, 0); 
	(void)menu_set(disable_scroll, MENU_INACTIVE, FALSE, 0);
    }

}

extern void
ttysw_set_scrolling(textsw_menu, ttysw_menu, allow_enable)
   Menu		textsw_menu, ttysw_menu;
   int		allow_enable;
{
    Menu_item	enable_scroll = ttysw_get_scroll_cmd_from_menu_for_ttysw(
					ttysw_menu);
    Menu_item	disable_scroll = ttysw_get_scroll_cmd_from_menu_for_textsw(
					textsw_menu);

    (void)ttysw_set_scrolling_cmds(
	disable_scroll, enable_scroll, allow_enable); 
}

/* static void
ttysw_do_enable_scrolling(ttysw)
    Ttysw	*ttysw;
{   
    
    Textsw	textsw = (Textsw)ttysw->ttysw_hist;
    Menu	ttysw_menu = (Menu)ttysw->ttysw_menu;
    Menu	textsw_menu = (Menu)textsw_get(textsw, TEXTSW_MENU);
    int		dont_allow_enable = 0;
    
    
    ttysw_setopt(ttysw, TTYOPT_TEXT, 1);
} */

/* ARGSUSED */
static void
ttysw_menu_page(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    Ttysw	*ttysw = (Ttysw *) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));
    
    if (ttysw->ttysw_frozen)
	(void) ttysw_freeze(ttysw, 0);
    else
	(void)ttysw_setopt((caddr_t) ttysw, TTYOPT_PAGEMODE, 
	    !ttysw_getopt((caddr_t) ttysw, TTYOPT_PAGEMODE));
}

/* ARGSUSED */
static void
ttysw_menu_flush(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    caddr_t	ttysw = (caddr_t) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));
    
    (void)ttysw_flush_input(ttysw);
}

/* ARGSUSED */
static void
ttysw_menu_put_get(menu, mi)
    Menu	menu;
    Menu_item	mi;
{   
    Ttysw	*ttysw = (Ttysw *) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));
    
    ttysw_do_put_get(ttysw);
}

/* cmdsw walking menu definitions */

ttysw_set_menu(textsw)
    Textsw		  textsw;
{
    Menu	 cmd_menu, cmds;
    Menu_item	 cmd_item1, cmd_item2, cmd_item, cmd_pullright_item;
    int		 ttysw_menu_append_only();

    cmd_menu = (Menu)textsw_get(textsw, TEXTSW_MENU);
    cmds = (Menu)menu_create(
	MENU_LEFT_MARGIN,	6,
	MENU_ITEM,
	    MENU_VALUE,		TTYSW_APPEND_ONLY,
    	    MENU_ACTION,	ttysw_menu_append_only,
    	    MENU_CLIENT_DATA,	textsw,
	    HELP_INFO("ttysw:mappendonly")
    	    MENU_INACTIVE,	FALSE,
    	    0,
	MENU_ITEM,
	    MENU_STRING,	ttysw_scrolling_disable,
    	    MENU_ACTION,	ttysw_disable_scrolling,
	    MENU_CLIENT_DATA,	textsw,
	    HELP_INFO("ttysw:mdsbscroll")
    	    MENU_INACTIVE,	FALSE,
    	    0,
 	HELP_INFO("ttysw:mcommands")
	0);
    cmd_pullright_item = (Menu_item)menu_create_item(
	MENU_PULLRIGHT_ITEM,	"Cmd Modes",	cmds,
	0);
/*****Changed to above pullright structure for 3.4
    cmd_item1 = (Menu_item)menu_create_item(
    	MENU_VALUE,		TTYSW_APPEND_ONLY,
    	MENU_ACTION,		ttysw_menu_append_only,
    	MENU_CLIENT_DATA,	textsw,
    	MENU_INACTIVE,		FALSE,
    	0);
    cmd_item2 = (Menu_item)menu_create_item(
    	MENU_STRING,		ttysw_scrolling_disable,
    	MENU_ACTION,		ttysw_disable_scrolling,
	MENU_CLIENT_DATA,	textsw,
    	MENU_INACTIVE,		FALSE,
    	0);
    if (cmdsw->append_only_log) {
	(void)menu_set(cmd_item1,	MENU_STRING,	ttysw_menu_edit_on, 0);
    } else {
	(void)menu_set(cmd_item1,	MENU_STRING,	ttysw_menu_edit_off, 0);
    }
     *
     * cmd_item2 has to be the last item of the menu; otherwise,
     * it will confuse menu_get() in ttysw_enable_scrolling() and
     * ttysw_text_event().     
     *
    (void)menu_set(cmd_menu, MENU_APPEND_ITEM, cmd_item1,
                             MENU_APPEND_ITEM, cmd_item2, 0);
*************/
    cmd_item = (Menu_item)menu_get(cmds, MENU_NTH_ITEM, 1);
    if (cmdsw->append_only_log) {
	(void)menu_set(cmd_item, MENU_STRING, ttysw_menu_edit_on, 0);
    } else {
	(void)menu_set(cmd_item, MENU_STRING, ttysw_menu_edit_off, 0);
    }
    (void)menu_set(cmd_menu, MENU_APPEND_ITEM, cmd_pullright_item, 0);
}

/* ARGSUSED */
ttysw_menu_append_only(cmd_menu, cmd_item)
    Menu	 cmd_menu;
    Menu_item	 cmd_item;
{
    Textsw	 textsw = (Textsw)(LINT_CAST(menu_get(cmd_item,
    					MENU_CLIENT_DATA)));
    Textsw_index tmp_index, insert;

    cmdsw->append_only_log = !cmdsw->append_only_log;
    if (cmdsw->append_only_log) {
	(void)menu_set(cmd_item, MENU_STRING, ttysw_menu_edit_on, 0);
	tmp_index = (int)textsw_find_mark(textsw, cmdsw->pty_mark);
	insert = (Textsw_index)textsw_get(textsw, TEXTSW_INSERTION_POINT);
	if (insert != tmp_index) {
	    (void)textsw_set(textsw, TEXTSW_INSERTION_POINT, tmp_index, 0);
	}
	cmdsw->read_only_mark =
	    textsw_add_mark(textsw,
	        cmdsw->cooked_echo ? tmp_index : TEXTSW_INFINITY-1,
	        TEXTSW_MARK_READ_ONLY);
    } else {
	(void)menu_set(cmd_item, MENU_STRING, ttysw_menu_edit_off, 0);
	textsw_remove_mark(textsw, cmdsw->read_only_mark);
    }
	  
}

/* ARGSUSED */
static void
ttysw_disable_scrolling(cmd_menu, cmd_item)
    Menu	 cmd_menu;
    Menu_item	 cmd_item;
{

    /* The reason for using menu_get() is menu_find() has problem. */
    Textsw   	textsw = (Textsw) LINT_CAST (menu_get(cmd_item,
    					  MENU_CLIENT_DATA));
    Ttysw	*ttysw = (Ttysw *) LINT_CAST (textsw_get(textsw,
    					  TEXTSW_CLIENT_DATA));
    Menu	ttysw_menu = (Menu)ttysw->ttysw_menu;
    Menu_item	enable_scroll = ttysw_get_scroll_cmd_from_menu_for_ttysw(
		 			  ttysw_menu);

    
    ttysw->ttysw_hist = (FILE *) LINT_CAST(textsw);
    ttysw_setopt(ttysw, TTYOPT_TEXT, 0); 
    scroll_disabled_from_menu = TRUE;
    
    (void)ttysw_set_scrolling_cmds(cmd_item, enable_scroll, TRUE);

}
				
static int
ttysw_use_new_menu_style()
{
    if (!ttysw_defaults_checked) {
	ttysw_use_new_menu = (int)defaults_get_boolean(
		"/Compatibility/New_Tty_Menu", TRUE, (int *)NULL);
	ttysw_defaults_checked = 1;
    }
    return (ttysw_use_new_menu);
}
