/*	@(#)filer.c 1.1 92/07/30 SMI	*/
/*****************************************************************************/
/*			4.0	filer.c					     */
/*****************************************************************************/

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/tty.h>
#include <suntool/textsw.h>
#include <suntool/seln.h>
#include <suntool/alert.h>
#include <sys/stat.h>	    /* stat call needed to verify existence of files */

/* these objects are global so their attributes can be modified or retrieved */
Frame		base_frame, edit_frame, ls_flags_frame;
Panel		panel, ls_flags_panel;
Tty		ttysw;
Textsw		editsw;
Panel_item	dir_item, fname_item, filing_mode_item, done_item;
int		quit_confirmed_from_panel;

#define		MAX_FILENAME_LEN	256
#define		MAX_PATH_LEN		1024

char *getwd();

main(argc, argv)
    int	   argc;
    char **argv;
{
    static Notify_value filer_destroy_func();
    void	ls_flags_proc();

    base_frame = window_create(NULL, FRAME,
		 FRAME_ARGS,		argc, argv,
		 FRAME_LABEL,		"filer",
		 FRAME_PROPS_ACTION_PROC, ls_flags_proc,
		 FRAME_PROPS_ACTIVE, TRUE,
		 FRAME_NO_CONFIRM,	TRUE,
		 0);
    (void) notify_interpose_destroy_func(base_frame, filer_destroy_func);

    create_panel_subwindow();
    create_tty_subwindow();
    create_edit_popup();
    create_ls_flags_popup();
    quit_confirmed_from_panel = 0;

    window_main_loop(base_frame);
    exit(0);
	/* NOTREACHED */
}

create_tty_subwindow()
{
    ttysw = window_create(base_frame, TTY, 0);
}

create_edit_popup()
{
    edit_frame = window_create(base_frame, FRAME,
		 FRAME_SHOW_LABEL, TRUE,
		 0);
    editsw = window_create(edit_frame, TEXTSW, 0);
}

create_panel_subwindow()
{
    void ls_proc(), ls_flags_proc(), quit_proc(), edit_proc(), 
	edit_sel_proc(), del_proc();

    char current_dir[MAX_PATH_LEN];

    panel = window_create(base_frame, PANEL, 0);

    (void) panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_X,			ATTR_COL(0),
	PANEL_LABEL_Y,			ATTR_ROW(0),
	PANEL_LABEL_IMAGE, 		panel_button_image(panel, "List Directory", 0, 0),
	PANEL_NOTIFY_PROC, 		ls_proc,
	0);

    (void) panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE, 		panel_button_image(panel, "Set ls flags", 0, 0),
	PANEL_NOTIFY_PROC, 		ls_flags_proc,
	0);

    (void) panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE, 		panel_button_image(panel, "Edit", 0, 0),
	PANEL_NOTIFY_PROC, 		edit_proc,
	0);

    (void) panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE, 		panel_button_image(panel, "Delete", 0, 0),
	PANEL_NOTIFY_PROC, 		del_proc,
	0);

    (void) panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE, 		panel_button_image(panel, "Quit", 0, 0),
	PANEL_NOTIFY_PROC, quit_proc,
	0);

    filing_mode_item = panel_create_item(panel, PANEL_CYCLE,
	PANEL_LABEL_X,			ATTR_COL(0),
	PANEL_LABEL_Y,			ATTR_ROW(1),
	PANEL_LABEL_STRING,		"Filing Mode:",
	PANEL_CHOICE_STRINGS,		"Use \"File:\" item",
					"Use Current Selection", 0,
	0);

    (void) panel_create_item(panel, PANEL_MESSAGE,
	PANEL_LABEL_X,			ATTR_COL(0),
	PANEL_LABEL_Y,			ATTR_ROW(2),
	0);

    dir_item = panel_create_item(panel, PANEL_TEXT,
	PANEL_LABEL_X,			ATTR_COL(0),
	PANEL_LABEL_Y,			ATTR_ROW(3),
	PANEL_VALUE_DISPLAY_LENGTH,	60,
	PANEL_VALUE,			getwd(current_dir),
	PANEL_LABEL_STRING,		"Directory: ",
	0);

    fname_item = panel_create_item(panel, PANEL_TEXT,
	PANEL_LABEL_X,			ATTR_COL(0),
	PANEL_LABEL_Y,			ATTR_ROW(4),
	PANEL_LABEL_DISPLAY_LENGTH,	60,
	PANEL_LABEL_STRING,		"File:  ",
	0);

    window_fit_height(panel);

    window_set(panel, PANEL_CARET_ITEM, fname_item, 0);
}

