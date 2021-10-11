#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tla_make.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *  tla_make.c - Make tool using attribute list.
 */

#include <sys/file.h>
#include <stdio.h>
#include <varargs.h>
#include <suntool/tool_hs.h>
#include <sunwindow/sun.h>
#include <sunwindow/win_ioctl.h>
#include <suntool/tool_impl.h>
#include <suntool/wmgr.h>
#include "sunwindow/sv_malloc.h"

#ifdef KEYMAP_DEBUG
#include "../../libsunwindow/win/win_keymap.h"
#else
#include <sunwindow/win_keymap.h>
#endif

#define MAX_ATTRS	4096

extern struct cursor tool_cursor;

int tool_tried_to_get_font_from_defaults; /* Also in tla_parse.c,  pw_text.c */
Bool defaults_get_boolean();

/*VARARGS0*/
struct	tool *
tool_make(va_alist)
va_dcl
{
	int		 iconic = 0,
			 parent_fd, tool_fd,
			 value,
			 parentlink, link;
	char		 parentname[WIN_NAMESIZE];
	struct inputmask im;
	struct rect	 rectnormal, recticon;
	struct tool	*tool;
	short		got_env_params = FALSE;
	va_list		valist;
	caddr_t		avlist[ATTR_STANDARD_SIZE];
        static char            *pr_grp;

extern int	tool_handlesigwinchstd(), tool_selectedstd();
extern Menu tool_walkmenu();
extern char *calloc(), *defaults_get_string();


/*	Make sure sunview is running  */

	if (we_getparentwindow(parentname)) {
		(void)fprintf(stderr,
			"Tool not passed parent window in environment\n");
		goto Failure;
	}

/*	Get tool structs & open its fd	*/

	tool = (struct tool *) (LINT_CAST(calloc(1, sizeof (struct tool))));
	if ( tool == (struct tool *) NULL) {
		return tool;
	}

	if (!tool_tried_to_get_font_from_defaults) {
	    char *fname;
	    
	    fname = defaults_get_string("/SunView/Font", "", (int *)NULL);
	    if (*fname != '\0') (void)setenv("DEFAULT_FONT", fname);
	    tool_tried_to_get_font_from_defaults = 1;
	}	
	
	tool->tl_menu = tool_walkmenu(tool);
	if ((int) defaults_get_boolean("/SunView/Embolden_labels", 
		(Bool)(LINT_CAST(FALSE)), (int *)NULL))
	    tool->tl_flags |= TOOL_EMBOLDEN_LABEL;
	if ((tool_fd = win_getnewwindow())<0) {
		(void)fprintf(stderr, "couldn't get a new tool window\n");
		goto Failure;
	}
	tool->tl_windowfd = tool_fd;
	tool->tl_io.tio_handlesigwinch = tool_handlesigwinchstd;
	tool->tl_io.tio_selected = tool_selectedstd;
	
/*  Open parent window */

	if ((parent_fd = open(parentname, O_RDONLY, 0)) < 0) {
		(void)fprintf(stderr, "%s (parent) would not open\n", parentname);
		goto Failure;
	}

/*  Setup links	*/

	parentlink = win_nametonumber(parentname);
	(void)win_setlink(tool_fd, WL_PARENT, parentlink);
	link = win_getlink(parent_fd, WL_TOPCHILD);
	(void)win_setlink(tool_fd, WL_OLDERSIB, link);

/*  Give tool own cursor */

	(void)win_setcursor(tool_fd, &tool_cursor);

/*  Open pixwin */

	if ((tool->tl_pixwin = pw_open_monochrome(tool_fd)) ==
	    (struct pixwin *)0) {
		goto Failure;
	}

/*  Open pf_sys */

	(void)pw_pfsysopen();

/*  Set up input mask so can do menu/window management stuff */

	(void)input_imnull(&im);
	im.im_flags |= IM_NEGEVENT;
	im.im_flags |= IM_ASCII | IM_META | IM_NEGMETA;
/*
 * Make sure input mask allows tracking cursor motion and buttons
 * for tool boundary moving.
 * Don't want to always track cursor else would drag in tool's code
 * and data when simply moved over it.
 */

	win_setinputcodebit(&im, LOC_MOVEWHILEBUTDOWN);
	win_keymap_set_smask(tool_fd, MS_LEFT);
	win_keymap_set_smask(tool_fd, MS_MIDDLE);
	win_keymap_set_smask(tool_fd, MS_RIGHT);
	win_keymap_set_smask(tool_fd, OPEN_KEY);
	win_keymap_set_smask(tool_fd, TOP_KEY);
	win_keymap_set_smask(tool_fd, DELETE_KEY);
	win_keymap_set_imask_from_std_bind(&im, MS_LEFT);
	win_keymap_set_imask_from_std_bind(&im, MS_MIDDLE);
	win_keymap_set_imask_from_std_bind(&im, MS_RIGHT);
	win_keymap_set_imask_from_std_bind(&im, OPEN_KEY);
	win_keymap_set_imask_from_std_bind(&im, TOP_KEY);
	win_keymap_set_imask_from_std_bind(&im, DELETE_KEY);
	win_setinputcodebit(&im, KBD_REQUEST);
	(void)win_setinputmask(tool_fd, &im, (struct inputmask *)0, WIN_NULLLINK);

        
        pr_grp = sv_malloc(50);
        sprintf (pr_grp, "PGRP_PID= %d",getpgrp(getpid()));
        if (putenv (pr_grp) != 0)
           printf("Unable to add PGRP_PID to env.\n");;

/*	Get any position / size / state info from environment.
 *	Iconic state could be passed in environment or in command-line;
 *	this ordering gives command-line precedence
 */


	rect_construct(&rectnormal, WMGR_SETPOS, WMGR_SETPOS,
				    WMGR_SETPOS, WMGR_SETPOS);
	recticon = rectnormal;
	(void)win_setrect(tool_fd, &rectnormal);
	(void)win_setsavedrect(tool_fd, &recticon);

	if (we_getinitdata(&rectnormal, &recticon, &iconic)) {
		struct screen screen;		/*   no initializers	*/
		int rootnumber;

		(void)win_screenget(parent_fd, &screen);
		rootnumber = win_nametonumber(screen.scr_rootname);
		if (parentlink != rootnumber) {
					/* tool within a tool -- punt	*/
			rect_construct(&rectnormal, 0, 0, 200, 400);
			recticon = rectnormal;
		}
	} else {		/*  Got some environment parameters	*/
		Rect rinit;

		/* only got env params if they are not
		 * all set to the default values.
		 */
		rect_construct(&rinit, WMGR_SETPOS, WMGR_SETPOS,
				WMGR_SETPOS, WMGR_SETPOS);
		got_env_params = iconic || !rect_equal(&rinit, &rectnormal) ||
		    !rect_equal(&rinit, &recticon);

		(void)we_clearinitdata();
	}

/*	Iconic state and name stripe have to be determined before any
 *	size parameters are interpreted, so the attribute list is mashed
 *	and explicitly interrogated for them here.
 */
 	va_start(valist);
	if ((attr_make(avlist, ATTR_STANDARD_SIZE, valist))
	    == (caddr_t *)NULL)  {
	    	va_end(valist);
		goto Failure;
	}
	va_end(valist);
	if (tool_find_attribute(avlist, (int)(LINT_CAST(WIN_ICONIC)), 
			(char **)(LINT_CAST(&value))) && value){
		iconic = TRUE;
	}

	if (tool_find_attribute(avlist, (int)(LINT_CAST(WIN_NAME_STRIPE)), 
		(char **)(LINT_CAST(&value)))  && !value) {
		tool->tl_flags &= ~TOOL_NAMESTRIPE;
	} else {
		tool->tl_flags |= TOOL_NAMESTRIPE;	/* default */
	}

	(void)win_lockdata(parent_fd);

	if (iconic)  {
		int	flags;

		tool->tl_flags |= TOOL_ICONIC;
		(void)win_setrect(tool_fd, &recticon);
		(void)win_setsavedrect(tool_fd, &rectnormal);
		flags = win_getuserflags(tool_fd);
		flags |= WMGR_ICONIC;
		(void)win_setuserflags(tool_fd, flags);
	} else {
		(void)win_setrect(tool_fd, &rectnormal);
		(void)win_setsavedrect(tool_fd, &recticon);
	}

/*	Now apply rest of attributes	*/

	if (tool_set_attr(tool, avlist) == -1) {
		(void)tool_destroy(tool);
		return((struct tool *)NULL);
	}

/*	Normalize state before calling wmgr placement	*/

	/* Make sure we set the environment parameters last */
	if (got_env_params) {
		if (iconic)  {
			tool->tl_flags |= TOOL_ICONIC;
			(void)win_setrect(tool_fd, &recticon);
			(void)win_setsavedrect(tool_fd, &rectnormal);
		} else {
			(void)win_setrect(tool_fd, &rectnormal);
			(void)win_setsavedrect(tool_fd, &recticon);
		}
	} else {
		if (tool->tl_flags & TOOL_ICONIC)  {
			(void)win_getrect(tool_fd, &recticon);
			(void)win_getsavedrect(tool_fd, &rectnormal);
		} else {
			(void)win_getrect(tool_fd, &rectnormal);
			(void)win_getsavedrect(tool_fd, &recticon);
		}
	}

/*  Use everything to establish positions and size      */

	if (recticon.r_width == WMGR_SETPOS) {
		recticon.r_width = TOOL_ICONWIDTH;
	}
	if (recticon.r_height == WMGR_SETPOS) {
		recticon.r_height = TOOL_ICONHEIGHT;
	}
        (void)wmgr_set_tool_rect(parent_fd, tool_fd, &rectnormal);
        (void)wmgr_set_icon_rect(parent_fd, tool_fd, &recticon);

  	if (tool->tl_flags & TOOL_ICONIC)  {
		int	flags;

		tool->tl_rectcache = recticon;
		(void)win_setrect(tool_fd, &recticon);
		(void)win_setsavedrect(tool_fd, &rectnormal);
		flags = win_getuserflags(tool_fd);
		flags |= WMGR_ICONIC;
		(void)win_setuserflags(tool_fd, flags);
	} else {
		int	flags;

		tool->tl_rectcache = rectnormal;
		flags = win_getuserflags(tool_fd);
		flags &= ~WMGR_ICONIC;
		(void)win_setuserflags(tool_fd, flags);
	}
        (void)win_unlockdata(parent_fd);
	(void)close(parent_fd);

/*  Cached rect is self relative.  */

	tool->tl_rectcache.r_left = 0;
	tool->tl_rectcache.r_top = 0;

	return(tool);

Failure:
	free((char *)(LINT_CAST(tool)));
	return((struct tool *)0);
}
