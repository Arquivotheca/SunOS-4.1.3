#ifndef lint
static  char sccsid[] = "@(#)fv.cmd.c 1.1 92/07/30 Copyr 1988 Sun Micro";
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

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#ifdef SV1
# include <suntool/scrollbar.h>
# include <suntool/sunview.h>
# include <suntool/panel.h>
# include <suntool/canvas.h>
#else
# include <view2/scrollbar.h>
# include <view2/view2.h>
# include <view2/panel.h>
# include <view2/canvas.h>
#endif

#include "fv.port.h"
#include "fv.h"
#include "fv.extern.h"

/* Control panel items */
static Panel_item File_button;
static Panel_item View_button;
static Panel_item Edit_button;
static Panel_item Find_button;
static Panel_item Home_button;
static Panel_item Load_button;		/* Load into application; global */
static Panel_item Path_item;

static Menu Home_menu;
static Menu Cmd_menu;
static int Ncmds;			/* Number of your commands */

static Menu Filemenu;			/* Access the file system menu */
#define M_OPEN		1
#define M_PRINT		2
#define M_CREATE_FOLDER	3
#define M_CREATE_FILE	4
#define M_YOUR_COMMAND	5
#define OPEN		1		/* Open's pull right menu */
#define RUN_WITH_ARGS	2
#define EDIT		3
#define SHELL		4

static Menu Viewmenu;			/* Change the visual state menu */
#define M_TREE		1
#define M_EXPAND	2
#define M_EXPAND_ALL	3
#define M_BEGINTREE	4
#define M_ADDPARENT	5

static Menu Editmenu;			/* Modify object menu */
#define M_UNDELETE	1
#define M_COPY		3
#define M_PASTE		4
#define M_CUT		5
#define M_DELETE	6
#define M_PROPS		7

#ifdef SV1				/* Space between EDIT and HOME */
# ifdef PROTO
#  define HOME_COL	19
# else
#  define HOME_COL	21
# endif
#else
# define HOME_COL	15
#endif

static Menu Gotomenu;
static char Goto_field[80];

/* Prototype has Shine Mark and menu title counts as one */

#ifdef PROTO
#define OPEN_EDIT_FLOATER	1
#define OPEN_VIEW_FLOATER	1
#define PROPS_VIEW_FLOATER	7

#define REMOTE_COPY	"Remote Transfer \002"
#define BIND_ACTION	"Bind Action to Object \002"
#define FILE_PROPS	"File Properties \002"
#else
#define OPEN_EDIT_FLOATER	2
#define OPEN_VIEW_FLOATER	2
#define PROPS_VIEW_FLOATER	8

#define REMOTE_COPY	"Remote Transfer..."
#define BIND_ACTION	"Bind Action to Object..."
#define FILE_PROPS	"File Properties..."
#endif

static Menu Tree_floater;	/* Path pane floating menu */
static Menu Folder_floater;	/* Folder pane floating menu */

/* IF YOU CHANGE THE WORDS, YOU MUST CHANGE THE ARRAY SIZES! */

static char *Path_s[2] = {"Path", "Tree"};
static char Path_p[5];
static char *Hide_s[2] = {"Hide Subfolders", "Show Subfolders"};
static char Hide_p[16];

static void
file_button(item, event)		/* Process File button, display menu */
	Panel_item item;
	Event *event;
{
	Menu_item mi;			/* Grab menu item for manipulation */
	int seln;			/* Selected object in folder pane */
	BOOLEAN no_print;		/* No printing? */
	BOOLEAN no_seln;		/* No selection in tree or folder pane? */

#ifdef PROTO	/* XXX BUG IN LEIF LOOK */
	if (event_id(event) == MS_LEFT)
		return;
#endif
	

	/* The philosophy behind menus is that nothing can be selected
	 * unless it can genuinely apply.
	 */

	seln = fv_getselectedfile();
	no_seln = seln == EMPTY && !Fv_tselected;
	/* XXX What about shell scripts? */
	no_print = seln == EMPTY || Fv_file[seln]->type != FV_IDOCUMENT;

	if (event_id(event) == MS_RIGHT && event_is_down(event))
	{
		mi = menu_get(Filemenu, MENU_NTH_ITEM, M_OPEN);
		menu_set(mi, MENU_INACTIVE, no_seln, 0);
		mi = menu_get(Filemenu, MENU_NTH_ITEM, M_PRINT);
		menu_set(mi, MENU_INACTIVE, no_print, 0);
		mi = menu_get(Filemenu, MENU_NTH_ITEM, M_CREATE_FOLDER);
		menu_set(mi, MENU_INACTIVE, !Fv_write_access, 0);
		mi = menu_get(Filemenu, MENU_NTH_ITEM, M_CREATE_FILE);
		menu_set(mi, MENU_INACTIVE, !Fv_write_access, 0);
		mi = menu_get(Filemenu, MENU_NTH_ITEM, M_YOUR_COMMAND);
		menu_set(mi, MENU_INACTIVE, Ncmds==0, 0);

#ifdef SV1
# ifdef PROTO
	}
# else
		menu_show(Filemenu, Fv_panel, event, 0);
		return;
	}
	else if (event_id(event) == MS_LEFT && event_is_up(event))
		open_object();
