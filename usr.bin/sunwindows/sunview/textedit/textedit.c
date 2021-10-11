#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textedit.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * This textedit is the envelope that binds various text display and
 *   editing facilities together.
 */

#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */
#include <sys/dir.h>   /* MAXNAMLEN */
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <locale.h>
#include <strings.h>
#include <sunwindow/defaults.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/sunview.h>
#include <suntool/scrollbar.h>
#include <suntool/text.h>
#ifdef ecd.help
#include <suntool/help.h>
#endif

/* performance: global cache of getdtablesize() */
extern int dtablesize_cache;
#define GETDTABLESIZE() \
	(dtablesize_cache?dtablesize_cache:(dtablesize_cache=getdtablesize()))

long	textsw_store_file();
char	*getwd(), *sprintf();
void	frame_cmdline_help();

static int		off();
static Notify_value	mysigproc();

static Frame		base_frame;
static char		current_filename[MAXNAMLEN];
static char		current_directory[MAXPATHLEN];
static int		handling_signal;
static Textsw		textsw;
static char		*cmd_name;
static int		user_label;
static int		caps_lock_on;
static int		edited;
static int      	read_only;

static short edit_ic_image[256]={
#include <images/textedit.icon>
};
mpr_static(edit_icon_mpr, ICON_DEFAULT_WIDTH, ICON_DEFAULT_HEIGHT, 1,
    edit_ic_image);

/*
 * The textedit command line options.
 */
static char	*option_names[] = {
			"adjust_is_pending_delete", "Ea",
			"auto_indent", "Ei",
			"okay_to_overwrite", "Eo",
			"lower_context", "EL",
			"margin", "Em",
			"multi_click_space", "ES",
			"multi_click_timeout", "ET",
			"number_of_lines", "En",
			"read_only", "Er",
			"scratch_window", "Es",
			"tab_width", "Et",
			"history_limit", "Eu",
			"upper_context", "EU",
			"checkpoint", "Ec",
#ifdef DEBUG
			"malloc_debug_level", "Ed",
			"edit_log_wraps_at", "Ew",
#endif
			0  /* Terminator! */  };

#define OPTION_ADJUST_IS_PD		(1<<0)
#define OPTION_AUTO_INDENT		(1<<1)
#define OPTION_ALWAYS_OVERWRITE		(1<<2)
#define OPTION_LOWER_CONTEXT		(1<<3)
#define OPTION_MARGIN			(1<<4)
#define OPTION_MULTI_CLICK_SPACE	(1<<5)
#define OPTION_MULTI_CLICK_TIMEOUT	(1<<6)
#define OPTION_NUMBER_OF_LINES		(1<<7)
#define OPTION_READ_ONLY		(1<<8)
#define OPTION_SCRATCH_WINDOW		(1<<9)
#define OPTION_TAB_WIDTH		(1<<10)
#define OPTION_UNDO_HISTORY		(1<<11)
#define OPTION_UPPER_CONTEXT		(1<<12)
#define OPTION_CHECKPOINT_FREQUENCY	(1<<13)

#ifdef DEBUG
#define OPTION_MALLOC_DEBUG_LEVEL	(1<<14)
#define OPTION_EDIT_LOG_WRAPS_AT	(1<<15)
#endif

