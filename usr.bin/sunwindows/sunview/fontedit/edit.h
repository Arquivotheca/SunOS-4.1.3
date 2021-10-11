/*	@(#)edit.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#define NUM_EDIT_PADS		5
#define NUM_CHAR_BUTTONS 	8

#define EDIT_BUTTON		0
#define NUM_EDIT_BUTTONS	1

#define TOP_SLIDER		0
#define BOTTOM_SLIDER		1
#define RIGHT_EDGE_SLIDER	2
#define ADV_SLIDER		3
#define NUM_SLIDERS		4

#define UNDO_BUTTON		0
#define SAVE_BUTTON		1
#define QUIT_BUTTON		2
#define NUM_COMMAND_BUTTONS	3

#define FIRST_WINDOW_X		35

#define PROOF_WIDTH		20
#define PROOF_HEIGHT		25
#define COMMAND_WIDTH		45
#define COMMAND_HEIGHT		25
#define SCALE_WIDTH		20
#define SCALE_HEIGHT		16

#define SCROLL_HEIGHT		25
#define SCROLL_TEXT_Y_OFFSET	16
#define SCROLL_INDICATOR_Y_OFFSET 3
#define SCROLL_INDICATOR_HEIGHT (SCROLL_TEXT_Y_OFFSET - SCROLL_INDICATOR_Y_OFFSET)


#define X_SPACE			5
#define Y_SPACE			5
#define EDIT_X_OFFSET		7
#define EDIT_Y_OFFSET		8	
#define WINDOW_X_OFFSET		20
#define WINDOW_Y_OFFSET		20

#define COMMAND_TEXT_X_OFFSET	4
#define COMMAND_TEXT_Y_OFFSET	15

#define SLIDER_TEXT_X_OFFSET	2
#define SLIDER_TEXT_Y_OFFSET	15


#define PROOF_X_OFFSET		8
#define PROOF_Y_OFFSET		5

#define SLIDER_WIDTH		20
#define SLIDER_HEIGHT		20
#define SLIDER_LONGER_WIDTH	35
#define SLIDER_LONGER_HEIGHT	35

 


typedef struct Undo_node {
    struct pixchar	*pix_char_ptr;
    struct Undo_node	*next;
    struct rect		window;
} undo_node;

typedef struct {
    int			open;		/* true if this window is open	*/
    int			modified;	/* true if the image has been	*/
    					/* modified sinced it was saved */
    int			char_num;	/* number of the character	*/
    struct pixchar	*pix_char_ptr;	/* representation of the character */
    button		proof;		/* the "proof" of the character */
    button		left_height_scale;	/* scale for side		*/
    button		right_height_scale;
    button		top_width_scale;
    button		bottom_width_scale;	/* scale for bottom		*/
    struct rect		whole;		/* rect that covers the whole pad */
    struct rect		window;		/* window into pixrect which is
					   the bitmap of the character 	*/
    undo_node		*undo_list;
} edit_info;

typedef struct {
    button_group	*cmd;		/* command buttons 		*/
    button_group	*edit;		/* editing buttons		*/
    slider		*sliders[NUM_SLIDERS];	/* sliders 		*/
}group_of_groups;


#ifdef EDIT$MAIN
group_of_groups		fted_edit_pad_groups[NUM_EDIT_PADS];
edit_info		fted_edit_pad_info[NUM_EDIT_PADS];
button			*fted_char_buttons[NUM_CHAR_BUTTONS];
button_group 		fted_char_button_group;
slider			*fted_scroll_slider;

#else
extern group_of_groups	fted_edit_pad_groups[NUM_EDIT_PADS];
extern edit_info	fted_edit_pad_info[NUM_EDIT_PADS];
extern button		*fted_char_buttons[NUM_CHAR_BUTTONS];
extern button_group	fted_char_button_group;
extern slider		*fted_scroll_slider;
#endif
