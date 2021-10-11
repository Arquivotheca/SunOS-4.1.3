#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)cmds.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - tool command handling
 */

/***************************************************************  
*******                                                  *******  
******  NOTE: please use the comment section at the end   ****** 
******        of the program to document source changes.  ******            
*******                                                  *******  
***************************************************************/ 
 
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <vfork.h>
#include <sunwindow/window_hs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sundev/kbd.h>
#include <fcntl.h>

#include <suntool/window.h>
#include <suntool/frame.h>
#include <suntool/panel.h>
#include <suntool/text.h>
#include <suntool/walkmenu.h>
#include <suntool/scrollbar.h>
#include <suntool/selection.h>
#include <suntool/selection_svc.h>
#include <suntool/selection_attributes.h>
#include <suntool/alert.h>

#include "glob.h"
#include "tool.h"

/* performance: global cache of getdtablesize() */
extern int dtablesize_cache;
#define GETDTABLESIZE() \
        (dtablesize_cache?dtablesize_cache:(dtablesize_cache=getdtablesize()))

#define	PRIMARY_PD_SELECTION	0x0010 
#define	PRIMARY_SELECTION	0x0001 

char    *strcpy(), *sprintf(), *strcat();

static char	*fields[] = {"|>recipients<|", "|>subject<|", "|>body of message<|", "|>other recipients<|", "|>blind carbon copies<|", 0};
static	char	*mt_get_command_string(), *mt_find_command_string();
Menu	mt_file_menu;		/* menu of most recent file names used.
				 * Behind File: panel item */


static void	mt_create_cmd_panel(), mt_do_del(), mt_do_deliver(); 
static void	mt_do_retrieve(), mt_do_save(), mt_set_nomail();
static int	mt_has_selection();
static void	mt_yield_all();

static Seln_holder	seln_holder;

/* notify procs, in alphabetical order */

/*
 * Abort the tool.
 */
/* ARGSUSED */
void
mt_abort_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	(void)win_release_event_lock(mt_cmdpanel_fd);
	mt_yield_all();

	mt_aborting = TRUE;
	if (event_ctrl_is_down(ie))
		mt_assign("expert", ""); /* XXX - force no confirm, just quit */
	(void)window_done((struct tool *)(LINT_CAST(mt_frame)));	/* Should free window */
}

/*
 * "Quietly" retrieves new mail, i.e.., does not scroll headersw or update
 * message sw. Does commit if more than specified number of messages have
 * been deleted. Used for auto-reading. See mt_itimer in tool.c 
 */
void
mt_auto_retrieve_mail_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	char           *p;

	p = mt_value("commitfrequency"); 
	mt_do_retrieve(item, ie, (!p || mt_del_count <= atoi(p)), TRUE);
}


/*
 * Cancel the current reply (or send, or forward).
 */
/* ARGSUSED */
void
mt_cancel_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	Panel           panel;
	struct reply_panel_data *ptr;
	char		scratch[256];

	panel = panel_get(item, PANEL_PARENT_PANEL);
	ptr = (struct reply_panel_data *)panel_get(panel, PANEL_CLIENT_DATA);

	if (!(event_ctrl_is_down(ie)) &&
		window_get(ptr->replysw, TEXTSW_MODIFIED) &&
		(ptr->behavior == mt_Stay_Up
			? !mt_confirm(ptr->replysw, FALSE, FALSE,
			"Confirm",
			"Do NOT clear window",
			"Are you sure you want to Clear window?",
			0)
			 : !mt_confirm(ptr->replysw, FALSE, FALSE,
			"Confirm",
			"Do NOT cancel window",
			"Are you sure you want to Cancel window?",
			 0)
	   ))
		return;

	if (mt_value("save")) {
		char	*s;
		FILE	*f;

		s = mt_value("DEAD");
		/* 
		 * (Bug# 1014141 fix) added code to recognise
		 * "/dev/null" and not to save to it. 
		 */ 
		if (strcmp(s,"/dev/null") != 0) 
		{
		  if (s == NULL || *s == '\0')
			s = "~/dead.letter";
		  mt_full_path_name(s, scratch);
		  if (f = mt_fopen(scratch, "w")) {
		  	/* check to see if can write to this file */
			(void) fclose(f);
			(void) textsw_store_file(ptr->replysw, scratch, 0, 0);
		  }
		}
	}
	textsw_reset(ptr->replysw, 0, 0);
	(void) unlink(ptr->replysw_file); 
	ptr->inuse = FALSE;
	if (ptr->frame != mt_frame)
		(void) window_set(ptr->frame,
			FRAME_ICON, &mt_empty_letter_icon,
			0);
	if (ptr->behavior == mt_Disappear)
		mt_stop_reply(ptr);
	else if (ptr->behavior == mt_Close)
		(void) window_set(ptr->frame, FRAME_CLOSED, TRUE, 0);
}

/*
/*
 * Change Mail's working directory.
 */
/* ARGSUSED */
void
mt_cd_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	char           *dir;

	if (mt_dir_item == NULL) {
		mt_create_cd_popup();
		return;
	}
	dir = (char *)panel_get_value(mt_dir_item);
	if (dir == NULL)
		dir = "~";
	if (strcmp(dir, mt_wdir) == 0)
		return;
	mt_mail_cmd("echo \"%s\"", dir);
	mt_info[strlen(mt_info) - 1] = '\0';
	if (chdir(mt_info) < 0) {
		extern int errno;
		extern int sys_nerr;
		extern char *sys_errlist[];

		if (errno < sys_nerr)
			(void) sprintf(mt_info, "cd: %s", sys_errlist[errno]);
		else
			(void) sprintf(mt_info, "cd: errno=%d", errno);
		mt_warn(mt_frame, mt_info, 0);
	} else {
		mt_set_mailwd(mt_info);
		(void) strcpy(mt_wdir, dir);
		mt_update_info("");
	}
	if (mt_cd_frame)
		mt_destroy_cd_popup();
}

/*
 * Commit the current set of changes.
 */
/* ARGSUSED */
void
mt_commit_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	static u_long   last;
	u_long          when;

	when = event_time(ie).tv_sec;
	if (last != 0 && when < last) {
		/*
		 * this event occurred BEFORE the last commit operation
		 * completed, i.e., user bounced on the key 
		 */
		last = when;
		return;
	}
	if (mt_nomail && mt_delp == NULL) {
		mt_warn(mt_frame, "No Mail", 0);
		return;
	}
	(void)win_release_event_lock(mt_cmdpanel_fd);
	mt_yield_all();	/* use mt_yield_all() to save shelf */
	if (strcmp(mt_folder, mt_mailbox) == 0)
		mt_new_folder("%", FALSE, FALSE);
	else
		mt_new_folder(mt_folder, FALSE, FALSE);
	last = mt_current_time();

}

