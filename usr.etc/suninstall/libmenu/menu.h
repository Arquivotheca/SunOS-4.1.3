/*
 * @(#)menu.h 1.1 92/07/30
 */

#ifndef _MENU_H_
#define _MENU_H_

/*
 *	Name:		menu.h
 *
 *	Description:	Global declarations for menu library.
 */


/*
 *	Global constants:
 */
#define ACTIVE		1		/* menu object is active */
#define ATTR_NORMAL	((menu_attr) 0)	/* normal attributes */
#define ATTR_STAND	((menu_attr) 1)	/* standout attributes */

#ifndef CP_NULL
#	define CP_NULL	((char *) 0)
#endif

#define NOT_ACTIVE	0		/* menu object is not active */

#ifndef NULL
#	define NULL	0
#endif

#ifndef PFI_NULL
#	define PFI_NULL	((int (*)()) 0)
#endif

#ifndef PTR_NULL
#	define PTR_NULL	((pointer) 0)
#endif




/*
 *	Global types:
 */

/*
 *	Do the typedefs up front so the data structures can be in
 *	alphabetical order.
 */
typedef struct form_button_t	form_button;
typedef struct form_field_t	form_field;
typedef struct form_map_t	form_map;
typedef struct form_noecho_t	form_noecho;
typedef struct form_radio_t	form_radio;
typedef struct form_t		form;
typedef struct form_yesno_t	form_yesno;
typedef unsigned int		menu_attr;
typedef unsigned char		menu_coord;
typedef struct menu_file_t	menu_file;
typedef struct menu_item_t	menu_item;
typedef struct menu_string_t	menu_string;
typedef struct menu_t		menu;
typedef int *			pointer;


/*
 *	Definition of a form:
 *
 *		m_type		- type of menu object
 *		m_help		- pointer to the help function
 *		f_active	- is this form active?
 *		f_name		- name of this form
 *		f_mstrings	- menu strings associated with this form
 *		f_fields	- field associated with this form
 *		f_files		- files associated with this form
 *		f_noechos	- noechos associated with this form
 *		f_radios	- radios associated with this form
 *		f_map		- map of objects on this form
 *		f_obj		- ptr to current object's map
 *		f_finish	- ptr to finish object
 *		f_shared	- ptr to shared yes/no object
 */
struct form_t {
	int		m_type;
	int (*		m_help)();
	int		f_active;
	char *		f_name;
	menu_string *	f_mstrings;
	form_field *	f_fields;
	menu_file *	f_files;
	form_noecho *	f_noechos;
	form_radio *	f_radios;
	form_yesno *	f_yesnos;
	form_map *	f_map;
	form_map *	f_obj;
	form_yesno *	f_finish;
	form_yesno *	f_shared;
};


/*
 *	Definition of a form button:
 *
 *		m_type		- type of menu object
 *		m_help		- pointer to the help function
 *		fb_active	- is this button active?
 *		fb_name		- name of this button
 *		fb_mstrings	- menu strings associated with this button
 *		fb_x		- x-coordinate of this button
 *		fb_y		- y-coordinate of this button
 *		fb_code		- code to place in radio's code buffer
 *		fb_func		- function to invoke if button is selected
 *		fb_arg		- argument for function
 *		fb_next		- pointer to next button
 *		fb_radio	- pointer to parent radio
 */
struct form_button_t {
	int		m_type;
	int (*		m_help)();
	char		fb_active;
	char *		fb_name;
	menu_string *	fb_mstrings;
	menu_coord	fb_x;
	menu_coord	fb_y;
	int		fb_code;
	int (*		fb_func)();
	pointer		fb_arg;
	form_button *	fb_next;
	form_radio *	fb_radio;
};


