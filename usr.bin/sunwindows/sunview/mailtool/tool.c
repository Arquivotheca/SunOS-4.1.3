#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)tool.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - tool creation, termination, handling
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sunwindow/window_hs.h>
#include <sunwindow/defaults.h>

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <suntool/window.h>
#include <suntool/frame.h>
#include <suntool/panel.h>
#include <suntool/text.h>

#include <suntool/scrollbar.h>
#include <suntool/selection.h>
#include <suntool/selection_svc.h>
#include <suntool/selection_attributes.h>
#include <suntool/walkmenu.h>
#include <suntool/icon.h>jobs


#include "glob.h"
#include "tool.h"

#define	TOOL_LINES	(mt_headerlines + mt_cmdlines + mt_maillines + 3)
#define	TOOL_COLS	80

Frame	mt_frame;
Textsw	mt_headersw;		/* the header subwindow */
Panel	mt_cmdpanel;		/* the command panel */
Textsw	mt_msgsw;			/* the message subwindow */
Textsw	mt_replysw;		/* the mail reply subwindow */
Panel	mt_replypanel;		/* the reply subwindow panel */

Pixfont	*mt_font;		/* the default font */
static Menu	mt_frame_menu;
Menu_item	mt_open_menu_item;
Menu		mt_open_menu_pullright;

char           *mt_namestripe, *mt_namestripe_left, *mt_namestripe_right;
int		mt_namestripe_width;
static	int	mt_headerlines;		/* lines of headers */
	int	mt_cmdlines;		/* command panel height */
static	int	mt_maillines;		/* lines for mail item */
	int	mt_popuplines;		/* lines for popup composition window */

enum mt_Panel_Style	mt_panel_style;
enum mt_Load_Messages	mt_load_messages;
int	mt_prevmsg;
int	mt_idle;
int	mt_nomail;
int	mt_retained;
int	mt_iconic;
int	mt_system_mail_box;	/* true if current folder is system mail box */
int	mt_3x_compatibility, mt_use_fields, mt_always_use_popup, mt_debugging;
int	mt_41_features;
int	mt_use_images;
int	mt_bells, mt_flashes;
int	mt_destroying;
char	*mt_load_from_folder;
char	*mt_wdir;			/* Mail's working directory */
time_t	mt_msg_time;		/* time msgfile was last written */

static void	mt_msgsw_notify_proc(), mail_load_messages();
static void	mt_prepare_to_die(), (*cached_mt_msgsw_notify)();
static Notify_value	mt_signal_func();

int	charheight, charwidth;	/* size of default font */
int             padding;	/* difference between width of window and
				 * width of interior space */

int	mt_memory_maximum;

static Notify_value	mt_itimer(), /* mt_itimer1(), */ mt_itimer_doit();
static Notify_value	mt_frame_event_proc(), mt_destroy();

static caddr_t	mt_just_open_proc(), mt_open_and_compose_proc(), mt_open_and_read_proc(), mt_switch_folder_action_proc();
/* not used - hala
static Menu	mt_switch_folder_gen_proc();
*/

Rect	screenrect;
u_long	last_event_time;


/*
 * Icons
 */
#define	static			/* XXX - gross kludge */
short	mt_mail_image[256] = {
#include <images/mail.icon>
};
DEFINE_ICON_FROM_IMAGE(mt_mail_icon, mt_mail_image);

short	mt_nomail_image[256] = {
#include <images/nomail.icon>
};
DEFINE_ICON_FROM_IMAGE(mt_nomail_icon, mt_nomail_image);

short	mt_busy_image[256] = {
#include <images/mailtool_busy.icon>
};
DEFINE_ICON_FROM_IMAGE(mt_busy_icon, mt_busy_image);

short	mt_unknown_image[256] = {
#include <images/dead.icon>
};
DEFINE_ICON_FROM_IMAGE(mt_unknown_icon, mt_unknown_image);

short	mt_empty_image[256] = {
#include <images/empty_letter.icon>
};
DEFINE_ICON_FROM_IMAGE(mt_empty_letter_icon, mt_empty_image);

