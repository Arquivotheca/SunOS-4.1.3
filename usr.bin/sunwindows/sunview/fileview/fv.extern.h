/*	@(#)fv.extern.h 1.1 92/07/30 SMI	*/
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

extern Frame Fv_frame;			/* Base frame */
extern Panel Fv_panel;			/* Command panel */
extern FV_TREE_WINDOW Fv_tree;		/* Path window stuff */
extern FV_FOLDER_WINDOW Fv_foldr;	/* Folder window stuff */

extern Pixfont *Fv_font;		/* Current font */
extern short Fv_fontwidth[];		/* Current font character widths */
extern struct pr_size Fv_fontsize;	/* Width and height of standard character */

extern FV_TNODE *Fv_troot;		/* Real root of tree '/' */
extern FV_TNODE *Fv_thead;		/* Current root of tree */
extern FV_TNODE *Fv_sibling;		/* Current root's sibling */
extern FV_TNODE *Fv_current;		/* Current tree node */
extern FV_TNODE *Fv_lastcurrent;	/* Last current tree node */
extern FV_TNODE *Fv_tselected;		/* Selected tree node */

extern Pixrect *Fv_icon[];	/* Icons */
extern Pixrect *Fv_lock;			/* Locked folder icon */
extern Pixrect *Fv_invert[];	/* Inverted icons */
extern Pixrect *Fv_list_icon[];	/* List icons */

extern FV_BIND Fv_bind[];	/* Bound list */
extern int Fv_nbind;			/* Number of bound objects */
extern short Fv_nappbind;		/* Number of sorted application icons */

#ifdef SV1
# ifdef PROTO
extern char sundesk_path[];		/* Font directory environment variable */
# else
extern Panel_item Fv_errmsg;		/* Error message for non-OPEN LOOK window */
# endif
#endif

extern short Fv_margin;		/* Window margin */
extern int Fv_winheight;
extern int Fv_winwidth;
extern int Fv_screenwidth;
extern int Fv_screenheight;

/* Properties */
extern int Fv_style;			/* Display style */
extern char Fv_filter[];		/* Folder filter */
extern BOOLEAN Fv_see_dot;		/* See dot files? */
extern short Fv_sortby;			/* Sort by what? */
extern short Fv_editor;			/* Which document editor to use */
extern short Fv_wantdu;			/* Do du -s on folders */
extern BOOLEAN Fv_confirm_delete;	/* Confirm deletions */
extern short Fv_update_interval;
extern char Fv_print_script[];
extern char Fv_other_editor[];
extern int Fv_maxwidth;

extern BOOLEAN Fv_treeview;		/* Tree view? */
extern char Fv_path[];	/* Current path */
extern char Fv_openfolder_path[];	/* Open path */

extern FV_FILE *Fv_file[];
extern int Fv_nfiles;			/* # in directory */
extern char *Fv_undelete;		/* Undelete buffer */

extern BOOLEAN Fv_running;		/* File manager in window main loop? */
extern BOOLEAN Fv_dont_paint;		/* Don't paint canvases? */
extern BOOLEAN Fv_fixed;		/* Fixed width fonts? */
extern BOOLEAN Fv_color;		/* Color monitor? */
extern BOOLEAN Fv_automount;		/* Automounter running? */
extern BOOLEAN Fv_write_access;	/* Can write in current directory? */
extern char *Fv_home;			/* Home folder environment variable */

extern char *Fv_history[];
extern short Fv_nhistory;

/* Error messages */
extern char *Fv_message[];
extern char *sys_errlist[];		/* System error messages */

extern char *getenv(), *strchr(), *malloc(), *fv_malloc();
