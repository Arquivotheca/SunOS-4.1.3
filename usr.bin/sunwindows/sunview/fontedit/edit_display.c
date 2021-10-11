#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)edit_display.c 1.1 92/07/30 Copyr 1984 Sun Micro";
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
#include "patches.h"
struct pixchar *fted_copy_pix_char();

/*
 * If there is room for an edit pad, open one by assigning 
 * info about the character to the pad data structure.
 * Also, copy the pix char containing the representation of
 * the character rather than using a pointer to the original;
 * this allows us to differentiate between the representation of
 * the character that is being edited and the representation that
 * is still part of the font.
 */


fted_make_edit_pad(char_to_edit)
int	char_to_edit;
{
    register int    i,
                    found_it;
    register    edit_info * pad_info;
    struct rect win_size;

    if ((fted_char_max_width == 0) || (fted_char_max_height == 0)) {
	fted_message_user ("Can't edit characters with 0 maximum dimension.");
	return (FALSE);
    }

    if ((fted_char_max_width > MAX_FONT_CHAR_SIZE_WIDTH)||
    	(fted_char_max_height > MAX_FONT_CHAR_SIZE_HEIGHT)) {
	fted_message_user (
	"Can't edit any characters. Their max size is too large. Largest size is %dx%d.",
	 MAX_FONT_CHAR_SIZE_WIDTH, MAX_FONT_CHAR_SIZE_HEIGHT);
	return (FALSE);
    }

 /* look for an open edit window */
    for (i = 0, found_it = FALSE; ((!found_it) && (i < NUM_EDIT_PADS)); i++) {
	if (fted_edit_pad_info[i].open == FALSE) {
	    found_it = TRUE;
	}
    }

    --i;
    if (found_it) {
	pad_info = &(fted_edit_pad_info[i]);
	win_getsize(fted_canvas_sub_win->ts_windowfd, &win_size);
	if ((pad_info->whole.r_left + pad_info->whole.r_width) >=
		win_size.r_width) {
	     fted_message_user("A new pad would not be completely visible.");
	     return(FALSE);
	}
	pad_info->open = TRUE;
	pad_info->modified = FALSE;
	pad_info->char_num = char_to_edit;
	pad_info->undo_list = NULL;
	pad_info->pix_char_ptr = fted_copy_pix_char(NULL,
					&(fted_cur_font->pf_char[char_to_edit]),
					TRUE);
        pad_info->window.r_left = 0;
	if (fted_cur_font->pf_char[char_to_edit].pc_pr != NULL) {
	    pad_info->window.r_top = 0 ;
		/*fted_font_base_line + pad_info->pix_char_ptr->pc_home.y;*/
	    pad_info->window.r_width = fted_cur_font->pf_char[char_to_edit].pc_pr->pr_size.x;
	    pad_info->window.r_height = fted_cur_font->pf_char[char_to_edit].pc_pr->pr_size.y;
	}
	else {
	    pad_info->window.r_top = 0;
	    pad_info->window.r_width = pad_info->pix_char_ptr->pc_pr->pr_size.x;
	    pad_info->window.r_height = pad_info->pix_char_ptr->pc_pr->pr_size.y;
	    pad_info->pix_char_ptr->pc_home.x = 0;
	    pad_info->pix_char_ptr->pc_home.y = -fted_font_base_line;
    	    pad_info->pix_char_ptr->pc_adv.x = fted_char_max_width;
    	    pad_info->pix_char_ptr->pc_adv.y = 0;
	}


    /* make buttons pickable by raising the number of buttons in the group
       from zero to NUM... */
	fted_edit_pad_groups[i].edit->num_buttons = NUM_EDIT_BUTTONS;
	fted_edit_pad_groups[i].cmd->num_buttons = NUM_COMMAND_BUTTONS;
	fted_draw_edit_pad (i);
	return (TRUE);
    }
    else {
	fted_message_user ("No more edit pads are availible.");
	return (FALSE);
    }
}

/*
 * Copy a pix_char into a new open. The make_max flag is true when we are 
 * copying a pix_char  which is going to be edited; a pixrect is create the 
 * size of the largest character of the font and the cur_pix_char pixrect is placed
 * inside the new pixrect. 
 */

