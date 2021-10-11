#ifndef lint
static  char sccsid[] = "@(#)fv.bind.c 1.1 92/07/30 Copyr 1988 Sun Micro";
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>		/* MAXPATHLEN */
#include <string.h>
#include <strings.h>
#include <ctype.h>

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
#include "fv.extern.h"

static short eq[] = {		/* Equal icon */
#include "equal.icon"
};
mpr_static_static(Equal_pr, 16, 16, 1, eq);

static short pl[] = {		/* Plus icon */
#include "plus.icon"
};
mpr_static_static(Plus_pr, 16, 16, 1, pl);

#define BSZ		255	/* Generic buffer size */
#define BINDGAP	 	10
#define MY_ICON_HEIGHT	17
#define MAXMAGIC 	128

static char *Magic[MAXMAGIC];	/* Magic Entry */
static short Moffset[MAXMAGIC];	/* Offset to description */
static int Nmagic;		/* Number of entries */
static int Mchosen=EMPTY;	/* Magic entry chosen */
static int Magic_box_height;	/* Magic list box height */

static Frame Bind_frame;	/* Bind dialogue window */
static Canvas Bcanvas;		/* Bind canvas */
static Panel Bind_panel;
static PAINTWIN Pw;		/* Bind list box paint handle */
static Scrollbar Bscrollbar;	/* Bind list box scrollbar */
static Canvas Mcanvas;		/* Magic number list box */
static int Listwidth;		/* List box width */
static int Bentry_height;	/* Height of bind item in list box */
static int Bchosen=EMPTY;	/* Bind entry chosen */

static Panel_item Choice_item;	/* Pattern or magic number? */
static Panel_item Pattern_item;	/* String relating to above */
static Panel_item App_item;	/* Application to run */
static Panel_item Icon_item;	/* Icon to display */
static Panel_item Bind_button;
static Panel_item Delete_button;

fv_read_bind()			/* Read in /etc/magic and user's bind entries */
{
	register FILE *fp;	/* File pointer */
	char buf[BSZ];		/* File buffer */
	register char *b_p;	/* Buffer pointer */
	register int i;		/* Index */
	char *magic = "/etc/magic";


	/* Read in /etc/magic stuff */

	if (fp=fopen(magic, "r"))
	{
		while (fgets(buf, BSZ, fp))
		{
			/* Skip comments and continuation lines... */

			if (buf[0] == '#' || buf[0] == '>')
				continue;

			/* Get description; the Magic array will contain these,
			 * the Moffset array contains offsets to start of entry.
			 * Thus we can display magic descriptions quickly, and when
			 * a description is chosen, go back to the entry and parse
			 * it.
			 */
			if ((b_p = strrchr(buf, '\t')) == NULL)
				continue;

			b_p[strlen(b_p)-1] = NULL;	/* Get rid of newline */
			Moffset[Nmagic] = b_p - buf+1;	/* Start of entry */
			if (Nmagic == MAXMAGIC)
			{
				fv_putmsg(TRUE, Fv_message[MEMAGIC], 0, 0);
				break;
			}
			if ((Magic[Nmagic] = fv_malloc((unsigned)strlen(buf)+1))==NULL)
				break;

			(void)strcpy(Magic[Nmagic], buf);
			Magic[Nmagic] += Moffset[Nmagic];
			Nmagic++;
		}
		(void)fclose(fp);
	}


	/* Read bind entries */

	(void)sprintf(buf, "%s/.bind", Fv_home);
	if ((fp=fopen(buf, "r")) == NULL)
		return;

	while (fgets(buf, BSZ, fp))
		if (make_entry(buf, &Fv_bind[Fv_nbind])==0)
		{
			Fv_nbind++;
			if (Fv_nbind == MAXBIND)
				break;
		}
	(void)fclose(fp);

	/* The bind array is in two parts; the first contains the "pattern = icon"
	 * to provide applications for icons.  This list is sorted alphabetically.
	 * The second part contains the "reg expr = [icon +] [application]".  Since
	 * regular expressions are used, this list can't be sorted.  It typically
	 * contains data file definitions.
	 */
	for (i = 0; i<Fv_nbind; i++)
		if (Fv_bind[i].magic || fv_regex_in_string(Fv_bind[i].pattern))
			break;
	Fv_nappbind = i;
}


