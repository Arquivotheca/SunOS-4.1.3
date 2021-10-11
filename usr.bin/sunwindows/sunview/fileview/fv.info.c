#ifndef lint
static  char sccsid[] = "@(#)fv.info.c 1.1 92/07/30 Copyr 1988 Sun Micro";
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
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <mntent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>		/* statfs */
#include <sys/param.h>		/* dbtob */
#include <sys/file.h>

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
#include "fv.colormap.h"

#ifdef SV1
# ifndef PROTO
static short pin_in_array[] = {
#include "pin_in.icon"
};
mpr_static_static(Pin_in, 64, 13, 1, pin_in_array);
static short pin_out_array[] = {
#include "pin_out.icon"
};
mpr_static_static(Pin_out, 64, 13, 1, pin_out_array);
static BOOLEAN Pinned;
# endif
#endif

/* List of file types in case file(1) gives up... */

static char *Typename[] = {
	"unknown type",
	"fifo",			/* S_IFIFO */
	"character special",	/* S_IFCHR */
	"unknown type",
	"directory",		/* S_IFDIR */
	"unknown type",
	"block special",	/* S_IFBLK */
	"unknown type",
	"regular file",		/* S_IFREG */
	"unknown type",
	"symbolic link",	/* S_IFLNK */
	"unknown type",
	"socket"		/* S_IFSOCK */
};

static	unsigned int Togglemask[] = {
	S_IREAD, S_IWRITE, S_IEXEC,
	S_IREAD>>3, S_IWRITE>>3, S_IEXEC>>3,
	S_IREAD>>6, S_IWRITE>>6, S_IEXEC>>6
};

static char *Label[] = {		/* Panel item labels */
	"Name:", "Owner:", "Group:",
	"Size:", 
	"Last Modified:",
	"Last Accessed:",
	"Type:",
	"Permissions", "Owner:", "Group:", "World:",
	"Mount Point:", "Mounted From:",
	"Free Space:", "Color:"
};

#ifdef SV1
#  define JUSTIFY_COL	16
#  define EXTRA		0
#  define PERM_COL	6
#else
# define JUSTIFY_COL	10
# define EXTRA		6
# define PERM_COL	4
#endif

#define NAME		 0
#define OWNERNAME	 1
#define GROUPNAME	 2
#define BYTESIZE	 3
#define MODTIME		 4
#define ACCTIME		 5
#define TYPE		 6
#define PERM		 7
#define OWNERPERM	 8
#define GROUPPERM	 9
#define OTHERPERM	10
#define MNTPT		11
#define MNTFS		12
#define FREE		13
#define COLOR		14

#define NUMITEMS	15
#define MAXTOGGLES	9

#define COLOR_WIDTH	26
#define COLOR_HEIGHT	17

static int Toggles[MAXTOGGLES];		/* Toggle values */
static Panel_item Toggleitem[MAXTOGGLES];/* Toggles */
static Panel_item Item[NUMITEMS];	/* Changeable items */
static Panel_item Tag_item[NUMITEMS];	/* Label items */

#ifdef SV1
static Panel_item Ok_button;		/* Automatically supplied in View2 */
#endif

static int Owner;			/* We we own this file? */

static Frame Fprops_frame;
static Panel Fprops_panel;
static struct stat Fstatus;		/* Current file status */
static char *Filename;			/* Current file name */
static char *Fname;			/* File name without path */

static Canvas Color_canvas;
static PAINTWIN Color_pw;
extern u_char Fv_red[], Fv_blue[], Fv_green[];
static int Color;			/* Original color */
static int New_color;			/* New color chosen */

static int generic_toggle();
static int props_proc();
static char *addcommas();
static char *docmd();

