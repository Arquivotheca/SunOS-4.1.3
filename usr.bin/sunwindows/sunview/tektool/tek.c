#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tek.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * tek makes the Sun into a Tektronix 4014
 *
 * Author: Steve Kleiman
 */

#include <sys/types.h>

#include <stdio.h>

#include "tek.h"

#undef DEBUG

extern char *calloc();
extern void cfree();

/*
 * constants
 */

#define ADDRBITS	037	/* mask for address bits in a char */
#define EXTRABITS	003	/* mask for extra address bits in a char */
#define LO		2	/* position of low order bits in pos reg */
#define HI		7	/* position of hi order bits in pos reg */
#define XEXTRA		0	/* position of x extra bits in char */
#define YEXTRA		2	/* position of Y extra bit in char */
#define CNTRL		00	/* control characters */
#define HIYX		040	/* hi order x or y addr */
#define LOX		0100	/* low order x addr */
#define LOYE		0140	/* low order y addr or extension bits */
#define SADDR		040	/* high order bits for sending address */
#define STATUS		0241	/* dummy status byte-see 4014 man. pg.3-30 */
#define SGRAPH		010	/* graph mode bit in status byte */
#define SMARGIN2	002	/* margin 2 bit in status byte */
#define NONCHR		0377	/* dummy noncharacter */

#define METACHR		0200	/* lowest meta character */

/*
 * macros
 */

/*
 * update an address from a char
 */
#define CHARTOADDR(r, c, s)	((r & ~(ADDRBITS<<s)) | ((c & ADDRBITS)<<s))

#define EXTRATOADDR(r, c, s)	((r & (~EXTRABITS)) | ((c >> s) & EXTRABITS))

/*
 * make a char from an address
 */
#define ADDRTOCHAR(r, s)	((r>>s) & ADDRBITS)

/*
 * get upper control bit from character
 */
#define CTYPE(c)	((c & ~ADDRBITS) & 0x7F)

/*
 * types and structures
 */

enum tmode {			/* term modes */
	ALPHA, GRAPH, GIN, INCPLOT, PTPLOT, SPPTPLOT
};

typedef char bool;

#define TRUE	1
#define FALSE	0

struct Point {
	int x;
	int y;
};

struct tek {
	int straps;			/* tek4014 straps see tek.h */
	struct Point tekgfxpos;		/* last tek4014 graphics position */
	struct Point tekpos;		/* current tek4014 position */
	u_char extra;		/* last extra byte sent */
	int font;			/* current character font */
	enum tmode mode;		/* tek4014 terminal mode */
	enum vtype vtype;		/* vector type */
	enum vstyle vstyle;		/* vector style */
	int margin;			/* alpha margin x location */
	int intensity;			/* pt plot intensity */
	bool full;			/* page full */
	bool lce;			/* current lce state */
	bool bypass;			/* bypass state see tek4014 manual */
	bool darkvec;			/* dark vector indicator */
	bool got_loy;			/* low y address has been sent */
	caddr_t tdata;			/* token to give to output routines */
};	

/*
 * global data
 */

static struct Point tekfontsize[4] = {
	{TEKFONT0X, TEKFONT0Y},
	{TEKFONT1X, TEKFONT1Y},
	{TEKFONT2X, TEKFONT2Y},
	{TEKFONT3X, TEKFONT3Y},
};

/*
 * internal routines
 */

static void tek_reset();	/* reset terminal */
static void tek_page();		/* clear page */
static void tek_putchar();	/* place alpha character */
static void tek_gfxdecode();	/* decode graphics address chars */
static void tek_spptdecode();	/* special point plot decode */
static void tek_incdecode();	/* inc plot decode */
static void tek_newmode();	/* goto new terminal mode */
static void tek_sendstatus();	/* send terminal status to tty */
static void tek_sendpos();	/* send position to tty */

#ifdef DEBUG
FILE *debug;
#endif

