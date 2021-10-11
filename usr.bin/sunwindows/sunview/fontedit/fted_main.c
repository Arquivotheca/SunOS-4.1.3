#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)fted_main.c 1.1 92/07/30 Copyr 1984, 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984, 1985 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <locale.h>
#include <signal.h>
#include "other_hs.h"
#include "fontedit.h"
#define FT$MAIN
#include "externs.h"
#include "button.h"
#include "slider.h"
#include "edit.h"
#include <sys/stat.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/icon_load.h>

static short    icon_data[256] = {
#include <images/fontedit.icon>
};
mpr_static(fted_my_icon_pr, 64, 64, 1, icon_data);
static struct icon	 my_icon  =  {
				TOOL_ICONWIDTH, TOOL_ICONHEIGHT, NULL,
				{0, 0, TOOL_ICONWIDTH, TOOL_ICONHEIGHT},
				&fted_my_icon_pr, {0, 0, 0, 0}, NULL, NULL, 0
			};

static void	sigwinched();
static		mytoolsigwinchhandler();
extern PIXFONT	*pf_open_private();

char	*fted_tool_name = "fontedit";

#define TINY_FONT	"/usr/lib/fonts/fixedwidthfonts/screen.r.7"
#define MSG_FONT 	"/usr/lib/fonts/fixedwidthfonts/screen.r.12"

/*
 * Initialize variables (such as the name of the font to be edited) and 
 * open subwindows for the tool - in oder of appearance, they are:
 * fted_msg_sub_win		- For messages and prompts to the user
 * fted_option_sub_win	- Holds options that govern properties of the font
 * fted_echo_sub_win		- Area where keystrokes are echoed in the current font
 * fted_canvas_sub_win	- Where the edit pads for each character are displayed.
 * 
 */

#ifdef STANDALONE
main(argc, argv)
#else
fontedit_main(argc,argv)
#endif
int  argc;
char *argv[];
{
	register int 	i;
	struct stat 	stat_buff;
	char	       **tool_attrs = NULL;
	struct sigvec	sv;

/*** For debugging, use SIGUP to call a routine which prints state info ***
	{
	int		fted_post_mortem(), fted_print_undo_list();
	struct sigvec	sv;

	sv.sv_handler = fted_post_mortem;
	sv.sv_mask = 0;
	sv.sv_onstack = 1;

	sigvec(SIGHUP, &sv, NULL);
	}
***/
	for (i = 0; i < MAX_NUM_CHARS_IN_FONT; i++) {
	    fted_char_info[i].open = FALSE;
	}

	argc--;  argv++;		/*	skip over program name	*/

	setlocale(LC_CTYPE, "");

	if (tool_parse_all(&argc, argv, &tool_attrs, fted_tool_name) == -1) {
	    fprintf(stderr,"usage: fontedit [generic window args] [font name]\n");
		tool_usage(fted_tool_name);
		exit(1);
	}
	
        fted_font_tool = tool_make(
            WIN_LABEL,          fted_tool_name,
	    WIN_TOP,		50,
	    WIN_LEFT,		50,
            WIN_WIDTH,          675,
            WIN_HEIGHT,         700,
            WIN_NAME_STRIPE,    1,
            WIN_BOUNDARY_MGR,   1,
            WIN_ICON,           &my_icon,
            WIN_ATTR_LIST,      tool_attrs,
            0);
	CHECK_RETURN(fted_font_tool, (struct tool *), "fontedit");
        tool_free_attribute_list(tool_attrs);

	fted_message_font = pf_open(MSG_FONT);
	CHECK_RETURN(fted_message_font, (PIXFONT *),MSG_FONT);
	fted_tiny_font = pf_open(TINY_FONT);
	CHECK_RETURN(fted_tiny_font, (PIXFONT *), "no tiny font (screen.r.7).");

	fted_msg_sub_win = panel_create(fted_font_tool, 
					PANEL_HEIGHT, 	MSG_WINDOW_HEIGHT,
					PANEL_FONT, 	fted_message_font,
					0);
	CHECK_RETURN(fted_msg_sub_win, (struct toolsw *), "message_sub_win");
	fted_msg_item = panel_create_item((Panel)fted_msg_sub_win->ts_data, PANEL_MESSAGE, 
					  PANEL_LABEL_STRING, 	"", 
					  0);
	CHECK_RETURN(fted_msg_item, (Panel_item), "message item");
	
	fted_init_panel_window(fted_font_tool, &fted_option_sub_win);
	
	fted_echo_sub_win = tool_createsubwindow(fted_font_tool, "",
	                 TOOL_SWEXTENDTOEDGE, ECHO_WINDOW_HEIGHT);
	CHECK_RETURN(fted_echo_sub_win, (struct toolsw *), "echo sub win");

	fted_canvas_sub_win = tool_createsubwindow(fted_font_tool, "",
	                TOOL_SWEXTENDTOEDGE, CANVAS_WINDOW_HEIGHT);
	CHECK_RETURN(fted_canvas_sub_win, (struct toolsw *), "canvas");

	/* Get the font the user wants to edit */
	if (argc == 1) {
	    strcpy(fted_font_name, argv[0]);
	    if ((fted_cur_font = pf_open_private(fted_font_name)) == NULL) {
		if (stat(fted_font_name, &stat_buff) == 0) {
		    fprintf(stderr,"%s is not in vfont format.\n",fted_font_name);
		    exit(-1);
		}
		fted_cur_font = alloctype(PIXFONT);
		fted_message_user("This is a new font.");
	    }
	}
	else {
	    fted_cur_font = NULL;
	    strcpy(fted_font_name,  "");
	}
	
	panel_set_value(fted_font_name_item, fted_font_name);
	
	fted_font_modified = FALSE;
	fted_undoing = FALSE;

	fted_init_bounds(TRUE);
	init_fted_echo_sub_win();
	fted_init_canvas();
	fted_create_button_size();
	fted_point_single0(0, 0, 0, 0, 0);

	/* signal(SIGWINCH, sigwinched); */
	sv.sv_handler = sigwinched;
	sv.sv_mask = sv.sv_onstack = 0;
	sigvec(SIGWINCH, &sv, 0);

	tool_install(fted_font_tool);
	tool_select(fted_font_tool, 0);
	tool_destroy(fted_font_tool);
	exit(0);
}


