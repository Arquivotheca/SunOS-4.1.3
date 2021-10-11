#ifndef lint
static  char sccsid[] = "@(#)fv.file.c 1.1 92/07/30 Copyr 1988 Sun Micro";
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
#include <sys/wait.h>
#include <string.h>
#ifdef OS3
# include <sys/dir.h>
#else
# include <dirent.h>
#endif

#ifdef SV1
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/tty.h>
#include <suntool/textsw.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>
#include <suntool/alert.h>
#include <suntool/fullscreen.h>
#else
#include <view2/view2.h>
#include <view2/panel.h>
#include <view2/tty.h>
#include <view2/textsw.h>
#include <view2/canvas.h>
#include <view2/scrollbar.h>
#include <view2/alert.h>
#include <view2/fullscreen.h>
#include <view2/seln.h>
#endif

#include "fv.port.h"
#include "fv.h"
#include "fv.extern.h"

/* The following three arrays contain the 8 or so RGB values for colors */
#include "fv.colormap.h"
u_char Fv_red[CMS_FVCOLORMAPSIZE];
u_char Fv_blue[CMS_FVCOLORMAPSIZE];
u_char Fv_green[CMS_FVCOLORMAPSIZE];

#define COLOR_FILE	"..color"	/* Contains sorted pairs of color/name*/
static short Ncolors;			/* # color entries in COLOR_FILE*/
static char *Color_fname[MAXFILES];	/* Name... */
static SBYTE Color_code[MAXFILES];	/* ...and color pairs */

static int Maxfilesperline;		/* Maximum files per line */
static int Incx;			/* Columnar Increment */
static int Startx;			/* Starting location */
static short Widest_name;		/* Widest file name */

/* Variable and fixed widths for List Options */
static short Permission_width[2] = {8, 11};	
static short Link_width[2] = {3, 5};
static short Owner_width[2] = {5, 9};
static short Group_width[2] = {5, 9};
static short Size_width[2] = {5, 9};
static short Date_width[2] = {9, 14};

static char Last_edited[MAXPATH];	/* Last edited filename */

#define ERASE		0177		/* Delete key on Sun-3 keyboards */
#define MAXRENAMELEN	80		/* Max length of rename string */
#define THRESHOLD	4		/* Move/copy mode (in pixels) */
#define LIST_HEIGHT	20		/* Height of lines in List mode */


static int compare_name(), compare_time(), compare_size(), compare_type(),
	compare_color();
static ROUTINE Compare_fn[FV_SORTCOLOR+1] = {
	compare_name, compare_time, compare_size, compare_type, compare_color,
};
extern time_t time();

static short If_image[] = {
#include "folderwin.icon"
};
mpr_static_static(f_icon_pr, 64, 48, 1, If_image);
static short Id_image[] = {
#include "docwin.icon"
};
mpr_static_static(d_icon_pr, 64, 64, 1, Id_image);

#ifdef SV1
static struct icon F_icon;
static struct icon D_icon;
#endif

#define MAX_TEXTSW	32
#ifdef SV1
static int Edit_id[MAX_TEXTSW];
static Textsw Edit_sw[MAX_TEXTSW];
#endif
static Frame Edit_fr[MAX_TEXTSW];
static int Edit_no;
#define MAX_FOLDERS	16
static Frame Folder_frame[MAX_FOLDERS];		/* Folder frames */
static int Nfolders;

static int Max_alloc;			/* Number of allocated FV_FILE */

#define SIX_MONTHS_IN_SECS	15552000
#define HOUR_IN_SECS		3600
#define ICON_AREA_HEIGHT	66	/* Icon height + text height + gap */

static Panel Text_panel;
static Panel_item Text_item;

static int Scroll_increment;
static int Renamed_file;		/* Index in Fv_file */
static void stop_editing();

#ifdef SV1
# define GET_SCROLL_VIEW_START (int)scrollbar_get(Fv_foldr.vsbar, SCROLL_VIEW_START)
#else
# define GET_SCROLL_VIEW_START ((int)scrollbar_get(Fv_foldr.vsbar, SCROLL_VIEW_START) * Scroll_increment)
#endif

void
fv_create_folder()			/* Create folder canvas */
{
	register int i;			/* Index */
	Notify_value folder_event();
	static int display_folder();
	struct singlecolor *color;


	Fv_foldr.canvas = window_create(Fv_frame, CANVAS,
		CANVAS_FAST_MONO,	TRUE,
		CANVAS_AUTO_SHRINK,	FALSE,
		CANVAS_AUTO_CLEAR,	FALSE,
		CANVAS_RETAINED,	FALSE,
		CANVAS_REPAINT_PROC,	display_folder,
		CANVAS_FIXED_IMAGE,	FALSE,
		WIN_BELOW,		Fv_tree.canvas,
		WIN_X,			0,
		WIN_Y,			0,
		WIN_ROWS,		10,
		WIN_VERTICAL_SCROLLBAR, 
			Fv_foldr.vsbar = scrollbar_create(
#ifdef SV1
				SCROLL_PLACEMENT, SCROLL_EAST, 0
#endif
				),
		0);


#ifdef SV1
	Fv_foldr.pw = (PAINTWIN)canvas_pixwin(Fv_foldr.canvas);
	Fv_color = Fv_foldr.pw->pw_pixrect->pr_depth > 1;
#else
	Fv_foldr.pw = (PAINTWIN)vu_get(Fv_foldr.canvas, CANVAS_NTH_PAINT_WINDOW, 0);
	Fv_color = (int)vu_get(Fv_frame, WIN_DEPTH) > 1;
#endif
	if (Fv_color)
	{
		/* Apply color first; then decide whether this is a color monitor.
		 * Otherwise we have problems with single color monitor systems that
		 * have the monochrome and color framebuffer on the same device.
		 * Later:
		 * Shit! We still screwed. If you supply color on a monochrome
		 * device we try and use it.  (It appears black.)
		 */
		cms_fvcolormapsetup(Fv_red, Fv_green, Fv_blue);

		if (window_get(Fv_frame, FRAME_INHERIT_COLORS))
		{
			/* Grab the current background and foreground color and
			 * replace our generic colors (white and black).  
			 * Reverse video display and color inheritance cause 
			 * problems.
			 */
			color = (struct singlecolor *)window_get(Fv_frame, 
				FRAME_FOREGROUND_COLOR);
			Fv_red[CMS_FVCOLORMAPSIZE-1] = Fv_red[1] = color->red;
			Fv_green[CMS_FVCOLORMAPSIZE-1] = Fv_green[1] = color->green;
			Fv_blue[CMS_FVCOLORMAPSIZE-1] = Fv_blue[1] = color->blue;
			color = (struct singlecolor *)window_get(Fv_frame, 
				FRAME_BACKGROUND_COLOR);
			Fv_red[0] = color->red;
			Fv_green[0] = color->green;
			Fv_blue[0] = color->blue;
		}
		pw_setcmsname(Fv_foldr.pw, "FVCOLORMAP");
		pw_putcolormap(Fv_foldr.pw, 0, CMS_FVCOLORMAPSIZE, 
			Fv_red, Fv_green, Fv_blue);
	}

#ifdef SV1
	notify_interpose_event_func(Fv_foldr.canvas, folder_event, NOTIFY_SAFE);
#else
	notify_interpose_event_func(Fv_foldr.pw, folder_event, NOTIFY_SAFE);
#endif
	/* Look for events */
	window_set(
#ifdef SV1
		Fv_foldr.canvas,
#else
		Fv_foldr.pw,
#endif
		WIN_CONSUME_PICK_EVENTS,
			LOC_WINENTER, LOC_WINEXIT,
			ERASE, SHIFT_CTRL, LOC_DRAG, KEY_LEFT(K_PROPS),
			KEY_LEFT(K_UNDO), KEY_LEFT(K_PUT), KEY_LEFT(K_GET), 
			KEY_LEFT(K_FIND), KEY_LEFT(K_DELETE), 0,
			0);


	/* Create a text panel for editing on the canvas */

	Text_panel = window_create(Fv_frame, PANEL, 
				WIN_SHOW, FALSE,
				WIN_HEIGHT, 20,
				0);
	Text_item = panel_create_item(Text_panel, PANEL_TEXT, 
			PANEL_VALUE_STORED_LENGTH, 255,
			PANEL_VALUE_X, 0, PANEL_VALUE_Y, 0, 
			PANEL_NOTIFY_PROC, stop_editing,
			0);


	/* Allocate enough memory for file structures.
	 * Allocating memory dynamically instead of statically allows
	 * us to swap pointers instead of copying structures whilst
	 * sorting, etc.
	 */ 

	Max_alloc = 128;
	for (i=0; i<Max_alloc; i++)
	{
		Fv_file[i] = (FV_FILE *)fv_malloc(sizeof(FV_FILE));
		if (Fv_file[i] == NULL)
			break;
	}
}


fv_file_scroll_height()			/* Set scroll increment */
{
	Scroll_increment = Fv_style & FV_DICON ? ICON_AREA_HEIGHT : LIST_HEIGHT;
	scrollbar_set(Fv_foldr.vsbar, SCROLL_LINE_HEIGHT, Scroll_increment, 0);
}


