#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)cmdtool.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *  shelltool - run a process in a tty subwindow
 */

#include <stdio.h>
#include <locale.h>
#include <strings.h>
#include <sys/types.h>
#include <sunwindow/attr.h>
#include <sunwindow/defaults.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/sunview.h>
#include <suntool/scrollbar.h>
#include <suntool/tty.h>
#include <suntool/textsw.h>
#ifdef ecd.help
#include <suntool/help.h>
#endif

#define EXIT(n)		exit(n)

#define HEIGHTADJUST 19
#define SCRATCH_MIN_LEN 8096  /* min. wraparound edit log size */

extern	char *getenv();
static void adjust_frame_height();
extern  int  ttysw_is_already_dead();

static short tty_image[258] = {
#include <images/terminal.icon>
};
mpr_static(tty_pixrect, 64, 64, 1, tty_image);

static short cmd_image[258] = {
#include <images/cmdtool.icon>
};
mpr_static(cmd_pixrect, 64, 64, 1, cmd_image);

static	struct pixfont	 *font;

/*
 *  In PSV, it won't be possible to get the default value of an
 *  attribute.  Width will be adjusted in either cmdsw creation,
 *  or afterwards.
 */
#define	WIDTH_ADJUST \
	((int)textsw_get(TEXTSW, TEXTSW_LEFT_MARGIN) + 2 + \
	 (int)scrollbar_get(SCROLLBAR, SCROLL_THICKNESS) + \
	 (2 * tool_borderwidth(0)))

static	base_width(base_frame)
	Frame	base_frame;
{
	int	  width = 80;
	
	if (base_frame) {
		width = (int) window_get(base_frame, WIN_COLUMNS);
	}
	return (width * font->pf_defaultsize.x);
}

print_usage(am_cmdtool, toolname)
	int	 am_cmdtool;
	char	*toolname;
{
	char	*mode_spec = (am_cmdtool)
				? "-P frequency] [-M size" :
				"-B boldstyle";
	
	(void)fprintf(stderr,
		"syntax: %s [-C] [-I initial_cmd] [%s] [program [args]]\n",
		toolname, mode_spec);
	if (!am_cmdtool) {
	    (void)fprintf(stderr,
		"-B	set bold style for this instance of %s\n", toolname);
	    (void)fprintf(stderr,
		"	where 'boldstyle' is a number from 1 to 8\n");
	}
	(void)fprintf(stderr,
		"-C	redirect console output to this instance of %s\n",
		toolname);
	(void)fprintf(stderr,
		"-I	'initial_cmd' is first command 'program' executes\n");
	if (am_cmdtool) {
	    (void)fprintf(stderr,
		"-P	checkpoint frequency for this %s, %s\n",
		toolname, "where 'frequency' is number");
	    (void)fprintf(stderr,
		"	of edits between checkpoints; %s\n",
		"a value of 0 means no checkpointing.");
	    (void)fprintf(stderr,
		"-M	wrap edit log for this %s %s\n", toolname,
		"at 'size' bytes; size is either");
	    (void)fprintf(stderr,
		"	at least 8096, or 0 which means no wrapping.\n");
	}
}

