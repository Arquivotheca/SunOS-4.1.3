#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)iconedit_panel.c 1.1 92/07/30 SMI";
#endif
#endif

/**************************************************************************/
/*                            iconedit_panel.c                            */
/*             Copyright (c) 1986 by Sun Microsystems Inc.                */
/**************************************************************************/

#include "iconedit.h"
#include <sys/param.h>
#include <sys/stat.h>
#include <suntool/alert.h>
#include <suntool/icon_load.h>
#include <suntool/frame.h>
#include <suntool/fullscreen.h>

static int confirmed;

static Menu store_menu;

extern char     *sys_errlist[];
extern int       errno;

/**************************************************************************/
/* pixfonts                                                               */
/**************************************************************************/

struct pixfont	*iced_screenr7, *iced_screenr11, *iced_screenb12,
		*iced_screenr12, *iced_screenr14, *iced_screenb14,
		*iced_cmrb14, *iced_cmrr14, *iced_gallantr19,
		*iced_font, *iced_bold_font,
		*iced_abc_font;

/**************************************************************************/
/* for invoking store or quit with keyboard accelerators                  */
/**************************************************************************/

#define ESC     27      /* escape for file-name completion */
#define CTRL_L  12	/* control-L for "load" */
#define CTRL_S  19	/* control-S for "store" */
#define CTRL_B  2	/* control-B for "browse" */
#define CTRL_Q  17	/* control-Q for "quit" */

static int store_invoked_from_keyboard = FALSE;
static int quit_invoked_from_keyboard  = FALSE;
static int ctrl_s_pending              = FALSE;
static int ctrl_q_pending              = FALSE;

/************************************************************************/
/* mouse and msg panel declarations                                     */
/************************************************************************/

Panel iced_mouse_panel;
Panel iced_msg_panel;

static Panel_item	mousemsg_left_mouse_item;
static Panel_item	mousemsg_left_text_item;
static Panel_item	mousemsg_mid_mouse_item;
static Panel_item	mousemsg_mid_text_item;
static Panel_item	mousemsg_right_mouse_item;
static Panel_item	mousemsg_right_text_item;
static Panel_item	canvasmsg_item;
static Panel_item	panelmsg_top_item;
static Panel_item	panelmsg_bottom_item;

static void		confirm_mouse_proc();
static void		canvas_mouse_proc();

/************************************************************************/
/* control panel declarations                                           */
/************************************************************************/

Panel iced_panel;

Panel_item		iced_dir_item;
Panel_item		iced_fname_item;
Panel_item		iced_load_item;
Panel_item		iced_store_item;
Panel_item		iced_browse_item;
Panel_item		iced_quit_item;
Panel_item		iced_size_item;
Panel_item		iced_grid_item;
Panel_item		iced_draw_item;
Panel_item		iced_mode_item;
Panel_item		iced_clear_item;
Panel_item		iced_invert_item;
Panel_item		iced_fill_canvas_item;
Panel_item		iced_fill_square_item;
Panel_item		iced_fill_circle_item;
Panel_item		iced_abc_item;
Panel_item		iced_font_item;
Panel_item		iced_proof_op_item;
Panel_item		iced_load_op_item;
Panel_item		iced_fill_op_item;
Panel_item		iced_proof_background_item;

static Panel_setting	dir_proc();
static Panel_setting	fname_proc();
static void		store_proc();
static void		quit_proc();
static void		size_proc();
static void		grid_proc();
static void		mode_proc();
static void		invert_proc();
static void		fill_canvas_proc();
static void		fill_square_proc();
static void		fill_circle_proc();
static Panel_setting	abc_proc();
static void		font_proc();
static void		proof_background_proc();
static void		proof_op_proc();

static void		store_button_event_proc();

static int		val_to_op();

/************************************************************************/
/* iced_init_mouse_panel                                                     */
/************************************************************************/

iced_init_mouse_panel() {

   iced_mouse_panel = window_create(iced_base_frame, PANEL,
		WIN_WIDTH,          iced_mouse_panel_width,
		WIN_HEIGHT,         iced_mouse_panel_height,
		WIN_FONT,           iced_bold_font,
		PANEL_LABEL_BOLD,   FALSE,
                WIN_ERROR_MSG,      "Unable to create mouse panel\n",
	 0);
   if (iced_mouse_panel == NULL) {
                (void)fprintf(stderr,"Unable to create mouse panel\n");
                exit(1);
   }

   mousemsg_left_mouse_item = panel_create_item(iced_mouse_panel, PANEL_MESSAGE,
		PANEL_LABEL_X,        10+PANEL_CU(11),
		PANEL_LABEL_Y,        2,
		PANEL_LABEL_IMAGE,    &iced_mouse_left,
	        0);
   mousemsg_left_text_item = panel_create_item(iced_mouse_panel, PANEL_MESSAGE,
		PANEL_LABEL_X,        30+PANEL_CU(11),
		PANEL_LABEL_Y,        3,
		PANEL_LABEL_STRING,   "Paint",
	        0);
   mousemsg_mid_mouse_item = panel_create_item(iced_mouse_panel, PANEL_MESSAGE,
		PANEL_LABEL_X,        105+PANEL_CU(11),
		PANEL_LABEL_Y,        2,
		PANEL_LABEL_IMAGE,    &iced_mouse_middle,
	        0);
   mousemsg_mid_text_item = panel_create_item(iced_mouse_panel, PANEL_MESSAGE,
		PANEL_LABEL_X,        125+PANEL_CU(11),
		PANEL_LABEL_Y,        3,
		PANEL_LABEL_STRING,   "Clear",
	        0);
   mousemsg_right_mouse_item = panel_create_item(iced_mouse_panel, PANEL_MESSAGE,
		PANEL_LABEL_X,        200+PANEL_CU(11),
		PANEL_LABEL_Y,        2,
		PANEL_LABEL_IMAGE,    &iced_mouse_right,
	        0);
   mousemsg_right_text_item = panel_create_item(iced_mouse_panel, PANEL_MESSAGE,
		PANEL_LABEL_X,        220+PANEL_CU(11),
		PANEL_LABEL_Y,        3,
		PANEL_LABEL_STRING,   "Undo",
	        0);
   canvasmsg_item = panel_create_item(iced_mouse_panel, PANEL_MESSAGE,
		PANEL_LABEL_X,        10,
		PANEL_LABEL_Y,        22,
		PANEL_LABEL_STRING,
		   "       Points:  Pick points to Paint or Clear.",
	        0);
}

/************************************************************************/
/* iced_init_msg_panel                                                       */
/************************************************************************/

iced_init_msg_panel() {

   iced_msg_panel = window_create(iced_base_frame, PANEL,
		WIN_WIDTH,		iced_msg_panel_width,
		WIN_HEIGHT,		iced_msg_panel_height,
		WIN_FONT,		iced_bold_font,
		PANEL_LABEL_BOLD,	FALSE,
                WIN_ERROR_MSG,          "Unable to create msg panel\n",
		0);
   if (iced_msg_panel == NULL) {
                (void)fprintf(stderr,"Unable to create msg panel\n");
                exit(1);
   }

   panelmsg_top_item = panel_create_item(iced_msg_panel, PANEL_MESSAGE,
		PANEL_LABEL_X,        10,
		PANEL_LABEL_Y,        3,
		PANEL_LABEL_STRING,   "",
	        0);

   panelmsg_bottom_item = panel_create_item(iced_msg_panel, PANEL_MESSAGE,
		PANEL_LABEL_X,        10,
		PANEL_LABEL_Y,        22,
		PANEL_LABEL_STRING,   "",
	        0);
}

