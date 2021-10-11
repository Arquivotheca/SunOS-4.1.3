#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_menu.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *  Text subwindow menu creation and support.
 */

#include <suntool/primal.h>

#include <suntool/textsw_impl.h>
#include <errno.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>
#include <sunwindow/defaults.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_struct.h>
#include "sunwindow/sv_malloc.h"
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/fullscreen.h>
#include <suntool/tool_struct.h>
#include <suntool/walkmenu.h>
#include <suntool/window.h>

#define HELP_INFO(s) HELP_DATA,s,

extern Ev_handle	ev_resolve_xy_to_view();
extern Ev_handle	ev_nearest_view();
extern Es_index		ev_index_for_line();

static void		stuff_clipboard();
static Seln_result	read_proc();
static FILE		*output;

static Menu_item 	show_clipboard();

static char		*put_then_get = "Copy then Paste";
static char		*put_then     = "Copy then";

#define			MAX_SIZE_MENU_STRING	30

static struct pixrect	*textsw_get_only_pr;

static int		textsw_menu_refcount;	/* refcount for textsw_menu */
static Menu		textsw_menu;		/* Let default to NULL */
static Textsw_enum 	textsw_menu_style;	/* default to organized */
static int		textsw_menu_style_already_set; /* default = 0 */
static Menu_item	*textsw_menu_items;	/* [TEXTSW_MENU_LAST_CMD] */
pkg_private void	textsw_alert_for_old_interface();

extern int		textsw_has_been_modified();
extern void		tool_expand_neighbors();
pkg_private void	textsw_get_extend_to_edge();
pkg_private void	textsw_set_extend_to_edge();

extern Textsw		textsw_split_view();
pkg_private int		await_neg_event();
extern Textsw_enum	textsw_get_menu_style_internal();
extern void		textsw_set_menu_style_internal();
pkg_private int		textsw_menu_prompt();
extern void		textsw_destroy_split();
extern Menu_item *	textsw_new_menu_table();
extern Menu		textsw_menu_init();
extern void		textsw_do_menu();
extern Menu		textsw_get_unique_menu();
extern Menu		textsw_build_extras_menu();
extern Menu_item	textsw_toggle_extras_item();

/* VARARGS0 */
static Menu
textsw_new_menu(folio)
	Textsw_folio	folio;
{
    Menu edit_cmd, top_menu, save_file, break_mode, old_break_mode,
	find_pattern,
	file_cmds, display_cmds, find_cmds, fast_exits_cmds,
	select_field_cmds, find_sel_cmds, build_clipboard,
	find_clipboard_cmds, undo_cmds, paste_cmds, save_cmds, old_edit_cmd;
	
    char *string_for_menu_item = sv_malloc(MAX_SIZE_MENU_STRING + 1);

    old_break_mode = menu_create(
	MENU_LEFT_MARGIN, 6,
	MENU_ITEM,
	    MENU_STRING,	"Clip Lines",
	    MENU_VALUE,		TEXTSW_MENU_CLIP_LINES,
	    HELP_INFO("textsw:mcliplines")
	    0,
	MENU_ITEM,
	    MENU_STRING,	"Wrap at Character",
	    MENU_VALUE,		TEXTSW_MENU_WRAP_LINES_AT_CHAR,
	    HELP_INFO("textsw:mwrapchars")
	    0,
	HELP_INFO("textsw:moldbreakmode")
	0);

    break_mode = menu_create(
	MENU_LEFT_MARGIN, 6,
	MENU_ITEM,
	    MENU_STRING,	"Wrap at Character",
	    MENU_VALUE,		TEXTSW_MENU_WRAP_LINES_AT_CHAR,
	    HELP_INFO("textsw:mwrapchars")
	    0,
	MENU_ITEM,
	    MENU_STRING,	"Wrap at Word",
	    MENU_VALUE,		TEXTSW_MENU_WRAP_LINES_AT_WORD,
	    HELP_INFO("textsw:mwrapwords")
	    0,
	MENU_ITEM,
	    MENU_STRING,	"Clip Lines",
	    MENU_VALUE,		TEXTSW_MENU_CLIP_LINES,
	    HELP_INFO("textsw:mcliplines")
	    0,
	HELP_INFO("textsw:mbreakmode")
	0);
	
    undo_cmds = menu_create(
	MENU_LEFT_MARGIN, 6,
	MENU_ITEM,
	    MENU_STRING,	"Undo Last Edit",
	    MENU_VALUE,		TEXTSW_MENU_UNDO,
	    HELP_INFO("textsw:mundolast")
	    0,
	MENU_ITEM,
	    MENU_STRING,	"Undo All Edits",
	    MENU_VALUE,		TEXTSW_MENU_UNDO_ALL,
	    HELP_INFO("textsw:mundoall")
	    0,
	HELP_INFO("textsw:mundocmds")
	0);

    if (textsw_menu_style == TEXTSW_STYLE_ORGANIZED) {
	fast_exits_cmds = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_ITEM,
	        MENU_STRING,	"Close, then Save Current File",
		MENU_VALUE,	TEXTSW_MENU_SAVE_CLOSE,
		HELP_INFO("textsw:msaveclose")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Save Current File, then Quit",
		MENU_VALUE,	TEXTSW_MENU_SAVE_QUIT,
		HELP_INFO("textsw:msavequit")
		0,
	    MENU_ITEM,
		MENU_STRING, 	"Save, then Empty document",
		MENU_VALUE,	TEXTSW_MENU_SAVE_RESET,
		MENU_LINE_AFTER_ITEM,	MENU_HORIZONTAL_LINE,
		HELP_INFO("textsw:msavereset")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Close, then Store as New File",
		MENU_VALUE,	TEXTSW_MENU_STORE_CLOSE,
		HELP_INFO("textsw:mstoreclose")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Store as New File, then Quit",
		MENU_VALUE,	TEXTSW_MENU_STORE_QUIT,
		HELP_INFO("textsw:mstorequit")
		0,
	    HELP_INFO("textsw:mfastexitcmds")
	    0);

	select_field_cmds = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_ITEM,
		MENU_STRING,	"Expand",
		MENU_VALUE,	TEXTSW_MENU_SEL_ENCLOSE_FIELD,
		HELP_INFO("textsw:mselexpand")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Next",
		MENU_VALUE,	TEXTSW_MENU_SEL_NEXT_FIELD,
		HELP_INFO("textsw:mselnext")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Previous",
		MENU_VALUE,	TEXTSW_MENU_SEL_PREV_FIELD,
		HELP_INFO("textsw:mselprevious")
		0,
	    HELP_INFO("textsw:mselfieldcmds")
	    0);

	find_sel_cmds = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_ITEM,
		MENU_STRING,	"Forward",
		MENU_VALUE,	TEXTSW_MENU_FIND,
		HELP_INFO("textsw:mfindforward")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Backward",
		MENU_VALUE,	TEXTSW_MENU_FIND_BACKWARD,
		HELP_INFO("textsw:mfindbackward")
		0,
	    HELP_INFO("textsw:mfindselcmds")
	    0);

	find_clipboard_cmds = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_ITEM,
		MENU_STRING,	"Forward",
		MENU_VALUE,			TEXTSW_MENU_FIND_SHELF,
		HELP_INFO("textsw:mfindcbforward")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Backward",
		MENU_VALUE,			TEXTSW_MENU_FIND_SHELF_BACKWARD,
		HELP_INFO("textsw:mfindcbbackward")
		0,
	    HELP_INFO("textsw:mfindcbcmds")
	    0);