#ifdef TEXTEDIT_HELP_STRING
static char	*help_msg()
/*
 * This routine is required because the help_msg is too big for the
 *   compiler to accept as a single string token.
 */
{
	extern char	*calloc();
	char		*result = calloc(1, 5000);
	
	(void)sprintf(result, "\n\t\tHelp for %s\n%s\n%s\n%s\n",
		window_get(base_frame, FRAME_LABEL),
"Mouse buttons: left is point, middle is adjust, right is menu.\n\
	Multi-click is only implemented for point.\n\
	Adjust-at-end-char is char-adjust.\n\
	CONTROL-select is pending-delete select, e.g. CONTROL-GET is MOVE.\n\
Function keys:",
#ifdef VT_100_HACK
"	L1 (unused)			L2 (or PF2) is AGAIN\n\
	L3 (unused)			L4 (or PF1) is UNDO\n\
	L5 TOP				L6 is PUT\n\
	L7 OPEN				L8 is GET\n\
	L9 (or ^F) is FIND		L10 (or ^D) is DELETE\n\
	^S copies the primary selection.\n\
	GET with no secondary selection (or ^G) is GET from SHELF.\n\
VT-100 compatibility: SHIFT-select is GET.\n\
Other SHIFTs:",
#else
"	L1 STOP				L2 is AGAIN\n\
	L3 (unused)			L4 is UNDO\n\
	L5 TOP				L6 is PUT\n\
	L7 OPEN				L8 is GET\n\
	L9 (or ^F) is FIND		L10 (or ^D) is DELETE\n\
	^P is an accelerator for the 'Put, then Get' menu item.\n\
	GET with no secondary selection (or ^G) is GET from SHELF.\n\
SHIFTs:",
#endif
"	SHIFT-back_char/word/line is forward_char/word/line.\n\
	SHIFT-TOP is BOTTOM; SHIFT-OPEN is CLOSE.\n\
Menu item notes: Save leaves old file as <file>%\n\
	Store writes to selected name, and then edits it.\n\
Startup: \"textedit <options> <file>\" is supported.\n\
Options are [where (E<letter>) indicates a short alternative]:\n\
	adjust_is_pending_delete (Ea) [on]	okay_to_overwrite (Eo) [on]\n\
	auto_indent (Ei) [off]			read_only (Er) [on]\n\
	lower_context (EL) <int>		scratch_window (Es) <int>\n\
	margin (Em) <int>			tab_width (Et) <int>\n\
	multi_click_space (ES) <int>		history_limit (Eu) <int>\n\
	multi_click_timeout (ET) <int>		upper_context (EU) <int>\n\
	number_of_lines (En) <int>\n\
Use defaultsedit to permanently set all of the above (and additional)\n\
	options for all text subwindows.\n\
Saving your edits in case of disaster:\n\
	If the textedit runs wild, send it a SIGHUP to force a Store."
	);
	return(result);
}
#endif

/*
 * Return pointer to longest suffix not beginning with '/'
 */
static char *
base_name(full_name)
	char *full_name;
{
	extern char	*rindex();
	register char	*temp;

	if ((temp = rindex(full_name, '/')) == NULL)
	    return(full_name);
	else
	    return(temp+1);
}