caddr_t
tek_create(tdata, straps)
	caddr_t tdata;
	int straps;
{
	register struct tek *tp;

#ifdef DEBUG
	debug = fopen("debug", "w+");
	/* setbuf(debug, NULL); */
#endif
	if ((tp = (struct tek *) calloc(1, sizeof (struct tek))) == NULL)
		return (NULL);
	tp->tdata = tdata;
	tp->straps = straps;
	tek_reset(tp);
	return ((caddr_t) tp);
}

void
tek_destroy(dp)
{
	register struct tek *tp;

	tp = (struct tek *) dp;
	cfree(tp);
}

void
tek_set_straps(dp, straps)
	caddr_t dp;
	int straps;
{
	register struct tek *tp;

	tp = (struct tek *) dp;
	tp->straps = straps;
	if (tp->straps & LOCAL)
		tp->bypass = FALSE;
}

void
tek_ttyinput(dp, c)
	caddr_t dp;
	register u_char c;
{
	register struct tek *tp;
	bool arm;		/* indicator for next lce (ESC) state */

	tp = (struct tek *) dp;
	c &= 0x7f;
	arm = FALSE;		/* default: reset lce */
	if (CTYPE(c) == CNTRL) {
		switch (c) {
		case NUL:
			if (tp->lce)
				arm = TRUE;
			break;
		case ENQ:
			if (tp->lce) {
				tp->bypass = TRUE;
				if (tp->mode == GIN) {
					tek_sendpos(tp);
				} else {
					tek_sendstatus(tp);
					tek_sendpos(tp);
				}
				tek_newmode(tp, ALPHA);
			}
			break;
		case BEL:
			tp->bypass = FALSE;
			tek_putchar(tp, c);
			break;
		case BS:
		case TAB:
		case VT:
			if ((tp->mode == ALPHA) || tp->lce)
				tek_putchar(tp, c);
			break;
		case LF:
			tp->bypass = FALSE;
			if (tp->lce) {
				arm = TRUE;
			} else {
				if (tp->mode == GRAPH) {
					tp->tekgfxpos.y -= 4;
					if (tp->tekgfxpos.y < 0)
						tp->tekgfxpos.y = 0;
				} else {
					tek_putchar(tp, c);
				}
				if (tp->straps & LFCR) {
					tp->tekgfxpos.x &= ~EXTRABITS;
					tp->tekgfxpos.y &= ~EXTRABITS;
					tp->vtype = VT_NORMAL;
					tp->vstyle = VS_NORMAL;
					tek_displaymode(tp->tdata,
					    tp->vstyle, tp->vtype);
					tek_newmode(tp, ALPHA);
					tek_putchar(tp, CR);
				}
			}
			break;
		case CR:
			tp->bypass = FALSE;
			if (tp->lce) {
				arm = TRUE;
			} else {
				tp->tekgfxpos.x &= ~EXTRABITS;
				tp->tekgfxpos.y &= ~EXTRABITS;
				tp->vtype = VT_NORMAL;
				tp->vstyle = VS_NORMAL;
				tek_displaymode(tp->tdata,
				    tp->vstyle, tp->vtype);
				tek_newmode(tp, ALPHA);
				tek_putchar(tp, c);
				if (tp->straps & CRLF)
					tek_putchar(tp, LF);
			}
			break;
		case SO:
			/* alt character set */
			break;
		case SI:
			tp->bypass = FALSE;
			/* normal character set */
			break;
		case FF:
			if (tp->lce) {
				tp->tekgfxpos.x &= ~EXTRABITS;
				tp->tekgfxpos.y &= ~EXTRABITS;
				tp->bypass = FALSE;
				tek_newmode(tp, ALPHA);
				tek_putchar(tp, c);
			}
			break;
		case ETB:
			if (tp->lce) {
				tek_makecopy(tp->tdata); /* make hard copy */
				tp->bypass = FALSE;
			}
			break;
		case CAN:
			if (tp->lce)
				tp->bypass = TRUE;
			break;
		case SUB:
			if (tp->lce) {
				tp->bypass = TRUE;
				tek_newmode(tp, GIN);
			}
			break;
		case ESC:
			arm = TRUE;
			break;
		case FS:
			if (tp->lce)
				tek_newmode(tp, SPPTPLOT);
			else
				tek_newmode(tp, PTPLOT);
			break;
		case GS:
			tp->bypass = FALSE;
			tek_newmode(tp, GRAPH);
			break;
		case RS:
			tek_newmode(tp, INCPLOT);
			break;
		case US:
			tp->bypass = FALSE;
			tek_newmode(tp, ALPHA);
			break;
		default:
			break;
		}
	} else if (tp->lce) {
		if (c >= '8' && c <= ';') {
			tp->font = c - '8';
			tek_chfont(tp->tdata, c - '8');
		} else if (c == '?') {
			c = 0x7f;
			if (tp->mode != ALPHA)
				goto InterpretAddr;
		} else if (c >= '`' && c <= 'w') {
			switch ((c - '`') >> 3) {
			case 0:
				tp->vtype = VT_NORMAL;
				break;
			case 1:
				tp->vtype = VT_DEFOCUSED;
				break;
			case 2:
				tp->vtype = VT_WRITETHRU;
				break;
			}
			switch ((c - '`') & 0x07) {
			case 0:
			case 5:
			case 6:
			case 7:
				tp->vstyle = VS_NORMAL;
				break;
			case 1:
				tp->vstyle = VS_DOTTED;
				break;
			case 2:
				tp->vstyle = VS_DOTDASH;
				break;
			case 3:
				tp->vstyle = VS_SHORTDASH;
				break;
			case 4:
				tp->vstyle = VS_LONGDASH;
				break;
			}
			tek_displaymode(tp->tdata, tp->vstyle, tp->vtype);
		} else if (c == 0x7f) {
			arm = TRUE;
		}
	} else if (!tp->bypass) {
InterpretAddr:
		switch (tp->mode) {
		case ALPHA:
			tek_putchar(tp, c);
			break;
		case PTPLOT:
		case GRAPH:
			tek_gfxdecode(tp, c);
			break;
		case SPPTPLOT:
			tek_spptdecode(tp, c);
			break;
		case INCPLOT:
			tek_incdecode(tp, c);
			break;
		default:
			break;
		}
	}
	tp->lce = arm;
}