/*
	paste_cmds = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_STRING_ITEM, "Paste",			TEXTSW_MENU_PASTE,
	    MENU_STRING_ITEM, "Paste from File",	TEXTSW_MENU_FILE_STUFF,
	    0);

	save_cmds = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_STRING_ITEM,    "Save File",		TEXTSW_MENU_SAVE,
	    MENU_STRING_ITEM,    "Store to Named file",	TEXTSW_MENU_STORE,
	    MENU_PULLRIGHT_ITEM, "Fast Exits",		fast_exits_cmds,
	    0);
*/
	file_cmds = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_ITEM,
		MENU_STRING,    "Save Current File",
		MENU_VALUE,	TEXTSW_MENU_SAVE,
		HELP_INFO("textsw:msavefile")
		0,
	    MENU_ITEM, 
		MENU_STRING,    "Store as New File",
 		MENU_VALUE,	TEXTSW_MENU_STORE,
		MENU_LINE_AFTER_ITEM,	MENU_HORIZONTAL_LINE,
		HELP_INFO("textsw:mstorefile")
		0,
	    MENU_ITEM,
		MENU_STRING,    "Load File",	
		MENU_VALUE,	TEXTSW_MENU_LOAD,
		HELP_INFO("textsw:mloadfile")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Include File",       
		MENU_VALUE,	TEXTSW_MENU_FILE_STUFF,
		HELP_INFO("textsw:mincludefile")
		0,
	    MENU_ITEM,
		MENU_STRING,    "Set Directory",
		MENU_VALUE,	TEXTSW_MENU_CD,
		MENU_LINE_AFTER_ITEM,	MENU_HORIZONTAL_LINE,
		HELP_INFO("textsw:msetdir")
		0,
	    MENU_ITEM,
		MENU_STRING,    "Empty Document",
		MENU_VALUE,	TEXTSW_MENU_RESET,
		HELP_INFO("textsw:memptydoc")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Finishing Up",
		MENU_PULLRIGHT,	fast_exits_cmds,
		HELP_INFO("textsw:mfastexit")
		0,
	    HELP_INFO("textsw:mfilecmds")
	    0);

	build_clipboard = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_ITEM,
		MENU_GEN_PROC,	show_clipboard,
		MENU_STRING,	string_for_menu_item,
		MENU_VALUE,	TEXTSW_MENU_SHOW_CLIPBOARD,
		HELP_INFO("textsw:mclipboard")
		0,
	    HELP_INFO("textsw:mclipboard")
	    0);

	edit_cmd = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_ITEM,
		MENU_STRING,    "Again",
		MENU_VALUE,	TEXTSW_MENU_AGAIN,
		HELP_INFO("textsw:meditagain")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Undo",
		MENU_PULLRIGHT,  undo_cmds,
		MENU_LINE_AFTER_ITEM,	MENU_HORIZONTAL_LINE,
		HELP_INFO("textsw:meditundo")
 		0,
	    MENU_ITEM,
		MENU_STRING,    "Copy",
		MENU_VALUE,	TEXTSW_MENU_COPY, 
		HELP_INFO("textsw:meditcopy")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Paste",
		MENU_VALUE,	TEXTSW_MENU_PASTE,
		HELP_INFO("textsw:meditpaste")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Cut",
		MENU_VALUE,	TEXTSW_MENU_CUT,
		MENU_LINE_AFTER_ITEM,	MENU_HORIZONTAL_LINE,
		HELP_INFO("textsw:meditcut")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Show Clipboard",
		MENU_VALUE,	TEXTSW_MENU_SHOW_CLIPBOARD_FROM_EDIT,
		MENU_PULLRIGHT,  build_clipboard,
		HELP_INFO("textsw:mclipboard")
		0,
	    MENU_ITEM,
		MENU_STRING,    "Copy, then Paste",
		MENU_VALUE,	TEXTSW_MENU_PUT_THEN_GET,
		HELP_INFO("textsw:meditcopypaste")
		0,
	    HELP_INFO("textsw:meditcmds")
	    0);

	display_cmds = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_ITEM,
		MENU_STRING,	"Select Line at Number",
		MENU_VALUE,	TEXTSW_MENU_NORMALIZE_LINE,
		HELP_INFO("textsw:mselectline")
		0,
	    MENU_ITEM,
		MENU_STRING,	"What Line Number?",
		MENU_VALUE,	TEXTSW_MENU_COUNT_TO_LINE,
		MENU_LINE_AFTER_ITEM,	MENU_HORIZONTAL_LINE,
		HELP_INFO("textsw:mwhatline")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Split View",
		MENU_VALUE,	TEXTSW_MENU_CREATE_VIEW,
		HELP_INFO("textsw:msplitview")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Destroy View",
		MENU_VALUE,	TEXTSW_MENU_DESTROY_VIEW,
		MENU_LINE_AFTER_ITEM,	MENU_HORIZONTAL_LINE,
		HELP_INFO("textsw:mdestroyview")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Show Caret at Top",
		MENU_VALUE,	TEXTSW_MENU_NORMALIZE_INSERTION,
		HELP_INFO("textsw:mshowcaret")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Move Caret to Start",
		MENU_VALUE,	TEXTSW_MENU_NORMALIZE_TOP,
		HELP_INFO("textsw:mcaretstart")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Move Caret to End",
		MENU_VALUE,	TEXTSW_MENU_NORMALIZE_BOTTOM, 
		MENU_LINE_AFTER_ITEM,	MENU_HORIZONTAL_LINE,
		HELP_INFO("textsw:mcaretend")
		0,
	    MENU_ITEM,
	        MENU_STRING,	"Change Line Wrap",
		MENU_PULLRIGHT,	break_mode,
		HELP_INFO("textsw:mchangelinewrap")
		0,
	    HELP_INFO("textsw:mdisplaycmds")
	    0);

	find_cmds = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_ITEM,
		MENU_STRING,	"Find and Replace",
		MENU_VALUE,	TEXTSW_MENU_FIND_AND_REPLACE,
		MENU_LINE_AFTER_ITEM,	MENU_HORIZONTAL_LINE,
		HELP_INFO("textsw:mfindreplace")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Find Selection",
		MENU_PULLRIGHT,	find_sel_cmds,
		HELP_INFO("textsw:mfindselcmds")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Find Clipboard",
		MENU_PULLRIGHT,	find_clipboard_cmds,
		HELP_INFO("textsw:findcbcmds")
		0,
	    MENU_ITEM,
		MENU_STRING,     "Show Clipboard",
		MENU_VALUE,      TEXTSW_MENU_SHOW_CLIPBOARD_FROM_FIND,
		MENU_PULLRIGHT,  build_clipboard,
		MENU_LINE_AFTER_ITEM,	MENU_HORIZONTAL_LINE,
		HELP_INFO("textsw:mclipboard")
		0,
	    MENU_ITEM,
		MENU_STRING,    "Find Marked Text",
		MENU_VALUE,	TEXTSW_MENU_SEL_MARK_TEXT,
		HELP_INFO("textsw:mfindtext")
		0,
	    MENU_ITEM,
		MENU_STRING,    "Match Delimiter",
		MENU_VALUE,	TEXTSW_MENU_MATCH_FIELD,
		HELP_INFO("textsw:mmatchdelim")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Replace |>field<| ",
		MENU_PULLRIGHT,	select_field_cmds,
		HELP_INFO("textsw:mselfieldcmds")
		0,
	    HELP_INFO("textsw:mfindcmds")
	    0);

	top_menu = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_ITEM,
		MENU_STRING,	"File",
		MENU_PULLRIGHT,	file_cmds,
		HELP_INFO("textsw:mfilecmds")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Edit",
		MENU_PULLRIGHT,	edit_cmd,
		HELP_INFO("textsw:meditcmds")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Display",
		MENU_PULLRIGHT,	display_cmds,
		HELP_INFO("textsw:mdisplaycmds")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Find",
		MENU_PULLRIGHT,	find_cmds,
		HELP_INFO("textsw:mfindcmds")
		0,
	    MENU_ITEM,
	        MENU_GEN_PROC,	textsw_toggle_extras_item,
		MENU_STRING,	"Extras",
		MENU_INACTIVE,	TRUE,
		MENU_GEN_PULLRIGHT,	textsw_build_extras_menu,
		MENU_CLIENT_DATA,	VIEW_FROM_FOLIO_OR_VIEW(folio),
		HELP_INFO("textsw:mextras")
		0,
	    HELP_INFO("textsw:mtopmenu")
	    0);
    } else {
	save_file = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_ITEM,
		MENU_STRING,	"Save file",
		MENU_VALUE,	TEXTSW_MENU_SAVE,
		HELP_INFO("textsw:msavefile")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Save then Quit",
		MENU_VALUE,	TEXTSW_MENU_SAVE_QUIT,
		HELP_INFO("textsw:msavequit")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Save then Reset",
		MENU_VALUE,	TEXTSW_MENU_SAVE_RESET,
		HELP_INFO("textsw:msavereset")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Close then Save",
		MENU_VALUE,	TEXTSW_MENU_SAVE_CLOSE, 
		HELP_INFO("textsw:msaveclose")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Store to named file",
		MENU_VALUE,	TEXTSW_MENU_STORE,
		HELP_INFO("textsw:mstorefile")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Store then Quit",
		MENU_VALUE,	TEXTSW_MENU_STORE_QUIT,
		HELP_INFO("textsw:mstorequit")
		0,
	    MENU_ITEM,
 		MENU_STRING,	"Close then Store",
		MENU_VALUE,	TEXTSW_MENU_STORE_CLOSE,
		HELP_INFO("textsw:mstoreclose")
		0,
	    HELP_INFO("textsw:msave")
	    0);

	find_pattern = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_ITEM,
		MENU_STRING,	"Find, forward",
		MENU_VALUE,	TEXTSW_MENU_FIND,
		HELP_INFO("textsw:mfindforward")
		0,
	    MENU_ITEM,
	        MENU_STRING,	"Find, backward",
		MENU_VALUE,	TEXTSW_MENU_FIND_BACKWARD,
		HELP_INFO("testsw:mfindbackward")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Find Shelf, forward",
		MENU_VALUE,	TEXTSW_MENU_FIND_SHELF,
		HELP_INFO("textsw:mfindcbforward")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Find Shelf, backward",
		MENU_VALUE,	TEXTSW_MENU_FIND_SHELF_BACKWARD,
		HELP_INFO("textsw:findcbbackward")
		0,
#ifdef _TEXTSW_FIND_RE
	    MENU_ITEM,
		MENU_STRING,	"Find RE, forward",
		MENU_VALUE,	TEXTSW_MENU_FIND_RE,
		HELP_INFO("textsw:mfindreforward")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Find RE, backward",
		MENU_VALUE,	TEXTSW_MENU_FIND_RE_BACKWARD,
		HELP_INFO("textsw:mfindrebackward")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Find tag, forward",
		MENU_VALUE,	TEXTSW_MENU_FIND_TAG,
		HELP_INFO("textsw:mfindtagforward")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Find tag, backward",
		MENU_VALUE,	TEXTSW_MENU_FIND_TAG_BACKWARD, 
		HELP_INFO("textsw:mfindtagbackward")
		0,
#endif
	    HELP_INFO("textsw:mfindpattern")
	    0);

/*	old_edit_cmd = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_STRING_ITEM,    "Again",		TEXTSW_MENU_AGAIN,
	    MENU_PULLRIGHT_ITEM, "Undo",		undo_cmds,
	    MENU_STRING_ITEM,    "Copy", 		TEXTSW_MENU_COPY, 
	    MENU_PULLRIGHT_ITEM, "Paste",		paste_cmds,
	    MENU_STRING_ITEM,    "Cut",			TEXTSW_MENU_CUT,
	    MENU_STRING_ITEM,    "Copy then Paste",	TEXTSW_MENU_PUT_THEN_GET,
	    0);
*/
	top_menu = menu_create(
	    MENU_LEFT_MARGIN, 6,
	    MENU_ITEM,
		MENU_STRING,	"Save",
		MENU_PULLRIGHT,	save_file,
		HELP_INFO("textsw:msavefile")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Load file",
		MENU_VALUE,	TEXTSW_MENU_LOAD,
		HELP_INFO("textsw:mloadfile")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Select line #",
		MENU_VALUE,	TEXTSW_MENU_NORMALIZE_LINE,
		HELP_INFO("textsw:mselectline")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Split view",
		MENU_VALUE,	TEXTSW_MENU_CREATE_VIEW,
		HELP_INFO("textsw:msplitview")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Destroy view",
		MENU_VALUE,	TEXTSW_MENU_DESTROY_VIEW,
		HELP_INFO("textsw:mdestroyview")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Reset",
		MENU_VALUE,	TEXTSW_MENU_RESET,
		HELP_INFO("textsw:memptydoc")
		0,
	    MENU_ITEM,
		MENU_STRING,    "What line #?",
		MENU_VALUE,	TEXTSW_MENU_COUNT_TO_LINE,
		HELP_INFO("textsw:mwhatline")
		0,
	    MENU_ITEM,
		MENU_STRING,    "Get from file",
		MENU_VALUE,	TEXTSW_MENU_FILE_STUFF,
		HELP_INFO("textsw:mincludefile")
		0,
	    MENU_ITEM,
		MENU_STRING,    "Caret to top",
		MENU_VALUE,	TEXTSW_MENU_NORMALIZE_INSERTION,
		HELP_INFO("textsw:mshowcaret")
		0,
#ifdef notdef
	    MENU_STRING_ITEM,    "Selection to top",
					          TEXTSW_MENU_NORMALIZE_SELECTION,
#endif
	    MENU_ITEM,
		MENU_STRING,	"Line break",
		MENU_PULLRIGHT,	old_break_mode, 
		HELP_INFO("textsw:moldbreakmode")
		0,
	    MENU_ITEM,
		MENU_STRING,    "Set directory",
		MENU_VALUE,	TEXTSW_MENU_CD,
		HELP_INFO("textsw:msetdir")
		0,
	    MENU_ITEM,
		MENU_STRING,	"Find",
		MENU_PULLRIGHT,	find_pattern,
	        HELP_INFO("textsw:mfindpattern")
		0,
	    MENU_ITEM,
		MENU_STRING,    put_then_get,
		MENU_VALUE,	TEXTSW_MENU_PUT_THEN_GET,
		HELP_INFO("textsw:meditcopypaste")
		0,
/* since these are the OLD menus, we won't even give them these two cmds
	    MENU_STRING_ITEM,    "Match",         TEXTSW_MENU_MATCH_FIELD,
	    MENU_PULLRIGHT_ITEM, "Edit", 	  old_edit_cmd,
*/
	    HELP_INFO("textsw:mtopmenu")
	    0);
    }
    (void)menu_set(top_menu, MENU_CLIENT_DATA, textsw_menu_style, 0);
    return top_menu;
}