main(argc,argv)
	int argc;
	char **argv;
{
	int	am_cmdtool = 0;
	Frame	base_frame;
	Tty	ttysw;
	Icon	tool_icon;
	char	*tool_name = argv[0];
	char	*shell_label = "shelltool";
	char	*cmd_label = "cmdtool";
	char	*console_label = " (CONSOLE) - ";
	char	frame_label[150];
	char	icon_label[30];
        char    *tmp_label1, *tmp_label2;
	int	become_console = 0;
	char	*bold_name = 0;
	char	*sh_argv[2];
	char	*init_cmd = 0;
	char	*filename = (char *)rindex(argv[0], '/');
	int	checkpoint = 0;
	int	edit_log_wraps_at = TEXTSW_INFINITY;
	int	tty_pid = 0;
	int	height_is_set, width_is_set;
	char	err_msg[50], error_msg[100];
	static  Notify_value my_destroy_func();
        int     i;
	
#ifdef GPROF
	if (argc > 1 && strcmp(argv[argc-1], "-gprof") == 0) {
	    moncontrol(1);
	    /* Pull the -gprof out of argc/v */
	    argc--;
	    argv[argc] = (char *)0;
	} else {
	    moncontrol(0);
	}
#endif
	
#ifdef DEBUG
	malloc_debug(0);   
#endif	
	
	if (!filename)
		filename = argv[0];
	else
		filename++;
	if (strcmp(filename, "cmdtool") == 0)
	    am_cmdtool = 1;
	setlocale(LC_CTYPE, "");

        for (i = 0; i < argc; i++) {
           if (strcmp(argv[i], "-Wt") == 0 || 
               strcmp(argv[i], "-font") == 0) {
              if (argv[i+1] != NULL) {
                 setenv ("DEFAULT_FONT", argv[i+1]); 
                 break;
              } else {
                 fprintf(stderr, "cmdtool: no font arg specified\n");
                 EXIT(1);
              }
           }
         }
	/*
	 *  Send the icon attr before argc, argv to give
	 *  commandline argument a chance to override.
	 *  A waste of space & time if commandline argument
	/*
	 *  Send the icon attr before argc, argv to give
	 *  commandline argument a chance to override.
	 *  A waste of space & time if commandline argument
	 *  is present.
	 */
	icon_label[0] = 0177;	/* del, highly unlikely as cmd arg */
	icon_label[1] = '\0';
	tool_icon = icon_create(
			ICON_IMAGE,	am_cmdtool ?
					&cmd_pixrect : &tty_pixrect,
			ICON_LABEL,	icon_label,
			0);
	(void)icon_set(tool_icon, FRAME_ICON, icon_label, 0);
	/* this is a hack to see if the user set the height of the frame
         * in pixels on the command line.  We need this to avoid the
	 * row correction done below (another hack).
	 */
	height_is_set = scan_for_height(argc, argv);
	if (am_cmdtool)
	    width_is_set = scan_for_width(argc, argv);
	base_frame = window_create((Window)NULL, FRAME,
			FRAME_LABEL,		NULL,
			FRAME_ICON,		tool_icon,
			FRAME_NO_CONFIRM,	TRUE, 
			FRAME_ARGC_PTR_ARGV,	&argc, argv,
			0);
			
	if (base_frame == NULL) {
	    fprintf(stderr, "Cannot create base frame.  Process aborted.\n");
	    EXIT(1);
	}		

	/*
	 * Interpose on the destroy function so we can tell user
	 * what to do.  (Avoids default veto message)
	 */
	(void) notify_interpose_destroy_func(base_frame, my_destroy_func); 

	/* Get ttysw related args */
	sh_argv[0] = NULL;
	sh_argv[1] = NULL;
	argv++;
	argc--;
	if (am_cmdtool) {
	    if (! (font =
		(struct pixfont *)window_get(base_frame, WIN_FONT))) {
		    fprintf(stderr, "Cannot get default font.\n");
		    EXIT(1);
	    }
	    if (!width_is_set) {
/* Add this term of charwidth-1 to insure that there is enough width */
		window_set(base_frame,
			WIN_WIDTH, base_width(base_frame) + WIDTH_ADJUST
	                    + font->pf_defaultsize.x - 1,
			0);
	    }
	    checkpoint =
		defaults_get_integer_check("/Tty/Checkpoint_frequency",
		0, 0, (int)TEXTSW_INFINITY, (int *)NULL);
	    edit_log_wraps_at =
		defaults_get_integer_check("/Tty/Text_wraparound_size",
		(int)TEXTSW_INFINITY, 0, (int)TEXTSW_INFINITY, (int *)NULL);
	}
	while (argc > 0 && **argv == '-') {
		switch (argv[0][1]) {
		case 'C':
			become_console = 1;
			break;
		case '?':
			(void)tool_usage(tool_name);
			print_usage(am_cmdtool, tool_name);
			EXIT(1);
		case 'B':
			if (argc > 1) {
				argv++;
				argc--;
				bold_name = *argv;
			}
			break;
		case 'I':
			if (argc > 1) {
				argv++;
				argc--;
				init_cmd = *argv;
			}

			break;
		case 'P':
                        if (argv[0][2] != '\0') {
                           fprintf (stderr,
                            "cmdtool: Space expected between -P option and count argument.\n");
                           fprintf (stderr, "Usage: cmdtool -P count\n");
                           EXIT(1);
                        }
                        else if (argv[1] == NULL) {
                           fprintf (stderr,
                            "cmdtool: No count value specified with -P option.\n");
                           fprintf (stderr, "Usage: cmdtool -P count\n");
                           EXIT(1);
                        }
			checkpoint = atoi(argv[1]);
			argc--, argv++;
			break;
		case 'M':
                        if (argv[0][2] != '\0') {
                           fprintf (stderr,
                            "cmdtool: Space expected between -M option and byte argument.\n");
                           fprintf (stderr, "Usage: cmdtool -M bytes\n");
                           EXIT(1);
                        }
                        else if (argv[1] == NULL) {
                           fprintf (stderr,
                            "cmdtool: No byte value specified with -M option.\n");
                           fprintf (stderr, "Usage: cmdtool -M bytes\n");
                           EXIT(1);
                        }
			edit_log_wraps_at = atoi(argv[1]);
			argc--, argv++;
			break;
		default:
			;
		}
		argv++;
		argc--;
	}
	if (argc == 0) {
		argv = sh_argv;
		if ((argv[0] = getenv("SHELL")) == NULL)
			argv[0] = "/bin/sh";
	}

	/* If FRAME_LABEL wasn't set by cmdline argument, set it */
	if ((tmp_label1 = window_get(base_frame, FRAME_LABEL)) == NULL) {
		(void)strncpy(frame_label,
		  am_cmdtool ? cmd_label : shell_label, sizeof(frame_label));
		if (become_console) {
			(void)strncat(frame_label, console_label,
				sizeof(frame_label));
		} else {
			(void)strncat(frame_label, " - ", sizeof(frame_label));
		}
		(void)strncat(frame_label, *argv, sizeof(frame_label));
		(void)window_set(base_frame,
				FRAME_LABEL,		frame_label,
				0);
	}
	tool_icon = (Icon)window_get(base_frame, FRAME_ICON);
	if (((tmp_label2 = icon_get(tool_icon, ICON_LABEL)) == NULL)
	||   *tmp_label2 == 0177) {
		if (tmp_label1) {
                        (void)strncpy(icon_label, tmp_label1, sizeof(icon_label));
                } else if (become_console) {
			(void)strncpy(icon_label, "CONSOLE", sizeof(icon_label));
		} else {
			(void)strncpy(icon_label, *argv, sizeof(icon_label));
		}
		(void)icon_set(tool_icon, ICON_LABEL, icon_label, 0);
	}
	(void)window_set(base_frame,
			FRAME_ICON,		tool_icon,
			0);
#ifdef ecd.help
	(void)window_set(base_frame,
			 HELP_DATA, (become_console ? "sunview:console" :
					(am_cmdtool ? "sunview:cmdtool" :
							"sunview:shelltool")),
			 0);
#endif

	/* only correct the height if the user did not give a pixel value
 	 * for the height.
	 */
	if (!height_is_set) {
	   adjust_frame_height(base_frame);
	}
	
	ttysw = window_create(base_frame,
		  am_cmdtool ? TERM : TTY,
		  TTY_ARGV,			argv,
		  TTY_QUIT_ON_CHILD_DEATH,	TRUE,
		  TTY_CONSOLE,			become_console,
                  WIN_ERROR_MSG,                error_msg,
		  0);
        if (ttysw == NULL) EXIT(1);

	tty_pid = (int) window_get(ttysw, TTY_PID);
	
	if (tty_pid == -1) {
	    if (am_cmdtool)
	       strcpy(err_msg, "Command Tool: ");
	    else
	        strcpy(err_msg, "Shell Tool: ");
	    strcpy(err_msg, "Out of swap space.  Cannot continue.\n");   
	    (void) ttysw_output(ttysw, err_msg, strlen(err_msg));        
	}
		  
#ifdef DEBUG
	(void)fprintf(stderr, "child pid = %d\n", tty_pid);
#endif DEBUG
	if (bold_name) {
		(void)window_set(ttysw, TTY_BOLDSTYLE_NAME, bold_name, 0);
	}
	if (init_cmd) {
	    int	len = strlen(init_cmd);

	    if (init_cmd[len-1] != '\n') {
		init_cmd[len] = '\n';
		len++;
	    }
	    (void)ttysw_input(ttysw, init_cmd, len);
	}

	if (am_cmdtool) {
	    Textsw	textsw = (Textsw)ttysw_to_textsw(ttysw);

	/* if the user specified wraparound size (thru the 
	 * -M flag or defaults) is less than the minimum,
	 * silently set it to SCRATCH_MIN_LEN  
        */
		if (edit_log_wraps_at < SCRATCH_MIN_LEN )
			edit_log_wraps_at = SCRATCH_MIN_LEN ;

	    (void) window_set(textsw,
			      TEXTSW_CHECKPOINT_FREQUENCY, checkpoint,
			      TEXTSW_WRAPAROUND_SIZE, edit_log_wraps_at,
			      0);
	}

	window_main_loop(base_frame);
	
	EXIT(0);
}