# endif
#else
	}
#endif

	panel_default_handle_event(item, event);
}


static
open_object()			/* Open selected object in path/folder pane */
{
	if (Fv_tselected)
		fv_open_folder();
	else
	{
		fv_busy_cursor(TRUE);
		fv_file_open(fv_getselectedfile(), FALSE, NULL, FALSE);
		fv_busy_cursor(FALSE);
	}
}


static
run_object(m, mi)		/* Process Open pull right */
	Menu m;
	Menu_item mi;
{
	int i = (int)menu_get(mi, MENU_CLIENT_DATA);
	register FV_FILE **f_p, **l_p;	/* Files array pointers */
	int j;

	fv_busy_cursor(TRUE);
	for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles, j=0; f_p != l_p; f_p++, j++)
		if ((*f_p)->selected)
			fv_file_open(j, i == EDIT, 
				i == RUN_WITH_ARGS ? panel_get_value(Path_item) : NULL,
				i == SHELL);
	fv_busy_cursor(FALSE);
}


static
print_object()			/* Print selected objects in folder pane */
{
	char buf[MAXPATH-80];		/* File buffer */
	char print[MAXPATH];		/* Print script */

	fv_busy_cursor(TRUE);

	fv_collect_selected(buf, TRUE);

	/* ...and send them to the printer */

	(void)sprintf(print, Fv_print_script, buf);
	if (system(print))
		fv_showerr();
	else
	{
		buf[60] = NULL;
		fv_putmsg(FALSE, Fv_message[MPRINT], (int)buf, 0);
	}
	fv_busy_cursor(FALSE);
}


fv_collect_selected(buf, docs_only)
	char *buf;
	BOOLEAN docs_only;
{
	register int i = 0;		/* Index */
	register FV_FILE **f_p, **l_p;	/* Files array pointers */

	/* Collect all the selected files... */

	for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles; f_p != l_p; f_p++)
		if ((*f_p)->selected)
		{
			if (docs_only && (*f_p)->type != FV_IDOCUMENT)
				continue;
			if (i)
				(void)strcat(buf, (*f_p)->name);
			else
				(void)strcpy(buf, (*f_p)->name);
			(void)strcat(buf, " ");
			i++;
		}
}

static
create_folder()
{
	static char name[12] = "NewFolder";	/* Generic name */
	new_file(TRUE, name, 9);
}


static
create_file()
{
	static char name[12] = "NewFile";	/* Generic name */
	new_file(FALSE, name, 7);
}


