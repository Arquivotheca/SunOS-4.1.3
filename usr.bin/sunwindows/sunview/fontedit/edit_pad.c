#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)edit_pad.c 1.1 92/07/30 Copyr 1984 Sun Micro";
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
#include <suntool/alert.h>
#include <suntool/frame.h>

struct pixchar	*fted_copy_pix_char();

static button	*cmd_hi_button = NULL;	/* button which is hi lighted */

/*
 * This file contains the routines for the commands on an edit pad
 * (undo, save, quit). The hilight routine is called when a mouse
 * button is pressed or  moved over a button. When the button is
 * released, the action is preformed. 
 */


fted_cmd_button_hi_light(but_ptr, but_num, cur_x, cur_y)
button	*but_ptr;			/* ptr to button that was picked */
int	but_num;			/* its number 			 */
int	cur_x, cur_y;
{
    if ( but_ptr != cmd_hi_button ) 
    	fted_cmd_button_un_hi_light();

    fted_FTdraw_rect(fted_canvas_pw, &(but_ptr->hi_light), HI_LIGHT_ON);
    cmd_hi_button = but_ptr;
}


fted_cmd_button_un_hi_light()
{
    if ( cmd_hi_button )
    	fted_FTdraw_rect(fted_canvas_pw, &(cmd_hi_button->hi_light), HI_LIGHT_OFF);
}


/*
 * There is one hack in undo! The "fted_undoing" variable is a global
 * flag that is set to tell other routines that an undo is taking
 * place and they should not do some action; for instance, the
 * fted_slider_up command would push the current pixchar onto the undo
 * stack! So this flag is needed to tell it not to.
 */


struct pixchar *fted_undo_pop();

fted_cmd_undo(but_ptr, but_num, cur_x, cur_y)
button	*but_ptr;			/* ptr to button that was picked */
int	but_num;			/* its number */
int	cur_x, cur_y;
{
    register int 	pad_num;
    register edit_info 	*pad;
    struct pixchar  	*new_pix_char_ptr;
    register slider 	*s;
    
    pad_num = but_ptr->id;
    pad = &(fted_edit_pad_info[pad_num]);
    if ((new_pix_char_ptr = fted_undo_pop(pad)) == NULL) {
	fted_message_user("Undone everything.");
	pad->modified = FALSE;
    }
    else {
	mem_destroy(pad->pix_char_ptr->pc_pr);
	pad->pix_char_ptr->pc_pr = new_pix_char_ptr->pc_pr;
	pad->pix_char_ptr->pc_home = new_pix_char_ptr->pc_home;
	pad->pix_char_ptr->pc_adv = new_pix_char_ptr->pc_adv;
	pw_write(fted_canvas_pw, pad->proof.out_fted_line2.r_left + PROOF_X_OFFSET,
	                    pad->proof.out_fted_line2.r_top + PROOF_Y_OFFSET,
			    fted_char_max_width, fted_char_max_height,
			    PIX_SRC, new_pix_char_ptr->pc_pr, 0, 0);
	fted_undoing = TRUE;
	s = fted_edit_pad_groups[pad_num].sliders[RIGHT_EDGE_SLIDER];
	fted_slider_down(s->button, pad_num,
		(s->button->out_fted_line2.r_left + pad->window.r_width * CELL_SIZE),
		s->button->out_fted_line2.r_top);
	fted_slider_up(s->button, pad_num,
		(s->button->out_fted_line2.r_left + pad->window.r_width * CELL_SIZE),
		s->button->out_fted_line2.r_top);
	
	s = fted_edit_pad_groups[pad_num].sliders[ADV_SLIDER];
	fted_slider_down(s->button, pad_num,
		(s->button->out_fted_line2.r_left + pad->pix_char_ptr->pc_adv.x * CELL_SIZE),
		s->button->out_fted_line2.r_top);
	fted_slider_up(s->button, pad_num,
		(s->button->out_fted_line2.r_left + pad->pix_char_ptr->pc_adv.x * CELL_SIZE),
		s->button->out_fted_line2.r_top);

	s = fted_edit_pad_groups[pad_num].sliders[BOTTOM_SLIDER];
	fted_slider_down(s->button, pad_num, s->button->out_fted_line2.r_left,
		(s->button->out_fted_line2.r_top + pad->window.r_height * CELL_SIZE));
	fted_slider_up(s->button, pad_num, s->button->out_fted_line2.r_left,
		(s->button->out_fted_line2.r_top + pad->window.r_height * CELL_SIZE));
		
	fted_undoing = FALSE;
	fted_paint_pad(pad, fted_edit_pad_groups[pad_num].edit->buttons[EDIT_BUTTON]);
	free(new_pix_char_ptr);
    }
    fted_FTdraw_rect(fted_canvas_pw, &(but_ptr->hi_light), HI_LIGHT_OFF);
}



