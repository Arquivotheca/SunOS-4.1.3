/*	@(#)slider.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "button.h"

typedef struct Slider {
    int		lower_bound;		/* range of the slider */
    int		upper_bound;
    int		(*round)();		/* func which rounds the ndc value 	*/
    PIXFONT	*font;			/* font to display the text string	*/
    button_group *group;		/* button group 			*/
    button	*button;		/* display info: text string, button outline */
    int 	offset;			/* how to offset the text location from the cursor */
    int		(*up_action)();		/* routine that gets called when button is released */
    int		is_x_axis;		/* true if the slider moves on the x axis	*/
    int		(*draw)();		/* routine that is called to do drawing for the slider */
    struct Slider *next;
} slider;

slider 	*fted_slider_create();
