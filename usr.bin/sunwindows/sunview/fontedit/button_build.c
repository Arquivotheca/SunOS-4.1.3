#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)button_build.c 1.1 92/07/30 Copyr 1984 Sun Micro";
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
#define EDIT$MAIN
#include "edit.h"


/* Coordinates of upper-left corner of bounding box */
#define CHAR_BBOX_X			35
#define CHAR_BBOX_Y			10
/* offest from upper-left corner of bounding box */
/* where first button starts */
#define CHAR_FIRST_BUTTON_OFFSET_X	10
#define CHAR_FIRST_BUTTON_OFFSET_Y 	10
#define CHAR_BUTTON_X_SPACING		5
#define CHAR_MIN_WIDTH			50
#define CHAR_MIN_HEIGHT			55


/* Coordinates of upper-left corner of bounding box of scroll bar */
#define SCROLL_BBOX_X			90
#define SCROLL_BBOX_Y			15
/* offest from upper-left corner of bounding box */
/* where first button starts */
#define SCROLL_FIRST_BUTTON_OFFSET_X	10
#define SCROLL_FIRST_BUTTON_OFFSET_Y 	10
#define SCROLL_BUTTON_WIDTH		30
#define SCROLL_BUTTON_HEIGHT		25
#define SCROLL_BUTTON_X_SPACING		8
#define SCROLL_BUTTON_TEXT_X_OFFSET	5
#define SCROLL_BUTTON_TEXT_Y_OFFSET	15



/*
 * Action routines for the edit_buttons and for the scroll slider.
 */

extern int 			fted_edit_button_hi_light();
extern int			fted_edit_button_move();
extern int			fted_edit_button_picked();
extern int			fted_edit_button_un_hi_light();
extern int 			fted_draw_scroll_indicator();
extern int			fted_fted_scroll_slider_round();
extern int			fted_fted_scroll_slider_picked();
extern int 			fted_scroll_char_pressed();

/*
 * Action routines for the sliders on an edit pad. 
 */
extern int			fted_slider_cmd_right_edge();
extern int			fted_slider_cmd_bottom_edge();
extern int			fted_slider_cmd_top_edge();
extern int			fted_draw_indicator();
extern int			fted_draw_bounding_box();

/*
 * Action routines for edit pad buttons.
 */

extern int			fted_cmd_button_hi_light();
extern int			fted_cmd_button_un_hi_light();
extern int			fted_cmd_undo();
extern int			fted_cmd_save();
extern int			fted_cmd_quit();


extern int	  		fted_canvas_sig_handler();
struct	pixchar			*fted_copy_pix_char();

/*
 * Set up the fted_canvas_sub_win's handlers.
 * tio_selected gets called when an input event happens in this window.
 * handlesigwinch is called when the window is changed.		 
 * Next, describe the input events this window is interested in.	 
 *
 * The for-loop creates buttons and sliders and also assigns infomation that
 * does not change; their positions are assigned later on.
 */