fv_info_object()	/* Show file properties */
{
	char *f_p;
	char buf[MAXPATH];
	int i;

	New_color = 0;
	if (Fv_tselected)
	{
		fv_getpath(Fv_tselected, buf);
		f_p = buf;
		Color = BLACK;
	}
	else
	{
		if ((i = fv_getselectedfile()) == EMPTY)
			return;
		f_p = Fv_file[i]->name;
		Color = Fv_file[i]->color;
		(void)sprintf(buf, "%s%s%s", Fv_path, Fv_path[1]?"/":"", f_p);
		f_p = buf;
	}

	/* Store name here; (pointers may get changed if the directory is 
	 * changed or removed).
	 */
	if (Filename)
		free(Filename);
	if ((Filename = fv_malloc((unsigned)strlen(f_p)+1)) == NULL)
		return;
	(void)strcpy(Filename, f_p);

	/* Where does the name start? */
	Fname = Filename[1] == 0 ? Filename : strrchr(Filename, '/')+1;

	makepopup();

	if ((int)window_get(Fprops_frame, WIN_SHOW) == FALSE)
		fv_winright(Fprops_frame);
	window_set(Fprops_frame, WIN_SHOW, TRUE, 0);
}


static
makepopup()				/* Make panel items */
{
#ifndef SV1
	static void apply(), reset();
#endif


	if (Fprops_frame == NULL)
	{
		if ((Fprops_frame = window_create(Fv_frame, 
#ifdef SV1
			FRAME,
# ifdef PROTO
			FRAME_CLASS, FRAME_CLASS_COMMAND,
			FRAME_ADJUSTABLE, FALSE,
			FRAME_SHOW_FOOTER, FALSE,
# endif
			FRAME_LABEL, "fileview: File Properties",
#else
			FRAME_PROPS, 
			FRAME_PROPS_APPLY_PROC, apply,
			FRAME_PROPS_RESET_PROC, reset,
			FRAME_LABEL, " File Properties",
			WIN_DYNAMIC_COLOR, TRUE,
#endif
			FRAME_SHOW_LABEL, TRUE,
			FRAME_NO_CONFIRM, TRUE,
				0)) == NULL ||
#ifndef SV1
		    (Fprops_panel = vu_get(Fprops_frame, FRAME_PROPS_PANEL)) == NULL)
#else
		    (Fprops_panel = window_create(Fprops_frame, PANEL, 
				0)) == NULL)
#endif
		{
			fv_putmsg(TRUE, Fv_message[MEWIN], 0, 0);
			return;
		}

#ifdef SV1
		make_ok_button();
# ifdef PROTO
		frame_set_font_size(Fprops_frame, 
			(int)window_get(Fv_frame, WIN_FONT_SIZE));
# endif
#endif

		makepanel(Fprops_panel);
		updatepanel(Filename);
		window_fit(Fprops_panel);
		window_fit(Fprops_frame);
	}
	else
		updatepanel(Filename);
}


static
makepanel(panel)
	register Panel panel;
{
	register int i;
	register char *s_p;

	/* Create changable panel items, ie name, owner, and group... */

	for (i = 0; i < BYTESIZE; i++)
	{
		s_p = Label[i];
		Item[i] = panel_create_item(panel, PANEL_TEXT,
			PANEL_LABEL_BOLD, TRUE,
			PANEL_LABEL_STRING, s_p,
			PANEL_VALUE_STORED_LENGTH, i ? 80 : 255,
			PANEL_VALUE_DISPLAY_LENGTH, 22,
			PANEL_VALUE_Y, ATTR_ROW(i),
			PANEL_VALUE_X, ATTR_COL(JUSTIFY_COL),
			0);
	}

	for (; i < NUMITEMS; i++)
	{
		s_p = Label[i];

		/* Non editable items are not bolded */
		Tag_item[i] = panel_create_item(panel, PANEL_MESSAGE,
			PANEL_LABEL_BOLD,  
#ifdef OPENLOOK
				TRUE,
#else
				i>=PERM && i<=OTHERPERM || i==COLOR,
#endif
			PANEL_LABEL_STRING, s_p,
			PANEL_VALUE_DISPLAY_LENGTH, 35,
			PANEL_VALUE_Y, ATTR_ROW(i),
			PANEL_VALUE_X, ATTR_COL(JUSTIFY_COL),
			0);

		/* We'll want to update everything except the permissions... */
		if (i < PERM || (i >= MNTPT && i < COLOR))
			Item[i] = panel_create_item(panel, PANEL_MESSAGE, 0);
	}

	Item[PERM] = panel_create_item(panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, " Read  Write Execute",
		PANEL_LABEL_BOLD, TRUE,
		PANEL_ITEM_Y, ATTR_ROW(PERM),
		PANEL_ITEM_X, ATTR_COL(JUSTIFY_COL),
		0);

	if (Fv_color)
		create_color_canvas();

	for (i = 0; i < MAXTOGGLES; i++)
		Toggleitem[i] = panel_create_item(panel, 
#ifdef SV1
			PANEL_TOGGLE,
#else
			PANEL_CHECK_BOX,
#endif
			PANEL_CLIENT_DATA, i,
			PANEL_VALUE, Toggles[i],
			PANEL_NOTIFY_PROC, generic_toggle,
			PANEL_ITEM_X, ATTR_COL(JUSTIFY_COL+(i%3)*PERM_COL+2),
			PANEL_ITEM_Y, ATTR_ROW(OWNERPERM+(i/3))-CHECK_OFFSET,
			0);
}