/* handy-dandy macros to create PANEL buttons and text items */
#define CREATE_BUTTON(panel, item, text, proc, x, y)			\
    item = panel_create_item(panel, PANEL_BUTTON, 			\
			     PANEL_LABEL_IMAGE, 			\
			     	panel_button_image(panel, 		\
						   text, strlen(text), NULL),\
			     PANEL_LABEL_Y, 		PANEL_CU(y) + 1,\
			     PANEL_NOTIFY_PROC, 	proc,		\
			     0);					\
    if (item == (Panel_item)NULL) {					\
	fprintf(stderr, "fontedit: can't create item: %s.\n", text);	\
	exit(-1);							\
    }
    
#define CREATE_TEXT_ITEM(panel, item, text, field_width, x,  y)		\
    item = panel_create_item(panel, PANEL_TEXT, 			\
				     PANEL_LABEL_STRING, 	text, 	\
				     PANEL_VALUE_DISPLAY_LENGTH, field_width,\
				     PANEL_LABEL_X, 		PANEL_CU(x), \
				     PANEL_LABEL_Y, 		PANEL_CU(y), \
				     0);				\
    if (item == (Panel_item)NULL) {					\
	fprintf(stderr, "fontedit: can't create item: %s.\n", text);	\
	exit(-1);							\
    }
    

fted_init_panel_window(tool, sub_win)
struct	tool	*tool;
struct	toolsw		**sub_win;
{   
    Panel	panel;
    
    
    *sub_win = panel_create(tool, 0);
    
    if (*sub_win == (struct toolsw *) NULL) {
	fprintf(stderr, "fontedit: can't create panel.\n");
	exit(-1);
    }
    panel = (Panel) (*sub_win)->ts_data;

    CREATE_BUTTON(panel, fted_read_item, "Load", fted_read_new_font, 1, 0);
    CREATE_BUTTON(panel, fted_save_item, "Store", fted_save_current_font, 7, 0);
    CREATE_BUTTON(panel, fted_exit_item, "Quit", fted_exit_fted_font_tool, 13, 0);
    CREATE_TEXT_ITEM(panel, fted_font_name_item, "Font name:", 60, 25, 0); 
    
    CREATE_TEXT_ITEM(panel, fted_max_width_item,  "  Max Width:", 8, 1, 1); 
    CREATE_TEXT_ITEM(panel, fted_mafted_x_height_item, " Max Height:", 8, 23, 1); 
    
    CREATE_TEXT_ITEM(panel, fted_caps_height_item,"Caps Height:", 8, 1, 2); 
    CREATE_TEXT_ITEM(panel, fted_x_height_item,   "   X Height:", 8, 23, 2); 
    CREATE_TEXT_ITEM(panel, fted_baseline_item,   "   Baseline:", 8, 45, 2); 
    CREATE_BUTTON(panel, fted_apply_item, "Apply", fted_apply_font_specs, 67, 2);
    
    fted_draw_options_item = panel_create_item(panel, PANEL_CHOICE, 
					  PANEL_LABEL_X, 	PANEL_CU(1),
					  PANEL_LABEL_Y, 	PANEL_CU(3), 
					  PANEL_LABEL_STRING, 	"Operation:", 
					  PANEL_CHOICE_STRINGS, 
					      "Single Pt", "Pt Wipe", "Line", 
					      "Rect", "Cut", "Copy", "Paste",
					      0, 
					  PANEL_NOTIFY_PROC,	
					      fted_set_drawing_option, 
					  PANEL_FEEDBACK, 	PANEL_MARKED,
					      0);
    
