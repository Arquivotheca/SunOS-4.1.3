/*	@(#)action_table.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/* editing states: */ 
typedef enum {
	EDIT_STATE_POINT_SINGLE,
	EDIT_STATE_POINT_WIPE,
	EDIT_STATE_LINE,
	EDIT_STATE_RECT,
	EDIT_STATE_CUT,
	EDIT_STATE_COPY,
	EDIT_STATE_PASTE,
	NUM_EDIT_STATES
} EDIT_STATE;

char *fted_edit_state_names[] = {
	"EDIT_STATE_POINT_SINGLE",
	"EDIT_STATE_POINT_WIPE",
	"EDIT_STATE_LINE",
	"EDIT_STATE_RECT",
	"EDIT_STATE_CUT",
	"EDIT_STATE_COPY",
	"EDIT_STATE_PASTE"
};

typedef enum {
	DOWN,
	MOVE,
	UP,
	EXIT,
	TIME,
	SET_STATE_POINT_SINGLE,
	SET_STATE_POINT_WIPE,
	SET_STATE_LINE,
	SET_STATE_RECT,
	SET_STATE_CUT,
	SET_STATE_COPY,
	SET_STATE_PASTE,
	RESET,
	NUM_INPUTS
} INPUT;

char *fted_input_names[] = {
	"DOWN",
	"MOVE",
	"UP",
	"EXIT",
	"TIME",
	"SET_STATE_POINT_SINGLE",
	"SET_STATE_POINT_WIPE",
	"SET_STATE_LINE",
	"SET_STATE_RECT",
	"SET_STATE_CUT",
	"SET_STATE_COPY",
	"SET_STATE_PASTE",
	"RESET"
};	
#define NUM_ACTION_INPUTS	14



extern int	fted_hilight_cell(),	fted_move_hilight(), fted_reset_cell_hilight();
extern int	fted_point_single0(), fted_point_single3(); 
extern int	fted_point_wipe0(), fted_point_wipe1(), fted_point_wipe2(), fted_point_wipe3(), fted_point_wipe_reset();
extern int	fted_line0(), fted_line1(), fted_line2(), fted_line3(), fted_line_reset();
extern int	fted_rect0(), fted_rect1(), fted_rect2(), fted_rect3(), fted_rect_reset();
extern int 	fted_cut_copy1(), fted_cut_copy2();
extern int	fted_cut0(), fted_cut3(), cut_reset();
extern int	fted_copy0(),fted_copy3(), copy2(), copy_reset();
extern int	fted_paste0(), fted_paste1(), fted_paste2(), fted_paste3(), fted_paste_reset();


typedef struct {
    EDIT_STATE	next_state;
    int		(*action)();
} action_state;

action_state 	fted_cur_action_state = { EDIT_STATE_POINT_SINGLE, fted_point_single0};