/************************************************************************/
/* message routines                                                     */
/************************************************************************/

static
mousemsg(left,mid,right) char *left,*mid,*right; {
   panel_set(mousemsg_left_text_item,  PANEL_LABEL_STRING, left,  0);
   panel_set(mousemsg_mid_text_item,   PANEL_LABEL_STRING, mid,   0);
   panel_set(mousemsg_right_text_item, PANEL_LABEL_STRING, right, 0);
}

static
canvasmsg(msg) char *msg; {
   panel_set(canvasmsg_item, PANEL_LABEL_STRING, msg, 0);
}

iced_panelmsg(top_msg,bottom_msg) char *top_msg,*bottom_msg; {
   panel_set(panelmsg_top_item,    PANEL_LABEL_STRING, top_msg,    0);
   panel_set(panelmsg_bottom_item, PANEL_LABEL_STRING, bottom_msg, 0);
}

/************************************************************************/
/* control panel section                                                */
/************************************************************************/

iced_init_panel() {

     int row1 = 5;
     int row2 = 28;
     int row3 = 53;
     int row4 = 84;
     int row5 = 109;
     char current_directory[MAXPATHLEN+1];

     iced_abc_font = iced_screenr12;

     iced_panel = window_create(iced_base_frame, PANEL,
	  	WIN_WIDTH,	       iced_panel_width,
		WIN_HEIGHT,	       iced_panel_height,
		WIN_CURSOR,	       &iconedit_main_cursor,
	        WIN_FONT,              iced_bold_font,
                WIN_ERROR_MSG,         "Unable to create iced panel\n",
	        PANEL_LABEL_BOLD,   FALSE,
		0);
     if (iced_panel == NULL) { 
                (void)fprintf(stderr,"Unable to create iced panel\n"); 
                exit(1); 
     }

     (void) getwd(current_directory);
     iced_dir_item = panel_create_item(iced_panel, PANEL_TEXT,
	PANEL_LABEL_X,             10,
	PANEL_LABEL_Y,             row1,
        PANEL_VALUE_DISPLAY_LENGTH,21,
	PANEL_LABEL_BOLD,          FALSE,
	PANEL_LABEL_FONT,          iced_bold_font,
	PANEL_VALUE_FONT,          iced_bold_font,
	PANEL_LABEL_STRING,        "Dir: ",
	PANEL_VALUE,               current_directory,
	PANEL_MENU_TITLE_STRING,   " Current Directory",
	PANEL_MENU_CHOICE_FONTS,   iced_bold_font,0,
	PANEL_MENU_CHOICE_STRINGS, " ^L - Load image from file",
			           " ^S - Store image to file",
			           " ^B - Browse directory", 
			           " ^Q - Quit",
			           0,
	PANEL_MENU_CHOICE_VALUES,  CTRL_L, CTRL_S, CTRL_B, CTRL_Q, 0,
	PANEL_SHOW_MENU,           TRUE,
	PANEL_NOTIFY_LEVEL,        PANEL_ALL,
	PANEL_NOTIFY_PROC,         dir_proc,	
	0);

     iced_fname_item = panel_create_item(iced_panel, PANEL_TEXT,
	PANEL_LABEL_X,             10,
	PANEL_LABEL_Y,             row2,
        PANEL_VALUE_DISPLAY_LENGTH,21,
	PANEL_LABEL_FONT,          iced_bold_font,
	PANEL_LABEL_BOLD,          FALSE,
	PANEL_LABEL_STRING,        "File:",
	PANEL_NOTIFY_LEVEL,        PANEL_ALL,
	PANEL_MENU_TITLE_STRING,   " Current File",
	PANEL_MENU_CHOICE_FONTS,   iced_bold_font,0,
        PANEL_MENU_CHOICE_STRINGS, "ESC - Filename completion",
				   " ^L - Load image from file",
			           " ^S - Store image to file",
			           " ^B - Browse directory", 
			           " ^Q - Quit",
			           0,
	PANEL_MENU_CHOICE_VALUES,  ESC, CTRL_L, CTRL_S, CTRL_B, CTRL_Q, 0,
	PANEL_SHOW_MENU,           TRUE,
	PANEL_VALUE_FONT,          iced_bold_font,
	PANEL_NOTIFY_PROC,         fname_proc,	
	0);

     iced_load_item  = panel_create_item(iced_panel, PANEL_BUTTON,
	PANEL_LABEL_X,             10,
	PANEL_LABEL_Y,             row3,
	PANEL_LABEL_IMAGE,         panel_button_image(iced_panel,"Load",4,iced_bold_font),
	PANEL_SHOW_MENU,           TRUE,
	PANEL_MENU_CHOICE_STRINGS, "Load image from file",0,
	PANEL_MENU_CHOICE_FONTS,   iced_bold_font,0,
	PANEL_NOTIFY_PROC,         iced_load_proc,
	0);

     iced_store_item = panel_create_item(iced_panel, PANEL_BUTTON,
	PANEL_LABEL_X,             60,
	PANEL_LABEL_Y,             row3,
	PANEL_LABEL_IMAGE,        panel_button_image(iced_panel,"Store",5,iced_bold_font),
	/*
	PANEL_SHOW_MENU,           TRUE,
	PANEL_MENU_CHOICE_STRINGS, "Store image to file", 0,
	*/
	PANEL_MENU_CHOICE_FONTS,   iced_bold_font,0,
	PANEL_NOTIFY_PROC,         store_proc,
	PANEL_EVENT_PROC,          store_button_event_proc,
	0);

     store_menu = menu_create(MENU_STRINGS, "Store entire image",
					    "Store trimmed image",
					    0,
                              0);

     iced_browse_item  = panel_create_item(iced_panel, PANEL_BUTTON,
	PANEL_LABEL_X,             118,
	PANEL_LABEL_Y,             row3,
	PANEL_LABEL_IMAGE,       panel_button_image(iced_panel,"Browse",6,iced_bold_font),
	PANEL_SHOW_MENU,           TRUE,
	PANEL_MENU_CHOICE_STRINGS, "Browse images in current directory",0,
	PANEL_MENU_CHOICE_FONTS,   iced_bold_font,0,
	PANEL_NOTIFY_PROC,         iced_browse_proc,
	0);

     iced_quit_item  = panel_create_item(iced_panel, PANEL_BUTTON,
	PANEL_LABEL_X,             184,
	PANEL_LABEL_Y,             row3,
	PANEL_LABEL_IMAGE,         panel_button_image(iced_panel,"Quit",4,iced_bold_font),
	PANEL_NOTIFY_PROC,         quit_proc,
	0);

     iced_size_item = panel_create_item(iced_panel, PANEL_CYCLE,
	PANEL_ITEM_X,            10,
	PANEL_ITEM_Y,            row4 - 4,
	PANEL_MARK_YS,		 row4, 0,
	PANEL_LABEL_STRING,      "Size",
	PANEL_CHOICE_STRINGS,    "Icon", "Cursor", 0,
	PANEL_NOTIFY_PROC,       size_proc,
	0);

     iced_grid_item = panel_create_item(iced_panel, PANEL_CYCLE,
	PANEL_ITEM_X,            145,
	PANEL_ITEM_Y,            row4 - 4,
	PANEL_LABEL_STRING,      "Grid",
	PANEL_MARK_YS,		    row4, 0,
	PANEL_CHOICE_STRINGS,    "Off", "On", 0,
	PANEL_NOTIFY_PROC,       grid_proc,
	0);

     /*
     iced_size_item = panel_create_item(iced_panel, PANEL_CYCLE,
	PANEL_ITEM_X,            10,
	PANEL_ITEM_Y,            88,
	PANEL_LABEL_FONT,        iced_bold_font,
	PANEL_CHOICE_FONTS,      iced_bold_font, 0,
	PANEL_LABEL_BOLD,        FALSE,
	PANEL_LABEL_STRING,      "Size:",
	PANEL_CHOICE_STRINGS,    "Icon", "Cursor", 0,
	PANEL_MENU_CHOICE_FONTS, iced_bold_font,0,
	PANEL_MENU_TITLE_STRING, "Canvas Size",
	PANEL_MENU_TITLE_FONT,   iced_bold_font,
	PANEL_MENU_CHOICE_STRINGS, "Icon   (64 x 64)", "Cursor (16 x 16)", 0,
	PANEL_NOTIFY_PROC,       size_proc,
	0);

     iced_grid_item = panel_create_item(iced_panel, PANEL_CYCLE,
	PANEL_ITEM_X,              135,
	PANEL_ITEM_Y,              88,
	PANEL_LABEL_STRING,        "Grid:",
	PANEL_LABEL_FONT,          iced_bold_font,
	PANEL_CHOICE_FONTS,        iced_bold_font, 0,
	PANEL_LABEL_BOLD,          FALSE,
	PANEL_CHOICE_STRINGS,      "On", "Off", 0,
	PANEL_MENU_TITLE_STRING,   "Canvas Grid ",
	PANEL_MENU_TITLE_FONT,     iced_bold_font,
	PANEL_MENU_CHOICE_STRINGS, "Off","On",0, 
	PANEL_MENU_CHOICE_FONTS,   iced_bold_font,0,
	PANEL_NOTIFY_PROC,         grid_proc,
	0);
     */

     /*
     iced_size_item = panel_create_item(iced_panel, PANEL_CHOICE,
	PANEL_LABEL_X,           23,
	PANEL_LABEL_Y,           61,
	PANEL_LABEL_FONT,        iced_bold_font,
	PANEL_LABEL_BOLD,        FALSE,
	PANEL_LABEL_STRING,      "Size:",
	PANEL_FEEDBACK,          PANEL_MARKED,
	PANEL_MARK_IMAGES,       &tri_right,0,
	PANEL_NOMARK_IMAGES,     0,
	PANEL_CHOICE_FONTS,      iced_bold_font, 0,
	PANEL_CHOICE_STRINGS,    "Icon", "Cursor", 0,
	PANEL_MARK_XS,           76,133,0,
	PANEL_MARK_YS,           row3 + 4,0,
	PANEL_CHOICE_XS,         90,146,0,
	PANEL_CHOICE_YS,         row3 + 2,0,
	PANEL_MENU_CHOICE_FONTS, iced_bold_font,0,
	PANEL_MENU_TITLE_STRING, "Canvas Size",
	PANEL_MENU_TITLE_FONT,   iced_bold_font,
	PANEL_MENU_CHOICE_STRINGS, "Icon   (64 x 64)",
	                         "Cursor (16 x 16)",
	                         0,
	PANEL_NOTIFY_PROC,       size_proc,
	0);

     iced_grid_item = panel_create_item(iced_panel, PANEL_CHOICE,
	PANEL_LABEL_X,             23,
	PANEL_LABEL_Y,             83,
	PANEL_LABEL_STRING,        "Grid:",
	PANEL_LABEL_FONT,          iced_bold_font,
	PANEL_LABEL_BOLD,          FALSE,
	PANEL_FEEDBACK,            PANEL_MARKED,
	PANEL_MARK_IMAGES,         &tri_right,0,
	PANEL_NOMARK_IMAGES,       0,
	PANEL_CHOICE_OFFSET,       15,
	PANEL_CHOICE_IMAGES,       &square_white,&grid_pr,0,
	PANEL_MARK_XS,             76,133,0, 
	PANEL_MARK_YS,             row4 + 4,0, 
	PANEL_CHOICE_XS,           90,147,0,
	PANEL_CHOICE_YS,           row4,0, 
	PANEL_MENU_TITLE_STRING,   "Canvas Grid ",
	PANEL_MENU_TITLE_FONT,     iced_bold_font,
	PANEL_MENU_CHOICE_STRINGS, "Off","On",0, 
	PANEL_MENU_CHOICE_FONTS,   iced_bold_font,0,
	PANEL_NOTIFY_PROC,         grid_proc,
	0);
	*/

     iced_clear_item = panel_create_item(iced_panel, PANEL_BUTTON,
	PANEL_LABEL_X,             10,
	PANEL_LABEL_Y,             row5, 
	PANEL_LABEL_IMAGE,        panel_button_image(iced_panel,"Clear",6,iced_bold_font),
	PANEL_SHOW_MENU,           TRUE,
	PANEL_MENU_CHOICE_STRINGS, "Clear canvas",0,
	PANEL_MENU_CHOICE_FONTS,   iced_bold_font,0,
	PANEL_NOTIFY_PROC,         iced_clear_proc,
	0);

     iced_fill_canvas_item = panel_create_item(iced_panel, PANEL_BUTTON,
	PANEL_LABEL_X,             89,
	PANEL_LABEL_Y,             row5, 
	PANEL_LABEL_IMAGE,         panel_button_image(iced_panel,"Fill",6,iced_bold_font),
	PANEL_SHOW_MENU,           TRUE,
	PANEL_MENU_CHOICE_STRINGS, "Fill canvas with rectangle fill pattern",0,
	PANEL_MENU_CHOICE_FONTS,   iced_bold_font,0,
	PANEL_NOTIFY_PROC,         fill_canvas_proc,
	0);

     iced_invert_item = panel_create_item(iced_panel, PANEL_BUTTON,
	PANEL_LABEL_X,             168,
	PANEL_LABEL_Y,             row5, 
	PANEL_LABEL_IMAGE,      panel_button_image(iced_panel,"Invert",6,iced_bold_font),
	PANEL_SHOW_MENU,           TRUE,
	PANEL_MENU_CHOICE_STRINGS, "Invert canvas",0,
	PANEL_MENU_CHOICE_FONTS,   iced_bold_font,0,
	PANEL_NOTIFY_PROC,         invert_proc,
	0);

     iced_mode_item = panel_create_item(iced_panel, PANEL_CHOICE,
	PANEL_FEEDBACK,            PANEL_MARKED,
	PANEL_CHOICE_IMAGES,       &iced_points_pr,
				   &iced_line_pr,
				   &iced_box_pr,
				   &iced_circle_pr,
				   &iced_screenb12_pr,
				   0,
	PANEL_CHOICE_XS,           57,0,
	PANEL_CHOICE_YS,           160,185,208,238,264,0,
	PANEL_MARK_XS,             6,0,
	PANEL_MARK_YS,             140,165,190,218,249,0,
	PANEL_MARK_IMAGES,         &iced_drawing_hand,0,
	PANEL_NOMARK_IMAGES,       0,
	PANEL_MENU_TITLE_STRING,   "Drawing Mode ",
	PANEL_MENU_TITLE_FONT,     iced_bold_font,
	PANEL_MENU_CHOICE_STRINGS, "Points ",
	                           "Line   ",
	                           "Rectangle ",
	                           "Circle ",
				   "Text   ",
	                           0, 
	PANEL_MENU_CHOICE_FONTS,   iced_bold_font,0,
	PANEL_NOTIFY_PROC,         mode_proc,
	0);

     iced_fill_square_item  = panel_create_item(iced_panel, PANEL_CYCLE,
	PANEL_LABEL_X,           97,
	PANEL_LABEL_Y,           213 - 6,
	PANEL_LABEL_STRING,      "Fill",
	PANEL_LABEL_FONT,        iced_bold_font,
	PANEL_LABEL_BOLD,        FALSE,

	PANEL_MARK_YS,		    213 - 2, 0,

        PANEL_CHOICE_FONTS,      iced_bold_font,0,
	PANEL_CHOICE_STRINGS,    "Border",
			         "White",
				 "25% Grey",
				 "root Grey", 
				 "50% Grey",
				 "75% Grey",  
				 "Black",
				 0, 
	PANEL_MENU_TITLE_STRING, "Rectangle Fill Pattern ",
	PANEL_MENU_TITLE_FONT,   iced_bold_font,
	PANEL_NOTIFY_PROC,       fill_square_proc,
	0);

     iced_fill_circle_item  = panel_create_item(iced_panel, PANEL_CYCLE,
	PANEL_LABEL_X,           97,
	PANEL_LABEL_Y,           241 - 6,
	PANEL_LABEL_STRING,      "Fill",

	PANEL_MARK_YS,		 241 - 2, 0,

	PANEL_LABEL_FONT,        iced_bold_font,
	PANEL_LABEL_BOLD,        FALSE,
        PANEL_CHOICE_FONTS,      iced_bold_font,0,
	PANEL_CHOICE_STRINGS,    "Border",
			         "White",
				 "25% Grey",
				 "root Grey", 
				 "50% Grey",
				 "75% Grey",  
				 "Black",
				 0, 
	PANEL_MENU_TITLE_STRING, "Circle Fill Pattern ",
	PANEL_MENU_TITLE_FONT,   iced_bold_font,
	PANEL_NOTIFY_PROC,       fill_circle_proc,
	0);

     iced_font_item  = panel_create_item(iced_panel, PANEL_CHOICE,
	PANEL_LABEL_X,             97,
	PANEL_LABEL_Y,             268,
	PANEL_LABEL_FONT,          iced_bold_font,
	PANEL_LABEL_BOLD,          FALSE,
	PANEL_LABEL_STRING,        "Fill:",
	PANEL_DISPLAY_LEVEL,       PANEL_NONE,
	PANEL_MENU_TITLE_STRING,   "Font for Inserted Text ",
	PANEL_VALUE,               SCREEN_B_12,
	PANEL_CHOICE_STRINGS,      "screen.r.7",
				   "screen.r.11",
				   "screen.r.12",
				   "screen.b.12",
				   "screen.r.14",
				   "screen.b.14",
				   "cmr.r.14",
				   "cmr.b.14",
				   "gallant.r.19",
			           0,
	PANEL_MENU_CHOICE_STRINGS, "screen 7 pt.",
				   "screen 11 pt.",
				   "screen 12 pt.",
				   "screen 12 pt. bold",
				   "screen 14 pt.",
				   "screen 14 pt. bold",
				   "computer modern 14 pt.",
				   "computer modern 14 pt. bold ",
				   "gallant 19 pt.",
			           0,
	PANEL_MENU_CHOICE_FONTS,   iced_screenr7,
				   iced_screenr11,
				   iced_screenr12,
				   iced_screenb12,
				   iced_screenr14,
				   iced_screenb14,
				   iced_cmrr14,
				   iced_cmrb14,
				   iced_gallantr19,
				   0,
	PANEL_NOTIFY_PROC,         font_proc,
	0);

     iced_abc_item  = panel_create_item(iced_panel, PANEL_TEXT,
	PANEL_LABEL_X,             144,
	PANEL_LABEL_Y,             268,
	PANEL_VALUE_FONT,          iced_screenb12,
	PANEL_LABEL_STRING,        "",
        PANEL_VALUE_DISPLAY_LENGTH,10,
	PANEL_NOTIFY_LEVEL,        PANEL_ALL,
	PANEL_SHOW_MENU,           FALSE,
	PANEL_NOTIFY_PROC,         abc_proc,
	0);

     iced_load_op_item   = panel_create_item(iced_panel, PANEL_CYCLE,
		      PANEL_MARK_XS,		    32, 0,
		      PANEL_MARK_YS,		    316+3, 0,
		      PANEL_LABEL_Y,	       304,
		      PANEL_LABEL_X,	       32,
		      PANEL_VALUE_Y,           316,
		      PANEL_VALUE_X,           32,
		      PANEL_LABEL_STRING,      "Load",
	              PANEL_LABEL_FONT,        iced_bold_font,
	              PANEL_LABEL_BOLD,        FALSE,
		      PANEL_CHOICE_FONTS,      iced_bold_font,0,
		      PANEL_CHOICE_STRINGS,    "Src", "Or", "Xor", "And", 0,
		      PANEL_MENU_TITLE_STRING, "Raster Op For Loading ",
		      PANEL_MENU_TITLE_FONT,   iced_bold_font,
		      0);

     iced_fill_op_item   = panel_create_item(iced_panel, PANEL_CYCLE,
		      PANEL_MARK_XS,		    97, 0,
		      PANEL_MARK_YS,		    316+3, 0,
		      PANEL_LABEL_Y,	       304,
		      PANEL_LABEL_X,	       97,
		      PANEL_VALUE_Y,           316,
		      PANEL_VALUE_X,           97,
		      PANEL_LABEL_STRING,      "Fill",
	              PANEL_LABEL_FONT,        iced_bold_font,
	              PANEL_LABEL_BOLD,        FALSE,
		      PANEL_CHOICE_FONTS,      iced_bold_font,0,
		      PANEL_CHOICE_STRINGS,    "Src", "Or", "Xor", "And", 0,
		      PANEL_MENU_TITLE_STRING, "Raster Op For Filling ",
		      PANEL_MENU_TITLE_FONT,   iced_bold_font,
		      0);

     iced_proof_op_item  = panel_create_item(iced_panel, PANEL_CYCLE,
		      PANEL_MARK_XS,	       154, 0,
		      PANEL_MARK_YS,	       316+3, 0,
		      PANEL_LABEL_Y,	       304,
		      PANEL_LABEL_X,	       154,
		      PANEL_VALUE_Y,           316,
		      PANEL_VALUE_X,           154,
		      PANEL_LABEL_STRING,      "Proof",
	              PANEL_LABEL_FONT,        iced_bold_font,
	              PANEL_LABEL_BOLD,        FALSE,
		      PANEL_CHOICE_FONTS,      iced_bold_font,0,
		      PANEL_CHOICE_STRINGS,    "Src", "Or", "Xor", "And", 0,
		      PANEL_MENU_TITLE_STRING, "Raster Op For Proof ",
		      PANEL_MENU_TITLE_FONT,   iced_bold_font,
		      PANEL_NOTIFY_PROC,   proof_op_proc,
		      0);

#define Z 33 
#define Q 37

     iced_proof_background_item  = panel_create_item(iced_panel, PANEL_CHOICE,
	PANEL_FEEDBACK,            PANEL_MARKED,
	PANEL_MARK_IMAGES,         &iced_tri_down,0,
	PANEL_NOMARK_IMAGES,       0,
	PANEL_CHOICE_IMAGES,       &iced_square_white,
				   &iced_square_25,
				   &iced_square_root,
				   &iced_square_50,
				   &iced_square_75,
				   &iced_square_black,
				   0,
	PANEL_CHOICE_XS,            Z, Z+30, Z+60, Z+90, Z+120, Z+150, 0,
	PANEL_CHOICE_YS,           345,345,345,345,345,345,0,
	PANEL_MARK_XS,              Q, Q+30, Q+60, Q+90, Q+120, Q+150,0,
	PANEL_MARK_YS,             363,363,363,363,363,363,0,
	PANEL_MENU_TITLE_STRING,   "Proof Background ",
	PANEL_MENU_TITLE_FONT,     iced_bold_font,
	PANEL_MENU_CHOICE_STRINGS, "White",
				   "25%  Grey",
				   "root Grey", 
				   "50%  Grey",
				   "75%  Grey",  
				   "Black",
				   0, 
	PANEL_MENU_CHOICE_FONTS,   iced_bold_font,0,
	PANEL_VALUE,               GR_ROOT_GRAY,
	PANEL_NOTIFY_PROC,         proof_background_proc,
	0);
}