create_ls_flags_popup()
{
    void done_proc();
    ls_flags_frame = window_create(base_frame, FRAME, 0);

    ls_flags_panel = window_create(ls_flags_frame, PANEL, 0);

    panel_create_item(ls_flags_panel, PANEL_MESSAGE,
		PANEL_ITEM_X,		ATTR_COL(14),
		PANEL_ITEM_Y,		ATTR_ROW(0),
		PANEL_LABEL_STRING,	"Options for ls command",
		PANEL_CLIENT_DATA,	"   ",
		0);

    panel_create_item(ls_flags_panel, PANEL_CYCLE,
		PANEL_ITEM_X,		ATTR_COL(0),
		PANEL_ITEM_Y,		ATTR_ROW(1),
		PANEL_DISPLAY_LEVEL,	PANEL_CURRENT,
		PANEL_LABEL_STRING,	"Format:                          ",
		PANEL_CHOICE_STRINGS,	"Short", "Long", 0,
		PANEL_CLIENT_DATA,	" 1 ",
		0);

    panel_create_item(ls_flags_panel, PANEL_CYCLE,
		PANEL_ITEM_X,		ATTR_COL(0),
		PANEL_ITEM_Y,		ATTR_ROW(2),
		PANEL_DISPLAY_LEVEL,	PANEL_CURRENT,
		PANEL_LABEL_STRING,	"Sort Order:                      ",
		PANEL_CHOICE_STRINGS,	"Descending", "Ascending", 0,
		PANEL_CLIENT_DATA,	" r ",
		0);

    panel_create_item(ls_flags_panel, PANEL_CYCLE,
		PANEL_ITEM_X,		ATTR_COL(0),
		PANEL_ITEM_Y,		ATTR_ROW(3),
		PANEL_DISPLAY_LEVEL,	PANEL_CURRENT,
		PANEL_LABEL_STRING,	"Sort criterion:                  ",
		PANEL_CHOICE_STRINGS,	"Name", "Modification Time",
					"Access Time", 0,
		PANEL_CLIENT_DATA,	" tu",
		0);

    panel_create_item(ls_flags_panel, PANEL_CYCLE,
		PANEL_ITEM_X,		ATTR_COL(0),
		PANEL_ITEM_Y,		ATTR_ROW(4),
		PANEL_DISPLAY_LEVEL,	PANEL_CURRENT,
		PANEL_LABEL_STRING,	"For directories, list:           ",
		PANEL_CHOICE_STRINGS,	"Contents", "Name Only", 0,
		PANEL_CLIENT_DATA,	" d ",
		0);

    panel_create_item(ls_flags_panel, PANEL_CYCLE,
		PANEL_ITEM_X,		ATTR_COL(0),
		PANEL_ITEM_Y,		ATTR_ROW(5),
		PANEL_DISPLAY_LEVEL,	PANEL_CURRENT,
		PANEL_LABEL_STRING,	"Recursively list subdirectories? ",
		PANEL_CHOICE_STRINGS,	"No", "Yes", 0,
		PANEL_CLIENT_DATA,	" R ",
		0);

    panel_create_item(ls_flags_panel, PANEL_CYCLE,
		PANEL_ITEM_X,		ATTR_COL(0),
		PANEL_ITEM_Y,		ATTR_ROW(6),
		PANEL_DISPLAY_LEVEL,	PANEL_CURRENT,
		PANEL_LABEL_STRING,	"List '.' files?                  ",
		PANEL_CHOICE_STRINGS,	"No", "Yes", 0,
		PANEL_CLIENT_DATA,	" a ",
		0);

    panel_create_item(ls_flags_panel, PANEL_CYCLE,
		PANEL_ITEM_X,		ATTR_COL(0),
		PANEL_ITEM_Y,		ATTR_ROW(6),
		PANEL_DISPLAY_LEVEL,	PANEL_CURRENT,
		PANEL_LABEL_STRING,	"Indicate type of file?           ",
		PANEL_CHOICE_STRINGS,	"No", "Yes", 0,
		PANEL_CLIENT_DATA,	" F ",
		0);
    done_item = panel_create_item(ls_flags_panel, PANEL_BUTTON,
		PANEL_ITEM_X,		ATTR_COL(0),
		PANEL_ITEM_Y,		ATTR_ROW(7),
		PANEL_LABEL_IMAGE, 	panel_button_image(panel, "Done", 0, 0),
		PANEL_NOTIFY_PROC, 	done_proc,
		0);

    window_fit(ls_flags_panel); /* fit panel around its items */
    window_fit(ls_flags_frame); /* fit frame around its panel */
}