/*
 *	Definition of a form field:
 *
 *		m_type		- type of menu object
 *		m_help		- pointer to the help function
 *		ff_active	- is this field active?
 *		ff_name		- name of this field
 *		ff_mstrings	- menu strings associated with this field
 *		ff_x		- starting x-coordinate of this field
 *		ff_y		- starting y-coordinate of this field
 *		ff_width	- width of this field
 *		ff_data		- pointer to data buffer for this field
 *		ff_datasize	- size of buffer 'ff_data'
 *		ff_lex		- pointer to the lexical function
 *		ff_prefunc	- pointer to pre-function.  The following
 *				  return values are expected:
 *					 1 -> proceed with getting data
 *					 0 -> return with out getting data
 *					-1 -> return with error condition
 *		ff_prearg	- argument for pre-function
 *		ff_postfunc	- pointer to post-function
 *					 1 -> data is valid
 *					 0 -> data is invalid try again
 *					-1 -> return with error condition
 *		ff_postarg	- argument for post-function
 *		ff_next		- pointer to next field
 */
struct form_field_t {
	int		m_type;
	int (*		m_help)();
	char		ff_active;
	char *		ff_name;
	menu_string *	ff_mstrings;
	menu_coord	ff_x;
	menu_coord	ff_y;
	menu_coord	ff_width;
	char *		ff_data;
	short		ff_datasize;
	int (*		ff_lex)();
	int (*		ff_prefunc)();
	pointer		ff_prearg;
	int (*		ff_postfunc)();
	pointer		ff_postarg;
	form_field *	ff_next;
};


/*
 *	Definition of a form map:
 *
 *		fm_obj		- pointer to the object in the form
 *		fm_func		- function to manipulate the object
 *		fm_prev		- pointer to the previous mapped object
 *		fm_next		- pointer to the next mapped object
 */
struct form_map_t {
	pointer		fm_obj;
	int (*		fm_func)();
	form_map *	fm_prev;
	form_map *	fm_next;
};


/*
 *	Definition of a form noecho:
 *
 *		m_type		- type of menu object
 *		m_help		- pointer to the help function
 *		fne_active	- is this noecho active?
 *		fne_name	- name of this noecho
 *		fne_mstrings	- menu strings associated with this noecho
 *		fne_x		- starting x-coordinate of this noecho
 *		fne_y		- starting y-coordinate of this noecho
 *		fne_data	- pointer to data buffer for this noecho
 *		fne_datasize	- size of buffer 'fne_data'
 *		fne_prefunc	- pointer to pre-function.  The following
 *				  return values are expected:
 *					 1 -> proceed with getting data
 *					 0 -> return with out getting data
 *					-1 -> return with error condition
 *		fne_prearg	- argument for pre-function
 *		fne_postfunc	- pointer to post-function
 *					 1 -> data is valid
 *					 0 -> data is invalid try again
 *					-1 -> return with error condition
 *		fne_postarg	- argument for post-function
 *		fne_next	- pointer to next noecho
 */
struct form_noecho_t {
	int		m_type;
	int (*		m_help)();
	char		fne_active;
	char *		fne_name;
	menu_string *	fne_mstrings;
	menu_coord	fne_x;
	menu_coord	fne_y;
	char *		fne_data;
	short		fne_datasize;
	int (*		fne_prefunc)();
	pointer		fne_prearg;
	int (*		fne_postfunc)();
	pointer		fne_postarg;
	form_noecho *	fne_next;
};


/*
 *	Definition of a form radio:
 *
 *		m_type		- type of menu object
 *		m_help		- pointer to the help function
 *		fr_active	- is this radio active?
 *		fr_name		- name of this radio
 *		fr_mstrings	- strings associated with this radio
 *		fr_buttons	- the buttons associated with this radio
 *		fr_pressed	- the currently pressed button
 *		fr_codeptr	- pointer to pressed button's code
 *		fr_prefunc	- pointer to pre-function.  The following
 *				  return values are expected:
 *					 1 -> proceed with getting button
 *					 0 -> return with out getting button
 *					-1 -> return with error condition
 *		fr_prearg	- argument for pre-function
 *		fr_postfunc	- pointer to post-function
 *					 1 -> button is valid
 *					 0 -> button is invalid try again
 *					-1 -> return with error condition
 *		fr_postarg	- argument for post-function
 *		fr_next		- pointer to next radio
 */