short	mt_reply_image[256] = {
#include <images/reply.icon>
};
DEFINE_ICON_FROM_IMAGE(mt_replying_icon, mt_reply_image);

short	mt_compose_image[256] = {
#include <images/compose.icon>
};
DEFINE_ICON_FROM_IMAGE(mt_composing_icon, mt_compose_image);
#undef static

struct icon *mt_icon_ptr;

void
mt_init_tool_storage()
{
	/* Dynamically allocate mt_wdir */
	mt_wdir = (char *)calloc(256, sizeof (char));
	/* Initialize data */
	mt_namestripe = (char *)malloc(256);
	mt_namestripe_left = (char *)malloc(256);
	mt_namestripe_right = (char *)malloc(256);
	mt_namestripe[0] = '\0';
	mt_idle = TRUE;
	mt_iconic = TRUE;
	mt_nomail = TRUE;
	mt_panel_style = mt_Old;
	mt_load_messages = mt_When_Opened;
	mt_3x_compatibility = FALSE;
	mt_41_features = FALSE;
#ifdef USE_IMAGES
	mt_use_images = TRUE;
#else
	mt_use_images = FALSE;
#endif
	mt_retained = FALSE;
	mt_use_fields = FALSE;
	mt_always_use_popup = FALSE;
	mt_debugging = FALSE;
	mt_destroying = FALSE;
	mt_load_from_folder = NULL;
	mt_icon_ptr = &mt_unknown_icon;
}

/*
 * Build and start the tool.
 */