struct pixchar *
fted_copy_pix_char(window, cur_pix_char, make_max)
struct rect		*window;
struct pixchar		*cur_pix_char;
       int		make_max;		/* true => make a pixchar the size of the maximun
       						   	   character and fix the cur_pix_char in it */
						/* false => just copy the pixchar 	*/
{
    struct pixchar	*new_pix_char;
    register int	num_bytes_in_char;
    struct pr_size	size;

    new_pix_char = (struct pixchar *) malloc(sizeof(struct pixchar));
    if (new_pix_char == 0) {
	fprintf(stderr,"Out of memory in fted_copy_pix_char 1. bye.\n");
	exit(-1);
    }
    new_pix_char->pc_home = cur_pix_char->pc_home;

    if (make_max) {
	new_pix_char->pc_adv = cur_pix_char->pc_adv;
	size.x = fted_char_max_width;
	size.y = fted_char_max_height;
	new_pix_char->pc_pr = mem_create(size.x, size.y, 1);
	if (new_pix_char->pc_pr == 0) {
	    fprintf(stderr,"In fted_copy_pix_char, couldn't make large_size pixrect. bye.\n");
	    exit(-1);
   	}
        if (cur_pix_char->pc_pr != NULL)
	   pr_rop(new_pix_char->pc_pr, 0, 0, 
	   			/*(fted_font_left_edge + cur_pix_char->pc_home.x), */
				/*(fted_font_base_line + cur_pix_char->pc_home.y), */
				       cur_pix_char->pc_pr->pr_size.x,
				       cur_pix_char->pc_pr->pr_size.y,
				       PIX_SRC,
				       cur_pix_char->pc_pr, 0, 0);
    }
    else {
	size.x = window->r_width;
	size.y = window->r_height;
	new_pix_char->pc_adv = cur_pix_char->pc_adv;

	new_pix_char->pc_pr = mem_create(size.x, size.y, 1);
    	if (new_pix_char->pc_pr == 0) {
	   fprintf(stderr,"In fted_copy_pix_char, could make pixrect. bye.\n");
	   exit(-1);
        }
	if (cur_pix_char->pc_pr != NULL)
	   pr_rop(new_pix_char->pc_pr, 0, 0,
				       window->r_width, window->r_height,
				       PIX_SRC,
				       cur_pix_char->pc_pr,
				       window->r_left, window->r_top);
    }

    return( new_pix_char );   
}    

    

fted_draw_edit_pads()
{
    register int i;

    for(i=0; i < NUM_EDIT_PADS; i++)
       if (fted_edit_pad_info[i].open == TRUE)
          fted_draw_edit_pad(i);
}

/*
 * Display an edit pad. 
 * -> Get the coordinates of the out_fted_line2 rect which acts the orgin 
 *    for the rest of the calculations
 * -> initialize the sliders using the properties of the character being
 *    edited such as the X advance. 
 * -> Calculate the "whole" rect which is covers the whole edit pad.
 *    It is used to erase the pad.
 * -> Draw the buttons and sliders.
 */