static
set_name_frame(textsw_local, attributes)
	Textsw		 textsw_local;
	Attr_avlist	 attributes;
{
	char		 frame_label[50+MAXNAMLEN+MAXPATHLEN];
	Icon		 edit_icon;
	char		 icon_text[sizeof(frame_label)];
	char		 *ptr;
	int		 len, pass_on = 0, repaint = 0;
	int		 was_read_only = read_only;
	Attr_avlist	 attrs;

	if (handling_signal)
	    return;
	icon_text[0] = '\0';
	ptr = icon_text;
	for (attrs = attributes; *attrs; attrs = attr_next(attrs)) {
	    repaint++;	/* Assume this attribute needs a repaint. */
	    switch ((Textsw_action)(*attrs)) {
	      case TEXTSW_ACTION_CAPS_LOCK:
		caps_lock_on = (int)attrs[1];
		ATTR_CONSUME(*attrs);
		break;
	      case TEXTSW_ACTION_CHANGED_DIRECTORY:
		switch (attrs[1][0]) {
		  case '/':
		    (void)strcpy(current_directory, attrs[1]);
		    break;
		  case '.':
		    if (attrs[1][1] != '\0')
			(void) getwd(current_directory);
		    break;
		  case '\0':
		    break;
		  default:
		    (void)strcat(current_directory, "/");
		    (void)strcat(current_directory, attrs[1]);
		    break;
		}
		ATTR_CONSUME(*attrs);
		break;
	      case TEXTSW_ACTION_USING_MEMORY:
		(void)strcpy(current_filename, "(NONE)");
		(void)strcpy(icon_text, "NO FILE");
		edited = read_only = 0;
		ATTR_CONSUME(*attrs);
		break;
	      case TEXTSW_ACTION_LOADED_FILE:
		(void)strcpy(current_filename, attrs[1]);
		edited = read_only = 0;
		goto Update_icon_text;
	      case TEXTSW_ACTION_EDITED_FILE:
		edited = 1;
		*ptr++ = '>';
Update_icon_text:
		len = (strlen(attrs[1]) > sizeof(icon_text) - 2) ?
		    sizeof(icon_text) - 2 : strlen(attrs[1]);
		    /* need 1 char for edit/not, 1 for null */
		(void)strncpy(ptr, attrs[1], len); ptr[len] = '\0';
		(void)strcpy(ptr, base_name(ptr));	/* strip path */
		ATTR_CONSUME(*attrs);
		break;
	      default:
		pass_on = 1;
		repaint--;	/* Above assumption was wrong. */
		break;
	    }
	}
	if (pass_on)
	    (void)textsw_default_notify(textsw_local, attributes);
	if (repaint) {
	    (void)sprintf(frame_label, "%stextedit - %s%s, dir: %s",
		    (caps_lock_on) ? "[CAPS] " : "",
		    current_filename,
		    (was_read_only) ? " (read only)"
			: (edited) ? " (edited)" : "",
		    current_directory);
	    (void)window_set(base_frame, FRAME_LABEL, frame_label, 0);
	    if (icon_text[0] != '\0') {
		struct rect	text_rect, *icon_rect;
		struct pixfont	*font;
		
		edit_icon = window_get(base_frame, FRAME_ICON);
		icon_rect = (Rect *) (LINT_CAST(icon_get(edit_icon, ICON_IMAGE_RECT)));
		font = (struct pixfont *) (LINT_CAST(icon_get(edit_icon, ICON_FONT)));
		ptr = (user_label) ?
			(char *) icon_get(edit_icon, ICON_LABEL) : icon_text;
		
		/* adjust icon text top/height to match font height */
		text_rect.r_height = font->pf_defaultsize.y;
		text_rect.r_top =
		    icon_rect->r_height - (text_rect.r_height + 2);

		/* center the icon text */
		text_rect.r_width = strlen(ptr)*font->pf_defaultsize.x;
		if (text_rect.r_width > icon_rect->r_width)
		    text_rect.r_width = icon_rect->r_width;
		text_rect.r_left = (icon_rect->r_width-text_rect.r_width)/2;

		(void)icon_set(edit_icon,
		    ICON_LABEL,		ptr,
		    ICON_LABEL_RECT,	&text_rect,
		    0);
		/* window_set actually makes a copy of all the icon fields */
		(void)window_set(base_frame, FRAME_ICON, edit_icon, 0);
	    }
	}
}

static Notify_value
my_destroy_func(client, status)
    Notify_client	client;
    Destroy_status	status;
{
    int		windowfd = (int)window_get((Window)(LINT_CAST(textsw)), WIN_FD);

    if (status == DESTROY_CHECKING) {
        if (window_get((Window)(LINT_CAST(textsw)), TEXTSW_MODIFIED)) {
	    int		result;
	    Event	event;

	    int		editing_memory = (int)((char *)textsw_get(
			    textsw, TEXTSW_FILE) == (char *)0);
	    if (editing_memory) {
		result = alert_prompt(
		        (Frame)base_frame,
		        &event,
		        ALERT_MESSAGE_STRINGS,
			    "The text has been edited, but you are currently",
			    "not editing a file. To save your work, push \"Cancel\",",
			    "select a file name, and choose Store as New File.",
			    "Quitting from this window will discard your edits.",
		            0,
		        ALERT_BUTTON_YES,	"Quit, discard edits",
		        ALERT_BUTTON_NO,	"Cancel",
		        0);
		if (result == ALERT_YES) {
		    (void) textsw_reset(textsw, 0, 0);
		    return(notify_next_destroy_func(client, status));
		} else if (result == ALERT_NO) {
		    (void) notify_veto_destroy((Notify_client)(LINT_CAST(client)));
		    return(NOTIFY_DONE);
		}
	    } else {
		result = alert_prompt(
		        (Frame)base_frame,
		        &event,
		        ALERT_MESSAGE_STRINGS,
		            "The text has been edited.",
			    "You may either save or discard your",
			    "edits before Quitting.",
			    " ",
			    "Otherwise, push \"Cancel\" to cancel Quitting.",
		            0,
		        ALERT_BUTTON_YES,  "Save edits",
			ALERT_BUTTON,	   "Discard edits", 99,
			ALERT_BUTTON_NO,   "Cancel",
		        0);
		if (result == 99) {/* Undo All Edits, then Quit */
		    (void) textsw_reset(textsw, 0, 0);
		    return(notify_next_destroy_func(client, status));
		} else if (result == ALERT_YES) {/* Save, then Quit */
		    (void) textsw_save(textsw, 0, 0);
		    return(notify_next_destroy_func(client, status));
		} else {/* Cancel */
		    (void) notify_veto_destroy(
			(Notify_client)(LINT_CAST(client)));
		    return(NOTIFY_DONE);
		}
	    }
	} else {
	    int		result;
	    Event	event;

	    result = alert_prompt(
	            (Frame)base_frame,
	            &event,
	            ALERT_MESSAGE_STRINGS,
	                "Are you sure you want to Quit?",
	                0,
	            ALERT_BUTTON_YES,	"Confirm",
		    ALERT_BUTTON_NO,	"Cancel",
		    ALERT_OPTIONAL,	1,
		    ALERT_NO_BEEPING,	1,
	            0);
	    if (result == ALERT_YES) {
	        return(notify_next_destroy_func(client, status));
	    } else {
	        (void) notify_veto_destroy(
	    	    (Notify_client)(LINT_CAST(client)));
	        return(NOTIFY_DONE);
	    }
	}
    }
    return(notify_next_destroy_func(client, status));
}