void
mt_start_tool(argc, argv)
	int             argc;
	char          **argv;
{
/* not used - hala  Panel_item      item; */
	int             sigwinched();
	int             ondeath();
	int		margin;

	struct reply_panel_data *ptr;

	mt_init_mailtool_defaults();

	/*
	 * Get the current directory.
	 */
	mt_get_mailwd(mt_wdir);
	(void) chdir(mt_wdir);

	/*
	 * Initialize the file names menu.
	 */
	mt_init_filemenu();

	/*
	 * Initialize the folder menu.  (Delayed to decrease start-up time 
	 * per ecd recommendation - hala)
	 */
/*
	mt_init_folder_menu();
*/

	/*
	 * Create the tool.
	 */
	margin = (int)defaults_get_integer("/Text/Left_margin", 4, 0);
	padding = 10 + margin + 2 +
		(int)scrollbar_get(SCROLLBAR, SCROLL_THICKNESS);
	mt_frame = window_create(0, FRAME,
		WIN_WIDTH, ATTR_COLS(TOOL_COLS) + padding,
		WIN_ROWS, TOOL_LINES,
		WIN_ERROR_MSG, "Unable to create mailtool frame\n",
		FRAME_LABEL, mt_cmdname,
		FRAME_SUBWINDOWS_ADJUSTABLE, TRUE,
	 	FRAME_CLOSED, (mt_iconic ? TRUE : FALSE),  
		FRAME_ICON, mt_icon_ptr,
		FRAME_SHOW_LABEL, TRUE,
		FRAME_ARGC_PTR_ARGV, &argc, argv,
		0);
        if (mt_frame == NULL) {
                if (mt_debugging) 
                    (void)fprintf(stderr, "Unable to create mailtool frame\n");
                mt_warn(0, "Unable to create mailtool frame", 0);
                exit(1);
        }
	mt_add_window(mt_frame, mt_Frame);
	/* enters mt_frame in array of windows */

	mt_parse_tool_args(argc, argv);

	mt_namestripe_width = (int)window_get(mt_frame, WIN_COLUMNS);
	mt_font = (Pixfont *)window_get(mt_frame, WIN_FONT);
	charwidth = mt_font->pf_defaultsize.x;
	charheight = mt_font->pf_defaultsize.y;
	screenrect = *((Rect *)window_get(mt_frame, WIN_SCREEN_RECT));

/***	mt_idle = (mt_load_messages == mt_When_Opened);
***/
	/* catch going from icon to window */
	notify_interpose_event_func(mt_frame, mt_frame_event_proc, NOTIFY_SAFE);


	mt_frame_menu = window_get(mt_frame, WIN_MENU);
	if (!mt_3x_compatibility &&
		(mt_open_menu_item = menu_find(mt_frame_menu,
			MENU_STRING, "Open", 0))
		) {
		mt_open_menu_pullright = menu_create(
			MENU_ITEM,
				MENU_STRING, "Read New Mail",
				MENU_ACTION_PROC, mt_open_and_read_proc,
				0,
			MENU_ITEM,
				MENU_STRING, "Read Folder",
				MENU_GEN_PULLRIGHT, mt_folder_menu_gen_proc,
				0,
			MENU_ITEM,
				MENU_STRING, "Compose Message",
				MENU_ACTION_PROC, mt_open_and_compose_proc,
				0,
			MENU_ITEM,
				MENU_STRING, "Just Open",
				MENU_ACTION_PROC, mt_just_open_proc,
				0,
			MENU_INITIAL_SELECTION_EXPANDED, FALSE,
			0);
		menu_set(mt_open_menu_item,
			MENU_PULLRIGHT, mt_open_menu_pullright,
			0);
	}

	/* catch when being destroyed */
	(void) notify_interpose_destroy_func(mt_frame, mt_destroy);
	/*
	 * Create the header textsubwindow.
	 */
	mt_headersw = (Textsw)window_create(mt_frame, TEXT,
		WIN_ROWS, mt_headerlines,
		WIN_ERROR_MSG, "mailtool: Unable to create header window\n",
		TEXTSW_MEMORY_MAXIMUM, mt_memory_maximum,
		TEXTSW_AUTO_INDENT, FALSE,
		TEXTSW_DISABLE_LOAD, TRUE,
		TEXTSW_READ_ONLY, TRUE, 
		0);
        if (mt_headersw == NULL) {
                if (mt_debugging)
                    (void)fprintf(stderr,"Unable to create header subwindow\n");
                mt_warn(mt_frame, "Unable to create header subwindow", 0);
                exit(1);
        }
	mt_add_window(mt_headersw, mt_Text);
	(void) window_set(mt_headersw,
		TEXTSW_LINE_BREAK_ACTION, TEXTSW_CLIP,
		0);

	/*
	 * Create the command panel.
	 */
	if (mt_panel_style == mt_3DImages)
		mt_create_3Dimages_panel();
	else if (mt_panel_style == mt_New)
		mt_create_new_style_panel();
	else if (mt_panel_style == mt_Old)
		mt_create_old_style_panel();
	else 
		fprintf(stderr, "unrecognized value for mt_panel_style\n");

	last_event_time = mt_current_time();
	if (mt_load_from_folder)/* allows user to set folder name from
				 * command line */
		panel_set_value(mt_file_item, mt_load_from_folder);

	/*
	 * Create the mail message textsubwindow.
	 */
	mt_msgsw = (Textsw)window_create(mt_frame, TEXT,
		WIN_ERROR_MSG, "mailtool: unable to create message window\n",
		WIN_X, 0,
		WIN_BELOW, mt_cmdpanel, 
		TEXTSW_MEMORY_MAXIMUM, mt_memory_maximum,
		TEXTSW_CONFIRM_OVERWRITE, 0,
		TEXTSW_STORE_SELF_IS_SAVE, 1,
		TEXTSW_DISABLE_LOAD, 1,
		0);
        if (mt_msgsw == NULL) {
                if (mt_debugging)  
                    (void)fprintf(stderr,"Unable to create message textsw\n");
                mt_warn(mt_frame, "Unable to create message textsw", 0);
                exit(1);
        }
	mt_add_window(mt_msgsw, mt_Text);

	cached_mt_msgsw_notify = (void (*) ())window_get(
			mt_msgsw, TEXTSW_NOTIFY_PROC);
	(void) window_set(mt_msgsw, TEXTSW_NOTIFY_PROC,
		mt_msgsw_notify_proc, 0);

	if (mt_3x_compatibility || !mt_always_use_popup) {
		/*
		 * Create the mail reply textsubwindow and its control panel.
		 */
		ptr = mt_create_reply_panel(mt_frame);
		if (ptr == NULL) /* unable to create reply panel */
		{
		   fprintf(stderr, 
			   "Unable to create reply panel in mt_start_tool\n");
		   exit(1);
		}
		mt_replypanel = ptr->reply_panel;
		mt_replysw = ptr->replysw;
		panel_set(mt_cmdpanel, PANEL_CLIENT_DATA, ptr, 0);
			/*
			 * store ptr here to be able to find the first
			 * replysw in mt_get_replysw 
			 */
	} else {
		mt_replypanel = NULL;
		mt_replysw = NULL;
	}

	/*
	 * Catch signals, install tool.
	 */
	(void)notify_set_signal_func(&mt_mailclient, mt_signal_func,
		SIGTERM, NOTIFY_ASYNC);
	(void)notify_set_signal_func(&mt_mailclient, mt_signal_func,
		SIGXCPU, NOTIFY_ASYNC);

	/*
	 * Kludge - we want the notifier to harvest all dead children
	 * so we tell it to wait for its own death.
	 */
	(void)notify_set_wait3_func(&mt_mailclient, notify_default_wait3,
		getpid());

	/* start up with correct icon */
	mt_check_mail_box();

	/* set timer intervals */
	mt_start_timer();

	window_main_loop(mt_frame);

	mt_stop_mail(mt_aborting);
}

