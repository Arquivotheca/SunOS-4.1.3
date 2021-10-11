#ifndef lint
static char     sccsid[] = "@(#)util.c 1.1 92/08/05 Copyr 1986 Sun Micro";
#endif

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/text.h>
#include <suntool/panel.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/param.h>		/* MAXPATHLEN (include types.h if removed) */
#include <sys/dir.h>		/* MAXNAMLEN */
#include "diff.h"


static short    wait_image[] = {
#include <images/hglass.cursor>
};
DEFINE_CURSOR_FROM_IMAGE(hourglass, 7, 7, PIX_SRC | PIX_DST, wait_image);

DEFINE_CURSOR(oldcursor1, 0, 0, 0,
	      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_CURSOR(oldcursor2, 0, 0, 0,
	      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_CURSOR(oldcursor3, 0, 0, 0,
	      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_CURSOR(oldcursor4, 0, 0, 0,
	      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_CURSOR(oldcursor5, 0, 0, 0,
	      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

static          oline, ofirst;

static	char	namebuf[100];
static char     current_directory[MAXPATHLEN];
static char	current_filename[MAXNAMLEN];
static          caps_lock_on, was_read_only, read_only, edited;

invalidate(pos)
{
	if (pos >= ofirst)
		return;
	oline = 0;
	ofirst = 0;
}

index_to_line(text, n)
	Textsw	text;
{
	int	first, line, endlinep1, marker;

	line = oline;
	if (n < ofirst) {
		first = ofirst-1;
		while (1) {
			marker = first;
			line--;
			textsw_find_bytes(text, &first, &endlinep1, "\n", 1,1);
			if (first <= n)
				break;
			if (first > marker) {
				endlinep1 = 0;
				break;
			}
		}
		oline = line;
		ofirst = endlinep1;
	}
	else {
		marker = first = ofirst;
		while (1) {
			textsw_find_bytes(text, &first, &endlinep1, "\n", 1,0);
			if (first >= n) {
				oline = line + 1;
				break;
			}
			if (first < marker) {
				oline = 1;
				break;
			}
			marker = first = endlinep1;
			line++;
		}
		ofirst = endlinep1;
	}
	/* 
	 * In order to make the check in invalidate() more likely
	 * to succeed, back up the remembered (line, first) pair
	 * by one.
	 */
	if (oline > 1) {
		oline--;
		ofirst--;
		textsw_find_bytes(text, &ofirst, &endlinep1, "\n", 1,1);
		ofirst = endlinep1;
	}
	return(line);
}

/*
 * put 'line' k lines from top if one of the following 2 cases holds
 * (1) 'line' is currently not visible
 * (2) 'line' is visible but 'line1' is not
 */
my_textsw_possibly_normalize(text, line, line1, k)
	Textsw          text;
{
	int             top, ht;

	textsw_file_lines_visible(text, &top, &ht);
	ht = (int)window_get(text1, WIN_HEIGHT)/charht;
	if (!readonly)
		ht = min((int)window_get(text3, WIN_HEIGHT)/charht, ht);
	if (line < top || top + ht <= line || top + ht <= line1) {
		line -= k;
		if (line < 0)
			line = 0;
		textsw_normalize_view(text,
		    textsw_index_for_file_line(text, line));
	}
}

/*
 * if pa1 is a directory, return pa1/pa2, else return pa1 
 */
char           *
filename(pa1, pa2)
	char           *pa1, *pa2;
{
	register char  *a1, *b1, *a2;

	if (pa1[0] == 0)
		return(pa1);
	a1 = pa1;
	a2 = pa2;
	if (isdir(a1)) {
		b1 = pa1 = (char *) malloc(100);
		while (*b1++ = *a1++);
		b1[-1] = '/';
		a1 = b1;
		while (*a1++ = *a2++)
			if (*a2 && *a2 != '/' && a2[-1] == '/')
				a1 = b1;
	}
	return (pa1);
}


/*
 * if pa1 is a directory, return either pa1/pa2 or pa1/pa3.
 * Otherwise return pa1.
 */
char           *
filename3(pa1, pa2, pa3)
	char           *pa1, *pa2, *pa3;
{
	register char  *a1, *b1, *a2;

	if (pa1[0] == 0)
		return(pa1);
	a1 = pa1;
	if (isdir(pa2))
		a2 = pa3;
	else
		a2 = pa2;
	if (isdir(a1)) {
		b1 = pa1 = (char *) malloc(100);
		while (*b1++ = *a1++);
		b1[-1] = '/';
		a1 = b1;
		while (*a1++ = *a2++)
			if (*a2 && *a2 != '/' && a2[-1] == '/')
				a1 = b1;
	}
	return (pa1);
}

changecursor()
{
	int             fd;

	fd = (int) window_get(panel, WIN_FD);
	win_getcursor(fd, &oldcursor1);
	win_setcursor(fd, &hourglass);

	fd = (int) window_get(text1, WIN_FD);
	win_getcursor(fd, &oldcursor2);
	win_setcursor(fd, &hourglass);

	fd = (int) window_get(text2, WIN_FD);
	win_getcursor(fd, &oldcursor3);
	win_setcursor(fd, &hourglass);

	fd = (int) window_get(frame, WIN_FD);
	win_getcursor(fd, &oldcursor4);
	win_setcursor(fd, &hourglass);

	if (text3) {
		fd = (int) window_get(text3, WIN_FD);
		win_getcursor(fd, &oldcursor5);
		win_setcursor(fd, &hourglass);
	}
}

restorecursor()
{
	int             fd;

	fd = (int) window_get(panel, WIN_FD);
	win_setcursor(fd, &oldcursor1);
	fd = (int) window_get(text1, WIN_FD);
	win_setcursor(fd, &oldcursor2);
	fd = (int) window_get(text2, WIN_FD);
	win_setcursor(fd, &oldcursor3);
	fd = (int) window_get(frame, WIN_FD);
	win_setcursor(fd, &oldcursor4);
	if (text3) {
		fd = (int) window_get(text3, WIN_FD);
		win_setcursor(fd, &oldcursor5);
	}
}

/* this code copied from textedit.c */

text_notify(textsw, attributes)
	Textsw          textsw;
	Attr_avlist     attributes;
{
	int             pass_on = 0, repaint = 0;
	Attr_avlist     attrs;

	was_read_only = read_only;
	for (attrs = attributes; *attrs; attrs = attr_next(attrs)) {
		repaint++;	/* Assume this attribute needs a repaint. */
		switch ((Textsw_action) (*attrs)) {
		case TEXTSW_ACTION_CAPS_LOCK:
			caps_lock_on = (int) attrs[1];
			ATTR_CONSUME(*attrs);
			break;
		case TEXTSW_ACTION_CHANGED_DIRECTORY:
			switch (attrs[1][0]) {
			case '/':
				strcpy(current_directory, attrs[1]);
				break;
			case '.':
				if (attrs[1][1] != '\0')
					(void) getwd(current_directory);
				break;
			case '\0':
				break;
			default:
				strcat(current_directory, "/");
				strcat(current_directory, attrs[1]);
				break;
			}
			ATTR_CONSUME(*attrs);
			break;
		case TEXTSW_ACTION_USING_MEMORY:
			strcpy(current_filename, "(NONE)");
			/*
			 * XXX would be better to disable
			 * reset, but for now do this:
			 */
			textsw_reset(text1, 0, 0);
			textsw_reset(text2, 0, 0);
			glyphind = 0;
			edited = read_only = 0;
			ATTR_CONSUME(*attrs);
			break;
		case TEXTSW_ACTION_LOADED_FILE:
			strcpy(current_filename, attrs[1]);
			strcpy(namebuf, attrs[1]);
			file3 = namebuf;
			edited = read_only = 0;
			break;
		case TEXTSW_ACTION_EDITED_FILE:
			edited = 1;
			ATTR_CONSUME(*attrs);
			break;
		default:
			pass_on = !0;
			repaint--;	/* Above assumption was wrong. */
			break;
		}
	}
	if (pass_on)
		textsw_default_notify(textsw, attributes);
	if (repaint && !dontrepaint)
		putnamestripe();
}

initdirectory()
{
	(void) getwd(current_directory);
}

initnamestripe()
{
	edited = 0;
	strcpy(current_filename, file3);
}

putnamestripe()
{
	char            frame_label[50 + MAXNAMLEN + MAXPATHLEN];

	if (readonly)
		sprintf(frame_label, "%sfilemerge",
			(caps_lock_on) ? "[CAPS] " : "");
	else
		sprintf(frame_label, "%sfilemerge - %s%s, dir: %s",
			(caps_lock_on) ? "[CAPS] " : "",
			current_filename,
			(was_read_only) ? " (read only)"
			: (edited) ? " (edited)" : "",
			current_directory);
	window_set(frame, FRAME_LABEL, frame_label, 0);
}

isdir(name)
	char	*name;
{
	struct stat     stbuf;

	return (stat(name, &stbuf) != -1
	    && ((stbuf.st_mode & S_IFMT) == S_IFDIR));
}

/* 
 * check for existance of name1 and also if
 * name1 is the same as name2
 */
samecheck(name1, name2)
	char	*name1, *name2;
{
	struct	stat	buf1, buf2;
	int	x;	
	
	if ((x = stat(name1, &buf1)) == 0 && stat(name2, &buf2) == 0
	    && buf1.st_dev == buf2.st_dev && buf1.st_ino == buf2.st_ino) {
		error("would overwrite %s", name1);
		return(1);
	}
	if (x != 0) {
		error("%s doesn't exist", name1);
		return(1);
	}
	return(0);
}

adjustposition()
{
	int	totwd, totht, wd, ht, x, y;
	
	if (getscreensize(&totwd, &totht) != 0)
		return;
	x = (int)window_get(frame, WIN_X);
	y = (int)window_get(frame, WIN_Y);
	wd = (int)window_get(frame, WIN_WIDTH);
	ht = (int)window_get(frame, WIN_HEIGHT);
	if (x + wd > totwd)
		window_set(frame, WIN_X, max(totwd - wd, 0), 0);
	if (y + ht > totht)
		window_set(frame, WIN_Y, max(totht - ht, 0), 0);
}

getscreensize(wdp, htp)
	int	*wdp, *htp;
{
	int windowfd;
	static int	first = 1;
	static		struct screen screen;

	if (first == 1) {
		if ((windowfd = open("/dev/win0", O_RDONLY)) < 0)
			return (1);
		win_screenget(windowfd, &screen);
		*wdp = screen.scr_rect.r_width;
		*htp = screen.scr_rect.r_height;
		close(windowfd);
		first = 0;
	} else {
		*wdp = screen.scr_rect.r_width;
		*htp = screen.scr_rect.r_height;
	}
	return(0);
}

/* return true if left mergeable */
leftmerge(i, grp)
	int	i;		/* index into glyphlist */
	int	grp;		/* groupno */
{
	if (glyphlist[i].g_rchar != 0)
		return(0);
	return(commonmerge(i, grp, 1));
}

/* return true if left mergeable */
rightmerge(i, grp)
	int	i;		/* index into glyphlist */
	int	grp;		/* groupno */
{
	int j;

	if (glyphlist[i].g_lchar != 0)
		return(0);
	return(commonmerge(i, grp, 0));
}

commonmerge(i, grp, left)
{
	int j;

	for (j = i; j < glyphind; j++) {
		if (glyphlist[j].g_groupno == grp+1) {
			if (glyphlist[j].g_lineno != glyphlist[j-1].g_lineno+1)
				break;
			if (left && glyphlist[j].g_rchar)
				return(0);
			if (!left && glyphlist[j].g_lchar)
				return(0);
			break;
		}
	}
	for (j = i; j >= 0; j--) {
		if (glyphlist[j].g_groupno == grp-1) {
			if (glyphlist[j].g_lineno != glyphlist[j+1].g_lineno-1)
				return(1);
			if (left && glyphlist[j].g_rchar)
				return(0);
			if (!left && glyphlist[j].g_lchar)
				return(0);
			return(1);
		}
	}
	return(1);
}

char *
expand(name)
	char	*name;
{
	char	buf[200];
	char	*p;	

	if (name == NULL)
		return(NULL);
	expand_path(name, buf);
	p = (char *)malloc(strlen(buf) + 1);
	strcpy(p, buf);
	return(p);
}