#ifndef DISPLAY_ONLY

void
tek_kbdinput(dp, c)
	caddr_t dp;
	register u_char c;
{
	register struct tek *tp;

	tp = (struct tek *) dp;
	c &= 0xff;
	if (tp->full && c != COPY) {
		tek_pagefull_off(tp->tdata);
		tp->full = 0;
	}
	if (c >= METACHR) {
		switch (c) {
		case RESET:
			tek_reset(tp);
			break;
		case PAGE:
			tek_page(tp);
			break;
		case COPY:
			tek_makecopy(tp->tdata);
			tp->bypass = FALSE;
		case RELEASE:
		default:
			break;
		}
	} else {
		if (!(tp->straps & LOCAL)) {
			tek_ttyoutput(tp->tdata, c);
		}
		if (tp->mode == GIN) {
			tp->bypass = FALSE;
			tek_sendpos(tp);
			tek_newmode(tp, ALPHA);
		} else if ((tp->straps & LOCAL) || (tp->straps & AECHO)) {
			tek_ttyinput((caddr_t) tp, c);
		}
	}
}

void
tek_posinput(dp, x, y)
	caddr_t dp;
	int x, y;
{
	register struct tek *tp;

	tp = (struct tek *) dp;
	if (tp->mode == GIN) {
		tp->tekpos.x = x;
		tp->tekpos.y = y;
		tek_move(tp->tdata, tp->tekpos.x, tp->tekpos.y);
	}
}

#endif

/*******************
*
* internal routines
*
*******************/

