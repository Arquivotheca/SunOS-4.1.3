#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tla_parse.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1984, 1988 by Sun Microsystems, Inc.
 */

/*
 *  tla_parse.c - Do standard parse on argv and in attribute list.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_struct.h>
#include "sunwindow/sv_malloc.h"
#include <suntool/icon.h>
#include <suntool/icon_load.h>
#include <suntool/tool.h>
#include <suntool/tool_impl.h>

int tool_tried_to_get_font_from_defaults; /* FIXME:
					   * This is a process wide default
					   * because the pixrect lib uses the
					   * environment variable DEFAULT_FONT.
					   * If we go to defaults for
					   * individual tools this will have
					   * to be fixed.
					   */

typedef struct {
	    char 	*short_name, *long_name;
	    char	num_args;
} Tool_cmd_line_flag;

typedef enum {
    TOOL_FLAG_FONT,
    TOOL_FLAG_WIDTH,
    TOOL_FLAG_HEIGHT,
    TOOL_FLAG_SIZE,
    TOOL_FLAG_POSITION,
    TOOL_FLAG_ICON_POSITION,
    TOOL_FLAG_LABEL,
    TOOL_FLAG_ICONIC,
    TOOL_FLAG_NO_NAME_STRIPE,
    TOOL_FLAG_FOREGROUND_COLOR,
    TOOL_FLAG_BACKGROUND_COLOR,
    TOOL_FLAG_SET_DEFAULT_COLOR,
    TOOL_FLAG_ICON_IMAGE,
    TOOL_FLAG_ICON_LABEL,
    TOOL_FLAG_ICON_FONT,
    TOOL_FLAG_HELP
} Tool_flag_name;

/* Note that the order of the command line flags
 * and the Tool_flag_name enum above must match.
 */
static Tool_cmd_line_flag	tool_cmd_line_flags[] = {
	"-Wt", "-font", 1,
	"-Ww", "-width", 1,
	"-Wh", "-height", 1,
	"-Ws", "-size", 2,
	"-Wp", "-position", 2,
	"-WP", "-icon_position", 2,
	"-Wl", "-label", 1,
	"-Wi", "-iconic", 0,
	"-Wn", "-no_name_stripe", 0,
	"-Wf", "-foreground_color", 3,
        "-Wb", "-background_color", 3,
	"-Wg", "-set_default_color", 0,
	"-WI", "-icon_image", 1,
	"-WL", "-icon_label", 1,
	"-WT", "-icon_font", 1,
	"-WH", "-help", 0,
	0, 0, 0
};

static Tool_cmd_line_flag	*tool_find_cmd_flag();

int
tool_parse_all(argc_ptr, argv, avlist_ptr, tool_name)
	int *argc_ptr;
	char **argv;
	char ***avlist_ptr;
	char *tool_name;
{
	int argc = *argc_ptr;
	int n;

	while (argc > 0) {
		switch ((n = tool_parse_one(argc, argv, avlist_ptr,
		    tool_name))) {
		case 0:
			argc--;
			argv++;
			break;
		case -1:
			return(-1);
		default:
			while (n--) {
				(void)cmdline_scrunch(argc_ptr, argv);
				argc--;
			}
		}
	}
	return(0);
}