fted_init_canvas()
{
	struct inputmask	mask;
	register int 		i;
	register button_group	*grp;
	register button		**buts;
	
	fted_canvas_pw = pw_open(fted_canvas_sub_win->ts_windowfd);
	fted_canvas_sub_win->ts_io.tio_selected = fted_canvas_selected;
	fted_canvas_sub_win->ts_io.tio_handlesigwinch = fted_canvas_sig_handler;
	fted_canvas_sub_win->ts_destroy = fted_nullproc; 
	input_imnull (&mask);
	win_setinputcodebit (&mask, MS_LEFT);
	win_setinputcodebit (&mask, MS_MIDDLE);
	win_setinputcodebit (&mask, MENU_BUT);
	win_setinputcodebit (&mask, LOC_MOVEWHILEBUTDOWN);
	win_setinputcodebit (&mask, LOC_WINEXIT);
	win_setinputcodebit (&mask, KEY_BOTTOMRIGHT);
 	mask.im_flags |= IM_ASCII;
	mask.im_flags |= IM_NEGEVENT;
	win_setinputmask(fted_canvas_sub_win->ts_windowfd, &mask,
				NULL, WIN_NULLLINK);
	fted_first_button_char = 'a';	/* start off with an 'a' */
	
	for(i = 0; i < NUM_CHAR_BUTTONS; i++) {
	    fted_char_buttons[i] = fted_button_create(	fted_edit_button_hi_light,
	    					fted_edit_button_move,
						fted_edit_button_picked,
						fted_nullproc,
						fted_edit_button_un_hi_light,
						BUTTON_TYPE_ACTION,
						i, (caddr_t) NULL);
	}
	fted_char_button_group.buttons = fted_char_buttons;
	fted_char_button_group.num_buttons = NUM_CHAR_BUTTONS;
	fted_add_button_group(&fted_char_button_group);

	fted_scroll_slider = fted_slider_create(fted_fted_scroll_slider_round,fted_fted_scroll_slider_picked,
				fted_draw_scroll_indicator, fted_scroll_char_pressed,
				fted_message_font, i, TRUE, NUM_CHAR_BUTTONS);

	for(i = 0; i < NUM_EDIT_PADS; i++) {
	    fted_edit_pad_info[i].open = fted_edit_pad_info[i].modified = FALSE;
	    
	    buts = (button **) malloc(NUM_COMMAND_BUTTONS * sizeof(button *));
	    CHECK_RETURN(buts, (button **), "allocating command buttons list");
	    grp  = (button_group *)  malloc(sizeof(button_group));
	    CHECK_RETURN(grp, (button_group *), "allocating command group");
	    buts[UNDO_BUTTON] = fted_button_create(	fted_cmd_button_hi_light,
	    					fted_cmd_button_hi_light,
						fted_cmd_undo,
						fted_nullproc,
						fted_cmd_button_un_hi_light,
						BUTTON_TYPE_ACTION,
						i, (caddr_t) NULL);
	    buts[SAVE_BUTTON] = fted_button_create(	fted_cmd_button_hi_light,
	    					fted_cmd_button_hi_light,
						fted_cmd_save,
						fted_nullproc,
						fted_cmd_button_un_hi_light,
						BUTTON_TYPE_ACTION,
						i, (caddr_t) NULL);
	    buts[QUIT_BUTTON] = fted_button_create(	fted_cmd_button_hi_light,
	    					fted_cmd_button_hi_light,
						fted_cmd_quit,
						fted_nullproc,
						fted_cmd_button_un_hi_light,
						BUTTON_TYPE_ACTION,
						i, (caddr_t) NULL);
	    grp->buttons = buts;
	    grp->num_buttons = 0 ;
	    fted_add_button_group(grp);
	    fted_edit_pad_groups[i].cmd = grp;
	    
            buts = (button **) malloc(NUM_EDIT_BUTTONS * sizeof(button *));
	    CHECK_RETURN(buts, (button **), "allocating edit buttons list");
	    grp  = (button_group *)  malloc(sizeof(button_group));
	    CHECK_RETURN(grp, (button_group *), "allocating edit group");
	    buts[EDIT_BUTTON] = fted_button_create(	(int *) NULL,
	    					(int *) NULL,
						(int *) NULL,
						fted_nullproc,
						fted_nullproc,
						BUTTON_TYPE_INPUT,
						i, (caddr_t) NULL);
	    grp->buttons = buts;
	    grp->num_buttons = 0 ;
	    fted_add_button_group(grp);
	    fted_edit_pad_groups[i].edit = grp;

	    fted_edit_pad_groups[i].sliders[TOP_SLIDER]      =
	    					fted_slider_create(fted_step_func_y,
							fted_nullproc /*fted_slider_cmd_top_edge*/ ,
	    						fted_nullproc /*fted_draw_indicator*/ ,
							fted_nullproc,
							fted_message_font,
							i, FALSE, 8);
	    fted_edit_pad_groups[i].sliders[BOTTOM_SLIDER]   =
	    					fted_slider_create(fted_step_func_y,
							fted_slider_cmd_bottom_edge,
	    						fted_draw_indicator,
							fted_nullproc,
							fted_message_font,
							i, FALSE, 8);
	    fted_edit_pad_groups[i].sliders[RIGHT_EDGE_SLIDER]=
	    					fted_slider_create(fted_step_func_x,
							fted_slider_cmd_right_edge,
	    						fted_draw_indicator,
							fted_nullproc,
							fted_message_font,
							i, TRUE,
							(SLIDER_WIDTH / 2));
	    fted_edit_pad_groups[i].sliders[ADV_SLIDER] 	 =
	    					fted_slider_create(fted_step_func_x,
							fted_slider_cmd_advance,
	    						fted_draw_indicator,
							fted_nullproc,
							fted_message_font, i,
							TRUE,
							(SLIDER_WIDTH / 2));
	}
}