static void
tek_putchar(tp, c)
	register struct tek *tp;
	register u_char c;
{
	register struct Point *tfsp;

	c &= 0xff;
	tfsp = &tekfontsize[tp->font];
	if (c == TAB)
		c = SP;
	if (c >= 0x20) {
		tek_char(tp->tdata, c);
		tp->tekpos.x += tfsp->x;
		if (tp->tekpos.x >= TXSIZE) {
			tp->tekpos.y -= tfsp->y;
			if (tp->tekpos.y < 0) {
				/* use other margin */
				if (tp->straps & MARG1) {
					if (tp->straps & ACOPY) {
						tek_makecopy(tp->tdata);
						tek_page(tp);
						tp->full = 0;
					} else if (!tp->full) {
						tek_pagefull_on(tp->tdata);
						tp->full = 1;
					}
					tp->margin = 0;
				} else if (tp->margin == 0) {
					tp->margin = TXSIZE / 2;
				} else {	
					if (tp->straps & MARG2) {
						if (tp->straps & ACOPY) {
							tek_makecopy(tp->tdata);
							tek_page(tp);
							tp->full = 0;
						} else if (!tp->full) {
							tek_pagefull_on(tp->tdata);
							tp->full = 1;
						}
					}
					tp->margin = 0;
				}
				tp->tekpos.y = TYSIZE - 1;
			}
			tp->tekpos.x = tp->margin;
		}
	} else {
		switch (c) {
		case LF:
			tp->tekpos.y -= tfsp->y;
			if (tp->tekpos.y < 0) {
				/* use other margin */
				if (tp->straps & MARG1) {
					if (tp->straps & ACOPY) {
						tek_makecopy(tp->tdata);
						tek_page(tp);
						tp->full = 0;
					} else if (!tp->full) {
						tek_pagefull_on(tp->tdata);
						tp->full = 1;
					}
					tp->margin = 0;
				} else if (tp->margin == 0) {
					tp->margin = TXSIZE / 2;
					if (tp->tekpos.x < TXSIZE / 2)
						tp->tekpos.x += TXSIZE / 2;
				} else {
					if (tp->straps & MARG2) {
						if (tp->straps & ACOPY) {
							tek_makecopy(tp->tdata);
							tek_page(tp);
							tp->full = 0;
						} else if (!tp->full) {
							tek_pagefull_on(tp->tdata);
							tp->full = 1;
						}
					}
					tp->margin = 0;
					if (tp->tekpos.x >= TXSIZE / 2)
						tp->tekpos.x -= TXSIZE / 2;
				}
				tp->tekpos.y = TYSIZE - 1;
			}
			break;
		case CR:
			tp->tekpos.x = tp->margin;
			break;
		case BS:
			tp->tekpos.x -= tfsp->x;
			if (tp->tekpos.x < tp->margin)
				tp->tekpos.x = tp->margin;
			break;
		case VT:
			/* can this back up a margin on the real thing? */
			tp->tekpos.y += tfsp->y;
			if (tp->tekpos.y >= TYSIZE)
				tp->tekpos.y = TYSIZE - 1;
			break;
		case BEL:
			tek_bell(tp->tdata);
			break;
		case FF:
			tek_clearscreen(tp->tdata);
			tp->tekpos.x = 0;
			tp->tekpos.y = TYSIZE - 1;
			tp->margin = 0;
		}
	}
	tek_move(tp->tdata, tp->tekpos.x, tp->tekpos.y);
}

static void
tek_reset(tp)
	register struct tek *tp;
{
	tp->lce = FALSE;
	tp->bypass = FALSE;
	tp->darkvec = TRUE;
	tp->got_loy = FALSE;
	tp->extra = 0;
	tp->tekgfxpos.x = 0;
	tp->tekgfxpos.y = 0;
	tp->vtype = VT_NORMAL;
	tp->vstyle = VS_NORMAL;
	tek_displaymode(tp->tdata, tp->vstyle, tp->vtype);
	tp->margin = 0;
	tp->tekpos.x = 0;
	tp->tekpos.y = TYSIZE - 1;
	tp->font = 0;
	tp->mode = ALPHA;
	tek_move(tp->tdata, tp->tekpos.x, tp->tekpos.y);
	tek_chfont(tp->tdata, 0);
	tek_cursormode(tp->tdata, ALPHACURSOR);
}