static void
stuff_clipboard()
{
    Seln_holder holder;
    Seln_rank rank = SELN_SHELF;
    char context = 0; /* initially =0(first time), change to 1 afterwards */
    char *destination = "/dev/console";

    if ((output = fopen(destination, "w")) == NULL) return;
    holder = seln_inquire(rank);
    if (holder.state == SELN_NONE) {
	fputs("\nThe clipboard is currently empty.", output);
	return;
    }
    seln_query(&holder, read_proc, &context, SELN_REQ_CONTENTS_ASCII, 0, 0);
    fclose(output);
}

static Menu_item
show_clipboard(item, operation)
    Menu_item	  item;
    Menu_generate operation;
{
    Seln_holder		holder;
    Seln_request	*buffer;
    Seln_rank 		rank = SELN_SHELF;
    char		*string_for_menu_item;
    int			i;

    switch (operation) {

	case MENU_DISPLAY: /* build string menu here */
	  string_for_menu_item = menu_get(item, MENU_STRING, 0);
	  for (i=0; i <= MAX_SIZE_MENU_STRING; i++) {
	      string_for_menu_item[i] = NULL;
	  }
	  menu_set(item, MENU_STRING, string_for_menu_item, 0); /* default */
	  holder = seln_inquire(rank);
	  if (holder.state == SELN_NONE) {
	      break; /* return item */
    	  }
	  buffer = seln_ask(&holder, SELN_REQ_CONTENTS_ASCII, 0, 0);
	  if (buffer == (Seln_request *) NULL ||
	    *((Seln_attribute *)
	      (LINT_CAST(buffer->data))) != SELN_REQ_CONTENTS_ASCII) {
	      break; /* all done, nothing to look at, later post message
			saying "Selection holder error--..." noting failure */
	  }
	  strncpy( /* and possibly truncate buffer to max size allowed */
	      string_for_menu_item,                   /* destination */
	      buffer->data + sizeof (Seln_attribute), /* source */
	      MAX_SIZE_MENU_STRING);	              /* num chars */
	  menu_set(item, MENU_STRING, string_for_menu_item, 0);
	  menu_set(item, MENU_INACTIVE, FALSE, 0);    /*not greyed out */
	  menu_set(item, MENU_FEEDBACK, FALSE, 0); /* no invert,no select */
	  break;

	case MENU_DISPLAY_DONE:
	  break;

	case MENU_NOTIFY:
	case MENU_NOTIFY_DONE:
	  break;

	default: 
	  break; 
    }
    return item; 
}

static Seln_result
read_proc(buffer)
    Seln_request         *buffer;
{
    char	         *destination = "/dev/console";
    char		 *intro_message = "\nCurrent clipboard: ";
    char		 *error = "Selection holder error -- unrecognized request\n";
    char                 *reply;

    if (*buffer->requester.context == 0) {
	if (buffer == (Seln_request *) NULL ||
	    *((Seln_attribute *)
	      (LINT_CAST(buffer->data))) != SELN_REQ_CONTENTS_ASCII) {
	    fputs(error, output);
	    fclose(output);
	    return(SELN_UNRECOGNIZED);
	}
	fputs(intro_message, output);
	reply = buffer->data + sizeof (Seln_attribute);
	*buffer->requester.context = 1;
    } else {
	reply = buffer->data;
    }
    fputs(reply, output);
    (void)fflush(output);
    return SELN_SUCCESS;
}

