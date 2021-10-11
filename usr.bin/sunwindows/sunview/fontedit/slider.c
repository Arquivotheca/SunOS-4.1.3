#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)slider.c 1.1 92/07/30 Copyr 1984 Sun Micro";
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

#define TEXT_X 		5
#define TEXT_Y		15

/*
 * A slider is a special type of button. A slider covers a range
 * of values and uses an "indicator" to indicate(!) some sub-section
 * of the range. The indicator can be slid over the range - thus
 * the name. 
 * 
 * The indicator is draw by a user supplied routine and it should
 * draw in XOR mode.
 * 
 */


int		fted_slider_down();
int		fted_slider_move();
int		fted_slider_up();
int		fted_slider_reset();

static slider	*slider_head = NULL;



slider * fted_slider_create( round, notify, draw, character,font, but_id, x_axis, offset)
int	(*round)();	/* rounding  function	*/
int 	(*notify)();	/* func that gets called when button is released */
int	(*draw)();	/* func to draw stuff for the slider button */
int 	(*character)(); /* func to handle input of a character */
PIXFONT *font;		/* font of text 	*/
int 	but_id;		/* button id 		*/
int	x_axis;		/* true if slider is on the x axis */
int	offset;		/* how far to offset the text from the cursor */
{
    slider	*new_slider;		/* the newly created slider */
    button	*new_button;
    button	**button_list;
    button_group *new_group;


    new_slider = (slider *) malloc(sizeof(slider));
    CHECK_RETURN(new_slider, (slider *), "Can't malloc slider");
    new_button = (button *) malloc(sizeof(button));
    CHECK_RETURN(new_button, (button *), "Can't malloc button in fted_slider_create");
    button_list = (button **) (malloc(sizeof(button *)));
    CHECK_RETURN(button_list, (button **), "Can't malloc button_list in fted_slider_create");
    new_group = (button_group *) malloc(sizeof(button_group));
    CHECK_RETURN(new_group, (button_group *), "Can't malloc button_group in fted_slider_create");

    new_slider->round	  = round;
    new_slider->up_action = notify;
    new_slider->draw	  = draw;
    new_slider->font      = font;
    new_slider->button    = new_button;
    new_slider->group     = new_group;
    new_slider->is_x_axis = x_axis;
    new_slider->offset	  = offset;

    *button_list = new_button;
    new_group->buttons = button_list;
    new_group->num_buttons = 0;
    fted_add_button_group(new_group);

    new_button->usr_data  = (caddr_t) new_slider;
    new_button->down = fted_slider_down;
    new_button->move = fted_slider_move;
    new_button->up   = fted_slider_up;
    new_button->character = character;
    new_button->id   = but_id;
    new_button->reset = fted_slider_reset;
    new_button->type = BUTTON_TYPE_ACTION;
    
    new_slider->next = slider_head;
    slider_head = new_slider;
    return(new_slider);
}



fted_slider_init(s, left, top, width, height,  text, init_x, init_y)
slider		*s;
int		left, top, width, height; /* rect that slider moves in */
char		*text;			/* text displayed 	*/
int		init_x, init_y;		/* init_position of slider */
{
    struct pr_size	size;		/* size of text string */

    s->button->text = text;
    if (text != NULL) {
	s->button->text_x =  s->font->pf_char[*text].pc_pr->pr_size.x >> 1;
        s->button->text_y = - s->font->pf_char[*text].pc_home.y;
	size = pf_textwidth(strlen(text), s->font, text);
        RECT_CONSTRUCT(s->button->out_fted_line1, init_x, init_y, size.x, size.y);
    }
    else {
	RECT_CONSTRUCT(s->button->out_fted_line1, init_x, init_y, 1, 1);
    }

    RECT_CONSTRUCT(s->button->out_fted_line2, left, top, width, height);

    s->group->bounding_box = s->button->out_fted_line2;
}


/*
 * Mouse button is pressed over  the button.
 */
static button	*but_to_reset = NULL;
static int	reset_left;
static int	reset_top;
fted_slider_down(but_ptr, but_num, cur_x, cur_y)
button	*but_ptr;			/* ptr to button that was picked */
int	but_num;			/* its number */
int	cur_x, cur_y;
{
    if (but_to_reset != but_ptr) {
	fted_slider_reset(0, 0);
	but_to_reset = but_ptr;
	reset_left = but_ptr->out_fted_line1.r_left;
	reset_top  = but_ptr->out_fted_line1.r_top;
    }
    fted_slider_move(but_ptr, but_num, cur_x, cur_y);
}


