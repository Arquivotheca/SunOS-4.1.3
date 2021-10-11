#ifndef lint
#ifdef sccs
static char  sccsid[] = "@(#)iconedit_mpr.c 1.1 92/07/30";
#endif
#endif

/**************************************************************************/
/*                        iconedit_mpr.c                                  */
/*             Copyright (c) 1986 by Sun Microsystems Inc.                */
/**************************************************************************/

#include <suntool/tool_hs.h>

/**************************************************************************/
/* cursors -- main, hourglass and confirm                                 */
/**************************************************************************/
 
static short    main_cursor_array[16]  =  {
        0x8000,0xC000,0xE000,0xF000,0xF800,0xFC00,0xFE00,0xF000,
        0xD800,0x9800,0x0C00,0x0C00,0x0600,0x0600,0x0300,0x0300
};
mpr_static(iced_main_cursor_pr, 16, 16, 1, main_cursor_array);
struct cursor    iconedit_main_cursor  =  
	{ 0, 0, PIX_SRC | PIX_DST, &iced_main_cursor_pr };
 

static short    hg_data[] = {
#include <images/hglass.cursor>
};
mpr_static(iced_hourglass_cursor_pr, 16, 16, 1, hg_data);
struct cursor    iconedit_hourglass_cursor = 
	{ 8, 8, PIX_SRC | PIX_DST, &iced_hourglass_cursor_pr };


static short cursor_data[] = {
#include <images/confirm.pr> 
};
mpr_static(iced_conf_pr, 16, 16, 1, cursor_data);
struct cursor	iconedit_confirm_cursor = { 8, 8, PIX_SRC, &iced_conf_pr };

/**************************************************************************/
/* 16 x 16 memory pixrects in white, grey and black patterns.             */
/**************************************************************************/

static unsigned short	white_data[16] = {
			0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000
		};
mpr_static(iced_white_patch, 16, 16, 1, white_data);

static unsigned short	gray17_data[16] = {
			0x8200, 0x2080, 0x0410, 0x1040,
			0x4100, 0x0820, 0x8200, 0x2080,
			0x0410, 0x1040, 0x4100, 0x0820,
			0x0000, 0x0000, 0x0000, 0x0000
		};
mpr_static(iced_gray17_patch, 12, 12, 1, gray17_data);

static unsigned short   gray20_data[16] = {
			0x8420, 0x2108, 0x0842, 0x4210,
			0x1084, 0x8420, 0x2108, 0x0842,
			0x4210, 0x1084, 0x8420, 0x2108,
			0x0842, 0x4210, 0x1084, 0x0000
		};
mpr_static(iced_gray20_patch, 15, 15, 1, gray20_data);

static unsigned short	gray25_data[16] = {
			0x8888, 0x2222, 0x4444, 0x1111,
			0x8888, 0x2222, 0x4444, 0x1111,
			0x8888, 0x2222, 0x4444, 0x1111,
			0x8888, 0x2222, 0x4444, 0x1111
		};
mpr_static(iced_gray25_patch, 16, 16, 1, gray25_data);

static unsigned short	gray50_data[16] = {
			0xAAAA, 0x5555, 0xAAAA, 0x5555,
			0xAAAA, 0x5555, 0xAAAA, 0x5555,
			0xAAAA, 0x5555, 0xAAAA, 0x5555,
			0xAAAA, 0x5555, 0xAAAA, 0x5555
		};
mpr_static(iced_gray50_patch, 16, 16, 1, gray50_data);


static unsigned short	gray75_data[16] = {
			0x7777, 0xDDDD, 0xBBBB, 0xEEEE,
			0x7777, 0xDDDD, 0xBBBB, 0xEEEE,
			0x7777, 0xDDDD, 0xBBBB, 0xEEEE,
			0x7777, 0xDDDD, 0xBBBB, 0xEEEE
		};
mpr_static(iced_gray75_patch, 16, 16, 1, gray75_data);


static unsigned short	root_gray_data[16] = {
			0x8888, 0x8888, 0x2222, 0x2222,
			0x8888, 0x8888, 0x2222, 0x2222,
			0x8888, 0x8888, 0x2222, 0x2222,
			0x8888, 0x8888, 0x2222, 0x2222
		};
mpr_static(iced_root_gray_patch, 16, 16, 1, root_gray_data);

