/*      @(#)iconedit.h 1.1 92/07/30 SMI      */

/**************************************************************************/
/*                        iconedit.h                                      */
/*             Copyright (c) 1986 by Sun Microsystems Inc.                */
/**************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <pixrect/pixrect_hs.h>
#include <sunwindow/window_hs.h>
#include <suntool/tool_struct.h>
#include <suntool/icon.h>
#include <suntool/window.h>
#include <suntool/frame.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/walkmenu.h>

/**************************************************************************/
/* typedefs                                                               */
/**************************************************************************/

typedef struct pr_pos   SCREEN_POS, CELL_POS;

/************************************************************************/
/* constants for drawing modes, patterns, and raster ops                */
/************************************************************************/
 
#define MODE_POINT              0
#define MODE_LINE               1
#define MODE_RECTANGLE          2
#define MODE_CIRCLE             3
#define MODE_TEXT               4
  
#define NONE                    0
#define GR_WHITE                0
#define GR_GRAY25               1
#define GR_ROOT_GRAY            2
#define GR_GRAY50               3
#define GR_GRAY75               4
#define GR_BLACK                5
#define IC_GRAYCOUNT            6
#define FILL_PATTERN_COUNT      7

#define OP_SRC                  0
#define OP_OR                   1
#define OP_XOR                  2
#define OP_AND                  3

/**************************************************************************/
/* misc #defines                                                          */
/**************************************************************************/

#define NULL_PR            ( (struct pixrect *) 0)

#define ICON_SIZE          7
#define CURSOR_SIZE        (ICON_SIZE * 4)
#define CURSOR             1
#define ICONIC             0
 
#define CANVAS_DISPLAY     (CURSOR_SIZE * 16)

#define BIG                (1<<14)

#define CANVAS_MARGIN      12
#define CANVAS_SIDE        (CANVAS_DISPLAY + 2 * CANVAS_MARGIN)

#define backup() pr_rop(&iced_undo_pr,0,0,iced_cell_count,iced_cell_count, \
			PIX_SRC,iced_canvas_pr,0,0)

/**************************************************************************/
/* font stuff                                                             */
/**************************************************************************/

#define SCREEN_R_7      0
#define SCREEN_R_11     1
#define SCREEN_R_12     2
#define SCREEN_B_12     3
#define SCREEN_R_14     4
#define SCREEN_B_14     5
#define CMR_R_14        6
#define CMR_B_14        7
#define GALLANT_R_19    8
#define FONT_COUNT      9
 
#define F_SCREEN_R_7    "/usr/lib/fonts/fixedwidthfonts/screen.r.7"
#define F_SCREEN_R_11   "/usr/lib/fonts/fixedwidthfonts/screen.r.11"
#define F_SCREEN_B_12   "/usr/lib/fonts/fixedwidthfonts/screen.b.12"
#define F_SCREEN_R_12   "/usr/lib/fonts/fixedwidthfonts/screen.r.12"
#define F_SCREEN_B_14   "/usr/lib/fonts/fixedwidthfonts/screen.b.14"
#define F_CMR_B_14      "/usr/lib/fonts/fixedwidthfonts/cmr.b.14"
#define F_CMR_R_14      "/usr/lib/fonts/fixedwidthfonts/cmr.r.14"
#define F_GALLANT_R_19  "/usr/lib/fonts/fixedwidthfonts/gallant.r.19"

#define STANDARD_FONT "/usr/lib/fonts/fixedwidthfonts/screen.r.14"
#define BOLD_FONT     "/usr/lib/fonts/fixedwidthfonts/screen.b.12"

extern struct pixfont *iced_screenr7,*iced_screenr11,*iced_screenb12,
		      *iced_screenr12,*iced_screenr14,*iced_screenb14,
		      *iced_cmrb14,*iced_cmrr14,*iced_gallantr19;

extern struct pixfont  *iced_font,*iced_bold_font;

/**************************************************************************/
/* pixrects                                                               */
/**************************************************************************/

extern struct pixrect iced_icon_pixrect;

extern struct pixrect iced_white_patch;
extern struct pixrect iced_gray17_patch;
extern struct pixrect iced_gray20_patch;
extern struct pixrect iced_gray25_patch;
extern struct pixrect iced_gray50_patch;
extern struct pixrect iced_gray75_patch;
extern struct pixrect iced_root_gray_patch;
extern struct pixrect iced_gray80_patch;
extern struct pixrect iced_gray83_patch;
extern struct pixrect iced_black_patch;

