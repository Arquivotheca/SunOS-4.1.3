#ifndef lint
static  char sccsid[] = "@(#)fv.tree.c 1.1 92/07/30 Copyr 1988 Sun Micro";
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
#ifdef OS3
# include <sys/dir.h>
#else
# include <dirent.h>
#endif

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

#define MAXSTACKDEPTH	5			/* How deep folders can layer */
#define THRESHOLD	4

static short Xpos;				/* Tree position global */
static short Xmax, Ymax;			/* Width, height of canvas */
static int Npath;				/* Number of path buttons */
static int Lastevent;				/* Last event seen */
static FV_TNODE *Chosen;			/* Chosen folder (mouse()) */
static FV_TNODE *Lastselected;			/* Last node selected */
static FV_TNODE *Mouse_stack[MAXSTACKDEPTH];	/* Selection in folder stack */
static FV_TNODE *Child;				/* Descendant node */
static BOOLEAN Found_child;			/* Did we find it? */

static FV_TNODE *path_chosen();
static FV_TNODE *mouse();

fv_create_tree_canvas()
{
	static int tree_repaint();
	static Notify_value tree_event();

	Fv_tree.canvas = window_create(Fv_frame, CANVAS,
		CANVAS_AUTO_CLEAR,	FALSE,
		CANVAS_AUTO_SHRINK,	FALSE,
		CANVAS_FAST_MONO,	TRUE,
		CANVAS_RETAINED,	FALSE,
		CANVAS_REPAINT_PROC,	tree_repaint,
		WIN_CONSUME_PICK_EVENTS,
			KEY_LEFT(K_PROPS), KEY_LEFT(K_UNDO), KEY_LEFT(K_PUT),
			KEY_LEFT(K_GET), KEY_LEFT(K_FIND), KEY_LEFT(K_DELETE),
			0,
		0);

#ifdef SV1
	Fv_tree.pw = (PAINTWIN)canvas_pixwin(Fv_tree.canvas);
#else
	Fv_tree.pw = (PAINTWIN)vu_get(Fv_tree.canvas, CANVAS_NTH_PAINT_WINDOW, 0);
#endif

        Fv_tree.r.r_width = (int)window_get(Fv_tree.canvas, WIN_WIDTH);
	Fv_tree.r.r_height = (int)window_get(Fv_tree.canvas, WIN_HEIGHT);
		 
#ifdef SV1
	notify_interpose_event_func(Fv_tree.canvas, tree_event, NOTIFY_SAFE);
#else
	notify_interpose_event_func(Fv_tree.pw, tree_event, NOTIFY_SAFE);
#endif
}


static Notify_value
tree_event(cnvs, evnt, arg, typ)
	Canvas cnvs;
	register Event *evnt;
	Notify_arg arg;
	Notify_event_type typ;
{
	static struct timeval last_click;	/* Last time clicked */
	struct timeval click;			/* This time clicked */
	BOOLEAN children;			/* More children found? */
	int x, y, i;				/* Last coordinates */

	if (event_is_down(evnt))
	if (event_id(evnt) == MS_LEFT)
	{
		/* Clear any message */
		fv_clrmsg();

		/* Single click = selection
		 * Double click = open folder
		 */

		if (Fv_treeview)
#ifdef SV1
			Fv_tselected = mouse(event_x(evnt)+Fv_tree.r.r_left,
				event_y(evnt)+Fv_tree.r.r_top, Fv_thead);
#else
			Fv_tselected = mouse(event_x(evnt), event_y(evnt), Fv_thead);
#endif
		else 
			Fv_tselected = path_chosen(event_x(evnt), event_y(evnt));

		if (Fv_tselected)
		{
			/* Only one window can have selections */

			fv_filedeselect();

			click = evnt->ie_time;
			if (Lastselected == Fv_tselected)
			{
				if (click.tv_sec-last_click.tv_sec<=1)
				{
					if (Fv_treeview)
						fv_open_folder();
					else
					{
						fv_busy_cursor(TRUE);
						Fv_current = Fv_tselected;
						fv_getpath(Fv_tselected, (char *)0);
						chdir(Fv_path);
						(void)strcpy(Fv_openfolder_path, Fv_path);
						fv_drawtree(TRUE);
						fv_display_folder(FV_BUILD_FOLDER);
						fv_busy_cursor(FALSE);
					}
					return(notify_next_event_func(cnvs, evnt,
						arg, typ));
				}
			}
			else
			{
				if (Fv_treeview)
				{
					fv_check_children(Fv_tselected, &children);
					if (children)
						fv_drawtree(TRUE);
					else
					{
						reverse(Lastselected);
						reverse(Fv_tselected);
					}
				}
				else
				{
					reverse(Lastselected);
					reverse(Fv_tselected);
				}
			}
			last_click = click;
			Lastselected = Fv_tselected;

#ifdef SV1
			if (Fv_treeview)
			{
				/* Drag folder? */
				x = event_x(evnt);
				y = event_y(evnt);
				while (window_read_event(cnvs, evnt) != -1 &&
					event_id(evnt) != MS_LEFT)
					if (((i = x-event_x(evnt))>THRESHOLD) ||
					      i<-THRESHOLD ||
					    ((i = y-event_y(evnt))>THRESHOLD) ||
					      i<-THRESHOLD)
					{
						drag(cnvs, x, y);
						break;
					}
			}
#endif
		}
		else
		{
			/* Clicking on white space clears selection */

			fv_treedeselect();
			fv_filedeselect();
		}
	}
	else
		if (event_id(evnt) == MS_RIGHT)
			fv_viewmenu(cnvs, evnt, TRUE);

	fv_check_keys(evnt);

	Lastevent = event_id(evnt);
	return(notify_next_event_func(cnvs,evnt,arg,typ));
}


/*ARGSUSED*/
static void
tree_repaint(canvas, pw, area)			/* Repaint tree canvas */
	Canvas canvas;
	PAINTWIN pw;
	Rectlist *area;
{
	register Rect *rn;
	static BOOLEAN lastview=TRUE;		/* Last view state */

	Fv_tree.r.r_height = (int)window_get(canvas, WIN_HEIGHT);
	Fv_tree.r.r_width = (int)window_get(canvas, WIN_WIDTH);

	rn = &area->rl_bound;

#ifdef SV1
	if (Fv_treeview && Lastevent == SCROLL_REQUEST)
	{
		/* The tree is visible; handle scrolling or repainting */

		if (Fv_tree.r.r_left != rn->r_left)
		{
			/* Horizontal scroll */

			if (rn->r_width+SCROLLBAR_WIDTH >= Fv_tree.r.r_width)
				Fv_tree.r.r_left = rn->r_left;
			else
				Fv_tree.r.r_left += Fv_tree.r.r_left < rn->r_left
							? rn->r_width
							: -rn->r_width;
		}
		else if (Fv_tree.r.r_top != rn->r_top)
		{
			/* Vertical scroll */

			if (rn->r_height+SCROLLBAR_WIDTH >= Fv_tree.r.r_height)
				Fv_tree.r.r_top = rn->r_top;
			else
				Fv_tree.r.r_top += Fv_tree.r.r_top < rn->r_top 
							? rn->r_height
							:-rn->r_height;
		}
	}
#else
	Fv_tree.r.r_top = (int)scrollbar_get(Fv_tree.vsbar, 
		SCROLL_VIEW_START) * 10 ;
	Fv_tree.r.r_left = (int)scrollbar_get(Fv_tree.hsbar, 
		SCROLL_VIEW_START) * 10 ;
#endif
	if (Fv_dont_paint)
		return;

	/* Recalculate tree positions when the tree view changes.  Always keep
	 * open folder visible when state changes from path to tree...
	 */

	fv_drawtree((BOOLEAN)(lastview!=Fv_treeview));
	if (!lastview && Fv_treeview && Fv_current)
		fv_visiblefolder(Fv_current);
	lastview = Fv_treeview;
}


