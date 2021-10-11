#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_once.c 1.1 92/07/30 Copyright 1988 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Initialization and finalization of text subwindows.
 */

#include <suntool/primal.h>
#include <suntool/textsw_impl.h>
#include <fcntl.h>
#include <varargs.h>
#include <sys/dir.h>
#include <signal.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_notify.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/window.h>
#include <suntool/walkmenu.h>
#include <sunwindow/defaults.h>
#include "sunwindow/sv_malloc.h"

#ifdef KEYMAP_DEBUG
#include "../../libsunwindow/win/win_keymap.h"
#else
#include <sunwindow/win_keymap.h>
#endif

extern Textsw_status	textsw_set();
pkg_private void	textsw_add_scrollbar_to_view();
pkg_private void	textsw_destroy_esh(), textsw_notify_replaced();
pkg_private Es_status	textsw_checkpoint();
static void		textsw_set_our_scrollbar_attrs();
extern Es_handle	ps_create(),
			es_mem_create(),
			textsw_create_mem_ps(),
			textsw_create_file_ps();
extern Ei_handle	ei_plain_text_create();
extern Ev_chain		ev_create_chain(), ev_destroy_chain_and_views();
extern Ev_status	ev_set();
extern Es_index		textsw_set_insert();
extern int		gettimeofday();

extern Ev_handle	ev_create_view();
extern Attr_avlist	attr_find();
extern Scrollbar	scrollbar_create();

#define MAXPATHLEN 1028

Textsw_folio	textsw_head;	/* = 0; implicit for cc -A-R */

DEFINE_CURSOR(textsw_cursor, 0, 0, PIX_SRC|PIX_DST,
	0x8000, 0xC000, 0xE000, 0xF000, 0xF800, 0xFC00, 0xFE00, 0xF000,
	0xD800, 0x9800, 0x0C00, 0x0C00, 0x0600, 0x0600, 0x0300, 0x0300);

/* VARARGS1 */
Textsw
textsw_init(windowfd, va_alist)
	int		  windowfd;
	va_dcl
{
	static Textsw_view	 textsw_init_internal();
	caddr_t			 attr_argv[ATTR_STANDARD_SIZE], *attrs;
	Textsw_view              view = NEW(struct textsw_view_object);
	Textsw_folio		 textsw;
	Textsw_status		 dummy_status;
	Textsw_status		*status = &dummy_status;
	va_list			 args;

	va_start(args);
	(void) attr_make(attr_argv, ATTR_STANDARD_SIZE, args);
	va_end(args);
	for (attrs = attr_argv; *attrs; attrs = attr_next(attrs)) {
	    switch ((Textsw_attribute)(*attrs)) {
	      case TEXTSW_STATUS:
		status = (Textsw_status *)LINT_CAST(attrs[1]);
		break;
	    }
	}
	if (view == 0) {
	    *status = TEXTSW_STATUS_CANNOT_ALLOCATE;
	} else {
	    view->magic = TEXTSW_VIEW_MAGIC;
	    view->window_fd = windowfd;
	    view = textsw_init_internal(view, status, (int (*)())0, attr_argv);
	    if (view) {
		textsw = FOLIO_FOR_VIEW(view);
		textsw->tool = (caddr_t)0;
	    }
	}
	return(VIEW_REP_TO_ABS(view));
}

pkg_private void
textsw_init_again(folio, count)
	register Textsw_folio	 folio;
	int			 count;
{
	register int		 i;
	register int		 old_count = folio->again_count;
	register string_t	*old_again = folio->again;

	VALIDATE_FOLIO(folio);
	folio->again_first = folio->again_last_plus_one = ES_INFINITY;
	folio->again_insert_length = 0;
	folio->again_count = count;
	folio->again = (string_t *)LINT_CAST(sv_malloc(
				(folio->again_count+1) *
				(sizeof(folio->again[0])+1)));
	for (i = 0; i < folio->again_count; i++) {
	    folio->again[i] = (i < old_count) ? old_again[i] : null_string;
	}
	for (i = folio->again_count; i < old_count; i++) {
	    textsw_free_again(folio, &old_again[i]);
	}
	if (old_again)
	    free((char *)old_again);
}

pkg_private void
textsw_init_undo(folio, count)
	register Textsw_folio	 folio;
	int			 count;
{
	register int		 i;
	register int		 old_count = folio->undo_count;
	register caddr_t	*old_undo = folio->undo;

	VALIDATE_FOLIO(folio);
	folio->undo_count = count;
	folio->undo = (caddr_t *)LINT_CAST(sv_malloc((folio->undo_count+1) *
					   (sizeof(folio->undo[0])+1)));
	for (i = 0; i < folio->undo_count; i++) {
	    folio->undo[i] =
		(i < old_count) ? old_undo[i] : ES_NULL_UNDO_MARK;
	}
	/* old_undo[ [folio->undo_count..old_count) ] are 32-bit
	 * quantities, and thus don't need to be deallocated.
	 */

        /*-----------------------------------------------------------
          ... but old_undo itself is a 260 byte quantity that should
          be deallocated to avoid a noticeable memory leak.
          This is a fix for bug 1020222.  -- Mick .
        -----------------------------------------------------------*/
        if(old_undo)
          free((char *)old_undo);

	if (old_count == 0)
	    folio->undo[0] = es_get(folio->views->esh, ES_UNDO_MARK);
}