struct form_radio_t {
	int		m_type;
	int (*		m_help)();
	char		fr_active;
	char *		fr_name;
	menu_string *	fr_mstrings;
	form_button *	fr_buttons;
	form_button *	fr_pressed;
	int *		fr_codeptr;
	int (*		fr_prefunc)();
	pointer		fr_prearg;
	int (*		fr_postfunc)();
	pointer		fr_postarg;
	form_radio *	fr_next;
};


/*
 *	Definition of a form yes/no question:
 *
 *		m_type		- type of menu object
 *		m_help		- pointer to the help function
 *		fyn_active	- is this yes/no active?
 *		fyn_name	- name of this yes/no question
 *		fyn_mstrings	- menu strings associated with this yes/no
 *		fyn_answer	- current answer
 *		fyn_answerp	- pointer to current answer
 *		fyn_x		- x-coordinate of this yes/no
 *		fyn_y		- y-coordinate of this yes/no
 *		fyn_prefunc	- pointer to pre-function.  The following
 *				  return values are expected:
 *					 1 -> proceed with getting answer
 *					 0 -> return with out getting answer
 *					-1 -> return with error condition
 *		fyn_prearg	- argument for pre-function
 *		fyn_nofunc	- function to invoke if no is selected.  The
 *				  following return values are expected:
 *					 1 -> answer was valid
 *					 0 -> answer was not valid, try again
 *					-1 -> return with error condition
 *		fyn_noarg	- argument for no function
 *		fyn_yesfunc	- function to invoke if yes is selected.  The
 *				  following return values are expected:
 *					 1 -> answer was valid
 *					 0 -> answer was not valid, try again
 *					-1 -> return with error condition
 *		fyn_yesarg	- argument for yes function
 *		fyn_next	- pointer to next yes/no
 */
struct form_yesno_t {
	int		m_type;
	int (*		m_help)();
	char		fyn_active;
	char *		fyn_name;
	menu_string *	fyn_mstrings;
	char		fyn_answer;
	char *		fyn_answerp;
	menu_coord	fyn_x;
	menu_coord	fyn_y;
	int (*		fyn_prefunc)();
	pointer		fyn_prearg;
	int (*		fyn_nofunc)();
	pointer		fyn_noarg;
	int (*		fyn_yesfunc)();
	pointer		fyn_yesarg;
	form_yesno *	fyn_next;
};


/*
 *	Definition of a menu:
 *
 *		m_type		- type of menu object
 *		m_help		- pointer to the help function
 *		m_active	- is this menu active?
 *		m_name		- name of this menu
 *		m_mstrings	- menu strings associated with this menu
 *		m_files		- files associated with this menu
 *		m_items		- items associated with this menu
 *		m_selected	- ptr to currently selected item
 *		m_ptr		- pointer to the current item
 *		m_finish	- ptr to finish object
 */
struct menu_t {
	int		m_type;
	int (*		m_help)();
	int		m_active;
	char *		m_name;
	menu_string *	m_mstrings;
	menu_file *	m_files;
	menu_item *	m_items;
	menu_item *	m_selected;
	menu_item *	m_ptr;
	form_yesno *	m_finish;
};


/*
 *	Definition of a menu file:
 *
 *		m_type		- type of menu object
 *		m_help		- pointer to the help function
 *		mf_active	- is this menu file active?
 *		mf_name		- name of this menu file
 *		mf_mstrings	- menu strings associated with this file
 *		mf_begin_y	- beginning y-coordinate
 *		mf_begin_x	- beginning x-coordinate
 *		mf_end_y	- ending y-coordinate
 *		mf_end_x	- ending x-coordinate
 *		mf_gap		- inter-column gap
 *		mf_path		- path to file to display
 */