fted_cmd_save(but_ptr, but_num, cur_x, cur_y)
button	*but_ptr;			/* ptr to button that was picked */
int	but_num;			/* its number */
int	cur_x, cur_y;
{
    register int char_num;
    register edit_info *pad;
    struct pixchar  *new_pix_char_ptr;
    
    pad = &(fted_edit_pad_info[but_ptr->id]);
    char_num = pad->char_num;
/*	FOLLOWING NO LONGER NEEDS TO BE INCREMENTALY FREED BECAUSE
	PIXRECT ALLOCATES EVERYTHING AT ONCE TO SAVE MEMORY
    mem_destroy(fted_cur_font->pf_char[char_num].pc_pr);
*/
    new_pix_char_ptr = fted_copy_pix_char(&(pad->window), pad->pix_char_ptr, FALSE);
    fted_cur_font->pf_char[char_num].pc_pr = new_pix_char_ptr->pc_pr;
    fted_cur_font->pf_char[char_num].pc_home = new_pix_char_ptr->pc_home;
    fted_cur_font->pf_char[char_num].pc_adv = new_pix_char_ptr->pc_adv;
    free(new_pix_char_ptr);
    pad->modified = FALSE;

    if ( (char_num >= fted_first_button_char) && ( char_num < (fted_first_button_char + NUM_CHAR_BUTTONS)))
    	fted_draw_edit_button((char_num - fted_first_button_char), char_num);
    fted_FTdraw_rect(fted_canvas_pw, &(but_ptr->hi_light), HI_LIGHT_OFF);
    fted_font_modified = TRUE;
}
    

    
    
fted_cmd_quit(but_ptr, but_num, cur_x, cur_y)
button	*but_ptr;			/* ptr to button that was picked */
int	but_num;			/* its number */
int	cur_x, cur_y;
{
    register int 	char_num;
    register edit_info 	*pad;
    struct pixchar  	*pix_char_ptr;
    
    pad = &(fted_edit_pad_info[but_ptr->id]);
    char_num = pad->char_num;

    if (pad->modified) {
	int	result;
	Event	alert_event;

	result = alert_prompt(
	    (Frame)0,
	    &alert_event,
	    ALERT_MESSAGE_STRINGS,
		"Quiting will discard edits.",
		"Please confirm.",
		0,
	    ALERT_BUTTON_YES,	"Confirm, discard edits",
	    ALERT_BUTTON_NO,	"Cancel",
	    0);
	if (result == ALERT_FAILED) {
	    fted_message_user(
"Pad has been modified. Press the left button to confirm QUIT.");
	    if (!cursor_confirm(fted_canvas_sub_win->ts_windowfd)) {
	        fted_message_user(" ");
	        fted_FTdraw_rect(
		    fted_canvas_pw, &(but_ptr->hi_light), HI_LIGHT_OFF);
    	        cmd_hi_button = NULL;
	        return;
	    }
	    fted_message_user(" ");
	} else if (result == ALERT_NO) {
		fted_FTdraw_rect(
		    fted_canvas_pw, &(but_ptr->hi_light), HI_LIGHT_OFF);
    	        cmd_hi_button = NULL;
	        return;
	    }
    }
    pix_char_ptr = pad->pix_char_ptr;
    mem_destroy(pix_char_ptr->pc_pr);
    free(pix_char_ptr);
    fted_undo_free_list(pad);
    pw_writebackground(fted_canvas_pw, pad->whole.r_left, pad->whole.r_top,
	                   pad->whole.r_width, pad->whole.r_height, PIX_CLR);
    fted_char_info[char_num].open = pad->open = FALSE;
    pad->modified = FALSE;
    fted_edit_pad_groups[but_ptr->id].cmd->num_buttons = 0;
    fted_edit_pad_groups[but_ptr->id].edit->num_buttons = 0;
    fted_slider_disable(but_ptr->id);

    if ( (char_num >= fted_first_button_char) && ( char_num < (fted_first_button_char + NUM_CHAR_BUTTONS)))
    	fted_draw_edit_button((char_num - fted_first_button_char), char_num);
    cmd_hi_button = NULL;
    
}
    