static
updatepanel(name)			/* Update panel items */
	char *name;			/* File name */
{
	char buf[100];			/* Generic buffer */
	register char *str_p;		/* Generic string pointer */
	register int i;			/* Index */
	struct stat sbuf;		/* File status */
	struct statfs fsbuf;		/* File system status */
	struct mntent *mnt, *getmntpt();/* Mount point info */
	int symlink;			/* Does this file have a symlink? */
	extern short Fv_wantdu;		/* Do 'du -s' on directories */
	char *getname();
	char *getgroup();
	

	panel_set(Item[NAME], PANEL_VALUE, Fname, 0);

	/* Get file status to determine symlink... */

	symlink = ((i = lstat(name, &sbuf)) == 0
	    && (sbuf.st_mode & S_IFMT) == S_IFLNK);
	if (symlink)			/* ...try to get real file */
		 i = stat(name, &sbuf);
	if (i == -1)
	{
		fv_putmsg(TRUE, Fv_message[MEREAD],
			(int)name, (int)sys_errlist[errno]);
		return;
	}
	Fstatus = sbuf;

	if ((str_p = getname(sbuf.st_uid)) == 0) {
		(void)sprintf(buf, "%d", sbuf.st_uid);
		str_p = buf;
	}
	panel_set(Item[OWNERNAME], PANEL_VALUE, str_p, 0);

	if ((str_p = getgroup(sbuf.st_gid)) == 0) {
		(void)sprintf(buf, "%d", sbuf.st_gid);
		str_p = buf;
	}
	panel_set(Item[GROUPNAME], PANEL_VALUE, str_p, 0);

	if (Fv_wantdu && ((sbuf.st_mode & S_IFMT) == S_IFDIR))
		(void)sprintf(buf, "%s blocks", docmd("du -s ", name, (struct stat *)0));
	else
		(void)sprintf(buf, "%s bytes", addcommas(sbuf.st_size));
	panel_set(Item[BYTESIZE], PANEL_LABEL_STRING, buf, 0);

	panel_set(Item[MODTIME],
		PANEL_LABEL_STRING, ctime(&sbuf.st_mtime), 0);
	panel_set(Item[ACCTIME],
	    PANEL_LABEL_STRING, ctime(&sbuf.st_atime), 0);

	/* If this is a symlink; get link's type... */
	if (symlink)
	{
		if ((sbuf.st_mode & S_IFMT) == S_IFLNK)
			str_p = "symlink to nowhere";
		else 
		{
			(void)strcpy(buf, "symlink to ");
			i = readlink(name, &buf[11], sizeof(buf));
			buf[11+i] = 0;
			str_p = buf;
		}
	}
	else
	{
		(void)strcpy(buf, docmd("file -L ", name, &sbuf));
		str_p = buf;
	}
	panel_set(Item[TYPE], PANEL_LABEL_STRING, str_p, 0);

	for (i = 0; i < MAXTOGGLES; i++)
	{
		Toggles[i] = ((sbuf.st_mode & Togglemask[i]) != 0);
		panel_set(Toggleitem[i], PANEL_VALUE, Toggles[i], 0);
	}

	panel_set(Tag_item[COLOR], PANEL_SHOW_ITEM, Fv_color, 0);

	if (Fv_color)
		draw_box(Color);

	if ((statfs(Fv_path, &fsbuf)==0) && (mnt = getmntpt(Fv_path)))
	{
		panel_set(Item[MNTPT],
			PANEL_LABEL_STRING, mnt->mnt_dir, 0);
		panel_set(Item[MNTFS],
			PANEL_LABEL_STRING, mnt->mnt_fsname, 0);

		i = fsbuf.f_blocks - (fsbuf.f_bfree - fsbuf.f_bavail);
		(void)sprintf(buf, "%s kbytes (%d%%)",
			addcommas((fsbuf.f_bavail*fsbuf.f_bsize)/1024),
			100-(100*(i - fsbuf.f_bavail))/i);
		panel_set(Item[FREE], PANEL_LABEL_STRING, buf, 0);
		freemnt(mnt);
	}

	/* If you don't own the file, you can't change most properties.
	 * But if you can write on the folder, you can change the name.
	 */

	Owner = getuid();
	Owner = Owner == sbuf.st_uid || Owner == 0;

#ifdef OPENLOOK
# ifdef SV1
	panel_set(Ok_button, PANEL_INACTIVE, !Owner, 0);
# endif
	for (i = 0; i < MAXTOGGLES; i++)
		panel_set(Toggleitem[i], PANEL_INACTIVE, !Owner, 0);
#endif
}


