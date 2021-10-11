#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)walkmenu_public.c 1.1 92/07/30 Copyright 1985 Sun Micro";
#endif
#endif

/*
 *  Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*-
	WALKING MENU PACKAGE

	walkmenu_public.c, Sun Jun 30 15:38:39 1985

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
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>

#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/defaults.h>
#include "sunwindow/sv_malloc.h"

#include <suntool/fullscreen.h>
#include <suntool/window.h>

#include <suntool/walkmenu_impl.h>

/*
 * win_ioctl.h is dependent on:
 * sys/{time,ioctl}.h, sunwindow/{rect,rectlist,win_screen,win_input}.h
 * some of which was already included before we added the ioctl
 */
#include <sys/ioctl.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_ioctl.h>

/* ------------------------------------------------------------------------- */

/* 
 * Public
 */
extern	struct cursor *walkmenu_cursor;

/* Pending display cursor for sticky menus */
/* /usr/view/2.0/usr/src/sun/usr.bin/suntool/images/menu_pending.cursor */
extern	struct cursor walkmenu_cursor_pending;

/* Define 3 shades of gray for shadow replrop */
extern	struct pixrect menu_gray25_pr;
extern	struct pixrect menu_gray50_pr;
extern	struct pixrect menu_gray75_pr;

Menu		menu_create();
Menu_item	menu_create_item();
caddr_t		menu_get();
int		menu_set();
void		menu_destroy();
void		menu_destroy_with_proc();
caddr_t		menu_show();
caddr_t		menu_show_using_fd();
Menu_item	menu_find();

caddr_t		menu_pullright_return_result();
caddr_t		menu_return_value();
caddr_t		menu_return_item();
caddr_t		menu_return_no_value();
caddr_t		menu_return_no_item();


/* 
 * Package private
 */
Pkg_extern	int menu_curitem();
Pkg_extern	int menu_render();

Pkg_private	struct menu_item *menu_create_item_avlist();

/* Ops vectors for menus and menu_items */
Pkg_extern	caddr_t	menu_gets(), menu_item_gets();
Pkg_extern	void	menu_sets(), menu_item_sets();
Pkg_private	void	menu_destroys(), menu_item_destroys();

Pkg_private	struct menu_ops_vector menu_ops =
{   
    menu_gets, menu_sets, menu_destroys
};
Pkg_private	struct menu_ops_vector menu_item_ops =
{   
    menu_item_gets, menu_item_sets, menu_item_destroys
};

Pkg_private	struct fullscreen *menu_fs;	  /* fullscreen object */

/* 
 * Private
 */
Private		caddr_t menu_return_result();
Private		accelerated_selection();

Private		Defaults_pairs defaults_shadow_enum[] = {
    			"25_percent_gray", (int)&menu_gray25_pr,
    			"50_percent_gray", (int)&menu_gray50_pr,
    			"75_percent_gray", (int)&menu_gray75_pr,
			NULL,		   (int)NULL
};
Private		Defaults_pairs defaults_selection_enum[] = {
    			"Default",	  (int)MENU_DEFAULT_ITEM,
    			"Last_Selection", (int)MENU_SELECTED_ITEM,
			NULL,		  (int)NULL
};

/* 
 * Private defs
 */
#define	MENU_DEFAULT_MARGIN		1
#define	MENU_DEFAULT_LEFT_MARGIN	16
#define	MENU_DEFAULT_RIGHT_MARGIN	6

/* ------------------------------------------------------------------------- */

/*
 * Display the menu, get the menu item, and call notify proc.
 * Default proc returns a pointer to the item selected or NULL.
 */