static int
textsw_view_chain_notify(folio, attributes)
	register Textsw_folio	 folio;
	Attr_avlist	 	 attributes;
{
	pkg_private Textsw_view	 textsw_view_for_entity_view();
	register Ev_handle	 e_view;
	register Textsw_view	 view = 0;
	register Attr_avlist	 attrs;
	Rect			*from_rect, *rect, *to_rect;

	for (attrs = attributes; *attrs; attrs = attr_next(attrs)) {
	    switch ((Ev_notify_action)(*attrs)) {
	      /* BUG ALERT: following need to be fleshed out. */
	      case EV_ACTION_VIEW:
		e_view = (Ev_handle)LINT_CAST(attrs[1]);
		view = textsw_view_for_entity_view(folio, e_view);
		break;
	      case EV_ACTION_EDIT:
		if (view && (folio->notify_level & TEXTSW_NOTIFY_EDIT)) {
		    textsw_notify_replaced((Textsw_opaque)view, 
				  (Es_index)attrs[1], (Es_index)attrs[2],
				  (Es_index)attrs[3], (Es_index)attrs[4],
				  (Es_index)attrs[5]);
		}
		textsw_checkpoint(folio);
		break;
	      case EV_ACTION_PAINT:
		if (view && (folio->notify_level & TEXTSW_NOTIFY_PAINT)) {
		    rect = (Rect *)LINT_CAST(attrs[1]);
		    textsw_notify(view, TEXTSW_ACTION_PAINTED, rect, 0);
		}
		break;
	      case EV_ACTION_SCROLL:
		if (view && (folio->notify_level & TEXTSW_NOTIFY_SCROLL)) {
		    from_rect = (Rect *)LINT_CAST(attrs[1]);
		    to_rect = (Rect *)LINT_CAST(attrs[2]);
		    textsw_notify(view,
				  TEXTSW_ACTION_SCROLLED, from_rect, to_rect,
				  0);
		}
		break;
	      default:
		LINT_IGNORE(ASSERT(0));
		break;
	    }
	}
}

static int
textsw_read_defaults(textsw, defaults, font)
	register Textsw_folio	textsw;
	register Attr_avlist	defaults;
	caddr_t			font;
{
	char			*def_str;   /* Strings owned by defaults. */
#ifndef lint
	register caddr_t	attr;
#endif

	def_str = defaults_get_string("/Text/Edit_back_char", "\177", NULL);
	textsw->edit_bk_char = def_str[0];
	def_str = defaults_get_string("/Text/Edit_back_word", "\027", NULL);
	textsw->edit_bk_word = def_str[0];
	def_str = defaults_get_string("/Text/Edit_back_line", "\025", NULL);
	textsw->edit_bk_line = def_str[0];
	textsw->es_mem_maximum = defaults_get_integer_check("/Text/Memory_Maximum", 20000, 0, TEXTSW_INFINITY+1, NULL);

#ifndef lint
	if (textsw_get(TEXTSW, TEXTSW_ADJUST_IS_PENDING_DELETE))
	    textsw->state |= TXTSW_ADJUST_IS_PD;
	else	
	    textsw->state &= ~TXTSW_ADJUST_IS_PD;
	if (textsw_get(TEXTSW, TEXTSW_AUTO_INDENT))
	    textsw->state |= TXTSW_AUTO_INDENT;
	else	
	    textsw->state &= ~TXTSW_AUTO_INDENT;
	if (textsw_get(TEXTSW, TEXTSW_BLINK_CARET))
	    textsw->caret_state |= TXTSW_CARET_FLASHING;
	else	
	    textsw->caret_state &= ~TXTSW_CARET_FLASHING;
	if (textsw_get(TEXTSW, TEXTSW_CONFIRM_OVERWRITE))
	    textsw->state |= TXTSW_CONFIRM_OVERWRITE;
	else	
	    textsw->state &= ~TXTSW_CONFIRM_OVERWRITE;
	if (textsw_get(TEXTSW, TEXTSW_STORE_CHANGES_FILE))
	    textsw->state |= TXTSW_STORE_CHANGES_FILE;
	else	
	    textsw->state &= ~TXTSW_STORE_CHANGES_FILE;
	if (textsw_get(TEXTSW, TEXTSW_STORE_SELF_IS_SAVE))
	    textsw->state |= TXTSW_STORE_SELF_IS_SAVE;
	else	
	    textsw->state &= ~TXTSW_STORE_SELF_IS_SAVE;
#endif
	if (defaults_get_boolean("/Text/Retained", False, NULL))
	    textsw->state |= TXTSW_RETAINED;
	else	
	    textsw->state &= ~TXTSW_RETAINED;

#ifndef lint
	textsw->multi_click_space =
	    (int)textsw_get(TEXTSW, TEXTSW_MULTI_CLICK_SPACE);
	textsw->multi_click_timeout =
	    (int)textsw_get(TEXTSW, TEXTSW_MULTI_CLICK_TIMEOUT);
	textsw->insert_makes_visible =
	    (Textsw_enum)textsw_get(TEXTSW, TEXTSW_INSERT_MAKES_VISIBLE);
#endif

	/*
	 * The following go through the standard textsw_set mechanism
	 * (eventually) because they rely on all of the side-effects that
	 * accompany textsw_set calls.
	 */
#ifndef lint
	*defaults++ = attr = (caddr_t)TEXTSW_AGAIN_LIMIT;
	*defaults++ = textsw_get(TEXTSW, attr);
	*defaults++ = attr = (caddr_t)TEXTSW_HISTORY_LIMIT;
	*defaults++ = textsw_get(TEXTSW, attr);
	*defaults++ = attr = (caddr_t)TEXTSW_AUTO_SCROLL_BY;
	*defaults++ = textsw_get(TEXTSW, attr);
	*defaults++ = attr = (caddr_t)TEXTSW_LOWER_CONTEXT;
	*defaults++ = textsw_get(TEXTSW, attr);
	*defaults++ = attr = (caddr_t)TEXTSW_UPPER_CONTEXT;
	*defaults++ = textsw_get(TEXTSW, attr);
	*defaults++ = attr = (caddr_t)TEXTSW_FONT;
	*defaults++ = (font) ? font : textsw_get(TEXTSW, attr);
	*defaults++ = attr = (caddr_t)TEXTSW_LINE_BREAK_ACTION;
	*defaults++ = textsw_get(TEXTSW, attr);
	*defaults++ = attr = (caddr_t)TEXTSW_LEFT_MARGIN;
	*defaults++ = textsw_get(TEXTSW, attr);
	*defaults++ = attr = (caddr_t)TEXTSW_RIGHT_MARGIN;
	*defaults++ = textsw_get(TEXTSW, attr);
	*defaults++ = attr = (caddr_t)TEXTSW_TAB_WIDTH;
	*defaults++ = textsw_get(TEXTSW, attr);
	*defaults++ = attr = (caddr_t)TEXTSW_LOAD_DIR_IS_CD;
	*defaults++ = textsw_get(TEXTSW, attr);
	*defaults++ = attr = (caddr_t)TEXTSW_CONTROL_CHARS_USE_FONT;
	*defaults++ = textsw_get(TEXTSW, attr);
#endif
	def_str = defaults_get_string("/Text/Contents", "", NULL);
	if (def_str && (strlen(def_str) > 0)) {
	    *defaults++ = (caddr_t)TEXTSW_CONTENTS;
	    *defaults++ = strdup(def_str);
	}
	*defaults++ = (caddr_t)WIN_VERTICAL_SCROLLBAR;
	if (defaults_get_boolean("/Text/Scrollable", True, NULL))
	    *defaults++ = TEXTSW_DEFAULT_SCROLLBAR;
	else
	    *defaults++ = NULL;
	*defaults = 0;
}