fv_save_bind()			/* Save user's bind entries */
{
	register FILE *fp;	/* File pointer */
	char buf[BSZ];		/* File buffer */
	register int i, c;	/* Temps */


	(void)sprintf(buf, "%s/.bind", Fv_home);
	if ((fp=fopen(buf, "w")) == NULL)
	{
		fv_putmsg(TRUE, Fv_message[MECREAT], (int)buf, 0);
		return;
	}

	for (i=0; i<Fv_nbind; i++)
	{
		c = *Fv_bind[i].pattern == NULL;
		(void)sprintf(buf, "%s,%s,%s,%s\n",
			c ? "" : Fv_bind[i].pattern,
			c ? Fv_bind[i].magic->str : "",
			Fv_bind[i].application,
			Fv_bind[i].iconfile);
		fputs(buf, fp);
	}

	(void)fclose(fp);
}


fv_bind_object()	/* Create bind popup */
{
	char buf[80];
	Rect *r;
	static void done();
	extern Canvas fv_create_listbox();
	static Notify_value bind_event();
	static void bind_repaint();
	static void bind_button();
	static void unbind_button();
	static void magic_callback();


	if (Bind_frame)
	{
		/* Only one bind popup allowed; bring forward by redisplaying */

		window_set(Bind_frame, WIN_SHOW, TRUE, 0);
		return;
	}

	Bentry_height = GLYPH_HEIGHT+MARGIN;


	if ((Bind_frame = window_create(Fv_frame, 
#ifdef SV1
		FRAME,
		FRAME_LABEL, "fileview: Bind Action to Object",
# ifdef PROTO
		FRAME_CLASS, FRAME_CLASS_COMMAND,
		FRAME_ADJUSTABLE, FALSE,
		FRAME_SHOW_FOOTER, TRUE,
# endif
#else
		FRAME_CMD,
		FRAME_SHOW_FOOTER, TRUE,
		FRAME_CMD_PUSHPIN_IN, TRUE,
		FRAME_LABEL, " Bind Action to Object",
#endif
		FRAME_DONE_PROC, done,
		FRAME_NO_CONFIRM, TRUE,
		FRAME_SHOW_LABEL, TRUE,
		0)) == NULL ||
	    (Bcanvas = window_create(Bind_frame, CANVAS,
		CANVAS_RETAINED, FALSE,
		CANVAS_HEIGHT, Bentry_height*Fv_nbind,
		CANVAS_AUTO_SHRINK, FALSE,
		CANVAS_REPAINT_PROC, bind_repaint,
		WIN_X, 0, 
		WIN_Y, 0,
#ifndef SV1
		WIN_CONSUME_EVENT, MS_LEFT, 
		WIN_CONSUME_EVENT, MS_MIDDLE,
#endif
		WIN_HEIGHT, Bentry_height*3,
		WIN_VERTICAL_SCROLLBAR,
			scrollbar_create(
#ifdef SV1
				SCROLL_PLACEMENT, SCROLL_EAST,
#endif
				SCROLL_LINE_HEIGHT, Bentry_height,
				0),
		0)) == NULL ||
#ifdef SV1
	    (Bind_panel = window_create(Bind_frame, PANEL,
		WIN_BELOW, Bcanvas,
		 0)) == NULL)
#else
	    (Bind_panel = vu_get(Bind_frame, FRAME_CMD_PANEL)) == NULL)
#endif
	{
		fv_putmsg(TRUE, Fv_message[MEWIN], 0, 0);
		return;
	}


#ifdef SV1
	Pw = (PAINTWIN)canvas_pixwin(Bcanvas);
#else
	Pw = (PAINTWIN)vu_get(Bcanvas, CANVAS_NTH_PAINT_WINDOW);