/*
 * resetting these variables after reading .mailrc will immediately affect
 * behavior of mailtool, so this procedure is called from both mt_start_tool
 * and from mt_mailrc_proc 
 */
void
mt_init_mailtool_defaults()
{
	int             i;
	char           *p;

	mt_3x_compatibility = !defaults_get_boolean(
		"/Compatibility/New_Mailtool_features", TRUE, 0);
	mt_retained = (int)defaults_get_boolean("/Text/Retained", FALSE, 0);
	mt_cmdlines = 4;	/* only used for oldstyle panel */
	if (mt_3x_compatibility) { 
		if ((p = mt_value("cmdlines")) && (i = atoi(p)) > 0)
			mt_cmdlines = i;
	} else {
		while ((p = index(mt_cmdname, '/')) != NULL)
			mt_cmdname = ++p;	/* strip off directories in
						 * name of command in order
						 * to conserve space in
						 * namestripe */
		mt_use_fields = (mt_value("disablefields") ? FALSE : TRUE);
		mt_always_use_popup =
			(mt_value("alwaysusepopup") ? TRUE : FALSE);
		mt_41_features = (mt_value("4.1features") ? TRUE : FALSE);
		if (!mt_41_features)
			mt_panel_style = mt_New;
		/*
		 * only if 4.1features is TRUE can you experiment with
		 * different panel styles 
		 */
		else {
			if (!mt_value("panelstyle")
				|| (strcmp(mt_value("panelstyle"), "New") == 0)
				)
				mt_panel_style = mt_New;
			else if (strcmp(mt_value("panelstyle"), "Old") == 0)
				mt_panel_style = mt_Old;
			else if (strcmp(mt_value("panelstyle"),
					"ThreeDImages") == 0) {
				mt_panel_style = mt_3DImages;
				if (!mt_use_images) {
					fprintf(stderr,
"*****3DImage panel style has been specified in user's defaults!\nMailtool must be compiled with -DUSE_IMAGES in order to use this feature!\nUsing default panel style instead.\n");
					mt_panel_style = mt_New;
				}
			} else {
				fprintf(stderr,
					"unrecognized default setting for panelstyle: %s\n",
					mt_value("panelstyle"));
				mt_panel_style = mt_New;
			}
		}
	}

/***	following implemented defaults options that controlled the behavior of the tool with respect to retrieving new mail. 
	if (!mt_value("loadmessages"))
		mt_load_messages = mt_When_Opened;
	else if (strcmp(mt_value("loadmessages"), "Startup") == 0)
		mt_load_messages = mt_At_Startup;
	else if (strcmp(mt_value("loadmessages"), "Asked") == 0)
		mt_load_messages = mt_When_Asked;
	else 
		(void)fprintf(stderr, "unrecognized default setting: %s\n",
			mt_value("loadmessages"));
***/

	mt_memory_maximum = TEXTSW_INFINITY;

	/*
	 * Get the sizes of the various subwindows.
	 */
	mt_headerlines = 10;
	mt_maillines = 30;
	mt_popuplines = 30;
	if ((p = mt_value("headerlines")) && (i = atoi(p)) > 0)
		mt_headerlines = i;
	if ((p = mt_value("maillines")) && (i = atoi(p)) > 0)
		mt_maillines = i;
	if ((p = mt_value("popuplines")) && (i = atoi(p)) > 0)
		mt_popuplines = i;
	mt_bells = 0;
	mt_flashes = 0;
	if (p = mt_value("bell")) {
		mt_bells = atoi(p);
		if (mt_bells <= 0)
			mt_bells = 0;
	}
	if (p = mt_value("flash")) {
		mt_flashes = atoi(p);
		if (mt_flashes <= 0)
			mt_flashes = 0;
	}

	/*
	 * If "autoprint" is set, turn it off.  We do it ourselves.
	 */
	if (mt_value("autoprint"))
		mt_set_var("autoprint", (char *)NULL);
}