#ifdef SV1
static
drag(cnvs, x, y)
	Canvas cnvs;
	register int x,y;			/* Mouse coordinate */
{
	register int x1, y1;			/* Next mouse coordinate */
	register int tx, ty;			/* Folder coordinate */
	int xoffset, yoffset;			/* Offsets into canvas */
	Event ev;				/* Next event */
	FV_TNODE *t_p;				/* We're over this node */
	FV_TNODE *tree_target = 0;		/* Current open node */
	FV_TNODE *tree_lock = 0;		/* Current locked node */
	register FV_TNODE *selected = Fv_tselected;	/* Store selected */
	char path[MAXPATH];			/* Target path */


	window_set(cnvs, WIN_GRAB_ALL_INPUT, TRUE, 0);
	tx = selected->x;
	ty = selected->y;
	xoffset = (int)scrollbar_get(Fv_tree.hsbar, SCROLL_VIEW_START);
	yoffset = (int)scrollbar_get(Fv_tree.vsbar, SCROLL_VIEW_START);
	while (window_read_event(cnvs, &ev) != -1 && event_id(&ev) != MS_LEFT)
	{
		pw_rop(Fv_tree.pw, tx, ty, GLYPH_WIDTH, TREE_GLYPH_HEIGHT,
			PIX_SRC^PIX_DST, Fv_icon[FV_IFOLDER], 0, TREE_GLYPH_TOP);

		x1 = event_x(&ev);
		y1 = event_y(&ev);
		tx += x1-x;
		ty += y1-y;

		t_p = mouse(x1+xoffset, y1+yoffset, Fv_thead);
		fv_tree_feedback(t_p, &tree_target, &tree_lock, selected, path);

		pw_rop(Fv_tree.pw, tx, ty, GLYPH_WIDTH, TREE_GLYPH_HEIGHT,
			PIX_SRC^PIX_DST, Fv_icon[FV_IFOLDER], 0, TREE_GLYPH_TOP);

		x = x1;
		y = y1;
	}
	window_set(cnvs, WIN_GRAB_ALL_INPUT, FALSE, 0);

	/* Remove last */
	pw_rop(Fv_tree.pw, tx, ty, GLYPH_WIDTH, TREE_GLYPH_HEIGHT,
		PIX_SRC^PIX_DST, Fv_icon[FV_IFOLDER], 0, TREE_GLYPH_TOP);

	/* Replace original */
	pw_rop(Fv_tree.pw, selected->x, selected->y, GLYPH_WIDTH, TREE_GLYPH_HEIGHT,
		PIX_SRC^PIX_DST, Fv_icon[FV_IFOLDER], 0, TREE_GLYPH_TOP);

	if (tree_target || tree_lock)
	{
		/* Make the move */
		if (tree_target)
		{
			if (fv_move_copy(path, FALSE) == SUCCESS)
			{
				/* Isolate source node from branch */
				t_p = selected->parent->child;
				if (t_p == selected)
					selected->parent->child = selected->sibling;
				else
				{
					while (t_p && t_p->sibling != selected)
						t_p = t_p->sibling;
					if (t_p)
						t_p->sibling = selected->sibling;
				}

				/* Insert source into target branch */
				selected->sibling = tree_target->child;
				selected->parent = tree_target;
				tree_target->child = selected;
				tree_target->mtime = time(0);
				fv_sort_children(tree_target);
				fv_getpath(selected, (char *)0);

				fv_drawtree(TRUE);
				return;
			}
		}
		else
			tree_target = tree_lock;
		pw_rop(Fv_tree.pw, tree_target->x, tree_target->y,
			GLYPH_WIDTH, TREE_GLYPH_HEIGHT, PIX_SRC, 
			Fv_icon[FV_IFOLDER], 0, TREE_GLYPH_TOP);
		fv_put_text_on_folder(tree_target);
	}
}
#endif


fv_tree_feedback(t_p, tree_target, tree_lock, self, target_tree_path)
	register FV_TNODE *t_p;			/* We're over this node */
	register FV_TNODE **tree_target;	/* Current open node */
	register FV_TNODE **tree_lock;		/* Current locked node */
	FV_TNODE *self;				/* Dragged node */
	char *target_tree_path;			/* Open path */
{
	if (t_p && t_p != self)
	{
		if (t_p != *tree_target)
		{
			if (*tree_target)
			{
				/* Close previous open folder */
				pw_rop(Fv_tree.pw, (*tree_target)->x, (*tree_target)->y,
					GLYPH_WIDTH, GLYPH_HEIGHT, PIX_SRC, 
					Fv_icon[FV_IFOLDER], 0, TREE_GLYPH_TOP);
				fv_put_text_on_folder(*tree_target);
				*tree_target = NULL;
			}

			fv_getpath(t_p, target_tree_path);
			if (access(target_tree_path, W_OK) == 0)
			{
				/* We can place objects here,
				 * open and invert new folder.
				 */
				*tree_target = t_p;
				pw_rop(Fv_tree.pw, (*tree_target)->x, (*tree_target)->y,
					GLYPH_WIDTH, TREE_GLYPH_HEIGHT, PIX_SRC,
					Fv_icon[FV_IOPENFOLDER], 0, TREE_GLYPH_TOP);
				fv_put_text_on_folder(t_p);
				pw_rop(Fv_tree.pw, (*tree_target)->x, (*tree_target)->y,
					GLYPH_WIDTH, TREE_GLYPH_HEIGHT,
					PIX_SRC^PIX_DST, 
					Fv_invert[FV_IOPENFOLDER], 0, TREE_GLYPH_TOP);
			}
			else if (*tree_lock != t_p)
			{
				if (*tree_lock)
				{
					/* Close previous locked folder */
					pw_rop(Fv_tree.pw,
						(*tree_lock)->x, (*tree_lock)->y,
						GLYPH_WIDTH, TREE_GLYPH_HEIGHT,
						PIX_SRC, Fv_icon[FV_IFOLDER],
						0, TREE_GLYPH_TOP);
					fv_put_text_on_folder(*tree_lock);
				}

				/* We can't place objects in this folder.  Show
				 * lock.
				 */
				*tree_target = NULL;
				*tree_lock = t_p;
				pw_rop(Fv_tree.pw, (*tree_lock)->x, (*tree_lock)->y,
					GLYPH_WIDTH, TREE_GLYPH_HEIGHT,
					PIX_SRC, Fv_lock, 0, TREE_GLYPH_TOP);
			}
		}
	}
	else if (*tree_target || *tree_lock)
	{
		/* Over empty space; close/unlock folder */
		if (*tree_lock)
			*tree_target = *tree_lock;
		pw_rop(Fv_tree.pw, (*tree_target)->x, (*tree_target)->y,
			GLYPH_WIDTH, TREE_GLYPH_HEIGHT, PIX_SRC, 
			Fv_icon[FV_IFOLDER], 0, TREE_GLYPH_TOP);
		fv_put_text_on_folder(*tree_target);
		*tree_target = NULL;
		*tree_lock = NULL;
	}
}