/* Compose a new mail message.
 */
void
mt_comp_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	struct reply_panel_data *ptr;
	int             msg, orig;
	int		lower_context;

	if (!(ptr = mt_get_replysw(item)))
		return;
	msg = 0;
	orig = event_ctrl_is_down(ie);
	if (orig && (msg = mt_get_curselmsg(0)) == 0)
		return;
	mt_save_curmsg();
	if (!(mt_compose_msg(msg, ptr->replysw_file,
		mt_value("askcc")
			? !event_shift_is_down(ie)
			: event_shift_is_down(ie),
		orig)))
		return;
	mt_start_reply(ptr);

	lower_context = (int)window_get(ptr->replysw, TEXTSW_LOWER_CONTEXT);
	(void) window_set(ptr->replysw,
		TEXTSW_LOWER_CONTEXT, -1,	/* work around to
						 * suppress scrolling as
						 * you insert */
		TEXTSW_FILE_CONTENTS, ptr->replysw_file,
		TEXTSW_LOWER_CONTEXT, lower_context,
		0);

	(void) window_set(ptr->replysw, TEXTSW_INSERTION_POINT, 4, 0); 
		/* after "To: " */
	if (mt_use_fields) {
		Textsw_index             first, last_plus_one;

		first = (Textsw_index)window_get(ptr->replysw,
			TEXTSW_INSERTION_POINT);
		textsw_match_bytes(ptr->replysw,
			&first, &last_plus_one, "|>", 2, "<|", 2,
			TEXTSW_FIELD_FORWARD);
		(void) textsw_set_selection(ptr->replysw, first, last_plus_one,
			PRIMARY_SELECTION | PRIMARY_PD_SELECTION);
		(void) window_set(ptr->replysw, TEXTSW_INSERTION_POINT,
			last_plus_one, 0);

	} 
	mt_display_reply(ptr);
	if (ptr->frame != mt_frame)
		(void) window_set(ptr->frame,
			FRAME_ICON, &mt_composing_icon,
			0);
	if ((nfds_avail()) < 4) {
		/*
		 * currently, you
		 * need 4 (choke!) fds to invoke a filter, two for each pair
		 * of pipes 
		 */
		mt_warn(window_get(ptr->replysw, WIN_OWNER),
			"Warning: low on fds. Don't use text filters",
			"or the text extras menu in composition windows.",
			0);

	}
	if (mt_debugging) {
		(void) printf("replysw setup, #fds = %d\n", nfds_avail());
		(void) fflush(stdout);
	}
}


/*
 * Copy the selected message to the specified file.
 */
/* ARGSUSED */
void
mt_copy_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{

        mt_do_save(0, ie, mt_has_selection());
}

/* Delete the current message and display the next message.
 */
/* ARGSUSED */
void
mt_del_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	int             msg;
	int		has_selection;
	char           *p;

	if (mt_nomail) {
		mt_warn(mt_frame, "No Mail", 0);
		return;
	}
	if ((msg = mt_get_curselmsg(0)) == 0)
		return;
	has_selection = mt_has_selection();
	mt_yield_all();       /* use mt_yield_all() to save shelf */
	if ((p = mt_value("trash")) || msg != mt_curmsg) {
		/* no need to save current message if no trash folder */
		mt_save_curmsg();
		if (p)
			(void)mt_copy_msg(msg, p);
	}
	mt_do_del(msg, ie, has_selection);
}

/*
 * Actually send the current reply-type message.
 */
/* ARGSUSED */
void
mt_deliver_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{

	mt_do_deliver(item, ie);
}

/* Saves the shelf before before yeilding 

/*
 * Actually send the current reply-type message, but leave window intact.
 */
/* ARGSUSED */
void
mt_deliver_intact_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	mt_do_deliver(item, ie);
}


/*
 * Yield all selections to Selection Service.  Use when exiting or
 * undertaking a lengthy process where the program
 * will be unable to respond to requests concerning selections held.
 * 					
 * (bug# 1011496 & bug# 1009592 fix)
 * Routine also saves the current shelf (clipboard) with seln_hold_file(),
 * even if mailtool exits completely.
 */
static void
mt_yield_all()
{
	Seln_holder	holder;

	holder = seln_inquire(SELN_SHELF);
	if (holder.state != SELN_NONE) 	/* if clipboard has something */
	{
	  int 		fd;
	  Seln_request	*request;

	  /* get ASCII contents of shelf, limit 2000 bytes */
	  request = seln_ask(&holder, SELN_REQ_CONTENTS_ASCII, 0, 0);
	  if (request->status == SELN_FAILED) 
	    (void) fputs("Unable to get & save shelf contents\n", stderr);
	  else if ((fd = open(mt_clipboard, (O_RDWR | O_TRUNC))) != -1) 
	  {
	    /* 
	     * The 1st 4 bytes of the data buffer contain info to 
	     * identify that a shelf has been returned.  Write out
	     * the ASCII contents minus the 4 bytes into the opened
	     * file, then close the file.
	     */
	    (void) write(fd, &request->data[4], strlen(request->data)-4);
	    (void) close(fd); 
	    /* tell the selection service to use file contents for shelf */
	    if ((seln_hold_file(SELN_SHELF, mt_clipboard)) != SELN_SUCCESS)
	      (void) fprintf(stderr,"Cannot get shelf in %s\n",mt_clipboard);
	  }
	  else
	    (void) fprintf(stderr,"Cannot open %s to save shelf\n",mt_clipboard);
	}
	seln_yield_all();
}



/*
 * Done with the current folder, close the tool.
 */
/* ARGSUSED */
void
mt_done_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	char           *p;

	mt_save_curmsg();

	/*
	 * Since a commit has been made, check to see if the header array can
	 * be reduced in size.
	 */
	if ((mt_maxmsg-mt_del_count) <= (current_size-INCR_SIZE))
	{
	  if((mt_message=(struct msg *)realloc((struct msg *)mt_message, 
	     (unsigned)(sizeof(struct msg)*(current_size-INCR_SIZE)))) == NULL)
		{
		  	fprintf(stderr,"Realloc failed to decrement storage\n");
			exit(1);
		}
		else
			current_size -= INCR_SIZE;
	}

	(void) window_set(mt_frame, FRAME_CLOSED, TRUE, 0);
	(void) win_post_id(mt_frame, WIN_RESIZE, NOTIFY_IMMEDIATE);
	(void) win_post_id(mt_frame, WIN_REPAINT, NOTIFY_IMMEDIATE);
	mt_tool_is_busy(TRUE, "going idle...");
	if (mt_open_menu_item)
		(void) menu_set(mt_open_menu_item,
			MENU_PULLRIGHT, mt_open_menu_pullright,
			0);
	(void) win_release_event_lock(mt_cmdpanel_fd);
	mt_yield_all();
	if (!mt_idle_mail()) {	/* can fail if can't problems writing to /tmp */
		mt_tool_is_busy(FALSE);
		return;
	}
	mt_idle = TRUE;
	mt_load_from_folder = NULL;
	(void) strcpy(mt_folder, "[None]");
	mt_set_nomail();
	if (p = mt_value("trash"))
		mt_del_folder(p);
	mt_tool_is_busy(FALSE);

}