struct menu_file_t {
	int		m_type;
	int (*		m_help)();
	int		mf_active;
	char *		mf_name;
	menu_string *	mf_mstrings;
	menu_coord	mf_begin_y;
	menu_coord	mf_begin_x;
	menu_coord	mf_end_y;
	menu_coord	mf_end_x;
	menu_coord	mf_gap;
	char *		mf_path;
	menu_file *	mf_next;
};


/*
 *	Definition of a menu item:
 *
 *		m_type		- type of menu object
 *		m_help		- pointer to the help function
 *		mi_active	- is this menu item active?
 *		mi_name		- name of this menu item
 *		mi_mstrings	- menu strings associated with this menu item
 *		mi_x		- starting x-coordinate of this menu item
 *		mi_y		- starting y-coordinate of this menu item
 *		mi_prefunc	- pointer to pre-function.  The following
 *				  return values are expected:
 *					 1 -> proceed with getting item
 *					 0 -> return with out getting item
 *					-1 -> return with error condition
 *		mi_prearg	- argument for pre-function
 *		mi_func		- pointer to post-function
 *					 1 -> item is valid
 *					 0 -> item is invalid try again
 *					-1 -> return with error condition
 *		mi_arg		- argument for post-function
 *		mi_chkfunc	- pointer to check-function
 *					 1 -> item is valid
 *					 0 -> item is invalid try again
 *					-1 -> return with error condition
 *		mi_chkarg	- argument for check-function
 *		mi_chkvalue	- value from mi_chkfunc
 *		mi_prev		- pointer to previous item
 *		mi_next		- pointer to next item
 *		mi_menu		- pointer to the parent menu
 */
struct menu_item_t {
	int		m_type;
	int (*		m_help)();
	char		mi_active;
	char *		mi_name;
	menu_string *	mi_mstrings;
	menu_coord	mi_x;
	menu_coord	mi_y;
	int (*		mi_prefunc)();
	pointer		mi_prearg;
	int (*		mi_func)();
	pointer		mi_arg;
	int (*		mi_chkfunc)();
	pointer		mi_chkarg;
	int		mi_chkvalue;
	menu_item *	mi_prev;
	menu_item *	mi_next;
	menu *		mi_menu;
};


/*
 *	Definition of a menu string:
 *
 *		ms_active	- is this menu string active?
 *		ms_x		- starting x-coordinate of this menu string
 *		ms_y		- starting y-coordinate of this menu string
 *		ms_attribute	- display attribute for this menu string
 *		ms_data		- the string to display
 *		ms_next		- pointer to next menu string
 */
struct menu_string_t {
	char		ms_active;
	menu_coord	ms_x;
	menu_coord	ms_y;
	unsigned int	ms_attribute;
	char *		ms_data;
	menu_string *	ms_next;
};




/*
 *	Function return types:
 */