static void
tek_page(tp)
	register struct tek *tp;
{

	tp->bypass = FALSE;
	tp->margin = 0;
	tp->vtype = VT_NORMAL;
	tp->vstyle = VS_NORMAL;
	tek_displaymode(tp->tdata, tp->vstyle, tp->vtype);
	tp->tekpos.x = 0;
	tp->tekpos.y = TYSIZE - 1;
	tek_move(tp->tdata, tp->tekpos.x, tp->tekpos.y);
	tek_newmode(tp, ALPHA);
	tek_putchar(tp, FF);
}

/*
 * decode graphics addresses and draw vectors as needed
 */
static void
tek_gfxdecode(tp, c)
	register struct tek *tp;
	register u_char c;
{
	c &= 0x7f;
	switch (CTYPE(c)) {
	case LOX:
		tp->tekgfxpos.x = CHARTOADDR(tp->tekgfxpos.x, c, LO);
		tp->got_loy = FALSE;
		tp->tekpos = tp->tekgfxpos;
		if (tp->darkvec) {
			tp->darkvec = FALSE;
			tek_move(tp->tdata, tp->tekpos.x, tp->tekpos.y);
#ifdef DEBUG
			fprintf(debug,
				"moving to %d,%d\n",
				tp->tekpos.x, tp->tekpos.y
			);
#endif
		} else {
			if (tp->mode == GRAPH) {
				tek_draw(tp->tdata, tp->tekpos.x, tp->tekpos.y);
#ifdef DEBUG
				fprintf(debug,
					"drawing vector to %d,%d\n",
					tp->tekpos.x, tp->tekpos.y
				);
#endif
			} else {
				/* point plot modes */
				tek_move(tp->tdata, tp->tekpos.x, tp->tekpos.y);
				tek_draw(tp->tdata, tp->tekpos.x, tp->tekpos.y);
#ifdef DEBUG
				fprintf(debug,
					"point at %d,%d\n",
					tp->tekpos.x, tp->tekpos.y
				);
#endif
			}
		}
		break;
	case HIYX:
		if (tp->got_loy) {
			tp->tekgfxpos.x = CHARTOADDR(tp->tekgfxpos.x, c, HI);
		} else {
			tp->tekgfxpos.y = CHARTOADDR(tp->tekgfxpos.y, c, HI);
		}
		break;
	case LOYE:
		if ((c == 0x7f) && !(tp->straps & DELLOY))
			break;
		if (tp->got_loy) {
			tp->tekgfxpos.x =
				EXTRATOADDR(tp->tekgfxpos.x, tp->extra, XEXTRA);
			tp->tekgfxpos.y =
				EXTRATOADDR(tp->tekgfxpos.y, tp->extra, YEXTRA);
		}
		tp->tekgfxpos.y = CHARTOADDR(tp->tekgfxpos.y, c, LO);
		tp->got_loy = TRUE;
		tp->extra = c;
		break;
	}
}

/* ARGSUSED */
static void
tek_spptdecode(tp, c)
	register struct tek *tp;
	register u_char c;
{
}

static void
tek_incdecode(tp, c)
	register struct tek *tp;
	register u_char c;
{
#define incpos(X,Y)	tp->tekpos.x += X; tp->tekpos.y += Y;

	switch (c) {
	case ' ':
		tp->darkvec = TRUE;
		return;
	case 'P':
		tp->darkvec = FALSE;
		return;
	case 'D':
		incpos(0, 1);
		break;
	case 'E':
		incpos(1, 1);
		break;
	case 'A':
		incpos(1, 0);
		break;
	case 'I':
		incpos(1, -1);
		break;
	case 'H':
		incpos(0, -1);
		break;
	case 'J':
		incpos(-1, -1);
		break;
	case 'B':
		incpos(-1, 0);
		break;
	case 'F':
		incpos(-1, 1);
		break;
	}
	if (tp->tekpos.x < 0)
		tp->tekpos.x = 0;
	if (tp->tekpos.x >= TXSIZE)
		tp->tekpos.x = TXSIZE - 1;
	if (tp->tekpos.y < 0)
		tp->tekpos.y = 0;
	if (tp->tekpos.y >= TYSIZE)
		tp->tekpos.y = TYSIZE - 1;
	if (tp->darkvec == FALSE) {
		tek_draw(tp->tdata, tp->tekpos.x, tp->tekpos.y);
#ifdef DEBUG
		fprintf(debug,
			"drawing vector to %d,%d\n",
			tp->tekpos.x, tp->tekpos.y
		);
#endif
	} else {
		tek_move(tp->tdata, tp->tekpos.x, tp->tekpos.y);
#ifdef DEBUG
		fprintf(debug,
			"moving to %d,%d\n",
			tp->tekpos.x, tp->tekpos.y
		);
#endif
	}
}