/*
 * Switch to the specified folder.
 */
/* ARGSUSED */
void
mt_folder_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	char           *file;
	static u_long   last;
	u_long          when;

	when = event_time(ie).tv_sec;
	/*
	 * (Bug# 1009020 fix) when mailtool is iconic and the user opens
	 * the tool into a particular folder, the above line will set to
	 * "when = 0".  I added that check below so mailtool will not 
	 * abort the loading of a folder from the iconic state.
	 */ 
	if (when != 0 && last != 0 && when < last ) {
		/*
		 * this event occurred BEFORE the last folder operation
		 * completed, i.e., user bounced on the key 
		 */
		last = when;
		return;
	}
	(void)win_release_event_lock(mt_cmdpanel_fd);
	file = (char *)panel_get_value(mt_file_item);
	if (file == NULL || *file == '\0') {
		mt_warn(mt_frame, "Must specify file name.", 0);
		return;
	}
	mt_save_filename(file);
	mt_new_folder(file, FALSE, FALSE);
	last = mt_current_time();
}

/*
 * Include the selected message in the message composition window.
 */
/* ARGSUSED */
void
mt_include_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	Frame           frame;
	struct reply_panel_data *ptr;
	int             msg;
	Textsw_status	status;

	if (mt_nomail) {
		mt_warn(mt_frame, "No Mail", 0);
		return;
	} else if ((msg = mt_get_curselmsg(0)) == 0) 
		/* warning displayed in mt_get_curselmsg */
		return;

	ptr = (struct reply_panel_data *)panel_get(
		panel_get(item, PANEL_PARENT_PANEL),
		PANEL_CLIENT_DATA);

	frame = window_get(ptr->replysw, WIN_OWNER);
	mt_save_curmsg();
	if (!(mt_include_msg(msg, mt_scratchfile, event_ctrl_is_down(ie))))
		return;
	(void) window_set(ptr->replysw,
		TEXTSW_STATUS, &status,
		TEXTSW_INSERT_FROM_FILE, mt_scratchfile,
		0);
	if (status == TEXTSW_STATUS_OUT_OF_MEMORY) 
		mt_warn(frame,
			"Insertion failed: memory buffer exceeded.",
			 "You can get around this by undoing the Include,",
			 "using the text menu to store the contents of the",
			 "message composition window to a file, and then",
			 "redoing the include. Or, you can enlarge the size",
			 "of the memory buffer by using defaultsedit to",
			 "increase the default value of \'memorymaximum\'",
			 "quitting mailtool, and starting again.",
			0);
	/*
	 * this copies the bytes, so don't have to worry about overwriting
	 * scratch file if more than one include. 
	 */
	(void) unlink(mt_scratchfile);
}

/*
 * Reread .mailrc to pick up any changes.
 */
/* ARGSUSED */
void
mt_mailrc_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
/* not used - hala
	struct reply_panel_data *ptr, *new;
*/

	mt_mail_cmd("source %s", mt_value("MAILRC"));
	mt_get_vars(0);
	mt_start_timer();
	mt_init_filemenu();
	mt_folder_menu = NULL;	/* will be rebuilt next time is used. */
/* not used - hala 
	ptr = (struct reply_panel_data *)panel_get(
		mt_cmdpanel, PANEL_CLIENT_DATA);
*/
	mt_start_timer();	/* in case user changed interval */
	mt_init_mailtool_defaults();
}

/*
 * Display the next message.
 */
/* ARGSUSED */
void
mt_next_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	int             nextmsg;
	int		has_selection;

	if (mt_nomail) {
		mt_warn(mt_frame, "No Mail", 0);
		return;
	}
	/* mt_save_curmsg(); XXX */

	/* give warning msg if no mssg selected in header subwindow */
	if ((nextmsg = mt_get_curselmsg(0)) == 0)
		return;

	has_selection = mt_has_selection();
	if (event_shift_is_down(ie)) {
		int savescandir;

		savescandir = mt_scandir;
		mt_scandir = -mt_scandir;	/* reverse scan direction */
		nextmsg = mt_next_msg(mt_curmsg);
		if (mt_scandir == -savescandir)	/* if no change, put it back */
			mt_scandir = savescandir;
	} else {
		if (mt_message[mt_curmsg].m_deleted)
			mt_curmsg = mt_next_msg(mt_curmsg);
		nextmsg = mt_next_msg(mt_curmsg);
	}
	mt_update_msgsw(nextmsg, TRUE, TRUE, TRUE, has_selection);
}


/*
 * Handles "new mail" and "done"
 */
void
mt_new_mail_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	mt_do_retrieve(item, ie, TRUE, FALSE);
}

/* ARGSUSED */
void
mt_new_style_panel_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	mt_create_cmd_panel(mt_New);
}

/*
 * Procedure that does nothing. Lives behind panel items that only have menus.
 */
/* ARGSUSED */
void
mt_nop_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
/* nop */
}

/* ARGSUSED */
void
mt_old_style_panel_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	mt_create_cmd_panel(mt_Old);
}

/*
 * Preserve the selected message.
 */
/* ARGSUSED */
void
mt_preserve_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	int	msg;

	if (mt_nomail) {
		mt_warn(mt_frame, "No Mail", 0);
		return;
	}
	if ((msg = mt_get_curselmsg(0)) == 0)
		return;
	mt_preserve_msg(msg);
}

/*
 * Display the previous message.
 */
/* ARGSUSED */
void
mt_prev_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{

	event_set_shiftmask(ie, SHIFTMASK);
	mt_next_proc(item, ie);
}


/*
 * Print the selected message on a printer.
 */