static Notify_value
folder_event(cnvs, evnt, arg, typ)	/* Process canvas events */
	Canvas cnvs;
	register Event *evnt;
	Notify_arg arg;
	Notify_event_type typ;
{
	static struct timeval last_click;	/* Last click time */
	struct timeval click;			/* Click time */
	static int last_selected=EMPTY;		/* Last file selected */
	int x, y;				/* Name coordinates */
	register int i;				/* Index */
	char *b_p;				/* String pointer */
#ifndef SV1
	char buf[MAXPATH];			/* Move request buffer */
	Fullscreen fs;				/* Drag detect whilst but down */
#endif


#ifndef SV1
	if (event_action(evnt) == ACTION_DRAG_LOAD)
	{
		/* Somebody is trying drop a file over me.  Move the file
		 * indicated by the primary selection into this folder.
		 */
		Seln_holder holder;
		Seln_request *sbuf;

		holder = seln_inquire(SELN_PRIMARY);
		if (holder.state == SELN_NONE)
			fv_putmsg(TRUE, "No selection!", 0, 0);
		else {
			fv_busy_cursor(TRUE);
			sbuf = seln_ask(&holder, SELN_REQ_CONTENTS_ASCII, 0, 0);

			/* This maybe a mail message... */
			if (strncmp(sbuf->data+sizeof(Seln_attribute), 
				"/tmp/MailItemsFor", 17) == 0 &&
			   (i = over_object(event_x(evnt), event_y(evnt))) != EMPTY)
			{
				if (!Fv_file[i]->selected) {
					fv_reverse(i);
					Fv_file[i]->selected = TRUE;
				}
				sprintf(buf, "/bin/cat %s %s > .X 2>/tmp/.err; /bin/mv .X %s 2>/tmp/.err",
					Fv_file[i]->name, 
					sbuf->data+sizeof(Seln_attribute),
					Fv_file[i]->name);
				if (system(buf))
					fv_showerr();
				else
					Fv_current->mtime = time(0);
				fv_putmsg(FALSE, "Mail message appended to folder %s",
					Fv_file[i]->name, 0);
			}
			else
			{
				/* Move the file in */
				sprintf(buf, "mv %s . 2>/tmp/.err",
					sbuf->data+sizeof(Seln_attribute));
				if (system(buf))
					fv_showerr();
				else
					fv_display_folder(FV_BUILD_FOLDER);
			}
			fv_busy_cursor(FALSE);
		}
	}
#endif

	if (event_is_down(evnt))
	if (event_id(evnt) == MS_LEFT)
	{
		/* Clear old message */

		fv_clrmsg();

		/* Ensure that we're still in this folder, (we could
		 * have changed to another by merely selecting another
		 * folder in the tree...) 
		 */
		 
		if (Fv_treeview && Fv_tselected != Fv_current)
		{
			/* We're in deep shit if we can't open current folder */
			if (chdir(Fv_openfolder_path)==-1)
				fv_putmsg(TRUE, Fv_message[MECHDIR], 
					Fv_openfolder_path, sys_errlist[errno]);
			(void)strcpy(Fv_path, Fv_openfolder_path);
		}

		i=over_object_name(event_x(evnt), event_y(evnt));
		if (i != EMPTY && Fv_file[i]->selected && Fv_write_access)
		{
			/* The user wishes to rename this file.  Place a panel
			 * text field over the existing filename.
			 */
			Renamed_file = i;
			y = trans(Renamed_file)-GET_SCROLL_VIEW_START;
			x = text_start(Renamed_file);
			if (Fv_style & FV_DICON)
				y += GLYPH_HEIGHT+Fv_fontsize.y-10;
			else
				y += LIST_HEIGHT-10;
			b_p = Fv_file[Renamed_file]->name;

			/* Get real length of text and approx. # characters */
			panel_set(Text_item, PANEL_VALUE, b_p,
				PANEL_VALUE_DISPLAY_LENGTH, strlen(b_p),
				0);
			window_set(Text_panel,
				WIN_X, x + window_get(Fv_foldr.canvas, WIN_X), 
				WIN_Y, y + window_get(Fv_foldr.canvas, WIN_Y), 
				WIN_WIDTH, Fv_file[Renamed_file]->width+
						Fv_fontsize.x,
				WIN_SHOW, TRUE, 0);
			fv_putmsg(FALSE, Fv_message[MRENAME], (int)b_p, 0);
			return(notify_next_event_func(cnvs, evnt, arg, typ));
		}
		else if (window_get(Text_panel, WIN_SHOW))
		{
			/* Clicking elsewhere completes the rename operation */
			stop_editing();
		}

		if (Fv_tselected)
		{
			/* Turn off tree selection.  Only 1 window can have 
			 * the selection.
			 */

			fv_treedeselect();
		}

		if ((i = over_object(event_x(evnt), event_y(evnt))) != EMPTY)
		{
			if (!Fv_file[i]->selected)
			{
				/* If this file isn't already selected then 
				 * turn off other selections.  If it is
				 * selected, the user may wish to drag.
				 */
				fv_filedeselect();
				Fv_file[i]->selected = TRUE;
				fv_reverse(i);
			}


			click = evnt->ie_time;
			if (last_selected == i && 
			    click.tv_sec-last_click.tv_sec <= 1)
			{
				/* Double click (inside a second) opens file */

				fv_busy_cursor(TRUE);
				fv_file_open(i, FALSE, (char *)0, FALSE);
				last_selected = EMPTY;
				fv_busy_cursor(FALSE);
				return(notify_next_event_func(cnvs, 
					evnt, arg, typ));
			}
			last_click = click;
			last_selected = i;

			/* Maybe user wants to drag... */

			x = event_x(evnt);
			y = event_y(evnt);
#ifndef SV1
			fs = (Fullscreen)vu_create(0, FULLSCREEN,
					FULLSCREEN_INPUT_WINDOW, cnvs,
					FULLSCREEN_SYNC, FALSE,
					WIN_CONSUME_EVENTS, WIN_MOUSE_BUTTONS,
						LOC_DRAG, LOC_MOVEWHILEBUTDOWN, 0,
					0);
#endif
			while (window_read_event(cnvs, evnt) != -1 && 
				event_id(evnt) != MS_LEFT)
			{
				 if (((i = x-event_x(evnt))>THRESHOLD) || i<-THRESHOLD ||
				    ((i = y-event_y(evnt))>THRESHOLD) || i<-THRESHOLD)
				 {
#ifndef SV1
					fullscreen_destroy(fs);
					fs = 0;
#endif
					drag(x, y, (BOOLEAN)event_ctrl_is_down(evnt));
					break;
				 }
			}
#ifndef SV1
			if (fs)
				fullscreen_destroy(fs);
#endif
		}
		else
			fv_filedeselect();
	}
	else if (event_id(evnt) == MS_MIDDLE)	/* Extend selection */
	{
		if ((i = over_object(event_x(evnt), event_y(evnt))) > EMPTY)
		{
			/* Ensure no tree object remains selected */
			if (Fv_tselected)
				fv_treedeselect();

			fv_reverse(i);
			Fv_file[i]->selected = !Fv_file[i]->selected;
		}
	}
	else if (event_id(evnt) == MS_RIGHT)
	{
		/* Put up floating edit menu */

		fv_editmenu(cnvs, evnt, TRUE);
	}

	fv_check_keys(evnt);


	return(notify_next_event_func(cnvs, evnt, arg, typ));
}


static
drag(x, y, copy)			/* Drag icon */
	register int x, y;		/* From coordinates */
	BOOLEAN copy;			/* Copy instead of Move */
{
	int iconwidth, iconheight;
	Event ev;			/* Current event */
	register int i;			/* Generic index */
	register int x1, y1;		/* New coordinates (relative from frame)*/
	register BOOLEAN really_drag;	/* Drag icons? */
	register FV_FILE **f_p, **l_p;	/* Files array pointers */
	struct {
		short fno;		/* Index to Fv_file array */
		short x, y;		/* Original coords on screen */
		Pixrect *pr;		/* Object's iconic image */
	} Seln[64];
	register int nseln;		/* Number of selections */
	int scroll_offset;		/* Scroll offset into canvas */
	int target;
	int lock;
	char target_tree_path[MAXPATH];
	FV_TNODE *tree_target;
	FV_TNODE *tree_lock;
	FV_TNODE *t_p;
	register struct fullscreen *fs;
	static int fd=0;
	int rootfd;
	extern FV_TNODE *fv_infolder();
	int winx, winy;
	BOOLEAN desktop_link;
	int x_off=0, y_off=0;
	int foldr_top, tree_top;
#ifndef SV1
	static Frame root_frame = 0;
	Cursor fv_get_drag_cursor();
#endif
	


#ifdef SV1
	/* Enter fullscreen mode and grab all events */

	if (!fd)
		fd = (int)window_get(Fv_frame, WIN_FD);
	fs = (struct fullscreen *)fullscreen_init(fd);
	win_grabio(fd);
	fv_cursor(fd, copy ? FV_COPYCURSOR : FV_MOVECURSOR);
#ifdef PROTO
	i = PROTO_TOP_MARGIN;
#else
	i = (int)window_get(Fv_frame, WIN_TOP_MARGIN);
#endif
	tree_top = (int)window_get(Fv_tree.canvas, WIN_Y)+i;
	foldr_top = (int)window_get(Fv_foldr.canvas, WIN_Y)+i;
#define PAINT		fullscreen_pw_write(fs->fs_pixwin,
#else
#define PAINT		pw_rop(root_frame,

	if (!root_frame)
		root_frame = vu_get(Fv_frame, VU_ROOT);
	win_translate_xy(Fv_foldr.canvas, root_frame, 0, 0, &x_off, &y_off);
	y_off += 7;	/* KLUDGE */

	fs = (struct fullscreen *)vu_create(0, FULLSCREEN,
			FULLSCREEN_INPUT_WINDOW, Fv_frame,
			WIN_CURSOR, fv_get_drag_cursor(copy),
			WIN_CONSUME_EVENTS, 
				WIN_MOUSE_BUTTONS, LOC_DRAG, LOC_MOVEWHILEBUTDOWN,0,
			FULLSCREEN_SYNC, FALSE,
			0);
#endif


	if (Fv_style & FV_DICON)
	{
		iconwidth = GLYPH_WIDTH;
		iconheight = GLYPH_HEIGHT;
	}
	else
	{
		iconwidth = iconheight = 16;
		y_off = 5;
	}
	/* Get offset into canvas; decrement y coordinate by offset */

	scroll_offset = GET_SCROLL_VIEW_START;

	x += Fv_margin;
	y += foldr_top;

	/* Well, the user must want to drag some icons around, so lets 
	 * set him up...
	 */
	nseln = 0;
	for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles, i=0; f_p != l_p; f_p++, i++)
		if ((*f_p)->selected)
		{
			Seln[nseln].fno = i;
			if ((Fv_style & FV_DICON) || !Fv_style )
				Seln[nseln].x = (*f_p)->x+Fv_margin+x_off;
			else
				Seln[nseln].x = MARGIN+Fv_margin+x_off;
			Seln[nseln].y = trans(i)-scroll_offset+foldr_top+y_off;
			/* Refuse to move icons which are invisible */
			if (Seln[nseln].y >= Fv_winheight)
				continue;
			if (Fv_style & FV_DICON)
				Seln[nseln].pr = 
				       (*f_p)->icon>EMPTY && Fv_bind[(*f_p)->icon].icon 
					? Fv_bind[(*f_p)->icon].icon
					: Fv_icon[(*f_p)->type];
			else
				Seln[nseln].pr = Fv_list_icon[(*f_p)->type];
			nseln++;

			if (nseln == 64)
			{
				fv_putmsg(TRUE, "Sorry, only 64 files can be dragged; all will be %s",
					(int)(copy ? "copied" : "moved"), 0);
				break;
			}
		}


	really_drag = copy;	/* Leave original in place for copy */
	target = EMPTY;
	lock = EMPTY;
	tree_target = NULL;
	tree_lock = NULL;

	while(window_read_event(Fv_frame, &ev) != -1 && event_id(&ev) != MS_LEFT)
	{
		x1 = event_x(&ev);
		y1 = event_y(&ev);

#ifdef SV1
		/* When copying files; we don't want to remove the
		 * images in the first iteration so as to give the
		 * user the illusion of having made a physical copy
		 * of the icons.
		 */
		if (really_drag==FALSE)
		{
			for (i=0; i<nseln; i++)
				PAINT Seln[i].x, Seln[i].y,
					iconwidth, iconheight,
					PIX_SRC^PIX_DST,
					Seln[i].pr, 0, 0);
		}
		really_drag=FALSE;