#endif
	Bscrollbar = (Scrollbar)window_get(Bcanvas, WIN_VERTICAL_SCROLLBAR);


	/* Pattern stores both the regular expression as well as
	 * the magic number.
	 */
	Choice_item = panel_create_item(Bind_panel, PANEL_CHOICE,
		PANEL_LABEL_STRING, "Bind",
		PANEL_LABEL_BOLD, TRUE,
		PANEL_CHOICE_STRINGS, "pattern", "magic number:", 0,
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_ITEM_Y, ATTR_ROW(0)-PANEL_CHOICE_BUG,
		0);
	r = (Rect*)panel_get(Choice_item, PANEL_ITEM_RECT);	
	Pattern_item = panel_create_item(Bind_panel, PANEL_TEXT,
		PANEL_VALUE_DISPLAY_LENGTH, 20,
		PANEL_VALUE_X, ATTR_COLS(2)+r->r_width,
		PANEL_ITEM_Y, ATTR_ROW(0),
		0);
	(void)sprintf(buf, "(%dx%d size) Icon Image File:",
		GLYPH_WIDTH, GLYPH_HEIGHT);
	Icon_item = panel_create_item(Bind_panel, PANEL_TEXT,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_LABEL_STRING, buf,
		PANEL_VALUE_DISPLAY_LENGTH, 20,
		PANEL_VALUE_X, ATTR_COLS(2)+r->r_width,
		PANEL_ITEM_Y, ATTR_ROW(1),
		0);
	App_item = panel_create_item(Bind_panel, PANEL_TEXT,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_LABEL_STRING, "Application:",
		PANEL_VALUE_DISPLAY_LENGTH, 20,
		PANEL_VALUE_X, ATTR_COLS(2)+r->r_width,
		PANEL_ITEM_Y, ATTR_ROW(2),
		0);
	Bind_button = panel_create_item(Bind_panel, PANEL_BUTTON,
		MY_BUTTON_IMAGE(Bind_panel, "Bind"),
		PANEL_NOTIFY_PROC, bind_button,
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_ITEM_Y, ATTR_ROW(3),
		0);
	Delete_button = panel_create_item(Bind_panel, PANEL_BUTTON,
		MY_BUTTON_IMAGE(Bind_panel, "Unbind"),
		PANEL_NOTIFY_PROC, unbind_button,
		0);
	if (Nmagic)
		panel_create_item(Bind_panel, PANEL_MESSAGE,
			PANEL_LABEL_BOLD, TRUE,
			PANEL_LABEL_STRING, "Magic Number List:",
			PANEL_ITEM_Y, ATTR_ROW(4),
			PANEL_ITEM_X, ATTR_COL(0),
			0);

	window_fit(Bind_panel);
	Listwidth = (int)window_get(Bind_panel, WIN_WIDTH);
	window_set(Bcanvas, WIN_WIDTH, Listwidth, 0);
	window_set(Bind_panel, WIN_BELOW, Bcanvas, 0);


	if (Nmagic)
	{
		/* Create magic number list box... */

		Magic_box_height = Fv_fontsize.y*6;
		if ((Mcanvas = fv_create_listbox(Bind_frame, Nmagic+1, &Mchosen,
			&Nmagic, Magic, magic_callback)) == NULL)
			return;

		window_set(Mcanvas, WIN_BELOW, Bind_panel, 
			WIN_X, 0,
			WIN_WIDTH, Listwidth,
			WIN_HEIGHT, Magic_box_height,
			0);
	}


	window_fit_height(Bind_panel);
	window_fit(Bind_frame);

#ifdef PROTO
	frame_set_font_size(Bind_frame, 
		(int)window_get(Fv_frame, WIN_FONT_SIZE));
#endif

	fv_winright(Bind_frame);
	window_set(Bind_frame, WIN_SHOW, TRUE, 0);

#ifdef SV1
	notify_interpose_event_func(Bcanvas, bind_event, NOTIFY_SAFE);
#else
	notify_interpose_event_func(Pw, bind_event, NOTIFY_SAFE);
#endif
}


static void
done()			/* Save changes, redisplay folder */
{
	static int compare();
	register int i;

	/* Sort name patterns into ascending order (We want applications 1st)*/
	if (Fv_nbind>1)
		qsort((char *)Fv_bind, Fv_nbind, sizeof(FV_BIND), compare);

	/* Get the correct number of applications bound to icons */
	for (i = 0; i<Fv_nbind; i++)
		if (Fv_bind[i].magic || fv_regex_in_string(Fv_bind[i].pattern))
			break;
	Fv_nappbind = i;

#ifndef SV1
	if (window_get(Bind_frame, FRAME_CMD_PUSHPIN_IN))
#endif
		window_destroy(Bind_frame),
		Bind_frame = NULL,
		fv_lb_delete(Mcanvas);

	fv_save_bind();

	fv_display_folder(FV_BUILD_FOLDER);
}


