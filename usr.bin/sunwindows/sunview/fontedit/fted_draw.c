#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)fted_draw.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <signal.h>
#include "other_hs.h"
#include "fontedit.h"
#include "externs.h"
#include "button.h"
#include "slider.h"
#include "patches.h"

#include "edit.h"
/*
 * This file contains the routines to do the drawing operations
 * in the edit pad. In general, most operations require 2 or more
 * steps (e.g. button down, and move to position a line and button
 * up to actually draw it) and information is saved during the 
 * transition from state to state in static, global variables.
 * 
 * It might be helpful to point out here that there are 3
 * coordinate systems used.
 * The first is the coordinate system of the subwindow with
 * the origin in the upper left hand corner of the subwindow.
 * The second is the coordinate system of the edit pad; it's 
 * origin is the upper left hand corner of the inner-most rect
 * of the edit_pad (i.e. the out_fted_line2 rect). The third system
 * that of the pixrect which holds the glyph of the character 
 * being edited. So, the cursor position is in the first system;
 * subtracting the the upper left hand corner of the out_fted_line2
 * rect from cursor position yields a coordinate in the second
 * system and dividing by the cell size yields a value in
 * the last system.
 * 
 * Most operations have 5 routines/states
 * xxx0: is the start state - it prints out an explanitory message
 * xxx1: what happens when the a mouse button is pressed down - init things
 * xxx2: what happens as the mouse is moved with the button down - feed back
 * xxx3: "    "       when the button is released - preform the action
 * xxx_reset: reset things 
 * 
 * See input.c and action_table.h 
 */


static int	hilight_displayed = FALSE;/* true if a hilight is displayed */

static int	old_org_x, old_org_y;
static int	old_cell_x, old_cell_y;	/* previously hilighted location */
static int	old_cell_color;		/* "          "         "'s color value */

static int	pt_wipe_down = FALSE;	/* true if button pressed down for */
					/* point wipe function  */
#define GET_ORG(X,Y)							\
    X = fted_edit_pad_groups[pad_num].edit->buttons[EDIT_BUTTON]->out_fted_line2.r_left;	\
    Y = fted_edit_pad_groups[pad_num].edit->buttons[EDIT_BUTTON]->out_fted_line2.r_top;  

#define GET_CELL(CELL_X, CELL_Y, X, Y, ORG_X, ORG_Y)			\
    CELL_X = (X - ORG_X) / CELL_SIZE;					\
    CELL_Y = (Y - ORG_Y) / CELL_SIZE;
    

/*
 * A hilighted cell is gray in color (rather than black or white).
 * fted_hilight_cell is the initializer routine and it saves the old
 * color of the cell by looking at the characters pixrect.
 * The next routine moves the hilight by restoring the previous
 * cell's value and hilighting the current cell.
 */

fted_hilight_cell(x, y, pad_num, mouse_button, shifted)
register int	x, y;			/* coord of point	*/
register int	pad_num;		/* index of button	*/
int		mouse_button;		/* which mouse button was used */
int		shifted;		/* true if the shift key is down */

{

    GET_ORG(old_org_x, old_org_y);
    GET_CELL(old_cell_x, old_cell_y, x, y, old_org_x, old_org_y);

    old_cell_color = pr_get(fted_edit_pad_info[pad_num].pix_char_ptr->pc_pr,
    					old_cell_x, old_cell_y);
    
    fted_paint_cell(old_org_x, old_org_y, old_cell_x, old_cell_y, HI_LIGHT);
    hilight_displayed = TRUE;

}