extern Menu_item *
textsw_new_menu_table(textsw)
    Textsw_folio textsw;
{
    int textsw_do_menu_action();
    int item;
    Menu_item menu_item, *menu_items;
    Menu top_menu = textsw->menu, save_file, edit_cmd, break_mode, 
	find_pattern, filing_cmd, display_cmd, exits_cmd, undo_cmd,
	paste_cmd, show_clip_cmd, find_sel_cmd, find_clip_cmd, save_cmd, 
	search_field_cmd;

    menu_items = (Menu_item *)LINT_CAST(sv_calloc((unsigned)TEXTSW_MENU_LAST_CMD,
	sizeof(Menu_item)));

    if (textsw_menu_style == TEXTSW_STYLE_ORGANIZED) {
	menu_item = (Menu_item)menu_get(top_menu, MENU_NTH_ITEM, 1);
	filing_cmd = (Menu)menu_get(menu_item, MENU_VALUE);
	menu_item = (Menu_item)menu_get(top_menu, MENU_NTH_ITEM, 2);
	edit_cmd = (Menu)menu_get(menu_item, MENU_VALUE);
    	menu_item = (Menu_item)menu_get(top_menu, MENU_NTH_ITEM, 3);
    	display_cmd = (Menu)menu_get(menu_item, MENU_VALUE);
    	menu_item = (Menu_item)menu_get(top_menu, MENU_NTH_ITEM, 4);
    	find_pattern = (Menu)menu_get(menu_item, MENU_VALUE);

	menu_item = (Menu_item)menu_get(edit_cmd, MENU_NTH_ITEM, 2);
	undo_cmd = (Menu)menu_get(menu_item, MENU_VALUE);

   	menu_item = (Menu_item)menu_get(filing_cmd, MENU_NTH_ITEM, 7);
    	exits_cmd = (Menu)menu_get(menu_item, MENU_VALUE);

    	menu_item = (Menu_item)menu_get(edit_cmd, MENU_NTH_ITEM, 6);
   	show_clip_cmd = (Menu)menu_get(menu_item, MENU_VALUE);

	menu_item = (Menu_item)menu_get(display_cmd, MENU_NTH_ITEM, 8);
    	break_mode = (Menu)menu_get(menu_item, MENU_VALUE);
    	
    	menu_item = (Menu_item)menu_get(find_pattern, MENU_NTH_ITEM, 2);
    	find_sel_cmd = (Menu)menu_get(menu_item, MENU_VALUE);
    	menu_item = (Menu_item)menu_get(find_pattern, MENU_NTH_ITEM, 3);
    	find_clip_cmd = (Menu)menu_get(menu_item, MENU_VALUE);
    	menu_item = (Menu_item)menu_get(find_pattern, MENU_NTH_ITEM, 7);
    	search_field_cmd = (Menu)menu_get(menu_item, MENU_VALUE);

	menu_items[(int)TEXTSW_MENU_SAVE_CLOSE] =
	    menu_get(exits_cmd, MENU_NTH_ITEM, 1);
	menu_items[(int)TEXTSW_MENU_SAVE_QUIT] =
	    menu_get(exits_cmd, MENU_NTH_ITEM, 2);
	menu_items[(int)TEXTSW_MENU_SAVE_RESET] =
	    menu_get(exits_cmd, MENU_NTH_ITEM, 3);
	menu_items[(int)TEXTSW_MENU_STORE_CLOSE] =
	    menu_get(exits_cmd, MENU_NTH_ITEM, 4);
	menu_items[(int)TEXTSW_MENU_STORE_QUIT] =
	    menu_get(exits_cmd, MENU_NTH_ITEM, 5);

	menu_items[(int)TEXTSW_MENU_SAVE] =
	    menu_get(filing_cmd, MENU_NTH_ITEM, 1);
	menu_items[(int)TEXTSW_MENU_STORE] =
	    menu_get(filing_cmd, MENU_NTH_ITEM, 2);
	menu_items[(int)TEXTSW_MENU_LOAD] =
	    menu_get(filing_cmd, MENU_NTH_ITEM, 3);
	menu_items[(int)TEXTSW_MENU_FILE_STUFF] =
	    menu_get(filing_cmd, MENU_NTH_ITEM, 4);
	menu_items[(int)TEXTSW_MENU_CD] =
	    menu_get(filing_cmd, MENU_NTH_ITEM, 5);
	menu_items[(int)TEXTSW_MENU_RESET] =
	    menu_get(filing_cmd, MENU_NTH_ITEM, 6);

	menu_items[(int)TEXTSW_MENU_UNDO] =
	    menu_get(undo_cmd, MENU_NTH_ITEM, 1);
	menu_items[(int)TEXTSW_MENU_UNDO_ALL] =
	    menu_get(undo_cmd, MENU_NTH_ITEM, 2);

	menu_items[(int)TEXTSW_MENU_AGAIN] =
	    menu_get(edit_cmd, MENU_NTH_ITEM, 1);
	menu_items[(int)TEXTSW_MENU_COPY] =
	    menu_get(edit_cmd, MENU_NTH_ITEM, 3);
	menu_items[(int)TEXTSW_MENU_PASTE] =
	    menu_get(edit_cmd, MENU_NTH_ITEM, 4);
        menu_items[(int)TEXTSW_MENU_CUT] =
	    menu_get(edit_cmd, MENU_NTH_ITEM, 5);
	menu_items[(int)TEXTSW_MENU_SHOW_CLIPBOARD_FROM_EDIT] =
	    menu_get(edit_cmd, MENU_NTH_ITEM, 6);
	menu_items[(int)TEXTSW_MENU_PUT_THEN_GET] =
	    menu_get(edit_cmd, MENU_NTH_ITEM, 7);

	menu_items[(int)TEXTSW_MENU_SHOW_CLIPBOARD] =
	    menu_get(show_clip_cmd, MENU_NTH_ITEM, 1);

	menu_items[(int)TEXTSW_MENU_NORMALIZE_LINE] =
	    menu_get(display_cmd, MENU_NTH_ITEM, 1);
	menu_items[(int)TEXTSW_MENU_COUNT_TO_LINE] =
	    menu_get(display_cmd, MENU_NTH_ITEM, 2);
	menu_items[(int)TEXTSW_MENU_CREATE_VIEW] =
	    menu_get(display_cmd, MENU_NTH_ITEM, 3);
	menu_items[(int)TEXTSW_MENU_DESTROY_VIEW] =
	    menu_get(display_cmd, MENU_NTH_ITEM, 4);
	menu_items[(int)TEXTSW_MENU_NORMALIZE_INSERTION] =
	    menu_get(display_cmd, MENU_NTH_ITEM, 5);
	menu_items[(int)TEXTSW_MENU_NORMALIZE_TOP] =
	    menu_get(display_cmd, MENU_NTH_ITEM, 6);
	menu_items[(int)TEXTSW_MENU_NORMALIZE_BOTTOM] =
	    menu_get(display_cmd, MENU_NTH_ITEM, 7);

	menu_items[(int)TEXTSW_MENU_WRAP_LINES_AT_CHAR] =
	    menu_get(break_mode, MENU_NTH_ITEM, 1);
	menu_items[(int)TEXTSW_MENU_WRAP_LINES_AT_WORD] =
	    menu_get(break_mode, MENU_NTH_ITEM, 2);
	menu_items[(int)TEXTSW_MENU_CLIP_LINES] =
	    menu_get(break_mode, MENU_NTH_ITEM, 3);
	    
	menu_items[(int)TEXTSW_MENU_FIND_AND_REPLACE] =
	    menu_get(find_pattern, MENU_NTH_ITEM, 1);
	menu_items[(int)TEXTSW_MENU_SHOW_CLIPBOARD_FROM_FIND] =
	    menu_get(find_pattern, MENU_NTH_ITEM, 4);
	 menu_items[(int)TEXTSW_MENU_SEL_MARK_TEXT] =
	    menu_get(find_pattern, MENU_NTH_ITEM, 5);   
	menu_items[(int)TEXTSW_MENU_MATCH_FIELD] =
	    menu_get(find_pattern, MENU_NTH_ITEM, 6);

	menu_items[(int)TEXTSW_MENU_FIND] =
	    menu_get(find_sel_cmd, MENU_NTH_ITEM, 1);
	menu_items[(int)TEXTSW_MENU_FIND_BACKWARD] =
	    menu_get(find_sel_cmd, MENU_NTH_ITEM, 2);

	menu_items[(int)TEXTSW_MENU_FIND_SHELF] =
	    menu_get(find_clip_cmd, MENU_NTH_ITEM, 1);
	menu_items[(int)TEXTSW_MENU_FIND_SHELF_BACKWARD] =
	    menu_get(find_clip_cmd, MENU_NTH_ITEM, 2);
	    
	menu_items[(int)TEXTSW_MENU_SEL_ENCLOSE_FIELD] =
	    menu_get(search_field_cmd, MENU_NTH_ITEM, 1);
	menu_items[(int)TEXTSW_MENU_SEL_NEXT_FIELD] =
	    menu_get(search_field_cmd, MENU_NTH_ITEM, 2); 
	menu_items[(int)TEXTSW_MENU_SEL_PREV_FIELD] =
	    menu_get(search_field_cmd, MENU_NTH_ITEM, 3);       
    } else {
	menu_items[(int)TEXTSW_MENU_LOAD] =
	    menu_get(top_menu, MENU_NTH_ITEM, 2);
	menu_items[(int)TEXTSW_MENU_NORMALIZE_LINE] =
	    menu_get(top_menu, MENU_NTH_ITEM, 3);
	menu_items[(int)TEXTSW_MENU_CREATE_VIEW] =
	    menu_get(top_menu, MENU_NTH_ITEM, 4);
	menu_items[(int)TEXTSW_MENU_DESTROY_VIEW] =
	    menu_get(top_menu, MENU_NTH_ITEM, 5);
	menu_items[(int)TEXTSW_MENU_RESET] =
	    menu_get(top_menu, MENU_NTH_ITEM, 6);
	menu_items[(int)TEXTSW_MENU_COUNT_TO_LINE] =
	    menu_get(top_menu, MENU_NTH_ITEM, 7);
	menu_items[(int)TEXTSW_MENU_FILE_STUFF] =
	    menu_get(top_menu, MENU_NTH_ITEM, 8);
	menu_items[(int)TEXTSW_MENU_NORMALIZE_INSERTION] =
	    menu_get(top_menu, MENU_NTH_ITEM, 9);
	menu_items[(int)TEXTSW_MENU_CD] =
	    menu_get(top_menu, MENU_NTH_ITEM, 11);
	menu_items[(int)TEXTSW_MENU_PUT_THEN_GET] =
	    menu_get(top_menu, MENU_NTH_ITEM, 13);
/*	menu_items[(int)TEXTSW_MENU_MATCH_FIELD] =
	    menu_get(top_menu, MENU_NTH_ITEM, 14);
*/
	menu_item = (Menu_item)menu_get(top_menu, MENU_NTH_ITEM, 1);
	save_file = (Menu)menu_get(menu_item, MENU_VALUE);
	menu_item = (Menu_item)menu_get(top_menu, MENU_NTH_ITEM, 10);
	break_mode = (Menu)menu_get(menu_item, MENU_VALUE);
	menu_item = (Menu_item)menu_get(top_menu, MENU_NTH_ITEM, 12);
	find_pattern = (Menu)menu_get(menu_item, MENU_VALUE);

/*	menu_item = (Menu_item)menu_get(top_menu, MENU_NTH_ITEM, 15);
	edit_cmd = (Menu)menu_get(menu_item, MENU_VALUE);
	menu_items[(int)TEXTSW_MENU_AGAIN] =
	    menu_get(edit_cmd, MENU_NTH_ITEM, 1);
	menu_items[(int)TEXTSW_MENU_UNDO] =
	    menu_get(edit_cmd, MENU_NTH_ITEM, 2);
	menu_items[(int)TEXTSW_MENU_CUT] =
	    menu_get(edit_cmd, MENU_NTH_ITEM, 3);
	menu_items[(int)TEXTSW_MENU_COPY] =
	    menu_get(edit_cmd, MENU_NTH_ITEM, 4); 
	menu_items[(int)TEXTSW_MENU_PASTE] =
	    menu_get(edit_cmd, MENU_NTH_ITEM, 5);
*/     
    
	menu_items[(int)TEXTSW_MENU_SAVE] =
	    menu_get(save_file, MENU_NTH_ITEM, 1);
	menu_items[(int)TEXTSW_MENU_SAVE_QUIT] =
	    menu_get(save_file, MENU_NTH_ITEM, 2);
	menu_items[(int)TEXTSW_MENU_SAVE_RESET] =
	    menu_get(save_file, MENU_NTH_ITEM, 3);
	menu_items[(int)TEXTSW_MENU_SAVE_CLOSE] =
	    menu_get(save_file, MENU_NTH_ITEM, 4); 
	menu_items[(int)TEXTSW_MENU_STORE] =
	    menu_get(save_file, MENU_NTH_ITEM, 5);
	menu_items[(int)TEXTSW_MENU_STORE_QUIT] =
	    menu_get(save_file, MENU_NTH_ITEM, 6);
	menu_items[(int)TEXTSW_MENU_STORE_CLOSE] =
	    menu_get(save_file, MENU_NTH_ITEM, 7); 

	menu_items[(int)TEXTSW_MENU_CLIP_LINES] =
	    menu_get(break_mode, MENU_NTH_ITEM, 1);
	menu_items[(int)TEXTSW_MENU_WRAP_LINES_AT_CHAR] =
	    menu_get(break_mode, MENU_NTH_ITEM, 2);

	menu_items[(int)TEXTSW_MENU_FIND] =
	    menu_get(find_pattern, MENU_NTH_ITEM, 1);
	menu_items[(int)TEXTSW_MENU_FIND_BACKWARD] =
	    menu_get(find_pattern, MENU_NTH_ITEM, 2);
	menu_items[(int)TEXTSW_MENU_FIND_SHELF] =
	    menu_get(find_pattern, MENU_NTH_ITEM, 3);
	menu_items[(int)TEXTSW_MENU_FIND_SHELF_BACKWARD] =
	    menu_get(find_pattern, MENU_NTH_ITEM, 4); 
#ifdef _TEXTSW_FIND_RE
	menu_items[(int)TEXTSW_MENU_FIND_RE] =
	    menu_get(find_pattern, MENU_NTH_ITEM, 5);
	menu_items[(int)TEXTSW_MENU_FIND_RE_BACKWARD] =
	    menu_get(find_pattern, MENU_NTH_ITEM, 6);
	menu_items[(int)TEXTSW_MENU_FIND_TAG] =
	    menu_get(find_pattern, MENU_NTH_ITEM, 7);
	menu_items[(int)TEXTSW_MENU_FIND_TAG_BACKWARD] =
	    menu_get(find_pattern, MENU_NTH_ITEM, 8); 
#endif
    } /* end of "if textsw_menu_style != organized" */

    for(item = (int)TEXTSW_MENU_NO_CMD;
	item < (int)TEXTSW_MENU_LAST_CMD;
	item++) {
        menu_set(menu_items[item],
        		MENU_ACTION, textsw_do_menu_action, 0);
    }

    return menu_items;
}
static struct pixrect *
textsw_init_get_only_pr(menu)
    Menu	menu;
{
    struct pixrect		*pr;
    struct pixfont		*menu_font;
    struct pr_subregion		 get_only_bound;
    struct pr_prpos		 get_only_prpos;
    int				 menu_left_margin;

    /* compute the size of the pixrect and allocate it */
    menu_font = (struct pixfont *)LINT_CAST(menu_get(menu, MENU_FONT));
    pf_textbound(&get_only_bound,
	strlen(put_then_get), menu_font, put_then_get);
    menu_left_margin = (int)menu_get(menu, MENU_LEFT_MARGIN);
    pr = (struct pixrect *)mem_create(
	get_only_bound.size.x + menu_left_margin +
	(int)menu_get(menu, MENU_RIGHT_MARGIN),
	get_only_bound.size.y, 1);
    /* write the text into the pixrect */
    get_only_prpos.pr = pr;
    get_only_prpos.pos.x = menu_left_margin
	- menu_font->pf_char[put_then_get[0]].pc_home.x;
    get_only_prpos.pos.y =
	- menu_font->pf_char[put_then_get[0]].pc_home.y;
    pf_text(get_only_prpos, PIX_SRC, menu_font, put_then_get);
    /* gray out "Copy then" */
    pf_textbound(&get_only_bound,
	strlen(put_then), menu_font, put_then);
    pr_replrop(pr, menu_left_margin, 0,
	       get_only_bound.size.x, get_only_bound.size.y, 
	       PIX_SRC & PIX_DST, &menu_gray50_pr, 0, 0);
    return pr;
}

extern Menu
textsw_menu_init(textsw)
    Textsw_folio textsw;
{
    Textsw_enum style;

    if (textsw_menu != 0) {
        textsw->menu = (caddr_t)textsw_menu;
    } else {
	if (!textsw_menu_style_already_set) {
	    Textsw_enum style = textsw_get_menu_style_internal();
	}
	(Menu)textsw->menu = textsw_menu = textsw_new_menu(textsw);
	if (textsw_get_only_pr == 0) {
	    textsw_get_only_pr = textsw_init_get_only_pr(textsw_menu);
	}
	textsw_menu_items = textsw_new_menu_table(textsw);
	textsw_menu_refcount = 0;
    }
    textsw->menu_table = (caddr_t)textsw_menu_items;
    textsw_menu_refcount++;
    return (textsw->menu);
}