/************************************************************************/
/* handlers for the mouse and message panel items                       */
/************************************************************************/

static void 
confirm_mouse_proc() 
{
   mousemsg("Confirm","Cancel","Cancel");
}

static void 
canvas_mouse_proc() 
{
   mousemsg("Paint","Clear","Undo");
}

/************************************************************************/
/* handlers for the control panel items, in their creation order        */
/************************************************************************/

static Panel_setting 
dir_proc(item, event) 
Panel_item item; 
Event *event; 
{
   iced_browser_filled = FALSE;
   switch (event_action(event)) {

      case CTRL_L:
	 iced_load_proc(item);
	 ctrl_s_pending = ctrl_q_pending = FALSE;
	 return PANEL_NONE;

      case CTRL_S:
	 store_invoked_from_keyboard = TRUE;
	 store_proc(item);
	 store_invoked_from_keyboard = FALSE;
	 ctrl_q_pending              = FALSE;
	 ctrl_s_pending              = ctrl_s_pending ? FALSE : TRUE;
	 return PANEL_NONE;

      case CTRL_B: 
	 iced_browse_proc();
	 return PANEL_NONE;

      case CTRL_Q:
	 quit_invoked_from_keyboard = TRUE;
	 quit_proc(item);
	 quit_invoked_from_keyboard = FALSE;
	 ctrl_s_pending             = FALSE;
	 ctrl_q_pending             = ctrl_q_pending ? FALSE : TRUE;
	 return PANEL_NONE;

      default:
	 if (ctrl_s_pending || ctrl_q_pending) {
	    iced_panelmsg("","");
	    ctrl_s_pending = ctrl_q_pending = FALSE;
	 }
	 return panel_text_notify(item, event);
   }
}