/* VARARGS3 */
caddr_t
menu_show(menu, win, iep, va_alist)
	Menu menu;
	Window win;
	struct inputevent *iep;
	va_dcl
{   
    Menu_attribute avlist[ATTR_STANDARD_SIZE];
    va_list valist;
    Menu_attribute *attrs;
    int fd;
   
    fd = window_fd(win);
    va_start(valist);
    (void) attr_make((char **)LINT_CAST(avlist), ATTR_STANDARD_SIZE,  valist);
    va_end(valist);
    for (attrs = avlist; *attrs; attrs = menu_attr_next(attrs)) {
        switch (attrs[0]) {

        case MENU_BUTTON:
    	    event_set_id(iep, (int)attrs[1]);
    	    break;
        
        case MENU_FD: /* Remove this someday */
    	    fd = (int)attrs[1];
    	    break;
	    
        case MENU_POS:
    	    iep->ie_locx = (int)attrs[1], iep->ie_locy = (int)attrs[2];
    	    break;
	    
        case MENU_NOP:
    	    break;
    
        default:
    	    if (ATTR_PKG_MENU == ATTR_PKG(attrs[0]))
    	        (void) fprintf(stderr,
			       "menu_show:  Menu attribute not allowed.\n%s\n",
    		    attr_sprint((caddr_t)NULL, (u_int)attrs[0]));
    	    break;
        }
    }
    return(menu_show_using_fd(menu, fd, iep));
}


caddr_t
menu_show_using_fd(menu, fd, iep)
	Menu menu;
	int fd;
	struct inputevent *iep;
{   
    struct inputevent ie;
    struct inputmask im;
    struct menu_info info;
    struct menu *m = (struct menu *)LINT_CAST(menu);
   
   if (!m || m->ops != &menu_ops) return MENU_NO_VALUE;
   /*
    * Grab all input and disable anybody but windowfd from writing
    * to screen while we are violating window overlapping.
    */
    menu_fs = fullscreen_init(fd);
    if (!menu_fs) return menu_return_no_value(menu, (Menu_item)0);

   /*
    * menu fullscreen operations are different from bounding boxes,
    * window moves, etc. so we want to truely know all plane groups
    * available.
    */
   (void) ioctl(menu_fs->fs_windowfd, WINGETAVAILPLANEGROUPS,
		menu_fs->fs_pixwin->pw_clipdata->pwcd_plane_groups_available);

   /*
    * Make sure input mask allows menu package to track cursor motion
    */
    
    /* Keymap debugging function; remove after debugging complete */
    
    /*	win_keymap_debug_start_blockio();     */
	
    im = menu_fs->fs_cachedim;
    im.im_flags |= IM_NEGEVENT;
    im.im_flags |= IM_ASCII|IM_META|IM_NEGMETA;
    win_keymap_set_smask(menu_fs->fs_windowfd, ACTION_HELP);
    win_setinputcodebit(&im, LOC_MOVE);
    win_setinputcodebit(&im, LOC_STILL);
    win_setinputcodebit(&im, iep->ie_code);
    (void) win_setinputmask(menu_fs->fs_windowfd, &im,
		     	    (struct inputmask *)NULL, WIN_NULLLINK);
#								ifdef needs_work
    if (m->allow_accelerated_selection)
	accelerated_selection(m, menu_fs->fs_windowfd, iep->ie_code);
#								endif needs_work
    info.first_iep = iep, info.last_iep = &ie;
    info.depth = 0;
    info.help = (caddr_t)0;

    {  /* FIXME: When accelerated selection works make conditional(?) */
	ie = *iep;
	if (m->stay_up) { /* Sticky menus (double click) */
	    (void)win_setcursor(menu_fs->fs_windowfd, &walkmenu_cursor_pending);
	    do {
		if (input_readevent(menu_fs->fs_windowfd, &ie) == -1) {
		    (void) fprintf(stderr,
			           "menu_show: failed to track cursor.\n");
		    perror("menu_show");
		    return MENU_NO_VALUE;
		}
	    } while (!(iep->ie_code == ie.ie_code && win_inputnegevent(&ie)));
	}

	(void)win_setcursor(menu_fs->fs_windowfd, walkmenu_cursor);
	(void)menu_render(m, &info, (struct menu_item *)0, RECTNODE_NULL, FALSE,fd);
		/*** Here we go ***/
    }
    (void)fullscreen_destroy(menu_fs);

    /* Keymap debugging function; remove after debugging complete */