extern Menu
textsw_get_unique_menu(textsw)
    Textsw_folio textsw;
{
    if (textsw->menu == textsw_menu) {
        /* refcount == 1 ==> textsw is the only referencer */
        if (textsw_menu_refcount == 1) {
            textsw_menu = 0;
            textsw_menu_items = 0;
            textsw_menu_refcount = 0;
        } else {
	    textsw->menu = (caddr_t)textsw_new_menu(textsw);
	    textsw->menu_table = (caddr_t)textsw_new_menu_table(textsw);
	    textsw_menu_refcount--;
        }
    }
    return (textsw->menu);
}

extern void
textsw_set_menu_style_internal(style)
	Textsw_enum	style;
{
	if (textsw_menu == 0) textsw_menu_style = style;
	textsw_menu_style_already_set = 1; /* set first time */
}

extern Textsw_enum
textsw_get_menu_style_internal()
{
    if (!textsw_menu_style_already_set) { /* not set yet */
	if (defaults_get_boolean(
		"/Compatibility/New_Text_Menu", (Bool)TRUE, (int *)NULL)) {
	    textsw_menu_style = TEXTSW_STYLE_ORGANIZED;
	} else textsw_menu_style = TEXTSW_STYLE_UNORGANIZED;
	textsw_menu_style_already_set = 1; 
    }
    return (textsw_menu_style);
}

#define MENU_ACTIVE	MENU_INACTIVE, FALSE

extern void
textsw_do_menu(view, ie)
    Textsw_view		 view;
    Event		*ie;
{
    Ev_handle		 current_view;
    int			 i;
    register Textsw_folio textsw = FOLIO_FOR_VIEW(view);
    Textsw		 abstract = VIEW_REP_TO_ABS(view);
    register Menu_item	*menu_items;
    int			 primary_selection_exists, shelf_isnt_empty;

    if (textsw->menu == 0) return;

    primary_selection_exists = textsw_is_seln_nonzero(textsw, EV_SEL_PRIMARY);
    shelf_isnt_empty = textsw_is_seln_nonzero(textsw, EV_SEL_SHELF);

    menu_items = (Menu_item *)LINT_CAST(textsw->menu_table);
    for (i = 0; i < (int)TEXTSW_MENU_LAST_CMD; i++)
	menu_set(menu_items[i],	MENU_INACTIVE,		TRUE,
				MENU_CLIENT_DATA,	abstract,
				0);

    /* Construct a menu appropriate to the the view it is invoked over.  */
    if (((textsw->state & TXTSW_NO_LOAD) == 0) && primary_selection_exists)
	menu_set(menu_items[(int)TEXTSW_MENU_LOAD], MENU_ACTIVE, 0);
    menu_set(menu_items[(int)TEXTSW_MENU_SAVE], MENU_ACTIVE, 0);
    menu_set(menu_items[(int)TEXTSW_MENU_STORE], MENU_ACTIVE, 0);
    if (textsw->views->first_view != EV_NULL) {
	Es_handle		 original;
	original = (Es_handle)LINT_CAST(
		   es_get(textsw->views->esh, ES_PS_ORIGINAL) );
	if ((!TXTSW_IS_READ_ONLY(textsw)) &&
	    (original != ES_NULL) &&
	    (Es_enum)LINT_CAST(es_get(original, ES_TYPE)) == ES_TYPE_FILE) {
	    menu_set(menu_items[(int)TEXTSW_MENU_SAVE_QUIT],
		     MENU_ACTIVE, 0);
	    menu_set(menu_items[(int)TEXTSW_MENU_SAVE_RESET],
		     MENU_ACTIVE, 0);
	    menu_set(menu_items[(int)TEXTSW_MENU_SAVE_CLOSE],
		     MENU_ACTIVE, 0);
	}
	if ((textsw->state & TXTSW_READ_ONLY_SW) == 0) {
   	    menu_set(menu_items[(int)TEXTSW_MENU_RESET], MENU_ACTIVE, 0);
	    if (primary_selection_exists) {
/* commented out since this item is set to active always *
	        menu_set(menu_items[(int)TEXTSW_MENU_STORE],
			MENU_ACTIVE, 0);
*/
	        menu_set(menu_items[(int)TEXTSW_MENU_STORE_QUIT],
			MENU_ACTIVE, 0);
	        menu_set(menu_items[(int)TEXTSW_MENU_STORE_CLOSE],
			MENU_ACTIVE, 0);
	        }
	}
	if (textsw->tool)
	    menu_set(menu_items[(int)TEXTSW_MENU_CREATE_VIEW],
		     MENU_ACTIVE, 0);
	if (((textsw->state & TXTSW_NO_CD) == 0)
		&& (primary_selection_exists))
	    menu_set(menu_items[(int)TEXTSW_MENU_CD], MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_NORMALIZE_INSERTION],
		 MENU_ACTIVE, 0);
/*
	menu_set(menu_items[(int)TEXTSW_MENU_NORMALIZE_SELECTION],
		 MENU_ACTIVE, 0);
*/
	menu_set(menu_items[(int)TEXTSW_MENU_NORMALIZE_TOP],
		     MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_NORMALIZE_BOTTOM],
		     MENU_ACTIVE, 0);
	if (!TXTSW_IS_READ_ONLY(textsw)) {
	    if (primary_selection_exists)
		menu_set(menu_items[(int)TEXTSW_MENU_FILE_STUFF],
		     MENU_ACTIVE, 0);
	    menu_set(menu_items[(int)TEXTSW_MENU_AGAIN],
		     MENU_ACTIVE, 0);

	    if (textsw_has_been_modified(abstract)) {
	        menu_set(menu_items[(int)TEXTSW_MENU_UNDO],
		     MENU_ACTIVE, 0);
	        menu_set(menu_items[(int)TEXTSW_MENU_UNDO_ALL],
		     MENU_ACTIVE, 0);
	    }

	    if (primary_selection_exists)
	    /* if there is a non-zero primary selection,
	     * set string to be put_then_get, and make active;
	     * else if there is a non-zero shelf,
	     * set string to be get_only, and make active;
	     * else set string to be put_then_get, and make inactive.
	     */
	    if (primary_selection_exists) {
	        menu_set(menu_items[(int)TEXTSW_MENU_PUT_THEN_GET],
	        	 MENU_ACTIVE,
	        	 MENU_IMAGE,  0,
	        	 MENU_STRING, put_then_get,
	        	 0);
	        menu_set(menu_items[(int)TEXTSW_MENU_CUT], MENU_ACTIVE, 0);
	        menu_set(menu_items[(int)TEXTSW_MENU_COPY], MENU_ACTIVE, 0);
	    } else if (shelf_isnt_empty) {
	        menu_set(menu_items[(int)TEXTSW_MENU_PUT_THEN_GET],
	        	 MENU_ACTIVE,
	        	 MENU_STRING, 0,
	        	 MENU_IMAGE,  textsw_get_only_pr,
	        	 0);
	    } else {
		/* Leave MENU_INACTIVE */
	        menu_set(menu_items[(int)TEXTSW_MENU_PUT_THEN_GET],
	        	 MENU_IMAGE,  0,
	        	 MENU_STRING, put_then_get,
	        	 0);
	    }
	    
	    if (shelf_isnt_empty) {
	        menu_set(menu_items[(int)TEXTSW_MENU_PASTE],
		     MENU_ACTIVE, 0);
	    }
	    
	}
	if (textsw->views->first_view->next != EV_NULL)
	    menu_set(menu_items[(int)TEXTSW_MENU_DESTROY_VIEW],
		     MENU_ACTIVE, 0);
	current_view = view->e_view;
	if (current_view != EV_NULL) {
	    if ((Ev_right_break)ev_get(current_view, EV_RIGHT_BREAK) ==
		 EV_CLIP) {
		menu_set(menu_items[(int)TEXTSW_MENU_WRAP_LINES_AT_CHAR],
			 MENU_ACTIVE, 0);
		menu_set(menu_items[(int)TEXTSW_MENU_WRAP_LINES_AT_WORD],
			 MENU_ACTIVE, 0);
	    } else if ((Ev_right_break)ev_get(current_view, EV_RIGHT_BREAK) ==
		 EV_WRAP_AT_CHAR) {
		menu_set(menu_items[(int)TEXTSW_MENU_CLIP_LINES],
			 MENU_ACTIVE, 0);
		menu_set(menu_items[(int)TEXTSW_MENU_WRAP_LINES_AT_WORD],
			 MENU_ACTIVE, 0);
	    } else {
		menu_set(menu_items[(int)TEXTSW_MENU_CLIP_LINES],
			 MENU_ACTIVE, 0);
		menu_set(menu_items[(int)TEXTSW_MENU_WRAP_LINES_AT_CHAR],
			 MENU_ACTIVE, 0);
	    }
	if (primary_selection_exists) {
	    menu_set(menu_items[(int)TEXTSW_MENU_FIND],
		MENU_ACTIVE, 0);
	    menu_set(menu_items[(int)TEXTSW_MENU_FIND_BACKWARD],
		MENU_ACTIVE, 0);
	    menu_set(menu_items[(int)TEXTSW_MENU_MATCH_FIELD],
		MENU_ACTIVE, 0); 
	    menu_set(menu_items[(int)TEXTSW_MENU_NORMALIZE_LINE],
		MENU_ACTIVE, 0);
	    menu_set(menu_items[(int)TEXTSW_MENU_COUNT_TO_LINE],
		MENU_ACTIVE, 0);
	}
	if (shelf_isnt_empty) {
	    menu_set(menu_items[(int)TEXTSW_MENU_SHOW_CLIPBOARD],
		MENU_ACTIVE, 0);
	    menu_set(menu_items[(int)TEXTSW_MENU_FIND_SHELF],
		MENU_ACTIVE, 0);
	    menu_set(menu_items[(int)TEXTSW_MENU_FIND_SHELF_BACKWARD],
		MENU_ACTIVE, 0);
	}

	/* should check if there is selection */
	menu_set(menu_items[(int)TEXTSW_MENU_FIND_AND_REPLACE], 
	    MENU_ACTIVE, 0);
	
	menu_set(menu_items[(int)TEXTSW_MENU_SEL_ENCLOSE_FIELD],
	    MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_SEL_NEXT_FIELD], MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_SEL_PREV_FIELD], MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_SEL_MARK_TEXT], MENU_ACTIVE, 0);
	
	menu_set(menu_items[(int)TEXTSW_MENU_SHOW_CLIPBOARD_FROM_EDIT],
	    MENU_ACTIVE, 0); 
	menu_set(menu_items[(int)TEXTSW_MENU_SHOW_CLIPBOARD_FROM_FIND],
	    MENU_ACTIVE, 0);