int
iced_change_directory()
{
   struct stat      stat_buf;
   char *new_dir;
   
   new_dir = (char *)panel_get_value(iced_dir_item);

   if (stat(new_dir, &stat_buf) < 0)
      return (0);

   if (chdir(new_dir) < 0)
      return (0);

   return (1);
}

/*
static int
textsw_change_directory(textsw, filename, might_not_be_dir, locx, locy)
        Text_handle      textsw;
        char            *filename;
        int              might_not_be_dir;
        int              locx, locy;
{
        char            *sys_msg;
        char            *full_pathname;
        char             msg[MAXNAMLEN+100];
        struct stat      stat_buf;
        int              result = 0;
 
        errno = 0;
        if (stat(filename, &stat_buf) < 0) {
            result = -1;
            goto Error;
        }
        if ((stat_buf.st_mode&S_IFMT) != S_IFDIR) {
            if (might_not_be_dir)
                return(-2);
        }
        if (chdir(filename) < 0) {
            result = errno;
            goto Error;
        }
        textsw_notify(textsw, TEXTSW_ACTION_CHANGED_DIRECTORY, filename, 0);
        return(result);
Error:
        full_pathname = textsw_full_pathname(filename);
        sprintf(msg, "Cannot %s '%s': ",
                (might_not_be_dir ? "access file" : "cd to directory"),
                full_pathname);
        free(full_pathname);
        sys_msg = (errno > 0 && errno < sys_nerr) ? sys_errlist[errno] : NULL;
        textsw_post_error(textsw, locx, locy, msg, sys_msg);
        return(result);
}
*/

