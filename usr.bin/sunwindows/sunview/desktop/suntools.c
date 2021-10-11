#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)suntools.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * Root window: Provides the background window for a screen.
 *	Put up environment manager menu.
 */

#include <suntool/tool_hs.h>
#include <sys/ioctl.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <pwd.h>
#include <syslog.h>
#include <strings.h>
#include <varargs.h>
#include <vfork.h>
#include <sunwindow/defaults.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/wmgr.h>
#include <suntool/selection.h>
#include <suntool/selection_svc.h>
#include <suntool/walkmenu.h>
#include <suntool/icon.h>
#include <suntool/icon_load.h>
#include <sunwindow/win_keymap.h>
#include <suntool/help.h>

#ifndef	PRE_IBIS
#include <sun/fbio.h>			/* ioctl definitions */
#endif	/* !PRE_IBIS */

#define Pkg_private

/* performance: global cache of getdtablesize() */
extern int dtablesize_cache;
#define GETDTABLESIZE() \
	(dtablesize_cache?dtablesize_cache:(dtablesize_cache=getdtablesize()))

Pkg_private int walk_getrootmenu(), walk_handlerootmenuitem();
void	expand_path();
int	wmgr_forktool();

#define GET_MENU (walk_getrootmenu)
#define MENU_ITEM (walk_handlerootmenuitem)

#define KEY_STOP	ACTION_STOP
#define KEY_PUT		ACTION_COPY
#define KEY_CLOSE	ACTION_CLOSE
#define KEY_GET		ACTION_PASTE
#define KEY_FIND	ACTION_FIND_FORWARD
#define KEY_DELETE	ACTION_CUT
#define KEY_HELP	ACTION_HELP

#define MAXPATHLEN	1024
#define MAXARGLEN	3000

extern int            errno;

extern char          *malloc(),
		     *calloc(),
		     *getenv();

extern struct pixrect *pr_load();
extern struct pixfont *pf_default(), *pf_open();

static void	      root_winmgr(),
		      root_sigchldhandler(),
		      root_sigwinchhandler(),
		      root_sigchldcatcher(),
		      root_sigwinchcatcher(),
                      root_sigusr1catcher(),
#ifdef ecd.suntools
		      root_sigtermhandler(),
#endif
		      root_initialsetup(),
		      root_set_background(),
		      root_set_pattern(),
		      root_start_service();

Pkg_private char *wmgr_savestr();

static int            dummy_proc();
static struct selection empty_selection = {
	SELTYPE_NULL, 0, 0, 0, 0
};

static Seln_client    root_seln_handle;
static char	     *seln_svc_file;

static int            rootfd,
		      rootnumber,
		      root_SIGCHLD,
                      root_SIGUSR1,
#ifdef ecd.suntools
		      root_SIGTERM = FALSE,
#endif
		      root_SIGWINCH;

static struct screen  screen;

static struct pixwin *pixwin;

#define	ROOTMENUITEMS	20
#define	ROOTMENUFILE	"/usr/lib/rootmenu"
#define ROOTMENUNAME	"SunView"
#define ARG_CHARS       1024

/* These files will be deleted when sunview exits */
static char *tmp_file[] = {
	"/tmp/textsw_shelf",
	"/tmp/ttyselection",
	"/tmp/winselection"
};

static char    window_journal[30]; 	/* string for WINDOW_JOURNAL env var */
static int     honor_sigusr1;		/* whether or not to catch siguser */
extern int     sv_journal;		/* journalling is running */

char                 *rootmenufile;
Pkg_private Menu      wmgr_rootmenu;

struct stat_rec {
    char             *name;	   /* Dynamically allocated menu file name */
    time_t            mftime;      /* Modified file time */
};

#define	MAX_FILES	40
Pkg_private struct stat_rec wmgr_stat_array[MAX_FILES];
Pkg_private int            wmgr_nextfile;

static	enum root_image_type {
	ROOT_IMAGE_PATTERN,
	ROOT_IMAGE_SOLID,
} root_image_type;
static struct pixrect *root_image_pixrect;
static root_color;

static short hglass_data[] = {
#include <images/hglass.cursor>
};

mpr_static_static(hglass_cursor, 16, 16, 1, hglass_data);

static short arrow_data[] = {
	0x0000, 0x7fe0, 0x7f80, 0x7e00,
	0x7e00, 0x7f00, 0x7f80, 0x67c0,
	0x63e0, 0x41f0, 0x40f8, 0x007c,
	0x003e, 0x001f, 0x000e, 0x0004
};

mpr_static_static(arrow_cursor, 16, 16, 1, arrow_data);


static char *progname;	/* my name */
static char *basename();

Pkg_private int _error();
#define	NO_PERROR	((char *) 0)

/* bug 1027565 -- add suntools security features */
#define SV_ACQUIRE	"/bin/sunview1/sv_acquire"
#define SV_RELEASE	"/bin/sunview1/sv_release"

#ifdef STANDALONE
main(argc, argv)
#else
suntools_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	char	name[WIN_NAMESIZE], setupfile[MAXNAMLEN];
	int	placeholderfd;
	int	donosetup = 0, printname = 0;
	char   *root_background_file = 0;

#ifdef	PRE_IBIS
	unsigned char red[256], green[256], blue[256];
	unsigned char red_o [2], green_o [2], blue_o [2];
	unsigned char red_oe [2], green_oe [2], blue_oe [2];
#else	/* !PRE_IBIS */
	/*
	 * The number of plane groups and their respective colormap size
	 * will vary dramatically between frame buffers.  We will declare
	 * some pointers here and malloc the exact colormap size later.
	 */
	int		cmsize;
	unsigned	char	*red	[PIX_MAX_PLANE_GROUPS],
			*green	[PIX_MAX_PLANE_GROUPS],
			*blue	[PIX_MAX_PLANE_GROUPS];
#endif	/* PRE_IBIS */

	register struct	pixrect *fb_pixrect;
