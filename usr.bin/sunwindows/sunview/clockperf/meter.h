/*	@(#)meter.h 1.1 92/07/30 SMI */

/*
 *	Copyright (c) 1985 by Sun Microsystems Inc.
 */

#define	MAXSAVE		60	/* how many data points to save */
#define	PIXELSPERCHAR	6	/* how many pixels per char of sail.r.6 */
#define	NAMEHEIGHT	8	/* pixel height of name in icon*/
#define	BORDER		3	/* border around graph area */
#define	ICONWIDTH	64	/* width of icon, dflt width of open tool */
#define	ICONHEIGHT	48	/* height of icon, dflt height of open tool */
#define	RICONHEIGHT	59	/* height for remote tool and icon */
#define	NUMCHARS	((ICONWIDTH - 5)/PIXELSPERCHAR)	/* chars in an icon */
#define	MAXCHARS	40	/* max number of chars in a meter */

/*
 * Structure describing value to be measured.
 */
struct	meter {
	char	*m_name;	/* name of quantity */
	int	m_maxmax;	/* maximum value of max */
	int	m_minmax;	/* minimum value of max */
	int	m_curmax;	/* current value of max */
	int	m_scale;	/* scale factor */
	int	m_lastval;	/* last value drawn */
	int	m_longave;	/* long (hour hand) average */
	int	m_undercnt;	/* count of times under max */
};

extern	struct	meter *meters;	/* [length] */

extern	struct tool *tool;
extern	struct toolsw *metersw;
extern	struct pixwin *pw;	/* pixwin for strip chart */
extern	struct pixrect *ic_mpr;
extern	struct pixfont *pfont;
extern	int (*old_selected)();
extern	int (*old_sigwinch)();
extern	int wantredisplay;	/* flag set by interrupt handler */
extern	int sampletime;		/* how often to take a sample*/
extern	int *save;		/* save values for redisplay */
#define	save_access(visible, data_point) \
		*(save+((visible)*MAXSAVE)+(data_point))
extern	int saveptr;		/* where I am in save[] */
extern	int dead;		/* is remote machine dead? */
extern	int sick;		/* is remote machine sick? (ie, sending bogus data) */
extern	int visible;		/* which quantity is visible*/
extern	int length;		/* length of meter array */
extern	int remote;		/* is meter remote? */
extern	int width;		/* current width of graph area */
extern	int height;		/* current height of graph area */
extern	char *hostname;		/* name of host to meter */