    panel_fit_height(panel);
}





/*
 * On a SIGWINCH, called the tool signal handler.
 */

    
static void
sigwinched()
{
	tool_sigwinch(fted_font_tool);
}



/*
 * This procedure does nothing, but does it quite well.
 */

fted_nullproc()
{
	return;
}


/*
 * Read in a new font by getting the value of the fted_font_name_item. Then call the
 * initialize routines so they can adjust to the size of the new font.	
 */
 
void fted_read_new_font()
{
    register int i;
    struct stat		stat_buff;
    register PIXFONT	*new_font;

    
    for(i = 0; i < NUM_EDIT_PADS; i++)
       if (fted_edit_pad_info[i].open) {
	   fted_message_user("Can't read in a new font when editing a character.");
	   return;
       }
       

    if (fted_font_modified) {
	int	result;
	Event	alert_event;

	result = alert_prompt(
	    (Frame)0,
	    &alert_event,
	    ALERT_MESSAGE_STRINGS,
		"Reading new font will discard unsaved",
		"modifications of current font.",
		"Please confirm reading of new font.",
		0,
	    ALERT_BUTTON_YES,	"Confirm",
	    ALERT_BUTTON_NO,	"Cancel",
	    0);
	if (result == ALERT_FAILED) {
	    fted_message_user("The font has been modified. Press the left button to confirm READ.");
	    if (!cursor_confirm(fted_canvas_sub_win->ts_windowfd)) {
	        fted_message_user(" ");
	        return;
	    }
	} else {
	    if (result == ALERT_NO) {
		fted_message_user(" ");
		return;
	    }
	}
	fted_message_user(" ");
    }

	
    (void) strcpy(fted_font_name, (char *)panel_get_value(fted_font_name_item));
    if ((new_font = pf_open_private(fted_font_name)) == NULL) {
	if (stat(fted_font_name, &stat_buff) == 0) {
	    fted_message_user("That font is not in vfont format.");
	    return;
	}
	new_font = alloctype(PIXFONT);
	fted_message_user("This is a new font.");
	sleep(2);
    }

    if (fted_cur_font != NULL)
        pf_close(fted_cur_font);
    fted_cur_font = new_font;

    fted_init_bounds(TRUE);
    fted_echo_init();
    fted_create_button_size();
    fted_redisplay();
    fted_font_modified = FALSE;
    fted_message_user(" ");
}

void fted_exit_fted_font_tool()
{
	int	result;
	Event	alert_event;

	result = alert_prompt(
	    (Frame)0,
	    &alert_event,
	    ALERT_MESSAGE_STRINGS,
		(fted_font_modified) ?
		    "Reading new font will discard unsaved" 
		    : "Are you sure you want to Quit?",
		(fted_font_modified) ?
		    "modifications of current font."
		    : "   ",
		(fted_font_modified) ?
		    "Please confirm reading of new font."
		    : "   ",
		0,
	    ALERT_NO_BEEPING,		(fted_font_modified) ? 0 : 1,
	    ALERT_OPTIONAL,		(fted_font_modified) ? 0 : 1,
	    ALERT_BUTTON_YES,		"Confirm",
	    ALERT_BUTTON_NO,		"Cancel",
	    0);
	if (result == ALERT_FAILED) {
	    if (fted_font_modified)
	        fted_message_user("The font has been modified. Press the left button to confirm EXIT.");
    	    else
		fted_message_user("Press the left button to confirm QUIT.");
	    if (!cursor_confirm(fted_canvas_sub_win->ts_windowfd)) {
		fted_message_user(" ");
		return;
	    }
	} else {
	    if (result == ALERT_NO) {
		fted_message_user(" ");
		return;
	    }
	}
	
	fted_message_user("Exiting fontedit.");
	tool_done(fted_font_tool);
}
    