fted_slider_move(but_ptr, but_num, cur_x, cur_y)
register button	*but_ptr;			/* ptr to button that was picked */
	 int	but_num;			/* its number */
	 int	cur_x, cur_y;
{
    register struct rect *ol1, *ol2;		/* out_line rects 1 and 2 */
    register slider	 *s;
    register int	cur_coord;
    register int	ref_pt;

    if ( but_to_reset == NULL)
        fted_slider_down(but_ptr, but_num, cur_x, cur_y);

    s = (slider *) but_ptr->usr_data;
    ol1 = &(but_ptr->out_fted_line1);
    ol2 = &(but_ptr->out_fted_line2);
    (*s->draw)(s); 
    if (s->is_x_axis) {
	cur_coord = cur_x;
	ref_pt = ol1->r_left;
	if ( cur_coord <= ol2->r_left ) {
	   cur_coord = ol2->r_left;
	}
	else if ( cur_coord >= (ol2->r_left + ol2->r_width) ) {
	   cur_coord = (ol2->r_left + ol2->r_width);
	}
	ref_pt += cur_coord - (ref_pt + (s->offset >> 1));
	ol1->r_left = ref_pt;
    }
    else {
	cur_coord = cur_y;
	ref_pt = ol1->r_top;
	if ( cur_coord <= ol2->r_top ) {
	   cur_coord = ol2->r_top;
	}
	else if ( cur_coord >= (ol2->r_top + ol2->r_height) ) {
	   cur_coord = (ol2->r_top + ol2->r_height);
	}
	ref_pt += cur_coord - (ref_pt + (s->offset >> 1));
	ol1->r_top = ref_pt;
    }
    (*s->draw)(s);
}



fted_slider_up(but_ptr, but_num, cur_x, cur_y)
register button	*but_ptr;			/* ptr to button that was picked */
	 int	but_num;			/* its number */
	 int	cur_x, cur_y;
{
    register struct rect *r;
    register slider	 *s;
    		int	 new_x, new_y;


    if ( but_to_reset == NULL)
        return;
	
    r = &(but_ptr->out_fted_line1);
    s = (slider *) but_ptr->usr_data;
    (*s->draw)(s);

    if (but_ptr->text != NULL) {
    	pw_text(fted_canvas_pw, (but_ptr->text_x + reset_left),
			   (but_ptr->text_y + reset_top),
			   PIX_SRC, s->font, " ");
    }

    (*s->round)(s, cur_x, cur_y, &new_x, &new_y);
    if((*s->up_action)(s, new_x, new_y)) {
    	r->r_left = new_x;
    	r->r_top = new_y;
    }
    
    if (but_ptr->text != NULL) {
    	pw_text(fted_canvas_pw, (but_ptr->text_x + r->r_left),
			   (but_ptr->text_y + r->r_top),
			   PIX_SRC, s->font, but_ptr->text);
    }
    (*s->draw)(s);
    but_to_reset = NULL;
}

fted_slider_reset(x, y)
register int x,y;
{
    register button		*but_ptr;
    register slider		*s;
    register struct rect 	*r;
    
    if (but_to_reset == NULL)
        return;

    but_ptr = but_to_reset;			/* put pointer into a register */
    s = (slider *) but_ptr->usr_data;
	 
    (*s->draw)(s);
    r = &(but_ptr->out_fted_line1);
    r->r_left = reset_left;
    r->r_top  = reset_top;
    if ( but_ptr->text != NULL) {
    	pw_text(fted_canvas_pw, (but_ptr->text_x + r->r_left),
			   (but_ptr->text_y + r->r_top),
			   PIX_SRC, s->font, but_ptr->text);
    }
    (*s->draw)(s);
    but_to_reset = NULL;
}


fted_slider_display(slider_list)
slider	*slider_list[];
{
    register slider	*s;
    register button 	*but_ptr;
    register struct rect *r;
    register int 	i;

    for (i = 0; i < NUM_SLIDERS; i++) {
	s = slider_list[i];
	but_ptr = s->button;
        r = &(but_ptr->out_fted_line1);
	if (but_ptr->text)
    		pw_text(fted_canvas_pw, (but_ptr->text_x + r->r_left),
			   (but_ptr->text_y + r->r_top),
			   PIX_SRC, s->font, but_ptr->text);
	(*s->draw)(s);
    }
}


fted_slider_enable(id)
int	id;
{
    register slider *s;

    for(s = slider_head; s != NULL; s =  s->next)
	if ( s->button->id == id )
            s->group->num_buttons = 1;
}



fted_slider_disable(id)
int	id;
{
    register slider *s;

    for(s = slider_head; s != NULL; s =  s->next){
	if ( s->button->id == id )
            s->group->num_buttons = 0;
    }
}