static void
my_frame_help(name)
	char	*name;
{
	frame_cmdline_help(name);
#ifdef TEXTEDIT_HELP_STRING
	(void)fprintf(stderr,
		"\nFor further information, use the switch -text_help.\n");
#endif
}

#if	(defined(STANDALONE) || defined(DEBUG) || defined(GPROF))
main(argc, argv)
	int	  argc;
	char	**argv;
{
#ifdef	GPROF
	if (argc > 1 && strcmp(argv[argc-1], "-gprof") == 0) {
	    moncontrol(1);
	    /* Pull the -gprof out of argc/v */
	    argc--;
	    argv[argc] = (char *)0;
	} else {
	    moncontrol(0);
	}
#endif	GPROF
	setlocale(LC_ALL, "");
	textedit_main(argc, argv);
}
#endif	(defined(STANDALONE) || defined(DEBUG) || defined(GPROF))

textedit_main(argc, argv)
	int	  argc;
	char	**argv;
{
#define	GET_INT_ATTR_VAL(var)						\
	if (argc > 0) {var = (caddr_t) atoi(argv[1]); argc--, argv++;}

	extern struct pixfont	 *pw_pfsysopen();

	Icon			  edit_icon;
	Textsw_status		  status;
	int			  scratch_lines = 0;
	int			  checkpoint = 0;
	int			  margin;
	int			  number_of_lines;
	int			  optioncount = 
				   sizeof(option_names)/sizeof(option_names[0]);
	struct stat           	  stb;
	caddr_t			  textsw_attrs[ATTR_STANDARD_SIZE];
	int			  attrc = 0;
	char			 *file_to_edit = NULL;
#ifdef DEBUG
	caddr_t			  edit_log_wraps_at = (caddr_t)TEXTSW_INFINITY;
#endif

	int	fd, max_fds = GETDTABLESIZE();
#define STDERR  2

	/*
	 * Init data
	 * Implicitely zeroed:	caps_lock_on, handling_signal,
	 *			read_only, edited
	 */

	edit_icon = icon_create(ICON_IMAGE, &edit_icon_mpr, 0);

	/* Flush inherited windowfds */
	for (fd = STDERR+1; fd < max_fds; fd++)  
	    (void)close(fd);

	cmd_name = *argv;		/* Must be BEFORE calls on die() */
	current_filename[0] = '\0';
	(void) getwd(current_directory);
	    /* Error message is placed into current_directory by getwd */

	scratch_lines =
	    defaults_get_integer_check("/Text/Scratch_window", 1,
	    0, 30, (int *)NULL);
	checkpoint =
	    defaults_get_integer_check("/Text/Checkpoint_frequency", 0,
	    0, (int)TEXTSW_INFINITY, (int *)NULL);

	/*
	 * Create the base frame.
	 * WIN_WIDTH & WIN_HEIGHT=0 used as flags to see if width
	 * or height set in user's argc,argv!
	 * If width still zero later, we'll set it to 80 chars.
	 * If height still zero later, we'll set it to number_of_lines rows.
	 */
	base_frame = window_create((Window)NULL, FRAME,
	    WIN_WIDTH,			0,
	    WIN_HEIGHT,			0,
	    WIN_ERROR_MSG,		"Unable to create frame\n",
	    FRAME_NO_CONFIRM,		TRUE,
	    FRAME_ICON,			edit_icon,
	    FRAME_LABEL,		"textedit",
	    FRAME_SHOW_LABEL,		TRUE,
	    FRAME_CMDLINE_HELP_PROC,	my_frame_help,
	    FRAME_ARGC_PTR_ARGV,	&argc, argv,
#ifdef ecd.help
	    HELP_DATA,			"sunview:textedit",
#endif
	    0);
	if (base_frame == NULL) {
		(void)fprintf(stderr,"Unable to create frame\n");
		exit(1);
	}
	number_of_lines = window_get((Window)(LINT_CAST(base_frame)), WIN_HEIGHT) ? 0 : 45;

	/*
	 * Set icon's font to system font [if user hasn't set icon font],
	 * but AFTER window_create has a chance to change it from the
	 * built-in font.
	 * If the user supplies a label, use it and don't override
	 * with our's later.
	 * Note that we get the icon from the Frame in case user
	 * over-rides via argc, argv!
	 */
	edit_icon = window_get(base_frame, FRAME_ICON);
	user_label = (int)icon_get(edit_icon, ICON_LABEL);
	if (!icon_get(edit_icon, ICON_FONT)) {
	    (void)icon_set(edit_icon, ICON_FONT, pw_pfsysopen(), 0);
	    if (!icon_get(edit_icon, ICON_FONT))
		die("Cannot get default font.\n", (char *)NULL, (char *)NULL);
	    (void)window_set(base_frame, FRAME_ICON, edit_icon, 0);
	}

	/*
	 * Interpose on the destroy function so we can tell user
	 * what to do.  (Avoids default veto message)
	 */
	(void) notify_interpose_destroy_func(base_frame, my_destroy_func);

	/*
	 * Pick up command line arguments to modify textsw behavior.
	 * Notes: FRAME_ARGC_PTR_ARGV above has stripped window flags.
	 *        case OPTION_MARGIN is used to compute WIN_WIDTH.
	 */
#ifndef lint
	margin = (int)textsw_get(TEXTSW, TEXTSW_LEFT_MARGIN);
#endif
	argc--; argv++;				/* Skip the cmd name */
	while ((argc--) && (attrc < ATTR_STANDARD_SIZE)) {
	    if (argv[0][0] == '-') {
		extern int	match_in_table();
		int		option =
				match_in_table(&(argv[0][1]), option_names);
		if (option < 0 || option >= optioncount) {
		    die(argv[0], " is not a valid option.\n", (char *)NULL);
		}
		switch (1<<(option/2)) {
		    case OPTION_NUMBER_OF_LINES:
			if (argc > 0) {
			    number_of_lines = atoi(argv[1]);
			    argc--, argv++;
			}
			break;
		    case OPTION_SCRATCH_WINDOW:
			if (argc > 0) {
			    scratch_lines = atoi(argv[1]);
			    argc--, argv++;
			}
			break;
		    case OPTION_READ_ONLY:
		 	read_only = 1;
		    	if ((argc > 0) && (argv[1][0] != '-')) {
			    argc--, argv++;
			    read_only = !off(argv[0]);
			}
			break;
			
		    case OPTION_AUTO_INDENT:
			textsw_attrs[attrc++] = (caddr_t) TEXTSW_AUTO_INDENT;
			textsw_attrs[attrc] = (caddr_t) 1;
			if ((argc > 0) && (argv[1][0] != '-')) {
			    argc--, argv++;
			    textsw_attrs[attrc] = (caddr_t) !off(argv[0]);
			}
			attrc++;
			break;
		    case OPTION_ADJUST_IS_PD:
		    	textsw_attrs[attrc++] =
		    		(caddr_t) TEXTSW_ADJUST_IS_PENDING_DELETE;
		    	textsw_attrs[attrc] = (caddr_t) 1;
			if ((argc > 0) && (argv[1][0] != '-')) {
			    argc--, argv++;
			    textsw_attrs[attrc] = (caddr_t) !off(argv[0]);
			}
			attrc++;
			break;
		    case OPTION_ALWAYS_OVERWRITE:
		 	textsw_attrs[attrc++] =
		 		(caddr_t) TEXTSW_CONFIRM_OVERWRITE;
		 	textsw_attrs[attrc] = (caddr_t) 0;
		    	if ((argc > 0) && (argv[1][0] != '-')) {
			    argc--, argv++;
			    textsw_attrs[attrc] = (caddr_t) off(argv[0]);
			}
			attrc++;
			break;
#ifdef DEBUG
		    case OPTION_EDIT_LOG_WRAPS_AT:
			GET_INT_ATTR_VAL(edit_log_wraps_at)
			break;
#endif
		    case OPTION_LOWER_CONTEXT:
			textsw_attrs[attrc++] = (caddr_t) TEXTSW_LOWER_CONTEXT;
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
#ifdef DEBUG
		    case OPTION_MALLOC_DEBUG_LEVEL:
			textsw_attrs[attrc++] =
				(caddr_t) TEXTSW_MALLOC_DEBUG_LEVEL;
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
#endif
		    case OPTION_MARGIN:
			textsw_attrs[attrc++] =
				(caddr_t) TEXTSW_LEFT_MARGIN;
			margin = atoi(argv[1]);
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
		    case OPTION_MULTI_CLICK_SPACE:
			textsw_attrs[attrc++] =
				(caddr_t) TEXTSW_MULTI_CLICK_SPACE;
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
		    case OPTION_MULTI_CLICK_TIMEOUT:
			textsw_attrs[attrc++] =
				(caddr_t) TEXTSW_MULTI_CLICK_TIMEOUT;
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
		    case OPTION_TAB_WIDTH:
			textsw_attrs[attrc++] = (caddr_t) TEXTSW_TAB_WIDTH;
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
		    case OPTION_UNDO_HISTORY:
			textsw_attrs[attrc++] = (caddr_t) TEXTSW_HISTORY_LIMIT;
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
		    case OPTION_UPPER_CONTEXT:
			textsw_attrs[attrc++] = (caddr_t) TEXTSW_UPPER_CONTEXT;
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
		    case OPTION_CHECKPOINT_FREQUENCY:
			if (argc > 0) {
			    checkpoint = atoi(argv[1]);
			    argc--, argv++;
			}
			break;
		    default:
			die("Unrecognized command line option.", (char *)NULL, (char *)NULL);
			break;
		}
	    } else if (file_to_edit == NULL) {
		file_to_edit = argv[0];
	    } else {
		die("Too many files specified.", (char *)NULL, (char *)NULL);
	    }

	    argv++;
	}
	textsw_attrs[attrc] = 0;	/* A-V list terminator */

	/*
	 * Set width to 80 chars if not set by user.
	 */
/* Add term of charwidth-1 to insure that there is enough width */
	if (!window_get(base_frame, WIN_WIDTH))
	    (void) window_set(base_frame,
		WIN_WIDTH, 81 * (int) window_get(base_frame, WIN_COLUMN_WIDTH) - 1 +
		margin + 2 +
		scrollbar_get((Scrollbar) (LINT_CAST(SCROLLBAR)), SCROLL_THICKNESS) +
		(int) window_get(base_frame, WIN_LEFT_MARGIN) +
		(int) window_get(base_frame, WIN_RIGHT_MARGIN),
		0);
	else
	    (void) window_set(base_frame,
		WIN_WIDTH, ((int) window_get(base_frame, WIN_COLUMNS) + 1)
		* (int) window_get(base_frame, WIN_COLUMN_WIDTH) - 1 + margin + 2
		+ scrollbar_get((Scrollbar) (LINT_CAST(SCROLLBAR)), SCROLL_THICKNESS)
		+ (int) window_get(base_frame, WIN_LEFT_MARGIN)
		+ (int) window_get(base_frame, WIN_RIGHT_MARGIN),
		0);

	/*
	 * Create subwindows
	 */
	if (scratch_lines > 0) {
	    textsw = (Textsw)(LINT_CAST(
	    	window_create((Window)base_frame, TEXTSW,
		    ATTR_LIST,			textsw_attrs,
		    TEXTSW_STATUS,		&status,
		    TEXTSW_CONTENTS,		"Scratch window",
		    TEXTSW_DISABLE_CD,		1,
		    TEXTSW_DISABLE_LOAD,	1,
		    TEXTSW_IGNORE_LIMIT,	TEXTSW_INFINITY,
		    WIN_ROWS,			scratch_lines,
                    WIN_ERROR_MSG,              "Unable to create subwindow\n",
		    0)));
	    switch (status) {
	      case TEXTSW_STATUS_OKAY:
		if (textsw)
		    break;
		/* else fall through */
	      default:
		die("Cannot create textsw, exiting!\n", (char *)NULL, (char *)NULL);
	    }
	}
	read_only = (read_only & (file_to_edit != NULL));
#ifdef DEBUG
	if (edit_log_wraps_at != (caddr_t)TEXTSW_INFINITY) {
	    textsw_attrs[attrc++] = (caddr_t) TEXTSW_WRAPAROUND_SIZE;
	    textsw_attrs[attrc++] = edit_log_wraps_at;
	    textsw_attrs[attrc] = 0;
	}
#endif
	if ((file_to_edit != NULL) && (stat(file_to_edit, &stb) < 0)) {
	       int	result;
	       Event	event;
	       char	msg[200];
	       
	       (void) sprintf(msg, "Filename '%s' does not exist.", 
                              file_to_edit);
	       result = alert_prompt(
		    (Frame)NULL,
		    (Event *)NULL,
		    ALERT_MESSAGE_STRINGS,
		       msg,
		       "Please confirm creation of new",
		       "file for textedit.",
		       0,
		    ALERT_BUTTON_YES,	"Confirm",
		    ALERT_BUTTON_NO,	"Cancel",
		    0);
	       if (result == ALERT_YES) {
	   	   if (open(file_to_edit, O_CREAT, 0666) == -1)
	               die("Cannot create new file, exiting!\n", 
	                   (char *)NULL, (char *)NULL);
	       } else exit(4);
	}
	textsw = (Textsw)(LINT_CAST(window_create(base_frame, TEXTSW,
		ATTR_LIST,			textsw_attrs,
		TEXTSW_STATUS,			&status,
		TEXTSW_READ_ONLY,		read_only,
		TEXTSW_FILE,			file_to_edit,
		TEXTSW_NOTIFY_PROC,		set_name_frame,
		TEXTSW_CHECKPOINT_FREQUENCY,	checkpoint,
                WIN_ERROR_MSG,                  "Unable to create subwindow\n",
	    	0)));
	switch (status) {
	  case TEXTSW_STATUS_CANNOT_OPEN_INPUT:
	    die("Cannot open file '", file_to_edit, "', exiting!\n");
	  case TEXTSW_STATUS_OKAY:
	    if (textsw)
		break;
	    /* else fall through */
	  default:
	    die("Cannot create textsw, exiting!\n", (char *)NULL, (char *)NULL);
	}
	/*
	 * Setup signal handlers.
	 */
	(void)notify_set_signal_func(base_frame, mysigproc, SIGINT,  NOTIFY_ASYNC);
	(void)notify_set_signal_func(base_frame, mysigproc, SIGXCPU, NOTIFY_ASYNC);
	(void)notify_set_signal_func(base_frame, mysigproc, SIGBUS,  NOTIFY_ASYNC);
	(void)notify_set_signal_func(base_frame, mysigproc, SIGHUP,  NOTIFY_ASYNC);
	(void)notify_set_signal_func(base_frame, mysigproc, SIGILL,  NOTIFY_ASYNC);
	(void)notify_set_signal_func(base_frame, mysigproc, SIGSEGV, NOTIFY_ASYNC);
	(void)notify_set_signal_func(base_frame, mysigproc, SIGFPE,  NOTIFY_ASYNC);
	/*
	 * Install us in tree of windows
	 */
	if (number_of_lines) {
	    (void)window_set((Window)(LINT_CAST(textsw)), WIN_ROWS, number_of_lines, 0);
	    (void)window_fit_height(base_frame);
	}
	window_main_loop(base_frame);
	exit(0);
}