static
new_file(folder, name, pos)			/* Create an empty file */
	BOOLEAN folder;
	char *name;
	register int pos;
{
	register FV_TNODE *f_p, *nf_p, *of_p;	/* Tree pointers */
	register int i;				/* Index */
	FV_TNODE *tree_p;			/* Who owns it? */
	int fd;


	/* Ensure we're back in current directory */

	if (Fv_tselected && (chdir(Fv_openfolder_path) == -1))
		fv_putmsg(TRUE, Fv_message[MECHDIR], (int)Fv_openfolder_path, 
			(int)sys_errlist[errno]);

	/* Make 10 attempts to make a unique file name */

	name[pos] = NULL;
	errno = EEXIST;
	for (i = 1; i <= 10 && errno == EEXIST; i++)
	{
		if (folder)
		{
			if (mkdir(name, 0777) == 0)
				break;
		}
		else
			if ((access(name, F_OK) == -1) &&
			    ((fd = open(name, O_WRONLY|O_CREAT, 00666)) != -1))
			{
				(void)close(fd);
				break;
			}
		name[pos] = i+'0';
		name[pos+1] = NULL;
	}

	if (i > 10)
	{
		/* No luck, bitch and return */
		fv_putmsg(TRUE, Fv_message[MECREAT],
			(int)name, (int)sys_errlist[errno]);
		return;
	}

	if (folder)
	{
	/* Build structure and insert it into tree, placing it in correct 
	 * alphabetic order..
	 */

	if ( ((f_p=(FV_TNODE *)fv_malloc(sizeof(FV_TNODE))) == NULL) ||
	     ((f_p->name = fv_malloc((unsigned)strlen(name)+1)) == NULL))
		return;

	f_p->child = NULL;
	f_p->parent = Fv_current;
	f_p->mtime = time((time_t)0);
	f_p->status = 0;
	(void)strcpy(f_p->name, name);

	for (of_p = NULL, nf_p = Fv_current->child; nf_p; 
		of_p = nf_p, nf_p = nf_p->sibling)
		if (strcmp(f_p->name, nf_p->name) < 0)
		{
			if (of_p)
				of_p->sibling = f_p;
			else
				Fv_current->child = f_p;
			f_p->sibling = nf_p;
			break;
		}

	/* Deselect folder in tree, select folder in folder pane, redraw... */

	if (Fv_tselected)
		fv_treedeselect();
	fv_drawtree(TRUE);
	}
	fv_display_folder(FV_BUILD_FOLDER);

	fv_match_files(name);
}


static void
view_button(item, event)		/* Process View button */
	Panel_item item;
	Event *event;
{
	if (event_id(event) == MS_RIGHT && event_is_down(event))
	{
		fv_viewmenu(Fv_panel, event, FALSE);
		return;
	}

#ifdef SV1
# ifndef PROTO
	else if (event_id(event) == MS_LEFT && event_is_up(event))
	{
		/* Toggle between tree and path */

		path_or_tree();
	}
# endif
#endif

	panel_default_handle_event(item, event);
}


fv_viewmenu(win, event, floater)		/* Display View menu */
	Window win;
	Event *event;
	BOOLEAN floater;
{
	Menu_item mi;
	Menu m = floater ? Tree_floater : Viewmenu;

	/* Again the same philosophy prevails; don't allow
	 * user to choose unavailable function.
	 */

	if (floater)
	{
#ifdef PROTO
		menu_set(m, MENU_FONT, window_get(Fv_frame, WIN_FONT), 0);
#endif
		mi = menu_get(m, MENU_NTH_ITEM, OPEN_VIEW_FLOATER);
		menu_set(mi, MENU_INACTIVE, !Fv_tselected, 0);
		mi = menu_get(m, MENU_NTH_ITEM, PROPS_VIEW_FLOATER);
		menu_set(mi, MENU_INACTIVE, !Fv_tselected, 0);
#ifndef SV1
		floater = 2;			/* View2 popup menus have title */
#endif
	}

	/* Expand/contract folder is context sensitive */

	if (Fv_tselected)
		(void)strcpy(Hide_p, Hide_s[Fv_tselected->mtime==0 ||
			Fv_tselected->status&PRUNE]);
	mi = menu_get(m, MENU_NTH_ITEM, M_EXPAND+floater);
	menu_set(mi, MENU_INACTIVE, 
		!Fv_tselected || !Fv_treeview ||
		(!Fv_tselected->child && Fv_tselected->mtime),
		0);

	mi = menu_get(m, MENU_NTH_ITEM, M_EXPAND_ALL+floater);
	menu_set(mi, MENU_INACTIVE, !Fv_tselected || !Fv_treeview, 0);
	mi = menu_get(m, MENU_NTH_ITEM, M_BEGINTREE+floater);
	menu_set(mi, MENU_INACTIVE, !Fv_tselected || !Fv_treeview, 0);
	mi = menu_get(m, MENU_NTH_ITEM, M_ADDPARENT+floater);
	menu_set(mi, MENU_INACTIVE, Fv_troot == Fv_thead, 0);

	/* The prototype and View2 have default actions on menus */
#ifdef SV1
# ifdef PROTO
	if (floater)
		menu_show(m, win, event, 0);
	else
		panel_default_handle_event(View_button, event);
# else
	menu_show(m, win, event, 0);
# endif
#else
	if (floater)
		menu_show(m, win, event, 0);
	else
		panel_default_handle_event(View_button, event);
#endif
}


