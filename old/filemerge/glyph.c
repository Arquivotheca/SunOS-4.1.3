#ifndef lint
static  char sccsid[] = "@(#)glyph.c 1.1 92/08/05 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/scrollbar.h>
#include <suntool/text.h>
#include "diff.h"

Textsw_mark		textsw_add_glyph_on_line();
Textsw_mark		getmark();

static short greater_array[] = {
#include "greater.pr"
};
mpr_static(greater, 16, 16, 1, greater_array);

static short less_array[] = {
#include "less.pr"
};
mpr_static(less, 16, 16, 1, less_array);

static short greaterb_array[] = {
#include "greaterb.pr"
};
mpr_static(greaterb, 16, 16, 1, greaterb_array);

static short lessb_array[] = {
#include "lessb.pr"
};
mpr_static(lessb, 16, 16, 1, lessb_array);

static short plus_array[] = {
#include "plus.pr"
};
mpr_static(plus, 16, 16, 1, plus_array);

static short plusb_array[] = {
#include "plusb.pr"
};
mpr_static(plusb, 16, 16, 1, plusb_array);

static short minus_array[] = {
#include "minus.pr"
};
mpr_static(minus, 16, 16, 1, minus_array);

static short minusb_array[] = {
#include "minusb.pr"
};
mpr_static(minusb, 16, 16, 1, minusb_array);

static short bar_array[] = {
#include "bar.pr"
};
mpr_static(bar, 16, 16, 1, bar_array);

static short barb_array[] = {
#include "barb.pr"
};
mpr_static(barb, 16, 16, 1, barb_array);

#define	EV_GLYPH_DISPLAY	0x0000001
#define	EV_GLYPH_LINE_START	0x0000002
#define	EV_GLYPH_WORD_START	0x0000004
#define	EV_GLYPH_LINE_END	0x0000008

static	struct {
	int	a_lineno;
	char	a_char;
} lglypharr[MAXGLYPHS], rglypharr[MAXGLYPHS];
static	int	la, ra;
static	int	lgl, rgl;

#define NOBOLD 0
#define BOLD 1
#define LEFT 0
#define RIGHT 1
struct pixrect *glyphtable[128][2][2];

removeglyphs()
{
	int	i;
	
	for (i = 0; i < glyphind; i++) {
		my_textsw_remove_glyph(text1, &glyphlist[i].g_lmark, 1);
		my_textsw_remove_glyph(text2, &glyphlist[i].g_rmark, 1);
		my_textsw_remove_glyph(text3, &glyphlist[i].g_bmark, 1);
	}
	glyphind = 0;
	bzero(lglypharr, sizeof(lglypharr));
	bzero(rglypharr, sizeof(rglypharr));
	la = ra = lgl = rgl = 0;
}

addglyph(ch, ln)
{
	static	shown;
	
	if (!commonfile) {
		if (glyphind >= MAXGLYPHS) {
			if (!shown)
				fprintf(stderr,
				    "filemerge: too many differences\n");
			shown = 1;
			return;
		}
		shown = 0;
		glyphlist[glyphind].g_lineno = ln;
		switch(ch) {
		case '<':
			glyphlist[glyphind].g_rchar = '\0';
			glyphlist[glyphind++].g_lchar = ch;
			break;
		case '>':
			glyphlist[glyphind].g_lchar = '\0';
			glyphlist[glyphind++].g_rchar = ch;
			break;
		case '|':
			glyphlist[glyphind].g_lchar = ch;
			glyphlist[glyphind++].g_rchar = ch;
			break;
		}
		return;
	}
	if (firstpass) {
		if (la >= MAXGLYPHS) {
			if (!shown)
				fprintf(stderr,
				    "filemerge: too many differences\n");
			shown = 1;
			return;
		}
		shown = 0;
		switch(ch) {
		case '<':
			lglypharr[la].a_lineno = ln;
			lglypharr[la++].a_char = '+';
			break;
		case '>':
			lglypharr[la].a_lineno = ln;
			lglypharr[la++].a_char = '-';
			break;
		case '|':
			lglypharr[la].a_lineno = ln;
			lglypharr[la++].a_char = '|';
			break;
		}
	}
	else {
		if (ra >= MAXGLYPHS) {
			if (!shown)
				fprintf(stderr,
				    "filemerge: too many differences\n");
			shown = 1;
			return;
		}
		shown = 0;
		switch(ch) {
		case '<':
			rglypharr[ra].a_lineno = ln;
			rglypharr[ra++].a_char = '-';
			break;
		case '>':
			rglypharr[ra].a_lineno = ln;
			rglypharr[ra++].a_char = '+';
			break;
		case '|':
			rglypharr[ra].a_lineno = ln;
			rglypharr[ra++].a_char = '|';
			break;
		}
	}
}

