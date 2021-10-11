#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)button.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <suntool/tool_hs.h>
#include "button.h"

/*
 * Create a button and return it's handle. 
 */

button	*fted_button_create(down, move, up, character, reset, type, id, usr_data)
int	(*down)(), (*move)(), (*up)(), (*reset)(), (*character)();
				/* action routines for the button	*/
int	type;			/* type of the button	*/
int 	id;			/* id of button 	*/
caddr_t usr_data;		/* user's data		*/
{
    register	button	*new_button;
		char	*malloc();
		
    new_button = (button *) malloc(sizeof(button));
    if (new_button == (button *) NULL) {
	fprintf(stderr, "Egads! can't malloc a button. returning 0\n");
	return ((button *) NULL);
    }

    new_button->down = down;
    new_button->move = move;
    new_button->up   = up;
    new_button->character = character;
    new_button->reset = reset;
    new_button->type = type;
    new_button->id   = id;
    new_button->usr_data = usr_data;

    return(new_button);
}


/*
 * Set the location of the given button.
 */

fted_button_set_loc(but, left, top, width, height,
			origin_rect, text, text_x_offset, text_y_offset)
register button *but;		/* the button we're working with 	*/
int 	left, top, width, height;/* location and dimension of the button */
int	origin_rect;		/* to which rectangle the origin refers */
char	*text;			/* text in the button			*/
int	text_x_offset, text_y_offset;	/* where the text goes 		*/
{

    if (origin_rect == BUTTON_LOC_INNER) {
	BUT_CONST_INNER((*but), left, top, width, height);
    }
    else {
	BUT_CONST_OUTER((*but), left, top, width, height);
    }

    but->text = text;
    but->text_x = left + text_x_offset;
    but->text_y = top + text_y_offset;
}



fted_button_erase(but, pw)
	 button	*but;			/* button to be erased */
struct pixwin	*pw;			/* pix win of the button */
{
    register struct rect	*r;

    r = &(but->out_fted_line1);
    pw_writebackground(pw, r->r_top, r->r_left, r->r_width, 
	r->r_height, PIX_CLR);
}