action_state	fted_action_states[NUM_EDIT_STATES][NUM_INPUTS] = {

/* inputs		POINT_SINGLE	*/
/* DOWN		*/	{EDIT_STATE_POINT_SINGLE,	fted_hilight_cell},
/* MOVE		*/	{EDIT_STATE_POINT_SINGLE,	fted_move_hilight},
/* UP		*/	{EDIT_STATE_POINT_SINGLE, 	fted_point_single3},
/* EXIT		*/	{EDIT_STATE_POINT_SINGLE,  	fted_point_single0},
/* TIME		*/	{EDIT_STATE_POINT_SINGLE,  	fted_point_single0},
/* SS_POINT_SINGLE*/	{EDIT_STATE_POINT_SINGLE,	fted_point_single0},
/* SS_POINT_WIPE*/	{EDIT_STATE_POINT_WIPE,		fted_point_wipe0},
/* SS_LINE	*/	{EDIT_STATE_LINE,		fted_line0},
/* SS_RECT	*/	{EDIT_STATE_RECT, 		fted_rect0},
/* SS_CUT	*/	{EDIT_STATE_CUT, 		fted_cut0},
/* SS_COPY	*/	{EDIT_STATE_COPY, 		fted_copy0},
/* SS_PASTE	*/	{EDIT_STATE_PASTE, 		fted_paste0},
/* RESET	*/	{EDIT_STATE_POINT_SINGLE, 	fted_reset_cell_hilight},

		/*	POINT_WIPE	*/
/* DOWN		*/	{EDIT_STATE_POINT_WIPE,		fted_point_wipe1},
/* MOVE		*/	{EDIT_STATE_POINT_WIPE,		fted_point_wipe2},
/* UP		*/	{EDIT_STATE_POINT_WIPE, 	fted_point_wipe3},
/* EXIT		*/	{EDIT_STATE_POINT_SINGLE,  	fted_point_single0},
/* TIME		*/	{EDIT_STATE_POINT_SINGLE,  	fted_point_single0},
/* SS_POINT_SINGLE*/	{EDIT_STATE_POINT_SINGLE,	fted_point_single0},
/* SS_POINT_WIPE*/	{EDIT_STATE_POINT_WIPE,		fted_point_wipe0},
/* SS_LINE	*/	{EDIT_STATE_LINE,		fted_line0},
/* SS_RECT	*/	{EDIT_STATE_RECT, 		fted_rect0},
/* SS_CUT	*/	{EDIT_STATE_CUT, 		fted_cut0},
/* SS_COPY	*/	{EDIT_STATE_COPY, 		fted_copy0},
/* SS_PASTE	*/	{EDIT_STATE_PASTE, 		fted_paste0},
/* RESET	*/	{EDIT_STATE_POINT_WIPE, 	fted_point_wipe_reset},

		/*	LINE	*/
/* DOWN		*/	{EDIT_STATE_LINE,		fted_line1},
/* MOVE		*/	{EDIT_STATE_LINE,		fted_line2},
/* UP		*/	{EDIT_STATE_LINE, 		fted_line3},
/* EXIT		*/	{EDIT_STATE_POINT_SINGLE,  	fted_point_single0},
/* TIME		*/	{EDIT_STATE_POINT_SINGLE,  	fted_point_single0},
/* SS_POINT_SINGLE*/	{EDIT_STATE_POINT_SINGLE,	fted_point_single0},
/* SS_POINT_WIPE*/	{EDIT_STATE_POINT_WIPE,		fted_point_wipe0},
/* SS_LINE	*/	{EDIT_STATE_LINE,		fted_line0},
/* SS_RECT	*/	{EDIT_STATE_RECT, 		fted_rect0},
/* SS_CUT	*/	{EDIT_STATE_CUT, 		fted_cut0},
/* SS_COPY	*/	{EDIT_STATE_COPY, 		fted_copy0},
/* SS_PASTE	*/	{EDIT_STATE_PASTE, 		fted_paste0},
/* RESET	*/	{EDIT_STATE_LINE, 		fted_line0},




		/*	RECT	*/
/* DOWN		*/	{EDIT_STATE_RECT,		fted_rect1},
/* MOVE		*/	{EDIT_STATE_RECT,		fted_rect2},
/* UP		*/	{EDIT_STATE_RECT, 		fted_rect3},
/* EXIT		*/	{EDIT_STATE_POINT_SINGLE,  	fted_point_single0},
/* TIME		*/	{EDIT_STATE_POINT_SINGLE,  	fted_point_single0},
/* SS_POINT_SINGLE*/	{EDIT_STATE_POINT_SINGLE,	fted_point_single0},
/* SS_POINT_WIPE*/	{EDIT_STATE_POINT_WIPE,		fted_point_wipe0},
/* SS_LINE	*/	{EDIT_STATE_LINE,		fted_line0},
/* SS_RECT	*/	{EDIT_STATE_RECT, 		fted_rect0},
/* SS_CUT	*/	{EDIT_STATE_CUT, 		fted_cut0},
/* SS_COPY	*/	{EDIT_STATE_COPY, 		fted_copy0},
/* SS_PASTE	*/	{EDIT_STATE_PASTE, 		fted_paste0},
/* RESET	*/	{EDIT_STATE_RECT, 		fted_rect0},




		/*	CUT	*/
/* DOWN		*/	{EDIT_STATE_CUT,		fted_cut_copy1},
/* MOVE		*/	{EDIT_STATE_CUT,		fted_cut_copy2},
/* UP		*/	{EDIT_STATE_CUT, 		fted_cut3},
/* EXIT		*/	{EDIT_STATE_POINT_SINGLE,  	fted_point_single0},
/* TIME		*/	{EDIT_STATE_POINT_SINGLE,  	fted_point_single0},
/* SS_POINT_SINGLE*/	{EDIT_STATE_POINT_SINGLE,	fted_point_single0},
/* SS_POINT_WIPE*/	{EDIT_STATE_POINT_WIPE,		fted_point_wipe0},
/* SS_LINE	*/	{EDIT_STATE_LINE,		fted_line0},
/* SS_RECT	*/	{EDIT_STATE_RECT, 		fted_rect0},
/* SS_CUT	*/	{EDIT_STATE_CUT, 		fted_cut0},
/* SS_COPY	*/	{EDIT_STATE_COPY, 		fted_copy0},
/* SS_PASTE	*/	{EDIT_STATE_PASTE, 		fted_paste0},
/* RESET	*/	{EDIT_STATE_CUT, 		fted_cut0},


		/*	COPY	*/
/* DOWN		*/	{EDIT_STATE_COPY,		fted_cut_copy1},
/* MOVE		*/	{EDIT_STATE_COPY,		fted_cut_copy2},
/* UP		*/	{EDIT_STATE_COPY, 		fted_copy3},
/* EXIT		*/	{EDIT_STATE_POINT_SINGLE,  	fted_point_single0},
/* TIME		*/	{EDIT_STATE_POINT_SINGLE,  	fted_point_single0},
/* SS_POINT_SINGLE*/	{EDIT_STATE_POINT_SINGLE,	fted_point_single0},
/* SS_POINT_WIPE*/	{EDIT_STATE_POINT_WIPE,		fted_point_wipe0},
/* SS_LINE	*/	{EDIT_STATE_LINE,		fted_line0},
/* SS_RECT	*/	{EDIT_STATE_RECT, 		fted_rect0},
/* SS_CUT	*/	{EDIT_STATE_CUT, 		fted_cut0},
/* SS_COPY	*/	{EDIT_STATE_COPY, 		fted_copy0},
/* SS_PASTE	*/	{EDIT_STATE_PASTE, 		fted_paste0},
/* RESET	*/	{EDIT_STATE_COPY, 		fted_copy0},

		/*	PASTE	*/
/* DOWN		*/	{EDIT_STATE_PASTE,		fted_paste1},
/* MOVE		*/	{EDIT_STATE_PASTE,		fted_paste2},
/* UP		*/	{EDIT_STATE_PASTE, 		fted_paste3},
/* EXIT		*/	{EDIT_STATE_POINT_SINGLE,  	fted_point_single0},
/* TIME		*/	{EDIT_STATE_POINT_SINGLE,  	fted_point_single0},
/* SS_POINT_SINGLE*/	{EDIT_STATE_POINT_SINGLE,	fted_point_single0},
/* SS_POINT_WIPE*/	{EDIT_STATE_POINT_WIPE,		fted_point_wipe0},
/* SS_LINE	*/	{EDIT_STATE_LINE,		fted_line0},
/* SS_RECT	*/	{EDIT_STATE_RECT, 		fted_rect0},
/* SS_CUT	*/	{EDIT_STATE_CUT, 		fted_cut0},
/* SS_COPY	*/	{EDIT_STATE_COPY, 		fted_copy0},
/* SS_PASTE	*/	{EDIT_STATE_PASTE, 		fted_paste0},
/* RESET	*/	{EDIT_STATE_PASTE, 		fted_paste0}

};