/* assume accessed in order */
lglyph(ln)
{
	for (; lgl < la; lgl++) {
		if (lglypharr[lgl].a_lineno == ln)
			return(lglypharr[lgl].a_char);
		if (lglypharr[lgl].a_lineno > ln)
			return(0);
	}
	return(0);
}

rglyph(ln)
{
	for (; rgl < ra; rgl++) {
		if (rglypharr[rgl].a_lineno == ln)
			return(rglypharr[rgl].a_char);
		if (rglypharr[rgl].a_lineno > ln)
			return(0);
	}
	return(0);
}

paintglyph()			/* also assign group nos */
{
	struct pixrect *pr;
	int	i, line;
	char	ch;
	char	lch, rch, oldlch, oldrch;
	int	group, l;
	
	window_set(frame, FRAME_LABEL,  "adding glyphs....", 0);
	group = -1;
	l = glyphlist[0].g_lineno-1;
	oldlch = ' ';
	oldrch = ' ';
	for (i = 0; i < glyphind; i++) {
		lch = glyphlist[i].g_lchar;
		rch = glyphlist[i].g_rchar;
		line =  glyphlist[i].g_lineno + 1;
		if (line != l+1 || lch != oldlch || rch != oldrch) {
			if (group >= MAXGROUP) {
				fprintf(stderr, "filemerge: too many groups\n");
				return(MAXGROUP-1);
			}
			group++;
		}
		oldlch = lch;
		oldrch = rch;
		l = line;
		glyphlist[i].g_groupno = group;
		if ((ch = glyphlist[i].g_lchar) != '\0') {
			pr = glyphtable[ch][NOBOLD][LEFT];
			glyphlist[i].g_lmark = textsw_add_glyph_on_line(text1,
			    line, pr, PIX_SRC^PIX_DST, 0, 0, 3|8);
		}
		if ((ch = glyphlist[i].g_rchar) != '\0') {
			pr = glyphtable[ch][NOBOLD][RIGHT];
			glyphlist[i].g_rmark = textsw_add_glyph_on_line(text2,
			    line, pr, PIX_SRC^PIX_DST, -pr->pr_size.x, 0, 3);
		}
	}
	return(group + 1);
}

linetogroup(i)
{
	int j;
	
	if (i < 0)
		return (i);
	for (j = 0; j < glyphind; j++) {/* XXX need to optimize this */
		if (glyphlist[j].g_lineno >= i)
			return (glyphlist[j].g_groupno);
	}
	return (glyphlist[glyphind-1].g_groupno);
}

grouptoind(i)
{
	int j;
	
	if (i < 0)
		return (i);
	for (j = 0; j < glyphind; j++) {/* XXX need to optimize this */
		if (glyphlist[j].g_groupno == i)
			return (j);
	}
	fprintf(stderr, "filemerge: Couldn't find group %d\n", i);
	return (0);
}

boldglyph(j, only3)
{
	int	g;

	if (j >= glyphind || j < 0)
		return;
	g = glyphlist[j].g_groupno;
	while (glyphlist[j].g_groupno == g && j < glyphind) {
		doboldglyph(j, only3);
		j++;
	}
}

doboldglyph(j, only3)
{
	struct pixrect *pr;
	int	line;
	int	n;
	char	ch;

	line = glyphlist[j].g_lineno + 1;
	if ((ch = glyphlist[j].g_lchar) != '\0') {
		pr = glyphtable[ch][BOLD][LEFT];
		if (!only3) {
			if (glyphlist[j].g_lmark)
				textsw_set_glyph_pr(text1,
				    glyphlist[j].g_lmark, pr);
			else {
				my_textsw_remove_glyph(text1,
				    &glyphlist[j].g_lmark,1);
				glyphlist[j].g_lmark
				    = textsw_add_glyph_on_line(text1,
				    line, pr, PIX_SRC^PIX_DST, 0, 0, 3|8);
			}
		}
	}
	if ((ch = glyphlist[j].g_rchar) != '\0') {
		pr = glyphtable[ch][BOLD][RIGHT];
		if (!only3) {
			if (glyphlist[j].g_rmark)
				textsw_set_glyph_pr(text2,
				    glyphlist[j].g_rmark, pr);
			else {
				my_textsw_remove_glyph(text2,
				    &glyphlist[j].g_rmark,1);
				glyphlist[j].g_rmark
				    = textsw_add_glyph_on_line(text2,
				    line, pr, PIX_SRC^PIX_DST, -pr->pr_size.x,
				    0, 3);
			}
		}
	}
	if (text3) {
		/* XXX should marks be local to main.c? */
		/*n = textsw_find_mark(text3, marks[glyphlist[j].g_lineno]); */
		n = textsw_find_mark(text3, getmark(j));
		line = index_to_line(text3, n) + 1;
		if (glyphlist[j].g_bmark == 0)
			glyphlist[j].g_bmark = textsw_add_glyph_on_line(text3,
			    line, pr, PIX_SRC^PIX_DST, -pr->pr_size.x, 0, 3);
	}
}