static Panel_setting 
fname_proc(item,event) 
Panel_item item; 
struct inputevent *event; 
{

   iced_browser_filled = FALSE;
   switch (event_action(event)) {

      case ESC:
	 iced_complete_filename();
         ctrl_s_pending = ctrl_q_pending = FALSE;
         return PANEL_NONE;

      case CTRL_L:
	 iced_load_proc(item);
	 ctrl_s_pending = ctrl_q_pending = FALSE;
	 return PANEL_NONE;

      case CTRL_S:
	 store_invoked_from_keyboard = TRUE;
	 store_proc(item);
	 store_invoked_from_keyboard = FALSE;
	 ctrl_q_pending              = FALSE;
	 ctrl_s_pending              = ctrl_s_pending ? FALSE : TRUE;
	 return PANEL_NONE;

      case CTRL_B: iced_browse_proc();
	 return PANEL_NONE;

      case CTRL_Q:
	 quit_invoked_from_keyboard = TRUE;
	 quit_proc(item);
	 quit_invoked_from_keyboard = FALSE;
	 ctrl_s_pending             = FALSE;
	 ctrl_q_pending             = ctrl_q_pending ? FALSE : TRUE;
	 return PANEL_NONE;

      default:
	 if (ctrl_s_pending || ctrl_q_pending) {
	    iced_panelmsg("","");
	    ctrl_s_pending = ctrl_q_pending = FALSE;
	 }
	 return panel_text_notify(item, event);
   }
}

/************************************************************************/
/* Load and Store                                                       */
/************************************************************************/

static struct pixrect *
load_old_format(file_name, error_msg)
	char		*file_name, *error_msg;
{
   int		 	c, count;
   FILE			*fd;
   icon_header_object	header;
   struct pixrect	*pr;

   fd = fopen(file_name, "r");
   if (fd == NULL) {
      iced_panelmsg("","Can't open file.");
      return NULL_PR;
   }
   while ( (c= getc(fd)) != '{')  {		/*  matching  }	*/
      if (c==EOF)  {
	 iced_panelmsg("Incorrect file format:","opening brace missing. ");
	 fclose(fd);
	 return NULL_PR;
      }
   }

   header.last_param_pos = ftell(fd);

   count = 0;
   while (1) {
	long junk;

	if (fscanf(fd, " 0x%X,", &junk) != 1)
		break;
	count++;
   };

   fseek(fd, header.last_param_pos, 0);

   switch (count)  {
    case   8:	header.height = 16;
		header.valid_bits_per_item = 32;
		break;
    case  16:	header.height = 16;
		header.valid_bits_per_item = 16;
		break;
    case 128:	header.height = 64;
		header.valid_bits_per_item = 32;
		break;
    case 256:	header.height = 64;
		header.valid_bits_per_item = 16;
		break;
    default:    iced_panelmsg(
    			"Incorrect file format:","wrong # of scan lines.");
		fclose(fd);
		return NULL_PR;
   }

   header.depth = 1;
   header.format_version = 1;
   header.width = header.height;

   pr = mem_create(header.width, header.height, header.depth);

   if (pr == NULL_PR) {
      strcpy(error_msg, "icon_load: pixrect create failed");
   }
   else if (icon_read_pr(fd, &header, pr)) {
      strcpy(error_msg, "icon_load: icon read failed");
      pr_destroy(pr);
      pr = NULL_PR;
   }

   fclose(fd);

   return pr;
}