int
tool_parse_one(argc, argv, avlist_ptr, tool_name)
	register int argc;
	register char **argv;
	register char ***avlist_ptr;
	char *tool_name;
{
	register int plus;
	int bad_arg = 0;
	register Tool_cmd_line_flag	*slot;
	Tool_flag_name			flag_name;
	extern char *defaults_get_string();
        struct stat stat_buf;
        char font_file[128]; /* Note: assuming max pathname length of 128 chars */

	if (!tool_tried_to_get_font_from_defaults) {
	    char *fname;

	    fname = defaults_get_string("/SunView/Font", 
	    			"", (int *)NULL);
	    if (*fname != '\0') (void)setenv("DEFAULT_FONT", fname);
	    tool_tried_to_get_font_from_defaults = 1;
	}	
	
	if (argc < 1 || **argv != '-')
		return(0);

	slot = tool_find_cmd_flag(argv[0]);

	if (!slot)
	    return 0;

	if (argc <= slot->num_args) {
	    (void)fprintf(stderr, "%s: missing argument after %s\n", tool_name, 
		    argv[0]);
	    return(-1);
	}

	flag_name = (Tool_flag_name) (slot - tool_cmd_line_flags);
	switch (flag_name) {
	    case TOOL_FLAG_WIDTH:
	    case TOOL_FLAG_HEIGHT:
		if ((plus = atoi(argv[1])) < 0) {
		    bad_arg = 1;
		    goto NegArg;
		}
		tool_avl_add(avlist_ptr, flag_name == TOOL_FLAG_WIDTH ?
			     (int)(LINT_CAST(WIN_COLUMNS)) : 
			     (int)(LINT_CAST(WIN_LINES)), 
			     (char *)(LINT_CAST(plus)));
		break;

	    case TOOL_FLAG_SIZE:
		if ((plus = atoi(argv[1])) < 0) {
			bad_arg = 1;
			goto NegArg;
		}
		tool_avl_add(avlist_ptr, (int)(LINT_CAST(WIN_WIDTH)), 
				(char *)(LINT_CAST(plus)));
		if ((plus = atoi(argv[2])) < 0) {
			bad_arg = 2;
			goto NegArg;
		}
		tool_avl_add(avlist_ptr, (int)(LINT_CAST(WIN_HEIGHT)), 
				(char *)(LINT_CAST(plus)));
		break;

	    case TOOL_FLAG_POSITION:
		tool_avl_add(avlist_ptr, (int)(LINT_CAST(WIN_LEFT)), 
				(char *)(LINT_CAST(atoi(argv[1]))));
		tool_avl_add(avlist_ptr, (int)(LINT_CAST(WIN_TOP)), 
				(char *)(LINT_CAST(atoi(argv[2]))));
		break;

	    case TOOL_FLAG_ICON_POSITION:
		tool_avl_add(avlist_ptr, (int)(LINT_CAST(WIN_ICON_LEFT)), 
				(char *)(LINT_CAST(atoi(argv[1]))));
		tool_avl_add(avlist_ptr, (int)(LINT_CAST(WIN_ICON_TOP)), 
				(char *)(LINT_CAST(atoi(argv[2]))));
		break;

	    case TOOL_FLAG_LABEL:
		tool_avl_add(avlist_ptr, (int)(LINT_CAST(WIN_LABEL)),
		    tool_copy_attr((int)(LINT_CAST(WIN_LABEL)), argv[1]));
		break;

	    case TOOL_FLAG_ICONIC:
		tool_avl_add(avlist_ptr, (int)(LINT_CAST(WIN_ICONIC)), 
				(char *)(LINT_CAST(1)));
		break;

	    case TOOL_FLAG_NO_NAME_STRIPE:
		tool_avl_add(avlist_ptr, (int)(LINT_CAST(WIN_NAME_STRIPE)), 
				(char *)0);
		break;

	    case TOOL_FLAG_FONT:
		/*
		 * Note: Need error checking on the existence and
		 * goodness of font file.
		 */
                /* If the font file specified does not exist, then
                 * try /usr/lib/fonts/fixedwidthfonts/<name>.
                 * If that doesn't exist either, then declare an error.
                 * Note: font_file is assumed to be <= 128 characters long.
                 */
                strcpy(font_file, argv[1]);
                if (stat(font_file, &stat_buf)) {
                        strcpy(font_file, "/usr/lib/fonts/fixedwidthfonts/");
                        strcat(font_file, argv[1]);
                        if (stat(font_file, &stat_buf))
                                goto BadFont;
                        }
                (void)setenv("DEFAULT_FONT", font_file);
		break;

	    case TOOL_FLAG_FOREGROUND_COLOR:
	    case TOOL_FLAG_BACKGROUND_COLOR: {
		struct singlecolor color;
		char **argvtmp;
		int attr = (int) (flag_name == TOOL_FLAG_FOREGROUND_COLOR ? 
				  WIN_FOREGROUND : WIN_BACKGROUND);

		argvtmp = argv+1;
		if (win_getsinglecolor(&argvtmp, &color) == 0) {
		    tool_avl_add(avlist_ptr, attr,
				 tool_copy_attr(attr, 
				 	(char *)(LINT_CAST(&color))));
		} else
		    /* win_getsinglecolor prints error message */
		    goto NoMsgError;
		break;
	    }

	    case TOOL_FLAG_SET_DEFAULT_COLOR:
		tool_avl_add(avlist_ptr, (int)(LINT_CAST(WIN_DEFAULT_CMS)), 
				(char *)(LINT_CAST(1)));
		break;

	    case TOOL_FLAG_ICON_IMAGE: {
		char err[IL_ERRORMSG_SIZE];
		struct pixrect *mpr;

		if ((mpr = icon_load_mpr(argv[1], err)) ==
		    (struct pixrect *)0) {
		    (void)fprintf(stderr, "%s: ", tool_name);
		    (void)fprintf(stderr, err);
		    goto NoMsgError;
		}
		tool_avl_add(avlist_ptr, (int)(LINT_CAST(WIN_ICON_IMAGE)), 
				(char *)(LINT_CAST(mpr)));
		break;
	    }

	    case TOOL_FLAG_ICON_LABEL:
		tool_avl_add(avlist_ptr, (int)(LINT_CAST(WIN_ICON_LABEL)),
			    tool_copy_attr((int)(LINT_CAST(WIN_ICON_LABEL)), 
			    		argv[1]));
		break;

	    case TOOL_FLAG_ICON_FONT: {
		PIXFONT *font;
		char *dfont;
		extern char *getenv();

		/*
		 * Note: Treating WIN_ICON_FONT's value as static until get
		 * reasonable font manager that handles reference counting.
		 * If kept changing icon font using the interface (assuming
		 * that old one freed) then could waste space in a BIG way.
		 */
		if ((dfont = getenv("DEFAULT_FONT")) &&
		    (strcmp(dfont, argv[1]) == 0))
		    font = pw_pfsysopen();
		else
		    font = pf_open(argv[1]);
		if (font) {
		    tool_avl_add(avlist_ptr, (int)(LINT_CAST(WIN_ICON_FONT)), 
		    		(char *)(LINT_CAST(font)));
		} else
		    goto BadFont;
		break;
	    }

	    case TOOL_FLAG_HELP:
		return(-1);

	    default:
		return(0);

        }

	return(slot->num_args + 1);

BadFont:
	(void)fprintf(stderr, "%s: bad font file (%s)\n", tool_name, argv[1]);
	return(-1);

NegArg:
	(void)fprintf(stderr, "%s: can't have negative argument %s after %s\n",
	    tool_name, argv[bad_arg], argv[0]);
	return(-1);

NoMsgError:
	return(-1);
}


