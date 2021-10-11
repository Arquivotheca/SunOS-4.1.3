#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)input.c 1.1 92/07/30 Copyr 1984 Sun Micro";
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
#include <sundev/kbd.h>
/* @&*#*%$@%!! kbd.h defines reset!!! undefine it, so i can use it */
#undef RESET

#define MAX_B_GROUPS		100
button_group		*fted_group_of_button_groups[MAX_B_GROUPS];
int			fted_b_grp_ptr = 0;
Menu			fted_menu_handle = NULL;

/*
 * The input process consists of 2 stages. The first filters the
 * input event (ie) and the second takes the output of the filter
 * and determines what action is to be taken because of the ie.
 * 
 * 1st stage:
 *  An input event could be from the keyboard, mouse or timer.
 * The timer is not used now but it could be in the future.
 * Anyhow, if the ie is from the keyboard, the appropriate routine
 * is called and that is that. When the input is from the mouse,
 * it is filtered. The state table below has as inputs the current
 * state and the mouse action and it generates a new state and 
 * INPUT_EVENT. For example, it generates INPUT_EVENT_MOVE if the
 * mouse moves with the button down held down and generates 
 * INPUT_EVENT_IGNORE if two mouse buttons are pressed.
 * 
 * 2nd Stage
 *  If the event is one of  INPUT_EVENT_{DOWN,MOVE,UP}, we
 * do pick correlation. If the location of the event is inside
 * a button, we do the action associated with that button which
 * is specified by the event if the button is an action button.
 * If the button is a input button, the event is treated as 
 * an input to a state table give in "action_table.h". 
 * 
 * 
 * The input routine keeps track of the last action done so that
 * it can be reset. For actions buttons, we keep a pointer to
 * a routine that can be called to reset the button -- for example,
 * when the  mouse moves off a button, the reset routine is called
 * and the button becomes un hilighted. 
 */


#include "action_table.h"


typedef enum {
	INPUT_STATE_START,
	INPUT_STATE_BUTTON_DOWN
} INPUT_STATE;

char *fted_input_state_names[] = {
    "START", "BUTTON DOWN"
};
#define NUM_EVENT_STATES	2

typedef enum {
	INPUT_EVENT_DOWN,
	INPUT_EVENT_MOVE,
	INPUT_EVENT_UP,
	INPUT_EVENT_EXIT,
	INPUT_EVENT_TIME,
	INPUT_EVENT_IGNORE,
	INPUT_EVENT_CLEAN_UP
} INPUT_EVENT;

char *fted_event_names[] = {
	"INPUT_EVENT_DOWN",
	"INPUT_EVENT_MOVE",
	"INPUT_EVENT_UP",
	"INPUT_EVENT_EXIT",
	"INPUT_EVENT_TIME",
	"INPUT_EVENT_IGNORE",
	"INPUT_EVENT_CLEAN_UP"
};


#define NUM_EVENT_INPUTS	5

typedef struct {
    INPUT_STATE     next_state;
    INPUT_EVENT     event_code;
}               event_state;

event_state	fted_event_states[NUM_EVENT_INPUTS][NUM_EVENT_STATES] = {
/*states:	START							*/
/*		BUTTON DOWN						*/
/*----------------------------------------------------------------------*/
/*inputs: 								*/
/* DOWN */ 	{INPUT_STATE_BUTTON_DOWN,	INPUT_EVENT_DOWN},
		{INPUT_STATE_BUTTON_DOWN, 	INPUT_EVENT_IGNORE},
/* MOVE */ 	{INPUT_STATE_START,		INPUT_EVENT_IGNORE},
		{INPUT_STATE_BUTTON_DOWN, 	INPUT_EVENT_MOVE},
/* UP   */ 	{INPUT_STATE_START,		INPUT_EVENT_IGNORE},
		{INPUT_STATE_START, 		INPUT_EVENT_UP},
/* EXIT */ 	{INPUT_STATE_START, 		INPUT_EVENT_CLEAN_UP},
		{INPUT_STATE_START, 		INPUT_EVENT_CLEAN_UP},
/* TIME */ 	{INPUT_STATE_START, 		INPUT_EVENT_IGNORE},
		{INPUT_STATE_BUTTON_DOWN, 	INPUT_EVENT_DOWN }
};