static void
get_file_name(item)
Panel_item item; {
    
    char full_file_name[1024];

    expand_path((char *)panel_get_value(item), full_file_name);
    strcpy(iced_file_name, full_file_name);
}

/************************************************************************/
/* iced_load_proc                                                            */
/************************************************************************/

/* ARGSUSED */
void
iced_load_proc(item)
Panel_item item;
{
   if (!iced_change_directory()) {
	iced_panelmsg("Unable to", "change to directory.");
   } else {
	if (special_characters())
		iced_browse_proc();
	else
		do_load();
   }

}

static
special_characters()
{
   char *filename;

   filename = panel_get_value(iced_fname_item);
   return (index(filename, '*'));
}

/* ARGSUSED */
static
do_load(item) 
Panel_item item;
{

   int			 size;
   u_int		 op, mode, needs_clearing = FALSE;
   struct pixrect	*new_pr;
   char			 error_msg[IL_ERRORMSG_SIZE];

   get_file_name(iced_fname_item);
   if (!strlen(iced_file_name)) {
      iced_panelmsg("Please enter name of","file to load.");
      return;
   }
   new_pr = icon_load_mpr(iced_file_name, error_msg);
   if (!new_pr)  {
      new_pr = load_old_format(iced_file_name, error_msg);
      if (!new_pr)
	 return;
   }
   size = max(new_pr->pr_size.x, new_pr->pr_size.y);
   if (size <= 16) {
	  iced_canvas_pr = &iced_new_cursor_pr;
	  mode = CURSOR;
	  if (new_pr->pr_size.x < 16 || new_pr->pr_size.y < 16)
		  needs_clearing = TRUE;
   } else if (size <= 64)  {
	  iced_canvas_pr = &iced_icon_pr;
	  mode = ICONIC;
	  if (new_pr->pr_size.x < 64 || new_pr->pr_size.y < 64)
		  needs_clearing = TRUE;
   } else 
      iced_panelmsg("Unable to load image;","(side > 64 pixels).");

   op = val_to_op((int)panel_get_value(iced_load_op_item));
   backup();
   iced_dirty_ul_cell.x = 0;
   iced_dirty_ul_cell.y = 0;
   iced_dirty_dr_cell.x = iced_cell_count-1;
   iced_dirty_dr_cell.y = iced_cell_count-1;
   if (op == PIX_SRC && needs_clearing) {
      pr_rop(iced_canvas_pr, 0, 0, 
             iced_canvas_pr->pr_size.x, iced_canvas_pr->pr_size.y,
             PIX_CLR, 0, 0, 0);
   }
   pr_rop(iced_canvas_pr, 0, 0, new_pr->pr_size.x, new_pr->pr_size.y,
	  op, new_pr, 0, 0);
   pr_destroy(new_pr);
   iced_icon_canvas_is_clear = FALSE;
   iced_state = -1;
   iced_set_state(mode);
   iced_panelmsg("","Image loaded.");
}

static char *
read_save_stuff(file_name)
char	 *file_name;
{
	extern char		*calloc();
	char			*result;
	char			 error_msg[IL_ERRORMSG_SIZE];
	long			 read;
	icon_header_object	 icon_header;
	register FILE		*fd;

	result = NULL;
	fd = icon_open_header(file_name, error_msg, &icon_header);
	if (fd) {
		long	stop_plus_one;
		stop_plus_one = ftell(fd);
		fseek(fd, icon_header.last_param_pos, 0);
		result = calloc(stop_plus_one-icon_header.last_param_pos+1,
				sizeof(*result));
		read = fread(result, sizeof(*result),
				stop_plus_one-2-icon_header.last_param_pos, fd);
		if (read != (stop_plus_one-2-icon_header.last_param_pos))
			abort();
		fclose(fd);
	}
	return(result);
}

/************************************************************************/
/* store_proc                                                           */
/************************************************************************/

#define BITS_PER_BYTE	8
#define ITEMS_PER_LINE	8
#define	MPR_D(pr) ((struct mpr_data *) (LINT_CAST((pr)->pr_data)))

/* ARGSUSED */
static void
store_proc(item)
	Panel_item item;
{
	int x, y, w, h;
	int count, items, pad;
	register struct pixrect *pr;
	struct pixrect *tpr = 0;
	register short *data;
	register FILE *fd;
	register char *save_stuff = NULL;
	struct stat stat_buf;

	if (!iced_change_directory()) {
		iced_panelmsg("Unable to", "change to directory.");
		return;
	}

	if (iced_state == CURSOR)
		pr = &iced_new_cursor_pr;
	else
		pr = &iced_icon_pr;

	w = pr->pr_size.x;
	h = pr->pr_size.y;

	/* store trimmed */
	if (!item) {
		if (!(tpr = mem_create(w, h, pr->pr_depth))) {
			iced_panelmsg("Error:", "out of memory");
			return;
		}

		/* mush rows together */
		for (y = 0; y < h; y++)
			pr_rop(tpr, 0, 0, w, 1, PIX_SRC | PIX_DST,
				pr, 0, y);

		/* find right pixel */
		for (; w > 0; w--)
			if (pr_get(tpr, w - 1, 0))
				break;

		/* mush columns together */
		for (x = 0; x < w; x++)
			pr_rop(tpr, 0, 0, 1, h, PIX_SRC | PIX_DST,
				pr, x, 0);

		/* find bottom pixel */
		for (; h > 0; h--)
			if (pr_get(tpr, 0, h - 1))
				break;

		/* copy non-blank part */
		pr_rop(tpr, 0, 0, pr->pr_width, pr->pr_height,
			PIX_CLR, (Pixrect *) 0, 0, 0);
		pr_rop(tpr, 0, 0, w, h,
			PIX_SRC, pr, 0, 0);

		pr = tpr;
	}

	get_file_name(iced_fname_item);
	if (!strlen(iced_file_name)) {
		iced_panelmsg("Please enter name of", "file to store.");
		goto Done;
	}
	if (stat(iced_file_name, &stat_buf) == -1) {
		ctrl_s_pending = TRUE;
		if (errno != ENOENT) {
			iced_panelmsg("Error:", sys_errlist[errno]);
			goto Done;
		}
	}
	else {
		/* stat succeeded; file exists	 */
		if (store_invoked_from_keyboard) {
			if (!ctrl_s_pending) {
				iced_panelmsg("Confirm overwrite of",
					"existing file with ^S...");
				goto Done;
			}
		}
		else {
			ctrl_s_pending = FALSE;
			iced_panelmsg("Confirm overwrite",
				"of existing file...");
			confirm_mouse_proc();
			if (!confirm()) {
				iced_panelmsg("", "");
				canvas_mouse_proc();
				goto Done;
			}
			canvas_mouse_proc();
		}
		save_stuff = read_save_stuff(iced_file_name);
	}
	fd = fopen(iced_file_name, "w");
	if (fd == NULL) {
		iced_panelmsg("", "Can't write to file.");
		goto Done;
	}

	fprintf(fd,
"/* Format_version=1, Width=%d, Height=%d, Depth=%d, Valid_bits_per_item=%d\n",
		w, h, pr->pr_depth,
		BITS_PER_BYTE * sizeof (*data));

	if (save_stuff) {
		extern cfree();
		char *temp;
		int len;

		/* Massage save_stuff to make read-write idempotent. */
		if (*(temp = save_stuff) == '\n')
			temp++;
		len = strlen(temp);
		if (temp[len - 1] == ' ')
			temp[len - 1] = '\0';
		fprintf(fd, "%s", temp);
		cfree(save_stuff);
	}

	fprintf(fd, " */\n");

	/* convert width to 16 bit chunks */
	w = (w + 15) >> 4;
	count = h * w;
	items = 0;
	x = 0;

	data = MPR_D(pr)->md_image;
	pad =  (MPR_D(pr)->md_linebytes >> 1) - w;

	while (--count >= 0) {
		fprintf(fd, items == 0 ? "\t" : ",");
#ifdef i386
		bitflip16(data);
#endif
		fprintf(fd, "0x%04X", (unsigned short) *data++);

		if (++items == ITEMS_PER_LINE || count == 0) {
			items = 0;
			fprintf(fd, count ? ",\n" : "\n");
		}

		if (++x == w) {
			x = 0;
			data += pad;
		}
	}
	fflush(fd);

	iced_panelmsg("",
		ferror(fd) ? "File write error." : "File written.");

Done:
	if (fd != NULL)
		fclose(fd);

	if (tpr != NULL_PR)
		pr_destroy(tpr);
}