/*
 * Assign the coordinates and sizes to the character buttons and to the scroll
 * slider. The size of the buttons is dependent upon the size 
 * of the characters being edited.
 * Then call fted_init_edit_pads with the y value which will be the top of the pads.
 */

fted_create_button_size()
{
	register int i;
	int 	char_button_inner_width,	/* size of inner rect */
		char_button_inner_height;
	int	button_x_offset;		/* x spacing between buttons */
	register int x, y;


	char_button_inner_width = ((fted_char_max_width > CHAR_MIN_WIDTH) ?
				    fted_char_max_width : CHAR_MIN_WIDTH) + 5;
	char_button_inner_height = (((char_button_inner_width - 20) >
							fted_char_max_height) ? 
						char_button_inner_width :
						(fted_char_max_height + 20)) + 5;
	
	button_x_offset = char_button_inner_width + (3*2) + CHAR_BUTTON_X_SPACING; 
		
	y = CHAR_BBOX_Y + CHAR_FIRST_BUTTON_OFFSET_Y ; 
	for(i=0; i< NUM_CHAR_BUTTONS; i++) {
	    x = CHAR_BBOX_X + CHAR_FIRST_BUTTON_OFFSET_X + (i * button_x_offset);
	    fted_button_set_loc( fted_char_buttons[i], x, y,
	    		char_button_inner_width, char_button_inner_height,
			BUTTON_LOC_INNER, (char *) NULL, 0, 0);
	    
	}
	y += char_button_inner_height + ( 3 * 2) + CHAR_FIRST_BUTTON_OFFSET_Y;
	fted_char_button_group.bounding_box = rect_bounding(fted_char_buttons[0],
					fted_char_buttons[NUM_CHAR_BUTTONS - 1]);
 	y += 4; 
	x = CHAR_FIRST_BUTTON_OFFSET_X + CHAR_BBOX_X;
	fted_slider_init(fted_scroll_slider, x, y,
		(2 * fted_num_chars_in_font),
		(SCROLL_HEIGHT - 4),
		(char *)NULL, 
		(x + 2 * fted_first_button_char), y);
	fted_scroll_slider->group->num_buttons = 1;

	fted_init_edit_pads( (y + 15) ); 
	
}



/*
 * Calculate the sizes and locations of the buttons that make up an edit pad.
 */