extern	form_yesno *	add_confirm_obj();
extern	form_yesno *	add_finish_obj();
extern	form_button *	add_form_button();
extern	form_field *	add_form_field();
extern	form_noecho *	add_form_noecho();
extern	form_radio *	add_form_radio();
extern	form_yesno *	add_form_yesno();
extern	menu_file *	add_menu_file();
extern	menu_item *	add_menu_item();
extern	menu_string *	add_menu_string();
extern	int		ckf_abspath();
extern	int		ckf_empty();
extern	int		ckf_ether_aton();
extern	int		ckf_inet_addr();
extern	int		ckf_int();
extern	int		ckf_long();
extern	int		ckf_netabs();
extern	int		ckf_netpath();
extern	int		ckf_octal();
extern	int		ckf_relpath();
extern	int		ckf_uint();
extern	void		clear_form();
extern	void		clear_form_button();
extern	void		clear_form_field();
extern	void		clear_form_noecho();
extern	void		clear_form_radio();
extern	void		clear_form_yesno();
extern	void		clear_menu();
extern	void		clear_menu_file();
extern	void		clear_menu_item();
extern	void		clear_menu_string();
extern	form *		create_form();
extern	menu *		create_menu();
extern	void		display_form();
extern	void		display_form_button();
extern	void		display_form_field();
extern	void		display_form_noecho();
extern	void		display_form_radio();
extern	void		display_form_yesno();
extern	void		display_item_status();
extern	void		display_menu();
extern	void		display_menu_file();
extern	void		display_menu_item();
extern	void		display_menu_string();
extern	void		end_menu();
extern	form_button *	find_form_button();
extern	form_field *	find_form_field();
extern	form_noecho *	find_form_noecho();
extern	form_radio *	find_form_radio();
extern	form_yesno *	find_form_yesno();
extern	menu_file *	find_menu_file();
extern	menu_item *	find_menu_item();
extern	void		form_help_screen();
extern	void		free_form();
extern	void		free_form_button();
extern	void		free_form_field();
extern	void		free_form_map();
extern	void		free_form_noecho();
extern	void		free_form_radio();
extern	void		free_form_yesno();
extern	void		free_menu();
extern	void		free_menu_file();
extern	void		free_menu_item();
extern	void		free_menu_string();
extern	int		get_form_field();
extern	int		get_form_noecho();
extern	int		get_form_radio();
extern	int		get_form_yesno();
extern	int		get_menu_item();
extern	void		help_screen_off();
extern	void		help_screen_on();
extern	void		init_menu();
extern	int		is_help_screen();
extern	int		lex_no_ws();
extern	void		menu_abort();
extern	void		menu_ack();
extern	char		menu_ask_num();
extern	int *		menu_alloc();
extern	int		menu_ask_yesno();
extern	menu_coord	menu_cols();
extern	int		menu_confirm_func();
extern	void		menu_flash_off();
extern	void		menu_flash_on();
extern	int		menu_goto_obj();
extern	void		menu_help_screen();
extern	int		menu_ignore_obj();
extern	menu_coord	menu_lines();
extern	void		menu_log();
extern	void		menu_mesg();
extern	int		menu_print();
extern	int		menu_repeat_obj();
extern	void		menu_sig_trap();
extern	int		menu_skip_io();
extern	void		off_form();
extern	void		off_form_button();
extern	void		off_form_field();
extern	void		off_form_noecho();
extern	void		off_form_radio();
extern	void		off_form_shared();
extern	void		off_form_yesno();
extern	void		off_menu();
extern	void		off_menu_file();
extern	void		off_menu_item();
extern	void		off_menu_string();
extern	void		on_form();
extern	void		on_form_button();
extern	void		on_form_field();
extern	void		on_form_noecho();
extern	void		on_form_radio();
extern	void		on_form_shared();
extern	void		on_form_yesno();
extern	void		on_menu();
extern	void		on_menu_file();
extern	void		on_menu_item();
extern	void		on_menu_string();
extern	void		paint_menu();
extern	char		read_menu_char();
extern	void		redisplay();
extern	void		set_form_map();
extern	void		set_menu_item();
extern	void		set_menu_log();
extern	void		set_menu_term();
extern	int		use_form();
extern	int		use_menu();
extern	void		_menu_mesg();




/*
 *	Global variable declarations:
 */
extern	pointer		_current_fm;		/* ptr to current form/menu */
extern	pointer		_current_obj;		/* pointer to current object */
extern	int		_menu_backward;		/* backward char pressed? */
extern	int		_menu_forward;		/* forward char pressed? */
extern	int		_menu_init_done;	/* was menu init done? */
#ifdef MENU_SIGNALS
extern	void (*		_menu_signals[])();	/* saved signal values */
#endif

#endif _MENU_H_