#define DONT_CARE_SHIFT         -1
        Firm_event fe_focus;
        int fe_focus_shift;
        Firm_event fe_restore_focus;
        int fe_restore_focus_shift;
        int set_focus = 0, set_restore_focus = 0, set_sync_tv = 0;
        struct timeval sync_tv;
	char *font_name;
	char    **args;
	struct singlecolor single_color;
	register int i;
	struct sigvec vec;
	struct stat tmpb;
	register int j;
	int	file_exists;
	Cursor  cursor;
	int	original_pg;
	char	plane_groups [PIX_MAX_PLANE_GROUPS];
	char	*journal_ptr;
	union wait status;


	progname = basename(argv[0]);

	seln_svc_file = "selection_svc";
	root_image_type = ROOT_IMAGE_PATTERN;
	root_image_pixrect = tool_bkgrd;
	root_color = -1;
	root_set_pattern(defaults_get_string("/SunView/Root_Pattern",
	    "on", (int *)NULL), &single_color);
	if (defaults_get_boolean("/SunView/Click_to_Type", (Bool)FALSE, (int *)NULL)) {
		/* 's'plit focus (See -S below if change) */
		fe_focus.id = MS_LEFT;
		fe_focus.value = 1;
		fe_focus_shift = DONT_CARE_SHIFT;
		set_focus = 1;
		fe_restore_focus.id = MS_MIDDLE;
		fe_restore_focus.value = 1;
		fe_restore_focus_shift = DONT_CARE_SHIFT;
		set_restore_focus = 1;
	}
	/*
	 * Parse cmd line.
	 */
	setupfile[0] = NULL;
	(void)win_initscreenfromargv(&screen, argv);
	if (argv) {
                for (args = ++argv;*args;args++) {
                        if ((strcmp(*args, "-s") == 0) && *(args+1)) {
				(void) strcpy(setupfile, *(args+1));
                                args++;
                        } else if (strcmp(*args, "-F") == 0) {
                                root_color = -1;
                                root_image_type = ROOT_IMAGE_SOLID;
                        } else if (strcmp(*args, "-B") == 0) {
                                root_color = 0;
                                root_image_type = ROOT_IMAGE_SOLID;
                        } else if (strcmp(*args, "-P") == 0)
				root_set_pattern("on", &single_color);
                        else if (strcmp(*args, "-n") == 0)
                                donosetup = 1;
                        else if (strcmp(*args, "-p") == 0)
                                printname = 1;
                        else if (strcmp(*args, "-color") == 0) {
				if (args_remaining(args) < 4)
					goto Arg_Count_Error;
				++args;
                                if (win_getsinglecolor(&args, &single_color))
                                	continue;
                                root_color = 1;
                        } else if (strcmp(*args, "-background") == 0) {
				if (args_remaining(args) < 2)
					goto Arg_Count_Error;
                                root_background_file = *++args;
                        } else if (strcmp(*args, "-pattern") == 0) {
				if (args_remaining(args) < 2)
					goto Arg_Count_Error;
				++args;
				root_set_pattern(*args, &single_color);
                        } else if (strcmp(*args, "-svc") == 0) {
				if (args_remaining(args) < 2)
					goto Arg_Count_Error;
                                seln_svc_file = *++args;
                                (void) _error(NO_PERROR,
                                	"starting selection service \"%s\"",
                                	seln_svc_file);
			} else if (strcmp(*args, "-S") == 0) {
				/*
				 * 's'plit focus (See Click_to_type above
				 * if change)
				 */
				fe_focus.id = MS_LEFT;
				fe_focus.value = 1;
				fe_focus_shift = DONT_CARE_SHIFT;
				set_focus = 1;
				fe_restore_focus.id = MS_MIDDLE;
				fe_restore_focus.value = 1;
				fe_restore_focus_shift = DONT_CARE_SHIFT;
				set_restore_focus = 1;
			} else if (strcmp(*args, "-c") == 0) {
				/* set 'c'aret */
				get_focus_from_args(&args, (short *)&fe_focus.id,
				    &fe_focus.value, &fe_focus_shift);
				set_focus = 1;
			} else if (strcmp(*args, "-r") == 0) {
				/* 'r'estore caret */
				get_focus_from_args(&args, (short *)&fe_restore_focus.id,
				    &fe_restore_focus.value,
				    &fe_restore_focus_shift);
				set_restore_focus = 1;
			} else if (strcmp(*args, "-t") == 0) {
				/* set 't'imeout */
				if (args_remaining(args) < 2)
					goto Arg_Count_Error;
				args++;
				sync_tv.tv_usec = 0;
				sync_tv.tv_sec = atoi(*args);
				set_sync_tv = 1;
			} else if (argc == 2 && *args[0] != '-')
				/*
				 * If only arg and not a flag then treat as
				 * setupfile (backward compatibility with 1.0).
				 */
				(void) strcpy(setupfile, *args);
                        else if (strcmp(*args, "-J") == 0)
                                honor_sigusr1 = 1;
                }
        }

	/*
	 * Set up signal catchers.
	 */
	/* (void) signal(SIGCHLD, (void (*)())(LINT_CAST(root_sigchldcatcher))); */

	vec.sv_handler = (void (*)())(LINT_CAST(root_sigchldcatcher));
	vec.sv_mask = vec.sv_onstack = 0;
	sigvec(SIGCHLD, &vec, 0);

	/* (void) signal(SIGWINCH, (void (*)())(LINT_CAST(root_sigwinchcatcher))); */

	vec.sv_handler = (void (*)())(LINT_CAST(root_sigwinchcatcher));
	sigvec(SIGWINCH, &vec, 0);

	/* journalling */
	if (honor_sigusr1) {
        	vec.sv_handler = (void (*)())(LINT_CAST(root_sigusr1catcher)) ;
        	sigvec(SIGUSR1, &vec, 0) ;
	}

#ifdef ecd.suntools
	vec.sv_handler = (void (*)())(LINT_CAST(root_sigtermhandler)) ;
	sigvec(SIGTERM, &vec, 0) ;

#endif

	/* intialization for journalling */
	if (honor_sigusr1) {
        	sv_journal = FALSE;
               	(void)sprintf(window_journal, "WINDOW_JOURNAL = OFF");
                setenv("WINDOW_JOURNAL", "OFF");
	}
	/*
	 * Find out what colormap is so can restore later.
	 * Do now before call win_screennew which changes colormap.
	 */
	if (screen.scr_fbname[0] == NULL)
		(void)strcpy(screen.scr_fbname, "/dev/fb");
	if ((fb_pixrect = pr_open(screen.scr_fbname)) == (struct pixrect *)0) {
		(void) _error(NO_PERROR, "invalid frame buffer %s",
		    screen.scr_fbname);
		exit(1);
	}
	original_pg = pr_get_plane_group (fb_pixrect);

/* bug 1027565 -- fork to change owner/group/permissions of window system devices */

	switch (vfork()) {
	    case -1:		/* ERROR */
	        (void)fprintf(stderr, "sunview: Fork failed.\n");
	        _exit(1);

	    case 0:		/* CHILD */
	        (void)execl(SV_ACQUIRE, "sv_acquire", "0", "256", "240", 0);
		/* should not return */
	        (void)fprintf(stderr, "sunview: exec for sv_acquire failed\n");
		_exit(1);

	    default:		/* PARENT */
		/* do nothing */
	        break;
	}
	(void)wait(&status);	/* child dies after changing 16 */

	if (status.w_retcode != 0) {
	    (void)fprintf(stderr, "sunview: failed to set ownership of win devices\n");
	    exit(1);
	}