static unsigned short	gray80_data[16] = {
			0x7BDE, 0xDEF6, 0xF7BC, 0xBDEE,
			0xEF7A, 0x7BDE, 0xDEF6, 0xF7BC,
			0xBDEE, 0xEF7A, 0x7BDE, 0xDEF6,
			0xF7BC, 0xBDEE, 0xEF7A, 0x0000
		};
mpr_static(iced_gray80_patch, 15, 15, 1, gray80_data);

static unsigned short	gray83_data[16] = {
			0x7DF0, 0xDF70, 0xFBE0, 0xEFB0,
			0xBEF0, 0xF7D0, 0x7DF0, 0xDF70,
			0xFBE0, 0xEFB0, 0xBEF0, 0xF7D0,
			0x0000, 0x0000, 0x0000, 0x0000
		};
mpr_static(iced_gray83_patch, 12, 12, 1, gray83_data);

static unsigned short	black_data[16] = {
			0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
			0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
			0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
			0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
		};
mpr_static(iced_black_patch, 16, 16, 1, black_data);

/**************************************************************************/
/* mouse symbols                                                          */
/**************************************************************************/

static short    dat_iced_mouse_left[] = {
#include <images/confirm_left.pr> 
};
mpr_static(iced_mouse_left, 16, 16, 1, dat_iced_mouse_left);

static short    dat_iced_mouse_middle[] = {
#include <images/confirm_middle.pr> 
};
mpr_static(iced_mouse_middle, 16, 16, 1, dat_iced_mouse_middle);

static short    dat_iced_mouse_right[] = {
#include <images/confirm_right.pr> 
};
mpr_static(iced_mouse_right, 16, 16, 1, dat_iced_mouse_right);

/**************************************************************************/
/* solid triangles pointing right and left                                */
/**************************************************************************/

static short    iced_tri_right_dat[] = {
#include <images/tri_right.pr> 
};
mpr_static(iced_tri_right, 7, 12, 1, iced_tri_right_dat);

static short    iced_tri_left_dat[] = {
#include <images/tri_left.pr> 
};
mpr_static(iced_tri_left, 16, 12, 1, iced_tri_left_dat);

static short    iced_tri_down_dat[] = {
#include <images/tri_down.pr> 
};
mpr_static(iced_tri_down, 16, 7, 1, iced_tri_down_dat);

static short    iced_tri_up_dat[] = {
#include <images/tri_up.pr> 
};
mpr_static(iced_tri_up, 16, 7, 1, iced_tri_up_dat);

/**************************************************************************/
/* box with grid                                                          */
/**************************************************************************/

static short    grid_data[] = {
#include <images/grid.pr> 
};
mpr_static(iced_grid_pr, 16, 16, 1, grid_data);

/**************************************************************************/
/* painting hand, point, line, box, circle                                */
/**************************************************************************/

static short    hand_dat[] = {
#include <images/painting_hand.pr> 
};
mpr_static(iced_drawing_hand, 48, 36, 1, hand_dat);

static short    points_data[] = {
#include <images/point.pr> 
};
mpr_static(iced_points_pr, 16, 16, 1, points_data);

static short    line_data[16] = {
#include <images/line.pr> 
};
mpr_static(iced_line_pr, 16, 16, 1, line_data);

static short    box_data[16] = {
#include <images/box.pr> 
};
mpr_static(iced_box_pr, 16, 16, 1, box_data);

static short    circle_data[16] = {
#include <images/circle.pr> 
};
mpr_static(iced_circle_pr, 16, 16, 1, circle_data);

static short    abc_data[16] = {
#include <images/abc.pr> 
};
mpr_static(iced_abc_pr, 16, 16, 1, abc_data);

static short    blank_data[] = {
#include <images/blank.pr> 
};
mpr_static(iced_blank_pr, 16, 16, 1, blank_data);

/**************************************************************************/
/* abc in different fonts                                                 */
/**************************************************************************/

static short    d1[] = {
#include <images/screenr7.pr> 
};
mpr_static(iced_screenr7_pr, 38, 18, 1, d1);

static short    d2[] = {
#include <images/screenr11.pr> 
};
mpr_static(iced_screenr11_pr, 38, 18, 1, d2);

static short    d3[] = {
#include <images/screenr12.pr>
};
mpr_static(iced_screenr12_pr, 38, 18, 1, d3);