#endif

		/* As the user drags his objects over folders we
		 * need to provide him with feedback indication that
		 * he can place the objects in this folder.  This can
		 * be difficult to do as the sampling of events drops
		 * as the mouse is moved faster.  So, for instance,
		 * we could be over a folder in the tree and on the
		 * next event, over a folder in the folder pane.
		 */
		if (y1>=tree_top && y1<=foldr_top)
		{
			/* In tree canvas...over anything? */

			t_p = fv_infolder(x1-Fv_margin, y1-tree_top);
			fv_tree_feedback(t_p, &tree_target, &tree_lock,
				Fv_current, target_tree_path);
		}
		else if (y1>=foldr_top && y1<=Fv_winheight)
		{
			/* In folder canvas.  
			 * List icon folder just get inverted if you can
			 * open them; nothing happens if you can't.
			 */

			i=over_object(x1-Fv_margin, y1-foldr_top);
			if (i != -1 && Fv_file[i]->type == FV_IFOLDER &&
				!Fv_file[i]->selected)
			{
				if (target != i)
				{
					if (target != EMPTY)
					{
						/* Close old... */
						draw_folder(target, FALSE);
						target = EMPTY;
					}


					if (access(Fv_file[i]->name, W_OK) == 0)
					{
						/* Open new... */
						draw_folder(i, TRUE);
						target = i;
					}
					else if (lock != i && Fv_style & FV_DICON)
					{
						/* Show lock... */
						if (lock != EMPTY)
							pw_rop(Fv_foldr.pw,
								Fv_file[lock]->x,
								trans(lock),
								GLYPH_WIDTH, 
								GLYPH_HEIGHT, PIX_SRC, 
								Fv_icon[FV_IFOLDER], 0, 0);
						target = EMPTY;
						lock = i;
						pw_rop(Fv_foldr.pw,
							Fv_file[lock]->x,
							trans(lock),
							GLYPH_WIDTH, 
							GLYPH_HEIGHT, PIX_SRC, 
							Fv_lock, 0, 0);
					}
				}
			}
			else if (target != EMPTY || lock != EMPTY)
			{
				/* Empty space; close/unlock */
				if (lock != EMPTY)
					target = lock;
				draw_folder(target, FALSE);
				target = EMPTY;
				lock = EMPTY;
			}
		}
		

		/* Draw new images after movement */
		for (i=0; i<nseln; i++)
		{
			Seln[i].x += x1-x;
			Seln[i].y += y1-y;
#ifdef SV1
			PAINT Seln[i].x, Seln[i].y,
				iconwidth, iconheight,
				PIX_SRC^PIX_DST, Seln[i].pr, 0, 0);
#endif
		}

		x = x1;
		y = y1;
	}



#ifdef SV1
	/* Restore original object outline.  */
	if (!copy)
		for (i=0; i<nseln; i++)
		{
			if ((Fv_style & FV_DICON) || !Fv_style )
				x = Fv_file[Seln[i].fno]->x+Fv_margin+x_off;
			else
				x = MARGIN+Fv_margin+x_off;
			PAINT x, trans(Seln[i].fno)-
				scroll_offset+foldr_top+y_off,
				iconwidth, iconheight,
				PIX_SRC^PIX_DST, 
				Seln[i].pr, 0, 0);
		}


	/* Remove last position */
	for (i=0; i<nseln; i++)
		PAINT Seln[i].x, Seln[i].y, 
			iconwidth, iconheight,
			PIX_SRC^PIX_DST, Seln[i].pr, 0, 0);

	fv_cursor(fd, FALSE);

	/* Stop looking for all events */

	win_releaseio(fd);
#endif
	fullscreen_destroy(fs);

	desktop_link = FALSE;

	winx = (int)window_get(Fv_frame, WIN_X);
	winy = (int)window_get(Fv_frame, WIN_Y);

#ifdef SV1
	if (rootfd = open(getenv("WINDOW_PARENT"), O_RDONLY))
#endif
	{
#ifdef SV1
		i = win_findintersect(rootfd, winx+x1, winy+y1);
#else
		i = win_pointer_under(Fv_frame, x1, y1);
#endif

		/* Copy out to desktop if not over fileview or not over 
		 * client */
#ifdef SV1
		if (i != (int)window_get(Fv_foldr.canvas, WIN_DEVICE_NUMBER) &&
		    i != (int)window_get(Fv_tree.canvas, WIN_DEVICE_NUMBER) &&
		    load_file(i) == -1 &&
		    fv_load_client(i) == -1)
#else
		if (i == 0)
#endif
		{
#ifndef SV1
			int data[5];
			data[0] = (int)VU_POINTER_WINDOW;
			data[1] = x1;
			data[2] = y1;
			data[3] = (int)vu_get(Fv_frame, VU_XID);
			(void)fv_set_primary_selection();
			vu_send_message(Fv_frame, 0, "VU_DO_DRAG_LOAD",
				32, data, 16);
#else
			desktop_link = TRUE;

			for (i=0; i<nseln; i++)
			{
				/* Ensure icon will be visible */
				x = winx+Seln[i].x;
				if (x < 0)
					x = 0;
				if (x>Fv_screenwidth-64)
					x = Fv_screenwidth-64;
				y = winy+Seln[i].y;
				if (y < 0)
					y = 0;
				if (y>Fv_screenheight-64)
					y = Fv_screenheight-64;

				link_to_desktop(Fv_file[Seln[i].fno], x, y);
			}
#endif
		}
#ifdef SV1
		(void)close(rootfd);
#endif
	}


	fv_busy_cursor(TRUE);

	/* If user has moved back into folder pane then don't 
	 * move/copy into last opened folder in tree.
	 */
	if (y1>=foldr_top && y1<=Fv_winheight && tree_target)
		desktop_link = TRUE;

	if (target != EMPTY || lock != EMPTY)
	{
		/* XXX It is possible to have locked/open folders in folder pane
		 * without being over them.
		 */
		if (desktop_link)
			target = target != EMPTY ? target : lock;
		else
			if (target != EMPTY)
			{
				(void)fv_move_copy(Fv_file[target]->name, copy);
				target = EMPTY;
			}
			else
				target = lock;
		if (target != EMPTY)
			pw_rop(Fv_foldr.pw, Fv_file[target]->x, 
				trans(target)-scroll_offset, iconwidth, 
				iconheight, PIX_SRC, Fv_icon[FV_IFOLDER], 0, 0);
	}
	else if (tree_target || tree_lock)
	{
		if (desktop_link)
			tree_target = tree_target ? tree_target : tree_lock;
		else
			if (tree_target)
				(void)fv_move_copy(target_tree_path, copy);
			else
				tree_target = tree_lock;
		pw_rop(Fv_tree.pw, tree_target->x, tree_target->y,
			GLYPH_WIDTH, TREE_GLYPH_HEIGHT, PIX_SRC, 
			Fv_icon[FV_IFOLDER], 0, TREE_GLYPH_TOP);
		fv_put_text_on_folder(tree_target);
	}
	else if (!desktop_link && copy)
		fv_duplicate();

	fv_busy_cursor(FALSE);
}


static
draw_folder(i, open)		/* Show folder in the folder pane open/close */
	register int i;		/* File index */
	BOOLEAN open;		/* Display opened? */
{
	SBYTE color =  Fv_file[i]->color;

	if (Fv_style & FV_DICON)
	{
		pw_rop(Fv_foldr.pw, Fv_file[i]->x, trans(i),
			GLYPH_WIDTH, GLYPH_HEIGHT, PIX_SRC | PIX_COLOR(color),
			Fv_icon[open?FV_IOPENFOLDER:FV_IFOLDER], 0, 0);

		if (open)
			pw_rop(Fv_foldr.pw, Fv_file[i]->x,
				trans(i), GLYPH_WIDTH, GLYPH_HEIGHT,
				(PIX_SRC^PIX_DST) | PIX_COLOR(color),
				Fv_invert[FV_IOPENFOLDER], 0, 0);
	}
	else
		fv_reverse(i);
}


static
link_to_desktop(f_p, x, y)	/* Load appl with file and place on desktop */
	register FV_FILE *f_p;	/* File */
	int x, y;		/* Location */
{
	char buf[MAXPATH];	/* buffer */
	Frame f;		/* Folder frame */
	int argc;		/* # Frame arguments */
	char *argv[10];		/* Frame arguments */
	Rect *r;		/* Icon rect */
	char red[4], blue[4], green[4];	/* RGB color strings */

	if (f_p->color > BLACK)
	{
		/* Paint window with file's colors */

		(void)sprintf(red, "%d", Fv_red[f_p->color]);
		(void)sprintf(green, "%d", Fv_green[f_p->color]);
		(void)sprintf(blue, "%d", Fv_blue[f_p->color]);
	}

	if (f_p->type == FV_IFOLDER)
	{
#ifdef SV1
		F_icon.ic_width = 64;
		F_icon.ic_height = 48;
		F_icon.ic_gfxrect.r_width = 64;
		F_icon.ic_gfxrect.r_height = 48;
		F_icon.ic_mpr = &f_icon_pr;
#endif
		(void)sprintf(buf, "fileview: %s/%s", 
			*Fv_path ? Fv_path : "", f_p->name);
			
		if (f_p->color > BLACK)
		{
			argc = 5;
			argv[0] = f_p->name;
			argv[1] = "-Wb";
			argv[2] = red;
			argv[3] = green;
			argv[4] = blue;
			argv[5] = 0;
		}
		else
		{
			argc = 0;
			argv[0] = 0;
		}
		if ((f = window_create(0, FRAME,
			FRAME_ARGS, argc, argv,
#ifdef SV1
			FRAME_ICON, &F_icon,
#else
			FRAME_ICON, vu_create(VU_NULL, ICON, ICON_IMAGE,
					&f_icon_pr, VU_WIDTH, 64, VU_HEIGHT, 48, 0),
#endif
			FRAME_CLOSED, TRUE,
			FRAME_NO_CONFIRM, TRUE,
			FRAME_LABEL, buf,
			0)) &&
		    (window_create(f, CANVAS,
			CANVAS_WIDTH, 400,
			CANVAS_HEIGHT, 200,
			0)))
		{
#ifdef PROTO
			frame_set_font_size(f, 
				window_get(Fv_frame, WIN_FONT_SIZE));
#endif
			fv_name_icon(f, f_p->name);
			if (r = (Rect *)window_get(f, FRAME_CLOSED_RECT))
			{
				r->r_top = y;
				r->r_left = x;

				window_set(f, FRAME_CLOSED_RECT, r,
					WIN_SHOW, TRUE, 0);
			}

			if (Nfolders < MAX_FOLDERS)
			{
				/* Keep track of folders */

				Folder_frame[Nfolders] = f;
				Nfolders++;
			}
		}
		else
			fv_putmsg(TRUE, Fv_message[MEWIN], 0, 0);
	}
	else if (f_p->type == FV_IAPPLICATION || f_p->type == FV_IDOCUMENT)
	{
		char xpos[6], ypos[6];

		if (f_p->type == FV_IDOCUMENT && 
			(f_p->icon == EMPTY || *Fv_bind[f_p->icon].application==0))
			edit(f_p->name, (char *)0, x, y, f_p->color);
		else
		{
			if (f_p->type == FV_IAPPLICATION)
				argv[0] = f_p->name; 
			else
				argv[0]=Fv_bind[f_p->icon].application;

			argv[1] = "-Wi";
			argv[2] = "-WP";
			(void)sprintf(xpos, "%d", x);
			(void)sprintf(ypos, "%d", y);
			argv[3] = xpos;
			argv[4] = ypos;
			argc = 5;
			if (f_p->color > BLACK)
			{
				argv[argc++] = "-Wb";
				argv[argc++] = red;
				argv[argc++] = green;
				argv[argc++] = blue;
			}
			if (f_p->type == FV_IDOCUMENT)
				argv[argc++] = f_p->name;
			argv[argc] = 0;
			fv_putmsg(FALSE, Fv_message[MLINK], (int)f_p->name, 0);
			fv_execit(argv[0], argv);
		}
	}
}