static int
compare(b1, b2)
	FV_BIND *b1, *b2;
{
	/* Magic numbers and regular expressions have the lowest sort order */
	int r1, r2;			/* Regular expressions? */

	if (b1->magic && b2->magic)
		return(0);
	else if (b1->magic)
		return(1);
	else if (b2->magic)
		return(-1);
	r1 = fv_regex_in_string(b1->pattern);
	r2 = fv_regex_in_string(b2->pattern);
	if (r1 && r2)
		return(0);
	else if (r1)
		return(1);
	else if (r2)
		return(-1);

	return(strcmp(b1->pattern, b2->pattern));
}


static void		/* ARGS IGNORED */
unbind_button()		/* Free selected entry, squeeze bind array, redisplay */	
{
	if (Bchosen>EMPTY)
	{
		register int i = Bchosen;

		free((char *)Fv_bind[i].buf);
		if (Fv_bind[i].magic)
			free((char *)Fv_bind[i].magic);
		Fv_nbind--;

		for (i=Bchosen; i<Fv_nbind; i++)
			Fv_bind[i] = Fv_bind[i+1];

		i = Bentry_height*Bchosen;
		if (Bchosen)
		i -= 2;
		pw_writebackground(Pw, 0, i,
			Listwidth, Bentry_height*(Fv_nbind-Bchosen+1),
			PIX_CLR);
		for (i=Bchosen; i<Fv_nbind; i++)
			draw_bind(i, Pw);
		Bchosen = EMPTY;
	}
	else
		error(TRUE, Fv_message[MESELFIRST]);
}


static void
magic_callback()	/* Display magic number description (from mcanvas) */
{
	panel_set(Pattern_item, PANEL_VALUE, Magic[Mchosen], 0);
}


static Notify_value
bind_event(cnvs, evnt, arg, typ)	/* Handle selection in bind canvas */
	Canvas cnvs;
	Event *evnt;
	Notify_arg arg;
	Notify_event_type typ;
{
	register int i, y;	/* Index, coordinate */

	if ((event_id(evnt) == MS_LEFT || event_id(evnt) == MS_MIDDLE)
		&& event_is_up(evnt))
	{
		y = event_y(evnt)
#ifdef SV1
			+(int)scrollbar_get(window_get(cnvs, WIN_VERTICAL_SCROLLBAR),
			SCROLL_VIEW_START)
#endif
			;

		if ( (i = y / Bentry_height)<Fv_nbind )
		{
			if (event_id(evnt) == MS_MIDDLE)
			{
				if (i == Bchosen)
				{
					y = Bchosen*Bentry_height;
					pw_rop(Pw, 0, Bchosen?y-2:y, Listwidth,
						Bentry_height,
						PIX_NOT(PIX_SRC)^PIX_DST, 
						(Pixrect *)0, 0, 0);
					Bchosen = EMPTY;
				}
				return(notify_next_event_func(cnvs, 
					evnt, arg, typ));
			}

			if (Bchosen!=EMPTY)
			{
				/* Turn off old selection */

				y = Bchosen*Bentry_height;
				pw_rop(Pw, 0, Bchosen?y-2:y, Listwidth, Bentry_height,
					PIX_NOT(PIX_SRC)^PIX_DST, 
					(Pixrect *)0, 0, 0);
			}

			/* Provide feedback to user */

			Bchosen = i;
			y = i*Bentry_height;
			if (i)
				y -= 2;
			pw_rop(Pw, 0, y, Listwidth, Bentry_height,
				PIX_NOT(PIX_SRC)^PIX_DST, 
				(Pixrect *)0, 0, 0);

			if (Fv_bind[i].magic)
				panel_set(Pattern_item, PANEL_VALUE, 
					Fv_bind[i].magic->str, 0);
			else
				panel_set(Pattern_item, PANEL_VALUE, 
					Fv_bind[i].pattern, 0);
			panel_set(Choice_item, PANEL_VALUE,
				Fv_bind[i].magic != NULL, 0);
			panel_set(App_item, PANEL_VALUE, 
				Fv_bind[i].application, 0);
			panel_set(Icon_item, PANEL_VALUE, 
				Fv_bind[i].iconfile, 0);
		}
	}
	return(notify_next_event_func(cnvs, evnt, arg, typ));
}