/* ARGSUSED */
static Notify_value
mt_frame_event_proc(client, event, arg, when)
	Notify_client   client;
	Event          *event;
	Notify_arg      arg;
	Notify_event_type when;
{
	Notify_value    rc;
	int             closed_initial, closed_current;

	closed_initial = (int)window_get(client, FRAME_CLOSED);
	rc = notify_next_event_func(client, event, arg, NOTIFY_SAFE);
	if (event_action(event) == WIN_RESIZE) {
		mt_namestripe_width = (int)window_get(mt_frame, WIN_COLUMNS);
		mt_set_namestripe_both();
		return (rc);
	}
	closed_current = (int)window_get(client, FRAME_CLOSED);
	if (closed_current || closed_initial) {
		if (mt_cd_frame) {
			int	displayed;

			displayed = (int)window_get(mt_cd_frame, WIN_SHOW);
			if (displayed == closed_current)
				window_set(mt_cd_frame,
					WIN_SHOW, !displayed,
					0);
		}
	}
	if (closed_initial && !closed_current) {
		menu_set(mt_open_menu_item, MENU_PULLRIGHT, NULL, 0);
		if (mt_idle) {
			/*
			 * If window was just opened and we are idle
			 * then read new mail.
			 */
			static struct itimerval itv = { {0, 0}, {0, 1} };

			/*
			 * Set up timer to go off once and immediately.
			 * This schedules the mail activity to be done upon
			 * opening the tool, to occur after the tool is open
			 * completely.
			 */

			notify_set_itimer_func(&mt_mailclient, mt_itimer_doit,
			    ITIMER_VIRTUAL, &itv, 0);
		}
	}
	return (rc);
}

/*
 * SIGTERM and SIGXCPU handler.
 * Save mailbox and exit.
 */
/* ARGSUSED */
static Notify_value
mt_signal_func(client, sig)
	Notify_client   client;
	int             sig;
{

	if (!mt_idle)
		mt_stop_mail(0);
	if (sig == SIGXCPU) {
		/* for debugging */
		(void)sigsetmask(0);
		abort();
	}
	mt_done(sig == SIGTERM ? 0 : 1);
}

static Notify_value
mt_destroy(client, status)
	Notify_client   client;
	Destroy_status  status;
{
	Notify_value	val;

	if (status == DESTROY_CHECKING) {
		if (mt_aborting) {
			if (!mt_confirm(mt_frame, FALSE, FALSE,
				"Confirm",
				"Cancel",
				"Are you sure you want to Quit",
				"without committing changes?",
				0)) {
				notify_veto_destroy(client);
				mt_aborting = FALSE;
			} else {
				mt_prepare_to_die(client);
				val = notify_next_destroy_func(client, status);
			}
		} else {
			if (!mt_confirm(mt_frame, FALSE, TRUE,
				"Confirm",
				"Cancel",
				"Are you sure you want to Quit?",
				0)) 
				notify_veto_destroy(client);
			else {	/* entire tool is going away */

				mt_save_curmsg();
				mt_prepare_to_die(client);
				val = notify_next_destroy_func(client, status);
			}
		}
	} else {
		val = notify_next_destroy_func(client, status);
		notify_remove(&mt_mailclient);
	}
	return (val);
}