fv_init_tree()		/* Initialize tree. Called before window main loop. */
{
	register FV_TNODE *f_p, *nf_p;		/* Node pointers */
	register char *n_p, *s_p;		/* String pointers */
	BOOLEAN new;				/* function parameter */


	if ( !getwd(Fv_path) )
	{
		(void)fprintf(stderr, Fv_message[MECHDIR], ".", 
			sys_errlist[errno]);
		exit(1);
	}
	(void)strcpy(Fv_openfolder_path, Fv_path);

	/* Build tree */

	if ( ((f_p = (FV_TNODE *)fv_malloc(sizeof(FV_TNODE))) == NULL) ||
	     ((f_p->name = fv_malloc(2)) == NULL ))
		exit(1);

	Fv_troot = Fv_thead = f_p;
	(void)strcpy(f_p->name, "/");
	f_p->parent = f_p->sibling = NULL;
	f_p->mtime = 0;
	f_p->status = 0;
	n_p = &Fv_path[1];

	while ( *n_p )
	{
		/* Go down chain collecting names. (I assumed longest name
		 * to be 20 characters...)
		 */

		if ( ((nf_p = (FV_TNODE *)fv_malloc(sizeof(FV_TNODE))) == NULL) ||
		     ((nf_p->name = fv_malloc(20)) == NULL ))
			exit(1);

		f_p->child = nf_p;
		nf_p->parent = f_p;
		nf_p->sibling = NULL;
		nf_p->mtime = 0;
		nf_p->status = 0;
		s_p = nf_p->name;
		while ( *n_p && *n_p != '/' )
		{
			if ( s_p - nf_p->name < MAXPATH )
				*s_p++ = *n_p;
			else
			{
				fprintf(stderr, "path too long\n");
				exit(1);
			}
			n_p++;
		}
		*s_p = NULL;
		if ( *n_p )
			n_p++;	/* Skip next / (if any) */
		f_p = nf_p;
	}
	f_p->child = NULL;
	Fv_current = f_p;
	Fv_lastcurrent = f_p;
	fv_add_children(f_p, &new, (char *)0);
}


fv_add_children(f_p, new, dname)		/* Add children to current node */
	register FV_TNODE *f_p;			/* Node */
	BOOLEAN *new;				/* Children found? */
	char *dname;				/* Directory name */
{
	register FV_TNODE *nf_p;		/* Next tree pointer */
	FV_TNODE *child[256];			/* Subdirectories */
	register FV_TNODE *existing_child;	/* Ignore existing child */
	register int nchild;			/* Number of children */
	register struct dirent *dp;		/* File in directory */
	DIR *dirp;				/* Directory file ptr */
	struct stat fstatus;			/* Status info */
	register int i;				/* Index */
	static int compare_name();		/* Key compare */
	BOOLEAN compare;
	register char *c_p = 0;			/* String pointer */



	/* If this directory is a symbolic link, note it and return */

	if ((lstat(dname ? dname : Fv_path, &fstatus)==0) &&
	    (fstatus.st_mode & S_IFMT) == S_IFLNK)
	{
		f_p->status |= SYMLINK;
		*new = TRUE;
		return(SUCCESS);
	}

	if (!dname)
		dname = ".";

	/* Bitch if we can't get in... */

	if ( (stat(dname, &fstatus) == -1) ||
	     ((dirp = opendir(dname)) == NULL) )
	{
		fv_putmsg(TRUE, sys_errlist[errno], 0, 0);
		return(-1);
	}


	compare = f_p->mtime && f_p->child;
	f_p->mtime = fstatus.st_mtime;
	nchild = 0;
	*new = FALSE;

	while (dp=readdir(dirp))
	{
		/* Ignore '.' and '..' and any existing child.
		 * (Names beginning with ".." are hidden for undelete)
		 */

		if (dp->d_name[0] == '.' && 
		    (dp->d_name[1] == '.' || dp->d_name[1] == NULL))
			continue;

		if (!compare)
			if ((f_p->child && 
			    (strcmp(f_p->child->name,dp->d_name)==0)))
				continue;

		/* Not in the current folder?  Then affix path to name...*/

		if (dname[0] != '.' && dname[1] != 0)
		{
			if (!c_p)
			{
				c_p = dname;			/* Get end of path */
				while (*c_p)
					c_p++;
			}
			*c_p = '/';
			(void)strcpy(c_p+1, dp->d_name);	/* Copy in name */
			i=stat(dname, &fstatus);
			*c_p = NULL;			/* Null out name again*/
		}
		else
			i=stat(dp->d_name, &fstatus);

		if ( i == 0 && (fstatus.st_mode & S_IFMT) == S_IFDIR )
		{
			/* Found a directory, bitch and return if
			 * we can't allocate memory for structure or
			 * name.
			 */
			if (((nf_p = (FV_TNODE *)fv_malloc(sizeof(FV_TNODE))) == NULL)||
			 ((nf_p->name = fv_malloc((unsigned)strlen(dp->d_name)+1))==NULL))
				return(-1);

			nf_p->parent = f_p;
			nf_p->child = NULL;
			(void)strcpy(nf_p->name, dp->d_name);
			nf_p->mtime = 0;
			nf_p->status = 0;

			/* Remember each child */

			child[nchild] = nf_p;
			nchild++;
			if (nchild==256)
				break;
		}
	}
	(void)closedir(dirp);


	if (nchild)
	{
		if (nchild>1)
		{
			/* Sort children alphabetically */

			qsort((char *)child, nchild, sizeof(FV_TNODE *),
				compare_name);
			
			/* Fix sibling pointers */

			for (i = 1; i < nchild; i++)
				child[i-1]->sibling = child[i];
		}
		child[nchild-1]->sibling = NULL;


		if (compare)
		{
			compare_child(f_p->child, child[0], new);
			return(SUCCESS);
		}

		existing_child = f_p->child;
		f_p->child = child[0];

		if (existing_child)
		{
		/* Insert existing child into sorted array ...*/

		for (i = 0; i < nchild; i++)
			if (strcmp(existing_child->name, child[i]->name)<0)
			{
				existing_child->sibling = child[i];
				if (i)
					child[i-1]->sibling = existing_child;
				else
					f_p->child = existing_child;
				break;
			}
		
		if (i==nchild)
		{
			/* ...must belong at end */

			child[i-1]->sibling = existing_child;
			existing_child->sibling = NULL;
		}
		}

		*new = TRUE;
	}
	else if (compare)
	{
		fv_destroy_tree(f_p->child);
		f_p->child = NULL;
		*new = TRUE;
	}


	return(SUCCESS);
}