fted_draw_edit_pad(pad_num)
register int	pad_num;	/* which pad to draw */
{
    register int    j;
    register button *b;
    register struct rect *window;
    int y, x, origin_x, origin_y;
    int x2,y2;
    char	buff[10];

    /* get size (or "window") of current character */
    b = fted_edit_pad_groups[pad_num].edit->buttons[EDIT_BUTTON];
    window  = &(fted_edit_pad_info[pad_num].window);
    origin_x = b->hi_light.r_left;
    origin_y = b->hi_light.r_top;
    /* determine size of the box which shows the extent of the character */
    x = origin_x + window->r_left * CELL_SIZE;
    y = origin_y + window->r_top * CELL_SIZE;
    b->out_fted_line2.r_left    = x + 1;
    b->out_fted_line2.r_top     = y + 1;
    b->out_fted_line2.r_width   = (window->r_width * CELL_SIZE); 
    b->out_fted_line2.r_height  = (window->r_height * CELL_SIZE);

    origin_x -= (1 + SLIDER_LONGER_WIDTH );
    y = b->out_fted_line2.r_top;
    y += ( (CELL_SIZE / 2) - (SLIDER_HEIGHT / 2) );
    fted_slider_init(fted_edit_pad_groups[pad_num].sliders[TOP_SLIDER],
    			origin_x, origin_y, SLIDER_LONGER_WIDTH,
			(b->hi_light.r_height - 2),
			NULL /*"T"*/ , origin_x, y);
    x = (origin_x + b->hi_light.r_width + SLIDER_LONGER_WIDTH );
    y = (b->out_fted_line2.r_top + b->out_fted_line2.r_height - CELL_SIZE);
    y += ( (CELL_SIZE / 2) - (SLIDER_HEIGHT / 2) );
    fted_slider_init(fted_edit_pad_groups[pad_num].sliders[BOTTOM_SLIDER],
    			x, origin_y, SLIDER_LONGER_WIDTH, 
			(b->hi_light.r_height - 2),
			"B", (x + SCALE_WIDTH + 4), y);

    origin_x += (SLIDER_LONGER_WIDTH );
    origin_y -= (1 + SLIDER_LONGER_HEIGHT );
    x = b->out_fted_line2.r_left + b->out_fted_line2.r_width - CELL_SIZE;
    x += ( (CELL_SIZE / 2) - (SLIDER_WIDTH / 2) );
    fted_slider_init(fted_edit_pad_groups[pad_num].sliders[RIGHT_EDGE_SLIDER],
    			origin_x, origin_y,
    			(b->hi_light.r_width - 1), SLIDER_LONGER_HEIGHT,
			"R", x, origin_y);
			
    origin_y =  b->out_fted_line1.r_top + b->out_fted_line1.r_height;
    x = origin_x+(fted_edit_pad_info[pad_num].pix_char_ptr->pc_adv.x - 1) * CELL_SIZE ;
    x += ( (CELL_SIZE / 2) - (SLIDER_WIDTH / 2) );
    fted_slider_init(fted_edit_pad_groups[pad_num].sliders[ADV_SLIDER],
    			origin_x, origin_y,
    			(b->hi_light.r_width - 1),
			SLIDER_LONGER_HEIGHT,
			"A", x, (origin_y + SLIDER_HEIGHT));
    fted_slider_enable(pad_num);
    fted_edit_pad_info[pad_num].whole = rect_bounding(&(fted_edit_pad_info[pad_num].whole),
	&(fted_edit_pad_groups[pad_num].sliders[ADV_SLIDER]->button->out_fted_line1));
    fted_edit_pad_info[pad_num].whole = rect_bounding(&(fted_edit_pad_info[pad_num].whole),
	&(fted_edit_pad_groups[pad_num].sliders[RIGHT_EDGE_SLIDER]->button->out_fted_line1));
    fted_edit_pad_info[pad_num].whole = rect_bounding(&(fted_edit_pad_info[pad_num].whole),
	&(fted_edit_pad_groups[pad_num].sliders[TOP_SLIDER]->button->out_fted_line1));
    fted_edit_pad_info[pad_num].whole.r_height++;
    fted_edit_pad_info[pad_num].whole.r_width++;
    
    
    /* draw horizontal scales */
    DRAW_BUTTON_STRUCT(fted_edit_pad_info[pad_num].top_width_scale,
    					FOREGROUND, FOREGROUND, FOREGROUND);
    DRAW_BUTTON_STRUCT(fted_edit_pad_info[pad_num].bottom_width_scale,
    					FOREGROUND, FOREGROUND, FOREGROUND);
    x = fted_edit_pad_info[pad_num].top_width_scale.out_fted_line2.r_left + 2;
    y = fted_edit_pad_info[pad_num].top_width_scale.out_fted_line2.r_top + SCALE_HEIGHT - 6;
    y2 = fted_edit_pad_info[pad_num].bottom_width_scale.out_fted_line2.r_top
    							+ SCALE_HEIGHT - 6;
    for ( j = 1; j <= fted_char_max_width; j++) {
	sprintf(buff,"%2d",j);
	pw_text(fted_canvas_pw, ( x + ((j - 1) * CELL_SIZE)), y, PIX_SRC,
							fted_tiny_font, buff);
	pw_text(fted_canvas_pw, ( x + ((j - 1) * CELL_SIZE)), y2, PIX_SRC,
							fted_tiny_font, buff);
    }

    /* draw verticle scales */
    DRAW_BUTTON_STRUCT(fted_edit_pad_info[pad_num].left_height_scale,
    					FOREGROUND, FOREGROUND, FOREGROUND);
    DRAW_BUTTON_STRUCT(fted_edit_pad_info[pad_num].right_height_scale,
    					FOREGROUND, FOREGROUND, FOREGROUND);
    x = fted_edit_pad_info[pad_num].left_height_scale.out_fted_line2.r_left + 2;
    x2 = fted_edit_pad_info[pad_num].right_height_scale.out_fted_line2.r_left + 2;
    y = fted_edit_pad_info[pad_num].left_height_scale.out_fted_line2.r_top - 4;
    for ( y2 = 1, j = -fted_edit_pad_info[pad_num].pix_char_ptr->pc_home.y;
    			y2 <= fted_char_max_height; y2++, j--) {
	if (j == fted_font_caps_height) {
    	    	strcpy(buff,"C ");
		pw_text(fted_canvas_pw, x, ( y + (y2 * CELL_SIZE)),
    			PIX_NOT(PIX_SRC), fted_tiny_font, buff);
        	pw_text(fted_canvas_pw, x2, ( y + (y2 * CELL_SIZE)),
			PIX_NOT(PIX_SRC), fted_tiny_font, buff);
    	}
	else if (j == fted_font_x_height) {
    	    	strcpy(buff,"X ");
		pw_text(fted_canvas_pw, x, ( y + (y2 * CELL_SIZE)),
    			PIX_NOT(PIX_SRC), fted_tiny_font, buff);
        	pw_text(fted_canvas_pw, x2, ( y + (y2 * CELL_SIZE)),
			PIX_NOT(PIX_SRC), fted_tiny_font, buff);
    	}
	else if (j == 1) {
    	    	strcpy(buff,"B ");
		pw_text(fted_canvas_pw, x, ( y + (y2 * CELL_SIZE)),
    			PIX_NOT(PIX_SRC), fted_tiny_font, buff);
        	pw_text(fted_canvas_pw, x2, ( y + (y2 * CELL_SIZE)),
			PIX_NOT(PIX_SRC), fted_tiny_font, buff);
    	}
	else {
		if (j == 0)
			j--;
		sprintf(buff,"%2d",(j < 0 ? -j : j));
		pw_text(fted_canvas_pw, x, ( y + (y2 * CELL_SIZE)),
				PIX_SRC, fted_tiny_font, buff);
		pw_text(fted_canvas_pw, x2, ( y + (y2 * CELL_SIZE)),
				PIX_SRC, fted_tiny_font, buff);
	}
    }


    {
	register button **b_ptr;
	
        for (j = 0, b_ptr = fted_edit_pad_groups[pad_num].cmd->buttons;
                    j < fted_edit_pad_groups[pad_num].cmd->num_buttons;
	            b_ptr++ , j++) {
            	pw_text(fted_canvas_pw, (*b_ptr)->text_x, (*b_ptr)->text_y, PIX_SRC,
					fted_message_font, (*b_ptr)->text);
		DRAW_BUTTON_PTR((*b_ptr), FOREGROUND,FOREGROUND,FOREGROUND);
    	}
    }
    fted_FTdraw_rect(fted_canvas_pw, &(fted_edit_pad_info[pad_num].proof.out_fted_line1), FOREGROUND);
    fted_FTdraw_rect(fted_canvas_pw, &(fted_edit_pad_info[pad_num].proof.hi_light), FOREGROUND);
    fted_FTdraw_rect(fted_canvas_pw, &(fted_edit_pad_info[pad_num].proof.out_fted_line2), FOREGROUND);
    pw_write(fted_canvas_pw,
    	fted_edit_pad_info[pad_num].proof.out_fted_line2.r_left + PROOF_X_OFFSET,
	fted_edit_pad_info[pad_num].proof.out_fted_line2.r_top + PROOF_Y_OFFSET,
	window->r_width, window->r_height, PIX_SRC,
	fted_edit_pad_info[pad_num].pix_char_ptr->pc_pr, window->r_left, window->r_top);
    
    fted_paint_pad(&(fted_edit_pad_info[pad_num]),
       	      fted_edit_pad_groups[pad_num].edit->buttons[EDIT_BUTTON]);
	      
    fted_slider_display(fted_edit_pad_groups[pad_num].sliders);
}