#ifdef	PRE_IBIS
	(void) pr_available_plane_groups (fb_pixrect, 6, plane_groups);
	for (i = 1; i < PIX_MAX_PLANE_GROUPS; i++) {
		if (plane_groups [i]) {
			pr_set_plane_group (fb_pixrect, i);
			switch (i) {
			case PIXPG_MONO:
			case PIXPG_OVERLAY:
				(void)pr_getcolormap(fb_pixrect, 0, 2, red_o,
				    green_o, blue_o);
				break;
			case PIXPG_OVERLAY_ENABLE:
				(void)pr_getcolormap(fb_pixrect, 0, 2, red_oe,
				    green_oe, blue_oe);
				break;
			case PIXPG_8BIT_COLOR:
			case PIXPG_24BIT_COLOR:
				(void)pr_getcolormap(fb_pixrect, 0, 256, red,
				    green, blue);
				break;
			}
		}
	}
#else	/* !PRE_IBIS */
	/* Alloc the exact amount of space and save the colormap. */
	(void)
	pr_available_plane_groups(fb_pixrect,PIX_MAX_PLANE_GROUPS,plane_groups);
	for (i = 1; i < PIX_MAX_PLANE_GROUPS; i++)
	{
	    if (plane_groups[i])
	    {
		pr_set_plane_group(fb_pixrect, i);
		if (pr_ioctl(fb_pixrect, FBIOGCMSIZE, &cmsize) < 0)
		{
		    /* for frame buffers which have no pr_ioctl implemented */
		    switch (i)
		    {
			case PIXPG_CURSOR:
			case PIXPG_CURSOR_ENABLE:
				/* Dont post a colormap */
				continue;
			break;
			case PIXPG_MONO:
			case PIXPG_OVERLAY:
			case PIXPG_OVERLAY_ENABLE:
			    cmsize = 2;		break;

			default:
			    cmsize = 256;	break;
		    }
		}
		red[i]	= (unsigned char *) malloc(cmsize);
		blue[i]	= (unsigned char *) malloc(cmsize);
		green[i]	= (unsigned char *) malloc(cmsize);
		(void)
		pr_getcolormap(fb_pixrect, 0, cmsize, red[i],green[i],blue[i]);
	    }
	}
#endif	/* !PRE_IBIS */

	pr_set_plane_group (fb_pixrect, original_pg);
	/*
	 * Create root window
	 */
	if ((rootfd = win_screennew(&screen)) == -1) {
		(void) _error("\
cannot create root window\n\
The devices necessary to run SunView may not be available (check /dev).\n\
System call error");
		exit(1);
	}
       	/*
	 * Set up root's name in environment (must be before call to
	 * pf_use_vm(), which must be before fonts are loaded, directly
	 * or indirectly).
	 */
	(void)win_fdtoname(rootfd, name);
	rootnumber = win_nametonumber(name);
	(void)we_setparentwindow(name);
	if (printname)
		(void)fprintf(stderr, "SunView window name is %s\n", name);
	/*
	 * Set DEFAULT_FONT environment variable from Defaults database.
	 */
	font_name = defaults_get_string("/SunView/Font", "", (int *)NULL);
	if (*font_name != '\0')
		(void)setenv("DEFAULT_FONT", font_name);
	/*
	 *	If loaded with shared libraries, turn on vm allocator for
	 * fonts then load all of the fonts listed in defaults database.
	 *	This must happen AFTER the framebuffer is opened to increase
	 * the probability that the vm address chosen for the fonts will
	 * also be free in the individual tools.
	 */
#define	ALERTS_DEFAULT_MENU_FONT "/usr/lib/fonts/fixedwidthfonts/screen.b.14"
#define	MENU_IMAGE_FONT "/usr/lib/fonts/fixedwidthfonts/screen.b.12"
	if (pf_linked_dynamic()) {
		pf_use_vm(1);
		pf_default();
		font_name = defaults_get_string("/Menu/Font", "", (int *)NULL);
		if (font_name[0] == 0)
			font_name = ALERTS_DEFAULT_MENU_FONT;
		(void) pf_open(font_name);
		(void) pf_open(MENU_IMAGE_FONT);
		font_name = defaults_get_string("/Text/Font", "", (int *)NULL);
		if (*font_name != '\0')
			(void) pf_open(font_name);
		pf_use_vm(0);
	}

	/*
	 * Finish initializing the root.
	 */
        if (root_background_file != (char *) NULL)
        	root_set_background(root_background_file);
	cursor = cursor_create((char *)0);
	(void)win_getcursor(rootfd, cursor);
	if (root_color != 1 && root_image_type == ROOT_IMAGE_SOLID) 
	        (void)cursor_set(cursor, CURSOR_OP, PIX_SRC^PIX_DST, 0);
	(void)cursor_set(cursor, CURSOR_IMAGE, &hglass_cursor, 0);
	(void)win_setcursor(rootfd, cursor);
	cursor_destroy(cursor);

	/* Set input parameters */
	if (set_focus)
		if (win_set_focus_event(rootfd, &fe_focus, fe_focus_shift)) {
			(void) _error("win_set_focus_event");
			exit(1);
		}
	if (set_restore_focus)
		if (win_set_swallow_event(rootfd,
		    &fe_restore_focus, fe_restore_focus_shift)){
			(void) _error("win_set_swallow_event");
			exit(1);
		}
	if (set_sync_tv)
		if (win_set_event_timeout(rootfd, &sync_tv)) {
			(void) _error("win_set_event_timeout");
			exit(1);
		}
	(void)win_screenget(rootfd, &screen);
	/*
	 * Open pixwin.
	 */
	if ((pixwin = pw_open_monochrome(rootfd)) == 0) {
		(void) _error("%s not available for window system usage",
		    screen.scr_fbname);
                exit(1);
        }


        /* Set up own color map if have own color */
        if (root_color == 1) {
        	char cmsname[CMS_NAMESIZE];
        	int color, bg, fg;
#define	ROOT_CMS_SIZE 4
        	u_char root_red[ROOT_CMS_SIZE], root_green[ROOT_CMS_SIZE],
        	    root_blue[ROOT_CMS_SIZE];
        	
		/*
		 * -pattern grey or -color got us here but if -i was specified,
		 * pwo_putcolormap will only check for SCR_SWITCHBKGRDFRGRD
		 * if the depth is 1.  In this situation, the root background
		 * and default cms have depth 8 and the SCR_SWITCHBKGRDFRGRD
		 * is lost.  Taking care of it at this time appears to yield
		 * correct results even for framebuffers with multiple planes.
		 */

		bg = (screen.scr_flags & SCR_SWITCHBKGRDFRGRD)
		    ? ROOT_CMS_SIZE - 1 : 0;
		fg = (screen.scr_flags & SCR_SWITCHBKGRDFRGRD)
		    ? 0 : ROOT_CMS_SIZE - 1;

		root_red[bg] = screen.scr_background.red;
		root_green[bg] = screen.scr_background.green;
		root_blue[bg] = screen.scr_background.blue;

		for (color = 1; color < ROOT_CMS_SIZE-1; color++) {
        		root_red[color] = single_color.red;
        		root_green[color] = single_color.green;
        		root_blue[color] = single_color.blue;
		}

        	root_red[fg] = screen.scr_foreground.red;
        	root_green[fg] = screen.scr_foreground.green;
        	root_blue[fg] = screen.scr_foreground.blue;

        	(void)sprintf(cmsname, "rootcolor%ld", getpid());
        	(void)pw_setcmsname(pixwin, cmsname);
		(void)pw_putcolormap(pixwin, 0, ROOT_CMS_SIZE,
		    root_red, root_green, root_blue);
        }
	/*
	 * Steal a window for the tool slot allocator
	 * & stash its name in the environment
	 */
	if ((placeholderfd = win_getnewwindow()) == -1)  {
		(void) _error("no window available for placing open windows");
	} else {
		(void)win_fdtoname(placeholderfd, name);
		(void)setenv("WMGR_ENV_PLACEHOLDER", name);
	}


	/*
	 * Set up tool slot allocator
	 */
	(void)wmgr_init_icon_position(rootfd);
	(void)wmgr_init_tool_position(rootfd);
	/*
	 * Setup tty parameters for all terminal emulators that will start.
	 */
	{int tty_fd;
	tty_fd = open("/dev/tty", O_RDWR, 0);
	if (tty_fd < 0)
		(void)ttysw_saveparms(2);	/* Try stderr */
	else {
		(void)ttysw_saveparms(tty_fd);
		(void) close(tty_fd);
	}
	}
	/*	try to make sure there is a selection service
	 */