static int stdmasks[] = { 0, -1 };
static int ctrlmasks[] = { CTRLMASK, -1 };
static int stdshiftmasks[] = { SHIFTMASK, -1 };
static int ctrlshiftmasks[] = { CTRLMASK|SHIFTMASK, -1 };
static int stdmetamasks[] = { META_SHIFT_MASK, -1 };
static int ctrlmetamasks[] = { CTRLMASK|META_SHIFT_MASK, -1 };
static int stdshiftmetamasks[] = { META_SHIFT_MASK|SHIFTMASK, -1 };
static int ctrlshiftmetamasks[] = { META_SHIFT_MASK|CTRLMASK|SHIFTMASK, -1 };

#define needctrlmask(c)		((0<=(c) && (c)<=31) || (128<=(c) && (c)<=159))
#define needmetamask(c)		(128<=(c) && (c)<=255)

static int
setmask(c)
    char c;
{
    register int i = (int) c;

    if (needmetamask(i)) {
        if (needctrlmask(i)) return CTRLMASK|META_SHIFT_MASK;
        else return META_SHIFT_MASK;
    } else {
        if (needctrlmask(i)) return CTRLMASK;
        else return 0;
    }
}

static int *
setshiftmasklist(c)
    char c;
{
    register int i = (int) c;

    if (needmetamask(i)) {
        if (needctrlmask(i)) return ctrlshiftmetamasks;
        else return stdshiftmetamasks;
    } else {
        if (needctrlmask(i)) return ctrlshiftmasks;
        else return stdshiftmasks;
    }
}

