#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)reply.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - managing the message composition window
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sunwindow/window_hs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sundev/kbd.h>

#include <suntool/window.h>
#include <suntool/frame.h>
#include <suntool/panel.h>
#include <suntool/text.h>
#include <suntool/walkmenu.h>
#include <suntool/scrollbar.h>

#include "glob.h"
#include "tool.h"

static	struct reply_panel_data		 *mt_create_new_replysw();

static Notify_value 	mt_destroy_replysw_proc();
static int 	mt_get_kbd_process();

struct reply_panel_data *
mt_get_replysw(item)
	Panel_item      item;
{
	Panel           panel;
	struct reply_panel_data *ptr, *last_ptr, *value;

	/*
	 * loop through all replysw's looking for one that is displayed and
	 * not in use. If none exist, take first one that is not in use and
	 * display it. If none exist, create a new one. 
	 */
	panel = NULL;
	if (item) {
		panel = panel_get(item, PANEL_PARENT_PANEL);
		ptr = (struct reply_panel_data *)panel_get(
			panel, PANEL_CLIENT_DATA);
	}
	if (panel && panel != mt_cmdpanel)
		/*
		 * if this button was not on the command panel, then take
		 * this replysw without looking further. Don't do inuse
		 * check. This allows user to use 'deliver, leave intact'
		 * option and then wrap new To: Subject: header around it 
		 */
		return (ptr);
	value = last_ptr = NULL;
	ptr = (struct reply_panel_data *)panel_get(
		mt_cmdpanel, PANEL_CLIENT_DATA);
	while (ptr) {
		if (!ptr->inuse && ptr->replysw &&
			(!mt_always_use_popup || (ptr->frame != mt_frame))) {
			/*
			 * if mt_always_use_popup is TRUE and this is the
			 * split window, don't use it. This allows user to
			 * set mt_always_use_popup during a session and do a
			 * source and have the right thing happen. It also
			 * allows the OPEN AND COMPOSE menu item to cause a
			 * popup to be used, even where the user has not set
			 * the default to be "alwaysusepopup"
			 */
			char	current_filename[1024];

			current_filename[0] = '\0';
			(void)textsw_append_file_name(ptr->replysw,
				current_filename);
			if (current_filename[0] != '\0' ||
				window_get(ptr->replysw, TEXTSW_MODIFIED)) 
				/*
				 * without clicking compose or reply, user
				 * has loaded a file into composition window
				 * or has entered text directly. Only occurs
				 * in popup windows, since ptr->inuse is
				 * always true for split composition window,
				 * because only way to open the window is via
				 * compose or reply button. 
				 */
				ptr->inuse = TRUE;
			else if (window_get(ptr->replysw, WIN_SHOW) &&
				window_get(ptr->frame, WIN_SHOW) &&
				!window_get(ptr->frame, FRAME_CLOSED) )
			    		return(ptr); /* visible, use this */
			else if (value == NULL)
			/*
			 * this one not in use, but not displayed. remember
			 * it but look further.  
			 */
				value = ptr;
		}
		last_ptr = ptr;
		ptr = ptr->next_ptr;
	}
	if (!value) {
		if (mt_3x_compatibility) {
			mt_warn(mt_frame, "Already replying.", 0);
			return(NULL);
		}
		value = mt_create_new_replysw();
		if (last_ptr)
			last_ptr->next_ptr = value;
		else
			(void) panel_set(mt_cmdpanel, PANEL_CLIENT_DATA, value, 0);
				/*
				 * occurs if default is alwaysusepopup, and
				 * this is the first replysw to be created 
				 */
	}
	return (value);
}