#define	SEL_SVC_SEC_WAIT	5
#define	SEL_CLIENT_NULL		(Seln_client) NULL
	if ((root_seln_handle = seln_create((void (*)())(LINT_CAST(dummy_proc)), 
		(Seln_result (*)())(LINT_CAST(dummy_proc)), 
	    (char *)(LINT_CAST(&root_seln_handle)))) == SEL_CLIENT_NULL) {
		root_start_service();
		for (i = 0;
		   i < SEL_SVC_SEC_WAIT && root_seln_handle == SEL_CLIENT_NULL;
		   i++) {
			usleep(1 << 20);
			root_seln_handle = seln_create(
				(void (*)())(LINT_CAST(dummy_proc)),
				(Seln_result (*)())(LINT_CAST(dummy_proc)),
				(char *)(LINT_CAST(&root_seln_handle))
			);
		}
		if (root_seln_handle == SEL_CLIENT_NULL)
			(void) _error(NO_PERROR,
		        "cannot find old, or start new, selection service");
	}


        /* 
         * Initialize root menu from menu file
         * -- must be done before background is displayed
         * -- must be done late enough so "cleanup" can be done.
         *    (or we could revise/modularize the code)
         * -- done here because it works...!
         */
        rootmenufile = defaults_get_string("/SunView/Rootmenu_filename", "",
                                           (int *)NULL);
        if (*rootmenufile == '\0'
            && (rootmenufile = getenv("ROOTMENU")) == NULL)
            rootmenufile = ROOTMENUFILE;
        wmgr_rootmenu = NULL;
        if (GET_MENU(rootmenufile) <= 0) {
            (void) _error(NO_PERROR, "invalid root menu");
            goto CLEANUP;
        }
 

	/*
	 * Draw background.
	 */
	root_sigwinchhandler();
	/*
	 * Do initial window setup.
	 */
	if (!donosetup)
		root_initialsetup(setupfile);
	/*
	 * Do window management loop.
	 */
	cursor = cursor_create((char *)0);
	(void)win_getcursor(rootfd, cursor);
	if (root_color != 1 && root_image_type == ROOT_IMAGE_SOLID)
                (void)cursor_set(cursor, CURSOR_OP, PIX_SRC^PIX_DST, 0);
	(void)cursor_set(cursor, CURSOR_IMAGE, &arrow_cursor, 0);
	(void)win_setcursor(rootfd, cursor);
	cursor_destroy(cursor);

	root_winmgr();

CLEANUP:
	/*
	 * Delete temporary files from /tmp for added security
	 */
	for (j=0; j<3; j++) {
		if ((file_exists = stat(tmp_file[j], &tmpb)) == 0)
			(void)unlink(tmp_file[j]);
	}
        if (plane_groups[PIXPG_CURSOR]) {
            
	    /* Take the cursor down if on a hawk - bugid 1058820 */
            Cursor cur;
            cur = cursor_create((char *)0);
            (void)win_getcursor(rootfd, cur);
            (void)cursor_set(cur, CURSOR_OP, PIX_CLR, CURSOR_IMAGE, NULL, 0);
            (void)win_setcursor(rootfd, cur);
            cursor_destroy(cur);
        }

	/*
	 * Destroy screen sends SIGTERMs to all existing windows and
	 * wouldn''t let any windows install themselves in the window tree.
	 * Calling process of win_screedestroy is spared SIGTERM.
	 */
	(void)win_screendestroy(rootfd);
	(void)close(placeholderfd);
	/*
	 * Lock screen before clear so don''t clobber frame buffer while
	 * cursor moving.
	 */
	(void)pw_lock(pixwin, &screen.scr_rect);
	/*
	 * Clear available plane groups
	 */

#ifdef	PRE_IBIS
	for (i = 1; i < PIX_MAX_PLANE_GROUPS; i++) {
		if (plane_groups [i]) {
			int op;

			/* Write to all plane groups */
			pr_set_planes(pixwin->pw_pixrect, i, PIX_ALL_PLANES);
			op = PIX_CLR;
			switch (i) {
			case PIXPG_MONO:
			case PIXPG_OVERLAY:
				if (! red_o [0])
				    red_o [0] = ~red_o [0];
				(void) pr_putcolormap (pixwin->pw_pixrect, 0,
				    2, red_o, green_o, blue_o);
				break;
			case PIXPG_OVERLAY_ENABLE:
				if (! red_oe [0])
				    red_oe [0] = ~red_oe [0];
				(void) pr_putcolormap (pixwin->pw_pixrect, 0,
				    2, red_oe, green_oe, blue_oe);
				if (plane_groups [PIXPG_24BIT_COLOR])
					op = PIX_SET;
				break;
			case PIXPG_24BIT_COLOR:
				/* Fall through to 8 bit case. */
			case PIXPG_8BIT_COLOR:
				(void) pr_putcolormap (pixwin->pw_pixrect, 0,
				    256, red, green, blue);
				break;
			}
			/* Clear screen */
			(void)pr_rop(pixwin->pw_pixrect,
			    screen.scr_rect.r_left, screen.scr_rect.r_top,
			    screen.scr_rect.r_width, screen.scr_rect.r_height,
			    op, (Pixrect *)0, 0, 0);
		}
	}
