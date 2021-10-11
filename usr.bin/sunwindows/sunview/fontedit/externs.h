/*	@(#)externs.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Declare these variables in the main file and have them be external in
 * other files.
 */

#ifdef FT$MAIN
/* declare items as they appear on screen */
Panel_item		fted_font_name_item;
char			fted_font_name[FT$PATHLEN];

Panel_item		fted_read_item;
void			fted_read_new_font();

Panel_item		fted_save_item;
void			fted_save_current_font();

Panel_item		fted_exit_item;
void			fted_exit_fted_font_tool();

Panel_item		fted_mafted_x_height_item;

Panel_item		fted_max_width_item;

Panel_item		fted_caps_height_item;

Panel_item		fted_x_height_item;
 
Panel_item		fted_baseline_item;

Panel_item 		fted_apply_item;
void			fted_apply_font_specs();

Panel_item 		fted_draw_options_item;
void			fted_set_drawing_option();



/* the tool and its subwindows */
struct tool		*fted_font_tool;
struct toolsw		*fted_msg_sub_win;
Panel_item		fted_msg_item;
struct toolsw		*fted_option_sub_win;
struct toolsw		*fted_canvas_sub_win;
struct toolsw		*fted_echo_sub_win;		/* where text is echoed in the current font */

PIXFONT			*fted_message_font;		 /* font msgs are displayed in */

int			fted_nullproc();
int			fted_canvas_selected();


struct pixwin		*fted_canvas_pw;		/* this window's pixwin fd */
struct pixwin		*fted_echo_pw;		/* window where text is echoed */

PIXFONT			*fted_tiny_font;		/* a small font for screen messages */

PIXFONT			*fted_cur_font;		/* the font being edited */
/* Info about each char in the font. */
int			fted_font_base_line;		/* measured from the top of */
						/* bitmap		    */
int			fted_font_left_edge;		/* properties of charactes */
int			fted_font_right_edge;	/* in the font		   */
int			fted_font_caps_height;
int			fted_font_x_height;
int			fted_char_max_width;		/* size of largest char in font */
int		 	fted_char_max_height;
int			fted_image_max_height;	/* size of editing area */
int			fted_image_max_widht;
int			fted_num_chars_in_font;	/* vfont sets this to 256 */
int			fted_font_modified;		/* true if font has been modified*/
int			fted_first_button_char;	/* the character to be displayed */
						/* in the first button -- */
					  	/* the one on the left */

a_fted_char_info		fted_char_info[MAX_NUM_CHARS_IN_FONT];
FILE			*fted_sysout = stdout;

int 			fted_slider_cmd_advance();
int 			fted_step_func_x();
int			fted_step_func_y();
int 			fted_undoing;

/*
 * define the variables as externs
 */

#else
/* declare items as they appear on screen */

extern Panel_item		fted_font_name_item;
extern char			fted_font_name[];

extern Panel_item		fted_read_item;
extern void			fted_read_new_font();

extern Panel_item		fted_save_item;
extern void			fted_save_current_font();

extern Panel_item		fted_exit_item;
extern void			fted_exit_fted_font_tool();

extern Panel_item		fted_mafted_x_height_item;

extern Panel_item		fted_max_width_item;

extern Panel_item		fted_caps_height_item;

extern Panel_item		fted_x_height_item;
 
extern Panel_item		fted_baseline_item;

extern Panel_item		fted_apply_item;
extern void			fted_apply_font_specs();

extern Panel_item		fted_draw_options_item;
extern void			fted_set_drawing_option();

extern Menu 			fted_menu_handle;

extern int			fted_nullproc();
extern int			fted_canvas_selected();



extern PIXFONT			*fted_message_font;

/* the tool and it's subwindows: */
extern struct tool		*fted_font_tool;
extern struct toolsw		*fted_msg_sub_win;
extern Panel_item		fted_msg_item;
extern struct toolsw		*fted_option_sub_win;
extern struct toolsw		*fted_canvas_sub_win;
extern struct toolsw		*fted_echo_sub_win;


extern struct pixwin		*fted_canvas_pw;
extern struct pixwin		*fted_echo_pw;
extern PIXFONT			*fted_tiny_font;		/* a small font for screen messages */

extern PIXFONT			*fted_cur_font;		/* the font being edited */
/* Info about each char in the font. */
extern int			fted_font_caps_height;
extern int			fted_font_base_line;
extern int			fted_font_left_edge;
extern int			fted_font_right_edge;
extern int			fted_font_x_height;
extern int			fted_char_max_width;
extern int		 	fted_char_max_height;
extern int			fted_num_chars_in_font;
extern int			fted_font_modified;
extern int			fted_first_button_char;	/* the character to be displayed in the */
							/* first button - the one on the left */

extern a_fted_char_info		fted_char_info[];

extern FILE		*fted_sysout;
int 			fted_slider_cmd_advance();
int 			fted_step_func_x();
int			fted_step_func_y();
extern int		fted_undoing;
#endif