static void
apply()
{
	props_proc(FRAME_PROPS_SET);
}

static void
reset()
{
	props_proc(FRAME_PROPS_RESET);
}

static
props_proc(op)
	short op;
{
	int num;
	char *str1;
	char newname[MAXPATH];
	int error = FALSE;			/* Do we have an error? */
	BOOLEAN changed = FALSE;		/* Properties changed? */
	BOOLEAN stay_up;			/* Pinned window? */
	

	if (op == FRAME_PROPS_RESET)
	{
		updatepanel(Filename);
		return;
	}

	if (access(Filename, F_OK) == -1)
	{
		fv_putmsg(TRUE, Fv_message[MEFIND], (int)Filename, 0);
		return;
	}

	num = toggletonum();
	if ((Fstatus.st_mode & ~S_IFMT) != num)
	{
		/* Change permissions... */

		if (chmod(Filename, num) == -1)
		{
			fv_putmsg(TRUE, sys_errlist[errno], 0, 0);
			error = TRUE;
		}
		else
			changed = TRUE;
	}


	str1 = (char *)panel_get_value(Item[NAME]);
	if (strcmp(str1, Fname))
	{
		/* Change name... */

		if (strchr(str1, '/')) {
			fv_putmsg(TRUE, Fv_message[MECHAR], '/', 0);
			return;
		}
		(void)strncpy(newname, Filename, Fname - Filename);
		newname[Fname-Filename] = NULL;
		(void)strcat(newname, str1);
		if (rename(Filename, newname) == -1)
		{
			fv_putmsg(TRUE, sys_errlist[errno], 0, 0);
			error = TRUE;
		}
		else
			changed = TRUE;
	}
	else
		newname[0] = 0;

	if ((num = fv_getuid((char *)panel_get_value(Item[OWNERNAME]))) == -1)
		fv_putmsg(TRUE, "Unknown owner", 0, 0);
	else if (num != Fstatus.st_uid)
	{
		/* Change owner... */

		if (chown(Filename, num, -1) == -1)
		{
			fv_putmsg(TRUE, sys_errlist[errno], 0, 0);
			error = TRUE;
		}
		else
			changed = Fv_style & FV_DOWNER;
	}

	if ((num = fv_getgid((char *)panel_get_value(Item[GROUPNAME]))) == -1)
		fv_putmsg(TRUE, "Unknown group", 0, 0);
	else if (num != Fstatus.st_gid)
	{
		/* Change group... */

		if (chown(Filename, -1, num) == -1)
		{
			fv_putmsg(TRUE, sys_errlist[errno], 0, 0);
			error = TRUE;
		}
		else
			changed = Fv_style & FV_DGROUP;
	}

	if (New_color && Color != New_color)
		fv_apply_color(New_color);

	if (!error)
	{
		if (changed)
			fv_display_folder(FV_BUILD_FOLDER);
#ifdef OPENLOOK
		stay_up = (BOOLEAN)window_get(Fprops_frame, FRAME_PROPS_PUSHPIN_IN);
#else
# ifdef SV1
		stay_up = Pinned;
# endif
#endif
		if (!stay_up)
			window_set(Fprops_frame, WIN_SHOW, FALSE, 0);
	}
}