static
path_or_tree()				/* Toggle between path and tree */
{
	/* Flip toggle every time we enter this function */

	(void)strcpy(Path_p, Path_s[Fv_treeview]);

	Fv_treeview = !Fv_treeview;

	Fv_dont_paint = TRUE;		/* Avoid repaint */

	if (!Fv_treeview)
	{
		fv_treedeselect();
		if (Fv_thead && Fv_thead != Fv_troot)
		{
			Fv_thead->sibling = Fv_sibling;
			Fv_thead = Fv_troot;
		}
	}

	if (Fv_treeview && Fv_tree.hsbar == 0) {
	       Fv_tree.hsbar = scrollbar_create(
#ifdef SV1
			SCROLL_PLACEMENT, SCROLL_SOUTH,
#endif
			SCROLL_LINE_HEIGHT, 10, 0);
		Fv_tree.vsbar = scrollbar_create(
#ifdef SV1
			SCROLL_PLACEMENT, SCROLL_EAST,
#endif
			SCROLL_LINE_HEIGHT, 10, 0);
	}


	/* Create or destroy scrollbars according to path or tree mode */

	window_set(Fv_tree.canvas,
		WIN_VERTICAL_SCROLLBAR, Fv_treeview ? Fv_tree.vsbar : NULL,
		WIN_HORIZONTAL_SCROLLBAR, Fv_treeview ? Fv_tree.hsbar : NULL,
		0);
	Fv_dont_paint = FALSE;

	/* Resize windows; this will redisplay entire tool */

	fv_resize(TRUE);
	fv_namestripe();
}


static void
edit_button(item, event)		/* Process edit button */
	Panel_item item;
	Event *event;
{
	if (event_id(event) == MS_RIGHT && event_is_down(event))
		fv_editmenu(Fv_panel, event, FALSE);
	else
		panel_default_handle_event(item, event);
}


fv_editmenu(window, event, floater)	/* Bring up edit menu */
	Window window;
	Event *event;
	BOOLEAN floater;
{
	Menu_item mi;
	Menu m = floater ? Folder_floater : Editmenu;
	BOOLEAN no_seln = fv_getselectedfile() == EMPTY;

	if (floater)
	{
#ifdef PROTO
		menu_set(m, MENU_FONT, window_get(Fv_frame, WIN_FONT), 0);
#endif
		mi = menu_get(m, MENU_NTH_ITEM,	OPEN_EDIT_FLOATER);
		menu_set(mi, MENU_INACTIVE, no_seln, 0);
#ifndef SV1
		floater = 2;			/* View2 popup menus have title */
#endif
	}
	else
		if (Fv_tselected)		/* Check trees */
			no_seln = FALSE;

	mi = menu_get(m, MENU_NTH_ITEM, M_UNDELETE+floater);
	menu_set(mi, MENU_INACTIVE, !Fv_undelete, 0);
	mi = menu_get(m, MENU_NTH_ITEM, M_CUT+floater);
	menu_set(mi, MENU_INACTIVE, no_seln, 0);
	mi = menu_get(m, MENU_NTH_ITEM, M_COPY+floater);
	menu_set(mi, MENU_INACTIVE, no_seln, 0);
	mi = menu_get(m, MENU_NTH_ITEM, M_PASTE+floater);
	menu_set(mi, MENU_INACTIVE, !Fv_write_access, 0);
	mi = menu_get(m, MENU_NTH_ITEM, M_DELETE+floater);
	menu_set(mi, MENU_INACTIVE, no_seln, 0);
	mi = menu_get(m, MENU_NTH_ITEM, M_PROPS+floater);
	menu_set(mi, MENU_INACTIVE, no_seln, 0);


	/* The prototype and View2 have default actions on menus */
#ifdef SV1
# ifdef PROTO
	if (floater)
		menu_show(m, window, event, 0);
	else
		panel_default_handle_event(Edit_button, event);
# else
	menu_show(m, window, event, 0);
# endif
#else
	if (floater)
		menu_show(m, window, event, 0);
	else
		panel_default_handle_event(Edit_button, event);
#endif
}


/*ARGSUSED*/
static
undelete(m, mi)				/* Display Undelete menu's pull right */
	Menu m;
	Menu_item mi;
{
	int i = (int)menu_get(mi, MENU_CLIENT_DATA);

	if (i==1)
		fv_undelete();
	else
		fv_commit_delete(TRUE);
}