fv_display_folder(mode)
	BYTE mode;			/* Build, style change, display? */
{
	register FV_FILE **f_p, **nf_p, **l_p;	/* Files array pointers */
	register int j;			/* Indeces */
	register int lower, upper, mid;	/* Binary search variables */
	time_t mtime;			/* Change date on folder */
	BOOLEAN new;			/* Tree changed? */
	FV_FILE *deleted[MAXFILES];	/* Deleted files */
	short ndeleted;			/* Number of deleted files */


	if (mode == FV_BUILD_FOLDER)
		build_folder(&mtime);
	else 
		Ncolors = 0;


	if (mode <= FV_STYLE_FOLDER)
	{
		/* Find bound icons and applications, clear selection */

		for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles; f_p < l_p; f_p++)
		{
			/* Look for bound icons... */

			(*f_p)->icon = EMPTY;
			if (Fv_style & FV_DICON)
			{
				if ((*f_p)->type == FV_IAPPLICATION && Fv_nappbind)
				{
					/* Binary search for application icon */

					lower=0;
					upper=Fv_nappbind-1;
					while (lower<=upper)
					{
						mid = (lower+upper)/2;
						if ((j=strcmp((*f_p)->name, 
							Fv_bind[mid].pattern))<0)
							upper = mid-1;
						else if (j>0)
							lower = mid+1;
						else
						{
							(*f_p)->icon = mid;
							break;
						}
					}
				}
				if ( (*f_p)->icon == EMPTY &&
				     (*f_p)->type == FV_IDOCUMENT )
					check_bind(*f_p);
			}
			(*f_p)->selected = FALSE;
			if (Ncolors)
			{
				/* Binary search for color entry */

				lower=0;
				upper=Ncolors-1;
				while (lower<=upper)
				{
					mid = (lower+upper)/2;
					if ((j=strcmp((*f_p)->name, Color_fname[mid]))<0)
						upper = mid-1;
					else if (j>0)
						lower = mid+1;
					else
					{
						(*f_p)->color = Color_code[mid];
						break;
					}
				}
			}
		}


		/* Sort files */

		if (Fv_sortby != FV_NOSORT)
			qsort((char *)Fv_file, Fv_nfiles, sizeof(FV_FILE *),
				Compare_fn[Fv_sortby]);

		display_folder(Fv_foldr.canvas, Fv_foldr.pw, (Rectlist *)0);

		if (mode == FV_BUILD_FOLDER)
		{
			/* XXX Can we combine these? */
			if (Fv_current->mtime == 0)
			{
				/* Tell tree that we've opened a folder
				 * that he hasn't seen before.
				 */
				Fv_current->mtime = mtime;
				fv_updatetree();
			}
			else if (Fv_current->mtime < mtime)
			{
				fv_add_children(Fv_current, &new, (char *)0);
				if (new && Fv_treeview)
					fv_drawtree(TRUE);
			}
		}
		return;
	}

	if (mode == FV_SMALLER_FOLDER)
	{
		/* Remove folders marked for deletion by squeezing array.
		 * Keep track of largest string width.
		 */

		ndeleted = 0;
		Widest_name = Fv_style & FV_DICON ? GLYPH_WIDTH : 0;
		l_p = Fv_file+Fv_nfiles;
		f_p = Fv_file;
		while (f_p < l_p)
		{
			if ((*f_p)->mtime == 1)
			{
				free((*f_p)->name);
				(*f_p)->mtime = 0;
				nf_p = f_p;
				deleted[ndeleted] = *f_p;
				ndeleted++;
				while (nf_p < l_p)
					*nf_p = *(nf_p+1), nf_p++;
			}
			else
			{
				if ((*f_p)->width > Widest_name)
					Widest_name = (*f_p)->width;
				f_p++;
			}
		}

		/* Put deleted structures at end */
		Fv_nfiles -= ndeleted;
		l_p = Fv_file+Fv_nfiles;
		while (ndeleted)
		{
			ndeleted--;
			*l_p = deleted[ndeleted];
			l_p++;
		}
	}

	display_folder(Fv_foldr.canvas, Fv_foldr.pw, (Rectlist *)0);
}


static
display_folder(cnvs, pw, area)		/* Display files in folder */
	Canvas cnvs;			/* Canvas handle */
	PAINTWIN pw;			/* Pixwin/Paint window */
	Rectlist *area;
{
	Rect *r;			/* Canvas dimensions */

	if (Fv_dont_paint)
		return;

	if (area)
		r = &area->rl_bound;
	else
	{
		/* If null, the area is the entire window */

		r = (Rect *)window_get(cnvs, WIN_RECT);
		r->r_top = GET_SCROLL_VIEW_START;
	}

#ifdef SV1
	pw_lock(pw, r);
#endif
	pw_writebackground(pw, r->r_left, r->r_top, r->r_width, r->r_height, 
		PIX_CLR);

	if (Fv_nfiles)
		if (!(Fv_style & FV_DICON) && Fv_style)
			display_list(pw, r);
		else
			display_icon(pw, r);

#ifdef SV1
	pw_unlock(pw);
#endif
}


static
display_list(pw, r)		/* List style with at least one List option */
	register PAINTWIN pw;		/* Paint window */
	register Rect *r;		/* Repaint area */
{
	register int i, j;		/* Indeces */
	register int x, y;		/* Coordinates */
	register FV_FILE **f_p, **l_p;	/* Files array pointers */
	char buffer[256];
	char *b_p;
	char *fmtmode();
	time_t now, sixmonthsago, onehourfromnow;
	char *getname(), *getgroup();


	i = (Fv_nfiles+1)*LIST_HEIGHT;
	if (i != Fv_foldr.height)		/* New or enlarged folder */
#ifdef SV1
		set_canvas_height(i, r);
#else
		if (set_canvas_height(i, r))
			return;
#endif

	if (Fv_style & FV_DDATE)
	{
		/* Allow for formatting the date sensibly... */

		now = time((time_t *)0);
		sixmonthsago = now - SIX_MONTHS_IN_SECS;
		onehourfromnow = now + HOUR_IN_SECS;
	}

	j = r->r_top/LIST_HEIGHT;
	y = (j+1)*LIST_HEIGHT;

	f_p = Fv_file+j;
	j = r->r_height/LIST_HEIGHT;
	j++;				/* One extra */
	for (l_p=Fv_file+Fv_nfiles, i=0; f_p < l_p && i<j; f_p++, i++)
	{
		x = MARGIN;
		pw_rop(pw, x, y-15, 16, 16, 
			PIX_SRC | PIX_COLOR((*f_p)->color),
		       Fv_list_icon[(*f_p)->type], 0, 0);
		x += LIST_HEIGHT;
		if (Fv_style & FV_DPERMISSIONS)
		{
			b_p = buffer;
			*b_p++ = (*f_p)->type == FV_IFOLDER ? 'd' : '-';
			b_p = fmtmode(b_p, (int)(*f_p)->mode);
			*b_p = NULL;
			pw_text(pw, x, y, PIX_SRC, Fv_font, buffer);
			x += Fv_fontsize.x*Permission_width[Fv_fixed];
		}
		if (Fv_style & FV_DLINKS)
		{
			(void)sprintf(buffer, "%3d", (*f_p)->nlink);
			pw_text(pw, x, y, PIX_SRC, Fv_font, buffer);
			x += Fv_fontsize.x*Link_width[Fv_fixed];
		}
		if (Fv_style & FV_DOWNER)
		{
			if ((b_p = getname((*f_p)->uid)) == 0)
			{
				(void)sprintf(buffer, "%d", (*f_p)->uid);
				b_p = buffer;
			}
			pw_text(pw, x, y, PIX_SRC, Fv_font, b_p);
			x += Fv_fontsize.x*Owner_width[Fv_fixed];
		}
		if (Fv_style & FV_DGROUP)
		{
			if ((b_p = getgroup((*f_p)->gid)) == 0)
			{
				(void)sprintf(buffer, "%d", (*f_p)->gid);
				b_p = buffer;
			}
			pw_text(pw, x, y, PIX_SRC, Fv_font, b_p);
			x += Fv_fontsize.x*Group_width[Fv_fixed];
		}
		if (Fv_style & FV_DSIZE)
		{
			(void)sprintf(buffer, "%-8ld", (*f_p)->size);
			pw_text(pw, x, y, PIX_SRC, Fv_font, buffer);
			x += Fv_fontsize.x*Size_width[Fv_fixed];
		}
		if (Fv_style & FV_DDATE)
		{
			b_p = ctime(&((*f_p)->mtime));
			if ( (*f_p)->mtime < sixmonthsago ||
			     (*f_p)->mtime > onehourfromnow )
				(void)sprintf(buffer,"%-7.7s%-4.4s", b_p+4,
					b_p+20);
			else
				(void)sprintf(buffer,"%-12.12s", b_p+4);
			pw_text(pw, x, y, PIX_SRC, Fv_font, buffer);
			x += Fv_fontsize.x*Date_width[Fv_fixed];
		}

		(*f_p)->x = x;
		pw_text(pw, x, y, PIX_SRC, Fv_font, (*f_p)->name);

		if ((*f_p)->selected)
			pw_rop(Fv_foldr.pw, MARGIN-1, y-16, 18, 18,
				(PIX_SRC^PIX_DST) | PIX_COLOR((*f_p)->color), 
				(Pixrect *)0, 0, 0);

		y += LIST_HEIGHT;
	}
}