/* Return -1, 0, 1 if less than, equal to, or greater than */

static
compare_name(f1, f2)
	FV_TNODE **f1, **f2;
{
	return(strcmp((*f1)->name, (*f2)->name));
}

static
compare_child(ff_p, fc_p, new)
	FV_TNODE *ff_p, *fc_p;
	BOOLEAN *new;
{
	register FV_TNODE *f_p, *c_p, *t_p;
	long delete = 1;			/* Unlikely time (delete marker */

	c_p = fc_p;
	while (c_p)
	{
		/* Test current children against previous */

		for (f_p = ff_p; f_p; f_p = f_p->sibling)
			if (strcmp(c_p->name, f_p->name) == 0)
				break;

		/* Exists, mark for delete */
		if (f_p)
			c_p->mtime = delete;
			
		c_p = c_p->sibling;
	}

	f_p = ff_p;
	while (f_p)
	{
		/* Test previous against current */

		for (c_p = fc_p; c_p; c_p = c_p->sibling)
			if (strcmp(c_p->name, f_p->name) == 0)
				break;

		/* No longer exists, mark for delete */
		if (!c_p)
			f_p->mtime = delete;
			
		f_p = f_p->sibling;
	}

	/* Remove deleted/moved folders */

	for (f_p = ff_p, c_p = NULL; f_p;)
		if (f_p->mtime == delete)
		{
			if (c_p)
				c_p->sibling = f_p->sibling;
			else
				f_p->parent->child = f_p->sibling;
			t_p = f_p->sibling;
			f_p->sibling = NULL;
			fv_destroy_tree(f_p);
			*new = TRUE;
			f_p = t_p;
		}
		else
		{
			c_p = f_p;
			f_p = f_p->sibling;
		}
	f_p = c_p;

	/* Add new folders */

	for (c_p = fc_p; c_p; c_p = c_p->sibling)
		if (c_p->mtime != delete)
		{
			/* Tack it on the end */
			if (f_p)
				f_p->sibling = c_p;
			else
				ff_p->child = c_p;
			f_p = c_p;
			*new = TRUE;
		}
		else
		{
			/* Delete it */
			free(c_p->name);
			free((char *)c_p);
		}
	if (f_p)
		f_p->sibling = NULL;
}


fv_sort_children(t_p)			/* Sort node's children by name */
	FV_TNODE *t_p;
{
	register FV_TNODE *c_p;
	FV_TNODE *child[256];
	register int cno;

	if (t_p->child == NULL || t_p->child->sibling == NULL)
		return;

	for (c_p = t_p->child, cno=0; c_p && cno<256; c_p = c_p->sibling)
		child[cno++] = c_p;

	qsort((char *)child, cno, sizeof(FV_TNODE *), compare_name);
		
	/* Fix sibling pointers */

	child[cno] = NULL;
	while (cno) {
		child[cno-1]->sibling = child[cno];
		cno--;
	}
	t_p->child = child[0];
}


/* Did we visit this node, does it have any children, etc? */

fv_check_children(f_p, new)
	register FV_TNODE *f_p;			/* Node */
	BOOLEAN *new;				/* New children? */
{
	struct stat fstatus;			/* File status info */


	*new = FALSE;

	/* Get path */

	if (fv_getpath(f_p, (char *)0) || chdir(Fv_path) == -1)
	{
		fv_putmsg(TRUE, sys_errlist[errno], 0, 0);
		Fv_tselected = FALSE;
		return;
	}

	if (f_p->mtime)
	{
		/* We're already seen this folder.  Compare the modification
		 * date and rebuild this portion of the tree if necessary.
		 */

		if (stat(".", &fstatus))
		{
			fv_putmsg(TRUE, sys_errlist[errno], 0, 0);
			return;
		}
		if (fstatus.st_mtime > f_p->mtime)
		{
			fv_add_children(f_p, new, (char *)0);
			fv_putmsg(FALSE, Fv_message[MMODIFIED],
				(int)f_p->name, 0);
		}
	}
}

static char *E_p;
fv_expand_all_nodes()
{
	FV_TNODE *sibling;

	fv_busy_cursor(TRUE);
	sibling = Fv_tselected->sibling;
	Fv_tselected->sibling = NULL;
	E_p = Fv_path+strlen(Fv_path);
	expand_all(Fv_tselected);
	Fv_tselected->sibling = sibling;
	fv_getpath(Fv_tselected, (char *)0);
	fv_drawtree(TRUE);
	fv_busy_cursor(FALSE);
}

static
expand_all(f_p)
	register FV_TNODE *f_p;
{
	BOOLEAN new;
	register char *c_p;

	while (f_p)
	{
		/* Unprune branches */
		if (f_p->status & PRUNE)
			f_p->status &= ~PRUNE;

		if (f_p->mtime == 0)
			fv_add_children(f_p, &new, Fv_path);
		if (f_p->child)
		{
			c_p = f_p->child->name;
			*E_p++ = '/';
			while (*E_p++ = *c_p++)
				;
			E_p--;
			expand_all(f_p->child);
		}
		if (f_p = f_p->sibling)
		{
			c_p = f_p->name;
			while (*E_p != '/')
				E_p--;
			while (*(++E_p) = *c_p++)
				;
		}
	}
	while (*E_p != '/')
		E_p--;
	*E_p = NULL;
}

fv_expand_node(f_p, new)
	register FV_TNODE *f_p;
	BOOLEAN *new;				/* New children? */
{

	if ((fv_add_children(f_p, new, (char *)0) == SUCCESS) && 
		*new == FALSE)
	{
		/* No children (but show that it has been explored) */

		reverse(f_p);
		pw_rop(Fv_tree.pw, f_p->x, f_p->y,
			GLYPH_WIDTH, (TREE_GLYPH_HEIGHT>>2)-(TREE_GLYPH_HEIGHT>>4),
			PIX_SRC, Fv_icon[FV_IFOLDER],
			0, TREE_GLYPH_TOP);
		reverse(f_p);
	}
}

/* Free up memory allocated to (portion of) tree */

fv_destroy_tree(f_p)
	register FV_TNODE *f_p;
{
	for ( ; f_p; f_p = f_p->sibling)
		if (f_p->child)
			fv_destroy_tree(f_p->child);
		else
		{
			free(f_p->name);
			free((char *)f_p);
		}
}

/* Get full path name from node in tree */

fv_getpath(c_p, buf)
	register FV_TNODE *c_p;		/* Node */
	char *buf;			/* Local copy of path */
{
	FV_TNODE *level[32];
	register int levelno;
	register char *p_p, *s_p;

	/* Get full path name; back up tree and copy it in fv_reverse
	 * order.
	 */
	for ( levelno = 0; c_p; c_p = c_p->parent, levelno++ )
	{
		if ( levelno > 32 )
		{
			fv_putmsg(TRUE, "> 32 levels deep", 0, 0);
			return(FAILURE);
		}
		level[levelno] = c_p;
	}

	if (!buf)
		buf = Fv_path;
	p_p = buf;
	levelno -= 2;		/* Skip root */
	while ( levelno >= 0 )
	{
		*p_p++ = '/';
		s_p = level[levelno]->name;
		while ( *s_p )
			*p_p++ = *s_p++;
		levelno--;
	}
	if ( p_p == buf )
		*p_p++ = '/';
	*p_p = NULL;

	c_p = level[0];

	return(SUCCESS);
}