static void
mt_prepare_to_die(client)
	Notify_client   client;
{
	Frame           frame;
	struct reply_panel_data *ptr;
	char		dead_letter[256];

	mt_destroying = TRUE;
	/*
	 * tells replysw's that any necessary confirmation has
	 * already taken place 
	 */
	textsw_reset(mt_headersw, 0, 0);
	textsw_reset(mt_msgsw, 0, 0);
	ptr = (struct reply_panel_data *)panel_get(
		mt_cmdpanel, PANEL_CLIENT_DATA);
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
		  mt_full_path_name(s, dead_letter);
		  if (f = mt_fopen(dead_letter, "w")) 
			/* check to see if can write to this file */
			(void) fclose(f);
		  else
			dead_letter[0] = '\0';
		}
		else
		  dead_letter[0] = '\0';
	}
	/*
	 * find all the popups and kill them. 
	 */
	while (ptr) {
		if (dead_letter[0] != '\0' && ptr->inuse)
		/*
		 * NEEDS TO BE FIXED TO HANDLE CASE WHERE THERE IS MORE THAN
		 * ONE ACTIVE REPLYSW, IE. TO CAT THE FILES TOGETHER INTO
		 * DEAD LETTER 
		 */
			(void) textsw_store_file(ptr->replysw,
				dead_letter, 0, 0);
		textsw_reset(ptr->replysw, 0, 0);
		frame = window_get(
			ptr->replysw, WIN_OWNER);
		if (frame != mt_frame) {
			window_set(frame,
			   FRAME_NO_CONFIRM, TRUE, 0);
			window_destroy(frame);
		}
		ptr = ptr->next_ptr;
	}
	window_set(client, FRAME_NO_CONFIRM, TRUE, 0);
}


/* timers */

/*
 * Start the timer to look for new mail.
 */
void
mt_start_timer()
{
	static struct itimerval itv;
	char           *p;
	int             interval = 0;

	if (p = mt_value("interval"))
		interval = atoi(p);
	if (interval == 0)
		interval = 5*60;	/* 5 minutes */
	itv.it_interval.tv_sec = interval;
	itv.it_value.tv_sec = interval;
	(void)notify_set_itimer_func(&mt_mailclient, mt_itimer,
		ITIMER_REAL, &itv, 0);
	if (mt_load_messages == mt_At_Startup) {
		static struct itimerval itv = { {0, 0}, {0, 1} };

		/*
		 * Set up timer to go off once and immediately.
		 */
		mt_load_messages = mt_When_Asked;	/* mt_start_timer gets
							 * called again later.
							 * The mt_At_Startup
							 * only affects the
							 * first time */
		notify_set_itimer_func(&mt_mailclient, mt_itimer_doit,
		    ITIMER_VIRTUAL, &itv, 0);
	}
}

/*
 * Check for new mail when timer expires. 
 */
/* ARGSUSED */
static Notify_value
mt_itimer(client, which)
	Notify_client   client;
	int             which;
{
/* not used - hala	char           *p, *s; */

	mt_check_mail_box();

/****	int             interval, commit = FALSE;

	p = mt_value("commitfrequency");
	if (p != NULL && mt_del_count > atoi(p))
		commit = TRUE;
	if ((mt_check_mail_box() || commit)
	   && mt_system_mail_box
	   && (p = mt_value("retrieveinterval"))
	   && (interval = atoi(p)) > 0
	   ) {
		if (mt_current_time() > (interval + last_event_time)) {
			static struct itimerval itv;
			mt_update_info(commit ?
				"About to commit your changes and retrieve your new mail..."
				: "About to retrieve your new mail...");
			window_bell(mt_cmdpanel);
			itv.it_interval.tv_sec = 15;
			itv.it_value.tv_sec = 15;
			last_event_time = 0;
			(void)notify_set_itimer_func(&mt_mailclient, 
				mt_itimer1, ITIMER_REAL, &itv, 0);
		}
	}
*****/
 	return (NOTIFY_DONE);
}


