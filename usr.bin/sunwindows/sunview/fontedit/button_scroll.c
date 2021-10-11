#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)button_scroll.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "other_hs.h"
#include "fontedit.h"
#include "externs.h"
#include "button.h"
#include "slider.h"
#include "edit.h"
#include "patches.h"

/*
 * Draw the outline of the scroll slider box. Since there are 256 chars
 * in a vfont font, make the box 512 pixels wide. The characters are
 * placed in the box such that their left edge is 2 times their ASCII
 * value from the left side of the box.
 */


fted_draw_fted_scroll_slider()
{
    register struct rect	*r;
    register int		x, y;
    
    r = &(fted_scroll_slider->button->out_fted_line2);
    x = r->r_left;
    y = r->r_top + SCROLL_TEXT_Y_OFFSET;
    pw_writebackground(fted_canvas_pw, r->r_left, r->r_top, r->r_width, r->r_height, PIX_CLR);
    fted_FTdraw_rect(fted_canvas_pw, r, FOREGROUND);
    pw_char(fted_canvas_pw, (x +  2 * '0'), y, PIX_SRC, fted_message_font, '0');
    pw_char(fted_canvas_pw, (x +  2 * '9'), y, PIX_SRC, fted_message_font, '9');
    pw_char(fted_canvas_pw, (x +  2 * 'A'), y, PIX_SRC, fted_message_font, 'A');
    pw_char(fted_canvas_pw, (x +  2 * 'Z'), y, PIX_SRC, fted_message_font, 'Z');
    pw_char(fted_canvas_pw, (x +  2 * 'a'), y, PIX_SRC, fted_message_font, 'a');
    pw_char(fted_canvas_pw, (x +  2 * 'z'), y, PIX_SRC, fted_message_font, 'z');
    (*fted_scroll_slider->draw)(fted_scroll_slider);
}

/*
 * The indicator is a black rectangle that is XORed into the slider
 * box.
 */

fted_draw_scroll_indicator(s)
register slider	*s;
{
    register struct rect *r;

    r = &(s->button->out_fted_line1);

    pw_write(fted_canvas_pw, r->r_left, (r->r_top + SCROLL_INDICATOR_Y_OFFSET) ,
    			(2 * NUM_CHAR_BUTTONS),
    			(SCROLL_INDICATOR_HEIGHT + 4), PIX_XOR,
			&fted_black_patch, 0, 0);
}


/*
 * This routine takes the current x value of the mouse and rounds it 
 * to the nearest even pixel value.
 */

fted_fted_scroll_slider_round(s, cur_x, cur_y, new_x, new_y)
register slider		*s;		/* the slider that's being moved */
register int		cur_x;		/* current location 		*/
	int		cur_y;	
	int		*new_x, *new_y;	/* new locataion 		*/
	
{
    register int origin_x;
    register int limit;
    
    *new_y = s->button->out_fted_line1.r_top;
    origin_x = s->button->out_fted_line2.r_left;
    cur_x = ((cur_x - origin_x ) &  0xFFFFFFFE) + origin_x;
    				/* that's the same as ( (int) (x /2) ) * 2 */
    limit = s->button->out_fted_line2.r_left;
    if ( cur_x <= limit ) {
	*new_x = limit;
	return;
    }
    else {
	limit = ((s->button->out_fted_line2.r_left + s->button->out_fted_line2.r_width) -
				(2 * NUM_CHAR_BUTTONS));
	if ( cur_x > limit) {
	    *new_x = limit; 
	    return;
	}
	*new_x = cur_x;
    }
}



/*
 * When a mouse button is released in the scroll slider, determine
 * what range of characters are to be displayed in the edit buttons.
 * The x value has already be rounded by the above routine, so all
 * that is left to do is to show characters on the right end 
 * gracefully.
 */

fted_fted_scroll_slider_picked(s, x, y)
	slider *s;
	int	x, y;
{

    fted_first_button_char = (x  - s->button->out_fted_line2.r_left) >> 1;
    
 
    if (fted_first_button_char > (fted_num_chars_in_font - NUM_CHAR_BUTTONS))
	fted_first_button_char = fted_num_chars_in_font - NUM_CHAR_BUTTONS - 1;
    else
	if (fted_first_button_char < 0)
	    fted_first_button_char = 0;
    fted_draw_edit_buttons ();

    return(TRUE);

}
	

/*
 * When a character is pressed and the mouse is in lowest window,
 * this routine is called. We have to get the button associated
 * with the slider from the global variable "fted_scroll_slider" and 
 * then simulate the press of a mouse button by calling the 
 * slider routines.
 */

fted_scroll_char_pressed(ch)
int 	ch;		/* character typed at the kbd */
{
    register button *but;

    but = fted_scroll_slider->button;
    fted_slider_down(but, 0, 0, 0);
    fted_slider_up(but, 0, (2 * ch + but->out_fted_line2.r_left), but->out_fted_line2.r_top);
}