#ifdef _TEXTSW_FIND_RE
	menu_set(menu_items[(int)TEXTSW_MENU_FIND_RE], MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_FIND_RE_BACKWARD], MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_FIND_TAG], MENU_ACTIVE, 0);
	menu_set(menu_items[(int)TEXTSW_MENU_FIND_TAG_BACKWARD], MENU_ACTIVE,
		 0);
#endif
	}
    }
    menu_show_using_fd(textsw->menu, view->window_fd, ie);
}

static int
textsw_do_menu_action(cmd_menu, cmd_item)
    Menu		 cmd_menu;
    Menu_item		 cmd_item;
{
    extern void		  textsw_find_selection_and_normalize();
    extern void		  textsw_reset();
    pkg_private void	  textsw_move_caret();
    extern int		  textsw_match_selection_and_normalize();
    Textsw		  abstract = (Textsw)LINT_CAST(
				menu_get(cmd_item, MENU_CLIENT_DATA, 0));
    register Textsw_view  view;
    register Textsw_folio textsw;
    Textsw_menu_cmd	  cmd = (Textsw_menu_cmd)
				menu_get(cmd_item, MENU_VALUE, 0);
    register Event	 *ie = (Event *)LINT_CAST(
			       menu_get(cmd_menu, MENU_FIRST_EVENT, 0) );
    register int	  locx, locy;
    Event		  event;
    register long unsigned
			  find_options = 0;
    Es_index		  first, last_plus_one;
    char		  current_selection[200], err_msg[300];

    if AN_ERROR(abstract == 0)
	return;

    view = VIEW_ABS_TO_REP(abstract);
    textsw = FOLIO_FOR_VIEW(view);

    if AN_ERROR(ie == 0) {
	locx = locy = 0;
    } else {
	locx = ie->ie_locx;
	locy = ie->ie_locy;
    }

    /* initialize arrays */
    current_selection[0] = 0; err_msg[0] = 0;

    switch (cmd) {

      case TEXTSW_MENU_RESET:
	textsw_empty_document(abstract, ie);
	break;

      case TEXTSW_MENU_LOAD:
	textsw_load_file_as_menu(abstract, ie);
	break;

      case TEXTSW_MENU_SAVE_CLOSE:
	textsw_notify(view, TEXTSW_ACTION_TOOL_CLOSE, 0);
	/* Fall through */
      case TEXTSW_MENU_SAVE:
	if (cmd == TEXTSW_MENU_SAVE) {
	    Es_handle		 original;

	    original = (Es_handle)LINT_CAST(
		es_get(textsw->views->esh, ES_PS_ORIGINAL) );
	    if ((TXTSW_IS_READ_ONLY(textsw)) ||
			(original == ES_NULL) ||
			(Es_enum)LINT_CAST(es_get(
			    original, ES_TYPE)) != ES_TYPE_FILE) {
		Event	alert_event;

		(void) alert_prompt(
		        (Frame)window_get(WINDOW_FROM_VIEW(view), WIN_OWNER),
		        &alert_event,
		        ALERT_MESSAGE_STRINGS,
			    "Unable to Save Current File.",
			    "You are not currently editing a file.",
			    "Use \"Store as New File\" to save your work.",
		            0,
		        ALERT_BUTTON_YES,	"Continue",
		        ALERT_TRIGGER,		ACTION_STOP,
	                0);
		break;
	    }
	}
      case TEXTSW_MENU_SAVE_QUIT:
      case TEXTSW_MENU_SAVE_RESET: {
	Es_status	status;
	status = (Es_status)textsw_save(VIEW_REP_TO_ABS(view), locx, locy);
	if (status == ES_SUCCESS) {
	    if (cmd == TEXTSW_MENU_SAVE_QUIT) {
		textsw_notify(
		    view, TEXTSW_ACTION_TOOL_DESTROY, (char *)ie, 0);
	    } else if (cmd == TEXTSW_MENU_SAVE_RESET) {
		textsw_reset(abstract, locx, locy);
		break;
	    }
	}
	break;
      }

      case TEXTSW_MENU_STORE_CLOSE:
	textsw_notify(view, TEXTSW_ACTION_TOOL_CLOSE, 0);
      case TEXTSW_MENU_STORE:
      case TEXTSW_MENU_STORE_QUIT: {
	Es_status	status;
	status = textsw_store_to_selection(textsw, locx, locy);
	if ((status == ES_SUCCESS) && (cmd == TEXTSW_MENU_STORE_QUIT)) {
	    textsw_notify(view, TEXTSW_ACTION_TOOL_DESTROY, (char *)ie, 0);
	}
	break;
      }

      case TEXTSW_MENU_CREATE_VIEW: {
	int	result, my_x, my_y;
	Rect	*my_place;
	Frame   base_frame = (Frame)window_get(WINDOW_FROM_VIEW(view), WIN_OWNER);
	
	if (!base_frame) {
	    (void)textsw_alert_for_old_interface(view);
	    return;
	}
	    
	result = alert_prompt(base_frame, &event,
	        ALERT_NO_BEEPING,		1,
	        ALERT_NO_IMAGE,			1,
	        ALERT_MESSAGE_STRINGS,
		    "Move pointer to where new view should", 
		    "begin, then click the left mouse button.",
		    " ",
		    "Otherwise, push \"Cancel Split View\".",
		    0,
	        ALERT_BUTTON_NO,		"Cancel Split View",
	        ALERT_TRIGGER,			MS_LEFT,
	        0);
	if (result != ALERT_TRIGGERED) break;

	my_place = (Rect *)window_get(WINDOW_FROM_VIEW(view), WIN_RECT);
	my_x = (event.ie_locx - my_place->r_left);
	my_y = (event.ie_locy - my_place->r_top - (2*TOOL_BORDERWIDTH));
	(void) textsw_split_view(VIEW_REP_TO_ABS(view), my_x, my_y);
	break;
      }

      case TEXTSW_MENU_DESTROY_VIEW: {
	(void) textsw_destroy_split(VIEW_REP_TO_ABS(view),
				    locx, locy);
	break;
      }

      case TEXTSW_MENU_CLIP_LINES:
	textsw_set(VIEW_REP_TO_ABS(view),
		   TEXTSW_LINE_BREAK_ACTION, TEXTSW_CLIP,
		   0);
	break;

      case TEXTSW_MENU_WRAP_LINES_AT_CHAR:
	textsw_set(VIEW_REP_TO_ABS(view),
		   TEXTSW_LINE_BREAK_ACTION, TEXTSW_WRAP_AT_CHAR,
		   0);
	break;

      case TEXTSW_MENU_WRAP_LINES_AT_WORD:
	textsw_set(VIEW_REP_TO_ABS(view),
		   TEXTSW_LINE_BREAK_ACTION, TEXTSW_WRAP_AT_WORD,
		   0);
	break;

      case TEXTSW_MENU_NORMALIZE_INSERTION: {
	extern Es_index	ev_get_insert();
	Es_index	insert;
	int		upper_context;
	insert = ev_get_insert(textsw->views);
	if (insert != ES_INFINITY) {
	    upper_context = (int)LINT_CAST(
			    ev_get(textsw->views, EV_CHAIN_UPPER_CONTEXT) );
	    textsw_normalize_internal(view, insert, insert, upper_context, 0,
					TXTSW_NI_DEFAULT);
	}
	break;
      }

      case TEXTSW_MENU_NORMALIZE_BOTTOM: {
	(void) textsw_move_caret(view, TXTSW_DOCUMENT_END);
	break;
      }

      case TEXTSW_MENU_NORMALIZE_TOP: {
	(void) textsw_move_caret(view, TXTSW_DOCUMENT_START);
	break;
      }

#ifdef TEXTSW_MENU_NORMALIZE_SELECTION
      case TEXTSW_MENU_NORMALIZE_SELECTION: {
	ev_get_selection(
	    textsw->views, &first, &last_plus_one, EV_SEL_PRIMARY);
	if (first != ES_INFINITY) {
	    textsw_normalize_view(VIEW_REP_TO_ABS(view), first);
	}
	break;
      }
#endif
      case TEXTSW_MENU_NORMALIZE_LINE: {
	Es_index	prev;
	char		buf[10];
	unsigned	buf_fill_len;
	int		line_number;

	line_number =
	    textsw_get_selection_as_int(textsw, EV_SEL_PRIMARY, 0);
	if (line_number == 0)
		goto Out_Of_Range;
	else {
	    buf[0] = '\n'; buf_fill_len = 1;
	    if (line_number == 1) {
		prev = 0;
	    } else {
		ev_find_in_esh(textsw->views->esh, buf, buf_fill_len,
	            (Es_index)0, (u_int)line_number-1, 0, &first, &prev);
		if (first == ES_CANNOT_SET)
		    goto Out_Of_Range;
	    }
	    ev_find_in_esh(textsw->views->esh, buf, buf_fill_len,
			    prev, 1, 0, &first, &last_plus_one);
	    if (first == ES_CANNOT_SET)
		goto Out_Of_Range;
	    textsw_possibly_normalize_and_set_selection(
		VIEW_REP_TO_ABS(view), prev, last_plus_one,
		EV_SEL_PRIMARY);
	    (void) textsw_set_insert(textsw, last_plus_one);
	}
Out_Of_Range:
	break;
      }

      case TEXTSW_MENU_COUNT_TO_LINE: {
	char		msg[200];
	int		result, count;

	ev_get_selection(
	    textsw->views, &first, &last_plus_one, EV_SEL_PRIMARY);
	if (first >= last_plus_one)
	    break;
	count = ev_newlines_in_esh(textsw->views->esh, 0, first);
	(void) sprintf(msg, "Selection starts in line %d.", count+1);
	result = alert_prompt(
	        (Frame)window_get(WINDOW_FROM_VIEW(view), WIN_OWNER),
	        &event,
	        ALERT_NO_BEEPING,	1,
	        ALERT_NO_IMAGE,		1,
	        ALERT_MESSAGE_STRINGS,
		    msg,
		    "Press \"Continue\" to proceed.",
		    0,
	        ALERT_BUTTON_YES,	"Continue",
		ALERT_TRIGGER,		ACTION_STOP,
	        0);
	break;
      }

      case TEXTSW_MENU_FILE_STUFF:{
	textsw_file_stuff(view, locx, locy);
	break;
      }

      case TEXTSW_MENU_CD: {
	textsw_cd(textsw, locx, locy); 
	break;
	}

      case TEXTSW_MENU_FIND_BACKWARD:
	find_options = TFSAN_BACKWARD;
	/* Fall through */
      case TEXTSW_MENU_FIND:
	find_options |= (EV_SEL_PRIMARY | TFSAN_SHELF_ALSO);
	textsw_find_selection_and_normalize(view, locx, locy, find_options);
	break;

      case TEXTSW_MENU_FIND_SHELF_BACKWARD:
	find_options = TFSAN_BACKWARD;
	/* Fall through */
      case TEXTSW_MENU_FIND_SHELF:
	find_options |= EV_SEL_SHELF;
	textsw_find_selection_and_normalize(view, locx, locy, find_options);
	break;
	
      case TEXTSW_MENU_SEL_ENCLOSE_FIELD: {
        int		first, last_plus_one;
        
        first = last_plus_one = ev_get_insert(textsw->views); 
        (void) textsw_match_field_and_normalize(view, &first, &last_plus_one, "<|", 2, TEXTSW_FIELD_ENCLOSE, FALSE);
	break;
	}
      case TEXTSW_MENU_SEL_NEXT_FIELD: 
        (void) textsw_match_selection_and_normalize(view, "|>", TEXTSW_FIELD_FORWARD);
	break;

      case TEXTSW_MENU_SEL_PREV_FIELD: 
        (void) textsw_match_selection_and_normalize(view, "<|", TEXTSW_FIELD_BACKWARD);
	break;
	
      case TEXTSW_MENU_SEL_MARK_TEXT:
        (void) textsw_create_match_frame(view);
        break;
        
#ifdef _TEXTSW_FIND_RE
      case TEXTSW_MENU_FIND_RE_BACKWARD:
	find_options = TFSAN_BACKWARD;
	/* Fall through */
      case TEXTSW_MENU_FIND_RE:
	find_options |= (EV_SEL_PRIMARY | TFSAN_REG_EXP | TFSAN_SHELF_ALSO);
	textsw_find_selection_and_normalize(view, locx, locy, find_options);
	break;

      case TEXTSW_MENU_FIND_TAG_BACKWARD:
	find_options = TFSAN_BACKWARD;
	/* Fall through */
      case TEXTSW_MENU_FIND_TAG:
	find_options |=
	    (EV_SEL_PRIMARY | TFSAN_REG_EXP | TFSAN_SHELF_ALSO | TFSAN_TAG);
	textsw_find_selection_and_normalize(view, locx, locy, find_options);
	break;
#endif

      case TEXTSW_MENU_PUT_THEN_GET:
        textsw_put_then_get(view);
	break;
	
      case TEXTSW_MENU_FIND_AND_REPLACE:
        (void) textsw_create_search_frame(view);
	break;

      case TEXTSW_MENU_MATCH_FIELD: {
        char		*start_marker;
        (void) textsw_match_selection_and_normalize(view, start_marker, TEXTSW_NOT_A_FIELD);
	break;
	}
      case TEXTSW_MENU_AGAIN:
        textsw_again(view, locx, locy);
        break;
        
      case TEXTSW_MENU_UNDO:
        textsw_undo(textsw);
        break;

      case TEXTSW_MENU_UNDO_ALL: {
	int	result;

	if (textsw_has_been_modified(abstract)) {
		result = alert_prompt(
		    (Frame)window_get(WINDOW_FROM_VIEW(view), WIN_OWNER),
		    &event,
		    ALERT_MESSAGE_STRINGS,
		        "Undo All Edits will discard unsaved edits.",
			"Please confirm.",
		        0,
		    ALERT_BUTTON_YES,	"Confirm, discard edits",
		    ALERT_BUTTON_NO,	"Cancel",
		    ALERT_NO_BEEPING,	1,
		    0);
		if (result == ALERT_YES) textsw_reset_2(abstract, locx, locy, TRUE, TRUE);
	}
	break;
	}

      case TEXTSW_MENU_SHOW_CLIPBOARD_FROM_EDIT:
	/* fall through */

      case TEXTSW_MENU_SHOW_CLIPBOARD_FROM_FIND:
	stuff_clipboard();
	break;
        
      case TEXTSW_MENU_CUT:
        textsw_function_delete(view);
        break;
        
      case TEXTSW_MENU_COPY:
        textsw_put(view);
        break;
        
      case TEXTSW_MENU_PASTE:
        (void) textsw_function_get(view);
        break;

      default:
	break;
    }
}