#define CANVAS_MARGIN   5

/*
 * Draw the canvas in the canvas window.
 *
 * The r rect encompasses the row of cells being draw for each row; ie.
 * it's top value changes with each row. It is use to lock the area of the
 * canvas that is being draw into.
 */
 
fted_paint_pad(pad_info, but)
edit_info		*pad_info;	/* the character to be displayed */
button			*but;
{
	register int	org_x, org_y;	/* coord of cell to draw        */
	register int 	x,y;
	struct rect	 r;		/* rect of area being draw into */
	register int 	cur_char_height;
	register int 	cur_char_width;
	struct pixrect	*pixrect_ptr;
	struct rect	*bounds;

	/* clear the canvas and draw the upper and left borders */
	pw_writebackground(fted_canvas_pw, (but->hi_light.r_left + 1),
				(but->hi_light.r_top + 1),
	                   	(but->hi_light.r_width - 2),
			   	(but->hi_light.r_height - 2), PIX_CLR);
	bounds = &(but->out_fted_line2);
	fted_FTdraw_rect(fted_canvas_pw, bounds, FOREGROUND);
	cur_char_width =  pad_info->window.r_left + pad_info->window.r_width;
 	cur_char_height = pad_info->window.r_top + pad_info->window.r_height;
	pixrect_ptr = pad_info->pix_char_ptr->pc_pr;
	org_x = r.r_left = bounds->r_left;
	org_y = r.r_top = bounds->r_top;
	r.r_width = cur_char_width *  CELL_SIZE;
	r.r_height = CELL_SIZE;
	/* foreach cell: if it is forground, paint it in. */
	for ( y = 0; y <  (bounds->r_height / CELL_SIZE); y++) {
		pw_lock(fted_canvas_pw, &r);
		for (x = 0; x < (bounds->r_width / CELL_SIZE); x++) {
			if (pr_get(pixrect_ptr, x, y))  {
				fted_paint_cell(org_x, org_y, x, y, 1);
 			}
		}
		pw_unlock(fted_canvas_pw);
		r.r_top += CELL_SIZE;		/* adjust the locked*/
		                                /* area for this row*/
	}
}