static void
adjust_frame_height(base_frame)
	Frame	base_frame;
{
	struct  pixfont *pf;
	int 		ttysw_pix_height;
	int		top_margin = (int)window_get(base_frame, WIN_TOP_MARGIN);
	int		bottom_margin = (int)window_get(base_frame, WIN_BOTTOM_MARGIN);
	int		num_of_row = (int)window_get(base_frame, WIN_ROWS);
	int		cur_frame_height = (int)window_get(base_frame, WIN_HEIGHT);
	int		correct_frame_height;
	
	pf = (struct pixfont *)(LINT_CAST(window_get(base_frame, WIN_FONT)));
	ttysw_pix_height = pf->pf_defaultsize.y * num_of_row;
	if (bottom_margin == 0) {
	/* bottom margin of a frame should never be zerp */
		bottom_margin = (int)window_get(base_frame, WIN_LEFT_MARGIN);
	}
	correct_frame_height = top_margin + bottom_margin + ttysw_pix_height;
	if (cur_frame_height != correct_frame_height) {
		(void)window_set(base_frame, WIN_HEIGHT, 
			    correct_frame_height, 0);
	}		    
}


/* Note that this is a terrible hack, since we look for the
 * window args of "-Ws", "-size", "-Wh", or "-height".
 * There is no other way to just "look" for the resulting attribute,
 * since tool_parse_all/one will print error messages if needed.
*/
static int
scan_for_height(argc, argv)
	register int	argc;
	register char	**argv;
{
        int     is_set = 0;
	char	*s;
 
        while (--argc > 0) {
		s = *argv;
		argv++;
		if ((strcmp(s, "-Ws") == 0) || (strcmp(s, "-size") == 0))
			is_set = 1;
		else if ((strcmp(s, "-Wh") == 0) || (strcmp(s, "-height") == 0))
			is_set = 0;
        }
	return is_set;
}