static
display_icon(pw, r)
	register PAINTWIN pw;
	register Rect *r;
{
	register int i, j;		/* Indeces */
	register int x, y;		/* Coordinates */
	register int h;			/* Height of icon */
	int w;				/* Name width */
	register char c, c1;
	register char *p;
	int widest;			/* For names widest than window */
	int startx;			/* Where to start display */
	int incx;			/* Increment, max x coord */
	register FV_FILE **f_p, **l_p;	/* Files array pointers */


	/* Collect increments, starting positions, etc... */

	/* Can't be wider than screen width */
	widest = Widest_name;
	if (widest > Fv_maxwidth)
		widest = Fv_maxwidth;
	if (Fv_style==0)
		widest += LIST_HEIGHT;
	if (widest > Fv_tree.r.r_width)
		widest = Fv_tree.r.r_width-(GLYPH_WIDTH>>3);

	Incx = incx = widest + (GLYPH_WIDTH>>3);

	/* Must be at least one file per line */
	if ((Maxfilesperline = Fv_tree.r.r_width/incx) == 0)
		Maxfilesperline = 1;

	/* Make left and right margins the same */
	startx = Fv_tree.r.r_width%incx/2;

	if (Fv_style & FV_DICON)
	{
		/* Icons should be centered over text... */
		if (GLYPH_WIDTH < widest)
			startx += (widest>>1)-(GLYPH_WIDTH>>1);

		/* Canvas height */
		h = ICON_AREA_HEIGHT;
		i = h*((Fv_nfiles/Maxfilesperline)+1)+TOP_MARGIN;

		/* Starting positions */
		j = (r->r_top-TOP_MARGIN)/h;
		if (j<0)
			j = 0;
		y = j*h+TOP_MARGIN;
		j *= Maxfilesperline;
	}
	else
	{
		h = LIST_HEIGHT;		/* Mini-icon height */
		i = h*((Fv_nfiles/Maxfilesperline)+1);

		/* Starting positions */
		j = r->r_top/h;
		/* Portion of previous line may only be visible; repaint */
		if (j)
			j--;		
		y = (j+1)*h;
		j *= Maxfilesperline;
	}

	if (i != Fv_foldr.height) 
	{
#ifdef SV1
		set_canvas_height(i, r);
#else
		if (set_canvas_height(i, r))
			return;
#endif
		r->r_top = 0;
		j = 0;
		y = Fv_style ? TOP_MARGIN : LIST_HEIGHT;
	}

	Startx = x = startx;
	for (f_p=Fv_file+j, l_p=Fv_file+Fv_nfiles, i=0; f_p < l_p; f_p++)
	{
		if (Fv_style & FV_DICON)
		{
			pw_rop(pw, x, y, GLYPH_WIDTH, GLYPH_HEIGHT, 
				PIX_SRC | PIX_COLOR((*f_p)->color),
			       (*f_p)->icon>EMPTY && Fv_bind[(*f_p)->icon].icon 
				? Fv_bind[(*f_p)->icon].icon
				: Fv_icon[(*f_p)->type], 0, 0);

			if ((*f_p)->width > widest) 
			{
				/* Name is too long; truncate it to max width
				 * This is calculated in pixels for variable
				 * width fonts.  Indicate truncation with
				 * arrow.
				 */

				if (Fv_fixed)
				{
					 p = ((*f_p)->name+(widest/Fv_fontsize.x));
					 w = widest;
				}
				else
				{
					p = (*f_p)->name;
					w = Fv_fontwidth['>'-' '];
					while (*p && w < widest)
						w += Fv_fontwidth[*p-' '], p++;
						
				}
				c = *p;
				c1 = *(p+1);
				*p = '>';
				*(p+1) = 0;

				pw_text(pw, (x+(GLYPH_WIDTH>>1))-(w/2),
					y+Fv_fontsize.y+GLYPH_HEIGHT,
					PIX_SRC, Fv_font, (*f_p)->name);
				*p = c;
				*(p+1) = c1;
			}
			else
			pw_text(pw, (x+(GLYPH_WIDTH>>1))-((*f_p)->width/2),
				y+Fv_fontsize.y+GLYPH_HEIGHT,
				PIX_SRC, Fv_font, (*f_p)->name);
		}
		else
		{
			pw_rop(pw, x, y-15, 16, 16, 
				PIX_SRC | PIX_COLOR((*f_p)->color),
			       Fv_list_icon[(*f_p)->type], 0, 0);
			pw_text(pw, x+LIST_HEIGHT, y,
				PIX_SRC, Fv_font, (*f_p)->name);
		}
		(*f_p)->x = x;


		if ((*f_p)->selected)
			fv_reverse(i+j);

		x += incx;
		i++;
		if ((i % Maxfilesperline) == 0)
		{
			x = startx;
			y += h;
			if (y > r->r_top+r->r_height+LIST_HEIGHT)
				break;
		}
	}
}


static
set_canvas_height(i, r)
	int i;
	Rect *r;
{
#ifdef SV1
	Fv_dont_paint = TRUE;
	if (i < (int)window_get(Fv_foldr.canvas, WIN_HEIGHT))
	{
		if (window_get(Fv_foldr.canvas, WIN_VERTICAL_SCROLLBAR))
		{
			/* Remove the scrollbar; no events */
			scrollbar_scroll_to(Fv_foldr.vsbar, 0);
			/* Ensure we start at the top */
			window_set(Fv_foldr.canvas, WIN_VERTICAL_SCROLLBAR, 
				NULL, 0);
			/* Removing the scrollbar doesn't send a repaint event */
			r->r_top = 0;
			pw_writebackground(Fv_foldr.pw, r->r_left, r->r_top, r->r_width+20, 
				r->r_height, PIX_CLR);
		}
	}
	else
	{
		window_set(Fv_foldr.canvas, CANVAS_HEIGHT, i, 0);

		/* Apply scrollbar to canvas if not present already */
		if (window_get(Fv_foldr.canvas, WIN_VERTICAL_SCROLLBAR)==0)
			window_set(Fv_foldr.canvas, WIN_VERTICAL_SCROLLBAR, 
				Fv_foldr.vsbar, 0);
	}
	Fv_dont_paint = FALSE;
	Fv_foldr.height = i;
#else
	if ((int)scrollbar_get(Fv_foldr.vsbar, SCROLL_VIEW_START))
		scrollbar_scroll_to(Fv_foldr.vsbar, 0);
	window_set(Fv_foldr.canvas, CANVAS_HEIGHT, i, 0);
	Fv_foldr.height = i;
	return(i > window_get(Fv_foldr.canvas, WIN_HEIGHT));
#endif
}

static
build_folder(mtime)
	time_t *mtime;			/* Change date on folder */
{
	register FV_FILE **f_p, **l_p;	/* File array pointers */
	register int i, j;		/* Indeces */
	register struct dirent *dp;	/* Directory content */
	register DIR *dirp;		/* Directory file pointer */
	struct stat fstatus;		/* File status */
	register FILE *fp;		/* Host's file pointer */
	char buf[255];			/* Host folder buffer */
	extern char Fv_hosts[];		/* Hosts file, display in /net */


	/* Ensure that we're still in this folder, (we could
	 * have changed to another by merely selecting a folder
	 * in the tree...) 
	 */
	 
	if (Fv_treeview && Fv_tselected != Fv_current)
	{
		if (chdir(Fv_openfolder_path)==-1)
			return;
		(void)strcpy(Fv_path, Fv_openfolder_path);
	}

	/* Remember folder history */

	if (Fv_nhistory < 2 || strcmp(Fv_openfolder_path, Fv_history[1]))
	{
		if (Fv_history[0] == NULL)
		{
			/* First slot in history array always contains
			 * the home folder.  (So the default selection
			 * off the HOME menu works correctly.)
			 */
			Fv_history[0] = malloc((unsigned)strlen(Fv_home)+1);
			(void)strcpy(Fv_history[0], Fv_home);
			Fv_nhistory++;
		}

		/* Is this folder different than previous folders? */
		for (i=0; i<Fv_nhistory; i++)
			if (strcmp(Fv_history[i], Fv_openfolder_path)==0)
				break;

		if (i==Fv_nhistory)
		{
			/* It's different; add it to our collection */

			if (Fv_nhistory==FV_MAXHISTORY)
				free(Fv_history[Fv_nhistory-1]);
			else
				Fv_nhistory++;
			for (i=Fv_nhistory-1; i>=1; i--)
				Fv_history[i] = Fv_history[i-1];
			Fv_history[1] = malloc((unsigned)strlen(Fv_openfolder_path)+1);
			if (Fv_history[1])
				(void)strcpy(Fv_history[1], Fv_openfolder_path);
		}

		/* Update the namestripe */
		if (Fv_treeview)
			fv_namestripe();
	}


	/* Read directory */

	if ( ((dirp=opendir(".")) == NULL) || fstat(dirp->dd_fd, &fstatus) )
	{
		if (dirp)
			(void)closedir(dirp);
		fv_putmsg(TRUE, "Can't open current folder: %s",
			(int)sys_errlist[errno], 0);
		return;
	}

	*mtime = fstatus.st_mtime;	/* To test against current node */
	Fv_write_access = access(".", W_OK) == 0;

	/* Free old names */

	for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles; f_p != l_p; f_p++)
		free((*f_p)->name);

	Widest_name = Fv_style & FV_DICON ? GLYPH_WIDTH : 0;
	i = 0;
	f_p = Fv_file;
	Ncolors = 0;

	/* Read hosts file if we've changed to the automounter's folder */

	if (Fv_automount && (strcmp(Fv_openfolder_path, "/net") == 0) &&
		(fp = fopen(Fv_hosts, "r")))
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			(*f_p)->type = FV_IFOLDER;
			j = strlen(buf);
			buf[--j] = NULL;	/* Get rid of newline */
			if (((*f_p)->name = fv_malloc((unsigned)j+1)) == NULL)
				return;
			(void)strcpy((*f_p)->name, buf);

			/* Get name length */

			(*f_p)->width = (short)fv_strlen(buf);
			if ((*f_p)->width > Widest_name)
				Widest_name = (short)(*f_p)->width;
			(*f_p)->color = BLACK;
			(*f_p)->mode = S_IFDIR;
			(*f_p)->nlink = 0;
			(*f_p)->uid = 0;
			(*f_p)->gid = 0;
			(*f_p)->mtime = time((time_t *)0);
			(*f_p)->size = 0;
			f_p++;
			i++;
		}
		(void)fclose(fp);
	}
	else
	while (dp=readdir(dirp))
	{
		if (Fv_color && strcmp(dp->d_name, COLOR_FILE) == 0)
			store_colors();

		/* Always ignore dot and parent */

		if (Fv_see_dot)
		{
			/* Always ignore ..* files */

			if (dp->d_name[0] == '.' &&
			    (dp->d_name[1] == '.' || dp->d_name[1] == NULL))
				continue;
		}
		else
			if (dp->d_name[0] == '.')
				continue;
			

		/* What type is it? */
		/* XXX The permissions may change without our knowing! */

		if (stat(dp->d_name, &fstatus) == 0)
		{
			if ((fstatus.st_mode & S_IFMT) == S_IFDIR)
				(*f_p)->type = FV_IFOLDER;
			else if ((fstatus.st_mode & S_IFMT) == S_IFREG)
			{
				if (fstatus.st_mode & S_IEXEC)
					(*f_p)->type = FV_IAPPLICATION;
				else if (fstatus.st_mode & S_IFREG)
					(*f_p)->type = FV_IDOCUMENT;
			}
			else
				(*f_p)->type = FV_ISYSTEM;
		}
		else
			(*f_p)->type = FV_IBROKENLINK;


		/* Apply filter...but we want to keep folders */

		if (Fv_filter[0] && (*f_p)->type!=FV_IFOLDER)
		{
			j = fv_match(dp->d_name, Fv_filter);
			if (j==0)
				continue;
			else if (j==-1)		/* Pattern in error, null it. */
				Fv_filter[0] = NULL;
		}

		j = strlen(dp->d_name);
		if (((*f_p)->name = fv_malloc((unsigned)j+1)) == NULL)
			return;

		(void)strcpy((*f_p)->name, dp->d_name);

		(*f_p)->width = fv_strlen(dp->d_name);
		if ((*f_p)->width > Widest_name)
			Widest_name = (short)(*f_p)->width;

		(*f_p)->color = BLACK;

		/* Store other file status stuff */

		(*f_p)->mode = fstatus.st_mode;
		(*f_p)->nlink = fstatus.st_nlink;
		(*f_p)->uid = fstatus.st_uid;
		(*f_p)->gid = fstatus.st_gid;
		(*f_p)->mtime = fstatus.st_mtime;
		(*f_p)->size = fstatus.st_size;

		f_p++;
		i++;
		if (i==Max_alloc)
		{
			if (Max_alloc == MAXFILES) {
				fv_putmsg(FALSE, Fv_message[ME2MANYFILES],
					MAXFILES, 0);
				break;
			}
			/* Allocate another chunk of memory for these file
			 * structures.  (We never free this memory, but we
			 * also may never need it.)
			 */
			j = Max_alloc;
			Max_alloc += 128;
			for (; j<Max_alloc; j++)
			{
				Fv_file[j] = (FV_FILE *)fv_malloc(sizeof(FV_FILE));
				if (Fv_file[j] == NULL)
					break;
			}
		}
	}
	(void)closedir(dirp);
	Fv_nfiles = i;
}

