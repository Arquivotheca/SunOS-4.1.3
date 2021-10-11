/*	@(#)teksw_imp.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * A tek subwindow structure
 */

struct teksw {
	Window owner;			/* owner of teksw */
	Window canvas;			/* canvas we're drawing on */
	Cursor cursor;			/* teksw cursor */
	Menu menu;			/* tektool menu */
	Window props;			/* properties sheet */
	Panel_item straps_item;		/* misc straps panel item */
	Panel_item gin_item;		/* gin terminator panel item */
	Panel_item marg_item;		/* margin control panel item */
	Panel_item copy_item;		/* copy control panel item */
	int straps;			/* current tek straps */
	int tty;
	int pty;
	int ttyslot;
	char ptyibuf[128];		/* input buffer from pty */
	char *ptyiwp;			/* read and write buffer ptrs */
	char *ptyirp;
	char ptyobuf[1024];		/* output buffer to pty */
	char *ptyowp;			/* read and write buffer ptrs */
	char *ptyorp;
	caddr_t temu_data;		/* tek emulator client handle */
	int uiflags;
#define ACURSORON	0x01		/* alpha cursor is on the screen */
#define GCURSORON	0x02		/* graphics cursor is on the screen */
#define PAGEFULL	0x04		/* pagefull */
#define RESTARTPTY	0x08		/* restart pty input */
#define BADCURSOR	0xf0		/* bad graphics cursor on screen */
#define BADCURS_XMIN	0x10		/* cursor less that min x */
#define BADCURS_XMAX	0x20		/* cursor more than max x */
#define BADCURS_YMIN	0x40		/* cursor less that min y */
#define BADCURS_YMAX	0x80		/* cursor more than max y */
	enum cursormode cursormode;	/* current cursor mode */
	struct pixwin *pwp;		/* window pixwin */
	struct pr_size imagesize;	/* image size */
	struct pr_size scalesize;	/* addressable pts in image */
	struct pr_pos curpos;		/* current position */
	struct pixfont *curfont;	/* current character font */
	enum vstyle style;		/* current line style (dotted etc.) */
	enum vtype type;		/* current line type (width/store) */
	struct pr_pos alphacursorpos; 	/* displayed alpha cursor position */
	struct pr_size alphacursorsize;	/* displayed alpha cursor size */
	struct pr_pos gfxcursorpos;	/* displayed graphic cursor position */
};

/*
 * Key mappings.
 */
#define KEY_PAGE_RESET	KEY_TOP(1)
#define KEY_COPY	KEY_TOP(2)
#define KEY_RELEASE	KEY_TOP(3)

/*
 * constants
 */
#define USAGE \
	"usage: tektool [-f fontdir] [-s [lcdeag[ce]m[12]] [-[cr] commands]\n"
#define MAXFONTNAMELEN	128	/* font name buffer size */
#define DEFAULTSTRAPS	(DELLOY)/* default tek straps */
#define DEFAULTFONTDIR	"/usr/lib/fonts/tekfonts" /* default font directory */
#define DEFAULTCOPY	"lpr -v"
#define BORDER		3	/* border around view region */

/* max and min addresses leaving room for character overflow */
#define WXSPACE		(tekfont[0]->pf_defaultsize.x + 2)
#define WYSPACE		(tekfont[0]->pf_defaultsize.y + 2)
#define WXMIN(TP)	(0)
#define WYMIN(TP)	(-tekfont[0]->pf_char['A'].pc_home.y + 1)
#define WXMAX(TP)	((TP)->scalesize.x + WXMIN(TP))
#define WYMAX(TP)	((TP)->scalesize.y + WYMIN(TP))

/*
 * Global data.
 */
extern struct pixfont *tekfont[];

/*
 * external tek_ui routines
 */
extern Notify_value teksw_event();
extern Notify_value teksw_pty_input();
extern Notify_value teksw_pty_output();
extern void teksw_props();
