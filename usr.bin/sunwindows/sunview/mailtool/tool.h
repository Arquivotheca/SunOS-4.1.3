/*      @(#)tool.h 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - tool global variables
 */

extern Frame    mt_frame;	/* the mailtool frame */
extern Textsw   mt_headersw;	/* the header subwindow */
extern Panel    mt_cmdpanel;	/* the command panel */
extern int      mt_cmdpanel_fd;	/* the command panel subwindow fd */
extern Textsw   mt_msgsw;	/* the mail message subwindow */
extern Panel	mt_replypanel;
extern Textsw	mt_replysw;
extern Frame	mt_cd_frame;
extern Menu	mt_open_menu_pullright;
extern Menu_item	mt_open_menu_item;

extern	Rect	screenrect;

extern struct icon *mt_icon_ptr;
extern struct icon mt_unknown_icon, mt_mail_icon, mt_nomail_icon;
extern struct icon mt_busy_icon, mt_empty_letter_icon, mt_composing_icon;
extern struct icon mt_replying_icon;

extern Pixrect *mt_newmail_pr, *mt_nomail_pr, *mt_trash_pr, *mt_trashfull_pr;

extern Pixfont	*mt_font;	/* the default font */

extern int      numWindows;	/* current number of frames + subwindows */
extern int	charheight, charwidth, padding;
extern int	mt_cmdlines;
extern int	mt_popuplines;
extern int	mt_idle;		/* closed, not processing mail */
extern int      mt_iconic;	/* specifies whether tool is to be created
				 * out iconic or not */
extern int      mt_destroying;	/* true if tool is in process of being
				 * destroyed */
extern int	mt_debugging;
extern int	mt_3x_compatibility;
extern int	mt_41_features;
extern int	mt_use_images;
extern int	mt_use_fields;
extern int	mt_always_use_popup;
extern int	mt_bells;	/* number of bells */
extern int	mt_flashes;	/* number of flashes */
extern int	mt_retained;	/* true if retained text windows in use */
extern int	mt_nomail;		/* no mail in current folder */
extern int	mt_last_sel_msg;	/* last selected message */
extern int      mt_system_mail_box;	/* true if current folder is system
					 * mail box */
extern u_long	last_event_time;	/* time stamp for last user operation */

extern char    *mt_wdir;	/* Mail's working directory */
extern char    *mt_load_from_folder;
extern char    *mt_command_summary;
extern char    *mt_namestripe, *mt_namestripe_left, *mt_namestripe_right;
extern int	mt_namestripe_width;
extern time_t	mt_msg_time;		/* time msgfile was last written */

extern Panel_item	mt_show_item, mt_del_item, mt_print_item, mt_state_item;
extern Panel_item	mt_compose_item, mt_folder_item, mt_others_item;
extern Panel_item	mt_others_label_item, mt_file_item, mt_info_item;
extern Panel_item	mt_deliver_item, mt_cancel_item;
extern Panel_item	mt_dir_item, mt_pre_item;
extern Panel_item	mt_cycle_item;

extern Menu	mt_folder_menu, mt_file_menu;

enum mt_Panel_Style {mt_3DImages, mt_New, mt_Old};
enum mt_Load_Messages {mt_At_Startup, mt_When_Opened, mt_When_Asked};
enum mt_Window_Type {mt_Frame, mt_Panel, mt_Text};
enum mt_Popup_Behavior	{mt_Disappear, mt_Stay_Up, mt_Close};

enum mt_Panel_Style 	mt_panel_style;
enum mt_Load_Messages	mt_load_messages;

struct reply_panel_data {
	Textsw		replysw;
	Frame		frame;
	Panel		reply_panel;
	Panel_item	deliver_item;
	Panel_item	cancel_item;
	Panel_item	cycle_item;
	char           *replysw_file;
	enum mt_Popup_Behavior	behavior;
	int		inuse;	/* 0 = available, 1 = inuse */
	int		normalized;	/* 0 = not normalized, 1 = normalized */
	struct reply_panel_data	*next_ptr;
};