static struct reply_panel_data *
mt_create_new_replysw()
{
	Frame           frame;
	struct reply_panel_data *ptr;
	int             height;

	if (mt_debugging)  {
		(void) printf("creating replysw, #fds = %d\n", nfds_avail());
		fflush(stdout);
	}

	if (nfds_avail() < 8) { 
		mt_warn(mt_frame, "Not enough fds for any more popup windows.",
			0);
		/*
		 * Three fds are used in creating the popup, one for the frame,
		 * and one each for the two subwindows. Two more fds will be
		 * consumed in loading the replysw, for a total of 5. It
		 * requires 3 fds in order to be able to do a deliver because
		 * of the way that textsw does saves. Therefore, you must
		 * have at least 8 fds at this point.  
		 */
		return(NULL);
	}

	frame = window_create(NULL, FRAME,
		WIN_ERROR_MSG, "Unable to create reply frame\n",
		WIN_SHOW, 	FALSE,
		WIN_WIDTH, 	window_get(mt_frame, WIN_WIDTH),
		WIN_ROWS, 	mt_popuplines + 3,
			/* allow for height of reply panel */
		FRAME_LABEL, 	"Reply to or Compose Mail",
		FRAME_SHOW_LABEL, TRUE,
		FRAME_ICON,	&mt_empty_letter_icon,
		0);
	if (frame == NULL) {
		if (mt_debugging)
			(void)fprintf(stderr,"Unable to create reply frame\n");
		mt_warn(frame, "Unable to create reply frame", 0);
		return (NULL);
	}
	height = (int)window_get(frame, WIN_HEIGHT);
	(void) window_set(frame,
		WIN_Y, screenrect.r_height - height,
		WIN_X, (int)window_get(mt_frame, WIN_X),
		0);
		/*
		 * position popup below tool, if possible, but make sure that
		 * it stays completely in view 
		 */

	ptr = mt_create_reply_panel(frame);
	if (ptr == NULL) 	/* unable to create reply panel */
	{
		/* warning message already given - so stifle this one
		   mt_warn(frame, "Unable to create composition window.", 0);
		*/

		/* turn off destroy confirmation alert */
		(void)window_set(frame, FRAME_NO_CONFIRM, TRUE, 0);

		/*
		 * Note: the interpose function mt_destroy_replysw_proc
		 * is set later, so this will correctly free the frame memory
		 * without first calling mt_destroy_replysw_proc.
		 */
		(void)window_destroy(frame);
		return(NULL);
	}	
	/* catch when being destroyed */
	notify_interpose_destroy_func(frame, mt_destroy_replysw_proc);

	mt_add_window(frame, mt_Frame);

	(void) window_set(ptr->reply_panel, WIN_SHOW, TRUE, 0);
	(void) window_set(ptr->replysw,
		WIN_X, 0,
		WIN_BELOW, ptr->reply_panel,
		WIN_SHOW, TRUE,
		0);
	(void) window_set(frame, WIN_CLIENT_DATA, ptr, 0);
	/*
	 * so can get from frame to reply panel data 
	 */
	return (ptr);
}

/*
 * user is quitting a popup reply window. This takes care of removing the
 * window from the tools data structures 
 */
static Notify_value 
mt_destroy_replysw_proc(client, status)
	Notify_client   client;
	Destroy_status  status;
{

	if (status == DESTROY_CHECKING) {
		struct reply_panel_data *ptr;
		
		ptr = (struct reply_panel_data *)window_get(
				client, WIN_CLIENT_DATA);
		if (!mt_destroying && !mt_confirm(client, FALSE,
				!window_get(ptr->replysw, TEXTSW_MODIFIED),
				/*
				 * alert is optional if window has not been
				 * changed
				 */
				"Confirm",
				"Cancel",
				"Are you sure you want to Quit composition window?",
				0)) {
			notify_veto_destroy(client);
			return (NOTIFY_DONE);
		} else {
			struct reply_panel_data *ptr, *prev;

			prev = NULL;
			ptr = (struct reply_panel_data *)panel_get(
				mt_cmdpanel, PANEL_CLIENT_DATA);
			while (ptr) {
				if (client == ptr->frame) {
					/* found it.  splice it out of chain */
					mt_remove_window(ptr->replysw);
					mt_remove_window(ptr->reply_panel);
					mt_remove_window(ptr->frame);
					textsw_reset(ptr->replysw, 0, 0);
					textsw_reset(ptr->replysw, 0, 0);
					/*
					 * to avoid veto by textsw if any
					 * edits 
					 */
					if (prev)
						prev->next_ptr = ptr->next_ptr;
					else
						(void) panel_set(mt_cmdpanel,
							PANEL_CLIENT_DATA,
							ptr->next_ptr, 0);
					break;
				}
				prev = ptr;
				ptr = ptr->next_ptr;
			}
			mt_update_info("");
		}
	} 
	(void) window_set(client, FRAME_NO_CONFIRM, TRUE, 0);
	return (notify_next_destroy_func(client, status));
}

void
mt_start_reply(ptr)
	struct reply_panel_data *ptr;
{
	Frame           frame;
	int             percent, height;
	Textsw          next_split;
	char           *p;

	frame = ptr->frame;

	mt_replypanel_init(ptr->reply_panel);
	if (frame == mt_frame && !window_get(ptr->replysw, WIN_SHOW)) {
		if ((p = mt_value("msgpercent")) != NULL) {
			percent = atoi(p);
			if (percent >= 100)
				percent = 50;
		} else
			percent = 50;
		height = 0;
		/*
		 * destroy all splits because too hard to figure out what
		 * height to give them when create composition window, and
		 * then restore them after message is delivered or cancelled 
		 */
		while (next_split = (Textsw)textsw_next(
			(Textsw)textsw_first(mt_msgsw))) {
			height = height + 5 +
				(int)window_get(next_split, WIN_HEIGHT);
			/* 5 for the border */
			(void)notify_post_destroy(next_split, DESTROY_CLEANUP,
				NOTIFY_IMMEDIATE);

		/*
		 * XXX shouldn't have to cast value. Problem is textsw_first
		 * textsw_next not extern in Textsw.h SHould be fixed 
		 */
		}
		height = (height + (int)window_get(mt_msgsw, WIN_HEIGHT) -
			(mt_3x_compatibility
				? 0
				: (int)window_get(ptr->reply_panel,
					WIN_HEIGHT))  )
		  	* percent / 100;
		(void) window_set(mt_msgsw, WIN_HEIGHT, height, 0);
		if (mt_3x_compatibility) {
			(void)panel_set(mt_deliver_item,
				PANEL_SHOW_ITEM, TRUE, 0);
			(void)panel_set(mt_cancel_item,
				PANEL_SHOW_ITEM, TRUE, 0);
			(void) window_set(ptr->replysw,
				WIN_X, 0,
				WIN_BELOW, mt_msgsw,
				WIN_SHOW, TRUE,
				WIN_HEIGHT, WIN_EXTEND_TO_EDGE,
				0);
		} else {
			(void) window_set(ptr->reply_panel,
				WIN_X, 0,
				WIN_BELOW, mt_msgsw,
				WIN_SHOW, TRUE,
				0);
			(void) window_set(ptr->replysw,
				WIN_X, 0,
				WIN_BELOW, ptr->reply_panel,
				WIN_SHOW, TRUE,
				WIN_HEIGHT, WIN_EXTEND_TO_EDGE,
				0);
		}
	}
	ptr->inuse = TRUE; 
	ptr->normalized = FALSE;
	/*
	 * work around to defer normalization until repaint actually occurs.
	 * Problem is that textsw_possibly_normalize uses information in line
	 * tables, which are not updated until the repaint.  When
	 * mt_replysw_event_proc gets a WIN_PAINT event, and ptr->normalized
	 * is FALSE, it also does a textsw_possibly_normalize.
	 */
}

