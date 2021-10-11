#ifndef lint
#ifdef sccs
static
char sccsid[] = "@(#)toolplaces.c 1.1 92/07/30";
#endif
#endif
/* 
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* 
 * 	TOOLPLACES
 *
 *	toolplaces.c,  Fri Jul  5 12:30:45 1985
 *	Print out positions of all tools on the desktop.
 *
 *	Craig Taylor, 
 *	Sun Microsystems
 *
 *	Options:
 *	  -u = update options that were specified.
 *	  -ua = update options and add option not specified.
 *	  -c = original command line without updated options.
 *	  -o = use old suntools format (should we stop supporting this?)
 *	  -O = old toolplaces format.  Does not open any of the system files.
 *
 *	This is a completely rewritten version of toolplaces.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <kvm.h>

/* Used to enumerate the tool positions */
#include <sys/file.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/cms_mono.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_ioctl.h>
#include <suntool/wmgr.h>

extern char	 *strcpy(), *strcat(), *strncat();

static char	**get_argv();

#define error(perror_p, emsg) { \
    (void)fprintf(stderr, "%s: %s\n", prog_name, emsg); \
    if ((int)perror_p) perror(prog_name); \
    }

#define error1(perror_p, emsg, earg) { \
    (void)fprintf(stderr, "%s: ", prog_name); \
    (void)fprintf(stderr, emsg, earg); \
    (void)fputc('\n', stderr); \
    if ((int)perror_p) perror(prog_name); \
    }

static enum {STD, UPDATE, UPDATE_ALL, OLDER, OLDEST, CMD_LINE} style;

static kvm_t *kd;
static char *prog_name;
static struct colormapseg cms;
struct cms_map cmap;
static int fg_color, bg_color;
static unsigned char cmap_red[256];
static unsigned char cmap_green[256];
static unsigned char cmap_blue[256];

#ifdef STANDALONE
main(argc, argv)
#else
toolplaces_main(argc, argv)
#endif
int argc;
char **argv;
{   
    char **targv, name[WIN_NAMESIZE];
    int toolfd, wfd, link, iconic, i, user_flags;
    struct rect rect, rectsaved, recttemp;
    struct screen screen;

    prog_name = *argv;
    parse_options(argc, argv);
    /*
     * Determine parent and get root fd
     */
    if (we_getparentwindow(name)) {
	error(0, "Parent window not in the environment.");
	error(0, "This program must be run under suntools.")
	exit(1);
    }
    if ((wfd = open(name, O_RDONLY, 0)) < 0) {
	error(1, "Parent would not open.");
    }
    (void)win_screenget(wfd, &screen);
    (void)close(wfd);
    if ((wfd = open(screen.scr_rootname, O_RDONLY, 0)) < 0)  {
	error1(1, "Screen(%s) would not open.", screen.scr_rootname);
    }
    if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, argv[0])) == NULL)
	exit(1);
    if (chdir("/dev") < 0)
	error(0, "Can't connect to /dev.");
    /*
     * Printout rects of root children.
     */
    for (link = win_getlink(wfd, WL_OLDESTCHILD); link != WIN_NULLLINK;) {
	/* 
	 * Open tool window
	 */
	(void)win_numbertoname(link, name);
	if ((toolfd = open(name, O_RDONLY, 0)) < 0)   {
	    error(1, "One of the top level windows would not open.");
	}
	/* 
	 * Get rect data
	 */
	(void)win_getrect(toolfd, &rect);
	(void)win_getsavedrect(toolfd, &rectsaved);
	user_flags = win_getuserflags(toolfd);
	iconic = (user_flags & WMGR_ICONIC);
	
	if (!iconic && (user_flags & WMGR_SUBFRAME))  { /* Don't process the window if it is a subframe */
	    goto Skip_to_Next;
	}
	     
	
	if (iconic) {
	    recttemp = rect;
	    rect = rectsaved;
	    rectsaved = recttemp;
	}
	/* 
	 * Get original argv from the process owning the tool.
	 */
	if (style != OLDEST) targv = get_argv(win_getowner(toolfd));
	/* 
	 * Print entry in the style requested.
	 */
	switch (style) {
	  case STD:
	    (void)printf("%-10s -Wp %4d %3d -Ws %3d %3d -WP %4d %3d ",
		   targv[0], 
		   rect.r_left, rect.r_top, rect.r_width, rect.r_height,
		   rectsaved.r_left, rectsaved.r_top,
		   rectsaved.r_width, rectsaved.r_height);
	    if (iconic) (void)printf("-Wi ");
	    print_colormap(toolfd, &screen);
	    print_striped_args(targv+1);
	    break;

	  case UPDATE:
	  case UPDATE_ALL:
	    (void)printf("%-10s ", targv[0]);
	    get_colormap_entries(toolfd, &screen);
	    print_updated_args(targv+1, &rect, &rectsaved, iconic,
			       style == UPDATE_ALL);
	    break;
	    
	  case OLDER:
	    (void)printf("%-10s %4d %3d %3d %3d %4d %3d %3d %3d  %d  ",
		   targv[0], 
		   rect.r_left, rect.r_top, rect.r_width, rect.r_height,
		   rectsaved.r_left, rectsaved.r_top,
		   rectsaved.r_width, rectsaved.r_height,
		   iconic);
	    print_colormap(toolfd, &screen);
	    print_striped_args(targv+1);
	    break;

	  case OLDEST:  /* Useless format */
	    (void)printf("toolname    %4d %3d %3d %3d    %4d %3d %3d %3d     %d\n",
		   rect.r_left, rect.r_top, rect.r_width, rect.r_height,
		   rectsaved.r_left, rectsaved.r_top,
		   rectsaved.r_width, rectsaved.r_height,
		   iconic);
	    break;
	    
	  case CMD_LINE:
	    i = 0;
	    while (targv[i] != NULL) print_arg(targv[i++]);
	    (void)printf("\n");
	    break;

	}
Skip_to_Next:	
	link = win_getlink(toolfd, WL_YOUNGERSIB); /* Next */
	(void)close(toolfd);
    }
    (void)close(wfd);
} /* main */

