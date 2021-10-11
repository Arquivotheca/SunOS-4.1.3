#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)button_char.c 1.1 92/07/30 Copyr 1984 Sun Micro";
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
#include "edit.h"

#define CHAR_BUTTON_TEXT_X_OFFSET	5
#define CHAR_BUTTON_TEXT_Y_OFFSET	15

/*
 *  This file contains the routines to draw the buttons for editting a
 * specific character and the action routines that are called to handle 
 * hi-lighting and picking of the buttons.
 * 
 * draw_edit-buttons is called on a sigwinch or if the font changes
 * and it redraws all of the 
 * buttons.
 */

fted_draw_edit_buttons()
{
    register int    i;
    register int    char_num;	/* number of character to be displayed */

    for (i = 0, char_num = fted_first_button_char;
	    i < NUM_CHAR_BUTTONS;
	    i++, char_num++) {
	fted_draw_edit_button (i, char_num);
    }

}

/*
 * This routine does the actual drawing of a button. The old button 
 * area is erased. The the two outline rectangles are draw; if the 
 * character displayed in the buttton is being editted, then the 
 * button is hilighted.  Then contents of the button are draw.
 */

fted_draw_edit_button(num, char_num)
register int num;		/* number of button to draw */
register int char_num;		/* numerical value of character */
{
    char    buff[10];		/* holds text */
    register int    x, y, h, w;

    
    pw_writebackground(fted_canvas_pw, fted_char_buttons[num]->out_fted_line1.r_left, 
				fted_char_buttons[num]->out_fted_line1.r_top,
    				fted_char_buttons[num]->out_fted_line1.r_width, 
				fted_char_buttons[num]->out_fted_line1.r_height, PIX_CLR);
    fted_FTdraw_rect (fted_canvas_pw, &(fted_char_buttons[num]->out_fted_line1), FOREGROUND);
    fted_FTdraw_rect (fted_canvas_pw, &(fted_char_buttons[num]->out_fted_line2), FOREGROUND);
    if (fted_char_info[char_num].open)
	fted_FTdraw_rect (fted_canvas_pw, &(fted_char_buttons[num]->hi_light), HI_LIGHT_ON);
    else
	fted_FTdraw_rect (fted_canvas_pw, &(fted_char_buttons[num]->hi_light), HI_LIGHT_OFF);

    sprintf (buff, "%3x:%c", char_num, char_num);
    x = fted_char_buttons[num]->out_fted_line2.r_left + CHAR_BUTTON_TEXT_X_OFFSET;
    y = fted_char_buttons[num]->out_fted_line2.r_top + CHAR_BUTTON_TEXT_Y_OFFSET;
    w = fted_char_buttons[num]->out_fted_line2.r_width;
    h = fted_char_buttons[num]->out_fted_line2.r_height;
    pw_text (fted_canvas_pw, x, y, PIX_SRC, fted_message_font, buff);
    x -= CHAR_BUTTON_TEXT_X_OFFSET;
 /* draw dividing line */
    pw_vector (fted_canvas_pw, x, (y + 4), (x + w), (y + 4), PIX_SRC, FOREGROUND);
    pw_vector (fted_canvas_pw, x, (y + 5), (x + w), (y + 5), PIX_SRC, FOREGROUND);

    if ((fted_cur_font != NULL) && (fted_cur_font->pf_char[char_num].pc_pr != NULL))
      if(char_num == 0) /* if the zeroth character contains a glyph */
        {
          /* if pw_char is used for (char) 0, it prints a null */
          pw_rop(fted_canvas_pw,
                 x+CHAR_BUTTON_TEXT_X_OFFSET, y+7,
                 fted_cur_font->pf_char[char_num].pc_pr->pr_size.x,
                 fted_cur_font->pf_char[char_num].pc_pr->pr_size.y,
                 PIX_SRC,
                 fted_cur_font->pf_char[char_num].pc_pr,
                 0, 0);
        }
      else
        {
     	  pw_char (fted_canvas_pw, (x + 3), (y + 7 + fted_font_base_line),
		    PIX_SRC, fted_cur_font, (char) char_num);
	}
}

/*
 * These routines handle the hi-lighting of the buttons. 
 */

static	button	*edit_hi_light_button = NULL;		/* the currently hi-lit button */
static  int	b_num;					/* the character number of the button */

/*
 * This is called when a mouse button is press on this button.
 * If a pad for this button is not already open, hilight the button.
 */

fted_edit_button_hi_light(but_ptr, but_num, cur_x, cur_y)
button	*but_ptr;
int	but_num;
int	cur_x, cur_y;
{
    if (!fted_char_info[but_num + fted_first_button_char].open) {
	fted_FTdraw_rect (fted_canvas_pw, &(but_ptr->hi_light), HI_LIGHT_ON);
	edit_hi_light_button = but_ptr;
	b_num = but_num + fted_first_button_char;
    }
}
/*
 * This rouitne is called when the mouse is moved with a button down. If the
 * button is not hi-lighted, hi-light it.
 */

fted_edit_button_move(but_ptr, but_num, cur_x, cur_y)
button	*but_ptr;
int	but_num;
int	cur_x, cur_y;
{
    if ((but_ptr != edit_hi_light_button) && !fted_char_info[but_num + fted_first_button_char].open) {
	fted_FTdraw_rect (fted_canvas_pw, &(but_ptr->hi_light), HI_LIGHT_ON);
	edit_hi_light_button = but_ptr;
	b_num = but_num + fted_first_button_char;
    }
}

/*
 * When the mouse button is released, the user has indicated that s/he wants
 * to edit this character. Sooo, if the character is not being edited, open
 * a pad for it.
 */

fted_edit_button_picked(but_ptr, but_num, cur_x, cur_y)
button	*but_ptr;
int	but_num;
int	cur_x, cur_y;
{
    register int i;
    
    if ((but_ptr != edit_hi_light_button) && !fted_char_info[but_num + fted_first_button_char].open) {
	fted_FTdraw_rect (fted_canvas_pw, &(but_ptr->hi_light), HI_LIGHT_ON);
	edit_hi_light_button = but_ptr;
	b_num = but_num + fted_first_button_char;
    }
    if (!fted_char_info[but_num + fted_first_button_char].open) {
       if (fted_make_edit_pad (but_num + fted_first_button_char)) 
          fted_char_info[but_num + fted_first_button_char].open = TRUE;
       else
	  fted_FTdraw_rect (fted_canvas_pw, &(but_ptr->hi_light), HI_LIGHT_OFF);
    }
/**** For now, don't do anything if a pick occurs on a open button
    else {
	i = 0;
	while ( fted_edit_pad_info[i].char_num != (but_num + fted_first_button_char))
	    i++;
	fted_cmd_save(fted_edit_pad_groups[i].cmd->buttons[SAVE_BUTTON], -1, cur_x, cur_y);
	fted_cmd_quit(fted_edit_pad_groups[i].cmd->buttons[QUIT_BUTTON], -1, cur_x, cur_y);
    }
*****/
}
/*
 * This routine is called when the mouse is moved off a button with a button
 * pressed down. Unhi-light the current button.
 */

fted_edit_button_un_hi_light()
{
    if ( (edit_hi_light_button != NULL) && !fted_char_info[b_num].open) {
	fted_FTdraw_rect (fted_canvas_pw, &(edit_hi_light_button->hi_light), HI_LIGHT_OFF);
	edit_hi_light_button = NULL;
    }
}