char *
compose_ls_options()
{
    static char  flags[20];
    char 	*ptr;
    char 	 flag;
    int 	 first_flag = TRUE;
    Panel_item	 item;
    char 	*client_data;
    int 	 index;

    ptr = flags;

    panel_each_item(ls_flags_panel, item)
	if (item != done_item) {
	    client_data = panel_get(item, PANEL_CLIENT_DATA, 0);
	    index = (int)panel_get_value(item);
	    flag = client_data[index];
	    if (flag != ' ') {
	        if (first_flag) {
		    *ptr++     = '-';
		    first_flag = FALSE;
	        }
	        *ptr++ = flag;
	    }
	}
    panel_end_each
    *ptr = '\0';
    return flags;
}

void
ls_proc()
{
    static char previous_dir[MAX_PATH_LEN];
    char *current_dir;
    char cmdstring[100];	/* dir_item's value can be 80, plus flags */

    current_dir = (char *)panel_get_value(dir_item);

    if (strcmp(current_dir, previous_dir)) {
	chdir((char *)panel_get_value(dir_item));
	strcpy(previous_dir, current_dir);
    }
    sprintf(cmdstring, "ls %s %s/%s\n",
		compose_ls_options(),
		current_dir,
		panel_get_value(fname_item));
    ttysw_input(ttysw, cmdstring, strlen(cmdstring));
}

void
ls_flags_proc()
{
    window_set(ls_flags_frame, WIN_SHOW, TRUE, 0);
}

void
done_proc()
{
    window_set(ls_flags_frame, WIN_SHOW, FALSE, 0);
}

/* return a pointer to the current selection */
char *
get_selection()
{
    static char	  filename[MAX_FILENAME_LEN];
    Seln_holder	  holder;
    Seln_request *buffer;

    holder = seln_inquire(SELN_PRIMARY);
    buffer = seln_ask(&holder, SELN_REQ_CONTENTS_ASCII, 0, 0);
    strncpy(
	filename, buffer->data + sizeof(Seln_attribute), MAX_FILENAME_LEN);
    return(filename);
}

/* return 1 if file exists, else print error message and return 0 */
stat_file(filename)
    char  *filename;
{
    static char previous_dir[MAX_PATH_LEN];
    char  *current_dir;
    char   this_file[MAX_PATH_LEN];
    struct stat statbuf;

    current_dir = (char *)panel_get_value(dir_item);

    if (strcmp(current_dir, previous_dir)) {
	chdir((char *)panel_get_value(dir_item));
	strcpy(previous_dir, current_dir);
    }
    sprintf(this_file, "%s/%s", current_dir, filename);
    if (stat(this_file, &statbuf) < 0) {
	char buf[MAX_FILENAME_LEN+11];  /* big enough for message */
	sprintf(buf, "%s not found.", this_file);
	msg(buf, 1);
	return 0;
    }
    return 1;
}

void
edit_proc()
{
    void edit_file_proc(), edit_sel_proc();
    int file_mode = (int)panel_get_value(filing_mode_item);

    if (file_mode) {
	(void)edit_sel_proc();
    } else {
	(void)edit_file_proc();
    }
}