struct panel_item_data {
	Menu            menu;
	Menu_item       menu_item;	/* the item, if any, that invoked
					 * this operation */
	int             column;		/* column number. if negative, count
					 * from right edge 
					 */
	int		width;		/* in columns */
	caddr_t         client_data;	/* for use by the individual item or
					 * its menu */
};				

typedef void    (*Void_proc) ();

struct Menu_value {
	Void_proc	notify_proc;	/* (Panel_item, Event) */
	int             mask;			/* shift mask */
};				
/*
 * a pointer to a struct of this type is the value returned by the menus
 * behind the buttons on the  mailtool control panel. The generic panel event
 * proc then calls the appropriate procedure after setting the shift mask. 
 */


/* Mailtool notify procs and event procs */

void	mt_abort_proc(), mt_auto_retrieve_mail_proc(), mt_cancel_proc();
void	mt_cd_proc(), mt_commit_proc();
void	mt_comp_proc(), mt_copy_proc(), mt_del_proc();
void	mt_deliver_proc(), mt_deliver_intact_proc(), mt_done_proc(); 
void	mt_folder_proc(), mt_3Dimages_style_proc();
void	mt_include_proc();
void	mt_mailrc_proc(), mt_misc_proc(), mt_next_proc(), mt_new_mail_proc();
void	mt_new_style_panel_proc(), mt_nop_proc(), mt_old_style_panel_proc();
void	mt_preserve_proc(), mt_prev_proc(), mt_print_proc(), mt_quit_proc();
void	mt_readdress_proc(), mt_reply_proc(), mt_replysw_proc();
void	mt_save_proc(), mt_show_proc(), mt_undel_proc();

void	mt_panel_event(), mt_call_cmd_proc(), mt_folder_event();
void	mt_file_event(), mt_log_event();

/* generate procs */

Menu	mt_cancel_gen_proc(), mt_compose_gen_proc(), mt_del_gen_proc();
Menu	mt_new_mail_gen_proc(), mt_panel_style_gen_proc(), mt_reply_gen_proc(); Menu	mt_save_gen_proc(), mt_undel_gen_proc(), mt_get_folder_menu();
Menu	mt_folder_menu_gen_proc();

/* miscellaneous routines shared by several modules */

Menu 	mt_create_menu_for_button(); 

void	mt_init_mailtool_defaults(), mt_parse_tool_args();
void	mt_create_old_style_panel(), mt_create_images_panel();
void	mt_create_3Dimages_panel(), mt_create_new_style_panel();
void	mt_create_cd_popup(), mt_destroy_cd_popup(), mt_replypanel_init();

struct reply_panel_data		*mt_create_reply_panel(), *mt_get_replysw();

int	mt_confirm();
int	nfds_avail();

void	mt_init_tool_storage(), mt_start_tool(), mt_add_menu_items(), mt_add_menu_item();
void	mt_start_timer(), mt_load_headers(), mt_append_headers();
void	mt_update_curmsg(), mt_update_headersw(), mt_update_msgsw();
void	mt_save_curmsg(), mt_set_curselmsg();
void	mt_update_info(), mt_del_header(), mt_ins_header(), mt_replace_header();
void	mt_set_namestripe_temp(), mt_set_namestripe_left(), mt_set_namestripe_right();
void	mt_restore_namestripe();
void	mt_set_namestripe_both(), mt_set_icon(), mt_announce_mail();
void	mt_warn(), mt_tool_is_busy(), mt_waitcursor(), mt_restorecursor();
void	mt_add_window(), mt_remove_window();
void	mt_init_filemenu(), mt_new_folder();
void	mt_start_reply(), mt_stop_reply(), mt_display_reply(), mt_move_input_focus(), mt_default_panel_event();
void	mt_trace(), mt_save_filename();

FILE	*mt_fopen();