static void
textsw_adjust_keymap(fd)
	int fd;
{
	Event	e;
	int		x;
	char *def_str;
	char *def_str2;
	char erase_char, erase_word, erase_line;
	
	def_str = defaults_get_string("/Text/Edit_back_char", "\177", NULL);
	erase_char = *def_str;
	def_str = defaults_get_string("/Text/Edit_back_word", "\027", NULL);
	erase_word = *def_str;
	def_str = defaults_get_string("/Text/Edit_back_line", "\025", NULL);
	erase_line = *def_str;
	
	/*
	 * Make entries for erase_char, erase_word, and
	 * erase_line in the keymap tables. Do this even
	 * if these are the default characters.
	 */

	win_keymap_map_code_and_mask(erase_char,
			setmask(erase_char),
			ACTION_ERASE_CHAR_BACKWARD, fd);

	win_keymap_map_code_and_mask(erase_word,
			setmask(erase_word),
			ACTION_ERASE_WORD_BACKWARD, fd);

	win_keymap_map_code_and_mask(erase_line,
			setmask(erase_line),
			ACTION_ERASE_LINE_BACKWARD, fd);

/*
 *  Much of the code below has become obsolete since it was decided not to
 *  use keymapping for the arrow keys (R8, R10, R12, R14) since this was
 *  incompatible with the vi (and emacs?) notion of arrow keys as escape
 *  sequences.  These escape sequences are generated directly by the
 *  kernel keymap.  When they are enabled via /Input/Arrow_Keys = "Yes",
 *  the R8, etc. keys disappear--replaced by escape sequences which the
 *  text subwindow recognizes in textsw_event.c.
 *
 *  The code for document top (R7) and bottom (R8) along with line forward
 *  (R11) is still needed.
 */
	def_str = defaults_get_string("/Input/Arrow_Keys", "Yes", NULL);
	def_str2 = defaults_get_string(
		"/Compatibility/New_keyboard_accelerators", "Enabled", NULL);

#ifdef ecd
        if (/* !strcmp(def_str, "Yes") && */ !strcmp(def_str2, "Enabled")) {
#else
	if (!strcmp(def_str, "Yes") && !strcmp(def_str2, "Enabled")) {
#endif
	    Inputmask mask;

            def_str = defaults_get_string("/Input/Left_Handed", "No", NULL);
            if (strcmp(def_str, "Yes")) {

#ifdef ecd
	    (void)win_keymap_map_code_and_mask(KEY_RIGHT(8),
	                stdmasks, ACTION_GO_COLUMN_BACKWARD, fd);
	    (void)win_keymap_map_code_and_mask(KEY_RIGHT(12),
	                stdmasks, ACTION_GO_CHAR_FORWARD, fd);
	    (void)win_keymap_map_code_and_mask(KEY_RIGHT(14),
	                stdmasks, ACTION_GO_COLUMN_FORWARD, fd);
	    (void)win_keymap_map_code_and_mask(KEY_RIGHT(10),
	                stdmasks, ACTION_GO_CHAR_BACKWARD, fd);
#endif
	    win_setinputcodebit(&mask, KEY_RIGHT(11));
	    win_setinputcodebit(&mask, KEY_RIGHT(7));
	    win_setinputcodebit(&mask, KEY_RIGHT(13));
	    win_keymap_set_smask(fd,KEY_RIGHT(11));
	    win_keymap_set_smask(fd,KEY_RIGHT(7));
	    win_keymap_set_smask(fd,KEY_RIGHT(13));

	    win_get_kbd_mask(fd, &mask);
#ifdef ecd
	    win_setinputcodebit(&mask, KEY_RIGHT(8));
	    win_setinputcodebit(&mask, KEY_RIGHT(12));
	    win_setinputcodebit(&mask, KEY_RIGHT(14));
	    win_setinputcodebit(&mask, KEY_RIGHT(10));
#endif
	    win_setinputcodebit(&mask, KEY_RIGHT(7));
	    win_setinputcodebit(&mask, KEY_RIGHT(13));
	    win_set_kbd_mask(fd, &mask);
	    } else {
#ifdef ecd
	    (void)win_keymap_map_code_and_mask(KEY_RIGHT(8),
	                stdmasks, ACTION_GO_COLUMN_BACKWARD, fd);
	    (void)win_keymap_map_code_and_mask(KEY_RIGHT(5),
	                stdmasks, ACTION_GO_CHAR_FORWARD, fd);
	    (void)win_keymap_map_code_and_mask(KEY_RIGHT(14),
	                stdmasks, ACTION_GO_COLUMN_FORWARD, fd);
	    (void)win_keymap_map_code_and_mask(KEY_RIGHT(2),
	                stdmasks, ACTION_GO_CHAR_BACKWARD, fd);

	    win_get_kbd_mask(fd, &mask);
	    win_setinputcodebit(&mask, KEY_RIGHT(8));
	    win_setinputcodebit(&mask, KEY_RIGHT(5));
	    win_setinputcodebit(&mask, KEY_RIGHT(14));
	    win_setinputcodebit(&mask, KEY_RIGHT(2));
	    win_set_kbd_mask(fd, &mask);
#endif
	    }

	}
}
	
static Pixwin *
textsw_init_view_internal(view)
	register Textsw_view	 view;
{
	Pixwin			*pw = pw_open_monochrome(view->window_fd);

	if (pw) {
	    win_getsize(view->window_fd, &view->rect);
	    win_setcursor(view->window_fd, &textsw_cursor);

	    textsw_set_base_mask(view->window_fd);
	    
	    /* Make some defaults inspired adjustments */

	    textsw_adjust_keymap(view->window_fd);

	}
	return(pw);
}

static Textsw_status
textsw_register_view(view)
	register Textsw_view	view;
{
	extern Notify_value	textsw_notify_destroy();
	extern Notify_value	textsw_event();
	extern Notify_value	textsw_input_func();

       	if (0 != win_register((Notify_client)view, PIXWIN_FOR_VIEW(view),
		    textsw_event, textsw_notify_destroy,
		    (unsigned)(PW_INPUT_DEFAULT | (
		        (FOLIO_FOR_VIEW(view)->state & TXTSW_RETAINED)
		        ? (PW_RETAIN | PW_REPAINT_ALL)
		        : 0 )) ))
	    return(TEXTSW_STATUS_OTHER_ERROR);
	(void) notify_set_input_func((Notify_client)view, textsw_input_func,
	    view->window_fd);
	(void) fcntl(view->window_fd, F_SETFL, FNDELAY);
	return(TEXTSW_STATUS_OKAY);
}

static void
textsw_unregister_view(view)
	register Textsw_view	view;
{
	(void) notify_set_input_func((Notify_client)view, NOTIFY_FUNC_NULL,
	    view->window_fd);
	win_unregister((Notify_client)view);
}

#define	TXTSW_NEED_SELN_CLIENT	(Seln_client)1

pkg_private Textsw_view
textsw_init_internal(view, status, default_notify_proc, attrs)
	Textsw_view	  view;
	Textsw_status	 *status;
	int		(*default_notify_proc)();
	caddr_t		 *attrs;
{
	extern Menu		  textsw_menu_init();
	caddr_t			  defaults_array[ATTR_STANDARD_SIZE];
	Attr_avlist		  defaults = defaults_array;
	Attr_avlist		  attr_font;
	Es_handle		  ps_esh;
	Ei_handle		  plain_text_eih;
	Pixwin			 *pw;
	char			 *name = 0, scratch_name[MAXNAMLEN];
	Es_status		  es_status;
	register Textsw_folio	  folio = NEW(Text_object);

	if (folio == 0) {
	    *status = TEXTSW_STATUS_CANNOT_ALLOCATE;
	    goto Error_Return;
	}
	*status = TEXTSW_STATUS_OTHER_ERROR;
	folio->magic = TEXTSW_MAGIC;
	folio->first_view = view;
	view->folio = folio;
	pw = textsw_init_view_internal(view);
	if (pw == 0)
	    goto Error_Return;
	plain_text_eih = ei_plain_text_create();
	if (plain_text_eih == 0)
	    goto Error_Return;
	(void) textsw_menu_init(folio);
	/*
	 * The following go through the standard textsw_set mechanism
	 * (eventually) because they rely on all of the side-effects that
	 * accompany textsw_set calls.
	 */
	*defaults++ = (caddr_t)TEXTSW_NOTIFY_PROC;
	*defaults++ = (caddr_t)LINT_CAST(default_notify_proc);
	*defaults++ = (caddr_t)TEXTSW_INSERTION_POINT;
	*defaults++ = (caddr_t)0;
	*defaults = 0;
	/* MAS: Work-around to fix bug 1005729.
	 * Pre-scan client attrs looking for either TEXTSW_FONT or WIN_FONT.
	 */
	attr_font = attr_find(attrs, TEXTSW_FONT);
	if (*attr_font == 0) attr_font = attr_find(attrs, WIN_FONT);
	if (*attr_font) attr_font++;		/* Move over attribute to value */
	textsw_read_defaults(folio, defaults, *attr_font);
	/*
	 *   Special case the initial attributes that must be handled as
	 * part of the initial set up.  Optimizing out creating a memory
	 * entity_stream and then replacing it with a file causes most of
	 * the following complications.
	 */
	defaults = attr_find(defaults_array, TEXTSW_FONT); /* Must be there! */
	(void) ei_set(plain_text_eih, EI_FONT, defaults[1], 0);
	ATTR_CONSUME(*defaults);
	if (*attr_font == 0) folio->state |= TXTSW_OPENED_FONT;
	/*
	 * Look for client provided entity_stream creation proc, and client
	 * provided data, which must be passed to the creation proc.
	 */
	defaults = attr_find(attrs, TEXTSW_ES_CREATE_PROC);
	if (*defaults) {
	    ATTR_CONSUME(*defaults);
	    folio->es_create = (Es_handle (*)())LINT_CAST(defaults[1]);
	} else
	    folio->es_create = ps_create;
	defaults = attr_find(attrs, TEXTSW_CLIENT_DATA);
	if (*defaults) {
	    ATTR_CONSUME(*defaults);
	    folio->client_data = defaults[1];
	}
	
	defaults = attr_find(attrs, TEXTSW_MEMORY_MAXIMUM);
	if (*defaults) {
	    folio->es_mem_maximum = (unsigned)defaults[1];
	}
	
	(void) textsw_set(VIEW_REP_TO_ABS(view),
			  TEXTSW_MEMORY_MAXIMUM, folio->es_mem_maximum, 0);
	defaults = attr_find(attrs, TEXTSW_FILE);
	if (*defaults) {
	    ATTR_CONSUME(*defaults);
	    name = defaults[1];
	}
	if (name) {
	    ps_esh = textsw_create_file_ps(folio, name,
					   scratch_name, &es_status);
	    if (es_status != ES_SUCCESS) {
		/* BUG ALERT: a few diagnostics might be appreciated. */
		*status = TEXTSW_STATUS_CANNOT_OPEN_INPUT;
	    }
	} else {
	    Attr_avlist		 attr = (Attr_avlist)attrs;
	    int			 have_file_contents;
	    char		*initial_greeting;
	    
	    attr = attr_find(attrs, TEXTSW_FILE_CONTENTS);
	    have_file_contents = (*attr != 0);
	    /* Always look for TEXTSW_CONTENTS in defaults_array so that it
	     * is freed, even if it is not used, to avoid storage leak.
	     * Similarly, always consume TEXTSW_CONTENTS from attrs.
	     */
	    defaults = attr_find(defaults_array, TEXTSW_CONTENTS);
	    attr = attr_find(attrs, TEXTSW_CONTENTS);
	    initial_greeting =
		(have_file_contents) ? ""
		: ((*attr) ? attr[1]
		: ((*defaults) ? defaults[1]
		: "" ));
	    ps_esh = es_mem_create((unsigned)strlen(initial_greeting),
				   initial_greeting);
	    ps_esh = textsw_create_mem_ps(folio, ps_esh);
	    if (*defaults) {
		ATTR_CONSUME(*defaults);
		free(defaults[1]);
	    }
	    if (*attr) {
		ATTR_CONSUME(*attr);
	    }
	}
	if (ps_esh == ES_NULL)
		goto Error_Return;
	/*
	 * Make the view chain and the initial view(s).
	 */
	folio->views = ev_create_chain(ps_esh, plain_text_eih);
	(void) ev_set((Ev_handle)0, folio->views,
		      EV_CHAIN_DATA, folio, 
		      EV_CHAIN_NOTIFY_PROC, textsw_view_chain_notify,
		      EV_CHAIN_NOTIFY_LEVEL, EV_NOTIFY_ALL,
		      0);
	view->e_view = ev_create_view(folio->views, pw, &view->rect);
	if (view->e_view == EV_NULL)
	    goto Error_Return;
	/*
	 * Register procedures with the central notifier.
	 */
	if (textsw_register_view(view) != TEXTSW_STATUS_OKAY)
	    goto Error_Return;
	/*
	 * Set the default help attribute.
	 */
	folio->help_data = "textsw:textsw";
	/*
	 * Set the default, and then the client's, attributes.
	 */
	(void) textsw_set_internal(view, defaults_array);
	(void) textsw_set_internal(view, attrs);
	/*
	 * Make last_point/_adjust/_ie_time close (but not too close) to
	 *   current time to avoid overflow in tests for multi-click.
	 */
	(void) gettimeofday(&folio->last_point, (struct timezone *)0);
	folio->last_point.tv_sec -= 1000;
	folio->last_adjust = folio->last_point;
	folio->last_ie_time = folio->last_point;
	/*
	 * Final touchups.
	 */
	folio->trash = ES_NULL;
	folio->to_insert_next_free = folio->to_insert;
	folio->to_insert_counter = 0;
	folio->span_level = EI_SPAN_CHAR;
	SET_TEXTSW_TIMER(&folio->timer);
	(void) ev_init_mark(&folio->save_insert);
	folio->owed_by_filter = 0;
	/*
	 * Get the user filters in the ~/.textswrc file.
	 * Note that their description is read only once per process, and
	 *   shared among all of the folios in each process.
	 */
	if (textsw_head) {
	    folio->key_maps = textsw_head->key_maps;
	} else
	    (void) textsw_parse_rc(folio);
	/*
	 * Initialize selection service data.
	 * Note that actual hookup will only be attempted when necessary.
	 */
	folio->selection_client = TXTSW_NEED_SELN_CLIENT;
	timerclear(&folio->selection_died);
	folio->selection_func.function = SELN_FN_ERROR;
	folio->selection_holder = (Seln_holder *)0;
	*status = TEXTSW_STATUS_OKAY;
	folio->state |= TXTSW_INITIALIZED;
	/*
	 * Link this folio in.
	 */
	if (textsw_head)
	    folio->next = textsw_head;
	textsw_head = folio;
	
 	folio->temp_filename = NULL;
	if (textsw_file_name(folio, &name)) {
	    textsw_notify(view, TEXTSW_ACTION_USING_MEMORY, 0);
	} else
	    textsw_notify(view,
			  TEXTSW_ACTION_LOADED_FILE, name,
			  0);
	return(view);
Error_Return:
	free((char *)folio);
	free((char *)view);
	if (pw) pw_close(pw);
	return(0);
}

pkg_private Textsw_view
textsw_create_view(folio, rect, left_margin, right_margin)
	Textsw_folio	 folio;
	Rect		 rect;
	int		 left_margin, right_margin;
{
	struct toolsw	*toolsw;
	Textsw_status	 status;
	Pixwin		*pw;
	Textsw_view	 result = NEW(struct textsw_view_object);

	if (result) {
	    toolsw = tool_createsubwindow((struct tool *)LINT_CAST(folio->tool)
		, "", rect.r_width, rect.r_height);
	    if (toolsw == 0)
		goto Free_Result;
	    toolsw->ts_data = (caddr_t)result;
	    result->magic = TEXTSW_VIEW_MAGIC;
	    result->window_fd = toolsw->ts_windowfd;
	    result->folio = folio;
	    win_setrect(result->window_fd, &rect);
	    pw = textsw_init_view_internal(result);
	    if (pw == 0)
		goto Free_Toolsw;
	    rect.r_top = rect.r_left = 0;
	    result->e_view = ev_create_view(folio->views, pw, &rect);
	    if (result->e_view == EV_NULL)
		goto Free_Toolsw;
	    (void) ev_set(result->e_view,
			  EV_LEFT_MARGIN, left_margin,
			  EV_RIGHT_MARGIN, right_margin,
			  0);
	    (void) ev_set(result->e_view, EV_NO_REPAINT_TIL_EVENT, FALSE, 0);		  
	    status = textsw_register_view(result);
	    if (status != TEXTSW_STATUS_OKAY)
		goto Free_Toolsw;
	    result->next = folio->first_view->next;
	    folio->first_view->next = result;
	}
	goto Return;

Free_Toolsw:
	if (pw) pw_close(pw);
	tool_destroysubwindow((struct tool *)LINT_CAST(folio->tool), toolsw);
Free_Result:
	free((char *)result);
	result = (Textsw_view)0;
Return:
	return(result);
}

static void
textsw_set_our_scrollbar_attrs(view)
	Textsw_view	view;
{
	scrollbar_set(SCROLLBAR_FOR_VIEW(view),
	    SCROLL_NOTIFY_CLIENT, view,
	    SCROLL_OBJECT, view,
	    SCROLL_MARGIN, 0,
	    SCROLL_NORMALIZE, 0,
	    0);
}

pkg_private void
textsw_add_scrollbar_to_view(view, sb)
	Textsw_view	 view;
	Scrollbar	 sb;
{
	Scrollbar	 old_sb;
	Rect		 rect;
	Rect		*sb_rect;

	VALIDATE_VIEW(view);
	rect = view->e_view->rect;
	/* Remove the old scrollbar if there is one */
	if ((old_sb = SCROLLBAR_FOR_VIEW(view)) != (Scrollbar)0) {
	    /* Update the entity view rect BEFORE destroying the scrollbar */
	    sb_rect = (Rect *)
		LINT_CAST(scrollbar_get(old_sb, SCROLL_RECT));
	    rect.r_width += sb_rect->r_width;
	    if (sb_rect->r_left <= rect.r_left) {
		/* Scrollbar is West, reclaim space by moving view West */
		rect.r_left -= sb_rect->r_width;
	    }
	    scrollbar_destroy(old_sb);
	}
	/* Create the new scrollbar */
	if (sb == TEXTSW_DEFAULT_SCROLLBAR) {
	    view->scrollbar = scrollbar_create(
					SCROLL_DIRECTION, SCROLL_VERTICAL,
					0);
	} else {
	    view->scrollbar = sb;
	    if (sb == (Scrollbar)0) {
		/* Don't move view (and cause paint) iff dying */
		if ((view->state & TXTSW_VIEW_DYING) == 0)
		    (void) ev_set(view->e_view, EV_RECT, &rect, 0);
		return;
	    }
	}
	textsw_set_our_scrollbar_attrs(view);
	sb_rect =
	    (Rect *)LINT_CAST(scrollbar_get(view->scrollbar, SCROLL_RECT));
	/* Move the entity view to accomodate the scrollbar */
	if (sb_rect->r_left <= rect.r_left) {
	    /* Scrollbar is West, move view East */
	    rect.r_left += sb_rect->r_width;
	}
	/* Always subtract out the width of the scrollbar */
	rect.r_width -= sb_rect->r_width;
	(void) ev_set(view->e_view, EV_RECT, &rect, 0);
}

static void
textsw_cleanup_folio(textsw, process_death)
	register Textsw_folio	 textsw;
	int			 process_death;
{
	if (!process_death)
	    textsw_init_again(textsw, 0);	/* Flush AGAIN info */
	/* Clean up of AGAIN info requires valid esh in case of piece frees.
	 * textsw_destroy_esh may try to give Shelf to Seln. Svc., so need
	 * to keep textsw->selection_client around.
	 */
	textsw_destroy_esh(textsw, textsw->views->esh);
	if (process_death)
	    return;
	/* exit() snaps TCP connection to Seln. Svc., so it will know we are
	 * gone when the process finishes dying.
	 */
	if (textsw->selection_client &&
	    (textsw->selection_client != TXTSW_NEED_SELN_CLIENT) ) {
	    seln_destroy(textsw->selection_client);
	    textsw->selection_client = 0;
	}
	if (textsw->state & TXTSW_OPENED_FONT) {
	    PIXFONT	*font = (PIXFONT *)
	    			LINT_CAST(ei_get(textsw->views->eih, EI_FONT));
	    pf_close(font);
	}
	textsw->views->eih = ei_destroy(textsw->views->eih);
	(void) ev_destroy_chain_and_views(textsw->views);
	textsw->caret_state &= ~TXTSW_CARET_ON;
	textsw_remove_timer(textsw);
	/*
	 * Unlink the textsw from the chain.
	 */
	if (textsw == textsw_head) {
	    textsw_head = textsw->next;
	    if (textsw->next == 0) {
		/*
		 * Last textsw in process, so free key_maps.
		 */
		Key_map_handle	this_key, next_key;
		for (this_key = textsw->key_maps; this_key;
		     this_key = next_key) {
		    next_key = this_key->next;
		    free((char *)this_key);
		}
	    }
	} else {
	    Textsw_folio	temp;
	    for (temp = textsw_head; temp; temp = temp->next) {
		if (textsw == temp->next) {
		    temp->next = textsw->next;
		    break;
		}
	    }
	}

/*      This has been added to compensate missing release of memory
        for the menu package, menu_table and undo struct. There are
        more memory leaks in textsw package, but I have no more time
        to spend on it.
        This will reduce memory leak from 8k per create/destroy cirle
        to less then 1k.
*/

        if(textsw->menu) menu_destroy(textsw->menu);
        if(textsw->menu_table) free(textsw->menu_table);
        if(textsw->undo) free(textsw->undo);

	free((char *)textsw);
}

static void
textsw_destroy_view(view)
	register Textsw_view	view;
/*
 * Several pieces of the system and client code know about the
 * folio->first_view, and thus that actual node cannot be freed unless it is
 * the only node, in which case the folio must be freed as well.
 */
{
    int		 destroy_folio = FALSE;
    Textsw_folio folio = FOLIO_FOR_VIEW(view);
    Textsw_view	 temp_view;

    view->state |= TXTSW_VIEW_DYING;
    /* Warn client that view is dying BEFORE killing it. */
    if (folio->notify_level & TEXTSW_NOTIFY_DESTROY_VIEW)
	textsw_notify(view, TEXTSW_ACTION_DESTROY_VIEW, 0);
    /* Unlink view from view chain */
    if (view == folio->first_view) {
	folio->first_view = view->next;
	if (!folio->first_view)
	    destroy_folio = TRUE;
    } else {
	FORALL_TEXT_VIEWS(folio, temp_view) {
	    if (temp_view->next == view) {
		temp_view->next = view->next;
		break;
	    }
	}
    }
    /* Destroy all of the view's auxillary objects and any back links */
    textsw_add_scrollbar_to_view(view, (Scrollbar)0);
    ev_destroy(view->e_view);
    textsw_unregister_view(view);
    pw_close(PIXWIN_FOR_VIEW(view)); /* win_unregister needs valid pw */
    free((char *)view);

    if (destroy_folio)
	textsw_cleanup_folio(folio, FALSE);
}

extern void
textsw_destroy(abstract)
	Textsw	abstract;
{
	Textsw_folio	folio = FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));
	Textsw_view	view;

	/* This loop cannot use FORALL_TEXT_VIEWS because it cannot handle
	 * changes to the view linking during the iteration.
	 */
	FOREVER {
	    view = folio->first_view;
	    if (view->next) {
		textsw_destroy_view(view->next);
	    } else {
		textsw_destroy_view(view);	/* Invalidates *folio */
		break;
	    }
	}
}