extern struct pixrect iced_conf_pr;
extern struct pixrect iced_mouse_left;
extern struct pixrect iced_mouse_middle;
extern struct pixrect iced_mouse_right;
extern struct pixrect iced_tri_right;
extern struct pixrect iced_tri_left;
extern struct pixrect iced_tri_down;
extern struct pixrect iced_tri_up;
extern struct pixrect iced_grid_pr;
extern struct pixrect iced_drawing_hand;
extern struct pixrect iced_points_pr;
extern struct pixrect iced_line_pr;
extern struct pixrect iced_box_pr;
extern struct pixrect iced_circle_pr;
extern struct pixrect iced_abc_pr;
extern struct pixrect iced_screenr7_pr;
extern struct pixrect iced_screenr11_pr;
extern struct pixrect iced_screenr12_pr;
extern struct pixrect iced_screenb12_pr;
extern struct pixrect iced_screenr14_pr;
extern struct pixrect iced_screenb14_pr;
extern struct pixrect iced_cmrr14_pr;
extern struct pixrect iced_cmrb14_pr;
extern struct pixrect iced_gallantr19_pr;
extern struct pixrect iced_circle_black;
extern struct pixrect iced_circle_75;
extern struct pixrect iced_circle_50;
extern struct pixrect iced_circle_root;
extern struct pixrect iced_circle_25;
extern struct pixrect iced_circle_0;
extern struct pixrect iced_sq_75;
extern struct pixrect iced_sq_50;
extern struct pixrect iced_sq_root;
extern struct pixrect iced_sq_25;
extern struct pixrect iced_sq_0;
extern struct pixrect iced_square_none;
extern struct pixrect iced_square_white;
extern struct pixrect iced_square_25;
extern struct pixrect iced_square_root;
extern struct pixrect iced_square_50;
extern struct pixrect iced_square_75;
extern struct pixrect iced_square_black;
extern struct pixrect iced_square_50;
extern struct pixrect iced_blank_pr;

extern struct pixrect *iced_canvas_pr;
extern struct pixrect iced_icon_pr;
extern struct pixrect iced_new_cursor_pr;
extern struct cursor  iced_new_cursor;
extern struct pixrect iced_undo_pr;

extern struct pixrect *iced_fill_sq_pr;
extern struct pixrect *iced_fill_ci_pr;
extern struct pixrect *iced_proof_pr;

struct pixrect *iced_patch_prs[];

/**************************************************************************/
/* windows                                                                */
/**************************************************************************/

extern Frame	iced_base_frame;
extern Panel	iced_mouse_panel;
extern Panel	iced_msg_panel;
extern Panel	iced_panel;
extern Canvas	iced_canvas;
extern Canvas	iced_proof;

/**************************************************************************/
/* panel items                                                            */
/**************************************************************************/

extern Panel_item iced_fname_item;
extern Panel_item iced_size_item;
extern Panel_item iced_grid_item;
extern Panel_item iced_fill_op_item;
extern Panel_item iced_fill_square_item;
extern Panel_item iced_fill_circle_item;
extern Panel_item iced_mode_item;
extern Panel_item iced_abc_item;

/**************************************************************************/
/* browsing stuff                                                         */
/**************************************************************************/

extern void	iced_init_browser(),
		iced_browse_proc();
extern int	iced_browser_filled;

/**************************************************************************/
/* misc                                                                   */
/**************************************************************************/

extern CELL_POS iced_dirty_ul_cell, iced_dirty_dr_cell;

extern int	iced_cell_count;

extern int	iced_mouse_panel_width,	iced_mouse_panel_height,
		iced_msg_panel_width,	iced_msg_panel_height,
		iced_panel_width,	iced_panel_height,
		iced_proof_width,       iced_proof_height;

extern struct pixwin *iced_canvas_pw;

extern int iced_icon_canvas_is_clear;

extern struct cursor iconedit_main_cursor;
extern struct cursor iconedit_hourglass_cursor;
extern struct cursor iconedit_confirm_cursor;

extern short iced_state;

extern char	iced_file_name[];

/**************************************************************************/
/* functions                                                              */
/**************************************************************************/

extern int iced_complete_filename();
extern int iced_paint_proof();
extern int iced_paint_proof_rect();

extern void iced_clear_proc();
extern void iced_load_proc();
extern void iced_paint_border();
extern void iced_paint_canvas();
extern void iced_paint_grid();