void
mt_display_reply(ptr)
	struct reply_panel_data *ptr;

{
	Frame           frame;

	frame = ptr->frame;
	if (!window_get(frame, WIN_SHOW)) 
		(void) window_set(frame, WIN_SHOW, TRUE, 0);
	else if (window_get(frame, FRAME_CLOSED))
		(void) window_set(frame, FRAME_CLOSED, FALSE, 0); 
	else if (frame != mt_frame) {
		int	framefd = window_fd(frame);
    		int	rootfd = rootfd_for_toolfd(framefd);

		wmgr_top(framefd, rootfd);	/* move popup to top */
		(void)close(rootfd);
	}
	mt_move_input_focus(ptr->replysw);
	
	if (mt_retained)
		textsw_insert_makes_visible(ptr->replysw);
	/*
	 * if the user is running with *retained* text windows, then a
	 * WIN_PAINT event will never happen. However, in this case, it is
	 * safe to do the textsw_insert_makes_visible at this point, because
	 * the window would have already been repainted at this point. Thus
	 * this textsw_insert_makes_visible handles normalizing the window when
	 * it is retained, and the one in mt_replysw_event_proc handles
	 * normalization when the window is not retained.
	 */
}

void
mt_move_input_focus(replysw)
	Textsw          replysw;
{
	int             focus;

	if (mt_value("moveinputfocus")
		|| (focus = mt_get_kbd_process()) < 0
		|| focus == getpid())
		(void) window_set(replysw, WIN_KBD_FOCUS, TRUE, 0);
}

/*
 * Done doing a reply (or send, or forward).
 * Close the reply subwindow destroying its
 * split views.
 */
void
mt_stop_reply(ptr)
	struct reply_panel_data *ptr;
{
	Textsw          next_split;
	Frame           frame;

	/* destroy any reply subwindow split views */
	while (next_split = (Textsw)textsw_next(
				(Textsw)textsw_first(ptr->replysw)))
		(void)notify_post_destroy(next_split, DESTROY_CLEANUP,
			NOTIFY_IMMEDIATE);
        /*
	 * (Bug# 1018269 fix) destroy any message subwindow split views 
	 * the user may have conjured up while the reply subwindow
	 * was up.
	 */
	while (next_split = (Textsw)textsw_next(    
				(Textsw)textsw_first(mt_msgsw)))
		(void)notify_post_destroy(next_split, DESTROY_CLEANUP,
			NOTIFY_IMMEDIATE);

	textsw_reset(ptr->replysw);
	frame = ptr->frame;
	if (mt_3x_compatibility) {
		(void)panel_set(mt_deliver_item, PANEL_SHOW_ITEM, FALSE, 0);
		(void)panel_set(mt_cancel_item, PANEL_SHOW_ITEM, FALSE, 0);
	}
	if (frame != mt_frame) 
		(void) window_set(frame, WIN_SHOW, FALSE, 0);
	else {
		(void) window_set(ptr->reply_panel, WIN_SHOW, FALSE, 0);
		(void) window_set(ptr->replysw, WIN_SHOW, FALSE, 0);
		(void) window_set(mt_msgsw, WIN_HEIGHT, WIN_EXTEND_TO_EDGE, 0);
	}
}

/*
 * Get the process id of the process holding the keyboard focus.
 */
static int
mt_get_kbd_process()
{
	int             fd, num;
	char            name[WIN_NAMESIZE];

	if ((fd = (int)window_get(mt_msgsw, WIN_FD, 0)) < 0)
		return (fd);
	if ((num = win_get_kbd_focus(fd)) == WIN_NULLLINK)
		return (-1);
	(void)win_numbertoname(num, name);
	if ((fd = open(name)) < 0)
		return (fd);
	num = win_getowner(fd);
	(void)close(fd);
	return (num);
}