/* Called by filer to request updating of tree */

fv_updatetree()
{
	register FV_TNODE *f_p, *lf_p;	/* Current, last nodes */
	register FV_TNODE *existing;	/* Existing child node */
	FV_TNODE *first_p;		/* First created node */
	register FV_FILE **file_p, **lfile_p;	/* Files array pointers */


	lf_p = NULL;
	existing = Fv_current->child;

	for (file_p=Fv_file, lfile_p=Fv_file+Fv_nfiles;
		file_p != lfile_p; file_p++)
		if ((*file_p)->type == FV_IFOLDER)
		{
		/* Don't allocate existing children */

		if (existing && strcmp(existing->name, (*file_p)->name) == 0)
			continue;

		if ((f_p = (FV_TNODE *)fv_malloc(sizeof(FV_TNODE))) == NULL ||
		   (f_p->name = fv_malloc((unsigned)strlen((*file_p)->name)+1))==NULL)
			return;

		if (!lf_p)
			first_p = f_p;
		f_p->parent = Fv_current;
		f_p->child = NULL;
		(void)strcpy(f_p->name, (*file_p)->name);
		f_p->mtime = 0;
		f_p->status = 0;
		if (lf_p)
			lf_p->sibling = f_p;
		lf_p = f_p;
	}
	if (lf_p)
		lf_p->sibling = NULL;
	else
		return;

	if (Fv_current->child)
	{
		/* Insert existing child in alphabetically correct spot.
		 * (Names returned by fv_getnextdir() have been sorted.)
		 */

		for (lf_p = NULL, f_p = first_p; f_p;
			lf_p = f_p, f_p = f_p->sibling )
			if (strcmp(existing->name, f_p->name) < 0)
			{
				/* Existing child should appear here */
				existing->sibling = f_p;
				break;
			}

		if (lf_p)
		{
			/* Existing child is no longer first element */

			lf_p->sibling = existing;
			Fv_current->child = first_p;
		}
	}
	else
		Fv_current->child = first_p;
}


static FV_TNODE *
mouse(x, y, f_p)			/* Are we over a folder? */
	int x, y;
	FV_TNODE *f_p;
{
	Chosen = NULL;
	mouse_recursive(x, y, f_p);
	return(Chosen);
}

static
mouse_recursive(x, y, f_p)
	register int x, y;
	register FV_TNODE *f_p;
{
	if (Chosen)
		return;

	for ( ; f_p; f_p = f_p->sibling )
	{
		if (f_p->sibling && f_p->sibling->stack)
		{
			/* The visible portion of a stacked folder should
			 * be selectable; since the last stacked folder
			 * is completely visible, work backwards...
			 */

			FV_TNODE *lf_p = f_p;
			register int sno = 0;

			/* Collect stacked folders... */

			Mouse_stack[sno++] = f_p;
			for (f_p = f_p->sibling; f_p && f_p->stack; f_p = f_p->sibling)
				Mouse_stack[sno++] = f_p;

			/* ... and compare them in reverse order */

			while (sno--)
				if ( ( x >= Mouse_stack[sno]->x && 
				       x <= Mouse_stack[sno]->x+GLYPH_WIDTH ) &&
				     ( y >= Mouse_stack[sno]->y && 
				       y <= Mouse_stack[sno]->y+TREE_GLYPH_HEIGHT ) )
				{
					Chosen = Mouse_stack[sno];
					return;
				}
			f_p = lf_p;
		}
		

		if ( ( x >= f_p->x && x <= f_p->x+GLYPH_WIDTH ) &&
		   ( y >= f_p->y && y <= f_p->y+TREE_GLYPH_HEIGHT ) )
		{
			Chosen = f_p;
			return;
		}
		if (f_p->child && !(f_p->status & PRUNE))
			mouse_recursive(x, y, f_p->child);
	}
}



FV_TNODE *
fv_infolder(x, y)		/* Return folder name at coordinate */
	int x, y;
{

	if (Fv_treeview)
	{
		/* Only the tree can scroll, adjust for scrolling... */
		x += Fv_tree.r.r_left;
		y += Fv_tree.r.r_top;

		return(mouse(x, y, Fv_thead));
	}
	else return(path_chosen(x, y));
}



static
reverse(f_p)				/* Highlight folder */
	register FV_TNODE *f_p;
{
	if (f_p)
		pw_rop(Fv_tree.pw, f_p->x, f_p->y,
			GLYPH_WIDTH, TREE_GLYPH_HEIGHT,
			PIX_SRC ^ PIX_DST,
			f_p==Fv_current?Fv_invert[FV_IOPENFOLDER]:
				f_p->mtime && !(f_p->status&PRUNE) ?
					     Fv_invert[FV_IFOLDER] :
					     Fv_invert[FV_IUNEXPLFOLDER],
			0, TREE_GLYPH_TOP);
}


fv_open_folder()			/* Open selected folder */
{
	int len;
	char buf[MAXPATH];
	BOOLEAN children;

	fv_busy_cursor(TRUE);		/* Cursor wait */

	/* Open reveals folder's children, if not currently visible */
	if (Fv_tselected->mtime == 0)
	{
		fv_expand_node(Fv_tselected, &children);
		if (children)
			fv_drawtree(TRUE);
	}

	/* Open reveals hidden folder */
	if (Fv_tselected->status & PRUNE)
	{
		Fv_tselected->status &= ~PRUNE;
		fv_drawtree(TRUE);
	}

	/* Open a symbolically linked folder displays the link contents */
	if (Fv_tselected->status & SYMLINK)
	{
		/* Symbolic link to directory; get reference's real name.
		 * These names may be relative pathnames...
		 */

		char *b_p;
		(void)strcpy(buf, Fv_path);
		len = strlen(Fv_path);
		b_p = buf+len;
		while (*b_p != '/')		/* Back to parent */
			b_p--;
		b_p++;				/* Skip / */

		if ((len=readlink(Fv_path, b_p, MAXPATH-1)) == -1)
		{
			fv_putmsg(TRUE, sys_errlist[errno], 0, 0);
			fv_busy_cursor(FALSE);
			return;
		}

		/* We've found where it's pointing to,
		 * Either (a) absolute pathname
		 *        (b) relative with no indirection
		 *     or (c) relative with indirection
		 */

		b_p[len] = NULL;
		fv_putmsg(FALSE, "%s linked to %s", 
			(int)Fv_tselected->name, (int)b_p);

		if (strncmp(b_p, "../", 3) == 0)	/* (c) */
		{
			register char *n_p;

			n_p = b_p;
			b_p -= 2;		/* Skip /. */
			while (*b_p != '/')
				b_p--;
			n_p += 3;		/* Skip ../ */
			b_p++;
			while (*b_p++ = *n_p++)
				;
		}

		/* Either (a) or (b) */

		reverse(Fv_tselected);
		Fv_tselected = NULL;
		fv_openfile(*b_p=='/' ? b_p : buf, (char *)0, TRUE);
	}
	else
	{
		Fv_current = Fv_tselected;		/* Selected is now open */
		reverse(Fv_tselected);			/* Turn reverse off */
		fv_showopen();				/* Show open folder */
		reverse(Fv_tselected);			/* Turn reverse back on */
		(void)strcpy(Fv_openfolder_path, Fv_path);	/* Open path */
		Fv_dont_paint = TRUE;
		scrollbar_scroll_to(Fv_foldr.vsbar, 0);
		Fv_dont_paint = FALSE;
		fv_display_folder(FV_BUILD_FOLDER);	/* Open & paint directory */
	}
	fv_busy_cursor(FALSE);		/* Cursor normal */
}