    /*	win_keymap_debug_stop_blockio(); */

    if (info.depth && m->jump_after_selection
	|| !info.depth && m->jump_after_no_selection)
	(void) win_setmouseposition(fd, iep->ie_locx, iep->ie_locy);

   /*
    * If help was requested, do it.
    */
    if (info.depth == 0 && info.help)
	help_request((caddr_t)0, info.help, info.last_iep);

    /*  
     * release event lock before calling notify proc 
     * to allow other window processes to receive input
     * events while notify proc is executing.
     */
     win_release_event_lock(fd);

   /*
    * Call notify_proc.  Should handle special case of selection = 0
    */
    info.notify_proc = m->notify_proc;
    if (!info.notify_proc) info.notify_proc = MENU_DEFAULT_NOTIFY_PROC;

    return info.depth > 0 ? menu_return_result(m, &info, (struct menu_item *)0)
	: menu_return_no_value(menu, (Menu_item)0);
}

/* Cache the standard menu data obtained from the defaults database */
static struct menu *m_cache;

/* VARARGS */
Menu
menu_create(va_alist)
	va_dcl
{   
    Menu_attribute avlist[ATTR_STANDARD_SIZE];
    va_list valist;
    register struct menu *m =
	(struct menu *)LINT_CAST(sv_calloc(1, sizeof(struct menu)));
    char *font_name;
#define	Null_status	(int *)0

    if (!m_cache) {
        m_cache = (struct menu *)LINT_CAST(sv_calloc(1, sizeof(struct menu)));

        m_cache->default_image.margin =
    	defaults_get_integer_check("/Menu/Margin",
    				   MENU_DEFAULT_MARGIN, 1, 100, Null_status);
        m_cache->default_image.left_margin =
    	defaults_get_integer_check("/Menu/Left_margin",
    				   MENU_DEFAULT_LEFT_MARGIN, 1, 100,Null_status);
        m_cache->default_image.right_margin =
    	defaults_get_integer_check("/Menu/Right_margin",
    				    MENU_DEFAULT_RIGHT_MARGIN, 1, 100,
    				    Null_status);
    
        m_cache->default_image.boxed =
    	(int)defaults_get_boolean("/Menu/Box_all_items", False, Null_status);
    
        m_cache->default_image.center = (int)defaults_get_boolean(
    	"/Menu/Center_string_items", False, Null_status);
    
        font_name =	defaults_get_string("/Menu/Font", "", Null_status);
        if (*font_name != NULL)
    	m_cache->default_image.font = pf_open(font_name);
    
        m_cache->column_major =
        (int)defaults_get_boolean("/Menu/Items_in_column_major",
    						False, Null_status);
    
        m_cache->shadow_pr = (struct pixrect *)
    	defaults_lookup(defaults_get_string("/Menu/Shadow", "50_percent_gray",
    					    Null_status),
    			defaults_shadow_enum);
    
        m_cache->pullright_delta =
    	defaults_get_integer_check("/Menu/Pullright_delta", 
    				   MENU_DEFAULT_PULLRIGHT_DELTA, 1, 9999,
    				   Null_status);
        m_cache->jump_after_selection =
    	(int)defaults_get_boolean("/Menu/Restore_cursor_after_selection",
    				  False, Null_status);
    
        m_cache->jump_after_no_selection =
    	(int)defaults_get_boolean("/Menu/Restore_cursor_after_no_selection",
    				  False, Null_status);
    
        m_cache->initial_selection = (Menu_attribute)
    	defaults_lookup(defaults_get_string("/Menu/Initial_selection",
    					    "Default", Null_status),
    			defaults_selection_enum);
    
        m_cache->help_data = "sunview:menu";
    
        m_cache->stand_off =
    	!defaults_get_boolean("/Menu/Initial_selection_selected", False,
    			      Null_status);
    
        m_cache->display_one_level =
    	!defaults_get_boolean("/Menu/Initial_selection_expanded", True,
    			      Null_status);
    
        m_cache->default_selection = (Menu_attribute)
    	defaults_lookup(defaults_get_string("/Menu/Default_selection",
    					    "Default", Null_status),
    			defaults_selection_enum);
    
        m_cache->stay_up =
        (int)defaults_get_boolean("/Menu/Stay_up", False, Null_status);


/*  FIXME:  Not very useful unless there is instance specific defaults.
    m_cache->default_position =
	defaults_get_integer_check("/Menu/Default", 1, 1, 9999, NULL);
 */
    }
    bcopy((caddr_t)m_cache, (caddr_t)m, sizeof(struct menu));

    /* 
     *  Malloc the menu storage and create the item list
     */
    m->ops = &menu_ops;
    m->nitems = 0, m->max_nitems = 2 * MENU_FILLER;
    m->item_list = (struct menu_item **)
	LINT_CAST(malloc(sizeof(struct menu_item *) * 2 * MENU_FILLER));
    if (!m->item_list) {
	(void) fprintf(stderr,
		"menu_create: Malloc failed to allocate an item list.\n");
	return NULL;
    }

    va_start(valist);
    (void) attr_make((char **)LINT_CAST(avlist), ATTR_STANDARD_SIZE, valist);
    va_end(valist);
    menu_sets(m, avlist);

    if (!m->notify_proc) m->notify_proc = MENU_DEFAULT_NOTIFY_PROC;

    return (Menu)m;
}