/**********************************************************************/
/* confirm                                                            */ 
/**********************************************************************/

static
confirm()
{
   struct fullscreen	*fsh;
   int			 fd;
   struct inputevent	 event;
   int			 result;

   fd = (int)window_get(iced_base_frame, WIN_FD);
   fsh = fullscreen_init(fd);
   window_set(iced_base_frame, 
	      WIN_CONSUME_PICK_EVENTS, WIN_NO_EVENTS, WIN_MOUSE_BUTTONS, 0,
	      WIN_CURSOR,              &iconedit_confirm_cursor, 
	      0);
   while (TRUE) {
      if (input_readevent(fd, &event) == -1)  {
	    perror("Cursor_confirm input failed");
	    abort();
      }
      switch (event_action(&event))  {
	 case MS_MIDDLE:
	 case MS_RIGHT:
	    result = FALSE; 
	    break;
	 case MS_LEFT:
	    result = TRUE; 
	    break;
	 default:
	    blink(fd); 
	    continue;
      }
      break;
   }

   fullscreen_destroy(fsh);
   return result;
}

static
blink(fd)
int	fd;
{
	register int	i = 10000;

	pr_rop(&iced_conf_pr, 0, 0, 16, 16, PIX_NOT(PIX_DST), 0, 0, 0);
	win_setcursor(fd, &iconedit_confirm_cursor);
	while (i--);
	pr_rop(&iced_conf_pr, 0, 0, 16, 16, PIX_NOT(PIX_DST), 0, 0, 0);
	win_setcursor(fd, &iconedit_confirm_cursor);
}

/* ARGSUSED */
static void
size_proc(item, val) 
Panel_item item;
int     val;
{
   iced_panelmsg("","");
   iced_set_state(val);
}

/* ARGSUSED */
static void
grid_proc(item, val)
Panel_item item;
int     val;
{
   iced_panelmsg("","");
   if (val)
      iced_paint_grid(PIX_SET);
   else {
      iced_paint_grid(PIX_CLR);
      iced_paint_border();
   }
}

/* ARGSUSED */
static void
mode_proc(item, val)
Panel_item item;
int   val;
{
   int fill_val;
   static int current_mode;

   iced_panelmsg("","");
   if (val != current_mode) {
      switch (val)  {
	 case MODE_POINT: 
	  canvasmsg("        Points: Pick points to paint or clear.");
	  window_set(iced_panel,PANEL_CARET_ITEM,iced_fname_item,0);
	  break;
	 case MODE_LINE: 
	  canvasmsg(" Line: Pick one endpoint, extend to opposite endpoint.");
	  window_set(iced_panel,PANEL_CARET_ITEM,iced_fname_item,0);
	  break;
	 case MODE_RECTANGLE: 
	  canvasmsg(" Rectangle: Pick one corner, extend to opposite corner.");
	  window_set(iced_panel,PANEL_CARET_ITEM,iced_fname_item,0);
	  break;
	 case MODE_CIRCLE: 
	  canvasmsg("       Circle: Pick center, extend to perimeter.");    
	  window_set(iced_panel,PANEL_CARET_ITEM,iced_fname_item,0);
	  break;
	 case MODE_TEXT: 
	  canvasmsg("           Text: Pick point to insert text.");       
	  window_set(iced_panel,PANEL_CARET_ITEM,iced_abc_item,0);
	  break;
      }
   } else {
      if (val == MODE_RECTANGLE) {
	 fill_val = (int)panel_get_value(iced_fill_square_item) + 1; 
	 fill_val = fill_val % FILL_PATTERN_COUNT;
	 panel_set(iced_fill_square_item, PANEL_VALUE, fill_val, 0);
	 fill_square_proc(iced_fill_square_item, fill_val);
      } else if (val == MODE_CIRCLE) {
	 fill_val = (int)panel_get_value(iced_fill_circle_item) + 1; 
	 fill_val = fill_val % FILL_PATTERN_COUNT;
	 panel_set(iced_fill_circle_item, PANEL_VALUE, fill_val, 0);
	 fill_circle_proc(iced_fill_circle_item,fill_val);
      } else if (val == MODE_TEXT) {
	 fill_val = (int)panel_get_value(iced_font_item) + 1; 
	 fill_val = fill_val % FONT_COUNT;
	 panel_set(iced_font_item, PANEL_VALUE, fill_val, 0);
	 font_proc(iced_font_item, fill_val);
      }
   }
   current_mode = val;
}

static void
invert_proc() {
   iced_panelmsg("","");
   backup();
   iced_dirty_ul_cell.x = 0;
   iced_dirty_ul_cell.y = 0;
   iced_dirty_dr_cell.x = iced_cell_count-1;
   iced_dirty_dr_cell.y = iced_cell_count-1;
   pr_rop(iced_canvas_pr,0,0,iced_cell_count,iced_cell_count,PIX_NOT(PIX_DST),0,0,0);
   iced_icon_canvas_is_clear = FALSE;
   iced_paint_canvas();
   iced_paint_proof_rect();
}

void
iced_clear_proc() {
   iced_panelmsg("","");
   backup();
   iced_dirty_ul_cell.x = 0;
   iced_dirty_ul_cell.y = 0;
   iced_dirty_dr_cell.x = iced_cell_count-1;
   iced_dirty_dr_cell.y = iced_cell_count-1;
   pr_rop(iced_canvas_pr, 0, 0, iced_cell_count, iced_cell_count, PIX_CLR, 0, 0, 0);
   pw_writebackground(iced_canvas_pw, 0, 0, BIG, BIG, PIX_CLR);
   if (panel_get_value(iced_grid_item)) iced_paint_grid(PIX_SET);
   iced_paint_border();
   iced_paint_proof_rect();
   if (iced_state == ICONIC)
      iced_icon_canvas_is_clear = TRUE;
}

static void
fill_canvas_proc() {
   int op,result;
   iced_panelmsg("","");
   if (!iced_fill_sq_pr)
      return;
   result = (int)panel_get_value(iced_fill_op_item);
   op     = val_to_op(result);
   backup();
   iced_dirty_ul_cell.x = 0;
   iced_dirty_ul_cell.y = 0;
   iced_dirty_dr_cell.x = iced_cell_count-1;
   iced_dirty_dr_cell.y = iced_cell_count-1;
   pr_replrop(iced_canvas_pr,0,0,iced_cell_count,iced_cell_count,op,
		iced_fill_sq_pr,0,0);
   if (iced_state == ICONIC) {
      if (iced_fill_sq_pr == iced_patch_prs[GR_WHITE])
	 iced_icon_canvas_is_clear = TRUE;
      else
	 iced_icon_canvas_is_clear = FALSE;
   }
   iced_paint_proof_rect();
   iced_paint_canvas();
}