#else	/* PRE_IBIS */
	for (i = 1; i < PIX_MAX_PLANE_GROUPS; i++) {
	    if (plane_groups[i]) {

		pr_set_planes(pixwin->pw_pixrect, i, PIX_ALL_PLANES);
		/* User is not permitted to post cursor colormap 
		 */
	        switch (i) {
		    case PIXPG_CURSOR:
			pr_rop(pixwin->pw_pixrect,
				screen.scr_rect.r_left, screen.scr_rect.r_top,
                            screen.scr_rect.r_width, screen.scr_rect.r_height,
                            PIX_CLR, (Pixrect *)0, 0, 0);
		    break;
		    case PIXPG_CURSOR_ENABLE:
			pr_rop(pixwin->pw_pixrect,
                                screen.scr_rect.r_left, screen.scr_rect.r_top, 
                            screen.scr_rect.r_width, screen.scr_rect.r_height, 
                            PIX_SET, (Pixrect *)0, 0, 0); 
                    break; 
		    default:
			if (pr_ioctl(fb_pixrect, FBIOGCMSIZE, &cmsize) < 0) {
			    /* for frame buffers which have no pr_ioctl 
			      implemented 
			    */
			    switch (i) {
				case PIXPG_MONO:
				case PIXPG_OVERLAY:
				case PIXPG_OVERLAY_ENABLE:
				    cmsize = 2;		break;

				default:
				    cmsize = 256;
			    }
			}
			(void)pr_putcolormap(pixwin->pw_pixrect, 0, 
						   cmsize, red[i], green[i],blue[i]);
			free(green[i]);
			free(blue[i]);
			free(red[i]);
			(void)pr_rop(pixwin->pw_pixrect,
			    screen.scr_rect.r_left,screen.scr_rect.r_top,
			    screen.scr_rect.r_width,	screen.scr_rect.r_height,
			    (i==PIXPG_OVERLAY_ENABLE) ? PIX_SET : PIX_CLR,
			    (Pixrect *)0, 0, 0);
		}
	    }
	}
#endif	/* !PRE_IBIS */

	pr_set_plane_group (pixwin->pw_pixrect, original_pg);
	/* be sure to push the state on "state-full" boards bugid 1046602 */
	(void) pr_get(pixwin->pw_pixrect, 0, 0);

	/*
	 * Unlock screen.
	 */
	(void)pw_unlock(pixwin);

/* bug 1027565 -- fork to change owner/group/permissions of window system devices */

	switch (vfork()) {
	    case -1:		/* ERROR */
	        (void)fprintf(stderr, "sunview: Fork failed.\n");
	        _exit(1);

	    case 0:		/* CHILD */
	        (void)execl(SV_RELEASE, "sv_release", 0);
		/* should not return */
	        (void)fprintf(stderr, "sunview: exec for sv_release failed\n");
		_exit(1);

	    default:		/* PARENT */
		/* do nothing */
	        break;
	}
	/*
	 * Performance: don't wait for the child to finish
	 */
	exit(0);

Arg_Count_Error:
	exit(_error(NO_PERROR, "%s arg count error", args));
}

static
args_remaining(args)
		char **args;
{
	register i;

	for (i = 0; *(args+i); i++) {}
	return (i);
}

static
get_focus_from_args(argv_ptr, event, value, shift)
	char ***argv_ptr;
	register short *event;
	register int *value;
	register int *shift;
{
#define	SHIFT_MASK(bit) (1 << (bit))
	char str[200];
	register char *arg;

	if (args_remaining(*argv_ptr) < 4) {
		exit(_error(NO_PERROR, "%s arg count error", *argv_ptr));
	}
	(*argv_ptr)++;
	arg = **argv_ptr;
	if (strcmp(arg, "LOC_WINENTER") == 0)
		*event = LOC_WINENTER;
	else if (strcmp(arg, "MS_LEFT") == 0)
		*event = MS_LEFT;
	else if (strcmp(arg, "MS_MIDDLE") == 0)
		*event = MS_MIDDLE;
	else if (strcmp(arg, "MS_RIGHT") == 0)
		*event = MS_RIGHT;
	else if (sscanf(arg, "BUT%s", str) == 1)
		*event = atoi(str)+BUT_FIRST;
	else if (sscanf(arg, "KEY_LEFT%s", str) == 1)
		*event = atoi(str)+KEY_LEFTFIRST-1;
	else if (sscanf(arg, "KEY_RIGHT%s", str) == 1)
		*event = atoi(str)+KEY_RIGHTFIRST-1;
	else if (sscanf(arg, "KEY_TOP%s", str) == 1)
		*event = atoi(str)+KEY_TOPFIRST-1;
	else if (strcmp(arg, "KEY_BOTTOMLEFT") == 0)
		*event = KEY_BOTTOMLEFT;
	else if (strcmp(arg, "KEY_BOTTOMRIGHT") == 0)
		*event = KEY_BOTTOMRIGHT;
	else
		*event = atoi(arg);
	(*argv_ptr)++;
	arg = **argv_ptr;
	if (strcmp(arg, "DOWN") == 0 || strcmp(arg, "Down") == 0 ||
	    strcmp(arg, "down") == 0)
		*value = 1;
	else if (strcmp(arg, "ENTER") == 0 || strcmp(arg, "Enter") == 0 ||
	    strcmp(arg, "enter") == 0)
		*value = 1;
	else if (strcmp(arg, "UP") == 0 || strcmp(arg, "Up") == 0 ||
	    strcmp(arg, "up") == 0)
		*value = 0;
	else
		*value = atoi(arg);
	(*argv_ptr)++;
	arg = **argv_ptr;
	if (strcmp(arg, "SHIFT_LEFT") == 0)
		*shift = SHIFT_MASK(LEFTSHIFT);
	else if (strcmp(arg, "SHIFT_RIGHT") == 0)
		*shift = SHIFT_MASK(RIGHTSHIFT);
	else if (strcmp(arg, "SHIFT_LEFTCTRL") == 0)
		*shift = SHIFT_MASK(LEFTCTRL);
	else if (strcmp(arg, "SHIFT_RIGHTCTRL") == 0)
		*shift = SHIFT_MASK(RIGHTCTRL);
	else if (strcmp(arg, "SHIFT_DONT_CARE") == 0)
		*shift = DONT_CARE_SHIFT;
	else if (strcmp(arg, "SHIFT_ALL_UP") == 0)
		*shift = 0;
	else
		*shift = atoi(arg);

}