pkg_private int
await_neg_event(windowfd, event)
	int			 windowfd;
	struct inputevent	*event;
{
	struct fullscreen	*fsh;
	struct inputevent	 new_event;

	fsh = fullscreen_init(windowfd);
	if (fsh == 0)
	    return(1);
	FOREVER {
	    if AN_ERROR(input_readevent(windowfd, &new_event) == -1) {
	        break;
	    }
	    if (event_action(&new_event) == event_action(event)
	    &&  win_inputnegevent(&new_event))
	        break;
	}
	fullscreen_destroy(fsh);
	return(0);
}

/* ARGSUSED */
extern Textsw
textsw_split_view(abstract, locx, locy)
	Textsw			abstract;
	int			locx, locy;	/* locx currently unused */
{
	extern struct toolsw	*tool_find_sw_with_client();
	extern Textsw_view	 textsw_create_view();
	extern caddr_t		 textsw_view_window_object();
	register struct toolsw	*toolsw;
	struct rect		 new_to_split_rect,
				 new_view_rect;
	int			 left_margin, right_margin, start_line;
	register int		 to_split_ts_height, to_split_ts_width;
	Es_index		 start_pos;
	register Textsw_view	 new_view;
	register Textsw_view	 to_split = VIEW_ABS_TO_REP(abstract);
	Textsw_folio		 folio = FOLIO_FOR_VIEW(to_split);

	/*
	 * Only split views that are large enough
	 */
	if ((locy < 0) || (to_split->rect.r_height < locy) ||
	    (textsw_screen_line_count(abstract) < 3)) {
	    return(TEXTSW_NULL);
	}
	win_getrect(to_split->window_fd, &new_to_split_rect);
	new_view_rect.r_left = new_to_split_rect.r_left;
	new_view_rect.r_width = new_to_split_rect.r_width;
	new_view_rect.r_height = to_split->rect.r_height - locy;
	if (new_view_rect.r_height < ei_line_height(folio->views->eih))
	    new_view_rect.r_height = ei_line_height(folio->views->eih);
	new_view_rect.r_top =
	    rect_bottom(&new_to_split_rect) + 1 - new_view_rect.r_height;
	/*
	 * start_line/pos must be determined BEFORE adjusting to_split.
	 */
	start_line = ev_line_for_y(to_split->e_view,
				   new_view_rect.r_top -
					new_to_split_rect.r_top);
	start_pos = ev_index_for_line(to_split->e_view, start_line);
	if (start_pos == ES_INFINITY) {
	    start_pos = ev_index_for_line(to_split->e_view, 0);
	}
	new_to_split_rect.r_height -=
	    new_view_rect.r_height + TOOL_SUBWINDOWSPACING;
	win_setrect(to_split->window_fd, &new_to_split_rect);
	toolsw = tool_find_sw_with_client((Tool *)LINT_CAST(folio->tool),
	    (caddr_t)LINT_CAST(to_split));
	if (toolsw) {
	    to_split_ts_height = toolsw->ts_height;
	    to_split_ts_width = toolsw->ts_width;
	    toolsw->ts_height = new_to_split_rect.r_height;
	}
	left_margin = (int)ev_get(to_split->e_view, EV_LEFT_MARGIN);
	right_margin = (int)ev_get(to_split->e_view, EV_RIGHT_MARGIN);
	new_view = textsw_create_view(folio, new_view_rect,
					left_margin, right_margin);
	if (to_split_ts_height == TOOL_SWEXTENDTOEDGE ||
	    to_split_ts_width == TOOL_SWEXTENDTOEDGE) {
	    toolsw = tool_find_sw_with_client((Tool *)LINT_CAST(folio->tool),
		(caddr_t)LINT_CAST(new_view));
	    if (toolsw) {
		if (to_split_ts_height == TOOL_SWEXTENDTOEDGE)
		    toolsw->ts_height = TOOL_SWEXTENDTOEDGE;
		if (to_split_ts_width == TOOL_SWEXTENDTOEDGE)
		    toolsw->ts_width = TOOL_SWEXTENDTOEDGE;
	    }
	}
	if (SCROLLBAR_FOR_VIEW(to_split)) {
	    extern void textsw_add_scrollbar_to_view();

	    textsw_add_scrollbar_to_view(new_view, TEXTSW_DEFAULT_SCROLLBAR);
	}
	(void) ev_set(new_view->e_view, EV_SAME_AS, to_split->e_view, 0);

	/*  
	 *  Although this call to ev_set_start will cause an unnecessary
	 *  display of the newly created view, but it is necessary
	 *  to update the line table of the new view.
	 */
	ev_set_start(new_view->e_view, start_pos); 

	window_create((Window)LINT_CAST(window_get(WINDOW_FROM_VIEW(to_split),
			  WIN_OWNER)),
		      textsw_view_window_object, WIN_COMPATIBILITY,
		      WIN_OBJECT, new_view,
		      0);
	/* BUG ALERT - why isn't the system doing this for us? */
	if (win_fdtonumber(to_split->window_fd)
	== win_get_kbd_focus(to_split->window_fd)
	&& folio->tool != 0)
	    tool_displaytoolonly(TOOL_FROM_FOLIO(folio));
	if (folio->notify_level & TEXTSW_NOTIFY_SPLIT_VIEW)
	    textsw_notify(to_split, TEXTSW_ACTION_SPLIT_VIEW, new_view, 0);
	ASSERT(allock());
	return(VIEW_REP_TO_ABS(new_view));
}

static Textsw_view
textsw_adjacent_view(view, coalesced_rect)
	register Textsw_view	 view;
	register Rect		*coalesced_rect;
{
	Textsw_folio		 folio = FOLIO_FOR_VIEW(view);
	Textsw_view		 above = (Textsw_view)0;
	Textsw_view		 under = (Textsw_view)0;
	register Textsw_view	 other;
	Rect			 view_rect, other_rect;

	win_getrect(WIN_FD_FOR_VIEW(view), &view_rect);
	FORALL_TEXT_VIEWS(folio, other) {
	    if (view == other)
		continue;
	    win_getrect(WIN_FD_FOR_VIEW(other), &other_rect);
	    if ((view_rect.r_left == other_rect.r_left) &&
		(view_rect.r_width == other_rect.r_width)) {
		/* Possible vertical split */
		if ((rect_bottom(&view_rect) + 1 +
		    TOOL_SUBWINDOWSPACING)
		==  other_rect.r_top) {
		    under = other;
		    *coalesced_rect = view_rect;
		    coalesced_rect->r_height =
			rect_bottom(&other_rect) + 1 - coalesced_rect->r_top;
		} else if ((rect_bottom(&other_rect) + 1 +
		    TOOL_SUBWINDOWSPACING)
		==  view_rect.r_top) {
		    above = other;
		    *coalesced_rect = other_rect;
		    coalesced_rect->r_height =
			rect_bottom(&view_rect) + 1 - coalesced_rect->r_top;
		    break;
		}
	    }
	}
	return((above) ? above : under);
}