static event_state 	cur_event_state = {INPUT_STATE_START, INPUT_EVENT_DOWN};
/* for post-mortem debugging: */
static event_state	last_event_state3;
static INPUT_EVENT	last_event3;
static event_state	last_event_state2;
static INPUT_EVENT	last_event2;
static event_state	last_event_state1;
static INPUT_EVENT	last_event1;

static int		(*reset)() = fted_nullproc;
static button          *button_to_be_reset = NULL;
static int		mouse_button;

typedef enum {
    ACTION_TYPE_NONE,
    ACTION_TYPE_ACTION,
    ACTION_TYPE_INPUT
} ACTION_TYPE;

ACTION_TYPE 	fted_last_action_taken;



fted_canvas_selected(nullsw, ibits, obits, ebits, timer)
caddr_t * nullsw;
int    *ibits, *obits, *ebits;
struct timeval **timer;
{
    struct inputevent   ie;
    register int    	i, j;
    register int    	x, y;
    register button 	*but;
    register button_group *b_grp;
    register button 	**but_ptr;
    int			action_taken;   
    INPUT_EVENT     	event_code;
    int     		shifted;
    Menu_item		menu_item;
    int			action;

    if (input_readevent (fted_canvas_sub_win -> ts_windowfd, &ie) == -1) {
	perror ("fontedit input failed in canvas_select");
	*ibits = *obits = *ebits = 0;
	return;
    }
    action = event_action(&ie);
    if (action <= ASCII_LAST) { /* input was from the keyboard */
	fted_scroll_char_pressed(action);
	*ibits = *obits = *ebits = 0;
	return;
    }
    if (*timer == NULL) {
	switch (action) {
	    case MS_LEFT: 
		if (win_inputnegevent (&ie))
		    event_code = INPUT_EVENT_UP;
		else
		    event_code = INPUT_EVENT_DOWN;
		mouse_button = MS_LEFT;
		break;
	    case MS_MIDDLE: 
		if (win_inputnegevent (&ie))
		    event_code = INPUT_EVENT_UP;
		else
		    event_code = INPUT_EVENT_DOWN;
		mouse_button = MS_MIDDLE;
		break;
	    case LOC_MOVEWHILEBUTDOWN: 
		event_code = INPUT_EVENT_MOVE;
		break;
	    case LOC_WINEXIT: 
		event_code = INPUT_EVENT_EXIT;
		break;
	    case MENU_BUT:
	        if (!win_inputnegevent (&ie)) { /* button is pressed down */
		    if (fted_menu_handle == NULL)
			/* actual menu data should be -1, this is to avoid
			 * the need to find out whether an item is selected
			 * or not
			 */
			fted_menu_handle = menu_create(
			    MENU_STRING_ITEM, "Single Pt",	(caddr_t) 1,
			    MENU_STRING_ITEM, "Pt Wipe",	(caddr_t) 2,
			    MENU_STRING_ITEM, "Line",		(caddr_t) 3,
			    MENU_STRING_ITEM, "Rect",		(caddr_t) 4,
			    MENU_STRING_ITEM, "Cut",		(caddr_t) 5,
			    MENU_STRING_ITEM, "Copy",		(caddr_t) 6,
			    MENU_STRING_ITEM, "Paste",		(caddr_t) 7,
			    0);
		    menu_item = menu_show_using_fd(fted_menu_handle,
				fted_canvas_sub_win->ts_windowfd, &ie);
		    if (menu_item != NULL) {
			(caddr_t)menu_item--;
			panel_set_value(fted_draw_options_item, menu_item);
		        fted_set_drawing_option(0, menu_item, 0);
		    }
		}
		*ibits = *obits = *ebits = 0;
		return;
	    default: 
		fprintf (stderr,
			"Bad ie code in fted_canvas_selected: %x\n", action);
		*ibits = *obits = *ebits = 0;
		return;

	}
	if (ie.ie_shiftmask & SHIFTMASK)
	    shifted = TRUE;
	else
	    shifted = FALSE;
    }
    else if ( ((*timer)->tv_sec == 0) && ((*timer)->tv_usec == 0) )
        event_code = INPUT_EVENT_DOWN;	/* pretend the button was pressed again */
    else {
	fprintf(stderr, "So confused in fted_canvas_selected...\n");
	exit(123);
    }
#ifdef DBG
 printf("event code is: %s\n",fted_event_names[(int)event_code]);
 printf("   old state : %15s, %s\n",fted_input_state_names[(int)cur_event_state.next_state],
                                      fted_event_names[(int)cur_event_state.event_code];
#endif

    last_event_state3 = last_event_state2;
    last_event3 = last_event2;
    last_event_state2 = last_event_state1;
    last_event2 = last_event1;
    last_event_state1 = cur_event_state; 
    last_event1 = event_code;

    cur_event_state = fted_event_states[(int)event_code][(int)cur_event_state.next_state];

#ifdef DBG
 printf("   new state : %15s, %s\n\n",fted_input_state_names[(int)cur_event_state.next_state],
                                       fted_event_names[(int)cur_event_state.event_code]);
#endif


    if ( cur_event_state.event_code == INPUT_EVENT_CLEAN_UP) {
	if (fted_last_action_taken == ACTION_TYPE_INPUT) {
	    fted_cur_action_state =
			fted_action_states[(int)fted_cur_action_state.next_state][(int)RESET];
	    (*fted_cur_action_state.action)(x, y, but->id, mouse_button);
	    fted_last_action_taken = ACTION_TYPE_NONE;
	}
	else if (fted_last_action_taken == ACTION_TYPE_ACTION) {
	    (*reset)(x, y);
	    reset = fted_nullproc;
	    fted_last_action_taken = ACTION_TYPE_NONE;
	}
	button_to_be_reset == NULL;
    }
    else if( cur_event_state.event_code == INPUT_EVENT_IGNORE ) {

    }

    else {		/* find which button the event occured in */
        x = ie.ie_locx;
	y = ie.ie_locy;
	action_taken = FALSE;
	for(i = 0; i < fted_b_grp_ptr ; i++) {
	    b_grp = fted_group_of_button_groups[i];
	    if( POINT_IN_RECT( b_grp->bounding_box, x, y) ) {
		for( but_ptr = b_grp->buttons, j = 0; j < b_grp->num_buttons;
		                                                 but_ptr++, j++) {
		    but = *but_ptr;
		    if( POINT_IN_RECT(but->out_fted_line2, x, y) ) {
			if ( button_to_be_reset == NULL) {
			    reset = but->reset;
			    button_to_be_reset = but;
			}
			else if ( button_to_be_reset != but ) {
			    if (fted_last_action_taken == ACTION_TYPE_INPUT) {
				fted_cur_action_state =
			    		fted_action_states
					[(int)fted_cur_action_state.next_state]
					[(int)RESET];
			        (*fted_cur_action_state.action)
					(x, y, but->id, mouse_button);
			    }
			    else if(fted_last_action_taken == ACTION_TYPE_ACTION) {
			    	(*reset)(x, y);
			    }
			    reset = (but->reset ? but->reset : fted_nullproc);
			    button_to_be_reset = but;
			}
/*
			else {
			    reset = but->reset;
			    button_to_be_reset = but;
			}
*/
			action_taken = TRUE;
			if (but->type == BUTTON_TYPE_ACTION) {
		            fted_last_action_taken = ACTION_TYPE_ACTION;
			    switch (cur_event_state.event_code) {
				case DOWN:
				  (*but->down)(but, j, x, y);
				  break;
				case MOVE:
				  (*but->move)(but, j, x, y);
				  break;
				case UP:
				  (*but->up)(but, j, x, y);
				  break;
				default:
				  fprintf(stderr,"Bad event code in button action CALL\n");
				  break;
			    }
			    *ibits = *obits = *ebits = 0;
			    return;
			}
			else {    /* assume button is an input to an FSA */
#ifdef DBG
 printf("ACTION_TYPE_INPUT:\n");
 printf("event code is: %s\n",fted_event_names[(int)event_code]);
 printf("   old state : %15s\n",fted_edit_state_names[(int)fted_cur_action_state.next_state]);
#endif 
			    fted_last_action_taken = ACTION_TYPE_INPUT;
 			    fted_cur_action_state =
			    	fted_action_states[(int)fted_cur_action_state.next_state][(int)event_code];
			    
#ifdef DBG
 printf("   new state : %15s\n\n", fted_edit_state_names[(int)fted_cur_action_state.next_state]);
#endif
			    (*fted_cur_action_state.action)(x, y, but->id,mouse_button,shifted);
			    *ibits = *obits = *ebits = 0;
			    return;
			}
		    } /*end if (x,y) in button */
		} /* for each button */
	    } /* end if (x,y) in button group */
	} /* end for each button group */
	if (!action_taken) {
	    if (fted_last_action_taken == ACTION_TYPE_INPUT) {
		fted_cur_action_state =
			fted_action_states[(int)fted_cur_action_state.next_state][(int)RESET];
		(*fted_cur_action_state.action)(x, y, but->id, mouse_button);
		fted_last_action_taken = ACTION_TYPE_NONE;
	    }
	    else if (fted_last_action_taken == ACTION_TYPE_ACTION) {
		(*reset)(x, y);
	        reset = fted_nullproc;
		fted_last_action_taken = ACTION_TYPE_NONE;
	    }
	    button_to_be_reset = NULL;
	}
    }
    *ibits = *obits = *ebits = 0;
    return;
}


fted_add_button_group( grp )
button_group	*grp;
{
    if (fted_b_grp_ptr >= MAX_B_GROUPS) {
	fprintf(stderr,"out of room in fted_add_button_group\n");
	exit(-98);
    }
    fted_group_of_button_groups[fted_b_grp_ptr++] = grp;
    fted_group_of_button_groups[fted_b_grp_ptr] = NULL;
}

/*
 * 
 */

    
void fted_set_drawing_option(item, new_val, event)
Panel_item		item;
int			new_val;		/* new option value */
struct inputevent	*event;
{

#ifdef DBG
  printf("  input for new state: %s\n",	fted_input_names[new_val + (int)SET_STATE_POINT_SINGLE]);
#endif
    fted_cur_action_state =
    	fted_action_states[(int)fted_cur_action_state.next_state][new_val + (int)SET_STATE_POINT_SINGLE];
    (*fted_cur_action_state.action)(0, 0, 0, 0);
}



fted_post_mortem()
{
    printf("\nPost_mortem of the input handler:\n");
    printf("3 event code: %s\n", fted_event_names[(int)last_event3]);
    printf("  state:\tnext: %s\tcode: %s\n\n",
    	fted_input_state_names[(int)last_event_state3.next_state],
	fted_event_names[(int)last_event_state3.event_code]);
    printf("2 event code: %s\n", fted_event_names[(int)last_event2]);
    printf("  state:\tnext: %s\tcode: %s\n\n",
    	fted_input_state_names[(int)last_event_state2.next_state],
	fted_event_names[(int)last_event_state2.event_code]);
    printf("1 event code: %s\n", fted_event_names[(int)last_event1]);
    printf("  state:\tnext: %s\tcode: %s\n\n\n",
    	fted_input_state_names[(int)last_event_state1.next_state],
	fted_event_names[(int)last_event_state1.event_code]);
}