/* VARARGS */
Menu_item
menu_create_item(va_alist)
	va_dcl
{
    Menu_attribute avlist[ATTR_STANDARD_SIZE];
    va_list valist;

    va_start(valist);
    (void) attr_make((char **)LINT_CAST(avlist), ATTR_STANDARD_SIZE, valist);
    va_end(valist);
    return (Menu_item)menu_create_item_avlist(avlist);
}


Pkg_private struct menu_item *
menu_create_item_avlist(avlist)
	Menu_attribute *avlist;
{   
    struct menu_item *mi;

    mi = (struct menu_item *)LINT_CAST(sv_calloc(1, sizeof(struct menu_item)));
    mi->image = (struct image *)LINT_CAST(image_create(
	LINT_CAST(IMAGE_RELEASE), 0));
    mi->ops = &menu_item_ops;

    menu_item_sets(mi, avlist);

    return mi;
}


/* VARARGS1 */
int
menu_set(m, va_alist)
	caddr_t m;
	va_dcl
{
     Menu_attribute avlist[ATTR_STANDARD_SIZE]; 
     va_list valist; 
     
     if (!m) return FALSE;

     va_start(valist); 
     (void) attr_make((char **)LINT_CAST(avlist), ATTR_STANDARD_SIZE, valist);
     va_end(valist); 
    
     (((struct menu *)LINT_CAST(m))->ops->menu_set_op)(m, avlist);
     return TRUE;
}


/* VARARGS2 */
caddr_t
menu_get(m, attr, v1)
	caddr_t m, attr, v1;
{
    return m ? (((struct menu *)LINT_CAST(m))->ops->menu_get_op)(m, attr, 
	v1) : NULL;
}


void
menu_destroy(m)
	caddr_t m; /* menu or menu_item */
{
    menu_destroy_with_proc(m, (void (*)())0);
}


void
menu_destroy_with_proc(m, destroy_proc)
	caddr_t m; /* menu or menu_item */
	void (*destroy_proc)();
{
    if (m)
	(((struct menu *)LINT_CAST(m))->ops->menu_destroy_op)(m, destroy_proc);
}


/* 
 *  Returns requested menu item position based on selection type.
 *  Always returns a legal item position.
 */
Pkg_private int
menu_curitem(m, type, default_pos)
	struct menu *m;
	Menu_attribute type;
{
    switch (type) {

      case MENU_SELECTED_ITEM:
      case MENU_SELECTED:
	if (range(m->selected_position, 1, m->nitems))
	    return m->selected_position;
	else if (m->default_position
		 && range(m->default_position, 1, m->nitems))
	    return m->default_position;
	else
	    return default_pos;
	
      case MENU_DEFAULT_ITEM:
      case MENU_DEFAULT:
	return range(m->default_position, 1, m->nitems)
	    ? m->default_position : default_pos;
	
      case NULL:  /* handles the case when the behavior wasn''t specified */
      case MENU_NOP:
      default:
	return (int)type > 0
	    && (int)type <= m->nitems ? (int)type : default_pos;
    }
}