fted_move_hilight(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register int cur_org_x, cur_org_y;
    register int cur_cell_x, cur_cell_y;
    
    cur_org_x = fted_edit_pad_groups[pad_num].edit->buttons[EDIT_BUTTON]->out_fted_line2.r_left;
    cur_org_y = fted_edit_pad_groups[pad_num].edit->buttons[EDIT_BUTTON]->out_fted_line2.r_top;

    cur_cell_x = (x - cur_org_x) / CELL_SIZE;	
    cur_cell_y = (y - cur_org_y) / CELL_SIZE;

    if (!hilight_displayed)
        fted_hilight_cell(x, y, pad_num, mouse_button, shifted);
    if ((cur_cell_x != old_cell_x) || (cur_cell_y != old_cell_y) ){
	fted_paint_cell(old_org_x, old_org_y, old_cell_x, old_cell_y, old_cell_color); 
	old_cell_color = pr_get(fted_edit_pad_info[pad_num].pix_char_ptr->pc_pr, cur_cell_x, cur_cell_y);
	old_org_x = cur_org_x;
	old_org_y = cur_org_y;
	old_cell_x = cur_cell_x;
	old_cell_y = cur_cell_y;
	fted_paint_cell(cur_org_x, cur_org_y, cur_cell_x, cur_cell_y, HI_LIGHT);
    }
}


fted_reset_cell_hilight()
{
    if (hilight_displayed) {
	fted_paint_cell(old_org_x, old_org_y, old_cell_x, old_cell_y, old_cell_color);
	hilight_displayed = FALSE;
    }
}

/*
 * These routines draw a single point on button up. the two previous
 * routines handle what happens as the mouse is moved around with
 * button down.
 */


fted_point_single0(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    fted_message_user(
       "Left button draws, middle erases a single point; release the button to do the action.");
}

fted_point_single3(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register int cur_org_x, cur_org_y;
    register int cur_cell_x, cur_cell_y;
    register int color;

    fted_reset_cell_hilight();
    cur_org_x = fted_edit_pad_groups[pad_num].edit->buttons[EDIT_BUTTON]->out_fted_line2.r_left;
    cur_org_y = fted_edit_pad_groups[pad_num].edit->buttons[EDIT_BUTTON]->out_fted_line2.r_top;

    cur_cell_x = (x - cur_org_x) / CELL_SIZE;	/* calculate the coords of the point in the definition of the character */
    cur_cell_y = (y - cur_org_y) / CELL_SIZE;

    fted_undo_push(&(fted_edit_pad_info[pad_num]));

    if (mouse_button == MS_LEFT) 
	color = FOREGROUND;
    else  
	color = BACKGROUND;
	
    fted_paint_cell(cur_org_x, cur_org_y, cur_cell_x, cur_cell_y, color);
    pr_put(fted_edit_pad_info[pad_num].pix_char_ptr->pc_pr, cur_cell_x, cur_cell_y, color);
    pw_put(fted_canvas_pw, fted_edit_pad_info[pad_num].proof.out_fted_line2.r_left + cur_cell_x + PROOF_X_OFFSET,
                      fted_edit_pad_info[pad_num].proof.out_fted_line2.r_top + cur_cell_y + PROOF_Y_OFFSET, color);
    hilight_displayed = FALSE;
    fted_edit_pad_info[pad_num].modified = TRUE;

}


/*
 * Point wipe draws a point while a mouse button is depressed.
 */

fted_point_wipe0(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    fted_message_user("Press and hold button to wipe points.");
}

fted_point_wipe1(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register int cur_org_x, cur_org_y;
    register int cur_cell_x, cur_cell_y;
    register int color;
    
    GET_ORG(cur_org_x, cur_org_y);
    GET_CELL(cur_cell_x, cur_cell_y, x, y, cur_org_x, cur_org_y);

    if (mouse_button == MS_LEFT) 
	color = FOREGROUND;
    else  
	color = BACKGROUND;
    fted_undo_push(&(fted_edit_pad_info[pad_num]));
	
    fted_paint_cell(cur_org_x, cur_org_y, cur_cell_x, cur_cell_y, color);
    pr_put(fted_edit_pad_info[pad_num].pix_char_ptr->pc_pr, cur_cell_x, 
		cur_cell_y, color);
    pw_put(fted_canvas_pw, fted_edit_pad_info[pad_num].proof.out_fted_line2.r_left + 
			cur_cell_x + PROOF_X_OFFSET,
		fted_edit_pad_info[pad_num].proof.out_fted_line2.r_top + 
			cur_cell_y + PROOF_Y_OFFSET, color);
    fted_edit_pad_info[pad_num].modified = TRUE;
    pt_wipe_down = TRUE;
    old_cell_x = cur_cell_x;
    old_cell_y = cur_cell_y;
}