/* ARGSUSED */
void
mt_print_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	int             msg;

	if (mt_nomail) {
		mt_warn(mt_frame, "No Mail", 0);
		return;
	}
	if ((msg = mt_get_curselmsg(0)) == 0)
		return;
	(void) unlink(mt_printfile);
	if (msg == mt_curmsg)
		mt_save_curmsg();
	(void)mt_copy_msg(msg, mt_printfile);
	if (vfork() == 0) {
		register int i;
		register char *p;

		for (i = GETDTABLESIZE(); i > 2; i--)
			(void) close(i);
		(void) close(0);
		(void) open(mt_printfile, 0);
		(void) unlink(mt_printfile);
		if ((p = mt_value("printmail")) != NULL)
			execl("/bin/sh", "sh", "-c", p, 0);
		else
			execl("/usr/ucb/lpr", "lpr", "-p", 0);
		exit(-1);
	}
}

/*
 * Quit the tool.
 */
/* ARGSUSED */
void
mt_quit_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{

	(void)win_release_event_lock(mt_cmdpanel_fd);
	mt_yield_all();
	if (event_ctrl_is_down(ie))
		mt_assign("expert", "");
		/* XXX - force no confirm, just quit */
	(void)window_done((struct tool *)(LINT_CAST(mt_frame)));
}

/*
 * Readdress.
 */
/* ARGSUSED */
void
mt_readdress_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	Panel		panel;
	struct reply_panel_data *ptr;
	int		lower_context;

	if (!mt_compose_msg(0, mt_scratchfile, mt_value("askcc"), FALSE))
		return;

	panel = panel_get(item, PANEL_PARENT_PANEL);
	ptr = (struct reply_panel_data *)panel_get(panel, PANEL_CLIENT_DATA);

	lower_context = (int)window_get(ptr->replysw, TEXTSW_LOWER_CONTEXT);
	(void) window_set(ptr->replysw,
		TEXTSW_INSERTION_POINT, 0,
		TEXTSW_LOWER_CONTEXT, -1,	/* work around to
						 * suppress scrolling as
						 * you insert */
		TEXTSW_INSERT_FROM_FILE, mt_scratchfile,
		TEXTSW_LOWER_CONTEXT, lower_context,
		TEXTSW_INSERTION_POINT, 4,	/* after "To: " */ 
		0);
	if (mt_use_fields) {
		Textsw_index             first, last_plus_one;

		first = (Textsw_index)window_get(ptr->replysw,
			TEXTSW_INSERTION_POINT);
		textsw_match_bytes(ptr->replysw,
			&first, &last_plus_one, "|>", 2, "<|", 2,
			TEXTSW_FIELD_FORWARD);
		(void) textsw_set_selection(ptr->replysw, first, last_plus_one,
			PRIMARY_SELECTION | PRIMARY_PD_SELECTION);
		(void) window_set(ptr->replysw, TEXTSW_INSERTION_POINT,
			last_plus_one, 0);
	} 
	mt_move_input_focus(ptr->replysw);
	if (ptr->frame != mt_frame)
		(void) window_set(ptr->frame,
			FRAME_ICON, &mt_composing_icon,
			0);
}

/*
 * Reply to the selected message.
 */
/* ARGSUSED */
void
mt_reply_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	struct reply_panel_data *ptr;
	int             msg, lower_context;

	if (mt_nomail) {
		mt_warn(mt_frame, "No Mail", 0);
		return;
	}
	else if ((msg = mt_get_curselmsg(0)) == 0)
		return;
	else if (!(ptr = mt_get_replysw(item)))
		return;
	mt_save_curmsg();
	if (!(mt_reply_msg(msg, ptr->replysw_file, event_shift_is_down(ie),
	    event_ctrl_is_down(ie))))
	    	return;
	mt_start_reply(ptr);

	lower_context = (int)window_get(ptr->replysw, TEXTSW_LOWER_CONTEXT);
	(void) window_set(ptr->replysw,
		TEXTSW_LOWER_CONTEXT, -1,	/* work around to
						 * suppress scrolling as
						 * you insert */
		TEXTSW_FILE_CONTENTS, ptr->replysw_file, 
		TEXTSW_LOWER_CONTEXT, lower_context,
		TEXTSW_INSERTION_POINT, TEXTSW_INFINITY,
		0);
	if (mt_use_fields) {
		Textsw_index	first, last_plus_one;

		first = (Textsw_index) window_get(ptr, TEXTSW_INSERTION_POINT);
		textsw_match_bytes(ptr->replysw,
			&first, &last_plus_one, "|>", 2, "<|", 2,
			TEXTSW_FIELD_BACKWARD);
		(void) textsw_set_selection(ptr->replysw, first, last_plus_one,
			PRIMARY_SELECTION | PRIMARY_PD_SELECTION);
		(void) window_set(ptr->replysw, TEXTSW_INSERTION_POINT,
			last_plus_one, 0);
 	} 
	mt_display_reply(ptr);

	/* (Bug# 1010744 fix) make sure beguinning of msg is on screen */
	textsw_possibly_normalize(ptr->replysw, 0);

	if (ptr->frame != mt_frame)
		(void) window_set(ptr->frame,
			FRAME_ICON, &mt_replying_icon,
			0);
	if (mt_debugging) {
		(void) printf("replysw setup, #fds = %d\n", nfds_avail());
		(void) fflush(stdout);
	}
	if ((nfds_avail()) < 4) {
		/*
		 * currently, you
		 * need 4 (choke!) fds to invoke a filter, two for each pair
		 * of pipes 
		 */
		mt_warn(window_get(ptr->replysw, WIN_OWNER),
			"Warning: low on fds. Don't use text filters",
			"or the text extras menu in composition windows.",
			0);
	}
}

/*
 * toggle permanent/temporary.
 */
/* ARGSUSED */
void
mt_replysw_proc(item, value, ie)
	Panel_item      item;
	int             value;
	Event          *ie;
{
	Panel           reply_panel;
	struct reply_panel_data *ptr;

	reply_panel = panel_get(item, PANEL_PARENT_PANEL);
	ptr = (struct reply_panel_data *)panel_get(
		reply_panel, PANEL_CLIENT_DATA);
	ptr->behavior = (enum mt_Popup_Behavior) value;
	mt_replypanel_init(reply_panel);
}

/*
 * Save the selected message in the specified file.
 */
/* ARGSUSED */
void
mt_save_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{

	mt_do_save(1, ie, mt_has_selection());
}

/*
 * Display message specified by current selection.
 * Selection must be numeric and we display the message
 * with that number.
 */
/* ARGSUSED */
void
mt_show_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	int             msg;

	if (mt_nomail) {
		mt_warn(mt_frame, "No Mail", 0);
		return;
	}
	if ((msg = mt_get_curselmsg(0)) == 0)
		return;
	mt_update_msgsw(msg, (event_shift_is_down(ie)) == 0,
		TRUE, FALSE, FALSE);
}