/*ARGSUSED*/
static
generic_toggle(item, value, event)
	Panel_item item;
	Event *event;			/* unused */
{
	Toggles[(int)panel_get(item, PANEL_CLIENT_DATA)] = value;
}


static
toggletonum()
{
	int bit, i, l;
	
	l = 0;
	bit = S_IREAD;
	for (i = 0; i < MAXTOGGLES; i++) {
		if (Toggles[i])
			l = l | bit;
		bit >>= 1;
	}
	return(l);
}


static char *
addcommas(k)
	off_t k;
{
	static char buf[20];
	int i, j;
	char *p, *q, tmp;
	
	j = i = 0;
	if (k < 10000)
		(void)sprintf(buf, "%d", k);
	else {
		while (k > 0) {
			j++;
			buf[i++] = '0' + (k%10);
			k = k/10;
			if (k > 0 && j == 3) {
				buf[i++] = ',';
				j = 0;
			}
		}
		/* Found string, now reverse in place */
		buf[i] = 0;
		p = buf;
		q = buf+i-1;
		while (p < q) {
			tmp = *p;
			*p = *q;
			*q = tmp;
			p++;
			q--;
		}
	}
	return(buf);
}


static char *
docmd(cmd, name, sbuf)
	char *cmd;
	char *name;
	struct stat *sbuf;
{
	FILE *fp;
	char buf1[MAXPATH], buf2[MAXPATH];
	int ln;
	register char *n_p, *b_p;

	(void)strcpy(buf1, "/usr/bin/");
	(void)strcat(buf1, cmd);
	for (b_p = buf1+strlen(buf1), n_p = name; *n_p;)
	{
		/* Check characters which may be misinterpreted by the
		 * shell.
		 */
		if (*n_p == '*' || *n_p == '&' || *n_p == '?')
		{
			fv_putmsg(TRUE, Fv_message[MECHAR], (int)*n_p, 0);
			return(NULL);
		}
		if (*n_p == ' ')
			*b_p++ = '\\';
		*b_p++ = *n_p++;
	}
	*b_p = NULL;

	fp = popen(buf1, "r");
	if (fp == NULL) {
		fv_putmsg(TRUE, "not enough file descriptors", 0, 0);
		return(NULL);
	}
	buf2[0] = 0;
	while (fgets(buf1, sizeof(buf1), fp)) {
		(void)strcat(buf2, buf1);
	}
	(void)pclose(fp);

	if (sbuf)
	{
		/* Must be file cmd */

		b_p = strchr(buf2, '\t');
		if (b_p == NULL)
			return(Typename[(sbuf->st_mode & S_IFMT) >> 12]);
		while (*b_p == '\t')
			b_p++;
		if ((ln = strlen(b_p)) > 1)	/* Is this necessary? */
			b_p[ln-1] = 0;
		return(b_p);
	}

	b_p = strchr(buf2, '\t');
	if (b_p)
		*b_p = NULL;
	return(buf2);
}

#include <pwd.h>
#include <grp.h>
#include <utmp.h>

struct	utmp utmp;
#define	NMAX	(sizeof (utmp.ut_name))

#undef MAXUID
#define MAXUID	2048
#define MINUID  -2		/* for nfs */
#define MAXGID	300

static int Outrangeuid = EMPTY;		/* Last uid received */
static char Outrangename[NMAX+1];	/* Last name returned */
static int Outrangegid = EMPTY;		/* Last gid received */
static char Outrangegroup[NMAX+1];	/* Last name returned */

char *
getname(uid)
	int uid;
{
	register struct passwd *pw;

	if (uid == Outrangeuid)
		return (Outrangename);
	if (uid < MINUID)
		return (NULL);

	pw = getpwuid(uid);
	if (pw == NULL)
		return (NULL);

	Outrangeuid = uid;
	(void)strncpy(Outrangename, pw->pw_name, NMAX);
	return (Outrangename);
}


char *
getgroup(gid)
	int gid;
{
	register struct group *gr;

	if (gid >= 0 && gid == Outrangegid)
		return (Outrangegroup);
	if (gid < 0)
		return (NULL);
		
	gr = getgrgid(gid);
	if (gr == NULL)
		return (NULL);
		
	Outrangegid = gr->gr_gid;
	(void)strncpy(Outrangegroup, gr->gr_name, NMAX);
	return(Outrangegroup);
}