fted_step_func_x(s, cur_x, cur_y, new_x, new_y)
register slider		*s;		/* the slider that's being moved */
	int		cur_x, cur_y;	/* current location 		*/
	int		*new_x, *new_y;	/* new locataion 		*/
	
{
    register int origin_x;
    register struct rect *ol1, *ol2;
    

    ol1 = &(s->button->out_fted_line1);
    ol2 = &(s->button->out_fted_line2);
    *new_y = ol1->r_top;
    origin_x = fted_edit_pad_groups[s->button->id].edit->buttons[EDIT_BUTTON]->hi_light.r_left + 1;
    if (cur_x <= origin_x)
       *new_x = origin_x;
    else {
	if (cur_x >= (ol2->r_left + ol2->r_width))
	     cur_x = (ol2->r_left + ol2->r_width);
	*new_x = ((int)(cur_x - origin_x) / CELL_SIZE) * CELL_SIZE + origin_x;
    }

    *new_x += ( (CELL_SIZE / 2) - (SLIDER_WIDTH / 2) );
}

fted_step_func_y(s, cur_x, cur_y, new_x, new_y)
register slider		*s;		/* the slider that's being moved */
	int		cur_x, cur_y;	/* current location 		*/
	int		*new_x, *new_y;	/* new locataion 		*/
	
{
    register int origin_y;
    register struct rect *ol1, *ol2;
    

    ol1 = &(s->button->out_fted_line1);
    ol2 = &(s->button->out_fted_line2);
    *new_x = ol1->r_left;
    origin_y = fted_edit_pad_groups[s->button->id].edit->buttons[EDIT_BUTTON]->hi_light.r_top + 1;
    if (cur_y <= origin_y)
       *new_y = origin_y;
    else {
	if (cur_y >= (ol2->r_top + ol2->r_height))
	     cur_y = (ol2->r_top + ol2->r_height);
	*new_y = ((int)(cur_y - origin_y) / CELL_SIZE) * CELL_SIZE + origin_y;
    }

    *new_y += ( (CELL_SIZE / 2) - (SLIDER_WIDTH / 2) );
}



fted_slider_cmd_advance(s, x, y)
slider	*s;
int 	x, y;
{
    register int new_advance;
    register edit_info 	*pad;

    pad = &(fted_edit_pad_info[s->button->id]);
    if (!fted_undo_push(pad))
	fprintf(stderr,"Looks like we're out of memory; couldn't push undo\n");

    x += (SLIDER_WIDTH / 2);
    new_advance = ((int)(x - s->button->out_fted_line2.r_left) / CELL_SIZE) + 1 ;

    pad->pix_char_ptr->pc_adv.x =  new_advance;
    pad->modified = TRUE;
    return(TRUE);
}

fted_slider_cmd_right_edge(s, x, y)
slider	*s;
int 	x, y;
{
    register int	new_edge;
    register edit_info	*pad;
    register button	*but;
	
    but = fted_edit_pad_groups[s->button->id].edit->buttons[EDIT_BUTTON];
    pad = &(fted_edit_pad_info[s->button->id]);
    if (!fted_undo_push(pad))
	fprintf(stderr,"Looks like we're out of memory; couldn't push undo\n");

    x += (SLIDER_WIDTH / 2);
    new_edge = ((int)(x - (but->hi_light.r_left + 1)) / CELL_SIZE) + 1;

    pad->window.r_width = new_edge;
    but->out_fted_line2.r_width = new_edge * CELL_SIZE;
    pr_rop(pad->pix_char_ptr->pc_pr,
    		new_edge, 0, fted_char_max_width, fted_char_max_height, PIX_CLR, NULL, 0, 0);
    pw_write(fted_canvas_pw, pad->proof.out_fted_line2.r_left + PROOF_X_OFFSET,
                pad->proof.out_fted_line2.r_top + PROOF_Y_OFFSET,
		fted_char_max_width, fted_char_max_height, PIX_SRC,
		pad->pix_char_ptr->pc_pr, 0, 0);
				
    pad->modified = TRUE;
    if (!fted_undoing)
    	fted_paint_pad(pad, but);
    return(TRUE);
}
fted_slider_cmd_bottom_edge(s, x, y)
slider	*s;
int 	x, y;
{
    register int new_edge;
    register edit_info	*pad;
    register button	*but;

    but = fted_edit_pad_groups[s->button->id].edit->buttons[EDIT_BUTTON];
    pad = &(fted_edit_pad_info[s->button->id]);
    if (!fted_undo_push(pad))
	fprintf(stderr,"Looks like we're out of memory; couldn't push undo\n");

    y += (SLIDER_WIDTH / 2);
    new_edge = ((int)(y - (but->hi_light.r_top + 1)) / CELL_SIZE) + 1;
    new_edge = new_edge - pad->window.r_top;
    if (new_edge <= 0) {
	fted_message_user("bottom edge can't be above the top edge.");
	return(FALSE);
    }

    pad->window.r_height = new_edge;
    but->out_fted_line2.r_height = new_edge * CELL_SIZE;
    pr_rop(pad->pix_char_ptr->pc_pr, 0, (pad->window.r_top + new_edge),
			fted_char_max_width, fted_char_max_height, PIX_CLR, NULL, 0, 0);
    pw_write(fted_canvas_pw, pad->proof.out_fted_line2.r_left + PROOF_X_OFFSET,
                    pad->proof.out_fted_line2.r_top + PROOF_Y_OFFSET,
		    fted_char_max_width, fted_char_max_height, PIX_SRC,
		    pad->pix_char_ptr->pc_pr, 0, 0);
				
    if (!fted_undoing)
    	fted_paint_pad(pad, but);
    pad->modified = TRUE;
    return(TRUE);
}