/* Return -1, 0, 1 if less than, equal to, or greater than */

static
compare_name(f1, f2)
	FV_FILE **f1, **f2;
{
	return(strcmp((*f1)->name, (*f2)->name));
}

static
compare_time(f1, f2)
	register FV_FILE **f1, **f2;
{
	if ((*f1)->mtime > (*f2)->mtime)
		return(-1);
	else if ((*f1)->mtime < (*f2)->mtime) 
		return(1);
	else
		return(0);
}

static
compare_size(f1, f2)
	register FV_FILE **f1, **f2;
{
	if ((*f1)->size > (*f2)->size)
		return(-1);
	else if ((*f1)->size < (*f2)->size) 
		return(1);
	else
		return(0);
}

static
compare_type(f1, f2)
	register FV_FILE **f1, **f2;
{
	if ((*f1)->type > (*f2)->type)
		return(1);
	else if ((*f1)->type < (*f2)->type) 
		return(-1);
	else
		return(strcmp((*f1)->name, (*f2)->name));
}

static
compare_color(f1, f2)
	register FV_FILE **f1, **f2;
{
	if ((*f1)->color > (*f2)->color)
		return(1);
	else if ((*f1)->color < (*f2)->color) 
		return(-1);
	else
		return(strcmp((*f1)->name, (*f2)->name));
}


static
trans(y)		/* Translate File index to y coordinate */
	register int y;
{
	if (Fv_style & FV_DICON )
	{
		y /= Maxfilesperline;
		y *= ICON_AREA_HEIGHT;
		return(y+TOP_MARGIN);
	}
	else if (!Fv_style)
		y /= Maxfilesperline;


	return(y*LIST_HEIGHT);
}

static
text_start(i)		/* Return text start */
{
	if (Fv_style & FV_DICON)
		return(Fv_file[i]->x+(GLYPH_WIDTH>>1)
			-(Fv_file[i]->width>>1));
	else if (Fv_style)
		return(Fv_file[i]->x);
	else
		return(Fv_file[i]->x+LIST_HEIGHT);
}


static
over_object_name(x, y)	/* Return file index if over name */
	register int x, y;
{
	register int i, j;

#ifdef SV1
	y += GET_SCROLL_VIEW_START;
#endif

	if (Fv_style & FV_DICON)
	{
		y -= TOP_MARGIN;
		i = y/ICON_AREA_HEIGHT;
		y -= i*ICON_AREA_HEIGHT;
		if (y >= GLYPH_HEIGHT && y <= GLYPH_HEIGHT+Fv_fontsize.y)
		{
			/* y coordinate is over text */

			j = (x-Startx)/Incx;
			j = x/Incx;
			i = (i*Maxfilesperline)+j;
			if (i < Fv_nfiles)
			{
				j = Fv_file[i]->x + (GLYPH_WIDTH/2);
				y = Fv_file[i]->width/2;
				if (x >= j-y && x <= j+y)
					return(i);
			}
		}
	}
	else if (Fv_style)
	{
		y /= LIST_HEIGHT;
		if (y < Fv_nfiles && x >= Fv_file[y]->x && 
			x <= Fv_file[y]->x + Fv_file[y]->width)
			return(y);
	}
	else
	{
		y /= LIST_HEIGHT;
		y *= Maxfilesperline;
		y += (x-Startx)/Incx;
		if (y < Fv_nfiles)
			if (x >= Fv_file[y]->x+LIST_HEIGHT && 
			    x <= Fv_file[y]->x+LIST_HEIGHT+Fv_file[y]->width)
				return(y);
	}

	return(EMPTY);
}


static
over_object(x, y)		/* Return file index if over icon */
	register int x, y;
{
	register int i, j;

#ifdef SV1
	y += GET_SCROLL_VIEW_START;
#endif

	/* Validate coordinates */
	if (x<0 || y<0)
		return(EMPTY);

	if (Fv_style & FV_DICON)
	{
		/* Before top margin? */
		if (y<TOP_MARGIN)
			return(EMPTY);
		y -= TOP_MARGIN;

		i = y/ICON_AREA_HEIGHT;
		y -= i*ICON_AREA_HEIGHT;

		/* Over name bar? */
		if (y>GLYPH_HEIGHT+Fv_fontsize.y)
			return(EMPTY);

		/* Before 1st? */
		if (x<Startx)
			return(EMPTY);
		j = (x-Startx)/Incx;

		i = (i*Maxfilesperline)+j;

		if (i < Fv_nfiles)
			if (x >= Fv_file[i]->x && x <= Fv_file[i]->x+GLYPH_WIDTH)
				return(i);
	}
	else if (!Fv_style)
	{
		/* Columnar names */

		y /= LIST_HEIGHT;
		y *= Maxfilesperline;
		y += (x-Startx)/Incx;
		if (y < Fv_nfiles)
			if (x >= Fv_file[y]->x && 
			    x <= Fv_file[y]->x+LIST_HEIGHT+Fv_file[y]->width)
				return(y);
	}
	else
	{
		/* Single file per line */

		y /= LIST_HEIGHT;
		if (y<Fv_nfiles)
			if (x >= MARGIN && x < MARGIN+LIST_HEIGHT ||
			    x >= Fv_file[y]->x && x <= Fv_file[y]->x+Fv_file[y]->width)
				return(y);
	}

	return(EMPTY);
}


fv_reverse(i)			/* Reverse object on canvas */
	register int i;
{
	int x, y;
	SBYTE color =  Fv_file[i]->color;

	if (Fv_style & FV_DICON)
	{
		/* Bound application icons are inverted with an ixon box */

		pw_rop(Fv_foldr.pw,
			Fv_file[i]->x, trans(i), GLYPH_WIDTH, GLYPH_HEIGHT,
			(PIX_SRC^PIX_DST) | PIX_COLOR(color),
			Fv_file[i]->type == FV_IAPPLICATION &&
			Fv_file[i]->icon != EMPTY ? 0 : Fv_invert[Fv_file[i]->type], 
			0, 0);
	}
	else
	{
		if (!Fv_style)
		{
			x = Fv_file[i]->x-1;
			y = (i/Maxfilesperline) * LIST_HEIGHT+4;
		}
		else
		{
			x = MARGIN-1;
			y = (i)*LIST_HEIGHT+4;
		}
		pw_rop(Fv_foldr.pw, x, y, 18, 18,
			(PIX_SRC^PIX_DST) | PIX_COLOR(color), 
			(Pixrect *)0, 0, 0);
	}

}


fv_file_open(seln, force_edit, arguments, shelltool)	/* Open file */
	int seln;
	BOOLEAN force_edit;
	char *arguments;		/* Run arg or Edit pattern */
	BOOLEAN shelltool;		/* Run in shelltool */
{
	register FV_FILE *n_p;
	register FV_TNODE *f_p;
	register char *s_p;
	BOOLEAN draw;
	BOOLEAN symlink;
	struct stat fstatus;
	char buf[MAXPATH];
	char *argv[32];			/* Big enough for most cases */
	int i;
	char red[4], green[4], blue[4];

	/* Error check */
	if (seln == EMPTY)
		return;

	n_p = Fv_file[seln];

	if (n_p->color > BLACK)
	{
		(void)sprintf(red, "%d", Fv_red[n_p->color]);
		(void)sprintf(green, "%d", Fv_green[n_p->color]);
		(void)sprintf(blue, "%d", Fv_blue[n_p->color]);
	}

	switch (n_p->type)
	{
	case FV_IFOLDER:

		symlink = ((lstat(n_p->name, &fstatus)==0) &&
		    (fstatus.st_mode & S_IFMT) == S_IFLNK);

		if (strcmp("/net", Fv_path) == 0)
		{
			(void)strcpy(buf, "/net/");
			(void)strcat(buf, n_p->name);
			if (chdir(buf) == -1)
			{
				fv_putmsg(TRUE, sys_errlist[errno], 0, 0);
				return;
			}
		}
		else
			if (chdir(n_p->name) == -1)
			{
				fv_putmsg(TRUE, sys_errlist[errno], 0, 0);
				return;
			}

		if (getwd(Fv_path) == 0)
		{
			/* Error message placed in pathname */
			fv_putmsg(TRUE, Fv_path, 0, 0);
			(void)strcpy(Fv_path, Fv_openfolder_path);
		}

		if (symlink)
		{
			fv_openfile(Fv_path, (char *)0, TRUE);
			break;
		}

		/* Keep current pointer in tree up to date */

		for (f_p = Fv_current->child; f_p; f_p = f_p->sibling)
			if (strcmp(f_p->name, n_p->name) == 0)
			{
				Fv_current = f_p;
				break;
			}

		/* Tell tree to redraw if we have't seen
		 * this folder before.  (The folder repaint
		 * routine informs the tree and resets the
		 * seen time.)
		 */
		draw = Fv_current->mtime == 0;
		(void)strcpy(Fv_openfolder_path, Fv_path);

		fv_display_folder(FV_BUILD_FOLDER);
		scrollbar_scroll_to(Fv_foldr.vsbar, 0);
		if (draw)
		{
			fv_drawtree(TRUE);
			fv_visiblefolder(Fv_current);
		}
		else
			fv_showopen();
		break;

	case FV_IAPPLICATION:
		if (force_edit)
			edit(n_p->name, (char *)0, EMPTY, EMPTY, n_p->color);
		else 
		{
			/* Supply arguments to application */
			i = 0;
			if (shelltool)
				argv[i++] = "/usr/bin/shelltool";
			else
				argv[i++] = n_p->name;
			if (n_p->color > BLACK)
			{
				argv[i++] = "-Wb";
				argv[i++] = red;
				argv[i++] = green;
				argv[i++] = blue;
			}
			if (shelltool)
				argv[i++] = n_p->name;
			if (arguments)
			{
				(void)strcpy(buf, arguments);
				s_p = (char *)strtok(buf, " ");
				while (s_p) {
					argv[i++] = s_p;
					s_p = (char *)strtok((char *)0, " ");
				}
			}
			argv[i] = 0;
			fv_execit(argv[0], argv);
		}
		break;

	case FV_IDOCUMENT:

		/* No large icons showing; check for bound applications */
		if ( !(Fv_style & FV_DICON) && n_p->icon == EMPTY)
			check_bind(n_p);

		if (n_p->icon != EMPTY && *Fv_bind[n_p->icon].application)
		{
			/* The user may have specified arguments; ie
			 * /bin/rm -r
			 */

			i = 0;
			(void)strcpy(buf, Fv_bind[n_p->icon].application);
			s_p = strtok(buf, " ");
			while (s_p) {
				argv[i++] = s_p;
				s_p = (char *)strtok((char *)0, " ");
			}
			if (n_p->color > BLACK)
			{
				argv[i++] = "-Wb";
				argv[i++] = red;
				argv[i++] = green;
				argv[i++] = blue;
			}
			argv[i++] = n_p->name;
			argv[i++] = 0;
			fv_execit(argv[0], argv);
		}
		else
			edit(n_p->name, arguments, EMPTY, EMPTY, n_p->color);
		break;

	default:
		fv_putmsg(TRUE, Fv_message[MEREAD], (int)n_p->name, 0);
	}
}