static int
scan_for_width(argc, argv)
	register int	argc;
	register char	**argv;
{
        int     is_set = 0;
	char	*s;
 
        while (--argc > 0) {
		s = *argv;
		argv++;
		if ((strcmp(s, "-Ws") == 0) || (strcmp(s, "-size") == 0))
			is_set = 1;
		else if ((strcmp(s, "-Ww") == 0) || (strcmp(s, "-width") == 0))
			is_set = 0;
        }
	return is_set;
}

static Notify_value
my_destroy_func(client, status)
    Notify_client	client;
    Destroy_status	status;
{
    int		windowfd = (int)window_get(
    		    (Window)(LINT_CAST(client)), WIN_FD);

    if (ttysw_is_already_dead())
        return(notify_next_destroy_func(client, status));
    if (status == DESTROY_CHECKING) {
	int	result;
	Event	event;

	result = alert_prompt(
		(Frame)client,
		&event,
		ALERT_MESSAGE_STRINGS,
		    "Are you sure you want to Quit?",
	            0,
		ALERT_BUTTON_YES,	"Confirm",
		ALERT_BUTTON_NO,	"Cancel",
		ALERT_OPTIONAL,		1,
		ALERT_NO_BEEPING,	1,
		0);
	if (result == ALERT_YES) {
	    return(notify_next_destroy_func(client, status));
	} else {
	    (void) notify_veto_destroy((Notify_client)(LINT_CAST(client)));
	    return(NOTIFY_DONE);
	}
    }
    return(notify_next_destroy_func(client, status));
}