static short    d4[] = {
#include <images/screenb12.pr>
};
mpr_static(iced_screenb12_pr, 38, 18, 1, d4);

static short    d5[] = {
#include <images/screenr14.pr>
};
mpr_static(iced_screenr14_pr, 38, 18, 1, d5);

static short    d6[] = {
#include <images/screenb14.pr>
};
mpr_static(iced_screenb14_pr, 38, 18, 1, d6);

static short    d7[] = {
#include <images/cmrr14.pr>
};
mpr_static(iced_cmrr14_pr, 38, 18, 1, d7);

static short    d8[] = {
#include <images/cmrb14.pr>
};
mpr_static(iced_cmrb14_pr, 38, 18, 1, d8);

static short    d9[] = {
#include <images/gallantr19.pr>
};
mpr_static(iced_gallantr19_pr, 38, 18, 1, d9);

/**************************************************************************/
/* fill patterns in circles with no outline                               */
/**************************************************************************/

static short    iced_circle_black_dat[] = {	
#include <images/circle_black.pr> 
};
mpr_static(iced_circle_black, 16, 16, 1, iced_circle_black_dat);

static short    iced_circle_75_dat[] = {	
#include <images/circle_75.pr> 
};
mpr_static(iced_circle_75, 16, 16, 1, iced_circle_75_dat);

static short    iced_circle_50_dat[] = {	
#include <images/circle_50.pr> 
};
mpr_static(iced_circle_50, 16, 16, 1, iced_circle_50_dat);

static short    iced_circle_root_dat[] = {	
#include <images/circle_root.pr> 
};
mpr_static(iced_circle_root, 16, 16, 1, iced_circle_root_dat);

static short    iced_circle_25_dat[] = {	
#include <images/circle_25.pr> 
};
mpr_static(iced_circle_25, 16, 16, 1, iced_circle_25_dat);

static short    iced_circle_0_dat[] = {	
#include <images/circle_0.pr> 
};
mpr_static(iced_circle_0, 32, 27, 1, iced_circle_0_dat);

/**************************************************************************/
/* fill patterns in squares with no outline                               */
/**************************************************************************/

static short    iced_sq_75_dat[] = {	
#include <images/square_75.pr> 
};
mpr_static(iced_sq_75, 16, 16, 1, iced_sq_75_dat);

static short    iced_sq_50_dat[] = {	
#include <images/square_50.pr> 
};
mpr_static(iced_sq_50, 16, 16, 1, iced_sq_50_dat);

static short    iced_sq_root_dat[] = {	
#include <images/square_root.pr> 
};
mpr_static(iced_sq_root, 16, 16, 1, iced_sq_root_dat);

static short    iced_sq_25_dat[] = {	
#include <images/square_25.pr> 
};
mpr_static(iced_sq_25, 16, 16, 1, iced_sq_25_dat);

static short    iced_sq_0_dat[] = {	
#include <images/square_0.pr> 
};
mpr_static(iced_sq_0, 32, 27, 1, iced_sq_0_dat);

/**************************************************************************/
/* fill patterns in square outline                                        */
/**************************************************************************/

static short    sq_none_data[16] = {
#include <images/none.cursor> 
};
mpr_static(iced_square_none, 16, 16, 1, sq_none_data);

static short    sq_white_data[] = {	
#include <images/white.pr> 
};
mpr_static(iced_square_white, 16, 16, 1, sq_white_data);

static short    sq_gr_25_data[] = {	
#include <images/gr_25.cursor> 
};
mpr_static(iced_square_25, 16, 16, 1, sq_gr_25_data);

static short    sq_gr_root_data[] = {	
#include <images/gr_root.cursor> 
};
mpr_static(iced_square_root, 16, 16, 1, sq_gr_root_data);

static short    sq_gr_50_data[] = {	
#include <images/gr_50.cursor> 
};
mpr_static(iced_square_50, 16, 16, 1, sq_gr_50_data);

static short    sq_gr_75_data[] = {	
#include <images/gr_75.cursor> 
};
mpr_static(iced_square_75, 16, 16, 1, sq_gr_75_data);

static short    sq_black_data[] = {	
#include <images/black.cursor> 
};
mpr_static(iced_square_black, 16, 16, 1, sq_black_data);