fted_point_wipe2(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register int cur_org_x, cur_org_y;
    register int cur_cell_x, cur_cell_y;
    register int color;
    
    GET_ORG(cur_org_x, cur_org_y);
    GET_CELL(cur_cell_x, cur_cell_y, x, y, cur_org_x, cur_org_y);

    if (!pt_wipe_down)
    	fted_point_wipe1(x, y, pad_num, mouse_button, shifted);

    if ((cur_cell_x == old_cell_x) && (cur_cell_y == old_cell_y))
	return;

    if (mouse_button == MS_LEFT) 
	color = FOREGROUND;
    else  
	color = BACKGROUND;

    fted_undo_push(&(fted_edit_pad_info[pad_num]));
	
    fted_paint_cell(cur_org_x, cur_org_y, cur_cell_x, cur_cell_y, color);
    pr_put(fted_edit_pad_info[pad_num].pix_char_ptr->pc_pr, cur_cell_x, 
		cur_cell_y, color);
    pw_put(fted_canvas_pw, fted_edit_pad_info[pad_num].proof.out_fted_line2.r_left + 
			cur_cell_x + PROOF_X_OFFSET,
		fted_edit_pad_info[pad_num].proof.out_fted_line2.r_top + 
			cur_cell_y + PROOF_Y_OFFSET, color);
    fted_edit_pad_info[pad_num].modified = TRUE;
    
    old_cell_x = cur_cell_x;
    old_cell_y = cur_cell_y;
}



fted_point_wipe3(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register int cur_org_x, cur_org_y;
    register int cur_cell_x, cur_cell_y;
    register int color;
    
    GET_ORG(cur_org_x, cur_org_y);
    GET_CELL(cur_cell_x, cur_cell_y, x, y, cur_org_x, cur_org_y);

    if (!pt_wipe_down)
    	fted_point_wipe1(x, y, pad_num, mouse_button, shifted);
    
    pt_wipe_down = FALSE;
    if ((cur_cell_x == old_cell_x) && (cur_cell_y == old_cell_y))
	return;

    if (mouse_button == MS_LEFT) 
	color = FOREGROUND;
    else  
	color = BACKGROUND;

    fted_undo_push(&(fted_edit_pad_info[pad_num]));
	
    fted_paint_cell(cur_org_x, cur_org_y, cur_cell_x, cur_cell_y, color);
    pr_put(fted_edit_pad_info[pad_num].pix_char_ptr->pc_pr, cur_cell_x, 
		cur_cell_y, color);
    pw_put(fted_canvas_pw, fted_edit_pad_info[pad_num].proof.out_fted_line2.r_left + 
			cur_cell_x + PROOF_X_OFFSET,
		fted_edit_pad_info[pad_num].proof.out_fted_line2.r_top + 
			cur_cell_y + PROOF_Y_OFFSET, color);
    fted_edit_pad_info[pad_num].modified = TRUE;
    
}