fv_put_text_on_folder(f_p)			/* Place folder name on folder */
	register FV_TNODE *f_p;			/* Tree node */
{
	int x, w, y, l;

	/* Don't bother if not visible */

	if (Fv_treeview &&
	    (f_p->y >= Fv_tree.r.r_top + Fv_tree.r.r_height ||
	     /*f_p->y-(TREE_GLYPH_HEIGHT>>1) < Fv_tree.r.r_top ||*/
	     f_p->x >= Fv_tree.r.r_left + Fv_tree.r.r_width))
		return;

	/* Some portion must be visible... */

	l = fv_strlen(f_p->name);
	x = f_p->x;
	w = GLYPH_WIDTH;
	if (f_p == Fv_current)
		w -= GLYPH_WIDTH>>4;

	if (l < w)
	{
		y = f_p->y+(TREE_GLYPH_HEIGHT>>1);

		pw_text(Fv_tree.pw, x+(w>>1)-(l/2), y,
			PIX_SRC | PIX_DST, Fv_font, f_p->name);
	}
	else
		wrap_text_on_folder(f_p, w, x);
}


static
wrap_text_on_folder(f_p, w, x)	/* Wrap overflow text onto two lines in folder */
	register FV_TNODE *f_p;
	int w, x;
{
	register char *s_p, *n_p;
	char first[16];
	char second[16];
	register short width;
	register BOOLEAN shadow;

	/* Name too large to fit within folder: so wrap within two lines and 
	 * truncate it if necessary.  (If displaying tree, don't bother with
	 * second line if there's another folder stacked over it.)
	 */

	shadow = Fv_treeview && f_p->sibling && f_p->sibling->stack>0;
	*second = NULL;
	s_p = first;
	n_p = f_p->name;
	width = 0;

	while (*n_p)
	{
		width += Fv_fontwidth[*n_p-' '];
		if (width > w)
			if (shadow)
				break;
			else if (*second)
					break;
				else
				{
					*s_p = NULL;
					s_p = second;
					width = Fv_fontwidth[*n_p-' '];
				}
		*s_p++ = *n_p++;
	}
	*s_p = NULL;


	pw_text(Fv_tree.pw, x, f_p->y+(TREE_GLYPH_HEIGHT>>1),
		PIX_SRC | PIX_DST, Fv_font, first);

	if (!shadow)
		pw_text(Fv_tree.pw, x, 
			f_p->y+TREE_GLYPH_HEIGHT-(TREE_GLYPH_HEIGHT>>2),
			PIX_SRC | PIX_DST, Fv_font, second);
}


static
calc_tree(f_p, y)			/* Calculate tree's coordinates */
	register FV_TNODE *f_p;			/* Current node */
	int y;					/* Current y level */
{
	register FV_TNODE *nf_p = NULL;		/* Previous sibling */
	register int stack = 0;			/* Current stack level */

	for (; f_p; f_p = f_p->sibling)
	{
		if (f_p->child && !(f_p->status & PRUNE))
		{
			calc_tree(f_p->child,y+(TREE_GLYPH_HEIGHT<<1));
			nf_p = f_p->child;
			for (; nf_p->sibling; nf_p = nf_p->sibling)
				;
			f_p->x = f_p->child->x+((nf_p->x-f_p->child->x)/2);
			stack = 0;
		}
		else
		{
			if (stack>MAXSTACKDEPTH)
			{
				nf_p = NULL;
				Xpos -= GLYPH_WIDTH;
			}

			if (nf_p && (nf_p->child == NULL||
					nf_p->status == PRUNE))
			{
				Xpos += GLYPH_WIDTH>>2;
				++stack;
			}
			else
			{
				stack = 0;
				Xpos += GLYPH_WIDTH+(GLYPH_WIDTH>>2);
			}
			f_p->x = Xpos;
		}
		if (Xpos>Xmax)
			Xmax = Xpos;
		f_p->stack = stack;
		f_p->y = y+(f_p->stack*((TREE_GLYPH_HEIGHT>>1)+
			(TREE_GLYPH_HEIGHT>>3)));
		if (f_p->y > Ymax)
			Ymax = f_p->y;
		nf_p = f_p;
	}
}


static
display_tree(f_p)			/* Display tree at folder */
	register FV_TNODE *f_p;
{
	register int x;

	for (; f_p; f_p = f_p->sibling)
	{
		pw_rop(Fv_tree.pw, f_p->x, f_p->y, GLYPH_WIDTH,
			TREE_GLYPH_HEIGHT, PIX_SRC,
			f_p==Fv_current?Fv_icon[FV_IOPENFOLDER]:
				f_p->mtime && !(f_p->status&PRUNE)?
					     Fv_icon[FV_IFOLDER] :
					     Fv_icon[FV_IUNEXPLFOLDER],
			0, TREE_GLYPH_TOP);
		fv_put_text_on_folder(f_p);

		/* Connect node with parent */

		if (f_p != Fv_thead && f_p->stack == 0)
		{
			/* Straight up to tab height... */
			x = f_p->x+(GLYPH_WIDTH>>1);
			pw_vector(Fv_tree.pw, x,
				f_p->y+(TREE_GLYPH_HEIGHT>>3),
				x, f_p->y,
				PIX_SRC, 1);

			/* ...then arc to parent */
			pw_vector(Fv_tree.pw, x, f_p->y,
				f_p->parent->x+(GLYPH_WIDTH>>1),
				f_p->y-TREE_GLYPH_HEIGHT, PIX_SRC, 1);
		}

		if (f_p->child && !(f_p->status & PRUNE))
			display_tree(f_p->child);
	}
}


fv_drawtree(calc)			/* Draw path pane */
	BOOLEAN calc;			/* Recalculate tree positions */
{
	FV_TNODE *saved_sibling;

	/* Lock and clear canvas... */

#ifdef SV1
	pw_lock(Fv_tree.pw, &Fv_tree.r);
#endif
	/* Clear background */

	pw_writebackground(Fv_tree.pw, Fv_tree.r.r_left, 
		Fv_tree.r.r_top, Fv_tree.r.r_width, 
		Fv_tree.r.r_height, PIX_CLR);

	if (Fv_treeview)
	{
		if (calc)
		{
			Xpos = MARGIN - (GLYPH_WIDTH+ (GLYPH_WIDTH>>2));
			Xmax = Xpos;
			Ymax = MARGIN;
			calc_tree(Fv_thead, Ymax);
			
			Fv_dont_paint = TRUE;
			window_set(Fv_tree.canvas, 
				CANVAS_WIDTH, Xmax+(GLYPH_WIDTH<<1),
				CANVAS_HEIGHT, Ymax+GLYPH_HEIGHT, 
				0);
			Fv_dont_paint = FALSE;
		}

		/* If we've changed the tree's root, we don't want to
		 * display any of it's possible siblings...
		 */

		saved_sibling = Fv_thead->sibling;
		Fv_thead->sibling = NULL;
		display_tree(Fv_thead);
		Fv_thead->sibling = saved_sibling;

		reverse(Fv_tselected);
	}
	else
		draw_path();

#ifdef SV1
	pw_unlock(Fv_tree.pw);			/* Unlock canvas */
#endif
}