/*
 * Undelete the most recently deleted message.
 */
/* ARGSUSED */
void
mt_undel_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	int		has_selection;
	int             msg;

	if (mt_delp == NULL) {
		mt_warn(mt_frame, "No deleted messages.", 0);
		return;
	}
	has_selection = mt_has_selection();
	msg = mt_delp - mt_message;	 /* the message number */
	mt_save_curmsg();
	mt_undel_msg(msg);
	mt_delp = mt_delp->m_next;
	if (mt_delp == NULL && mt_del_item) { /* last deleted message */
		struct panel_item_data *p;
		if (mt_panel_style == mt_3DImages)
			(void) panel_set(mt_del_item,
				PANEL_LABEL_IMAGE, mt_trash_pr,
		 		0);
		p = (struct panel_item_data *)panel_get(
			mt_del_item, PANEL_CLIENT_DATA);
		(void) menu_set(menu_get(p->menu, MENU_NTH_ITEM, 2),
			MENU_INACTIVE, TRUE, 0);
	}
	mt_ins_header(mt_message[msg].m_start, mt_message[msg].m_header);
	mt_curmsg = msg;
	if (mt_nomail) {
		mt_nomail = FALSE;
		mt_update_msgsw(mt_curmsg, TRUE, FALSE, TRUE, has_selection);
	} else if (mt_value("autoprint"))
		mt_update_msgsw(mt_curmsg, TRUE, FALSE, TRUE, has_selection);
	if (!mt_3x_compatibility) {
		(void)sprintf(mt_info, "%d deleted message%s", mt_del_count, 
			mt_del_count != 1 ? "s" : "");
		mt_update_info(mt_info);
		mt_set_namestripe_right(mt_info, TRUE);
	}
}


/* ARGSUSED */
void
mt_3Dimages_style_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	if (!mt_use_images) 
		mt_warn(mt_frame,
			"This instance of mailtool was not compiled with -DUSE_IMAGES.",
			"No images are available.",
			0);
	else
		mt_create_cmd_panel(mt_3DImages);
}

static void
mt_create_cmd_panel(style)
	enum mt_Panel_Style style;
{
	char            file_item[128], info_item[128];
	struct reply_panel_data *ptr;

	ptr = (struct reply_panel_data *)panel_get(mt_cmdpanel,
		PANEL_CLIENT_DATA);

	/* get file & info string if it exists */
	(void) strcpy(file_item, (char *)panel_get_value(mt_file_item));
	if (mt_info_item != NULL)
		(void) strcpy(info_item, (char *)panel_get_value(mt_info_item));

	mt_remove_window(mt_cmdpanel);
	(void) window_destroy(mt_cmdpanel);
	mt_info_item = NULL;
	if (style == mt_3DImages)
		mt_create_3Dimages_panel();
	else {
		mt_del_item = NULL;
		if (style == mt_New) 
			mt_create_new_style_panel();
		else if (style == mt_Old) {
			mt_create_old_style_panel();
			mt_set_namestripe_right("", TRUE);
		}
	}
	mt_panel_style = style;
	if (!mt_always_use_popup) {
		/* get rid of old reply panel & sw */
		mt_remove_window(mt_replypanel);
		(void) window_destroy(mt_replypanel);
		mt_remove_window(mt_replysw);
		(void) window_destroy(mt_replysw);

		/* create new reply panel & sw */
		ptr = mt_create_reply_panel(mt_frame);
		if (ptr == NULL)
		{
		   fprintf(stderr, "Unable to create reply panel in mt_create_cmd_panel\n");
		   exit(1);
		}
		mt_replypanel = ptr->reply_panel;
		mt_replysw = ptr->replysw;
	}
	(void) panel_set(mt_cmdpanel, PANEL_CLIENT_DATA, ptr, 0);
	(void) panel_set(mt_file_item, PANEL_VALUE, file_item,
		PANEL_PAINT, PANEL_NONE, 0);
	if (mt_info_item != NULL)
		(void) panel_set(mt_info_item, PANEL_VALUE, info_item,
			PANEL_PAINT, PANEL_NONE, 0);
	(void) window_set(mt_cmdpanel,
		WIN_X, 0,
		WIN_BELOW, mt_headersw,
		0);
	(void) window_set(mt_msgsw,
		WIN_X, 0,
		WIN_BELOW, mt_cmdpanel,
		0);
	if (mt_cd_frame)
		mt_destroy_cd_popup();
}

/*
 * Delete a message.
 * Called by "delete" and "save".
 */
/* ARGSUSED */
static void
mt_do_del(msg, ie, has_selection)
	int             msg;
	Event          *ie;
	int		has_selection;
{
	int             display_next;	/* display next msg after delete? */
	int             nmsg;		/* new message after delete */

	mt_del_header(mt_message[msg].m_start, mt_message[msg+1].m_start);
	mt_del_msg(msg);
	if (mt_delp == NULL && mt_del_item) {
		/*
		 * this is the first deleted message. mt_del_item is NULL for
		 * old style panels 
		 */
		struct panel_item_data *p;
		if (mt_panel_style == mt_3DImages)
			(void) panel_set(mt_del_item,
				PANEL_LABEL_IMAGE, mt_trashfull_pr,
		 		0);
		p = (struct panel_item_data *)panel_get(
			mt_del_item, PANEL_CLIENT_DATA);
		(void) menu_set(menu_get(p->menu, MENU_NTH_ITEM, 2),
			MENU_INACTIVE, FALSE, 0);
	}
	mt_message[msg].m_next = mt_delp;
	mt_delp = &mt_message[msg];

	display_next = event_ctrl_is_down(ie) ?
	    mt_value("autoprint") == NULL : mt_value("autoprint") != NULL;

	if (msg == mt_curmsg || display_next) {
		if (event_shift_is_down(ie)) {
			int savescandir;

			savescandir = mt_scandir;
			mt_scandir = -mt_scandir;	/* reverse scan
							 * direction */
			nmsg = mt_next_msg(msg);
			if (mt_scandir == -savescandir)
				mt_scandir = savescandir;
		} else
			nmsg = mt_next_msg(msg);
		if (nmsg == 0)
			mt_set_nomail();
		else
			if (display_next && mt_curmsg != nmsg)
				mt_update_msgsw(nmsg, TRUE, FALSE,
					TRUE, has_selection);
			else {
				if (msg == mt_curmsg)
					textsw_reset(mt_msgsw, 0, 0);
				mt_update_headersw(nmsg);
				/*
				 * scrolls header subwindow if necessary to
				 * bring next/previous message into view
				 */
				mt_set_curselmsg(nmsg); 
			}
	} else { 
		nmsg = mt_next_msg(msg);
		mt_update_headersw(nmsg);
				/*
				 * scrolls header subwindow if necessary to
				 * bring next/previous message into view
				 */
		mt_set_curselmsg(nmsg); 
	}
	if (!mt_3x_compatibility) {
		(void)sprintf(mt_info, "%d deleted message%s", mt_del_count, 
			mt_del_count > 1 ? "s" : "");
		mt_update_info(mt_info);
		mt_set_namestripe_right(mt_info, TRUE);
	}

}