fted_point_wipe_reset(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */
{
    pt_wipe_down = FALSE;
}



/*
 * The operations rect, cut, copy, paste and, to an extent, line all use 
 * a rectangle that is drawn on the screen. These static variables
 * hold info about the rect (in the various coordinate systems).
 * 
 */

static int base_x_wc;	/* base corner of rectangle or endpt of line (in window coordinates)	*/
static int base_y_wc;
static int base_x_cc;	/* base corner of rectangle or endpt of line (in cell coordinates)	*/
static int base_y_cc;
static int end_x_wc;	/* end point of rectangle or endpt of line (in window coordinates)	*/
static int end_y_wc;
static int pad_org_x;	/* origin of pad (in window coordinates)				*/
static int pad_org_y;
static int button_pressed = FALSE; /* true if the mouse button has been pressed down		*/

#define DRAW_RECT_OUTLINE(x, y) 					\
   pw_vector(fted_canvas_pw, base_x_wc, base_y_wc, x,			\
   		base_y_wc, PIX_XOR, FOREGROUND);			\
   pw_vector(fted_canvas_pw, x, base_y_wc, x, y, PIX_XOR, FOREGROUND);	\
   pw_vector(fted_canvas_pw, x, y, base_x_wc, y, PIX_XOR, FOREGROUND);	\
   pw_vector(fted_canvas_pw, base_x_wc, y, base_x_wc,			\
   		base_y_wc, PIX_XOR, FOREGROUND);
		
#define DRAW_LINE(x,y)							\
   pw_vector(fted_canvas_pw, base_x_wc, base_y_wc, x, y, PIX_XOR, FOREGROUND);



fted_line0(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    fted_message_user("To draw a line, press down a mouse button and move the mouse.");
    fted_line_reset(x, y, pad_num, mouse_button, shifted);
}


fted_line1(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register int cur_cell_x, cur_cell_y;
    
    GET_ORG(pad_org_x, pad_org_y);
    GET_CELL(cur_cell_x, cur_cell_y, x, y, pad_org_x, pad_org_y);

    base_x_cc = cur_cell_x;
    base_y_cc = cur_cell_y;
    base_x_wc = end_x_wc = (cur_cell_x * CELL_SIZE + pad_org_x) + (CELL_SIZE / 2);
    base_y_wc = end_y_wc = (cur_cell_y * CELL_SIZE + pad_org_y) + (CELL_SIZE / 2);

    DRAW_LINE(end_x_wc, end_y_wc);
    fted_message_user("Now, move the mouse.");
    button_pressed = TRUE;
}



fted_line2(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register int cur_cell_x, cur_cell_y;

    if (!button_pressed)
       return;
    GET_CELL(cur_cell_x, cur_cell_y, x, y, pad_org_x, pad_org_y);
    DRAW_LINE(end_x_wc, end_y_wc);
    end_x_wc = (cur_cell_x * CELL_SIZE + pad_org_x) + (CELL_SIZE / 2);
    end_y_wc = (cur_cell_y * CELL_SIZE + pad_org_y) + (CELL_SIZE / 2);
    DRAW_LINE(end_x_wc, end_y_wc);

}

fted_line_reset(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */
{
    if (button_pressed) {
	DRAW_LINE(end_x_wc, end_y_wc);
	button_pressed = FALSE;
    }
}
    


fted_line3(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register int cur_cell_x, cur_cell_y;
    int  color;
    struct pixrect *pr;
	    
    if (!button_pressed)
       return;

    DRAW_LINE(end_x_wc, end_y_wc);
    
    GET_CELL(cur_cell_x, cur_cell_y, x, y, pad_org_x, pad_org_y);

    if (mouse_button == MS_LEFT)
	color = FOREGROUND;
    else
	color = BACKGROUND;
    
    fted_undo_push(&(fted_edit_pad_info[pad_num]));
    fted_draw_line_of_cells(pad_org_x, pad_org_y, cur_cell_x, cur_cell_y,
	                   			 base_x_cc, base_y_cc, color);
    pr = fted_edit_pad_info[pad_num].pix_char_ptr->pc_pr;
    pr_vector(pr,cur_cell_x, cur_cell_y, base_x_cc, base_y_cc, PIX_SRC, color);
    
    pw_write(fted_canvas_pw, fted_edit_pad_info[pad_num].proof.out_fted_line2.r_left + PROOF_X_OFFSET,
	                    fted_edit_pad_info[pad_num].proof.out_fted_line2.r_top + PROOF_Y_OFFSET,
			    fted_char_max_width, fted_char_max_height,
			    PIX_SRC, pr, 0, 0);

    fted_message_user("  ");
    button_pressed = FALSE;
    fted_edit_pad_info[pad_num].modified = TRUE;
}


fted_rect0(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    fted_message_user("To define a rectangle, press down a mouse button and move the mouse.");
    fted_rect_reset(x, y, pad_num, mouse_button, shifted);
}


fted_rect1(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register int cur_cell_x, cur_cell_y;
    
    GET_ORG(pad_org_x, pad_org_y);
    GET_CELL(cur_cell_x, cur_cell_y, x, y, pad_org_x, pad_org_y);

    base_x_cc = cur_cell_x;
    base_y_cc = cur_cell_y;
    base_x_wc = end_x_wc =
    				(cur_cell_x * CELL_SIZE + pad_org_x) + (CELL_SIZE / 2);
    base_y_wc = end_y_wc =
    				(cur_cell_y * CELL_SIZE + pad_org_y) + (CELL_SIZE / 2);

    DRAW_RECT_OUTLINE(end_x_wc, end_y_wc);
    fted_message_user("Now, move the mouse.");
    button_pressed = TRUE;
}



fted_rect2(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register int cur_cell_x, cur_cell_y;

    if (!button_pressed)
       return;
    GET_CELL(cur_cell_x, cur_cell_y, x, y, pad_org_x, pad_org_y);
    DRAW_RECT_OUTLINE(end_x_wc, end_y_wc);
    end_x_wc = (cur_cell_x * CELL_SIZE + pad_org_x) + (CELL_SIZE / 2);
    end_y_wc = (cur_cell_y * CELL_SIZE + pad_org_y) + (CELL_SIZE / 2);
    DRAW_RECT_OUTLINE(end_x_wc, end_y_wc);

}

fted_rect_reset(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */
{
    if (button_pressed) {
	DRAW_RECT_OUTLINE(end_x_wc, end_y_wc);
	button_pressed = FALSE;
    }
}
    


fted_rect3(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register int cur_cell_x, cur_cell_y;
    int  color;
    struct pixrect *pr;
	    
    if (!button_pressed)
       return;
    DRAW_RECT_OUTLINE(end_x_wc, end_y_wc);
    
    GET_CELL(cur_cell_x, cur_cell_y, x, y, pad_org_x, pad_org_y);

	if (mouse_button == MS_LEFT)
	   color = FOREGROUND;
	else
	   color = BACKGROUND;

    fted_undo_push(&(fted_edit_pad_info[pad_num]));
    fted_draw_line_of_cells(pad_org_x, pad_org_y, cur_cell_x, cur_cell_y,
                			 base_x_cc, cur_cell_y, color);
    fted_draw_line_of_cells(pad_org_x, pad_org_y, base_x_cc, cur_cell_y,
        	       			 base_x_cc, base_y_cc, color);
    fted_draw_line_of_cells(pad_org_x, pad_org_y, base_x_cc, base_y_cc,
					 cur_cell_x, base_y_cc, color);
    fted_draw_line_of_cells(pad_org_x, pad_org_y, cur_cell_x, base_y_cc,
	 				 cur_cell_x, cur_cell_y, color);
    pr = fted_edit_pad_info[pad_num].pix_char_ptr->pc_pr;
    pr_vector(pr, cur_cell_x, cur_cell_y, base_x_cc, cur_cell_y,
					PIX_SRC, color);
    pr_vector(pr, base_x_cc, cur_cell_y, base_x_cc, base_y_cc,
					PIX_SRC, color);
    pr_vector(pr, base_x_cc, base_y_cc,    cur_cell_x, base_y_cc,
					PIX_SRC, color);
    pr_vector(pr, cur_cell_x, base_y_cc, cur_cell_x,
					cur_cell_y, PIX_SRC, color);

    pw_write(fted_canvas_pw, fted_edit_pad_info[pad_num].proof.out_fted_line2.r_left +
					PROOF_X_OFFSET,
	                    fted_edit_pad_info[pad_num].proof.out_fted_line2.r_top +
			    		PROOF_Y_OFFSET,
			    fted_char_max_width, fted_char_max_height,
			    PIX_SRC, pr, 0, 0);

    fted_message_user("  ");
    button_pressed = FALSE;
    fted_edit_pad_info[pad_num].modified = TRUE;
}

/*
 * A pixrect is used as the buffer to hold data used by the cut,copy and
 * paste routines. fted_buffer_rect contains the location and size of the
 * buffer.
 * 
 * previous_shift is used in a hack to implement an accelerator for
 * the cut/copy - paste steps. If the shift key is held down while in
 * cut or copy mode, paste mode is entered. previous_shift is used to
 * tell if the shift key is pressed or left up during an operation. 
 */

static unsigned int buffer[((MAX_FONT_CHAR_SIZE_WIDTH / 32) + 1)*MAX_FONT_CHAR_SIZE_HEIGHT];
mpr_static(fted_buffer_pixrect, MAX_FONT_CHAR_SIZE_WIDTH, MAX_FONT_CHAR_SIZE_HEIGHT, 1,
			(short *)buffer);
struct pixrect	*fted_buffer_pr = &fted_buffer_pixrect;
struct rect 	fted_buffer_rect;
static int	buffer_empty = TRUE;

static int 	previous_shift;			/* the previous value of the shift key*/

fted_cut_copy1(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */
{
    previous_shift = shifted;

    if (shifted)
    	fted_paste1(x, y, pad_num, mouse_button, shifted);
    else
    	fted_rect1(x, y, pad_num, mouse_button, shifted);
}

fted_cut_copy2(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */
{
    if (previous_shift != shifted) {
	fted_message_user("Sorry, can't change the shift key in the middle of an operation.");
	fted_rect_reset(x, y, pad_num, mouse_button, shifted);
	return;
    }

    if (shifted)
    	fted_paste2(x, y, pad_num, mouse_button, shifted);
    else
    	fted_rect2(x, y, pad_num, mouse_button, shifted);
}





fted_cut0(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    fted_message_user("Cut a rectangular area from the character and place it in the buffer.");
    fted_rect_reset(x, y, pad_num, mouse_button, shifted);

}

fted_cut3(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register	int 		cur_cell_x, cur_cell_y;
    register	struct pixrect	*pr;

    if (previous_shift != shifted) {
	fted_message_user("Sorry, can't change the shift key in the middle of an operation.");
	fted_rect_reset(x, y, pad_num, mouse_button, shifted);
	return;
    }

    if (shifted) {
    	fted_paste3(x, y, pad_num, mouse_button, shifted);
	return;
    }
    
    if (!button_pressed)
       return;
       
    fted_undo_push(&(fted_edit_pad_info[pad_num]));
    DRAW_RECT_OUTLINE(end_x_wc, end_y_wc);
    GET_CELL(cur_cell_x, cur_cell_y, x, y, pad_org_x, pad_org_y);
    if ( base_x_cc < cur_cell_x) {
	fted_buffer_rect.r_left = base_x_cc;
	fted_buffer_rect.r_width = cur_cell_x - base_x_cc + 1;
    }
    else {
	fted_buffer_rect.r_left = cur_cell_x;
	fted_buffer_rect.r_width = base_x_cc -  cur_cell_x + 1;
    }
    if ( base_y_cc < cur_cell_y) {
	fted_buffer_rect.r_top = base_y_cc;
	fted_buffer_rect.r_height = cur_cell_y - base_y_cc + 1;
    }
    else {
	fted_buffer_rect.r_top = cur_cell_y;
	fted_buffer_rect.r_height = base_y_cc -  cur_cell_y + 1;
    }
    
    pr = fted_edit_pad_info[pad_num].pix_char_ptr->pc_pr;
    pr_rop(fted_buffer_pr, fted_buffer_rect.r_left, fted_buffer_rect.r_top, 
			fted_buffer_rect.r_width,
    			fted_buffer_rect.r_height, PIX_SRC, pr,
    			fted_buffer_rect.r_left, fted_buffer_rect.r_top);
    pr_rop(pr, fted_buffer_rect.r_left, fted_buffer_rect.r_top, fted_buffer_rect.r_width,
    			fted_buffer_rect.r_height, PIX_CLR, NULL,
    			fted_buffer_rect.r_left, fted_buffer_rect.r_top);
    pw_write(fted_canvas_pw, fted_edit_pad_info[pad_num].proof.out_fted_line2.r_left +
    				PROOF_X_OFFSET,
	                fted_edit_pad_info[pad_num].proof.out_fted_line2.r_top +
				PROOF_Y_OFFSET,
			    fted_char_max_width, fted_char_max_height,
			    PIX_SRC, pr, 0, 0);
    pw_writebackground(fted_canvas_pw,
    			(pad_org_x + (fted_buffer_rect.r_left * CELL_SIZE) + 1),
    			(pad_org_y + (fted_buffer_rect.r_top * CELL_SIZE) + 1),
			((fted_buffer_rect.r_width * CELL_SIZE) - 1),
			((fted_buffer_rect.r_height * CELL_SIZE) - 1), PIX_CLR);
    fted_message_user("Rect cut and placed in buffer.");
    buffer_empty = FALSE;
    button_pressed = FALSE;
    fted_edit_pad_info[pad_num].modified = TRUE;
}


fted_copy0(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    fted_message_user("Copy a rectangular area from the character and place it in the buffer.");
    fted_rect_reset(x, y, pad_num, mouse_button, shifted);

}


fted_copy3(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register	int 		cur_cell_x, cur_cell_y;
    register	struct pixrect	*pr;
	    

    if (previous_shift != shifted) {
	fted_message_user("Sorry, can't change the shift key in the middle of an operation.");
	fted_rect_reset(x, y, pad_num, mouse_button, shifted);
	return;
    }

    if (shifted) {
    	fted_paste3(x, y, pad_num, mouse_button, shifted);
	return;
    }

    if (!button_pressed)
       return;

    DRAW_RECT_OUTLINE(end_x_wc, end_y_wc);
    GET_CELL(cur_cell_x, cur_cell_y, x, y, pad_org_x, pad_org_y);
    if ( base_x_cc < cur_cell_x) {
	fted_buffer_rect.r_left = base_x_cc;
	fted_buffer_rect.r_width = cur_cell_x - base_x_cc + 1;
    }
    else {
	fted_buffer_rect.r_left = cur_cell_x;
	fted_buffer_rect.r_width = base_x_cc -  cur_cell_x + 1;
    }
    if ( base_y_cc < cur_cell_y) {
	fted_buffer_rect.r_top = base_y_cc;
	fted_buffer_rect.r_height = cur_cell_y - base_y_cc + 1;
    }
    else {
	fted_buffer_rect.r_top = cur_cell_y;
	fted_buffer_rect.r_height = base_y_cc -  cur_cell_y + 1;
    }
    
    pr = fted_edit_pad_info[pad_num].pix_char_ptr->pc_pr;
    pr_rop(fted_buffer_pr, fted_buffer_rect.r_left, fted_buffer_rect.r_top, fted_buffer_rect.r_width,
    			fted_buffer_rect.r_height, PIX_SRC, pr,
    			fted_buffer_rect.r_left, fted_buffer_rect.r_top);
    pw_write(fted_canvas_pw, fted_edit_pad_info[pad_num].proof.out_fted_line2.r_left+PROOF_X_OFFSET,
                    fted_edit_pad_info[pad_num].proof.out_fted_line2.r_top + PROOF_Y_OFFSET,
			    fted_char_max_width, fted_char_max_height,
			    PIX_SRC, pr, 0, 0);
    buffer_empty = FALSE;
    button_pressed = FALSE;
    fted_message_user("Rect copied into buffer.");
}


fted_paste0(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    fted_message_user("Paste the contents of the buffer into a character; press down the mouse button.");
    fted_paste_reset(x, y, pad_num, mouse_button, shifted);

}


fted_paste1(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register int cur_cell_x, cur_cell_y;

    if (buffer_empty) {
	fted_message_user("Nothing is in the buffer to be pasted!");
	return;
    }

    button_pressed = TRUE;
    GET_ORG(pad_org_x, pad_org_y);
    GET_CELL(cur_cell_x, cur_cell_y, x, y, pad_org_x, pad_org_y);

    end_x_wc = (cur_cell_x * CELL_SIZE + pad_org_x) + (CELL_SIZE / 2);
    base_x_wc = end_x_wc + (fted_buffer_rect.r_width - 1) * CELL_SIZE;
    end_y_wc = (cur_cell_y * CELL_SIZE + pad_org_y) + (CELL_SIZE / 2);
    base_y_wc = end_y_wc + (fted_buffer_rect.r_height - 1) * CELL_SIZE;
    DRAW_RECT_OUTLINE(end_x_wc, end_y_wc);
    fted_message_user("Now, move the mouse.");
}



fted_paste2(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register int cur_cell_x, cur_cell_y;

    if (buffer_empty) {
	fted_message_user("buffer is empty.");
	return;
    }
    if (!button_pressed)
       return;
    DRAW_RECT_OUTLINE(end_x_wc, end_y_wc);
    GET_CELL(cur_cell_x, cur_cell_y, x, y, pad_org_x, pad_org_y);

    end_x_wc = (cur_cell_x * CELL_SIZE + pad_org_x) + (CELL_SIZE / 2);
    base_x_wc = end_x_wc + (fted_buffer_rect.r_width - 1) * CELL_SIZE;
    end_y_wc = (cur_cell_y * CELL_SIZE + pad_org_y) + (CELL_SIZE / 2);
    base_y_wc = end_y_wc + (fted_buffer_rect.r_height - 1) * CELL_SIZE;
    DRAW_RECT_OUTLINE(end_x_wc, end_y_wc);

}


fted_paste3(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */

{
    register 	int 		cur_cell_x, cur_cell_y;
    register 	struct pixrect 	*pr;
    		int		diff, width, height;
		struct rect	*window;
    
    if (buffer_empty) {
	fted_message_user("buffer is empty.");
	return;
    }
    if (!button_pressed)
       return;
    button_pressed = FALSE;
    fted_undo_push(&(fted_edit_pad_info[pad_num]));
    DRAW_RECT_OUTLINE(end_x_wc, end_y_wc);
    GET_CELL(cur_cell_x, cur_cell_y, x, y, pad_org_x, pad_org_y);

    pr = fted_edit_pad_info[pad_num].pix_char_ptr->pc_pr;
    window = &(fted_edit_pad_info[pad_num].window);
    diff = (cur_cell_x + fted_buffer_rect.r_width) - (window->r_left + window->r_width);
    if (diff > 0)
        width = fted_buffer_rect.r_width - diff;
    else
        width = fted_buffer_rect.r_width;
    diff = (cur_cell_y + fted_buffer_rect.r_height) - (window->r_top + window->r_height);
    if (diff > 0)
        height = fted_buffer_rect.r_height - diff;
    else
        height = fted_buffer_rect.r_height;
	
    pr_rop(pr, cur_cell_x, cur_cell_y, width, height,
			PIX_SRC, fted_buffer_pr,
    			fted_buffer_rect.r_left, fted_buffer_rect.r_top);
    pw_write(fted_canvas_pw, fted_edit_pad_info[pad_num].proof.out_fted_line2.r_left + PROOF_X_OFFSET,
	                    fted_edit_pad_info[pad_num].proof.out_fted_line2.r_top + PROOF_Y_OFFSET,
			    fted_char_max_width, fted_char_max_height,
			    PIX_SRC, pr, 0, 0);
    fted_paint_pad(&(fted_edit_pad_info[pad_num]),
       	      fted_edit_pad_groups[pad_num].edit->buttons[EDIT_BUTTON]);

    fted_message_user("Rect pasted into character.");
    fted_edit_pad_info[pad_num].modified = TRUE;
}




fted_paste_reset(x, y, pad_num, mouse_button, shifted)
register int	x, y;				/* coord of point	*/
register int	pad_num;			/* index of button	*/
int		mouse_button;			/* which mouse button was used */
int		shifted;			/* true if the shift key is down */
{
    if (button_pressed) {
	DRAW_RECT_OUTLINE(end_x_wc, end_y_wc);
	button_pressed = FALSE;
    }
}