fted_init_edit_pads(y_start)
int	y_start;		/* where  to start the first pad */
{
    int			i;
    register int	cur_x, cur_y;		/* upper left corner of current rect */
    int			proof_width;
    int			proof_height;
    register button		**buttons;
    register button_group	*group;
    int			canvas_width;
    int			canvas_height;
    int			j;
    int			x_start;

    proof_width = ((fted_char_max_width > PROOF_WIDTH) ? fted_char_max_width : PROOF_WIDTH) + PROOF_X_OFFSET + 4;
    proof_height = ((proof_width > fted_char_max_height) ? proof_width : fted_char_max_height) + PROOF_Y_OFFSET+4;
    canvas_width = fted_char_max_width * CELL_SIZE;	
    canvas_height = fted_char_max_height * CELL_SIZE;
    y_start += SLIDER_LONGER_HEIGHT + 4;
    for (i = 0; i < NUM_EDIT_PADS; i++) {
	cur_y = y_start;
	cur_x = x_start = FIRST_WINDOW_X +
		i * (canvas_width + 2*SLIDER_LONGER_WIDTH +
			SCALE_WIDTH + 4 + WINDOW_X_OFFSET + COMMAND_WIDTH);

	BUT_CONST_OUTER(fted_edit_pad_info[i].left_height_scale, cur_x, cur_y + SCALE_HEIGHT - 2,
							SCALE_WIDTH, (canvas_height + 4));
	BUT_CONST_OUTER(fted_edit_pad_info[i].right_height_scale, (cur_x + SCALE_WIDTH + canvas_width),
							cur_y + SCALE_HEIGHT - 2,
							SCALE_WIDTH, (canvas_height + 4));

	BUT_CONST_OUTER(fted_edit_pad_info[i].top_width_scale, cur_x + SCALE_WIDTH - 2, cur_y,
						      (canvas_width + 4), SCALE_HEIGHT);
	BUT_CONST_OUTER(fted_edit_pad_info[i].bottom_width_scale, cur_x + SCALE_WIDTH - 2,
					cur_y + canvas_height + SCALE_HEIGHT,
						      (canvas_width + 4), SCALE_HEIGHT);

	cur_x += canvas_width + 4 + X_SPACE + SCALE_WIDTH + SLIDER_LONGER_WIDTH;
	BUT_CONST_INNER(fted_edit_pad_info[i].proof, cur_x, cur_y, proof_width, proof_height);

	/* command buttons */
	group = fted_edit_pad_groups[i].cmd;
	buttons = group->buttons;

	cur_y += proof_height + 2 * Y_SPACE;
	group->bounding_box.r_left = cur_x;
	group->bounding_box.r_top  = cur_y;
	group->bounding_box.r_width = COMMAND_WIDTH + 2;
	group->bounding_box.r_height = COMMAND_HEIGHT * NUM_COMMAND_BUTTONS +
						Y_SPACE * (NUM_COMMAND_BUTTONS - 1);
	fted_button_set_loc(buttons[UNDO_BUTTON], cur_x, cur_y, COMMAND_WIDTH, COMMAND_HEIGHT,
						BUTTON_LOC_OUTER, "Undo",
						COMMAND_TEXT_X_OFFSET, COMMAND_TEXT_Y_OFFSET);
	cur_y += COMMAND_HEIGHT + Y_SPACE;
	fted_button_set_loc(buttons[SAVE_BUTTON], cur_x, cur_y, COMMAND_WIDTH, COMMAND_HEIGHT,
						BUTTON_LOC_OUTER, "Store",
						COMMAND_TEXT_X_OFFSET, COMMAND_TEXT_Y_OFFSET);
	cur_y += COMMAND_HEIGHT + Y_SPACE;
	fted_button_set_loc(buttons[QUIT_BUTTON], cur_x, cur_y, COMMAND_WIDTH, COMMAND_HEIGHT,
						BUTTON_LOC_OUTER, "Quit",
						COMMAND_TEXT_X_OFFSET, COMMAND_TEXT_Y_OFFSET);

	/* editing buttons */
	cur_x = x_start + SCALE_WIDTH - 2;
	cur_y = y_start + SCALE_HEIGHT - 2;
	group = fted_edit_pad_groups[i].edit; 
	buttons = group->buttons;

	group->bounding_box.r_left = cur_x;
	group->bounding_box.r_top  = cur_y;
	group->bounding_box.r_width = canvas_width + 4;
	group->bounding_box.r_height = canvas_height + 4;
	fted_button_set_loc(buttons[EDIT_BUTTON], cur_x, cur_y, (canvas_width + 4),
					(canvas_height + 4),
					BUTTON_LOC_OUTER, (char *) NULL, 0,0);
	fted_edit_pad_info[i].whole = rect_bounding(&(fted_edit_pad_info[i].proof.out_fted_line1),
					&(group->bounding_box));
	fted_edit_pad_info[i].whole = rect_bounding(&(fted_edit_pad_info[i].whole),
					&(fted_edit_pad_groups[i].cmd->bounding_box));
    }
}
    
	    
/*
 *  on a sigwinch, re-draw the buttons. 
 */

fted_canvas_sig_handler()
{
	struct rect size;
	
	pw_damaged (fted_canvas_pw);
	win_getsize(fted_canvas_sub_win->ts_windowfd,&size);
	pw_writebackground(fted_canvas_pw, 0, 0, size.r_width,
		size.r_height, PIX_CLR);
	fted_draw_edit_buttons();
	fted_draw_fted_scroll_slider();
	fted_draw_edit_pads();
	pw_donedamaged (fted_canvas_pw);
}

/*
 * Redisplay the canvas window -- this routine is usually called
 * when the user has changed a characteristic of the font that
 * affects every character (such as changing the max size).
 */

fted_redisplay()
{
	struct rect size;

	win_getsize(fted_canvas_sub_win->ts_windowfd,&size);
	pw_writebackground(fted_canvas_pw, 0, 0, size.r_width,
		size.r_height, PIX_CLR);
	fted_draw_edit_buttons();
	fted_draw_fted_scroll_slider();
	fted_draw_edit_pads();
}