static void
root_winmgr()
{
    struct inputmask      im;
    struct inputevent     event;
    int                   keyexit = 0;

    /*
     * Set up input mask so can do menu stuff 
     */
    (void)input_imnull(&im);
    im.im_flags |= IM_NEGEVENT;
    im.im_flags |= IM_ASCII;
    win_setinputcodebit(&im, SELECT_BUT);
    win_setinputcodebit(&im, MENU_BUT);
    win_keymap_set_smask(rootfd, ACTION_STOP);
    win_keymap_set_imask_from_std_bind(&im, ACTION_STOP);
    win_setinputcodebit(&im, KBD_REQUEST);
    win_keymap_set_smask(rootfd, KEY_PUT);
    win_keymap_set_smask(rootfd, KEY_GET);
    win_keymap_set_smask(rootfd, KEY_FIND);
    win_keymap_set_smask(rootfd, KEY_DELETE);
    win_keymap_set_smask(rootfd, KEY_HELP);
    win_keymap_set_imask_from_std_bind(&im, KEY_HELP);
    win_keymap_set_imask_from_std_bind(&im, KEY_PUT);
    win_keymap_set_imask_from_std_bind(&im, KEY_GET);
    win_keymap_set_imask_from_std_bind(&im, KEY_FIND);
    win_keymap_set_imask_from_std_bind(&im, KEY_DELETE);
    (void)win_setinputmask(rootfd, &im, (struct inputmask *) 0, WIN_NULLLINK);
    /*
     * Read and invoke menu items 
     */
    for (;;) {
	fd_set                   ibits;
 	int			 nfds;
	int 			 maxfds = GETDTABLESIZE();

#ifdef ecd.suntools
	if (root_SIGTERM)
		return ;

#endif
	/*
	 * Use select (to see if have input) so will return on SIGWINCH or
	 * SIGCHLD. 
	 */
	/* ibits = 1 << rootfd; */
	FD_ZERO(&ibits);
        FD_SET(rootfd, &ibits);
	do {
	    if (root_SIGCHLD)
		root_sigchldhandler();
	    if (root_SIGWINCH)
		root_sigwinchhandler();
	} while (root_SIGCHLD || root_SIGWINCH);
	nfds = select(maxfds, &ibits, (fd_set *) 0, (fd_set *) 0,
		      (struct timeval *) 0);
	if (nfds == -1) {
	    if (errno == EINTR)
		/*
		 * Go around again so that signals can be handled.  ibits may
		 * be non-zero but should be ignored in this case and they will
		 * be selected again. 
		 */
		continue;
	    else {
		abort();
		(void) _error("root window select error");
		break;
	    }
	}
	/* if (ibits & (1 << rootfd)) { */
	if (FD_ISSET(rootfd, &ibits)) {
	    /*
	     * Read will not block. 
	     */
	    if (input_readevent(rootfd, &event) < 0) {
		if (errno != EWOULDBLOCK) {
		    (void) _error("root window event read error");
		    break;
		}
	    }
	} else
	    continue;

	switch (event_action(&event)) {
	  case CTRL(q):	       /* Escape for getting out	 */
	    if (keyexit) {	       /* when no mouse around	 */
		return;
	    }
	    continue;
	  case CTRL(d):
	    keyexit = 1;
	    continue;
	  case CTRL(l):
	  case CTRL(r):	  
	    wmgr_refreshwindow(rootfd);
	    break;
	  case KEY_STOP:
	    if (root_seln_handle != (Seln_client) NULL) {
		seln_clear_functions();
	    }
	    break;
	  case KBD_REQUEST:	/* Always refuse keyboard focus request */
	    (void)win_refuse_kbd_focus(rootfd);
	    break;
	  case KEY_PUT:
	  case KEY_GET:
	  case KEY_FIND:
	  case KEY_DELETE:
	    if (root_seln_handle != (Seln_client) NULL) {
		seln_report_event((Seln_client)(LINT_CAST(
			root_seln_handle)), &event);
	    }
	    break;
	  case KEY_CLOSE:
	    break;
	  case MS_LEFT:
	    if (win_inputposevent(&event)) {
		/*  the left button went down; clear the selection.  */
		(void)selection_set(&empty_selection, dummy_proc, dummy_proc, rootfd);
		if (root_seln_handle != (Seln_client) NULL) {
		    Seln_rank             rank;

		    rank = seln_acquire((Seln_client)(LINT_CAST(
		    	root_seln_handle)), SELN_UNSPECIFIED);
		    (void)seln_done((Seln_client)(LINT_CAST(root_seln_handle)),
		    	 rank);
		}
	    }
	    break;
	  case MENU_BUT:
	    if (win_inputposevent(&event)) {
		if (!root_menu_mgr(&event)) {
		    return;
		}
	    }
	    break;
	  case KEY_HELP:
	    if (win_inputposevent(&event))
		(void)help_request(NULL, "sunview:rootwindow", &event);
	    break;
	  default:
	    break;
	}
	keyexit = 0;
    }
}

static int
root_menu_mgr(event)
	struct inputevent    *event;
{
	Menu_item     mi; /* Old and new menu item */
	int	      exit_local;

	if (GET_MENU(rootmenufile) <= 0) {
	    (void) _error(NO_PERROR, "invalid root menu");
	    return 1;
	}
	for (;;) {
	    struct inputevent tevent;

	    exit_local = 0;
	    tevent = *event;
	    mi = menu_show_using_fd(wmgr_rootmenu, rootfd, event);
	    if (mi)
		exit_local = MENU_ITEM(wmgr_rootmenu, mi, rootfd) == -1;
	    if (event_action(event) == MS_LEFT && !exit_local) {
		*event = tevent;
/*		win_setmouseposition(rootfd, event->ie_locx, event->ie_locy);*/
	    } else {
		break;
	    }
	}
	return (!exit_local);
}

static void
root_sigchldhandler()
{
	union	wait status;

	root_SIGCHLD = 0;
	while (wait3(&status, WNOHANG, (struct rusage *)0) > 0)
		{}
}

