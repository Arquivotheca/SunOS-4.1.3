/*	@(#)diff.h 1.1 92/08/05 SMI */

/*
 *	1986 by Sun Microsystems Inc.
 *
 */

#define	TEXTMARGIN 16

#define MAXGROUP	4096	/* highest value of groupno */
#define MAXGLYPHS	8192	/* max size of glyphlist */

#define MAXGAP	4096

Frame	frame;
Textsw	text1, text2, text3;
Panel	panel;
int	debug;
int	done_button(), apply_button(), reset_button();
int	cmdpanel_event();
int	expert;
int	autoadvance;
int	moveundo;
int	dontrepaint;
int	charwd, charht;
int	readonly;
int	blanks;
FILE	*listfp;
char	*list;
char	*file1;
char	*file2;
char	*file3;
char	*commonfile;
char	**tool_attrs;

struct glyphlist {
	int		g_lineno;
	int		g_groupno;/* consecutive glyphs are grouped together */
	char		g_lchar;
	char		g_rchar;
	Textsw_mark	g_lmark;  /* left */
	Textsw_mark	g_rmark;  /* right */
	Textsw_mark	g_bmark;  /* bottom */
};
struct glyphlist glyphlist[];
int glyphind;		/*next free spot in glyphlist */

linecnt;
int	firstpass;