/* ARGSUSED */
static void
fill_square_proc(item,val)
Panel_item item;
int   val;
{
   iced_panelmsg("","");
   switch (val) {
      case 0:
	 panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 2, &iced_box_pr, 0);
	 break;
      case 1:
	 panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 2, &iced_sq_0, 0);
	 break;
      case 2:
	 panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 2, &iced_sq_25, 0);
	 break;
      case 3:
	 panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 2, &iced_sq_root, 0);
	 break;
      case 4:
	 panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 2, &iced_sq_50, 0);
	 break;
      case 5:
	 panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 2, &iced_sq_75, 0);
	 break;
      case 6:
	 panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 2, &iced_square_black, 0);
	 break;
   }
   if (val) iced_fill_sq_pr = iced_patch_prs[val-1];
   else     iced_fill_sq_pr = (struct pixrect *) NULL;
}

/* ARGSUSED */
static void
fill_circle_proc(item, val)
Panel_item item;
int   val;
{
   iced_panelmsg("","");
   switch (val) {
      case 0:
	 panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 3, &iced_circle_pr, 0);
	 break;
      case 1:
	 panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 3, &iced_circle_0, 0);
	 break;
      case 2:
	 panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 3, &iced_circle_25, 0);
	 break;
      case 3:
	 panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 3, &iced_circle_root, 0);
	 break;
      case 4:
	 panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 3, &iced_circle_50, 0);
	 break;
      case 5:
	 panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 3, &iced_circle_75, 0);
	 break;
      case 6:
	 panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 3, &iced_circle_black, 0);
	 break;
   }
   if (val) iced_fill_ci_pr = iced_patch_prs[val-1];
   else     iced_fill_ci_pr = (struct pixrect *) NULL;
}

static Panel_setting 
abc_proc(item, event)
Panel_item item; 
struct inputevent *event; 
{
   int	action = event_action(event);
   
   if (action >= ' ' && action <= '~')
      iced_panelmsg("","");
   return fname_proc(item, event);
}

/* ARGSUSED */
static void
font_proc(item, val)
Panel_item item; 
int   val;
{
   int value_y;

   if ((Panel_item)window_get(iced_panel,PANEL_CARET_ITEM) != iced_abc_item) {
      window_set(iced_panel, PANEL_CARET_ITEM, iced_abc_item, 0);
      panel_set(iced_mode_item, PANEL_VALUE, MODE_TEXT, 0);
      if (!val)
	 val = FONT_COUNT + 1;
      panel_set(iced_font_item, PANEL_VALUE, --val, 0);
   } else {
      switch (val) {
	 case SCREEN_R_7:
	    iced_abc_font = iced_screenr7;
	    panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 4, &iced_screenr7_pr, 0);
	    value_y = 273;
	    break;
	 case SCREEN_R_11:
	    iced_abc_font = iced_screenr11;
	    panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 4, &iced_screenr11_pr, 0);
	    value_y = 271;
	    break;
	 case SCREEN_B_12:
	    iced_abc_font = iced_screenb12;
	    panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 4, &iced_screenb12_pr, 0);
	    value_y = 268;
	    break;
	 case SCREEN_R_12:
	    iced_abc_font = iced_screenr12;
	    panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 4, &iced_screenr12_pr, 0);
	    value_y = 268;
	    break;
	 case SCREEN_B_14:
	    iced_abc_font = iced_screenb14;
	    panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 4, &iced_screenb14_pr, 0);
	    value_y = 266;
	    break;
	 case SCREEN_R_14:
	    iced_abc_font = iced_screenr14;
	    panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 4, &iced_screenr14_pr, 0);
	    value_y = 266;
	    break;
	 case CMR_B_14:
	    iced_abc_font = iced_cmrb14;
	    panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 4, &iced_cmrb14_pr, 0);
	    value_y = 266;
	    break;
	 case CMR_R_14:
	    iced_abc_font = iced_cmrr14;
	    panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 4, &iced_cmrr14_pr, 0);
	    value_y = 266;
	    break;
	 case GALLANT_R_19:
	    iced_abc_font = iced_gallantr19;
	    panel_set(iced_mode_item, PANEL_CHOICE_IMAGE, 4, &iced_gallantr19_pr, 0);
	    value_y = 264;
	    break;
	 default:
	    break;
      }
      panel_set(iced_abc_item, 
		PANEL_VALUE_FONT,   iced_abc_font,
	        PANEL_VALUE_Y,      value_y,
	        0);
   }
}

/* ARGSUSED */
static void
proof_background_proc(item, val)
Panel_item item;
int   val;
{
   iced_panelmsg("","");
   iced_proof_pr = iced_patch_prs[val];
   iced_paint_proof();
}

/* ARGSUSED */
static void
proof_op_proc(item, val) 
Panel_item item;
int   val;
{
   iced_panelmsg("","");
   iced_new_cursor.cur_function = val_to_op(val);
   if (iced_state == CURSOR)
      window_set(iced_proof, WIN_CURSOR, &iced_new_cursor, 0);
   iced_paint_proof_rect();
}

static int 
val_to_op(val) int val; {
   switch(val) {
      case OP_SRC:
	 return(PIX_SRC);
      case OP_OR:
	 return(PIX_SRC | PIX_DST);
      case OP_XOR:
	 return(PIX_SRC ^ PIX_DST);
      case OP_AND:
	 return(PIX_SRC & PIX_DST);
   }
   /* NOTREACHED */
}

/* ARGSUSED */
static void
quit_proc(item) 
Panel_item item;
{
   int		result;
   Event	alert_event;

   if (quit_invoked_from_keyboard) {
      if (!ctrl_q_pending) {
	 iced_panelmsg("","Confirm quit with ^Q...");
	 return;
      }
   } else {
      ctrl_q_pending = FALSE;
      result = alert_prompt(
	  (Frame)iced_base_frame,
	  &alert_event,
	  ALERT_MESSAGE_STRINGS,
		"Are you sure you want to Quit?",
		0,
	  ALERT_BUTTON_YES,	"Confirm",
	  ALERT_BUTTON_NO,	"Cancel",
	  ALERT_OPTIONAL,	1,
	  0);
      if (result == ALERT_FAILED) {
	  iced_panelmsg("","Confirm quit...");
	  confirm_mouse_proc();
	  if (!confirm()) {
	     iced_panelmsg("","");
	     canvas_mouse_proc();
	     return;
	  }
      } else {
	  if (result == ALERT_YES) {
		result = 1;
	  } else result = 0;
      }
      if (!result) {
	  return;
      }
   }
   iced_panelmsg("","Quitting...");
   window_set(iced_base_frame, FRAME_NO_CONFIRM, TRUE, 0);
   window_destroy(iced_base_frame);
}

/**************************************************************************/
/* store button event proc                                                */
/**************************************************************************/

static void
store_button_event_proc(item, event)
Panel_item item;
Event *event;
{
   if (event_action(event) == MS_RIGHT && event_is_down(event)) {
      int result = (int)menu_show(store_menu, iced_panel, event, 0);
      switch (result) {
	 case 1: store_proc(item);
		 break;
	 case 2: store_proc(0);
		 break;
      }
   } else
      panel_default_handle_event(item, event);
}