void
edit_file_proc()
{
    char *filename;

    /* return if no selection */
    if (!strlen(filename = (char *)panel_get_value(fname_item))) {
	msg("Please enter a value for \"File:\".", 1);
	return;
    }

    /* return if file not found */
    if (!stat_file(filename))
	return;

    window_set(editsw, TEXTSW_FILE, filename, 0);

    window_set(edit_frame, FRAME_LABEL, filename, WIN_SHOW, TRUE, 0);
}

void
edit_sel_proc()
{
    char *filename;

    /* return if no selection */
    if (!strlen(filename = get_selection())) {
	msg("Please select a file to edit.", 0);
	return;
    }

    /* return if file not found */
    if (!stat_file(filename))
	return;

    window_set(editsw, TEXTSW_FILE, filename, 0);

    window_set(edit_frame, FRAME_LABEL, filename, WIN_SHOW, TRUE, 0);
}

void
del_proc()
{
    char    buf[300];
    char   *filename;
    int	    result;
    Event   event;   /* unused */
    int     file_mode = (int)panel_get_value(filing_mode_item);

    /* return if no selection */
    if (file_mode) {
	if (!strlen(filename = get_selection())) {
	    msg("Please select a file to delete.", 1);
	    return;
        }
    } else {
	if (!strlen(filename = (char *)panel_get_value(fname_item))) {
	    msg("Please enter a file name to delete.", 1);
	    return;
        }
    }

    /* return if file not found */
    if (!stat_file(filename))
	return;

    /* user must confirm the delete */
    result = alert_prompt(base_frame, &event,
	ALERT_MESSAGE_STRINGS,
	    "Ok to delete file:",
	    filename,
	    0,
	ALERT_BUTTON_YES,	"Confirm, delete file",
	ALERT_BUTTON_NO,	"Cancel",
	0);
    switch (result) {
	case ALERT_YES:
	    unlink(filename);
	    sprintf(buf, "%s deleted.", filename);
	    msg(buf, 0);
	    break;
	case ALERT_NO:
	    break;
	case ALERT_FAILED: /* not likely to happen unless out of memory */
	    sprintf(buf, "Ok to delete file %s?", filename);
	    result = confirm_yes(buf);
	    if (result) {
		unlink(filename);
		sprintf(buf, "%s deleted.", filename);
		msg(buf, 1);
	    }
	    break;
    }
}

int
confirm_quit()
{
    int	    result;
    Event   event;   /* unused */
    char    *msg = "Are you sure you want to Quit?";

    result = alert_prompt(base_frame, &event,
	ALERT_MESSAGE_STRINGS,
	    "Are you sure you want to Quit?",
	    0,
	ALERT_BUTTON_YES,	"Confirm",
	ALERT_BUTTON_NO,	"Cancel",
	0);
    switch (result) {
	case ALERT_YES:
	    break;
	case ALERT_NO:
	    return 0;
	case ALERT_FAILED: /* not likely to happen unless out of memory */
	    result = confirm_yes(msg);
	    if (!result) {
		return 0;
	    }
	    break;
    }
    return 1;
}

static Notify_value
filer_destroy_func(client, status)
    Notify_client	client;
    Destroy_status	status;
{
    if (status == DESTROY_CHECKING) {
	if (quit_confirmed_from_panel) {
	    return(notify_next_destroy_func(client, status));
	} else if (confirm_quit() == 0) {
	    (void) notify_veto_destroy((Notify_client)(LINT_CAST(client)));
    	    return(NOTIFY_DONE);
	}
    }
    return(notify_next_destroy_func(client, status));
}

void
quit_proc()
{
    if (confirm_quit()) {
	quit_confirmed_from_panel = 1;
        window_destroy(base_frame);
    }
}

