#ifndef lint
static  char sccsid[] = "@(#)fv.glob.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

#ifdef SV1
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>
#else
#include <view2/view2.h>
#include <view2/panel.h>
#include <view2/canvas.h>
#include <view2/scrollbar.h>
#endif

#include "fv.port.h"
#include "fv.h"

Frame Fv_frame;			/* Base frame */
Panel Fv_panel;			/* Command panel */
FV_TREE_WINDOW Fv_tree;		/* Path window stuff */
FV_FOLDER_WINDOW Fv_foldr;	/* Folder window stuff */

Pixfont *Fv_font;		/* Current font */
short Fv_fontwidth[96];		/* Current font character widths */
struct pr_size Fv_fontsize;	/* Width and height of standard character */

FV_TNODE *Fv_troot;		/* Real root of tree '/' */
FV_TNODE *Fv_thead;		/* Current root of tree */
FV_TNODE *Fv_sibling;		/* Current root's sibling */
FV_TNODE *Fv_current;		/* Current tree node */
FV_TNODE *Fv_lastcurrent;	/* Last current tree node */
FV_TNODE *Fv_tselected;		/* Selected tree node */

Pixrect *Fv_icon[FV_MAXOBJECTS];	/* Icons */
Pixrect *Fv_lock;			/* Locked folder icon */
Pixrect *Fv_invert[FV_MAXOBJECTS];	/* Inverted icons */
Pixrect *Fv_list_icon[FV_MAXOBJECTS];	/* List icons */

FV_BIND Fv_bind[MAXBIND];	/* Bound list */
int Fv_nbind;			/* Number of bound objects */
short Fv_nappbind;		/* Number of sorted application icons */

#ifdef SV1
# ifdef PROTO
char sundesk_path[80];		/* Font directory environment variable */
# else
Panel_item Fv_errmsg;		/* Error message for non-OPEN LOOK window */
# endif
#endif

short Fv_margin;		/* Window margin */
int Fv_winheight;
int Fv_winwidth;
int Fv_screenwidth;
int Fv_screenheight;

/* Properties */
int Fv_style;			/* Display style */
char Fv_filter[81];		/* Folder filter */
BOOLEAN Fv_see_dot;		/* See dot files? */
short Fv_sortby;		/* Sort by what? */
short Fv_editor;		/* Which document editor to use */
short Fv_wantdu;		/* Do du -s on folders */
BOOLEAN Fv_confirm_delete;	/* Confirm deletions */
short Fv_update_interval;
char Fv_print_script[80];
char Fv_other_editor[80];
int Fv_maxwidth;

BOOLEAN Fv_treeview;		/* Tree view? */
char Fv_path[MAXPATH+1];	/* Current path */
char Fv_openfolder_path[MAXPATH+1];	/* Open path */

FV_FILE *Fv_file[MAXFILES];
int Fv_nfiles;			/* # in directory */
char *Fv_undelete;		/* Undelete buffer */

BOOLEAN Fv_running;		/* File manager in window main loop? */
BOOLEAN Fv_dont_paint;		/* Don't paint canvases? */
BOOLEAN Fv_fixed;		/* Fixed width fonts? */
BOOLEAN Fv_color;		/* Color monitor? */
BOOLEAN Fv_automount;		/* Automounter running? */
BOOLEAN Fv_write_access;	/* Can write in current directory? */
char *Fv_home;			/* Home folder environment variable */

char *Fv_history[FV_MAXHISTORY];
short Fv_nhistory;

/* Messages */

char *Fv_message[] = {
"Sorry, can't create new window; try deleting some windows",
"Out of memory",
"Can't enter folder '%s': %s",
"Printing %s...",
"Can't create '%s': %s",
"Can't remove '%s': %s",
"Deleting '%s' will move you to '%s', continue?",
"Too many magic numbers",
"You must select something first",
"Sorry, only 128 bind entries allowed",
"Sorry, can't have / in pattern",
"You must enter a pattern or magic number",
"You must enter an Icon Image File",
"Unknown magic number",
"Please press Load button first",
"Can't find client %d",
"%s to '%s' succeeded",
"Copy",
"Move",
"Can't read '%s': %s",
"Can't copy '%s'",
"Are you sure you want to remove these file(s)?",
"Removing '%s' failed: %s",
"Undeleting '%s' failed: %s",
"Sorry, '%s' already exists",
"Renaming '%s'...",
"Linking '%s' to workspace...",
"Can't read more than %d files",
"Can't find '%s'",
"Can't rename '%s': %s",
"Invalid '%s' date; I expect month/day/year, eg 12/25/88",
"%s is not a valid owner name",
"Will search from current folder: '%s'",
"Ran out of pty's:  Find aborted",
"Illegal character '%c' in filename",
"Internal Error: can't find list box data\n",
"",
"fileview: seln_create failed!\n",
"%s these file(s) by entering the target folder and selecting 'Paste'",
"No selection on clipboard",
"'%s' has been modified",
"Please set Show Hidden Files",
"Command failed",
"Can't run '%s': %s",
"Starting '%s'; check console for errors"
};