/*
 *	SIGNAL handlers
 */

/* ARGSUSED */
static Notify_value
mysigproc(me, sig, when)
	Notify_client		 me;
	int			 sig;
	Notify_signal_mode	 when;
{
	char			 name_to_use[MAXNAMLEN];
	int			 pid = getpid();
	int			 was_SIGILL = (sig == SIGILL);
	struct sigvec vec;

	if (handling_signal == 2)
	    _exit(3);
	if (handling_signal++ == 1) {
	    (void)fprintf(stderr, "Signal catcher called recursively: ");
	    goto Die;
	}
	if (sig == SIGINT) {
	    if (window_get((Window)(LINT_CAST(textsw)), TEXTSW_MODIFIED)) {
		(void)window_destroy(base_frame);	/* It will be vetoed */
		handling_signal = 0;
	    } else {
		/* Skip more user confirmation - just die (but cleanly)! */
		(void) notify_post_destroy(base_frame, DESTROY_PROCESS_DEATH,
					   NOTIFY_IMMEDIATE);
		(void) notify_stop();
	    }
	    return(NOTIFY_DONE);
	}
	(void)sprintf(name_to_use, "textedit.%d", pid);
	(void)fprintf(stderr, "attempting Store to %s ... ", name_to_use);
	(void)fflush(stderr);
	if (textsw_store_file(textsw, name_to_use, 0, 0) == 0)
	    goto Done;
	(void)sprintf(name_to_use, "/usr/tmp/textedit.%d", pid);
	(void)fprintf(stderr, "failed!\nAttempting Store to %s ... ", name_to_use);
	(void)fflush(stderr);
	if (textsw_store_file(textsw, name_to_use, 0, 0) == 0)
	    goto Done;
	(void)sprintf(name_to_use, "/tmp/textedit.%d", pid);
	(void)fprintf(stderr, "failed!\nAttempting Store to %s ... ", name_to_use);
	(void)fflush(stderr);
	if (textsw_store_file(textsw, name_to_use, 0, 0) == 0)
	    goto Done;
	(void)fprintf(stderr, "failed!\nSorry, cannot save your edits: ");
	    goto Die;
Done:
	(void)fprintf(stderr, "finished; ");
Die:
	(void)fprintf(stderr, "aborting for post-mortem ...\n");
	(void)fflush(stderr);
	(void)sigsetmask(0);			/* Make sure signals get through */
	if (was_SIGILL) {
#ifndef lint
	    char	dummy, *bad_ptr = 0;
	    /* (void)signal(SIGSEGV, SIG_DFL);	/* Make sure 0 deref dumps. */
	    vec.sv_handler = SIG_DFL;
	    vec.sv_mask = vec.sv_onstack = 0;
	    sigvec(SIGSEGV, &vec, 0);

	    dummy = *bad_ptr;
#endif
	} else {
	    /* (void)signal(SIGILL, SIG_DFL);	/* Make sure abort() dumps. */
            vec.sv_handler = SIG_DFL; 
	    vec.sv_mask = vec.sv_onstack = 0; 
            sigvec(SIGILL, &vec, 0);

	    abort();
	}
	return(NOTIFY_DONE);
}


/*
 * Misc. utilities
 */
static
die(msg1, msg2, msg3)
	char	*msg1, *msg2, *msg3;
{
	char	*dummy = "";
	(void)fprintf(stderr, "%s: %s%s%s\n", cmd_name, msg1,
			(msg2?msg2:dummy), (msg3?msg3:dummy));
	exit(4);
}

static int
off(str)
	char	*str;
{
    return ((strcmp(str, "off") == 0) ||
	    (strcmp(str, "Off") == 0) ||
	    (strcmp(str, "OFF") == 0)
	   );
}