/*
 * Check to make sure the recipient field is correctly filled out.
 * Assumes "To:" must appear in pos 0 in reply sw and must be
 * followed by some alphnumeric characters.
 */
int
mt_check_to_who(ptr)
struct reply_panel_data	*ptr;
{
  Textsw_index	first = 0, last_plus_one;
  char		wstr[256];
  int		i;

  strcpy(wstr,"To:");
  if ((textsw_find_bytes(ptr->replysw, &first, &last_plus_one,
       wstr, strlen(wstr), 0)) == -1)
  { /* 'To:' not found, so insert it in pos 0, i.e. line 1, column 1 */
    mt_warn(mt_frame, 
	    "Warning: recipient field not found.",
	    "         Inserting field into text.",0);
 
    (void)window_set(ptr->replysw, TEXTSW_INSERTION_POINT, 0, 0);
    (void)sprintf(wstr, "To: %s\n", (mt_use_fields ? "|>recipients<|" : ""));
    if (textsw_insert(ptr->replysw, wstr, strlen(wstr)) == 0)
    {
      mt_warn(mt_frame,"Unable to insert recipient field", 0);
    }
    else
    {
       if (mt_use_fields) 
       { /* select the field for the user */
	 first = 0;
	 textsw_match_bytes(ptr->replysw,
			    &first, &last_plus_one, "|>", 2, "<|", 2,
			    TEXTSW_FIELD_BACKWARD);
	 (void) textsw_set_selection(ptr->replysw, first, last_plus_one,
		 PRIMARY_SELECTION | PRIMARY_PD_SELECTION);
	 (void) window_set(ptr->replysw, TEXTSW_INSERTION_POINT,
		 last_plus_one, 0);
       }
       else
	 /* set insertion point right after "To: " */
	 (void)window_set(ptr->replysw, TEXTSW_INSERTION_POINT, 5, 0);
    }
    return(0);  /* unsuccessful */
  }

  if (first != 0)  /* 'To:' must be in pos 0, i.e. line 1, col 0 */
  {
    mt_warn(mt_frame,
	    "The recipient designator 'To:' must be",
	    "at the beginning of the text subwindow.",0);
    return(0);  /* unsuccessful */
  }
  
  /* 
   * Check for blank recipient field.  The reply sw starts at
   * line 0, col 0.  The next operation sets last_plus_one to
   * the position of the 1st char on the 2nd visible line.
   */
  last_plus_one = textsw_index_for_file_line(ptr->replysw, 1);
  if (last_plus_one != TEXTSW_CANNOT_SET)
  {
    (void) window_get(ptr->replysw, TEXTSW_CONTENTS, 4, wstr, last_plus_one);

    for (i=0; wstr[i] != '\n' && i < (int)last_plus_one-4; i++)
  	if isalnum(wstr[i])
  		return(1);  /* successful - chars detected */
  }

  mt_warn(mt_frame, "You need to specify a recipient to mail to", 0);
  return(0);  /* unsuccessful */   
}

/* ARGSUSED */
static void
mt_do_deliver(item, ie)
	Panel_item      item;
	Event          *ie;
{
	char            current_filename[1024];
	Panel           panel;
	struct reply_panel_data *ptr;
	Textsw_index             first, last_plus_one;
	register char **fp;
	int		intact = (event_ctrl_is_down(ie));

	panel = panel_get(item, PANEL_PARENT_PANEL);
	ptr = (struct reply_panel_data *)panel_get(panel, PANEL_CLIENT_DATA);

	/* erase "|>subject<|" and other similiar fields */
	for (fp = &fields[0]; *fp != (char *) 0; fp++) 
	{
		first = 1;
		if (textsw_find_bytes(ptr->replysw, &first, &last_plus_one,
			*fp, strlen(*fp), 0) != -1)
			textsw_erase(ptr->replysw, first, last_plus_one);
	}

	if (mt_check_to_who(ptr) == 0)  /* check the recipient field */
		return;

	current_filename[0] = '\0';
	(void)textsw_append_file_name(ptr->replysw, current_filename);

	/* (Bug# 1012325 fix)  see comments at end of program */

        if ((window_get(ptr->replysw, TEXTSW_LENGTH) > 0) || 
	    (window_get(ptr->replysw, TEXTSW_MODIFIED))) 
	{
		/* typical case. edits made, need to write them to a file */
		int	save = FALSE;

		if (current_filename[0] != '\0' &&
			strcmp(current_filename, ptr->replysw_file) != 0) {
			/*
			 * user either loaded a file in, or else did a store
			 * along the way. Ask him if he wants his recent
			 * changes written back out to this file 
			 */
			int	expert = FALSE;

			(void) strcpy(line, current_filename);
			(void) strcat(line, " ?");
			if (mt_value("expert")) {
				expert = TRUE;
				mt_deassign("expert");
			}
			save = mt_confirm(ptr->replysw, TRUE, FALSE,
				"Yes: save edits in file",
				"No: use temporary file",
				"Do you want the edits to this window written back to",
				line,
				0);
			if (expert)
				mt_assign("expert", "");
		}
		if (save) {
			if (textsw_save(ptr->replysw, 0, 0))
				goto failed;
		} else {
			if (textsw_store_file(ptr->replysw,
				ptr->replysw_file, 0, 0)) 
				goto failed;
			(void) strcpy(current_filename, ptr->replysw_file);
		}
	}

	if (intact) {}
	else if (ptr->frame != mt_frame && ptr->behavior == mt_Close)
		(void) window_set(ptr->frame, FRAME_CLOSED, TRUE, 0);
	else if (ptr->behavior == mt_Disappear)
		mt_stop_reply(ptr);

	if (vfork() == 0) {
		register int i;

		for (i = GETDTABLESIZE(); i > 2; i--)
			(void)close(i);
		(void) close(0);
		(void) open(current_filename, 0);
		if (!intact)
			(void) unlink(ptr->replysw_file);  
		execlp("Mail", "Mail", "-t", 0);
		/*
		 * -t tells Mail to interpret the headers in the message,
		 * rather than requiring them to be supplied as arguments to
		 * the program. This allows Mail to pick up the "To:" line in
		 * the message and figure our who to send to. 
		 */
		exit(-1);
	}
	if (!intact) {
		textsw_reset(ptr->replysw);
		ptr->inuse = FALSE;
	}
	if (ptr->frame != mt_frame)
		(void) window_set(ptr->frame,
			FRAME_ICON, &mt_empty_letter_icon,
			0);
	return;
   failed:
	mt_warn(mt_frame, "Can't save changes.", 0);
	return;
}