Private caddr_t
menu_return_result(menu, dynamic, parent)
	struct menu *menu;
	struct menu_info *dynamic;
	struct menu_item *parent;
{   
    register struct menu *m;
    register struct menu_item *mi;
    struct menu *(*m_gen_proc)();
    struct menu_item *(*mi_gen_proc)();
    caddr_t v, (*notify_proc)();
    
    menu->dynamic_info = dynamic, menu->parent = parent;
    --dynamic->depth;

    if (m_gen_proc = menu->gen_proc) {
	m = (m_gen_proc)(menu, MENU_NOTIFY);
	if (m == NULL) return 0;
	m->dynamic_info = dynamic;
	m->parent = parent;
    } else m = menu; 
    
    /* 1017887 & 1011052 vmh */
    if (menu_get(m, MENU_NITEMS) <= 0) 
          {
          m = (m_gen_proc)(m, MENU_DISPLAY);
          } 
    /* 1017887 & 1011052 vmh */
    

    if (m->dynamic_info->depth < 0
	|| !range(m->selected_position, 1, m->nitems))
	m->selected_position = menu_curitem(m, m->default_selection, 1);
    mi = m->item_list[m->selected_position - 1];

    mi->selected = m->dynamic_info->depth <= 0;

    mi->parent = m;

    if (mi->inactive) {
	v = menu_return_no_value((Menu)m, (Menu_item)mi);
	goto cleanup;
    }

    if (mi_gen_proc = mi->gen_proc) {
	mi = (mi_gen_proc)(mi, MENU_NOTIFY);
	if (mi == NULL) return menu_return_no_value((Menu)menu, (Menu_item)mi);
	mi->parent = m;
    }
	    
    notify_proc =
	(m->dynamic_info->depth <= 0  && mi->notify_proc) ? mi->notify_proc
	    : m->notify_proc ? m->notify_proc : m->dynamic_info->notify_proc;
    
    v = (notify_proc)(m, mi);
    	
    if (mi_gen_proc) (mi_gen_proc)(mi, MENU_NOTIFY_DONE);

cleanup:
    if (m_gen_proc) {
	m->dynamic_info = NULL;
	
	/* 1017887 & 1011052 vmh */
	(m_gen_proc)(m, MENU_DISPLAY_DONE);
	/* 1017887 & 1011052 vmh */

	(m_gen_proc)(m, MENU_NOTIFY_DONE);
    }

    menu->dynamic_info = NULL;
    	
    return v;
}
	    

caddr_t
menu_pullright_return_result(menu_item)
	Menu_item menu_item;
{   
    register struct menu *m;
    struct menu *(*gen_proc)();
    register struct menu *mn;
    caddr_t v;
    register struct menu_item *mi = (struct menu_item *)LINT_CAST(menu_item);
    
    if (!mi || !mi->pullright) return NULL;

    m = mi->parent;

    if (gen_proc = mi->gen_pullright) {
	mn = (gen_proc)(mi, MENU_NOTIFY);
	if (mn == NULL) return menu_return_no_value((Menu)m, menu_item);
    } else {
	mn = (struct menu *)LINT_CAST(mi->value);
    }
    
    v = menu_return_result(mn, m->dynamic_info, mi);
    m->valid_result = mn->valid_result;
    
    if (gen_proc) (gen_proc)(mi, MENU_NOTIFY_DONE);
    
    return v;
}