/*ARGSUSED*/
static void
bind_repaint(cnvs, pw, area)	/* Repaint damaged portions of canvas */
	Canvas cnvs;		/* unused */
	PAINTWIN pw;
	Rectlist *area;
{
	register int i, j;	/* Indeces */
	Rect *rn;		/* Damaged area */
	
	rn = &area->rl_bound;
	i = rn->r_top / Bentry_height;
	if (i >= Fv_nbind)
		return;
	j = (rn->r_top+rn->r_height) / Bentry_height+1;
	if (j >= Fv_nbind)
		j = Fv_nbind-1;

	pw_lock(pw, rn);
	while (i<=j)
	{
		draw_bind(i, pw);
		i++;
	}

	/* Repair damaged section */

	if (Bchosen != EMPTY)
		pw_rop(pw, rn->r_left, Bchosen * Bentry_height,
			rn->r_width, Bentry_height,
			PIX_NOT(PIX_SRC)^PIX_DST, 
			(Pixrect *)0, 0, 0);

	pw_unlock(pw);
}


static
draw_bind(i, pw)		/* Display bind entry */
	register int i;		/* Entry number */
	register PAINTWIN pw;	/* Canvas's pixwin */
{
	register int y, xoffset;
	register char *s_p;
	struct pr_size size;

	/* Display name pattern or magic number */

	y = Bentry_height*i;
	xoffset = MARGIN;
	s_p = Fv_bind[i].pattern[0] ? Fv_bind[i].pattern
				: Fv_bind[i].magic->str;
	pw_text(pw, xoffset, y+(Bentry_height/2), PIX_SRC, Fv_font, s_p);
	size = pf_textwidth(strlen(s_p), Fv_font, s_p);
	xoffset += BINDGAP+size.x;

	pw_rop(pw, xoffset, y+(Bentry_height/2)-8, 16, 16,
		PIX_SRC, &Equal_pr, 0, 0);
	xoffset += 16 + BINDGAP;

	if (*Fv_bind[i].iconfile)
	{
		pw_rop(pw, xoffset, y, GLYPH_WIDTH,
			GLYPH_HEIGHT, PIX_SRC, Fv_bind[i].icon,
			0, 0);
		xoffset += GLYPH_WIDTH + MARGIN;
	}

	if (*Fv_bind[i].application)
	{
		if (*Fv_bind[i].iconfile)
		{
			pw_rop(pw, xoffset, y+(Bentry_height/2)-8, 16, 16,
				PIX_SRC, &Plus_pr, 0, 0);
			xoffset += 17 + BINDGAP;
		}
		pw_text(pw, xoffset, y+(Bentry_height/2), PIX_SRC,
			Fv_font, Fv_bind[i].application);
	}
}


static void		/* ARGS IGNORED */
bind_button()		/* Sanity check, and request a bind entry */
{
	char *pattern, *applic, *icon;
	char buf[256];
	int choice;


	if (Fv_nbind >= MAXBIND)
	{
		error(TRUE, Fv_message[MEBINDENT]);
		return;
	}

	pattern = (char *)panel_get(Pattern_item, PANEL_VALUE);
	if (strchr(pattern, '/'))
	{
		error(TRUE, Fv_message[MEPATTERN]);
		return;
	}
	applic = (char *)panel_get(App_item, PANEL_VALUE);
	icon = (char *)panel_get(Icon_item, PANEL_VALUE);

	if (*pattern == NULL)
		error(TRUE, Fv_message[MEMUSTENTER1]);
	else if (*icon == NULL)
		error(TRUE, Fv_message[MEMUSTENTER2]);
	else
	{
		choice = (int)panel_get(Choice_item, PANEL_VALUE);
		(void)sprintf(buf, "%s,%s,%s,%s", choice?"":pattern,
			choice?pattern:"", applic, icon);
		if (make_entry(buf, &Fv_bind[Fv_nbind]) == 0)
		{
			/* Make the canvas bigger, scroll to new entry */
			window_set(Bcanvas, CANVAS_HEIGHT, 
				(Fv_nbind+2)*Bentry_height, 0);
			scrollbar_scroll_to(Bscrollbar, Fv_nbind*GLYPH_WIDTH);
			draw_bind(Fv_nbind, Pw);
			Fv_nbind++;
		}
	}
}