fted_slider_cmd_top_edge(s, x, y)
slider	*s;
int 	x, y;
{
    register int	new_top;
    register int	diff;
    register edit_info	*pad;
    register button	*but;
    
    y += (SLIDER_WIDTH / 2);
    but = fted_edit_pad_groups[s->button->id].edit->buttons[EDIT_BUTTON];
    pad = &(fted_edit_pad_info[s->button->id]);
    
    new_top 		= ((int)(y - (but->hi_light.r_top + 1)) / CELL_SIZE)/* + 1 */;
    diff 		= new_top - pad->window.r_top;
    if ((pad->window.r_height - diff) <= 0) {
	fted_message_user("top edge can't be below the bottom edge.");
	return(FALSE);
    }

    pad->window.r_top 	= new_top;
    but->out_fted_line2.r_top = (new_top * CELL_SIZE) + (but->hi_light.r_top + 1);
    
    but->out_fted_line2.r_height = pad->window.r_height * CELL_SIZE;
    if (rect_bottom(&(but->out_fted_line2)) < rect_bottom(&(but->hi_light)))
    	but->out_fted_line2.r_height = rect_bottom(&(but->hi_light)) - 1 - but->out_fted_line2.r_top;
	
    pad->pix_char_ptr->pc_home.y += diff;
    pw_write(fted_canvas_pw, pad->proof.out_fted_line2.r_left + PROOF_X_OFFSET,
                    pad->proof.out_fted_line2.r_top + PROOF_Y_OFFSET,
		    fted_char_max_width, fted_char_max_height, PIX_SRC,
		    pad->pix_char_ptr->pc_pr, 0, 0);
				
    if (!fted_undoing)
    	fted_paint_pad(pad, but);
    return(TRUE); 
}



fted_draw_indicator(s)
register slider 	*s;
{
    register int x1, y1, x2, y2;
    register button *b;
    
    b = s->button;
    if (s->is_x_axis) {
	x1 = b->out_fted_line1.r_left ;
	x2 =  x1 + SLIDER_WIDTH;
	if ( *(b->text) == 'A' )  {
	    y1 = b->out_fted_line1.r_top + b->out_fted_line1.r_height;
	    y2 = y1 - SLIDER_LONGER_HEIGHT;
	}
	else {
	    y1 = b->out_fted_line1.r_top ;
	    y2 = y1 + SLIDER_LONGER_HEIGHT;
	}
	pw_vector(fted_canvas_pw, x1, y1, x2, y1, PIX_XOR, FOREGROUND);
	pw_vector(fted_canvas_pw, x1, y1, x1, y2, PIX_XOR, FOREGROUND);
	pw_vector(fted_canvas_pw, x2, y1, x2, y2, PIX_XOR, FOREGROUND);
    }
    else {
	x1 = b->out_fted_line1.r_left ;
	if ( *(b->text) == 'B' ) {
	    x1 += b->out_fted_line1.r_width + b->text_x;
	    x2 = x1 - SLIDER_LONGER_WIDTH;
	}
	else
	    x2 = x1 + SLIDER_LONGER_WIDTH;
	y1 = b->out_fted_line1.r_top ;
	y2 = y1 + SLIDER_HEIGHT;
	pw_vector(fted_canvas_pw, x1, y1, x1, y2, PIX_XOR, FOREGROUND);
	pw_vector(fted_canvas_pw, x1, y1, x2, y1, PIX_XOR, FOREGROUND);
	pw_vector(fted_canvas_pw, x1, y2, x2, y2, PIX_XOR, FOREGROUND);
    }
  
}
	
	
   
fted_draw_bounding_box(s)
register slider 	*s;
{
    fted_FTdraw_rect(fted_canvas_pw, &(s->group->bounding_box), FOREGROUND);
}
	
	
