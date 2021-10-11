/*	@(#)button.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Structures and macros used in manipulating buttons
 */

/*
 * Draw and maintain a set of buttons.
 * 
 * A button is made up of a number of boxes:
 * |          |        |          | 
 * | outfted_line1 | hilite | outfted_line2 | 
 * |          |        |          | 
 * and each box is one pixel wide. The 
 * diagram shows a cross-section of the left edge of a button. 
 * A button is made up of struct rect's. 
 *
 * An action  button has actions routines associated with the press of
 * a mouse button over it, the movement of the mouse over it when
 * a mouse button is pressed and the up transition of a mouse button.
 * The user supplies the action routines when the button is
 * created (i.e. a button struct is allocated in memory); these
 * routines take care of hilighting the button.
 * 
 * An input button acts as an input to a user state machine. At
 * this moment, there is one state machine for the whole program
 * (which is in input.c).
 * 
 * A group of buttons is an button group. A button group can be used
 * to improve pick correlation: a the pick point is first checked
 * to see if it is inside a button group; if it is, then it is
 * tested to see if it is inside a button.
 * 
 * A slider is a super-set of a button. It has a movable indicator
 * that deliniates a region. Data-structure-wise: sliders know
 * about buttons but buttons don't know about sliders. Buttons
 * have a field called "user_data" which is used by sliders as
 * a back pointer to the slider structure; this is rather unclean
 * and should be cleaned up.
 *
 */

#ifndef BUTTON_H_INCLUDED

#define BUTTON_H_INCLUDED

/* 0 = background color, 1 = foreground color */
#define HI_LIGHT_ON	0
#define HI_LIGHT_OFF	1

#define BUTTON_TYPE_ACTION	0
#define BUTTON_TYPE_INPUT	1

#define BUTTON_LOC_INNER	0
#define BUTTON_LOC_OUTER	1

/*
 * The button structure:
 */

typedef struct {
    struct rect	out_fted_line1;	/* outer box		*/
    struct rect	hi_light;	/* the hi-light box	*/
    struct rect	out_fted_line2;	/* the inner box	*/
    int		(*reset)();	/* func which is called to un-hilight a button */
    int		(*down)();	/* 		    "   when button is pressed */
    int		(*move)();	/* 		    "   when mouse is moved w/ button pressed */
    int		(*up)();	/*		    "   when button is released */
    int		(*character)(); /*		    "   when a character is pressed in buttton */
    int		type;		/* type of button - ACTION or input to state table */
    int		id;		/* id for the button... */
    char	*text;		/* text that is displayed in the button		*/
    int		text_x;		/* text location 	*/
    int		text_y;
    caddr_t     usr_data;	/* user's data area     */
} button;

/* a collection of buttons				*/
typedef struct {
    struct rect	bounding_box;	/* the bounding box of all the buttons */
    int		num_buttons;	/* the number of buttons */
    button	**buttons;	/* the buttons		*/
} button_group;



/*         Routines      	*/

button		*fted_button_create();	/* creates a button structure	*/
int		fted_button_set_loc();	/* set location of a  button	*/
int		fted_button_erase();		/* erases a button		*/


/* Useful macros to operate on a rect rather than a pointer to one */
#define PRINT_RECT(str,b) printf("%s\t(%3d, %3d, %3d, %3d)\n",str,b.r_left,b.r_top,b.r_width,b.r_height)

#define POINT_IN_RECT(r,x,y)  ( (x > r.r_left) && (x < (r.r_left + r.r_width)) &&	\
	     			(y > r.r_top) &&  (y < (r.r_top + r.r_height)))
	  

#define RECT_CONSTRUCT(r, x, y, w, h)  r.r_left = (x); r.r_top = (y); r.r_width = (w); r.r_height = (h);

#define BUT_CONST_INNER(b, x, y, w, h) 			\
	RECT_CONSTRUCT(b.out_fted_line2, x, y, w, h);		\
	RECT_CONSTRUCT(b.hi_light, (x-1), (y-1), (w+2), (h+2));	\
	RECT_CONSTRUCT(b.out_fted_line1, (x-2), (y-2), (w+4), (h+4));


#define BUT_CONST_OUTER(b, x, y, w, h) 			\
	RECT_CONSTRUCT(b.out_fted_line1, x, y, w, h);		\
	RECT_CONSTRUCT(b.hi_light, (x+1), (y+1), (w-2), (h-2));	\
	RECT_CONSTRUCT(b.out_fted_line2, (x+2), (y+2), (w-4), (h-4));


#define DRAW_BUTTON_PTR(b, x1, x2, x3) 						\
        fted_FTdraw_rect (fted_canvas_pw, &(b->out_fted_line1), x1);				\
	fted_FTdraw_rect (fted_canvas_pw, &(b->hi_light), x2);				\
	fted_FTdraw_rect (fted_canvas_pw, &(b->out_fted_line2), x3);


#define DRAW_BUTTON_STRUCT(b, x1, x2, x3) 					\
        fted_FTdraw_rect (fted_canvas_pw, &(b.out_fted_line1), x1);				\
	fted_FTdraw_rect (fted_canvas_pw, &(b.hi_light), x2);				\
	fted_FTdraw_rect (fted_canvas_pw, &(b.out_fted_line2), x3);


#endif