unboldglyph(j)
{
	int	g;

	if (j >= glyphind || j < 0)
		return;
	g = glyphlist[j].g_groupno;
	while (glyphlist[j].g_groupno == g && j < glyphind) {
		dounboldglyph(j);
		j++;
	}
}

dounboldglyph(i)
{
	struct pixrect *pr;
	int	line;
	char	ch;

	line = glyphlist[i].g_lineno + 1;
	if ((ch = glyphlist[i].g_lchar) != '\0') {
		pr = glyphtable[ch][NOBOLD][LEFT];
		if (glyphlist[i].g_lmark)
			textsw_set_glyph_pr(text1, glyphlist[i].g_lmark, pr);
		else {
			my_textsw_remove_glyph(text1, &glyphlist[i].g_lmark,1);
			glyphlist[i].g_lmark = textsw_add_glyph_on_line(text1,
			    line, pr, PIX_SRC^PIX_DST, 0, 0, 3|8);
		}
	}
	if ((ch = glyphlist[i].g_rchar) != '\0') {
		pr = glyphtable[ch][NOBOLD][RIGHT];
		if (glyphlist[i].g_rmark)
			textsw_set_glyph_pr(text2, glyphlist[i].g_rmark, pr);
		else {
			my_textsw_remove_glyph(text2, &glyphlist[i].g_rmark,1);
			glyphlist[i].g_rmark = textsw_add_glyph_on_line(text2,
			    line, pr, PIX_SRC^PIX_DST, -pr->pr_size.x, 0, 3);
		}
	}
	if (text3)
		my_textsw_remove_glyph(text3, &glyphlist[i].g_bmark, 1);
}

tmpremoveglyph()
{
	int	g, j;
	
	/* XXX should curbold be local to main? */
	if (glyphind == 0)
		return;
	j = grouptoind(getcurgrpno());
	if (j >= glyphind || j < 0)
		return;
	g = glyphlist[j].g_groupno;
	while (glyphlist[j].g_groupno == g && j < glyphind) {
		my_textsw_remove_glyph(text3, &glyphlist[j].g_bmark, 1);
		j++;
	}
}

/*
 * this assumes that add_glyph_on_line always returns a nonzero value,
 * which is true for 3.4
 */
my_textsw_remove_glyph(text, glyphp, flag)
	Textsw		text;
	Textsw_mark	*glyphp;
{
	if (*glyphp) {
		textsw_remove_glyph(text, *glyphp, flag);
		*glyphp = NULL;
	}
}

initglyphtable()
{
	glyphtable['>'][NOBOLD][LEFT] = &greater;
	glyphtable['>'][NOBOLD][RIGHT] = &greater;
	glyphtable['>'][BOLD][LEFT] = &greaterb;
	glyphtable['>'][BOLD][RIGHT] = &greaterb;

	glyphtable['<'][NOBOLD][LEFT] = &less;
	glyphtable['<'][NOBOLD][RIGHT] = &less;
	glyphtable['<'][BOLD][LEFT] = &lessb;
	glyphtable['<'][BOLD][RIGHT] = &lessb;

	glyphtable['+'][NOBOLD][LEFT] = &plus;
	glyphtable['+'][NOBOLD][RIGHT] = &plus;
	glyphtable['+'][BOLD][LEFT] = &plusb;
	glyphtable['+'][BOLD][RIGHT] = &plusb;

	glyphtable['-'][NOBOLD][LEFT] = &minus;
	glyphtable['-'][NOBOLD][RIGHT] = &minus;
	glyphtable['-'][BOLD][LEFT] = &minusb;
	glyphtable['-'][BOLD][RIGHT] = &minusb;

	glyphtable['|'][NOBOLD][LEFT] = &bar;
	glyphtable['|'][NOBOLD][RIGHT] = &bar;
	glyphtable['|'][BOLD][LEFT] = &barb;
	glyphtable['|'][BOLD][RIGHT] = &barb;
}
