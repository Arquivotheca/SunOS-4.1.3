#ifndef lint
static  char sccsid[] = "@(#)fv.util.c 1.1 92/07/30 Copyr 1988 Sun Micro";
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
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pwd.h>
#include <grp.h>

#ifdef SV1
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/tty.h>
#include <suntool/textsw.h>
#include <suntool/canvas.h>
#include <suntool/alert.h>
#include <suntool/scrollbar.h>
#else
#include <view2/view2.h>
#include <view2/panel.h>
#include <view2/tty.h>
#include <view2/textsw.h>
#include <view2/canvas.h>
#include <view2/alert.h>
#include <view2/scrollbar.h>
#include <view2/cursor.h>
#include <view2/svrimage.h>
#include <view2/server.h>
#endif

#include "fv.port.h"
#include "fv.h"
#include "fv.extern.h"

static short Wait_image[] = {
#include "busy.cursor"
};
static short Move_image[] = {
#include "move.cursor"
};
static short Copy_image[] = {
#include "copy.cursor"
};

#ifdef SV1
/* Cache existing cursors during busy cursor display */
DEFINE_CURSOR(Frame_cursor, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_CURSOR(Panel_cursor, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_CURSOR(Tree_cursor, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_CURSOR(Folder_cursor, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
static int Frame_fd, Panel_fd, Tree_fd, Folder_fd;

DEFINE_CURSOR_FROM_IMAGE(Busy_cursor, 7, 7, PIX_SRC ^ PIX_DST, Wait_image);
DEFINE_CURSOR_FROM_IMAGE(Move_cursor, 0, 0, PIX_SRC ^ PIX_DST, Move_image);
DEFINE_CURSOR_FROM_IMAGE(Copy_cursor, 0, 0, PIX_SRC ^ PIX_DST, Copy_image);
#else
mpr_static_static(Wait_pr, 16, 16, 1, Wait_image);
mpr_static_static(Move_pr, 16, 16, 1, Move_image);
mpr_static_static(Copy_pr, 16, 16, 1, Copy_image);
static Cursor Busy_cursor;
Cursor Move_cursor;
Cursor Copy_cursor;
static Cursor Frame_cursor, Panel_cursor, Tree_cursor, Folder_cursor;
#endif

static BOOLEAN Message;			/* Message displayed on footer? */
char Fv_hosts[64];			/* Hosts file */

#ifndef lint
fv_putmsg(bell, msg, s1, s2)
	BOOLEAN bell;
	char *msg;
	int s1, s2;
{
	char buffer[MAXPATH+80];

	if (s1 || s2)
	{
		(void)sprintf(buffer, msg, s1, s2);
		msg = buffer;
	}

	if (Fv_running)
	{
#ifdef OPENLOOK
		window_set(Fv_frame, FRAME_MESSAGE, msg, 0);
#else
		panel_set(Fv_errmsg, PANEL_LABEL_STRING, msg, 0);
#endif
		if (bell)
			window_bell(Fv_frame);
		Message = TRUE;
	}
	else if (*msg)
		(void)fprintf(stderr, "fileview: %s\n", msg);
}
#endif


fv_clrmsg()			/* Clear footer message */
{
	if (Message)
	{
#ifdef OPENLOOK
		window_set(Fv_frame, FRAME_MESSAGE, "", 0);
#else
		panel_set(Fv_errmsg, PANEL_LABEL_STRING, "", 0);
#endif
		Message = FALSE;
	}
}


fv_showerr()				/* Display stderr message */
{
	FILE *fp;
	char buf[256];

	if (fp=fopen("/tmp/.err", "r"))
	{
		if (fgets(buf, 255, fp)==NULL)
			(void)strcpy(buf, Fv_message[MECMD]);
		(void)fclose(fp);
		fv_putmsg(TRUE, buf, 0, 0);
	}
}


fv_alert(msg)				/* Prompt user for confirmation */
	char *msg;
{
	return(alert_prompt(Fv_frame, NULL, 
		ALERT_MESSAGE_STRINGS, msg, 0,
		ALERT_BUTTON_YES, "Yes",
		ALERT_BUTTON_NO, "No",
		0) == ALERT_YES);
}


fv_initcursor()
{
#ifdef SV1
	Frame_fd = (int)window_get(Fv_frame, WIN_FD);
	Panel_fd = (int)window_get(Fv_panel, WIN_FD);
	Tree_fd = (int)window_get(Fv_tree.canvas, WIN_FD);
	Folder_fd = (int)window_get(Fv_foldr.canvas, WIN_FD);
#else
	Busy_cursor = vu_create(0, CURSOR,
		CURSOR_IMAGE,	&Wait_pr,
		CURSOR_XHOT,	7,
		CURSOR_YHOT,	7,
		CURSOR_OP,	PIX_SRC | PIX_DST,
		0);
#endif
}


fv_busy_cursor(on)			/* Turn busy cursor on/off */
	BOOLEAN on;
{
	if (on)
	{
#ifdef SV1
		win_getcursor(Frame_fd, &Frame_cursor);
		win_getcursor(Panel_fd, &Panel_cursor);
		win_getcursor(Tree_fd, &Tree_cursor);
		win_getcursor(Folder_fd, &Folder_cursor);
		win_setcursor(Frame_fd, &Busy_cursor);
		win_setcursor(Panel_fd, &Busy_cursor);
		win_setcursor(Tree_fd, &Busy_cursor);
		win_setcursor(Folder_fd, &Busy_cursor);
#else
		Frame_cursor = vu_get(Fv_frame, WIN_CURSOR);
		Panel_cursor = vu_get(Fv_panel, WIN_CURSOR);
		Tree_cursor = vu_get(Fv_tree.canvas, WIN_CURSOR);
		Folder_cursor = vu_get(Fv_foldr.canvas, WIN_CURSOR);
		vu_set(Fv_frame, WIN_CURSOR, Busy_cursor, 0);
		vu_set(Fv_panel, WIN_CURSOR, Busy_cursor, 0);
		vu_set(Fv_tree.canvas, WIN_CURSOR, Busy_cursor, 0);
		vu_set(Fv_foldr.canvas, WIN_CURSOR, Busy_cursor, 0);
#endif
	}
	else
	{
#ifdef SV1
		win_setcursor(Frame_fd, &Frame_cursor);
		win_setcursor(Panel_fd, &Panel_cursor);
		win_setcursor(Tree_fd, &Tree_cursor);
		win_setcursor(Folder_fd, &Folder_cursor);
#else
		vu_set(Fv_frame, WIN_CURSOR, Frame_cursor, 0);
		vu_set(Fv_panel, WIN_CURSOR, Panel_cursor, 0);
		vu_set(Fv_tree.canvas, WIN_CURSOR, Tree_cursor, 0);
		vu_set(Fv_foldr.canvas, WIN_CURSOR, Folder_cursor, 0);
#endif
	}
#ifndef SV1
	vu_set(Fv_frame, FRAME_BUSY, on, 0);
#endif
}

#ifdef SV1
fv_cursor(fd, type)
	int fd;
	BYTE type;
{
	if (type)
	{
		win_getcursor(fd, &Frame_cursor);
		win_setcursor(fd, type == FV_COPYCURSOR ? &Copy_cursor
							: &Move_cursor);
	}
	else
		win_setcursor(fd, &Frame_cursor);
}
#endif


#ifndef SV1
Cursor
fv_get_drag_cursor(copy)
	BOOLEAN copy;
{
	static Cursor fs_cursor=0;
	static Server_image cur_image=0;
	register FV_FILE *f_p;

	/* Build image */
	if (cur_image == 0)
		cur_image = vu_create(0, SERVER_IMAGE,
				VU_WIDTH, GLYPH_WIDTH,
				VU_HEIGHT, GLYPH_HEIGHT,
				SERVER_IMAGE_DEPTH, 1, 0);

	/* Fill in */
	f_p = Fv_file[fv_getselectedfile()];
	pw_rop(cur_image, 0, 0, GLYPH_WIDTH, GLYPH_HEIGHT, PIX_CLR, 0, 0, 0); 

	if (Fv_style & FV_DICON) {
		pw_rop(cur_image, 0, 0, GLYPH_WIDTH, GLYPH_HEIGHT, PIX_SRC, 
		       f_p->icon>EMPTY && Fv_bind[f_p->icon].icon 
				? Fv_bind[f_p->icon].icon
				: Fv_icon[f_p->type],
			0, 0);
		pw_rop(cur_image, 20, 20, 16, 16, PIX_SRC^PIX_DST, 
			copy ? &Copy_pr : &Move_pr, 0, 0); 
	}
	else {
		pw_rop(cur_image, 0, 0, 16, 16, PIX_SRC, 
			Fv_list_icon[f_p->type], 0, 0);
		pw_rop(cur_image, 8, 8, 16, 16, PIX_SRC^PIX_DST, 
			copy ? &Copy_pr : &Move_pr, 0, 0); 
	}


	/* Build or assign cursor image */
	if (fs_cursor==0)
		fs_cursor = vu_create(0, CURSOR,
				CURSOR_IMAGE, cur_image,
				CURSOR_XHOT,  20,
				CURSOR_YHOT,  20,
				CURSOR_OP, PIX_SRC|PIX_DST,
				0);
	else
		vu_set(fs_cursor, CURSOR_IMAGE, cur_image, 0);

	return(fs_cursor);
}
#endif

fv_namestripe()				/* Update the frame label */
{
	char buffer[256];

	(void)sprintf(buffer, "fileview:%s%s %s",
		Fv_filter[0] ? " filter: " : "",
		Fv_filter[0] ? Fv_filter : "",
		Fv_treeview ? Fv_path: "");
	window_set(Fv_frame, FRAME_LABEL, buffer, 0);
}


fv_winright(win)			/* Place popup to right of baseframe */
	Frame win;
{
	int basex, popw, fw, x;

	basex = (int)window_get(Fv_frame, WIN_X);
	fw = (int)window_get(Fv_frame, WIN_WIDTH);
	popw = (int)window_get(win, WIN_WIDTH);

	x = basex+fw;
	if (x+popw > Fv_screenwidth)
		x -= x+popw-Fv_screenwidth;

	/* BUG: Sunview 1.75 doesn't position negative WIN_Y offsets correctly;
	 * so ignore popups partly below window for now.
	 */
	window_set(win, WIN_X, x-basex, WIN_Y, 0, 0);
}


char *
fv_malloc(size)				/* Malloc check */
	unsigned int size;
{
	char *m_p = malloc(size);

	/* XXX See if we can grab memory from someplace else if we fail */

	if (m_p == NULL)
		fv_putmsg(TRUE, Fv_message[MENOMEMORY], 0, 0);
	
	return(m_p);
}


fv_regex_in_string(s_p)			/* Regular expression in string? */
	register char *s_p;
{
	while (*s_p)
	{
		if (*s_p == '*' || *s_p == '[' || *s_p == ']' ||
		    *s_p == '{' || *s_p == '}' || *s_p == '?')
			break;
		s_p++;
	}
	return(*s_p);
}

fv_name_icon(frame, name)
	Frame frame;
	char *name;
{
#ifdef SV1
	struct rect text_rect, *icon_rect;
	Icon edit_icon;
	char buf[255];
	
	(void)sprintf(buf, name);	/* Icon label seems to null out name */
	edit_icon = window_get(frame, FRAME_ICON);

	icon_rect = (Rect *) (LINT_CAST(icon_get(edit_icon, ICON_IMAGE_RECT)));
	
	/* Icon font seems to be null for some reason */
	/* adjust icon text top/height to match font height */
	text_rect.r_height = Fv_font->pf_defaultsize.y;
	text_rect.r_top = icon_rect->r_height - (text_rect.r_height + 2);

	/* Center the icon text */
	text_rect.r_width = strlen(buf)*Fv_font->pf_defaultsize.x;
	if (text_rect.r_width > icon_rect->r_width-4)
	    text_rect.r_width = icon_rect->r_width-4;
	text_rect.r_left = (icon_rect->r_width-text_rect.r_width)/2;

	(void)icon_set(edit_icon,
	    ICON_FONT,		Fv_font,
	    ICON_LABEL,		buf,
	    ICON_LABEL_RECT,	&text_rect,
	    0);

	/* Window_set actually makes a copy of all the icon fields */
	(void)window_set(frame, FRAME_ICON, edit_icon, 0);
#endif
}


fv_init_automount()		/* Is automounter running? */
{
    char hostName[MAXPATH], testPathName[MAXPATH];
    struct stat sbuf1, sbuf2;

    (void)gethostname (hostName, MAXPATH);
    if (!hostName[0])
	return (FALSE);

    (void)sprintf (testPathName, "/net/%s/tmp", hostName);

    if (stat ("/tmp", &sbuf1) == -1)
	return (FALSE);
    if (stat (testPathName, &sbuf2) == -1)
	return (FALSE);

    if (sbuf1.st_ino == sbuf2.st_ino)
    {
	(void)sprintf(Fv_hosts, "%s/.fv_net", Fv_home);
	return (TRUE);
    }
    else
	return (FALSE);
}


fv_getgid(name)				/* Get group id */
	char *name;
{
	struct group *gr;
	if ((gr = getgrnam(name)) == NULL)
		return(-1);
	return(gr->gr_gid);
}

fv_getuid(name)				/* Get owner id */
	char *name;
{
	struct passwd *pw;
	if ((pw = getpwnam(name)) == NULL)
		return(-1);
	return(pw->pw_uid);
}


fv_execit(name, av)			/* Launch application */
	char	*name;
	char	**av;
{
	union wait status;
	int j;
	
 	while (wait3(&status, WNOHANG, (struct rusage *)0) > 0)
		continue;
	if ((j=vfork()) == 0) {
		for (j = getdtablesize(); j > 2; j--)
			(void)close(j);
		j = execvp(name, av);
		exit(127);
	}

	if (j == -1)
		fv_putmsg(TRUE, Fv_message[MEEXEC],
			(int)name, (int)sys_errlist[errno]);
	else
		fv_putmsg(FALSE, Fv_message[MEXEC], (int)name, 0);
}


fv_strlen(str)				/* Return pixel width of string */
	register char *str;
{
	register int width = 0;

	while (*str >= ' ') {
		width += Fv_fontwidth[*str-' '];
		str++;
	}
	return(width);
}