static
parse_options(argc, argv)
	int argc;
	char **argv;
{   
    style = STD;
    if (argc > 2)
    {   
	(void)fprintf(stderr, "Usage: %s [ -uoO ] [ -help ]\n", *argv);
	exit(1);
    }
    argv++;
    if (argc == 2) {
	if (! strcmp("-a", *argv)) style = STD;
	else if (! strcmp("-u", *argv)) style = UPDATE;
	else if (! strcmp("-ua", *argv)) style = UPDATE_ALL;
	else if (! strcmp("-o", *argv)) style = OLDER;
	else if (! strcmp("-O", *argv)) style = OLDEST;
	else if (! strcmp("-c", *argv)) style = CMD_LINE;
	else if (! strcmp("-help", *argv)) {
	    (void)fprintf(stderr, 
"TOOLPLACES prints the locations and other relevant information about\n");
	    (void)fprintf(stderr,
"currently active tools.\n");
	    (void)fprintf(stderr, "The options are:\n");
	    (void)fprintf(stderr,
"  -u\tprint updated information only, e.g.,\n");
	    (void)fprintf(stderr,
"\tif icon position information isn't specified when the tool is\n");
	    (void)fprintf(stderr,
"\tstarted then toolplaces won't print the icon's position.\n");
	    (void)fprintf(stderr,
"  -o\tprint the information in the older suntools format.\n");
	    (void)fprintf(stderr,
"  -O\tprint the information in the original toolplaces format without names.\n");
	    (void)fprintf(stderr,
"\nThe default behavior prints all tool information.\n\n");
	}
	else {
	    (void)fprintf(stderr,
		    "Unrecognized option %s.  Usage: %s [ -uoO ] [ -help ]\n",
		    *argv, *(argv - 1));
	    exit(1);
	}
    }
} /* parse_options */


/* 
 * Remove all positioning and sizing information and print the remaining
 * options.
 */
static
print_striped_args(argv)
	char **argv;
{   
    while (*argv != NULL) {
	if (!((strcmp("-Ww", *argv) && strcmp("-width", *argv) &&
	       strcmp("-Wh", *argv) && strcmp("-height", *argv)))) {
	    argv += 2;
	} else if (!((strcmp("-Ws", *argv) && strcmp("-size", *argv) &&
		      strcmp("-Wp", *argv) && strcmp("-position", *argv) &&
		      strcmp("-WP", *argv) && strcmp("-icon_position", *argv)))) {
	    argv += 3;
	} else if (!((strcmp("-Wi", *argv) && strcmp("-iconic", *argv) &&
		      strcmp("-WH", *argv) && strcmp("-help", *argv)))) {
	    argv += 1;
	} else if (!(strcmp("-Wf", *argv) &&
		     strcmp("-foreground_color", *argv) &&
		     strcmp("-Wb", *argv) &&
		     strcmp("-background_color", *argv))) {
	    argv += 4;
	} else {
	    do print_arg(*argv++);
	    while (*argv != NULL && **argv != '-');
	}
    }
    puts("");
} /* print_striped_args */


/* 
 * Search for positioning and sizing options and update their values.
 * If all_info is true added any any missing options to the end of the list.
 */