/*
 * Do the save or copy.
 */
static void
mt_do_save(del, ie, has_selection)
	int             del;
	Event          *ie;
	int		has_selection;
{
	int             msg;
	char           *file;
	static char     buf[256];
	char           *p;
	extern char    *index();

	if (mt_nomail) {
		mt_warn(mt_frame, "No Mail", 0);
		return;
	}
	if ((msg = mt_get_curselmsg(0)) == 0)
		return;
	file = (char *)panel_get_value(mt_file_item);
	if (file == NULL || *file == '\0') {
		mt_warn(mt_frame, "Must specify file name.", 0);
		return;
	}
	mt_yield_all();       /* use mt_yield_all() to save shelf */
	/*
	 * if (msg == mt_curmsg) - bug: if edit current message, select
	 * something else, and click save, edits aren't saved 
	 */
	mt_save_curmsg();
	if (mt_copy_msg(msg, file))
		mt_save_filename(file);	/* only save name if copy succeeds */
	else {
		mt_warn(mt_frame, mt_info, 0);
		del = 0;		/* don't delete if copy fails */
	}
	(void) strcpy(buf, mt_info);
	if (del)
		mt_do_del(msg, ie, has_selection);
	mt_update_info(buf);
	if (p = index(&buf[1], ']')) 
		*(++p) = '\0';
	mt_set_namestripe_right(buf, TRUE);
}

/*
 * Handle input events when over the "File" item.
 * Menu request gives menu of recent file names.
 */
void
mt_file_event(item, ie)
	Panel_item      item;
	Event          *ie;
{
	Menu_item       mi;

	if (event_action(ie) == MS_RIGHT && event_is_down(ie)) {
		if (mt_file_menu == NULL) {
			mt_warn(mt_frame, "No saved file names", 0);
		} else {
			mi = menu_show(mt_file_menu, mt_cmdpanel,
				panel_window_event(mt_cmdpanel, ie), 0);
			if (mi) {
				char	*s;
				s = menu_get(mi, MENU_STRING);
				(void) panel_set_value(mt_file_item, s);
				(void) menu_set(mt_file_menu,
					MENU_SELECTED_ITEM, mi,
					0);
				mt_trace("set folder name to ", s);
			}
		}
	} else
		mt_default_panel_event(item, ie,
			"input folder name", "", NULL);
}

/*
 * Handles "new mail", "commit" and "done"
 */
static void
mt_do_retrieve(item, ie, incorporate, quietly)
	Panel_item      item;
	Event          *ie;
{
	static u_long   last;
	u_long          when;

	when = event_time(ie).tv_sec;
	if (last != 0 && when != 0 && when < last) {
		/*
		 * this event occurred BEFORE the last new mail operation
		 * completed, i.e., user bounced on the key. The when != 0
		 * check is to check for synthetic events, i.e., calls to
		 * mt_do_retrieve from mt_itimer_doit because user opened the
		 * tool when it was idle. 
		 */
		last = when;
		return;
	}
	if (event_ctrl_is_down(ie)) {
		mt_done_proc(item, ie);
		return;
	} 
	(void)win_release_event_lock(mt_cmdpanel_fd);
	mt_yield_all();
	mt_new_folder("%", incorporate, quietly);
	last = mt_current_time();
}

/*
 * Save a file name in the "File:" item menu.
 * The last "filemenusize" file names are saved.
 */
void
mt_save_filename(file)
	char           *file;
{
	Menu_item       mi, smi;
	int             height;
	register int    i, n, cn;
	static int      fileuse;

	/* filter out any white space before or after filename */
	while (isspace(*file) && (*file != '\0'))
		*file++;
	n = strlen(file) - 1;
	while (isspace(*(file + n)) && (*(file + n) != '\0'))
	{
		*(file + n) = '\0';
		n--;
	}

	if (mt_file_menu == NULL) {
		mt_file_menu = menu_create(MENU_NOTIFY_PROC,
			menu_return_item, 0);
	}
	n = fileuse + 1;
	smi = NULL;
	for (i = 1; mi = menu_get(mt_file_menu, MENU_NTH_ITEM, i); i++) {
		if (strcmp(file, (char *)menu_get(mi, MENU_STRING)) == 0) {
			(void) menu_set(mi, MENU_VALUE, ++fileuse, 0);
			return;
		}
		if ((cn = (int)menu_get(mi, MENU_VALUE)) < n) {
			smi = mi;
			n = cn;
		}
	}
	height = i - 1;
	if (mt_value("filemenusize") == NULL)
		i = DEFMAXFILES;
	else
		i = atoi(mt_value("filemenusize"));
	if (height < i) {
		(void) menu_set(mt_file_menu,
		    MENU_STRING_ITEM, mt_savestr(file), ++fileuse, 0);
	} else {
		free((char *)menu_get(smi, MENU_STRING));
		(void) menu_set(smi, MENU_STRING, mt_savestr(file), 0);
		(void) menu_set(smi, MENU_VALUE, ++fileuse, 0);
	}
}