caddr_t
menu_return_value(menu, menu_item)
	Menu menu;
	Menu_item menu_item;
{   
    register struct menu *m = (struct menu *)LINT_CAST(menu);
    register struct menu_item *mi = (struct menu_item *)LINT_CAST(menu_item);

    if (!m || !mi) {					/* No menu or item */
	if (m) m->valid_result = FALSE;
	return (caddr_t)MENU_NO_VALUE;
    }

    if (mi->pullright) return menu_pullright_return_result(menu_item);

    m->valid_result = TRUE;
    return (caddr_t)mi->value;				/* Return value */
}

    
caddr_t
menu_return_item(menu, menu_item)
	Menu menu;
	Menu_item menu_item;
{   
    register struct menu *m = (struct menu *)LINT_CAST(menu);
    register struct menu_item *mi = (struct menu_item *)LINT_CAST(menu_item);

    if (!m || !mi) {					/* No menu or item */
	if (m) m->valid_result = FALSE;
	return (caddr_t)MENU_NO_ITEM;
    }

    if (mi->pullright) return menu_pullright_return_result(menu_item);

    m->valid_result = TRUE;
    return (caddr_t)mi;					/* Return pointer */
}


/* ARGSUSED */    
caddr_t
menu_return_no_value(menu, menu_item)
	Menu menu;
	Menu_item menu_item;
{   
    struct menu *m = (struct menu *)LINT_CAST(menu);

    if (m) m->valid_result = FALSE;
    return (caddr_t)MENU_NO_VALUE;
}


/* ARGSUSED */    
caddr_t
menu_return_no_item(menu, menu_item)
	Menu menu;
	Menu_item menu_item;
{   
    struct menu *m = (struct menu *)LINT_CAST(menu);

    if (m) m->valid_result = FALSE;
    return (caddr_t)MENU_NO_ITEM;
}


/* 
 * Find the menu_item specified by the avlist.
 */