fv_showopen()			/* Avoid repainting tree to show open folder */
{
	if (!Fv_treeview)
	{
		draw_path();
		Fv_lastcurrent = Fv_current;
		return;
	}

	/* We've seen this folder already.  Just 
	 * exchange the open folder icons...
	 */
	if (Fv_lastcurrent)
	{
		register FV_TNODE *f_p;

		pw_rop(Fv_tree.pw, Fv_lastcurrent->x, Fv_lastcurrent->y,
			GLYPH_WIDTH, TREE_GLYPH_HEIGHT,
			PIX_SRC, Fv_icon[FV_IFOLDER], 0, TREE_GLYPH_TOP);
		fv_put_text_on_folder(Fv_lastcurrent);

		f_p = Fv_lastcurrent->sibling;
		if (!Fv_lastcurrent->child && f_p && f_p->stack)
		{
			/* Open folder was in stack--repaint rest of
			 * stack...
			 */
			for (; f_p && f_p->stack; f_p = f_p->sibling)
			{
				pw_rop(Fv_tree.pw, f_p->x, f_p->y,
					GLYPH_WIDTH,
					TREE_GLYPH_HEIGHT,
					PIX_SRC,
					f_p->mtime ? Fv_icon[FV_IFOLDER]
						: Fv_icon[FV_IUNEXPLFOLDER],
					0, TREE_GLYPH_TOP);
				fv_put_text_on_folder(f_p);
			}
		}
				
	}
	pw_rop(Fv_tree.pw, Fv_current->x, Fv_current->y,
		GLYPH_WIDTH, TREE_GLYPH_HEIGHT,
		PIX_SRC, Fv_icon[FV_IOPENFOLDER], 0, TREE_GLYPH_TOP);
	fv_put_text_on_folder(Fv_current);

	/* Ensure that this folder is visible... */

	fv_visiblefolder(Fv_current);

	Fv_lastcurrent = Fv_current;
}

static
draw_path()					/* Draw folder path */
{
	FV_TNODE *level[32];			/* Nodes in path */
	register int x;				/* X coordinate */
	register int y;				/* Y coordinate */
	register int levelno;			/* level array index */
	register FV_TNODE *f_p;			/* Node pointer */


	/* Get full path name; back up tree */

	f_p = Fv_current;
	for ( levelno = 0; f_p; f_p = f_p->parent, levelno++ )
	{
		if ( levelno > 32 )
		{
			fv_putmsg(TRUE, "> 32 levels deep", 0, 0);
			return;
		}
		level[levelno] = f_p;
	}

	y = Fv_tree.r.r_top + MARGIN;

	/* Do we have enough room to display full path?  If not then
	 * display from current folder back with a line to indicate
	 * the truncation.
	 */

	if ((levelno*GLYPH_WIDTH) +((levelno)*(GLYPH_WIDTH>>2))
		> Fv_tree.r.r_width)
	{
		levelno = (Fv_tree.r.r_width-MARGIN)/(GLYPH_WIDTH+
			(GLYPH_WIDTH>>2));

		if (levelno<1)
			levelno = 1;		/* MUST be one visible! */

		pw_vector(Fv_tree.pw, 0, y+(TREE_GLYPH_HEIGHT>>1), GLYPH_WIDTH>>2,
			y+(TREE_GLYPH_HEIGHT>>1), PIX_SRC, 1);
	}
	x = (GLYPH_WIDTH>>2)+Fv_tree.r.r_left;

	/* Display path */

	Npath = levelno;
	levelno--;
	while (levelno > -1)
	{
		f_p = level[levelno];
		pw_rop(Fv_tree.pw, x, y, GLYPH_WIDTH, TREE_GLYPH_HEIGHT,
			PIX_SRC, 
			levelno ? Fv_icon[FV_IFOLDER]
				: Fv_icon[FV_IOPENFOLDER],
			0, TREE_GLYPH_TOP);

		f_p->x = x;
		f_p->y = y;
		fv_put_text_on_folder(f_p);

		x += GLYPH_WIDTH + (GLYPH_WIDTH>>2);

		/* Connecting arc to next folder */

		if (levelno)
			pw_vector(Fv_tree.pw, x-(GLYPH_WIDTH>>2),
				y+(TREE_GLYPH_HEIGHT>>1),
				x,
				y+(TREE_GLYPH_HEIGHT>>1), PIX_SRC, 1);

		levelno--;
	}
	Fv_lastcurrent = Fv_current;
	reverse(Fv_tselected);
}


static FV_TNODE *
path_chosen(x, y)			/* What path was chosen? */
	register int x, y;
{
	register int w, fno;
	FV_TNODE *f_p;

	/* Check reasonable boundaries */

	w = GLYPH_WIDTH+(GLYPH_WIDTH>>2);
	if (x<MARGIN || y<MARGIN || y>(MARGIN+TREE_GLYPH_HEIGHT) || x>(w*Npath))
		return(NULL);

	/* Get folder number chosen (ensure that we're not in folder gap) */

	x -= GLYPH_WIDTH>>2;
	fno = x / w;
	if (fno>0)
		x -= w*fno;
	if (x>GLYPH_WIDTH)
		return(NULL);

	fno++;
	f_p = Fv_current;
	while (fno < Npath)
	{
		f_p = f_p->parent;
		fno++;
	}
	return(f_p);
}


fv_visiblefolder(f_p)			/* Make folder visible in tree */
	register FV_TNODE *f_p;
{
	int newpos;

	/* Try and centre folder in window */

	if (f_p->x < Fv_tree.r.r_left || 
	    f_p->x+(GLYPH_WIDTH>>1) > Fv_tree.r.r_left+Fv_tree.r.r_width)
	{
		newpos = f_p->x - (Fv_tree.r.r_width>>1);
		if (newpos<0)
			newpos = 0;
#ifndef SV1
		newpos /= 10;
#endif
		scrollbar_scroll_to(Fv_tree.hsbar, newpos);
	}

	if (f_p->y < Fv_tree.r.r_top ||
	    f_p->y+(TREE_GLYPH_HEIGHT>>1) > Fv_tree.r.r_top+Fv_tree.r.r_height)
	{
		newpos = f_p->y - (Fv_tree.r.r_height>>1);
		if (newpos<0)
			newpos = 0;
#ifndef SV1
		newpos /= 10;
#endif
		scrollbar_scroll_to(Fv_tree.vsbar, newpos);
	}
}