static
edit(name, pattern, x, y, color)	/* Edit filename */
	char *name;			/* Filename */
	char *pattern;			/* String pattern */
	int x, y;			/* Screen coordinates */
	SBYTE color;			/* Frame color */
{
	Textsw editsw;			/* Text editor window */
	Frame editframe;		/* Window frame */
	char buf[MAXPATH+30];		/* Buffer */
	Textsw_index first;
	Textsw_index last_plus_one;	/* Pattern match positions */
	Rect *r;
	int argc;
	char xpos[5], ypos[5];
	char *argv[14];
	char red[5], green[5], blue[5];
	static Notify_value textsw_recover();
#ifndef SV1
	static Notify_value editsw_event();
#endif


	if (Fv_editor == FV_TEXTEDIT)
		argv[0] = "textedit";
	else if (Fv_editor == FV_OTHER_EDITOR)
		argv[0] = Fv_other_editor;
	else
		argv[0] = "/bin/shelltool";
	argc = 1;
	if (color > BLACK)
	{
		argv[argc++] = "-Wb";
		(void)sprintf(red, "%d", Fv_red[color]);
		(void)sprintf(green, "%d", Fv_green[color]);
		(void)sprintf(blue, "%d", Fv_blue[color]);
		argv[argc++] = red;
		argv[argc++] = green;
		argv[argc++] = blue;
	}


#ifndef BUG	/* XXX BUG IN LEIF LOOK */
	if (Fv_editor == FV_TEXTEDIT)
	{
#ifdef SV1
		D_icon.ic_width = 56;
		D_icon.ic_height = 64;
		D_icon.ic_gfxrect.r_width = 56;
		D_icon.ic_gfxrect.r_height = 64;
		D_icon.ic_mpr = &d_icon_pr;
#endif

		argv[argc] = 0;

		editframe = window_create(0, FRAME,
			FRAME_SHOW_LABEL, TRUE,
			FRAME_CLOSED, x != EMPTY,
#ifdef SV1
			FRAME_ARGS, argc, argv,
			FRAME_EMBOLDEN_LABEL, TRUE,
			FRAME_ICON, &D_icon,
# ifdef PROTO
			FRAME_SHOW_FOOTER, FALSE,
# endif
#else
			FRAME_ICON, vu_create(VU_NULL, ICON, ICON_IMAGE, 
				&d_icon_pr, ICON_WIDTH, 56, ICON_HEIGHT, 64, 0),
#endif
			FRAME_NO_CONFIRM, TRUE,
			0);

		if ((editframe == NULL) ||
		    (editsw = (Textsw)window_create(editframe, TEXTSW,
			WIN_ROWS, 20,
			WIN_COLUMNS, 60,
			0)) == NULL)

		{
			fv_putmsg(TRUE, Fv_message[MEWIN], 0, 0);
			return;
		}
#ifndef SV1
		notify_interpose_event_func(vu_get(editsw, OPENWIN_NTH_VIEW, 0),
			editsw_event, NOTIFY_SAFE);
#endif

		if (Edit_no < MAX_TEXTSW)
		{
#ifdef SV1
			Edit_id[Edit_no] = (int)window_get(editsw, WIN_DEVICE_NUMBER);
			Edit_sw[Edit_no] = editsw;
#endif
			Edit_fr[Edit_no] = editframe;
			notify_interpose_destroy_func(editframe, textsw_recover);
			Edit_no++;
		}
		if (x != EMPTY &&
		   (r = (Rect *)window_get(editframe, FRAME_CLOSED_RECT)))
		{
			r->r_top = y;
			r->r_left = x;
			window_set(editframe, FRAME_CLOSED_RECT, r, 0);
		}

#ifdef PROTO
		frame_set_font_size(editframe, window_get(Fv_frame, WIN_FONT_SIZE));
#endif
		window_fit(editframe);

#ifndef SV1
		window_set(editsw,
			WIN_DESIRED_WIDTH, WIN_EXTEND_TO_EDGE,
			WIN_DESIRED_HEIGHT, WIN_EXTEND_TO_EDGE,
			WIN_CONSUME_EVENTS, LOC_WINENTER, LOC_WINEXIT, LOC_DRAG, 0,
			0);
#endif
		(void)sprintf(buf, "%s/%s", Fv_path[1] == 0 ? "" : Fv_path, name);
		read_file(editframe, editsw, buf, FALSE);

		if (pattern)
		{
			/* Textedit can be told to do pattern matching;
			 * however textedit can't understand regular 
			 * expressions...  so ignore any errors.  Set 
			 * selection.
			 */
			if (textsw_find_bytes(editsw, &first, &last_plus_one,
				pattern, strlen(pattern), 0) != -1)
			{
				textsw_normalize_view(editsw, first);
				textsw_set_selection(editsw, first,
					last_plus_one, 1);
			}
		}
	}
	else
#endif
	{
		if (x != EMPTY)
		{
			(void)sprintf(xpos, "%d", x);
			(void)sprintf(ypos, "%d", y);
			argv[argc++] = "-Wi";
			argv[argc++] = "-WP";
			argv[argc++] = xpos;
			argv[argc++] = ypos;
		}
		argv[argc++] = "-WL";
		argv[argc++] = name;
		if (Fv_editor == FV_VI)
			argv[argc++] = "vi";
		argv[argc++] = name;
		argv[argc] = 0;
		fv_execit(argv[0], argv);
	}
}

#ifdef SV1
static
load_file(id)
	int id;
{
	register int i;
	char buf[MAXPATH];

	for (i = 0; i< Edit_no; i++)
	{
		if (id == Edit_id[i])
		{
			(void)sprintf(buf, "%s/%s", Fv_path[1] == 0 ? "" : Fv_path,
				Fv_file[fv_getselectedfile()]->name);
			read_file(Edit_fr[i], Edit_sw[i], buf, TRUE);
			return(0);
		}
	}
	return(-1);
}
#endif


#ifndef SV1
Notify_value
editsw_event(win, evnt, arg, type)
	Vu_Window win;
	Event *evnt;
	Notify_arg arg;
	Notify_event_type type;
{
	char buf[MAXPATH];

	if (event_action(evnt) == ACTION_DRAG_LOAD)
	{
		Seln_holder holder;
		Seln_request *sbuf;

		holder = seln_inquire(SELN_PRIMARY);
		if (holder.state == SELN_NONE)
			fv_putmsg(TRUE, "No selection!", 0, 0);
		else {
			sbuf = seln_ask(&holder, SELN_REQ_CONTENTS_ASCII, 0, 0);
			read_file(window_get(win, WIN_FRAME), win, 
				sbuf->data+sizeof(Seln_attribute), TRUE);
		}
	}
	return(notify_next_event_func(win, evnt, arg, type));
}
#endif


fv_quit_frames()
{
	register int i;

	for (i = 0; i < Edit_no; i++)
		if (Edit_fr[i])
			window_destroy(Edit_fr[i]);
	for (i = 0; i < Nfolders; i++)
		if (Folder_frame[i])
			window_destroy(Folder_frame[i]);
}


static 
read_file(frame, editsw, name, old)
	Frame frame;
	Textsw editsw;
	char *name;
	BOOLEAN old;
{
	Textsw_status tstatus;			/* Error code from textsw */
	char buf[255];

	if (old)
		if (window_get(editsw, TEXTSW_MODIFIED))
			if (!alert_edit(editsw))
				return;

	window_set(editsw, TEXTSW_STATUS, &tstatus,
		TEXTSW_FILE, name,
		TEXTSW_FIRST, 0,
		0);
	if (tstatus != TEXTSW_STATUS_OKAY )
	{
		fv_putmsg(TRUE, "Can't read %s: %s", (int)name, 
			(int)sys_errlist[errno]);
		return;
	}

#ifdef SV1
	fv_name_icon(frame, strrchr(name, '/')+1);
#else
	{
	char *b_p = strrchr(name, '/')+1;	/* View2 bug */
	vu_set(vu_get(frame, FRAME_ICON), ICON_LABEL, b_p, 0);
	}
#endif

	(void)strcpy(Last_edited, name);
	(void)sprintf(buf, "fileview: %s", name);
	window_set(frame, FRAME_LABEL, buf, WIN_SHOW, TRUE, 0);
}


static
alert_edit(editsw)
	Textsw editsw;
{
	char buf[MAXPATH];
	int result;

	(void)sprintf(buf, "'%s' has been edited, what do you wish to do?",
		Last_edited);
	result = alert_prompt(editsw, NULL,
		ALERT_MESSAGE_STRINGS, buf, 0,
		ALERT_BUTTON, "Save edits", 1,
		ALERT_BUTTON, "Discard edits", 2,
		ALERT_BUTTON, "Cancel", 3,
		0);
	if (result==1)
		textsw_save(editsw, 0, 0);
	else if (result==2)
		textsw_reset(editsw, 0, 0);
	else
		return(FALSE);
	return(TRUE);
}