fv_delete_object()			/* Delete selected object */
{
	register FV_TNODE *f_p;		/* Tree pointer */
	char *b_p;			/* String pointer */
	char buf[256];			/* 'Invisible' file */


	/* Confirm deletion of selected object(s) */
	if (Fv_confirm_delete && !fv_alert(Fv_message[MCONFIRMDEL]))
			return;

	if (Fv_tselected)
	{
		if (access(".", W_OK) == -1)
		{
			fv_putmsg(TRUE, Fv_message[MEDELETE],
				(int)Fv_tselected->name, sys_errlist[errno]);
			return;
		}

		if (!Fv_treeview)
		{
			sprintf(buf, Fv_message[MDELCURRENT],
				Fv_tselected->name, Fv_tselected->parent->name);
			if (fv_alert(buf) == FALSE)
				return;
		}

		/* Get out of current directory */

		(void)chdir("..");

		/* Move folder to 'invisible' name */

		b_p = buf;
		*b_p++ = '.';
		*b_p++ = '.';
		(void)strcpy(b_p, Fv_tselected->name);
		if (rename(Fv_tselected->name, buf) == -1)
		{
			fv_putmsg(TRUE, sys_errlist[errno], 0, 0);

			/* Change back */
			(void)chdir(Fv_tselected->name);
		}
		else
		{
			/* Cut node from tree... */

			if (Fv_tselected == Fv_tselected->parent->child)
				Fv_tselected->parent->child 
					= Fv_tselected->sibling;
			else
			{
				/* Fix sibling pointers... */

				for (f_p = Fv_tselected->parent->child; 
					f_p && f_p->sibling != Fv_tselected;
					f_p = f_p->sibling)
					;
				if (f_p)
					f_p->sibling = Fv_tselected->sibling;
			}

			fv_getpath(Fv_tselected->parent, 0);

			/* Prepare for undelete... */

			(void)fv_commit_delete();
			if ((Fv_undelete = fv_malloc((unsigned)strlen(Fv_path)+
						strlen(Fv_tselected->name)+5)))
				(void)sprintf(Fv_undelete, "%s ..%s ", Fv_path,
					Fv_tselected->name);

			if (fv_descendant(Fv_current, Fv_tselected))
			{
				/* If current folder gets zapped, then
				 * back up one level.
				 */
				(void)strcpy(Fv_openfolder_path, Fv_path);
				Fv_lastcurrent = NULL;
				Fv_current = Fv_tselected->parent;
				fv_display_folder(FV_BUILD_FOLDER);
			}

			Fv_tselected->sibling = NULL;
			fv_destroy_tree(Fv_tselected);
			fv_treedeselect();

			fv_drawtree(TRUE);
		}
	}
	else
		fv_delete_selected();
}


static
exec_command(m, mi)
	Menu m;
	Menu_item mi;
{
	register int i = 0;			/* Arg count */
	char buf[4096];				/* Command (can expand) */
	char *argv[255];
	char *s = (char *)menu_get(mi, MENU_STRING);
	register char *b_p;

	/* Is there a '%s' somewhere in the string */
	b_p = s;
	while (*b_p)
		if (*b_p == '%' && *(b_p+1) == 's')
			break;
		else
			b_p++;

	/* Yes there is, copy the selected names in */
	if (b_p)
		sprintf(buf, s, (char *)fv_getmyselns(FALSE));
	else
		strcpy(buf, s);

	/* Now break up command into argument line */
	/* XXX Embedded spaces */
	s = (char *)strtok(buf, " ");
	while (s) {
		argv[i++] = s;
		s = (char *)strtok((char *)0, " ");
	}
	argv[i] = 0;
	fv_execit(argv[0], argv);
}


static
build_cmd_menu()
{
	char buf[MAXPATH];
	register int i, j, l;
	char *av[256];
	register FILE *fp;

	av[0] = (char *)MENU_NOTIFY_PROC;
	av[1] = (char *)exec_command;
	j = 2;
	i = 1;

	sprintf(buf, "%s/.fvcmd", Fv_home);
	if (fp = fopen(buf, "r"))
	{
		while (fgets(buf, 255, fp) && j < 255)
		{
			av[j] = (char *)MENU_STRING_ITEM;
			av[j+1] = malloc(l = strlen(buf));
			l--;
			if (buf[l] == '\n')
				buf[l] = 0;		/* No newline */
			strcpy(av[j+1], buf);
			av[j+2] = (char *)i;
			j += 3;
			i++;
		}
	}
	av[j] = 0;
	Cmd_menu = menu_create(ATTR_LIST, &av[0], 0);
	Ncmds = i-1;
}