void
fv_addparent_button()		/* Add tree root's parent */
{
	/* Restore current head */
	if (Fv_sibling && Fv_sibling->parent == Fv_thead->parent)
		Fv_thead->sibling = Fv_sibling;
	add_parent(Fv_thead->parent);
}


static
add_parent(f_p)			/* Add parent to folder */
	FV_TNODE *f_p;
{
	Fv_thead = f_p;
	fv_drawtree(TRUE);
	if (Fv_tselected)
		fv_visiblefolder(Fv_tselected);		/* Track selected */
}


fv_hide_node()			/* Don't show selected folder's descendents */
{
	BOOLEAN new;

	if (Fv_tselected->mtime == 0)
	{
		fv_busy_cursor(TRUE);
		fv_expand_node(Fv_tselected, &new);
		fv_busy_cursor(FALSE);
		if (!new)
			return;
	}
	else if (Fv_tselected->status & PRUNE)
		Fv_tselected->status &= ~PRUNE;
	else
		Fv_tselected->status |= PRUNE;

	fv_drawtree(TRUE);
	fv_visiblefolder(Fv_tselected);		/* Track selected */
}


fv_set_root()			/* Set tree root to current folder */
{
	if (Fv_tselected != Fv_thead)
	{
		Fv_sibling = Fv_tselected->sibling;
		Fv_tselected->sibling = NULL;
		add_parent(Fv_tselected);
		Fv_lastcurrent = NULL;
	}
}


fv_openfile(name, pattern, folder)	/* Open a file */
	char *name;			/* File name (incl path) */
	char *pattern;			/* Pattern it matched */
	BOOLEAN folder;			/* Goto folder? */
{
	register char *n_p;		/* Name pointer */
	char *fname;			/* File name (excl path) */
	register FV_TNODE *f_p, *parent;/* Tree pointers */
	register int more;		/* More of path name to come? */
	BOOLEAN make_folder = FALSE;	/* Did we make a folder? */
	FV_TNODE *child;		/* Temp folder child */


	if (folder)
		fname = NULL;
	else
	{
		/* Remove last component of path */

		n_p = name;
		while (*n_p)
			n_p++;
		while (*n_p != '/' && n_p != name)
			n_p--;

		/* It's a filename, match it */
		if (n_p == name)
		{
			fname = n_p;
			goto match;
		}

		fname = n_p+1;
		*n_p = NULL;
	}

	more = TRUE;

	if (*name == '/')
	{
		/* Begin at root... */
		name++;
		if (*name == NULL)
		{
			more = FALSE;
			f_p = Fv_troot;
		}
		else
		{
			f_p = Fv_troot->child;
			parent = Fv_troot;
		}

		/* Make sure root is visible */
		Fv_dont_paint = TRUE;
		while (Fv_thead != Fv_troot)
			fv_addparent_button();
		Fv_dont_paint = FALSE;
	}
	else
	{
		/* Begin at selected or current folder... */
		f_p = Fv_tselected ? Fv_tselected : Fv_current;
		parent = f_p;
		f_p = f_p->child;
		f_p->status &= ~PRUNE;
	}

	while (more)
	{
		/* Get next folder name... */

		while (*name == '/')		/* Skip extra /'s */
			*name++;
		n_p = name;
		while (*n_p && *n_p != '/')
			n_p++;
		if (more = (*n_p=='/'))
			*n_p = NULL;
		else if (n_p==name)		/* / at end */
			break;
		
		for (; f_p; f_p = f_p->sibling)
			if (strcmp(f_p->name, name)==0)
				break;

		if (f_p)			/* Found folder? */
		{
			if (more)
			{
				parent = f_p;
				f_p = f_p->child;
				if (f_p)
					f_p->status &= ~PRUNE;
			}
			else
				break;		/* Last folder in path */
		}
		else				/* We have to make folder */
		{
			make_folder = TRUE;
			if (((f_p=(FV_TNODE *)fv_malloc(sizeof(FV_TNODE)))==NULL) ||
			    ((f_p->name=fv_malloc((unsigned)strlen(name)+1))==NULL))
				return;

			if (parent && parent->mtime == 0 && parent->child)
			{
				/* Assign parent some bogus time so it thinks
				 * we've seen this folder, (revisiting folder
				 * will expand it, but preserve existing
				 * children)
				 */

				 parent->mtime = 10;
			}
			(void)strcpy(f_p->name, name);
			f_p->parent = parent;
			f_p->sibling = parent->child;
			f_p->child = NULL;
			f_p->status = 0;
			f_p->mtime = 0;
			parent->child = f_p;

			parent = f_p;
		}
		if (more)
		{
			*n_p = '/';
			name = n_p+1;
		}
	}

	/* We've arrived at the correct folder */

	if (fname)
		*(fname-1) = '/';		/* Restore name */

	if (f_p != (Fv_tselected ? Fv_tselected : Fv_current))
	{
		/* File isn't in current directory... */

		fv_getpath(f_p, (char *)0);
		if (chdir(Fv_path)==-1)
			return;
		(void)strcpy(Fv_openfolder_path, Fv_path);
		Fv_current = f_p;

		/* Did folder repaint discover new folders? */

		child = f_p->child;
		fv_display_folder(FV_BUILD_FOLDER);
		if (child != f_p->child)
			make_folder = TRUE;

		if (Fv_thead != Fv_troot && !fv_descendant(f_p, Fv_thead))
		{
			/* Folder not in current subtree; back to real root */

			Fv_thead->sibling = Fv_sibling;
			Fv_thead = Fv_troot;
			make_folder = TRUE;
		}

		if (make_folder || !Fv_treeview)
			fv_drawtree(make_folder);
		else
			fv_showopen();
			
		if (Fv_treeview)
		{
			fv_visiblefolder(Fv_current);
			reverse(Fv_current);
			Lastselected = Fv_tselected = Fv_lastcurrent = Fv_current;
		}
		else
			Lastselected = Fv_tselected = Fv_lastcurrent = NULL;
	}

	/* If we were passed a pattern, then try and edit it, otherwise
	 * just match it.
	 */
	if (fname)
	{
match:
		if (*fname == '.' && !Fv_see_dot)
			fv_putmsg(TRUE, Fv_message[MEHIDDEN], 0, 0);
		else if (pattern)
			fv_gotofile(fname, pattern);
		else
			fv_match_files(fname);
	}
	else
		scrollbar_scroll_to(Fv_foldr.vsbar, 0);
}


fv_treedeselect()		/* Deselect folder */
{
	reverse(Fv_tselected ? Fv_tselected : Lastselected);
	Lastselected = NULL;
	Fv_tselected = NULL;
}


fv_descendant(c_p, f_p)		/* Search folder tree for descendant of folder */
	FV_TNODE *c_p;
	FV_TNODE *f_p;
{
	if (c_p == f_p)
		return(TRUE);
	Child = c_p;
	Found_child = FALSE;
	descendant(f_p);
	return(Found_child);
}

static
descendant(f_p)
	register FV_TNODE *f_p;
{
	if (Found_child)
		return;

	for ( ; f_p; f_p = f_p->sibling)
		if (f_p == Child)
		{
			Found_child = TRUE;
			break;
		}
		else if (f_p->child)
			descendant(f_p->child);
}