static Notify_value
textsw_recover(frame, status)		/* Recover space in Edit array */
	Frame frame;
	Destroy_status status;
{
	register int i;

	if (status == DESTROY_CHECKING)
	{
		for (i = 0; i < Edit_no; i++)
			if (Edit_fr[i] == frame)
				break;
		while (i<Edit_no)
			Edit_fr[i] = Edit_fr[i+1], i++;
		Edit_no--;
	}
	return(notify_next_destroy_func(frame, status));
}


fv_gotofile(name, pattern)	/* Open file from FIND list box */
	register char *name;		/* Filename */
	char *pattern;			/* String pattern matched in file */
{
	register int i;			/* Index into File array */
	register int y;			/* Coordinate */
	int top;			/* Current top of screen */


	/* XXX We could probably do a binary search here as long as it was
	 * sorted alphabetically...
	 */

	for (i=0; i<Fv_nfiles; i++)
		if (strcmp(Fv_file[i]->name, name) == 0)
			break;

	if (i>=Fv_nfiles || (access(Fv_file[i]->name, F_OK) == -1))
	{
		fv_putmsg(TRUE, Fv_message[MEFIND], (int)name, 0);
		return;
	}

	/* Found the file; scroll it into view... */

	y = trans(i);
	top = GET_SCROLL_VIEW_START;

	if (y<top || y>(top+(int)window_get(Fv_foldr.canvas, WIN_HEIGHT)))
		scrollbar_scroll_to(Fv_foldr.vsbar, y>GLYPH_HEIGHT ? y-GLYPH_HEIGHT : 0);

	fv_file_open(i, FALSE, Fv_file[i]->type == FV_IDOCUMENT ? pattern : NULL, FALSE);
}


static void
stop_editing()
{
	int l;				/* Length of text */
	FV_TNODE *t_p;			/* Tree pointer */
	char *new_name;			/* New name */
	FV_FILE *f_p = Fv_file[Renamed_file];

	new_name = (char *)panel_get(Text_item, PANEL_VALUE);
	window_set(Text_panel, WIN_SHOW, FALSE, 0);

	if (strlen(new_name) && strcmp(f_p->name, new_name))
		if (access(new_name, F_OK) == 0)
		{
			fv_putmsg(TRUE, Fv_message[MEEXIST],
				(int)new_name, 0);
		}
		else if (rename(f_p->name, new_name) == -1)
			fv_putmsg(TRUE, Fv_message[MERENAME],
				(int)sys_errlist[errno], f_p->name);
		else
		{
			char *m_p;

			/* Change name in tree */

			if (f_p->type == FV_IFOLDER)
			{
				for (t_p = Fv_current->child; t_p; t_p = t_p->sibling)
					if (strcmp(t_p->name, f_p->name) == 0)
						break;
				if (t_p)
				{
					if (strlen(new_name) > strlen(t_p->name))
					{
						m_p = fv_malloc((unsigned)strlen(new_name)+1);
						free(t_p->name);
						t_p->name = m_p;
					}
					(void)strcpy(t_p->name, new_name);
					fv_sort_children(t_p->parent);
				}
				if (Fv_treeview)
					fv_drawtree(FALSE);
			}

			m_p = fv_malloc((unsigned)strlen(new_name)+1);
			if (m_p)
			{
				free(f_p->name);
				f_p->name = m_p;
				(void)strcpy(f_p->name, new_name);
				Fv_current->mtime = time((time_t *)0);
			}

			/* Recalculate string width */
			l = fv_strlen(new_name);
			if (l > Widest_name)
				Widest_name = l;
			f_p->width = l;

			fv_display_folder(FV_STYLE_FOLDER);
			fv_match_files(new_name);
		}
}

/* Select files which match pattern */

fv_match_files(pattern)
	char *pattern;
{
	register FV_FILE **f_p, **l_p;	/* Files array pointers */
	register int i;			/* Index */
	register int y = EMPTY;
	int top;

	fv_treedeselect();		/* Turn off old selections */

	for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles, i=0; 
		f_p != l_p; f_p++, i++)
		if (fv_match((*f_p)->name, pattern) == 1)
		{
			if (!(*f_p)->selected)
				fv_reverse(i);
			(*f_p)->selected = TRUE;
			if (y == EMPTY)
				y = i;
		}
		else if ((*f_p)->selected)
		{
			/* Turn off old selections */

			fv_reverse(i);
			(*f_p)->selected = FALSE;
		}

	if (y != EMPTY)
	{
		y = trans(y);
		top = GET_SCROLL_VIEW_START;
		if (y<top || y+GLYPH_HEIGHT>(top+
			(int)window_get(Fv_foldr.canvas, WIN_HEIGHT)))
		{
			/* File is either before or after view window */

			if ((i = y-GLYPH_HEIGHT)<0)
				i = 0;
#ifndef SV1
			else if (i+window_get(Fv_foldr.canvas, WIN_HEIGHT) > 
				window_get(Fv_foldr.canvas, CANVAS_HEIGHT))
				i = window_get(Fv_foldr.canvas, CANVAS_HEIGHT) -
				     window_get(Fv_foldr.canvas, WIN_HEIGHT);

			i /= Scroll_increment;
#endif
			scrollbar_scroll_to(Fv_foldr.vsbar, i);
		}
	}
	else
		fv_putmsg(TRUE, Fv_message[MEFIND], (int)pattern, 0);
}

/* Return currently selected filename */

fv_getselectedfile()
{
	register FV_FILE **f_p, **l_p;	/* Files array pointers */
	register int i;

	/* Stop at first selection */

	for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles, i=0; f_p != l_p; f_p++, i++)
		if ((*f_p)->selected)
			return(i);

	return(EMPTY);
}

fv_select_all()
{
	register FV_FILE **f_p, **l_p;	/* Files array pointers */
	register int i;			/* Index */


	for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles, i=0; 
		f_p != l_p; f_p++, i++)
		if (!(*f_p)->selected)
		{
			(*f_p)->selected = TRUE;
			fv_reverse(i);
		}

	if (Fv_tselected)
	{
		/* Turn off tree selection.  Only 1 window
		 * can have selected objects...
		 */

		fv_treedeselect();
	}
}

/* Deselect any selected objects */

fv_filedeselect()
{
	register FV_FILE **f_p, **l_p;	/* Files array pointers */
	register int i;			/* Index */


	for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles, i=0; 
		f_p != l_p; f_p++, i++)
		if ((*f_p)->selected)
		{
			(*f_p)->selected = FALSE;
			fv_reverse(i);
		}
}


fv_apply_color(code)
	int code;
{
	register FV_FILE **f_p, **l_p;	/* Files array pointers */
	register int i;			/* Index */
	register FILE *fp;

	for (i=0, f_p=Fv_file, l_p=Fv_file+Fv_nfiles; f_p != l_p; f_p++, i++)
		if ((*f_p)->selected)
		{
			(*f_p)->color = (SBYTE)code;

			pw_rop(Fv_foldr.pw, (*f_p)->x, 
				trans(i),
				GLYPH_WIDTH, GLYPH_HEIGHT, 
				PIX_SRC | PIX_COLOR((*f_p)->color),
			       (*f_p)->icon>EMPTY && Fv_bind[(*f_p)->icon].icon 
				? Fv_bind[(*f_p)->icon].icon
				: Fv_icon[(*f_p)->type], 0, 0);
			fv_reverse(i);
		}


	if (fp = fopen(COLOR_FILE, "w"))
	{
		/* Ensure names are sorted alphabetically */

		if (Fv_sortby != FV_SORTALPHA)
			qsort((char *)Fv_file, Fv_nfiles, sizeof(FV_FILE *),
				Compare_fn[FV_SORTALPHA]);

		for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles; 
			f_p != l_p; f_p++)
			if ((*f_p)->color > BLACK)
			{
				(void)fputc((*f_p)->color+'0', fp);
				fputs((*f_p)->name, fp);
				(void)fputc('\n', fp);
			}
		(void)fclose(fp);

		/* Resort to old sort */

		if (Fv_sortby != FV_SORTALPHA && Fv_sortby != FV_NOSORT)
			qsort((char *)Fv_file, Fv_nfiles, sizeof(FV_FILE *),
				Compare_fn[Fv_sortby]);
	}
	else
		fv_putmsg(TRUE, Fv_message[MECREAT], (int)COLOR_FILE, 0);

}


static
store_colors()			/* Store filename,color pair in ..colors */
{
	static int ncolors = 0;
	char buf[256];
	register FILE *fp;
	register int i;
	register unsigned l;

	/* Free up old color association array */

	i = ncolors;
	while (i)
	{
		i--;
		free(Color_fname[i]);
	}

	/* Read in new... */

	if ((fp=fopen(COLOR_FILE, "r"))==NULL)
		return;

	while (fgets(buf, 255, fp) && ncolors < MAXFILES)
	{
		Color_code[i] = buf[0] - '0';
		l = (unsigned)strlen(buf);
		buf[l-1] = NULL;
		Color_fname[i] = malloc(l);
		(void)strcpy(Color_fname[i], buf+1);
		i++;
	}
	(void)fclose(fp);
	Ncolors = ncolors = i;
}


static int m1[] = { 1, S_IREAD>>0, 'r', '-' };
static int m2[] = { 1, S_IWRITE>>0, 'w', '-' };
static int m3[] = { 3, S_ISUID|(S_IEXEC>>0), 's', S_IEXEC>>0, 'x', S_ISUID, 'S', '-' };
static int m4[] = { 1, S_IREAD>>3, 'r', '-' };
static int m5[] = { 1, S_IWRITE>>3, 'w', '-' };
static int m6[] = { 3, S_ISGID|(S_IEXEC>>3), 's', S_IEXEC>>3, 'x', S_ISGID, 'S', '-' };
static int m7[] = { 1, S_IREAD>>6, 'r', '-' };
static int m8[] = { 1, S_IWRITE>>6, 'w', '-' };
static int m9[] = { 2, S_ISVTX, 't', S_IEXEC>>6, 'x', '-' };

static int *m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

static char *
fmtmode(lp, flags)
	char *lp;
	int flags;
{
	int **mp;

	for (mp = &m[0]; mp < &m[sizeof(m)/sizeof(m[0])]; ) {
		register int *pairp = *mp++;
		register int n = *pairp++;

		while (n-- > 0) {
			if ((flags&*pairp) == *pairp) {
				pairp++;
				break;
			} else
				pairp += 2;
		}			
		*lp++ = *pairp;
	}
	return (lp);
}


static
check_bind(f_p)
	register FV_FILE *f_p;
{
	register int j;
	struct stat fstatus;		/* File status */

	for (j=Fv_nappbind; j<Fv_nbind; j++)
		if (*Fv_bind[j].pattern)
		{
			/* Name regular expression */

			if (fv_match(f_p->name, Fv_bind[j].pattern)==1)
			{
				f_p->icon = j;
				break;
			}
		}
		else
		{
			/* Magic number */

			stat(f_p->name, &fstatus);
			if (fv_magic(f_p->name, Fv_bind[j].magic, &fstatus))
			{
				f_p->icon = j;
				break;
			}
		}
}