static
tool_avl_add(avlist_ptr, attr, v)
	char ***avlist_ptr;
	int attr;
	char *v;
{
	register char **argv;
	register int i, n;
	char *calloc();

	if (*avlist_ptr == (char **)0)
		*avlist_ptr = (char **)(LINT_CAST(
			sv_calloc(TOOL_ATTR_MAX, sizeof(char *))));
	argv = *avlist_ptr;
	i = 0;
	while (i < TOOL_ATTR_MAX) {
		if (argv[i]) {
			if ((n = tool_card_attr((int)(LINT_CAST(argv[i])))) == -1) {
				(void)fprintf(stderr,
				    "tool_attr doesn't support choice lists\n");
				return;
			}
			i += n+1;
		} else {
			if (i+2 >= TOOL_ATTR_MAX) {
				(void)fprintf(stderr, "tool_attr overflow\n");
				return;
			}
			argv[i++] = (char *)attr;
			argv[i++] = v;
			argv[i] = NULL;
			return;
		}
	}
}
	

static Tool_cmd_line_flag *
tool_find_cmd_flag(key)
	register char	*key;
{
	register Tool_cmd_line_flag	*slot = tool_cmd_line_flags;

	for (slot = tool_cmd_line_flags; slot->short_name; slot++)
	    if ((strcmp(key, slot->short_name) == 0) ||
	        (strcmp(key, slot->long_name) == 0))
		    return slot;
	return 0;
}


tool_parse_font(argc, argv)
	register int argc;
	register char **argv;
{
    register 		int n;
    Tool_cmd_line_flag	*slot;
    struct stat stat_buf;
    char font_file[128]; /* Note: assuming max pathname length of 128 chars */
    int ret_val=0;

    while (argc > 0) {
	if ((strcmp("-Wt", argv[0]) == 0) || 
	    (strcmp("-font", argv[0]) == 0)) {
            /* if no value for font */
            if (argv[1] == NULL) ret_val = 1;
            else  {
                /* If the font file specified does not exist, then
                 * try /usr/lib/fonts/fixedwidthfonts/<name>.
                 * Note: font_file is assumed to be <= 128 characters long.
                 */
                strcpy(font_file, argv[1]);
                if (stat(font_file, &stat_buf)) {
                        strcpy(font_file, "/usr/lib/fonts/fixedwidthfonts/");
                        strcat(font_file, argv[1]);
                }
                (void)setenv("DEFAULT_FONT", font_file);
	    }
	    n = 2;
	} else {
	    slot = tool_find_cmd_flag(argv[0]);
	    n = slot ? slot->num_args + 1 : 1;
	}
	argv += n;
	argc -= n;
    }
    return ret_val;
}


int	tool_defaultlines = 34;
int	tool_defaultcolumns = 80;

extern	struct pixfont *pf_sys;

int
tool_heightfromlines(n, namestripe)
	int	n, namestripe;
{
	return (n * pf_sys->pf_defaultsize.y)
		+ TOOL_BORDERWIDTH
		+ tool_headerheight(namestripe);
}

int
tool_widthfromcolumns(x)
        int x; 
{ 
        return (x * pf_sys->pf_defaultsize.x) + (2 * TOOL_BORDERWIDTH);
}

int
tool_linesfromheight(tool, y)
        struct tool *tool;   
        int y;  
{  
	return((y-tool_borderwidth(tool)
		- tool_headerheight(tool->tl_flags&TOOL_NAMESTRIPE))/
	    pf_sys->pf_defaultsize.y);
}

int
tool_columnsfromwidth(tool, x)
        struct tool *tool;   
        int x;  
{  
        return(((x) - 2 * tool_borderwidth((tool))) /
	    (pf_sys->pf_defaultsize.x));
}