static
make_entry(buf, bind) 		/* Create a bind entry */
	char *buf;		/* Pattern, magic, app, icon */
	register FV_BIND *bind;	/* Bind structure */
{
	register int j;			/* Index */
	register char *b_p, *s_p;	/* Temp pointers */
	char errmsg[80];		/* load icon error message */
	static FV_MAGIC *make_magic();


	if ((bind->buf=fv_malloc((unsigned)strlen(buf)+1))==NULL)
		return(-1);

	(void)strcpy(bind->buf, buf);

	/* Pattern */
	b_p = bind->buf;
	s_p = b_p;
	while (*b_p && *b_p != ',')
		b_p++;
	*b_p = NULL;
	bind->pattern = s_p;

	/* Magic number */
	s_p = ++b_p;
	while (*b_p && *b_p != ',')
		b_p++;
	*b_p = NULL;

	bind->magic = NULL;
	if (*s_p)
	{
		/* Match s_p with magic description... */

		for (j=0; j<Nmagic; j++)
		{
			if (strcmp(Magic[j], s_p) == 0)
			{
				if ((bind->magic = make_magic(Magic[j]-Moffset[j]))==NULL)
					return(-1);
				bind->magic->str = s_p;
				break;
			}
		}
		if (j==Nmagic)
		{
			error(TRUE, "Unknown magic number");
			return(-1);
		}
	}

	/* Application */
	s_p = ++b_p;
	while (*b_p && *b_p != ',')
		b_p++;
	*b_p = NULL;
	bind->application = s_p;

	/* Icon */
	s_p = ++b_p;
	while (*b_p && *b_p != '\n')
		b_p++;
	*b_p = NULL;
	if (*(bind->iconfile = s_p) == NULL)
	{
		error(TRUE, "No icon");
		return(-1);
	}

	if (*s_p)
	{
		if ((bind->icon = (Pixrect *)icon_load_mpr(s_p, errmsg)) == NULL)
		{
			error(TRUE, errmsg);
			if (bind->magic)
				free((char *)bind->magic);
			return(-1);
		}
	}
	else
		bind->icon = NULL;
	
	return(0);
}

/* This stuff is based on the magic number code in file(1)... */

#define FBSZ	64		/* Read this number of bytes... */

#define	NTYPES	4		/* Number of operand types */

#define	LONG	0
#define	STR	1
#define	ABYTE	2
#define	SHORT	3

#define	NOPS	5		/* Number of operators */

#define	EQ	0
#define	GT	1
#define	LT	2
#define	STRC	3		/* ...string compare */
#define	ANY	4
#define	SUB	64		/* ...or'ed in */

fv_magic(fname, magic, fstatus)	/* Match filename with magic number description */
	char *fname;		/* Candidate */
	FV_MAGIC *magic;	/* Magic number description */
	struct stat *fstatus;	/* Candidate's file status */
{
	int result=FALSE;	/* Match? */
	int fd;			/* Candidate's file descriptor */
	struct utimbuf {	/* Reset time after open & read */
		time_t	actime;
		time_t	modtime;
	} utb;
	char buf[FBSZ];		/* First n bytes of file */


	if (fd=open(fname, 0))
	{
		if (read(fd, buf, FBSZ))
			result = check_magic(buf, magic);

		utb.actime = fstatus->st_atime;
		utb.modtime = fstatus->st_mtime;
		(void)utime(fname, &utb);
		(void)close(fd);
	}
	return(result);
}


static
check_magic(buf, magic)
	char *buf;		/* First n bytes of file */
	register FV_MAGIC *magic;/* Magic number description */
{
	register char *p;	/* Temp pointer */
	long val;		/* Numeric value */

	p = &buf[magic->off];

	switch(magic->type)
	{
	case LONG:
		val = (*(long *) p);
		break;

	case STR:
		return(strncmp(p, magic->value.str, strlen(magic->value.str))==0);

	case ABYTE:
		val = (long)(*(unsigned char *) p);
		break;

	case SHORT:
		val = (long)(*(unsigned short *) p);
		break;
	}

	if (magic->mask)
		val &= magic->mask;

	switch (magic->opcode & ~SUB)
	{
	case EQ:
		return(val == magic->value.num);

	case GT:
		return(val <= magic->value.num);

	case LT:
		return(val >= magic->value.num);
	}

	return(FALSE);
}