/*
 * Given a name like /usr/src/etc/foo.c returns the mntent
 * structure for the file system it lives in.
 */
struct mntent *
getmntpt(file)
	char *file;
{
	FILE *mntp;
	struct mntent *mnt, *mntsave;
	struct mntent *mntdup();

	if ((mntp = setmntent(MOUNTED, "r")) == 0)
		return(NULL);

	/* Get the filesystem that's mounted at the lowest point in the
	 * path.  Malloc it. 
	 */
	mntsave = NULL;
	while (mnt = getmntent(mntp))
	{
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
		    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0)
			continue;
		if (prefix(mnt->mnt_dir, file)) {
			if (!mntsave
			   || strlen(mntsave->mnt_dir) <= strlen(mnt->mnt_dir))
			{
				if (mntsave)
					freemnt(mntsave);
				mntsave = mntdup(mnt);
			}
		}
	}
	(void) endmntent(mntp);
	return(mntsave ? mntsave : NULL);
}


static
struct mntent *
mntdup(mnt)
	register struct mntent *mnt;
{
	register struct mntent *new;
	char *xmalloc();

	new = (struct mntent *)xmalloc(sizeof(*new));

	new->mnt_fsname = xmalloc((unsigned)strlen(mnt->mnt_fsname) + 1);
	(void) strcpy(new->mnt_fsname, mnt->mnt_fsname);

	new->mnt_dir = xmalloc((unsigned)strlen(mnt->mnt_dir) + 1);
	(void) strcpy(new->mnt_dir, mnt->mnt_dir);

	new->mnt_type = xmalloc((unsigned)strlen(mnt->mnt_type) + 1);
	(void) strcpy(new->mnt_type, mnt->mnt_type);

	new->mnt_opts = xmalloc((unsigned)strlen(mnt->mnt_opts) + 1);
	(void) strcpy(new->mnt_opts, mnt->mnt_opts);

	new->mnt_freq = mnt->mnt_freq;
	new->mnt_passno = mnt->mnt_passno;

	return (new);
}

static char *
xmalloc(size)
	unsigned int size;
{
	register char *ret;
	char *malloc();
	
	if ((ret = fv_malloc(size)) == NULL)
		exit(1);

	return (ret);
}

static
freemnt(mnt)
	struct mntent *mnt;
{
	free(mnt->mnt_fsname);
	free(mnt->mnt_dir);
	free(mnt->mnt_type);
	free(mnt->mnt_opts);
	free((char *)mnt);
}

/* is str1 a prefix of str2? */
prefix(str1, str2)
	register char *str1, *str2;
{
	while (*str1 == *str2++)
		if (*str1++ == 0)
			return(1);
	return(!*str1);
}

#ifdef SV1
static Menu Ok_menu;

static void
ok_button(item, event)
	Panel_item item;
	Event *event;
{
	if (event_is_down(event) && event_id(event) == MS_RIGHT)
		menu_show(Ok_menu, Fprops_panel, event, 0);
	else if (event_is_up(event) && event_id(event) == MS_LEFT)
		apply();
	panel_default_handle_event(item, event);
}

#ifndef PROTO
static Panel_item Pin_item;

static void
pin_proc(item, event)
	Panel_item item;
	Event *event;
{

	if (event_is_up(event) && event_id(event) == MS_LEFT)
	{
		Pinned = !Pinned;
		panel_set(Pin_item, PANEL_LABEL_IMAGE, 
			Pinned ? &Pin_in : &Pin_out, 0);
	}
	panel_default_handle_event(item, event);
}
#endif


static
make_ok_button()
{
	Ok_menu = menu_create(
		MENU_ITEM,
			MENU_STRING, "Apply",
			MENU_NOTIFY_PROC, apply, 0,
		MENU_ITEM,
			MENU_STRING, "Reset",
			MENU_NOTIFY_PROC, reset, 0,
		0);
	Ok_button = panel_create_item(Fprops_panel, MY_MENU(Ok_menu),
		MY_BUTTON_IMAGE(Fprops_panel, "OK"),
#ifndef PROTO
		PANEL_EVENT_PROC, ok_button,
#endif
		0);
#ifndef PROTO
	Pin_item = panel_create_item(Fprops_panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, &Pin_out,
		PANEL_EVENT_PROC, pin_proc,
		0);
#endif
}
#endif