static void
root_sigwinchhandler()
{
	root_SIGWINCH = 0;
	(void)pw_damaged(pixwin);
	switch (root_image_type) {
	case ROOT_IMAGE_PATTERN:
		(void)pw_replrop(pixwin,
		    screen.scr_rect.r_left, screen.scr_rect.r_top,
		    screen.scr_rect.r_width, screen.scr_rect.r_height,
		    PIX_SRC | PIX_COLOR(root_color), root_image_pixrect, 0, 0);
		break;
	case ROOT_IMAGE_SOLID:
	default:
		(void)pw_writebackground(pixwin,
		    screen.scr_rect.r_left, screen.scr_rect.r_top,
		    screen.scr_rect.r_width, screen.scr_rect.r_height,
		    PIX_SRC | PIX_COLOR(root_color));
	}
	(void)pw_donedamaged(pixwin);
	return;
}

static void
root_sigchldcatcher()
{
	root_SIGCHLD = 1;
}

static void
root_sigwinchcatcher()
{
	root_SIGWINCH = 1;
}

static void
root_sigusr1catcher()
{
        if (sv_journal) {
                (void)sprintf(window_journal, "WINDOW_JOURNAL = OFF");
                setenv("WINDOW_JOURNAL", "OFF");
                sv_journal=FALSE;
        }
        else {
                (void)sprintf(window_journal, "WINDOW_JOURNAL = ON");
                setenv("WINDOW_JOURNAL","ON");
                sv_journal=TRUE;
        }
}

#ifdef ecd.suntools
  static void
root_sigtermhandler()
{
	root_SIGTERM = TRUE ;
}

#endif
static char *
get_home_dir()
{
	extern	char *getlogin();
	extern	struct	passwd *getpwnam(), *getpwuid();
	struct	passwd *passwdent;
	char	*home_dir = getenv("HOME"), *loginname;

	if (home_dir != NULL)
		return(home_dir);
	loginname = getlogin();
	if (loginname == NULL) {
		passwdent = getpwuid(getuid());
	} else {
		passwdent = getpwnam(loginname);
	}
	if (passwdent == NULL) {
		(void) _error(NO_PERROR,
			"cannot find user in password file");
		return(NULL);
	}
	if (passwdent->pw_dir == NULL) {
		(void) _error(NO_PERROR,
			"no home directory for %s in password file",
			passwdent->pw_name);
		return(NULL);
	}
	return(passwdent->pw_dir);
}

#define	ROOT_ARGBUFSIZE		1000
#define ROOT_SETUPFILE		"/.sunview"
#define	ROOT_SETUPFILE_ALT	"/.suntools"
#define	ROOT_MAXTOOLDELAY	10
#define	ROOT_DEFAULTSETUPFILE	"/usr/lib/.sunview"

static void
root_initialsetup(requestedfilename)
	char	*requestedfilename;
{
	register i;
	FILE	*file;
	char	filename[MAXNAMLEN], programname[MAXNAMLEN],
		otherargs[ROOT_ARGBUFSIZE];
	struct	rect rectnormal, recticonic;
	int	iconic, topchild, bottomchild, seconds, j;
	char	line[ARG_CHARS], full_programname[MAXPATHLEN];

	if (requestedfilename[0] == NULL) { /* first look for .sunview */
		char *home_dir = get_home_dir();
		if (home_dir == NULL)
			return;
		(void) strcpy(filename, home_dir);
		(void) strncat(filename, ROOT_SETUPFILE,
		    sizeof(filename)-1-strlen(filename)-strlen(
			ROOT_SETUPFILE));
	} else (void) strncpy(filename,requestedfilename,sizeof(filename)-1);
	file = fopen(filename, "r");
	if (!file && !requestedfilename[0]) { /* else get .suntools */
		char *home_dir = get_home_dir();
		if (home_dir == NULL)
			return;
		(void) strcpy(filename, home_dir);
		(void) strncat(filename, ROOT_SETUPFILE_ALT,
		    sizeof(filename)-1-strlen(filename)-strlen(
			ROOT_SETUPFILE_ALT));
		file = fopen(filename, "r");
	}
	if (!file && !requestedfilename[0]) {
	/* If default file not found in HOME, look in public library */
		(void) strcpy(filename, ROOT_DEFAULTSETUPFILE);
		file = fopen(filename, "r");
	}

/*  We used to not give an error if looking for default .sunview.
    Now that we check the defaults lib dir, we give an error message.
		if (requestedfilename[0] == NULL)
			return;
*/

/* If we cannot open startup file specified by the -s option
 * or we cannot open any of the default startup files then we exit
 * rather than return, so that we avoid hanging.
*/

	if (!file) {
		(void) _error("cannot open startup file %s", filename);
		exit(1);
	}

	while (fgets(line, sizeof (line), file)) {
		register char *t;
		for (t = line; isspace(*t); t++);
		if (*t == '#' || *t == '\0')
			continue;
		otherargs[0] = '\0';
		programname[0] = '\0';
		i = sscanf(line, "%s%hd%hd%hd%hd%hd%hd%hd%hd%hd%[^\n]\n",
		    programname,
		    &rectnormal.r_left, &rectnormal.r_top,
		    &rectnormal.r_width, &rectnormal.r_height,
		    &recticonic.r_left, &recticonic.r_top,
		    &recticonic.r_width, &recticonic.r_height,
		    &iconic, otherargs);
		if (i == EOF)
			break;
		if (i < 10 || i > 11) {
		   /*
		    * Just get progname and args.
		    */
		    otherargs[0] = '\0';
		    programname[0] = '\0';
		    j = sscanf(line, "%s%[^\n]\n", programname, otherargs);
		    if (j > 0) {
			iconic = 0;
			rect_construct(&recticonic, WMGR_SETPOS, WMGR_SETPOS,
			    WMGR_SETPOS, WMGR_SETPOS);
			rect_construct(&rectnormal, WMGR_SETPOS, WMGR_SETPOS,
			    WMGR_SETPOS, WMGR_SETPOS);
		    } else {
			(void)_error(NO_PERROR, "\
in file %s fscanf gave %ld, correct format is:\n\n\
program open-left open-top open-width open-height close-left close-top close-width close-height iconicflag [args] <newline>\n\
 OR\n\
program [args] <newline>\n",
				filename, i);
			continue;
		    }
		}
		/*
		 * Remember who top and bottom children windows are for use when
		 * trying to determine when tool is installed.
		 */
		topchild = win_getlink(rootfd, WL_TOPCHILD);
		bottomchild = win_getlink(rootfd, WL_BOTTOMCHILD);
		/*
		 * Fork tool.
		 */
		suntools_mark_close_on_exec();
		expand_path(programname, full_programname);
		(void) wmgr_forktool(full_programname, otherargs,
		    &rectnormal, &recticonic, iconic);
		/*
		 * Give tool chance to intall self in tree before starting next.
		 */
		for (seconds = 0; seconds < ROOT_MAXTOOLDELAY; seconds++) {
			usleep(1 << 20);
			if (topchild != win_getlink(rootfd, WL_TOPCHILD) ||
			    bottomchild != win_getlink(rootfd, WL_BOTTOMCHILD))
				break;
		}
	}
	(void) fclose(file);
}