static FV_MAGIC *
make_magic(buf)
	char *buf;		/* Raw magic number description */
{
	static char *types[NTYPES] = { "long", "string", "byte", "short" };
	static char ops[NOPS] = { '=', '>', '<', '=', 'x' };
	register char *p, *p2, *p3;/* Temp pointer */
	int i;			/* Index */
	FV_MAGIC *magic;	/* Magic number description */
	char *e_p;		/* Error message pointer */
	char *getstr();
	long atolo();
	

	if ((magic=(FV_MAGIC *)fv_malloc(sizeof(FV_MAGIC)))==NULL)
		return(NULL);

	p = buf;

	p2 = strchr(p, '\t');	/* OFFSET... */
	if (!p2)
	{
		e_p = "No tab after offset";
		goto bad;
	}
	magic->off = atoi(p);

	while(*p2 == '\t')	/* TYPE... */
		p2++;
	p = p2;
	p2 = strchr(p, '\t');
	if(p2 == NULL)
	{
		e_p = "No tab after type";
		goto bad;
	}
	*p2++ = NULL;
	p3 = strchr(p, '&');	/* MASK... */
	if (p3)
	{
		*p3++ = NULL;
		magic->mask = atoi(p3);
	} else
		magic->mask = 0L;

	for (i = 0; i < NTYPES; i++)
		if (strcmp(p, types[i]) == 0)
			break;
	if (i==NTYPES)
	{
		e_p = "Illegal type";
		goto bad;
	}
	magic->type = i;

	*(p2-1) = '\t';		/* Put tab back */
	while(*p2 == '\t')	/* TYPE... */
		p2++;
	p = p2;			/* OP VALUE... */
	p2 = strchr(p, '\t');
	if (!p2)
	{
		e_p = "No tab after value";
		goto bad;
	}
	*p2 = NULL;
	if (magic->type != STR)
	{
		/* Get operator; missing op is assumed to be '=' */

		for (i = 0; i < NOPS; i++)
			if (*p == ops[i])
			{
				magic->opcode = i;
				p++;
				break;
			}
		if (i==NOPS)
			magic->opcode = EQ;
	}

	if (magic->opcode != ANY) {
		if (magic->type == STR)
		{
			if ((magic->value.str = getstr(p))==NULL)
			{
				e_p = Fv_message[MENOMEMORY];
				goto bad;
			}
		}
		else
			magic->value.num = atolo(p);
	}
	*p2 = '\t';


	return(magic);

bad:
	error(TRUE, e_p);
	if (magic)
		free((char *)magic);
	return(NULL);
}


static char *
getstr(s)
	register char *s;
{
	static char *store;
	register char *p;
	register char c;
	register int val;

	if ((store = fv_malloc((unsigned)strlen(s) + 1)) == NULL)
		return(NULL);

	p = store;
	while((c = *s++) != '\0') {
		if(c == '\\') {
			switch(c = *s++) {

			case '\0':
				goto out;

			default:
				*p++ = c;
				break;

			case 'n':
				*p++ = '\n';
				break;

			case 'r':
				*p++ = '\r';
				break;

			case 'b':
				*p++ = '\b';
				break;

			case 't':
				*p++ = '\t';
				break;

			case 'f':
				*p++ = '\f';
				break;

			case 'v':
				*p++ = '\v';
				break;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				val = c - '0';
				c = *s++;  /* try for 2 */
				if(c >= '0' && c <= '7') {
					val = (val<<3) | (c - '0');
					c = *s++;  /* try for 3 */
					if(c >= '0' && c <= '7')
						val = (val<<3) | (c-'0');
					else
						--s;
				}
				else
					--s;
				*p++ = val;
				break;
			}
		} else
			*p++ = c;
	}
out:
	*p = '\0';
	return(store);
}


static long
atolo(s)
	register char *s;
{
	register char *fmt = "%ld";
	auto long j = 0L;

	if(*s == '0') {
		s++;
		if(*s == 'x') {
			s++;
			fmt = "%lx";
		} else
			fmt = "%lo";
	}
	(void)sscanf(s, fmt, &j);
	return(j);
}

static 
error(bell, msg)
	BOOLEAN bell;
	char *msg; 
{
#ifdef SV1
# ifdef PROTO
	if (Bind_frame)
	{
		window_set(Bind_frame, FRAME_MESSAGE, msg, 0);
		if (bell)
			window_bell(Bind_frame);
	}
	else
		fv_putmsg(bell, msg, 0, 0);
# else
	fv_putmsg(bell, msg, 0, 0);
# endif
#else
	if (Bind_frame)
	{
		window_set(Bind_frame, FRAME_MESSAGE, msg, 0);
		if (bell)
			window_bell(Bind_frame);
	}
	else
		fv_putmsg(bell, msg, 0, 0);
#endif
}