/*
 *
 * Set cell (x,y) to the given color. (dx,dy) is the upper-left-hand coord
 * of the cell in the coordinate system of the canvas window. 
 * The actual size of the area that is drawn is 2 pixels less cell size.
 */
 
fted_paint_cell(org_x, org_y, x, y, color)
int	org_x, org_y;
int	x, y, color;
{
	register int	dx, dy; 

	dx = org_x + CELL_SIZE*x + 1;
	dy = org_y + CELL_SIZE*y + 1;

	if (color == FOREGROUND)
	    pw_write(fted_canvas_pw, dx, dy, (CELL_SIZE -1), (CELL_SIZE -1),
	    		PIX_SRC, &fted_black_patch, 1, 1);
	else if (color == BACKGROUND)
	    pw_write(fted_canvas_pw, dx, dy, (CELL_SIZE -1), (CELL_SIZE -1),
	    		PIX_SRC, &white_patch, 1, 1);
	else if (color == HI_LIGHT)
	    pw_write(fted_canvas_pw, dx, dy, (CELL_SIZE -1), (CELL_SIZE -1),
	    		PIX_SRC, &fted_gray50_patch, 1, 1);
	else
	    pw_write(fted_canvas_pw, dx, dy, (CELL_SIZE -1), (CELL_SIZE -1),
	    		PIX_SRC, &fted_root_gray_patch, 1, 1);
}


