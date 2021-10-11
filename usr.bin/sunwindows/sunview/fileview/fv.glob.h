/*	@(#)fv.glob.h 1.1 92/07/30 SMI	*/

/* EXTERNS: */

extern Frame Fv_frame;			/* Base frame */
extern Panel Fv_panel;			/* Command panel */
extern FV_TREE_WINDOW Fv_tree;		/* Folder window stuff */
extern FV_FOLDER_WINDOW Fv_foldr;	/* Folder window stuff */
extern Panel_item Fv_addparent;		/* Add node's parent button */
extern char *Fv_winerr;			/* Window create error message */

extern Pixfont *Fv_font;		/* Current font */
extern short Fv_fontwidth[];		/* Current font char widths */
extern struct pr_size Fv_fontsize;	/* Standard char size (ht & wd) */

extern char *sys_errlist[];		/* System error messages */

extern FV_TNODE *Fv_troot;		/* Real root of tree '/' */
extern FV_TNODE *Fv_thead;		/* Current root of tree */
extern FV_TNODE *Fv_tselected;		/* Current tree node */
extern FV_TNODE *Fv_current;		/* Current tree node */
extern FV_TNODE *Fv_lastcurrent;	/* Last current tree node */
extern FV_TNODE *Fv_sibling;		/* Current root's sibling */

extern Pixrect *Fv_icon[];		/* Objects */
extern Pixrect *Fv_invert[];		/* Inverted objects */
extern Pixrect *Fv_lock;		/* Locked folder */
extern int Fv_ticonheight;
extern int Fv_ticontop;

extern FV_BIND Fv_bind[];		/* Bound list */
extern int Fv_nbind;			/* Number of bound objects */

extern int Fv_foldr_top;
extern int Fv_tree_top;
extern int Fv_winheight;
extern int Fv_winwidth;
extern int Fv_screenwidth;
extern int Fv_screenheight;

extern int Fv_style;			/* Display style */
extern char Fv_filter[];		/* Folder filter */
extern BOOLEAN Fv_see_dot;		/* See dot files? */
extern short Fv_sortby;			/* Sort by what? */

extern BOOLEAN Fv_treeview;		/* Tree view state */
extern char Fv_path[];			/* Current path */
extern char Fv_openfolder_path[];	/* Open path */

extern BOOLEAN Fv_expert;
extern int Fv_record;
extern short Fv_editor;
extern FV_FILE *Fv_file[];
extern short Fv_wantdu;			/* du -s on folders */
extern BOOLEAN Fv_confirm_delete;	/* Confirm delete */
extern BOOLEAN Fv_running;		/* In window main loop */
extern BOOLEAN Fv_dont_paint;
extern BOOLEAN Fv_fixed;
extern BOOLEAN Fv_color;
extern BOOLEAN Fv_write_access;
extern Panel_item Fv_back;
extern char *Fv_undelete;
extern int Fv_nfiles;			/* # in directory */
extern char *Fv_nomemory;
extern char Fv_print_script[];
extern short Fv_update_interval;
extern char *Fv_history[];
extern short Fv_nhistory;
extern short Fv_nappbind;
extern Panel_item Fv_errmsg;
extern BOOLEAN Fv_automount;
extern short Fv_margin;
extern char *Fv_home;
extern Pixrect *Fv_list_icon[];		/* Samll icon images */

#include <string.h>
#include <sys/types.h>
char *malloc(), *fv_malloc(), *sprintf(), *getenv(), *getwd();