static
print_updated_args(argv, rect, srect, iconic, all_info)
	char **argv;
	struct rect *rect,  *srect;
	int iconic, all_info;
{   
    int flgp, flgP, flgs, flgi, flgf, flgb;
    flgp = flgP = flgs = flgi = flgf = flgb = 0;
    while (*argv != NULL) {
	if (!((strcmp("-Ww", *argv) && strcmp("-width", *argv) &&
	       strcmp("-Wh", *argv) && strcmp("-height", *argv)))) {
	    argv += 2;
	} else if (!(strcmp("-Wp", *argv) && strcmp("-position", *argv))) {
	    (void)printf("%s %4d %3d ", *argv, rect->r_left, rect->r_top);
	    argv += 3; flgp++;
	} else if (!(strcmp("-Ws", *argv) && strcmp("-size", *argv))) {
	    (void)printf("%s %4d %3d ", *argv, rect->r_width, rect->r_height);
	    argv += 3; flgs++;
	} else if (!(strcmp("-WP", *argv) && strcmp("-icon_position", *argv))) {
	    (void)printf("%s %3d %3d ", *argv, srect->r_left, srect->r_top);
	    argv += 3; flgP++;
	} else if (!(strcmp("-Wi", *argv) && strcmp("-iconic", *argv))) {
	    if (iconic) (void)printf("%s ", *argv);
	    argv += 1; flgi++;
	} else if (!(strcmp("-Wf", *argv) &&
		     strcmp("-foreground_color", *argv))) {
	    argv += 4; flgf++;
	    if (fg_color) (void)printf("%s %3d %3d %3d ", *argv, 
				 cmap_red[1], cmap_green[1], cmap_blue[1]);
	} else if (!(strcmp("-Wb", *argv) &&
		     strcmp("-background_color", *argv))) {
	    argv += 4; flgb++;
	    if (bg_color) (void)printf("%s %3d %3d %3d ", *argv, 
				 cmap_red[0], cmap_green[0], cmap_blue[0]);
	} else if (!(strcmp("-WH", *argv) && strcmp("-help", *argv))) {
	    argv += 1;
	} else {
	    do print_arg(*argv++);
	    while (*argv != NULL && **argv != '-');
	}
    }
    if (all_info) {
	if (!flgp) (void)printf("%s %d %d ", "-Wp", rect->r_left, rect->r_top);
	if (!flgs) (void)printf("%s %d %d ", "-Ws", rect->r_width, rect->r_height);
	if (!flgP) (void)printf("%s %d %d ", "-WP", srect->r_left, srect->r_top);
	if (!flgf && fg_color) (void)printf("-Wf %3d %3d %3d ",
				      cmap_red[0], cmap_green[0], cmap_blue[0]);
	if (!flgb && bg_color) (void)printf("-Wb %3d %3d %3d ",
				      cmap_red[1], cmap_green[1], cmap_blue[1]);
    }	
    if (!flgi && iconic) (void)printf("%s", "-Wi");
    puts("");
} /* print_updated_args */


static
print_arg(arg)
	char *arg;
{
    char *cp;
    int quote;
    
    if (*arg)
	for (cp = arg; *cp; cp++)
	    if (quote = *cp == ' ') break;
    if (quote || *arg == 0)
	(void)printf("\"%s\" ", arg);
    else
	(void)printf("%s ", arg);
}


static
get_colormap_entries(wfd, screen)
	int wfd;
	struct screen *screen;
{   
/*  cms.cms_size = 256;  Win_getcms bug.  Should not have to call it twice */
    bg_color = fg_color = 0;
    cmap.cm_red = cmap.cm_green = cmap.cm_blue = 0;
    (void)win_getcms(wfd, &cms, &cmap); /* Initialize cms_size to be the correct
				   * number of entries. */
    if (cms.cms_size != 2 || strcmp(cms.cms_name, CMS_MONOCHROME) == 0) return;
    cmap.cm_red = cmap_red;
    cmap.cm_green = cmap_green;
    cmap.cm_blue = cmap_blue;
    cms.cms_addr = 0;
    (void)win_getcms(wfd, &cms, &cmap);
    bg_color = (cmap_red[0] != screen->scr_background.red ||
		cmap_green[0] != screen->scr_background.green ||
		cmap_blue[0] != screen->scr_background.blue);
    fg_color = (cmap_red[1] != screen->scr_foreground.red ||
		cmap_green[1] != screen->scr_foreground.green ||
		cmap_blue[1] != screen->scr_foreground.blue);
} /* get_colormap_entries */


static
print_colormap(wfd, screen)
	int wfd;
	struct screen *screen;
{   
    get_colormap_entries(wfd, screen);
    if (bg_color)
	(void)printf("-Wb %3d %3d %3d ", cmap_red[0], cmap_green[0], cmap_blue[0]);
    if (fg_color)
	(void)printf("-Wf %3d %3d %3d ", cmap_red[1], cmap_green[1], cmap_blue[1]);
}
    

/* 
 * Get argv for requested pid
 */

static char **
get_argv(pid)
	short pid;
{   
    static char *def_argv[2] = {"toolname", NULL};
    static char **argv = def_argv;
    struct proc *proc;
    struct user *u;

    if (argv != def_argv) {
	free((char *)argv);
        argv = def_argv;
    }
    if (!pid) return argv;

    if ((proc = kvm_getproc(kd, pid)) == NULL) {
	error1(0, "No process %d.", pid);
	return argv;
    }
    if ((u = kvm_getu(kd, proc)) == NULL) {
	error1(0, "No u-area for process %d.", pid);
	return argv;
    }
    if (kvm_getcmd(kd, proc, u, &argv, NULL) == -1)
	error1(0, "Unrecognizable u-area for process %d.", pid);
    return argv;
} /* getargv */