void fted_apply_font_specs()
{
    char	string[100];
    int 	new_max_width, new_max_height, new_base_line;
    struct rect window;
    register struct pixrect *pr;
    int 	copy_it;
    register int i;
    struct pixchar *new_pix_char_ptr, *fted_copy_pix_char();
    

    if (fted_cur_font == NULL) {
	fted_message_user(
		"Sorry, I need a font to which these parameters can be applied.");
	return;
    }

    (void) strcpy(string, (char *)panel_get_value(fted_max_width_item));
    new_max_width = atoi(string);
    (void) strcpy(string, (char *)panel_get_value(fted_mafted_x_height_item));
    new_max_height = atoi(string);
    if (new_max_width <= 0) {
	fted_message_user("Width must be greater than zero");
	new_max_width = fted_char_max_width;
    }
    else if (new_max_width > MAX_FONT_CHAR_SIZE_WIDTH) {
	fted_message_user("Width is too large. Max is %d.", MAX_FONT_CHAR_SIZE_WIDTH);
	new_max_width = fted_char_max_width;
    }
    if (new_max_height <= 0) {
	fted_message_user("Height must be greater than zero.");
	new_max_height = fted_char_max_height;
    }
    else if (new_max_height > MAX_FONT_CHAR_SIZE_HEIGHT) {
	fted_message_user("Height is too large. Max is %d.", MAX_FONT_CHAR_SIZE_HEIGHT);
	new_max_height = fted_char_max_height;
    }
    if ( (new_max_width < fted_char_max_width) ||
    			(new_max_height < fted_char_max_height)) {
	window.r_top = window.r_left = 0;
	for (i = 0; i < fted_num_chars_in_font; i++) {
	    pr = fted_cur_font->pf_char[i].pc_pr;
	    if (pr != NULL) {
		copy_it = FALSE;
		if (pr->pr_size.x > new_max_width) {
		    window.r_width = new_max_width;
		    copy_it = TRUE;
		}
		else
		    window.r_width = pr->pr_size.x;
		    
		if ( pr->pr_size.y > new_max_height) {
		    window.r_height = new_max_height;
		    copy_it = TRUE;
		}
		else
		    window.r_height = pr->pr_size.y;
		    
		if (copy_it) {
		    new_pix_char_ptr = fted_copy_pix_char(&(window),
		    			&(fted_cur_font->pf_char[i]), FALSE);
		    fted_cur_font->pf_char[i].pc_pr = new_pix_char_ptr->pc_pr;
		    fted_cur_font->pf_char[i].pc_home = new_pix_char_ptr->pc_home;
		    fted_cur_font->pf_char[i].pc_adv.x = window.r_width;
		    fted_cur_font->pf_char[i].pc_adv.y = 0;
		    free(new_pix_char_ptr);
		}
		    
	    }
	} /*end for */
    }
    
    window.r_top = window.r_left = 0;
    window.r_width = new_max_width;
    window.r_height = new_max_height;
    for (i = 0; i < NUM_EDIT_PADS; i++) {
	if (fted_edit_pad_info[i].open) {
	    pr = fted_edit_pad_info[i].pix_char_ptr->pc_pr;
	    new_pix_char_ptr = fted_copy_pix_char(&(window),
					fted_edit_pad_info[i].pix_char_ptr , FALSE);
	    mem_destroy(fted_edit_pad_info[i].pix_char_ptr->pc_pr);
	    fted_edit_pad_info[i].pix_char_ptr->pc_pr = new_pix_char_ptr->pc_pr;
	    fted_edit_pad_info[i].pix_char_ptr->pc_home = new_pix_char_ptr->pc_home;
	    fted_edit_pad_info[i].pix_char_ptr->pc_adv.x =
	    		(new_pix_char_ptr->pc_adv.x > window.r_width ?
			 window.r_width : new_pix_char_ptr->pc_adv.x);
	    fted_edit_pad_info[i].pix_char_ptr->pc_adv.y = 0;

	    fted_edit_pad_info[i].window.r_width =
	    		(fted_edit_pad_info[i].window.r_width > window.r_width ?
			 window.r_width : fted_edit_pad_info[i].window.r_width);
	    fted_edit_pad_info[i].window.r_height = 
	    		(fted_edit_pad_info[i].window.r_height > window.r_height ?
			 window.r_height : fted_edit_pad_info[i].window.r_height);
	    free(new_pix_char_ptr);
	}
    } /*end for */
	
    fted_char_max_width = new_max_width;
    fted_char_max_height = new_max_height;

    (void) strcpy(string, (char *)panel_get_value(fted_caps_height_item));
    fted_font_caps_height = atoi(string);
    (void) strcpy(string, (char *)panel_get_value(fted_x_height_item));
    fted_font_x_height = atoi(string);
    (void) strcpy(string, (char *)panel_get_value(fted_baseline_item));
    new_base_line = atoi(string);
    if (new_base_line != fted_font_base_line) {
	fted_font_base_line = new_base_line;
	for (i = 0; i < fted_num_chars_in_font; i++) {
	    if (fted_cur_font->pf_char[i].pc_pr != NULL) 
		fted_cur_font->pf_char[i].pc_home.y = -fted_font_base_line;
	}
	for (i = 0; i < NUM_EDIT_PADS; i++) {
	    if (fted_edit_pad_info[i].open)
	        fted_edit_pad_info[i].pix_char_ptr->pc_home.y = -fted_font_base_line;
	}
    }
		
    fted_echo_init();
    fted_create_button_size ();
    fted_redisplay();
    fted_font_modified = TRUE;
}