static
home_button(item, event)	/* Generate most recent visited folders menu */
	Panel_item item;
	Event *event;
{
	Menu_item mi;
	char *av[(FV_MAXHISTORY+1)*3];
	register int i, j;
	static int nitems = 1;

	if (event_id(event) == MS_RIGHT && event_is_down(event))
	{
		/* Generate most recent visited folders menu */

		for (j=i=0; i<Fv_nhistory; i++, j+=3)
		{
			av[j] = (char *)MENU_STRING_ITEM;
			av[j+1] = Fv_history[i];
			av[j+2] = (char *)i+1;
		}
		av[j] = 0;
		av[j+1] = 0;
		if (Home_menu)
			menu_destroy(Home_menu);
#ifdef SV1
		Home_menu = menu_create(ATTR_LIST, &av[0], 0);
#else
		Home_menu = vu_create(VU_NULL, MENU, ATTR_LIST, &av[0], 0);
#endif
#ifdef PROTO
		menu_set(Home_menu, 
			MENU_FONT, window_get(Fv_frame, WIN_FONT),
			0);
#endif
		if (i = (int)menu_show(Home_menu, Fv_panel, event, 0))
		{
			fv_busy_cursor(TRUE);
			fv_openfile(Fv_history[i-1], (char *)0, TRUE);
			fv_busy_cursor(FALSE);
		}
	}
	else if (event_id(event) == MS_LEFT && event_is_up(event))
		fv_openfile(Fv_home, (char *)0, TRUE);
}


static
goto_object()				/* Either match or goto */
{
	register char *str_p;		/* User's input */
	struct stat fstatus;		/* Check for folder */
	char resolved_name[MAXPATH];
	char tilde_name[MAXPATH];


	if ((str_p=panel_get_value(Path_item)) && *str_p)
	{
		/* Could this be a regular expression?  If so, match
		 * and select those files...
		 */

		if (fv_regex_in_string(str_p))
			fv_match_files(str_p);
		else
		{
			fv_busy_cursor(TRUE);

			if (*str_p == '~') {
				/* Expand home folder pointer */
				strcpy(tilde_name, Fv_home);
				strcat(tilde_name, str_p+1);
				str_p = tilde_name;
			}
#ifdef OS3
			strcpy(resolved_name, str_p);
#else
			/* Resolve references to "." and ".." */

			if (realpath(str_p, resolved_name) == 0)
				fv_putmsg(TRUE, sys_errlist[errno], 0, 0);
			else 
#endif
				if (stat(resolved_name, &fstatus))
				fv_putmsg(TRUE, sys_errlist[errno], 0, 0);
			else
				fv_openfile(resolved_name, (char *)0,
					(fstatus.st_mode & S_IFMT)==S_IFDIR);

			fv_busy_cursor(FALSE);
		}
	}
}

static void
goto_button(item, event)		/* Fill in field menu item */
	Panel_item item;
	Event *event;
{
	Menu_item mi;

#ifdef PROTO	/* XXX BUG IN LEIF LOOK */
	if (event_id(event) == MS_LEFT)
		return;
#endif
	
	if (event_id(event) == MS_RIGHT && event_is_down(event))
	{
		(void)strncpy(Goto_field, panel_get(Path_item, PANEL_VALUE), 20);
		mi = menu_get(Gotomenu, MENU_NTH_ITEM, 1);
		menu_set(mi, MENU_INACTIVE, Goto_field[0]==0, 0);
		if (Goto_field[0] == 0)
			strcpy(Goto_field, "Entry");
		menu_show(Gotomenu, Fv_panel, event, 0);
	}
	else if (event_id(event) == MS_LEFT && event_is_up(event))
		goto_object();
}