void
mt_new_folder(file, incorporate, quietly)
	char           *file;
	int             quietly, incorporate;
	/*
	 * quietly is used in conjunction with auto-retrieval, which is
	 * currently not implemented. It gets new messages, but leave msgsw
	 * viewing the message that it is currently displaying 
	 */
{
	int             n, goto_system_mail_box;
	char		s[128];
	char           *p;
	extern char    *index();

	/* allow other processes to proceed */
	(void)win_release_event_lock(mt_cmdpanel_fd);
	mt_yield_all();       /* use mt_yield_all() to save shelf */
	if (!mt_nomail)
		mt_save_curmsg();
	goto_system_mail_box = (strcmp(file, "%") == 0);
	quietly = (quietly && !mt_nomail &&
		mt_system_mail_box && goto_system_mail_box);
	if (incorporate && !mt_nomail &&
		mt_system_mail_box && goto_system_mail_box
		&& !mt_3x_compatibility) {
		/*
		 * if in system mail box, and asking to get mail from system
		 * mail box, simply incorporate new mail without committing
		 * existing changes. 
		 */

		register	int i;

		mt_tool_is_busy(TRUE, "retrieving new mail...");
		for (i = mt_maxmsg; i > 0; i--) 
			if(!mt_message[i].m_deleted)
				break;
		if (mt_incorporate_mail()) {
		/*
		 * mt_incorporate_mail returns FALSE if
		 * user is not running latest mail. In this case, drop
		 * through and do it old way, i.e., commit and retrieve. 
		 */
			int	maxmsg = mt_maxmsg;

			(void) strcpy(s, mt_info);	/* n new, m unread */
			if (mt_is_prefix("No new mail for", mt_info)) {} 
			else if ((strcmp(mt_info,
				"Mail: skipping garbage before messages\n")
					== 0) ||
				(!mt_get_headers(maxmsg + 1)) ) 
			/*
			 * The skipping garbage message occurs if the user
			 * runs Mail while inside of mailtool, deletes some
			 * messages, and commits, receives some more
			 * messages, and then does an incorporate. It is
			 * caused by the fact that the place that the Mail
			 * that mailtool is attached to thought was the end
			 * of the spool file is now in the middle of a
			 * message. If the user runs Mail, delets some
			 * messages, commits, and then does an incorporate
			 * before any new messages have arrived, then Mail
			 * will think there is new mail because the spool
			 * file has been written, but there won't be any. In
			 * this case, mt_get_headers will not find any new
			 * messages and will return FALSE. 
			 */
				mt_warn(mt_headersw, 
			"Mail is confused about the state of your spool file.",
			"This could occur if you did a commit while running",
			"another instance of Mail or mailtool since the last", 
			"time you committed in *this* mailtool. Suggest you",
			"exit mailtool without committing your changes using",
			"the menu behind the 'Done' button. (Note: this will",
			"cause any messages that you deleted since the last", 
			"time you committed in mailtool to reappear.)",
			0);
			else {
				if (!quietly) 
					mt_curmsg = maxmsg + 1;
				mt_append_headers(!quietly);
				if (!quietly) 
					mt_update_msgsw(maxmsg + 1,
						TRUE, FALSE, FALSE,
						/* mt_load_headers has
						 * already positioned
						 * the header subwindow
						 */
						mt_has_selection());
			}
			mt_scandir = 1;
			mt_tool_is_busy(FALSE);
			mt_update_info(s);
			if (p = index(&s[1], ':'))
				mt_set_namestripe_right(++p, TRUE);
			else
				mt_set_namestripe_right(s, TRUE);
			return;
		} else 
			mt_tool_is_busy(FALSE);
	}
	s[0] = '\0';
	if (mt_3x_compatibility)
		(void) strcat(s, "loading messages");
	else if (!mt_nomail || strcmp(mt_folder, "[None]") != 0)
		(void) strcat(s, "committing changes");
	if (mt_3x_compatibility) {}
 	else if (goto_system_mail_box) {
		if (s[0] != '\0')
			(void) strcat(s, " and ");
		(void) strcat(s, "retrieving new mail");
	} else if (strcmp(file, mt_folder) != 0) {
		if (s[0] != '\0')
			(void) strcat(s, " and ");
		(void) strcat(s, "switching to ");
		(void) strcat(s, file);
	}
	(void) strcat(s, "...");
	mt_tool_is_busy(TRUE, s);
	if ((n = mt_set_folder(file)) > 0) {
		(void) strcpy(s, mt_info);	/* n new, m unread */
		mt_system_mail_box = goto_system_mail_box;
		mt_delp = NULL;	/* a commit was done so there are no deleted
				 * messages to undelete */
		mt_del_count = 0;
		mt_get_folder();
		(void) mt_get_headers(1);	/* sets mt_curmsg, mt_maxmsg */
		mt_load_headers();
		mt_prevmsg = 0;
		mt_update_msgsw(mt_curmsg,
				TRUE, FALSE, FALSE,	/* mt_load_headers has
							 * already positioned
							 * the header subwindow
							 */
				mt_has_selection());
		mt_tool_is_busy(FALSE);
		mt_nomail = FALSE;
		mt_scandir = 1;
		if (mt_panel_style == mt_3DImages)
			(void) panel_set(mt_del_item, PANEL_LABEL_IMAGE,
				mt_trash_pr, 0);
		if (!mt_3x_compatibility)
			mt_update_info(s);	/* info item is cleared by
						 * mt_update_msgsw */
		mt_set_namestripe_left(mt_folder, "", FALSE);
		if (p = index(&s[1], ':'))
			mt_set_namestripe_right(++p, TRUE);
		else
			mt_set_namestripe_right(++p, TRUE);
	} else if (n == 0) {	/* no new mail */
		(void) strcpy(s, mt_info);
		mt_system_mail_box = goto_system_mail_box;
		mt_delp = NULL;
		mt_del_count = 0;
		mt_get_folder();
		mt_update_info(mt_info);
		mt_set_namestripe_left(mt_folder, "[No Mail]", FALSE);
		if (p = index(&s[1], ':'))
			mt_set_namestripe_right(++p, TRUE);
		else
			mt_set_namestripe_right(++p, TRUE);
		mt_tool_is_busy(FALSE);
		mt_set_nomail();
		mt_prevmsg = 0;
		if (mt_panel_style == mt_3DImages)
			(void) panel_set(mt_del_item, PANEL_LABEL_IMAGE,
				mt_trash_pr, 0);
	} else {
		/*
		 * either no mail at all, or else a file or folder that
		 * doesn't exist 
		 */
		mt_tool_is_busy(FALSE);
		mt_update_info(mt_info);
		mt_set_namestripe_left(mt_folder,
			(strcmp(file, "%") == 0) ? "" :"[No Mail]",
			FALSE);
		mt_set_namestripe_right(mt_info, TRUE);
		if (mt_is_prefix("No mail for", mt_info)) {}
		else
			mt_warn(mt_frame, mt_info, 0);
			/* incorrect file or folder should be shown to user */
	}
}

static void
mt_set_nomail()
{
	mt_nomail = TRUE;
	textsw_reset(mt_msgsw, 0, 0);
	(void) unlink(mt_msgfile);
	mt_msg_time = 0;
	textsw_reset(mt_headersw, 0, 0);
}

static int
mt_has_selection()
{

	seln_holder = seln_inquire(SELN_PRIMARY);
	return (seln_holder.state != SELN_NONE &&
	    seln_holder_same_client(&seln_holder, mt_headersw)) ;
}


/**************** << C O M M E N T   S E C T I O N  >> **************** 
 
Name    Date    Comment  
----    ----    ------- 
mjb     5/12/89 Bug# 1012325 fix - checked length of textsw to insure
		the replysw will be saved when there is text inside.

***********************************************************************/ 