extern Notify_value
textsw_notify_destroy(abstract, status)
	Textsw			abstract;
	register Destroy_status	status;
{
	register Textsw_view	view = VIEW_ABS_TO_REP(abstract);
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	register int		tool_fd;
	int			destroy_next_view_too = FALSE;

	switch (status) {
	  case DESTROY_CHECKING:
	    if (textsw_has_been_modified(abstract) &&
		(!folio->first_view->next ||
		  folio->state & TXTSW_DESTROY_ALL_VIEWS) &&
		(folio->ignore_limit != TEXTSW_INFINITY)) {
		Event	event;
		tool_fd = win_get_fd(folio->tool);
		if (tool_fd == -1) {
		    (void) notify_veto_destroy((Notify_client)abstract);
		} else {
		    extern void tool_veto_destroy();
		    int old_fcntl_flags =
			fcntl(tool_fd, F_GETFL, 0);

		    (void) fcntl(tool_fd, F_SETFL, FNDELAY);
		    (void) alert_prompt(
		            (Frame)window_get(
				WINDOW_FROM_VIEW(view), WIN_OWNER),
		            &event,
		            ALERT_MESSAGE_STRINGS,
		                "Unable to Quit.  The text has been edited.",
				"Please Save Current File or Store as New File",
				"or Undo All Edits before quiting.",
		                0,
		            ALERT_BUTTON_YES,	"Continue",
			    ALERT_TRIGGER,	ACTION_STOP,
		            0);
		    (void) fcntl(tool_fd, F_SETFL, old_fcntl_flags);
		    tool_veto_destroy((struct tool *)LINT_CAST(folio->tool));
		}
	    }
	    break;
	  case DESTROY_PROCESS_DEATH:

	    if (view == folio->first_view)
		textsw_cleanup_folio(folio, TRUE);
	    break;
	  case DESTROY_CLEANUP:
	  default:		    /* Conservative in face of new cases. */

	    destroy_next_view_too =
		(folio->state & TXTSW_DESTROY_ALL_VIEWS) &&
		folio->first_view->next;
	    textsw_destroy_view(view);
	    if (destroy_next_view_too)
		window_destroy((Window)LINT_CAST(folio->first_view));
	    break;
	}
	return(NOTIFY_DONE);
}