/* called from mt_frame_event_proc, mt_start_timer */
/* ARGSUSED */
static Notify_value
mt_itimer_doit(client, which)
	Notify_client   client;
	int             which;
{
	Event	event;

	event_set_shiftmask(&event, 0);
	event_time(&event).tv_sec = 0;	/* indicates that this is a
					 * "synthetic" event */
	mail_load_messages(
		(mt_load_from_folder ? mt_folder_item : mt_state_item),
		&event, NULL);
	return (NOTIFY_DONE);
}

/* called from mt_frame_event_proc, mt_start_timer */
/* ARGSUSED */
static void
mail_load_messages(item, event, notify_proc)
	Panel_item      item;
	Event          *event;
	Void_proc       notify_proc;

{
	mt_idle = FALSE;
	mt_call_cmd_proc(item, event, notify_proc);
}


/* ARGSUSED */
static caddr_t
mt_just_open_proc(m, mi)
	Menu            m;
	Menu_item       mi;
{
	Event		event;

	event_set_shiftmask(&event, 0);
	event_time(&event).tv_sec = 0;	/* indicates that this is a
					 * "synthetic" event */
	mt_idle = FALSE;
	(void) window_set(mt_frame, FRAME_CLOSED, FALSE, 0);
}

/* ARGSUSED */
static caddr_t
mt_open_and_compose_proc(m, mi)
	Menu            m;
	Menu_item       mi;
{
	Event		event;
	int		always_use_popup = mt_always_use_popup;

	event_set_shiftmask(&event, 0);
	event_time(&event).tv_sec = 0;	/* indicates that this is a
					 * "synthetic" event */
	mt_always_use_popup = TRUE;
	mt_comp_proc(mt_compose_item, &event);
	mt_always_use_popup = always_use_popup;
}

/* ARGSUSED */
static caddr_t
mt_open_and_read_proc(m, mi)
	Menu            m;
	Menu_item       mi;
{
	mt_idle = TRUE;
	(void) window_set(mt_frame, FRAME_CLOSED, FALSE, 0);
}

/* not used - hala 
static Menu
mt_switch_folder_gen_proc(mi, operation)
	Menu_item       mi;
	Menu_generate   operation;
{
	Menu            pull_right, parent;
	struct Menu_client_data *menu_data;

	mt_folder_menu = mt_get_folder_menu(mt_folder_menu, "", "");
	return (mt_folder_menu);
}
*/

static void
mt_msgsw_notify_proc(textsw, attributes)
	Textsw          textsw;
	Attr_avlist     attributes;
{
	int             pass_on = FALSE;
	Attr_avlist     attrs;

	for (attrs = attributes; *attrs; attrs = attr_next(attrs)) {
		switch ((Textsw_action) (*attrs)) {
			case  TEXTSW_ACTION_EDITED_MEMORY:
				if (!mt_value("editmessagewindow") &&
					!mt_confirm(mt_frame, TRUE, TRUE,
						"Confirm",
						"Cancel",
				"Did you really intend to edit this window?",
						0)) {
					window_set(textsw,
						TEXTSW_RESET_TO_CONTENTS, 0);
					return;
					}
			default:
				pass_on = TRUE;
				break;
		}
	}
	if (pass_on)
		cached_mt_msgsw_notify(textsw, attributes);
}

/****
 *
 * Used for autoreading. Checks to see if user has performed any activity in
 * last 15 seconds, thereby vetoing autoread 
 *
static Notify_value
mt_itimer1(client, which)
	Notify_client   client;
	int             which;
{
	int             interval;

	interval = atoi(mt_value("retrieveinterval"));
	if (last_event_time == 0) {
		 *
		 * user may have performed some activity during 15 second
		 * period in order to prevent committing/retrieving new mail 
		 *
		Event	event;

		if (mt_debugging)
			printf("retrieving mail");
		mt_update_info("");
		event_set_shiftmask(&event, 0);
		mail_load_messages(mt_state_item, &event,
			mt_auto_retrieve_mail_proc);
		last_event_time = mt_current_time();

	} 
	* restore timer *
	mt_start_timer();
	return (NOTIFY_DONE);
}

****/