/* VARARGS1 */
Menu_item
menu_find(menu, va_alist)
	Menu menu;
	va_dcl
{   
    Menu_attribute avlist[ATTR_STANDARD_SIZE];
    va_list valist;
    register struct menu_item *mi, **mip;
    register Menu_attribute *attrs;
    register int nitems, valid;
    int submenus = FALSE, descend_first = FALSE;
    struct menu *m, *(*gen_proc)();
    struct menu_item *(*gen_item_proc)();
    struct menu *m_base = (struct menu *)LINT_CAST(menu);

    if (!m_base) return NULL;

    va_start(valist);    
    (void) attr_make((char **)LINT_CAST(avlist), ATTR_STANDARD_SIZE, valist);
    va_end(valist);

    nitems = m_base->nitems;
    for (attrs = avlist; *attrs; attrs = menu_attr_next(attrs))
	if (attrs[0] == MENU_DESCEND_FIRST) descend_first = (int)attrs[1];

    if (gen_proc = m_base->gen_proc) {
	m = (gen_proc)(m_base, MENU_DISPLAY);
	if (m == NULL) {
	    (void) fprintf(stderr,
		    "menu_find: menu's gen_proc failed to generate a menu.\n");
	    return NULL;
	}
    } else {
	m = m_base;
    }
    
    nitems = m->nitems;
    for (mip = m->item_list;valid = TRUE, mi = *mip, nitems--; mip++) {

	if (gen_item_proc = mi->gen_proc) {
	    mi = (gen_item_proc)(mi, MENU_DISPLAY);
	    if (mi == NULL) {
		(void) fprintf(stderr,
    "menu_find: menu item's gen_proc failed to generate a menu item.\n");
		goto exit;
	    }
	}
    
	for (attrs = avlist; *attrs; attrs = menu_attr_next(attrs)) {
	    switch (attrs[0]) {

	      case MENU_ACTION: /* & case MENU_NOTIFY_PROC: */
		valid &= mi->notify_proc == (caddr_t (*)())attrs[1];
		break;
	    
	      case MENU_CLIENT_DATA:
		valid &= mi->client_data == (caddr_t)attrs[1];
		break;

	      case MENU_FEEDBACK:
		valid &= !mi->no_feedback == (unsigned)attrs[1];
		break;
      
	      case MENU_FONT:
		valid &= image_get(mi->image, (struct image *)0, IMAGE_FONT) ==
		    (caddr_t)attrs[1];
		break;
	    
	      case MENU_GEN_PROC:
		valid &= mi->gen_proc == (struct menu_item *(*)())attrs[1];
		break;

	      case MENU_GEN_PULLRIGHT:
		valid &= mi->pullright &&
		    mi->gen_pullright == (struct menu *(*)())attrs[1];
		break;

	      case MENU_IMAGE:
		valid &= mi->image->pr == (struct pixrect *)attrs[1];
		break;

	      case MENU_INACTIVE:
		valid &= mi->inactive == (unsigned)attrs[1];
		break;
	    
	      case MENU_INVERT:
		valid &= mi->image && mi->image->invert == (unsigned)attrs[1];
		break;
	    
	      case MENU_LEFT_MARGIN:
		valid &= mi->image && mi->image->left_margin == (int)attrs[1];
		break;
	    
	      case MENU_MARGIN:
		valid &= mi->image && mi->image->margin == (int)attrs[1];
		break;
	    
	      case MENU_PARENT:
		valid &= mi->parent == (struct menu *)attrs[1];
		break;

	      case MENU_PULLRIGHT:
		valid &= mi->pullright && mi->value == (caddr_t)attrs[1];
		break;

	      case MENU_RIGHT_MARGIN:
		valid &= mi->image && mi->image->right_margin == (int)attrs[1];
		break;
	    
	      case MENU_STRING:
		valid &= strcmp(mi->image->string, (caddr_t)attrs[1]) == 0;
		break;

	      case MENU_VALUE:
		valid &= mi->value == (caddr_t)attrs[1];
		break;

	    }
	    if (!valid) break;
	}

	if (gen_item_proc) (gen_item_proc)(mi, MENU_DISPLAY_DONE);

	if (valid) goto exit;
	
	if (mi->pullright)
	    if (descend_first) {
		mi = (struct menu_item *)LINT_CAST(menu_find((Menu)mi->value,
						   ATTR_LIST, avlist, 0));
		if (mi) goto exit;
	    } else {
		submenus = TRUE;
	    }
    }
    
    if (submenus) {
	nitems = m->nitems;
	for (mip = m->item_list; mi = *mip, nitems--; mip++)
	    if (mi->pullright) {
		mi = (struct menu_item *)LINT_CAST(menu_find((Menu)mi->value,
						    ATTR_LIST, avlist, 0));
		if (mi) goto exit;
	    }
    }

    mi = NULL;

exit:
    if (gen_proc) (gen_proc)(m, MENU_DISPLAY_DONE);

    return  (Menu_item)mi;
}


#								ifdef needs_work
/*
 * FIXME: Needs more work.  Add small timeout for non-blocking read
 * 200msec?  Add attr for allow_accelerated_selection
 */
Private
accelerated_selection(m, fd, code)
    register struct menu *m;
{
    struct inputevent ie;
    register struct inputevent *iep = &ie;
    register int dopt, item;
    int		returncode;
    
    menu_selection = NULL;
    fcntl(fd, F_SETFL, FNDELAY);	/* Start 4.2BSD-style non-blocking I/O */
    while ( (returncode = read(fd, iep, sizeof(ie))) != -1) {
		if (iep->ie_code == code && win_inputnegevent(iep))
	    	while (m) {
				item = menu_curitem(m, m->initial_selection, 1);
				menu_selection = m->item_list[item - 1];
				if (menu_selection->pullright && menu_selection->value)
				    m = (struct menu *)menu_selection->value; /* generate? */
				else
				    m = NULL;
	    }
    dopt = fcntl(fd, F_GETFL, 0);/* Turn off non-blocking I/O */
    fcntl(fd, F_SETFL, dopt & ~FNDELAY);
}
#								endif needs_work


#ifdef no_longer_needed
menu_show_event(m, iep, fd)
	struct menu *m;
	struct inputevent *iep;
	int fd;
{   
    return (int)menu_show_using_fd(m, fd, iep);
}
#endif