static Textsw_view
textsw_wrapped_adjacent_view(view, rect)
    register Textsw_view	 view;
    register Rect		*rect;
{
    Textsw_view	 upper_view;
    Textsw_view	 coalesce_with = 0;

    coalesce_with = textsw_adjacent_view(view, rect);
    if (coalesce_with) {
	if ((int)window_get(WINDOW_FROM_VIEW(view), WIN_Y)
	<   (int)window_get(WINDOW_FROM_VIEW(coalesce_with), WIN_Y)) {
	    upper_view = view;
	} else {
	    upper_view = coalesce_with;
	}
	rect->r_top = (int)LINT_CAST(window_get(WINDOW_FROM_VIEW(upper_view),
	    WIN_Y));
	rect->r_left = (int)LINT_CAST(window_get(WINDOW_FROM_VIEW(upper_view),
	    WIN_X));
    } else {
	win_getrect((int)window_get(WINDOW_FROM_VIEW(view), WIN_FD), rect);
    }
    return (coalesce_with);
}

static Notify_value
textsw_menu_destroy_view(abstract, status)
    Textsw		abstract;
    Destroy_status	status;
{
    register Textsw_view	 view = VIEW_ABS_TO_REP(abstract);
    register Textsw_folio 	 folio = FOLIO_FOR_VIEW(view);
    Rect	 rect;
    int		 win_show;
    int		 had_kbd_focus;
    Notify_value nv;
    int		 height, width;

    if (status != DESTROY_CLEANUP) {
	return (notify_next_destroy_func((Notify_client)abstract, status));
    }

    /*--------------------------------------------------------------------
      If the folio has any pop-up frame associated with it, see if that
      frame was created from the view we are about to destroy.
      You can tell by checking whether its WIN_CLIENT_DATA field contains
      a pointer to the view in question.
      Any such pop-up should be destroyed along with the view that owns it.
    ---------------------------------------------------------------------*/
    if((folio->search_frame != NULL) &&
       (view == (Textsw_view) window_get(folio->search_frame, WIN_CLIENT_DATA))
      )
    {  
      /*-------------------------------------------------------------------
        search_frame is a global that represents the frame that the user
	can bring up to "find" operations on the text.  If the view which
	owns it is destroyed it must also be destroyed and set NULL.
      -------------------------------------------------------------------*/
      extern    Frame   search_frame;

      window_set(folio->search_frame, FRAME_NO_CONFIRM, TRUE, 0);
      window_destroy(folio->search_frame);
      search_frame = folio->search_frame = NULL;
    }

    /* Look to see if we can coalesce with one of our own views. */
    folio->coalesce_with = textsw_wrapped_adjacent_view(view, &rect);
    textsw_get_extend_to_edge(view, &height, &width);

    win_show = (int)LINT_CAST(window_get(WINDOW_FROM_VIEW(view), WIN_SHOW));
    had_kbd_focus = (int)LINT_CAST(window_get(WINDOW_FROM_VIEW(view),
	    WIN_KBD_FOCUS));

    /* Unlink view from everywhere and remove its storage */
    nv = notify_next_destroy_func((Notify_client)abstract, status);

    if (folio->coalesce_with) {
	if ((int)LINT_CAST(window_get(WINDOW_FROM_VIEW(folio->coalesce_with),
	    WIN_SHOW)) && !win_show) {
	    window_set(WINDOW_FROM_VIEW(folio->coalesce_with),
	    	       WIN_SHOW, win_show, 0);
	} else {
	    if (had_kbd_focus) {
		window_set(folio->coalesce_with, WIN_KBD_FOCUS, TRUE, 0);
	    }
	}
	window_set(WINDOW_FROM_VIEW(folio->coalesce_with), 
		   WIN_RECT, &rect, 0);
	textsw_set_extend_to_edge(folio->coalesce_with, height, width);

	if (win_fdtonumber(folio->coalesce_with->window_fd)
	==  win_get_kbd_focus(folio->coalesce_with->window_fd)) {
	    tool_displaytoolonly(TOOL_FROM_FOLIO(folio));
	}
    } else if (folio->tool) {
	tool_expand_neighbors(TOOL_FROM_FOLIO(folio), TOOLSW_NULL, &rect);
    }
    folio->coalesce_with = 0;
    return (nv);
}

static Notify_value
textsw_menu_destroy_first(abstract, status)
    Textsw		abstract;
    Destroy_status	status;
{
    register Textsw_view	 tagged_view = VIEW_ABS_TO_REP(abstract);
    register Textsw_view	 first_view;
    register Textsw_folio 	folio = FOLIO_FOR_VIEW(tagged_view);
    Rect	 rect;
    Rect	 empty_rect;
    Es_index	 tagged_view_first = ES_CANNOT_SET;
    int		 win_show;
    int		 had_kbd_focus;
    Notify_value nv;
    int		 height, width;

    if (status != DESTROY_CLEANUP) {
	return (notify_next_destroy_func((Notify_client)abstract, status));
    }
    first_view = folio->first_view;
    /* Look to see if we can coalesce with one of our own views. */
    folio->coalesce_with = textsw_wrapped_adjacent_view(first_view, &rect);

    if (folio->coalesce_with) {
	/* ASSERT(folio->coalesce_with == tagged_view); */

	/* 
	 * At this point folio->coalesce_with should point to the first view.
	 * tagged_view is pointing to the view going to be destroyed.
	 */

	textsw_get_extend_to_edge(folio->coalesce_with, &height, &width);
	folio->coalesce_with = first_view;
    } else {
	empty_rect = rect;
	rect = *((Rect *)LINT_CAST(window_get(WINDOW_FROM_VIEW(tagged_view),
	    WIN_RECT)));
	textsw_get_extend_to_edge(tagged_view, &height, &width);
    }
    win_show = (int)LINT_CAST(window_get(WINDOW_FROM_VIEW(tagged_view),
	    WIN_SHOW));
    had_kbd_focus = (int)LINT_CAST(window_get(WINDOW_FROM_VIEW(tagged_view),
	    WIN_KBD_FOCUS));
    tagged_view_first
	= (Es_index)textsw_get(abstract, TEXTSW_FIRST);
    if (tagged_view_first == (Es_index)LINT_CAST(textsw_get(
	VIEW_REP_TO_ABS(first_view), TEXTSW_FIRST))) {
	tagged_view_first = ES_CANNOT_SET;
    }

    /* Unlink view from everywhere and remove its storage */
    nv = notify_next_destroy_func((Notify_client)abstract, status);

    if ((int)LINT_CAST(window_get(WINDOW_FROM_VIEW(first_view), WIN_SHOW)) &&
	!win_show) {
	window_set(WINDOW_FROM_VIEW(first_view), WIN_SHOW, win_show, 0);
    } else {
	if (had_kbd_focus) {
	    window_set(first_view, WIN_KBD_FOCUS, TRUE, 0);
	}
    }
    if (tagged_view_first != ES_CANNOT_SET) {
	textsw_set(VIEW_REP_TO_ABS(first_view),
		   TEXTSW_FIRST, tagged_view_first, 0);
    }
    window_set(WINDOW_FROM_VIEW(first_view), WIN_RECT, &rect, 0);
    textsw_set_extend_to_edge(first_view, height, width);

    if (win_fdtonumber(first_view->window_fd)
    ==  win_get_kbd_focus(first_view->window_fd)) {
	tool_displaytoolonly(TOOL_FROM_FOLIO(folio));
    }
    if (folio->coalesce_with) {
	folio->coalesce_with = 0;
    } else {
	tool_expand_neighbors(TOOL_FROM_FOLIO(folio), TOOLSW_NULL,&empty_rect);
    }
    return (nv);
}

/* ARGSUSED */
extern void
textsw_destroy_split(abstract, locx, locy)
	Textsw		 abstract;
	int		 locx, locy;	/* Currently unused */
{
	register Textsw_view	 designated_view = VIEW_ABS_TO_REP(abstract);
	register Textsw_view	 tagged_view;
	Textsw_view		 adjacent_view;
	register Textsw_folio	 folio = FOLIO_FOR_VIEW(designated_view);
	Rect			 rect;

	if (folio->first_view == designated_view) {
	    if (!designated_view->next)
		return;
	    adjacent_view = textsw_adjacent_view(designated_view, &rect);
	    if (adjacent_view) {
		tagged_view = adjacent_view;
	    } else {
		tagged_view = designated_view->next;
	    }
	    notify_interpose_destroy_func(
	        (Notify_client)tagged_view, textsw_menu_destroy_first);
	} else {
	    tagged_view = designated_view;
	    notify_interpose_destroy_func(
	        (Notify_client)tagged_view, textsw_menu_destroy_view);
	}
	window_destroy(WINDOW_FROM_VIEW(tagged_view));
}

pkg_private void
textsw_set_extend_to_edge(view, height, width)
	Textsw_view	view;
	int		height, width;
	
{
	Textsw_folio	folio;
	register struct toolsw	*toolsw;
	
	if (view) {
	    folio = FOLIO_FOR_VIEW(view);

	    toolsw = tool_find_sw_with_client((Tool *)LINT_CAST(folio->tool),
		(caddr_t)LINT_CAST(view));
		
	    if (toolsw) {
	        if (height == TOOL_SWEXTENDTOEDGE)
	            toolsw->ts_height = TOOL_SWEXTENDTOEDGE;
	        if (width == TOOL_SWEXTENDTOEDGE)
	            toolsw->ts_width = TOOL_SWEXTENDTOEDGE;
	    }
	}	
}

pkg_private void
textsw_get_extend_to_edge(view, height, width)
	Textsw_view	view;
	int		*height, *width;
	
{
	Textsw_folio	folio;
	register struct toolsw	*toolsw;
	
	*height = 0;
	*width = 0;
	
	if (view) {
	    folio = FOLIO_FOR_VIEW(view);
	    toolsw = tool_find_sw_with_client((Tool *)LINT_CAST(folio->tool),
		(caddr_t)LINT_CAST(view));
	    if (toolsw) {
	        *height = toolsw->ts_height;
	        *width = toolsw->ts_width;
	    }
	}
}

pkg_private void
textsw_alert_for_old_interface(view)
{
        Event	event;
        
	(void) alert_prompt(NULL,
		&event,
		ALERT_MESSAGE_STRINGS,
		    "This feature is not available,",
		    "due to the fact this text subwindow is created with",
		    "pre SunView tool interface.  In order to utilize",
		    "the full text subwindow features, please upgrade the application.",
		    0,
		ALERT_BUTTON_YES,	"Continue",
		ALERT_TRIGGER,		ACTION_STOP,
		0);
}    