msg(msg, beep)
    char *msg;
    int   beep;
{
    char    buf[300];
    int	    result;
    Event   event;   /* unused */
    char    *contine_msg = "Press \"Continue\" to proceed.";

    result = alert_prompt(base_frame, &event,
	ALERT_MESSAGE_STRINGS,
	    msg,
	    contine_msg,
	    0,
	ALERT_NO_BEEPING,  (beep) ? 0:1,
	ALERT_BUTTON_YES,  "Continue",
	ALERT_TRIGGER,	   ACTION_STOP, /* allow either YES or NO answer */
	0);
    switch (result) {
	case ALERT_YES:
	case ALERT_TRIGGERED: /* result of ACTION_STOP trigger */
	    break;
	case ALERT_FAILED: /* not likely to happen unless out of memory */
	    sprintf(buf, "%s  Press \"Continue\" to proceed.", msg);
	    result = confirm_ok(buf);
	    break;
    }
}

/* confirmer routines to be used if alert fails for any reason */

static Frame	init_confirmer();
static int	confirm();
static void	yes_no_ok();

int
confirm_yes(message)
    char	*message;
{
    return confirm(message, FALSE);
}

int
confirm_ok(message)
    char	*message;
{
    return confirm(message, TRUE);
}

static int
confirm(message, ok_only)
    char	*message;
    int		ok_only;
{
    Frame	confirmer;
    int		answer;

    /* create the confirmer */
    confirmer = init_confirmer(message, ok_only);
    /* make the user answer */
    answer = (int) window_loop(confirmer);
    /* destroy the confirmer */
    window_set(confirmer, FRAME_NO_CONFIRM, TRUE, 0);
    window_destroy(confirmer);
    return answer;
}

static Frame
init_confirmer(message, ok_only)
    char	*message;
    int		 ok_only;
{
    Frame		confirmer;
    Panel		panel;
    Panel_item		message_item;
    int			left, top, width, height;
    Rect		*r;
    struct pixrect	*pr;

    confirmer	 = window_create(0, FRAME, FRAME_SHOW_LABEL, FALSE, 0);
    panel	 = window_create(confirmer, PANEL, 0);
    message_item = panel_create_item(panel, PANEL_MESSAGE, 
			PANEL_LABEL_STRING, message, 0);

    if (ok_only) {
	pr = panel_button_image(panel, "Continue", 8, 0);
	width = pr->pr_width;
    } else {
	pr = panel_button_image(panel, "Cancel", 8, 0);
	width = 2 * pr->pr_width + 10;
    }

    /* center the yes/no or ok buttons under the message */
    r = (Rect *) panel_get(message_item, PANEL_ITEM_RECT);
    left = (r->r_width - width) / 2;
    if (left < 0)
	left = 0;
    top = rect_bottom(r) + 5;

    if (ok_only) {
	panel_create_item(panel, PANEL_BUTTON,
	    PANEL_ITEM_X, left, PANEL_ITEM_Y, top,
	    PANEL_LABEL_IMAGE, pr,
	    PANEL_CLIENT_DATA, 1,
	    PANEL_NOTIFY_PROC, yes_no_ok,
	    0);
    } else {
	panel_create_item(panel, PANEL_BUTTON,
	    PANEL_ITEM_X, left, PANEL_ITEM_Y, top,
	    PANEL_LABEL_IMAGE, pr,
	    PANEL_CLIENT_DATA, 0,
	    PANEL_NOTIFY_PROC, yes_no_ok,
	    0);
	panel_create_item(panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE, panel_button_image(panel, "Confirm", 8, 0),
	    PANEL_CLIENT_DATA, 1,
	    PANEL_NOTIFY_PROC, yes_no_ok,
	    0);
    }

    window_fit(panel);
    window_fit(confirmer);

    /* center the confirmer frame on the screen */
    r		= (Rect *) window_get(confirmer, WIN_SCREEN_RECT);
    width	= (int) window_get(confirmer, WIN_WIDTH);
    height	= (int) window_get(confirmer, WIN_HEIGHT);
    left	= (r->r_width - width) / 2;
    top		= (r->r_height - height) / 2;
    if (left < 0)
	left = 0;
    if (top < 0)
	top = 0;
    window_set(confirmer, WIN_X, left, WIN_Y, top, 0);

    return confirmer;
}

static void
yes_no_ok(item, event)
    Panel_item	 item;
    Event	*event;
{
    window_return(panel_get(item, PANEL_CLIENT_DATA));
}