fv_create_panel(p) 				/* Create tool's control panel */
	register Panel p;			/* Panel */
{
	Menu open_menu, undel_menu, m;		/* Generic menu */
	extern void fv_addparent_button();	/* Add parent button */
	extern void fv_hide_node();		/* Hide notify proc */
	extern void fv_set_root();		/* Change root proc */
	extern int fv_info_object();
	extern int fv_find_object();
	extern int fv_bind_object();
	extern int fv_select_all();
	extern int fv_load_button();
	extern int fv_copy_to_shelf();
	extern int fv_paste();
	extern int fv_undelete();
	extern int fv_cut_to_shelf();
	extern int fv_expand_all_nodes();
	extern void fv_remote_transfer();


	open_menu = menu_create( 
		MENU_NOTIFY_PROC, run_object,
		MENU_ITEM,
			MENU_CLIENT_DATA, OPEN,
			MENU_STRING, "Open File", 0,
		MENU_ITEM,
			MENU_CLIENT_DATA, RUN_WITH_ARGS,
			MENU_STRING, "Open with 'Name' Arguments", 0,
		MENU_ITEM,
			MENU_CLIENT_DATA, EDIT,
			MENU_STRING, "Open in Document Editor", 0,
		MENU_ITEM,
			MENU_CLIENT_DATA, SHELL,
			MENU_STRING, "Open in Shelltool", 0,
		0);

	build_cmd_menu();

	Filemenu = menu_create(
		MENU_PULLRIGHT_ITEM, "Open", open_menu,
		MENU_ACTION_ITEM, "Print File", print_object,
		MENU_ACTION_ITEM, "Create Folder", create_folder,
		MENU_ACTION_ITEM, "Create File", create_file,
		MENU_PULLRIGHT_ITEM, "Your Commands", Cmd_menu,
#ifdef SV1
		MENU_ACTION_ITEM, REMOTE_COPY, fv_remote_transfer,
		MENU_ACTION_ITEM, BIND_ACTION, fv_bind_object,
#else
		MENU_ITEM, 
		  MENU_STRING, "Remote Transfer",
		  MENU_ACTION_PROC, fv_remote_transfer,
		  MENU_WINDOW_MARK, TRUE, 0,
		MENU_ITEM, 
		  MENU_STRING, "Bind Action to Object",
		  MENU_ACTION_PROC, fv_bind_object,
		  MENU_WINDOW_MARK, TRUE, 0,
#endif
		0);

	File_button = panel_create_item(p, MY_MENU(Filemenu),
			MY_BUTTON_IMAGE(p, "File"),
			PANEL_EVENT_PROC, file_button,
			0);

	(void)strcpy(Path_p, Path_s[1]);
	(void)strcpy(Hide_p, *Hide_s);

	Viewmenu = menu_create(
		MENU_ACTION_ITEM, Path_p, path_or_tree,
		MENU_ACTION_ITEM, Hide_p, fv_hide_node,
		MENU_ACTION_ITEM, "Show All Subfolders", fv_expand_all_nodes,
		MENU_ACTION_ITEM, "Begin Tree Here", fv_set_root,
		MENU_ACTION_ITEM, "Add Tree's Parent", fv_addparent_button,
		0);

	View_button = panel_create_item(p, MY_MENU(Viewmenu),
			MY_BUTTON_IMAGE(p, "View"),
			PANEL_EVENT_PROC, view_button,
			0);

	undel_menu = menu_create( 
		MENU_NOTIFY_PROC, undelete,
		MENU_ITEM,
			MENU_CLIENT_DATA, 1,
			MENU_STRING, "Recover Files", 0,
		MENU_ITEM,
			MENU_CLIENT_DATA, 2,
			MENU_STRING, "Really Delete", 0,
		0);

	Editmenu = menu_create(
		MENU_PULLRIGHT_ITEM, "Undelete", undel_menu,
		MENU_ACTION_ITEM, "Select All", fv_select_all,
		MENU_ACTION_ITEM, "Copy", fv_copy_to_shelf,
		MENU_ACTION_ITEM, "Paste", fv_paste,
		MENU_ACTION_ITEM, "Cut", fv_cut_to_shelf,
		MENU_ACTION_ITEM, "Delete", fv_delete_object,
#ifdef SV1
		MENU_ACTION_ITEM, FILE_PROPS, fv_info_object,
#else
		MENU_ITEM, 
		  MENU_STRING, "File Properties",
		  MENU_ACTION_PROC, fv_info_object,
		  MENU_WINDOW_MARK, TRUE, 0,
#endif
		0);

	Edit_button = panel_create_item(p, MY_MENU(Editmenu),
			MY_BUTTON_IMAGE(p, "Edit"),
			PANEL_EVENT_PROC, edit_button,
			0);

	Tree_floater = menu_create(
#ifndef SV1
		MENU_TITLE_ITEM, "Path Commands",
#endif
		MENU_ACTION_ITEM, "Open", open_object,
		MENU_ACTION_ITEM, Path_p, path_or_tree,
		MENU_ACTION_ITEM, Hide_p, fv_hide_node,
		MENU_ACTION_ITEM, "Show All Subfolders", fv_expand_all_nodes,
		MENU_ACTION_ITEM, "Begin Tree Here", fv_set_root,
		MENU_ACTION_ITEM, "Add Tree's Parent", fv_addparent_button,
#ifdef SV1
		MENU_ACTION_ITEM, FILE_PROPS, fv_info_object,
#else
		MENU_ITEM, 
		  MENU_STRING, "File Properties",
		  MENU_ACTION_PROC, fv_info_object,
		  MENU_WINDOW_MARK, TRUE, 0,
#endif
		0);

	Folder_floater = menu_create(
#ifndef SV1
		MENU_TITLE_ITEM, "Folder Commands",
#endif
		MENU_PULLRIGHT_ITEM, "Open", open_menu,
		MENU_PULLRIGHT_ITEM, "Undelete", undel_menu,
		MENU_ACTION_ITEM, "Select All", fv_select_all,
		MENU_ACTION_ITEM, "Copy", fv_copy_to_shelf,
		MENU_ACTION_ITEM, "Paste", fv_paste,
		MENU_ACTION_ITEM, "Cut", fv_cut_to_shelf,
		MENU_ACTION_ITEM, "Delete", fv_delete_object,
#ifdef SV1
		MENU_ACTION_ITEM, FILE_PROPS, fv_info_object,
#else
		MENU_ITEM, 
		  MENU_STRING, "File Properties",
		  MENU_ACTION_PROC, fv_info_object,
		  MENU_WINDOW_MARK, TRUE, 0,
#endif
		0);
		
	/* We'll supply real menu in event function;
	 * There should really be a gap between file, view, edit and
	 * the other buttons
	 */

	Home_menu = menu_create(MENU_STRING_ITEM, Fv_home, 1, 0);
	Home_button = panel_create_item(p, MY_MENU(Home_menu),
		MY_BUTTON_IMAGE(p, "Home"),
		PANEL_EVENT_PROC, home_button,
		PANEL_ITEM_X, ATTR_ROW(HOME_COL),
		0);

	Gotomenu = menu_create(
		MENU_ACTION_ITEM, Goto_field, goto_object,
		MENU_ACTION_ITEM, 
#ifdef OPENLOOK
			"Find \002",
#else
			"Find...",
#endif
			fv_find_object,
		0);
	Find_button = panel_create_item(p, MY_MENU(Gotomenu),
		MY_BUTTON_IMAGE(p, "Goto:"),
		PANEL_EVENT_PROC, goto_button,
		0);
	Path_item = panel_create_item(p, PANEL_TEXT,
		PANEL_LABEL_Y, ATTR_ROW(0),
		PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE_DISPLAY_LENGTH, 32,
		PANEL_VALUE_STORED_LENGTH, 256,
		PANEL_NOTIFY_PROC, goto_object,
		0);
	
	m = menu_create(
		MENU_ACTION_ITEM, "Load", fv_load_button,
		MENU_ACTION_ITEM, "Cancel", fv_load_button,	/* XXX */
		0);
	Load_button = panel_create_item(p, MY_MENU(m),
		MY_BUTTON_IMAGE(p, "Load"),
		PANEL_NOTIFY_PROC, fv_load_button,
		PANEL_SHOW_ITEM, FALSE,
		0);

#ifndef OPENLOOK
	Fv_errmsg = panel_create_item(p, PANEL_MESSAGE,
		PANEL_LABEL_X, ATTR_COL(0),
		PANEL_LABEL_Y, ATTR_ROW(1),
		PANEL_VALUE_DISPLAY_LENGTH, 40,
		0);
	window_set(p, WIN_ROWS, 2, 0);
	window_fit_width(p);
#else
	window_fit(p);
#endif
}

void
fv_show_load_button(on)
	BOOLEAN on;
{
	/* XXX Insert this button before the HOME menu */

	panel_set(Load_button, PANEL_SHOW_ITEM, on, 0);
}