Pkg_private
suntools_mark_close_on_exec()
{
	register i;
	int limit_fds = GETDTABLESIZE();

	/* Mark all fds (other than stds) as close on exec */
	for (i = 3; i < limit_fds; i++)
		(void) fcntl(i, F_SETFD, 1);
}

static void
root_set_pattern(token, ptr_single_color)
	char *token;
	struct singlecolor *ptr_single_color;
{
	char err[IL_ERRORMSG_SIZE];
	struct pixrect *mpr;

	if (strcmp(token, "on") == 0) {
		root_image_type = ROOT_IMAGE_PATTERN;
	} else if (strcmp(token, "off") == 0) {
		root_image_type = ROOT_IMAGE_SOLID;
	} else if (strcmp(token, "grey") == 0 || strcmp(token, "gray") == 0) {
		ptr_single_color->red = ptr_single_color->green =
		    ptr_single_color->blue = 128;
		root_color = 1;
		root_image_type = ROOT_IMAGE_SOLID;
	} else if ((mpr = icon_load_mpr(token, err))== (struct pixrect *)0) {
		(void) _error(NO_PERROR, "pattern file %s, error %s",
			token, err);
	} else {
		root_image_pixrect = mpr;
		root_image_type = ROOT_IMAGE_PATTERN;
	}
}

static void
root_set_background(filename)
	char	*filename;
{
	int		 x, y;
	FILE		*file;
	register struct pixrect	*tmp_pr, *root_pr;

	if ((file = fopen(filename, "r"))  == (FILE *) NULL)  {
		(void) _error("cannot open background file %s", filename);
		return;
	}
	if ((tmp_pr = pr_load(file, (colormap_t *)NULL/* Ignoring colormap */))
	    == (struct pixrect *) NULL)  {
		(void) _error(NO_PERROR, "cannot load background from %s",
			 filename);
		(void)fclose(file);
		return;
	}
	(void)fclose(file);
	root_pr =
	    mem_create(screen.scr_rect.r_width, screen.scr_rect.r_height, 1);
	/* Center image */
	x = (screen.scr_rect.r_width - tmp_pr->pr_width) / 2;
	y = (screen.scr_rect.r_height - tmp_pr->pr_height) / 2;
	/* Initialize background */
	switch (root_image_type) {
	case ROOT_IMAGE_PATTERN:
		(void)pr_replrop(root_pr, 0, 0,
		    screen.scr_rect.r_width, screen.scr_rect.r_height,
		    PIX_SRC | PIX_COLOR(root_color), root_image_pixrect, 			    0, 0);
		break;
	case ROOT_IMAGE_SOLID:
	default:
		(void)pr_rop(root_pr, 0, 0,
		    screen.scr_rect.r_width, screen.scr_rect.r_height,
		    PIX_SRC | PIX_COLOR(root_color), (Pixrect *)0, 0, 0);
	}
	/* Draw picture on image pixrect */
	(void)pr_rop(root_pr, x, y, tmp_pr->pr_width, tmp_pr->pr_height,
		PIX_SRC, tmp_pr, 0, 0);
	(void)pr_destroy(tmp_pr);
	root_image_pixrect = root_pr;
	/* PATTERN will cause image pixrect to be roped without replication */
	root_image_type = ROOT_IMAGE_PATTERN;
}

Pkg_private int
wmgr_menufile_changes()
{
	struct stat statb;
	int sa_count;

	if (wmgr_nextfile == 0) return 1;
	/* Whenever existing menu going up, stat menu files */
	for (sa_count = 0;sa_count < wmgr_nextfile;sa_count++) {
		if (stat(wmgr_stat_array[sa_count].name, &statb) < 0) {
			if (errno == ENOENT)
				return(1);
			return _error("menu file %s stat error",
				wmgr_stat_array[sa_count].name);
		}
		if (statb.st_mtime > wmgr_stat_array[sa_count].mftime)
			return(1);
	}
	return 0;
}

Pkg_private
wmgr_free_changes_array()
{   
    int sa_count = 0;
    
    while (sa_count < wmgr_nextfile) {
	free(wmgr_stat_array[sa_count].name);      /* file name */
	wmgr_stat_array[sa_count].name = NULL;
	wmgr_stat_array[sa_count].mftime = 0;
	sa_count++;
    }
    wmgr_nextfile = 0;
}

Pkg_private char *
wmgr_save2str(s, t)
	register char *s, *t;
{
	register int len = strlen(s) + 1;
	register char *p;

	if ((p = malloc((unsigned) len + (t ? strlen(t) + 1 : 0))) == NULL) {
		if (rootfd)
			(void)win_screendestroy(rootfd);
		(void) _error(NO_PERROR, "out of memory for menu strings");
		exit(1);
	}
	(void) strcpy(p, s);
	if (t)
		(void) strcpy(p + len, t);
	return (p);
}

Pkg_private char *
wmgr_savestr(s)
	register char *s;
{
	return wmgr_save2str(s, (char *) 0);
}

/* dummy proc for selection_set()
*/
static int
dummy_proc()
{
}


static void
root_start_service()
{
	register int	i;
	static char *args[2] = {"selection_svc", 0 };
	if (vfork() == 0)  {
		/* XXX: should this be getdtablesize? */
		for (i = 30; i > 2; i--)  {
			(void)close(i);
		}
		execvp(seln_svc_file, args, 0);
		(void)_error(NO_PERROR, "cannot fork selection service");
		usleep(7 << 20);
		_exit(1);
	}
}


static char *
basename(path)
	char *path;
{
	register char *p = path, c;

	while (c = *p++)
		if (c == '/')
			path = p;

	return path;
}

/* _error([no_perror, ] fmt [, arg ...]) */
/*VARARGS*/
Pkg_private int
_error(va_alist)
va_dcl
{
	int saved_errno;
	va_list ap;
	int no_perror = 0;
	char *fmt;
	extern int errno;

	saved_errno = errno;

	(void) fprintf(stderr, "%s: ", progname);

	va_start(ap);
	if ((fmt = va_arg(ap, char *)) == 0) {
		no_perror = 1;
		fmt = va_arg(ap, char *);
	}
	(void) _doprnt(fmt, ap, stderr);
	va_end(ap);

	if (no_perror)
		(void) fprintf(stderr, "\n");
	else {
		(void) fprintf(stderr, ": ");
		errno = saved_errno;
		perror("");
	}

	return -1;
}