static
create_color_canvas()			/* Hack for color choice */
{
	static Notify_value color_event();
	static void color_repaint();
	Rect *r;

	/* Sunview won't allow colors directly on panels, create color
	 * canvas and place it on top of panel.  It will have the
	 * appearance of a color exclusive choice item.
	 */

	r = (Rect*)panel_get(Tag_item[COLOR], PANEL_ITEM_RECT);
	if ((Color_canvas = window_create(Fprops_frame, CANVAS, 
			CANVAS_REPAINT_PROC, color_repaint,
#ifndef SV1
			WIN_CONSUME_EVENT, MS_LEFT,
#endif
			WIN_WIDTH, 6*COLOR_WIDTH+2,
			WIN_HEIGHT, COLOR_HEIGHT+EXTRA,
			WIN_X, ATTR_COL(JUSTIFY_COL+1),
			WIN_Y, r->r_top, 0)) == NULL)
	{
		fv_putmsg(TRUE, Fv_message[MEWIN], 0, 0);
		return;
	}

	pw_setcmsname(canvas_pixwin(Color_canvas), "FVCOLORMAP");
	pw_putcolormap(canvas_pixwin(Color_canvas), 0, CMS_FVCOLORMAPSIZE,
		Fv_red, Fv_green, Fv_blue);

#ifdef SV1
	Color_pw = canvas_pixwin(Color_canvas);
	notify_interpose_event_func(Color_canvas, color_event, NOTIFY_SAFE);
#else
	Color_pw = (PAINTWIN)vu_get(Color_canvas, CANVAS_NTH_PAINT_WINDOW, 0);
	notify_interpose_event_func(Color_pw, color_event, NOTIFY_SAFE);
#endif
}


static Notify_value
color_event(cnvs, evnt, arg, typ)	/* Process color choice */
	Canvas cnvs;
	Event *evnt;
	Notify_arg arg;
	Notify_event_type typ;
{
	if (event_is_up(evnt) && event_id(evnt) == MS_LEFT && Owner)
	{
		New_color = (event_x(evnt)/COLOR_WIDTH)+1;
#ifdef SV1
		pw_lock(Color_pw, (Rect *)window_get(Color_canvas, WIN_RECT));
#endif
		draw_box(New_color);
#ifdef SV1
		pw_unlock(Color_pw);
#endif
	}
	return(notify_next_event_func(cnvs, evnt, arg, typ));
}


static
draw_box(color)
	SBYTE color;
{
	static SBYTE old_color = EMPTY;
	if (color==0)
		color = 1;
	else
		color--;

	if (old_color != EMPTY)
	{
		box(old_color*COLOR_WIDTH, PIX_CLR, 3);
		box(old_color*COLOR_WIDTH, PIX_SET, 1);
	}
	box(color*COLOR_WIDTH, PIX_SET, 3);
	old_color = color;
}


static
box(x, op, width)
	int x;
	int op;
	int width;
{
	struct pr_brush br;
	br.width = width;
	pw_line(Color_pw, x, 0, x+COLOR_WIDTH, 0, &br, NULL, op);
	pw_line(Color_pw, x+COLOR_WIDTH, 0, x+COLOR_WIDTH, COLOR_HEIGHT-1,
		&br, NULL, op);
	pw_line(Color_pw, x+COLOR_WIDTH, COLOR_HEIGHT-1, x, COLOR_HEIGHT-1,
		&br, NULL, op);
	pw_line(Color_pw, x, COLOR_HEIGHT-1, x, 0, &br, NULL, op);
}

/*ARGSUSED*/
static void
color_repaint(canvas, pw, rlist)
	Canvas canvas;
	PAINTWIN pw;
	Rectlist *rlist;
{
	int i;

	for (i = 1; i<7; i++)
	{
		box((i-1)*COLOR_WIDTH, PIX_SET, 1);
		pw_rop(pw, ((i-1)*COLOR_WIDTH)+3, 3, COLOR_WIDTH-5,
			COLOR_HEIGHT-6, PIX_SRC | PIX_COLOR(i),
			(Pixrect *)0, 0, 0);
	}
}