/*
 * enter new terminal mode
 */
static void
tek_newmode(tp, nmode)
	register struct tek *tp;
	register enum tmode nmode;
{
	if (tp->full && nmode != ALPHA) {
		tek_pagefull_off(tp->tdata);
		tp->full = 0;
	}
	switch (nmode) {
	case ALPHA:
		if (tp->mode != ALPHA) {
			tp->margin = 0;	/* this is not strictly right */
		}
		tek_cursormode(tp->tdata, ALPHACURSOR);
		break;
	case GIN:
		tek_cursormode(tp->tdata, GFXCURSOR);
		break;
	case GRAPH:
	case PTPLOT:
	case INCPLOT:
	case SPPTPLOT:
		tek_cursormode(tp->tdata, NOCURSOR);
		if (nmode == INCPLOT)
			tp->darkvec = FALSE;
		else
			tp->darkvec = TRUE;
		break;
	}
	tp->mode = nmode;
}

/*
 * send terminal status
 */
static void
tek_sendstatus(tp)
	register struct tek *tp;
{
	register u_char status;

	if (tp->straps & LOCAL)
		return;
	status = STATUS;
	if (tp->mode == GRAPH)
		status |= SGRAPH;
	if (tp->margin != 0)
		status |= SMARGIN2;
	tek_ttyoutput(tp->tdata, status);
}

/*
 * send position
 */
static void
tek_sendpos(tp)
	register struct tek *tp;
{
#ifndef DISPLAY_ONLY
#ifdef DEBUG
	fprintf(debug, "sending status ");
#endif
	if (tp->straps & LOCAL)
		return;
	tek_ttyoutput(tp->tdata, (SADDR | ADDRTOCHAR(tp->tekpos.x, HI)));
	tek_ttyoutput(tp->tdata, (SADDR | ADDRTOCHAR(tp->tekpos.x, LO)));
	tek_ttyoutput(tp->tdata, (SADDR | ADDRTOCHAR(tp->tekpos.y, HI)));
	tek_ttyoutput(tp->tdata, (SADDR | ADDRTOCHAR(tp->tekpos.y, LO)));
#ifdef DEBUG
	fprintf(debug, " 0x%x", (SADDR | ADDRTOCHAR(tp->tekpos.x, HI)));
	fprintf(debug, " 0x%x", (SADDR | ADDRTOCHAR(tp->tekpos.x, LO)));
	fprintf(debug, " 0x%x", (SADDR | ADDRTOCHAR(tp->tekpos.y, HI)));
	fprintf(debug, " 0x%x", (SADDR | ADDRTOCHAR(tp->tekpos.y, LO)));
#endif
	/*
	 * send GIN terminator sequence be carefull to auto echo CR since this
	 * will clear bypass the above stuff and EOT won't
	 */
	switch (tp->straps & GINTERM) {
	case GINNONE:
		break;
	case GINCR:		/* send CR */
		tek_ttyoutput(tp->tdata, CR);
		if (tp->straps & AECHO)
			tek_ttyinput(tp, CR);
#ifdef DEBUG
		fprintf(debug, " 0x%x ", CR);
#endif
		break;
	case GINCRE:		/* send CR EOT */
		tek_ttyoutput(tp->tdata, CR);
		tek_ttyoutput(tp->tdata, EOT);
		if (tp->straps & AECHO)
			tek_ttyinput(tp, CR);
#ifdef DEBUG
		fprintf(debug, " 0x%x ", CR);
		fprintf(debug, " 0x%x ", EOT);
#endif
		break;
	}
#ifdef DEBUG
	fprintf(debug, "\n");
#endif
#endif
}
