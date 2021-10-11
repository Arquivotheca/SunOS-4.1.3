#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)wmgr_policy.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *	Implementation of window manager icon- and tool-placement policies.
 */

#include <sunwindow/sun.h>
#include <sunwindow/defaults.h>
#include <suntool/wmgr_policy.h>

extern char	*getenv(), *index(), *malloc();
struct level_parms  {
	int		self;
	int		prev;
	int		next;
	Close_level	level;
};
typedef struct level_parms	Level_parms;

static struct wmgr_policy	 default_policy = {
	North, Front, Total, Icons, FALSE
};

static Defaults_pairs	directions[] = {
	"North",	(int)North,
	"South",	(int)South,
	"East",		(int)East,
	"West",		(int)West,
	0,		-1
},
			levels[] = {
	"Ahead_of_all",	  (int)Front,
	"Ahead_of_icons", (int)Boundary,
	"Behind_all",	  (int)Back,
	0,		  -1
};

static void	fold_down(),
		parse_policy_attribute(),
		wmgr_adjust_tool(),
		wmgr_advance_icon(),
		wmgr_advance_tool(),
		wmgr_find_icon_slot(),
		wmgr_findlevel(),
		wmgr_get_trimmed_bounds(),
		wmgr_place_first_icon(),
		wmgr_stow_rects();

static int
		match(),
		wmgr_test_rect(),
		wmgr_test_slots();

static enum win_enumerator_result
		wmgr_find_boundary();

static struct rectlist *
		wmgr_find_free_space();

static void
fold_down(from, to)
	register char	*from, *to;
{
	register char	 c;

	do  {
		if (isupper(c = *from++))
			*to++ = tolower(c);
		else
			*to++ = c;
	} while (c);
}

static int
match(str1, str2)
	register char	*str1, *str2;
{
	register char	c1, c2;

	while ( (c1 = *str1++) && (c2 = *str2++) )  {
		if (c1 != c2)  {
			return FALSE;
		}
	}
	return TRUE;
}

static void
parse_policy_attribute(attr, val, policy)
	char		*attr, *val;
	Policy_handle	 policy;
{
	if (match(attr, "gravity") ) {
		switch (*val)  {
		  case 'e':	policy->gravity = East;
				return;
		  case 's':	policy->gravity = South;
				return;
		  case 'w':	policy->gravity = West;
				return;
		  case 'n':
		  default:	policy->gravity = North;
				return;
		}
	}
	if (match(attr, "close_level") ) {
		if (match(val, "front") )  {
			policy->close_level = Front;
		} else if (match(val, "cur_level") )  {
			policy->close_level = Cur_level;
		} else if (match(val, "boundary") )  {
			policy->close_level = Boundary;
		} else if (match(val, "back") )  {
			policy->close_level = Back;
		}
	}
	if (match(attr, "visibility") ) {
		if (match(val, "total") )  {
			policy->visibility = Total;
		} else if (match(val, "partial") )  {
			policy->visibility = Partial;
		}
	}
	if (match(attr, "free_from") ) {
		if (match(val, "cur_windows") )  {
			policy->obstructors = Cur_windows;
		} else if (match(val, "icons") )  {
			policy->obstructors = Icons;
		} else if (match(val, "open") )  {
			policy->obstructors = Open;
		} else if (match(val, "all_windows") )  {
			policy->obstructors = All_windows;
		}
	}
	if (match(attr, "mobile") ) {
		policy->mobile = TRUE;
	}
	if (match(attr, "nomobile") ) {
		policy->mobile = FALSE;
	}
}

Policy_handle
wmgr_get_icon_policy()
{
static Policy_handle	 policy;
	char		*parm, buffer[1024];
	register char   *attr, *val;
	Close_level	 level;
	Gravity		 gravity;

	if (policy != (Policy_handle) NULL ) 
		return policy;

	policy = (Policy_handle) (LINT_CAST(sv_malloc(sizeof *policy)));
	*policy = default_policy;
	level = (Close_level) defaults_lookup(
			defaults_get_string("/SunView/Icon_close_level", 
				(char *)0, (int *)0),
			levels);
	if ((int)level != -1) {
		policy->close_level = level;
	}
	gravity = (Gravity) defaults_lookup(
			defaults_get_string("/SunView/Icon_gravity", (char *)0, (int *)0),
			directions);
	if ((int)gravity != -1) {
		policy->gravity = gravity;
	}
	if ( (parm = getenv("ICON_POLICY") ) != NULL) {
		fold_down(parm, buffer);
		attr = buffer;
		while (*attr)  {
			register char	*next;
	
			if (val = index(attr, '=') ) {
				*val++ = '\0';
				next = index(val, ',');
				if (!next) {
					next = index(val, ' ');
				}
			} else {
				next = 0;
			}
			if (next) {
				*next++ = '\0';
			} else {
				next = index(buffer, '\0');
			}
			parse_policy_attribute(attr, val, policy);
			attr = next;
		}
		if (*attr) {
			(void)fprintf(stderr, "unrecognized icon policy parm: %s\n",
				attr);
		}
	}
	return policy;
}

static int
wmgr_test_rect(r, rl, policy)
	struct rect	*r;
	struct rectlist	*rl;
	Policy_handle	 policy;
{
	struct rectlist	 new_rl;
	int		 result;

	new_rl = rl_null;
	(void)rl_rectintersection(r, rl, &new_rl);
	switch (policy->visibility)  {
	  case Total:	(void)rl_coalesce(&new_rl);
			result = rl_equalrect(r, &new_rl);
			break;
	  case Partial:	result = ! rl_empty(&new_rl);
			break;
	} 
	(void)rl_free(&new_rl);
	return result;
}

static void
wmgr_place_first_icon(gravity, bounds, candidate)
        Gravity		gravity;
        struct rect    *bounds, *candidate;
{
        if (gravity == East)  {
                candidate->r_left = bounds->r_width - candidate->r_width;
        } else {
                candidate->r_left = 0;
        }
        while (candidate->r_left % WMGR_ICON_ALIGN_X != 0) {
        	candidate->r_left--;
        }
        if (gravity == South)  {
                candidate->r_top = bounds->r_height - candidate->r_height;
        } else {
                candidate->r_top = 0;
        }
        while (candidate->r_top % WMGR_ICON_ALIGN_Y != 0) {
        	candidate->r_top--;
        }
}

static void
wmgr_advance_icon(gravity, bounds, candidate)
	Gravity		 gravity;
	struct rect	*bounds, *candidate;
{
	int             vertical, x_advance, y_advance;

	switch (gravity) {
	   case East:   vertical = TRUE;
			x_advance = -TOOL_ICONWIDTH;
			y_advance = TOOL_ICONHEIGHT;
			break;
	   case West:   vertical = TRUE;
			x_advance = TOOL_ICONWIDTH;
			y_advance = TOOL_ICONHEIGHT;
			break;
	   case North:  vertical = FALSE;
			x_advance = TOOL_ICONWIDTH;
			y_advance = TOOL_ICONHEIGHT;
			break;
	   case South:  vertical = FALSE;
			x_advance = TOOL_ICONWIDTH;
			y_advance = -TOOL_ICONHEIGHT;
			break;
	}
	if (vertical) {
		candidate->r_top += y_advance;
		while (candidate->r_top % WMGR_ICON_ALIGN_Y != 0) {
			candidate->r_top += 1; /* y_advance is known > 0 */
		}
		if (candidate->r_top + TOOL_ICONHEIGHT > bounds->r_height) {
			candidate->r_left += x_advance;
			while (candidate->r_left % WMGR_ICON_ALIGN_X != 0) {
				candidate->r_left += (x_advance > 0)  ?  1 : -1;
			}
			if (candidate->r_left < 0
			 || candidate->r_left + TOOL_ICONWIDTH
				> bounds->r_width)  {
				return;	/*  all spots exhausted	*/
			}
			candidate->r_top = 0;
		}
	} else {
		candidate->r_left += x_advance;
		while (candidate->r_left % WMGR_ICON_ALIGN_X != 0) {
			candidate->r_left += 1; /* x_advance is known > 0 */
		}
		if (candidate->r_left + TOOL_ICONWIDTH > bounds->r_width) {
			candidate->r_top += y_advance;
			while (candidate->r_top % WMGR_ICON_ALIGN_Y != 0) {
				candidate->r_top += (y_advance > 0)  ?  1 : -1;
			}
			if (candidate->r_top < 0
			 || candidate->r_top + TOOL_ICONHEIGHT
				> bounds->r_height)  {
				return;	/*  all spots exhausted  */
			}
			candidate->r_left = 0;
		}
        }
}

static void
wmgr_find_icon_slot(root, window, candidate, old_icon)
	Window_handle	 root, window;
	struct rect	*candidate;
	int		 old_icon;
{
	struct rectlist	*rl;
	struct rect	 cached_icon, root_rect;
	Policy_handle	 policy;

	(void)wmgr_is_child_inserted(root, window);
	policy = wmgr_get_icon_policy();
	if (old_icon)  {
		if (!policy->mobile)  {
			return;
		} else {
			cached_icon = *candidate;
		}
	} else {
		if (candidate->r_width == WMGR_SETPOS)  {
			candidate->r_width = TOOL_ICONWIDTH;
		}
		if (candidate->r_height == WMGR_SETPOS)  {
			candidate->r_height = TOOL_ICONHEIGHT;
		}
	}
	(void)win_getrect(root, &root_rect);

	/*
	 *  Try all slots against increasingly laxer policies
	 *  until one passes; that's the choice.
	 */
Compute_free_space:		/*  top of outer loop	*/
	rl = wmgr_find_free_space( root,  window, policy);

Reuse_free_space:		/*  top of inner loop	*/
	if (old_icon)  {
		if (wmgr_test_rect(&cached_icon, rl, policy)) {
			*candidate = cached_icon;    /* old spot's as good   */
			goto Found_place;	     /* as any other; use it */
		}
	}
	if (wmgr_test_slots(candidate, &root_rect, rl, policy) ) {
		goto Found_place;	/* candidate works under this policy */
	}
	/*  if that didn't work, try relaxing something	*/

	if (policy->visibility == Total)  {
		policy->visibility = Partial;
		goto Reuse_free_space;
	}
	if (policy->obstructors == All_windows)  {
		policy->obstructors = Cur_windows;
		goto Compute_free_space;
	}
	if (policy->obstructors == Cur_windows)  {
		policy->obstructors = Icons;	/* this will cycle	*/
		policy->visibility = Total;	/* the inner loop, too	*/
		goto Compute_free_space;
	}
	/*  None of that worked; give up	*/
	if (rect_includesrect(&root_rect, &cached_icon) )  {
		*candidate = cached_icon;
	} else {
		candidate->r_left =
			(root_rect.r_width - TOOL_ICONWIDTH) / 2;
		candidate->r_top =
			(root_rect.r_height - TOOL_ICONHEIGHT) / 2;
	}

Found_place:
	(void)win_setsavedrect(window, candidate);
	(void)rl_free(rl);
	free((char *)(LINT_CAST(rl)));
}

static struct rectlist *
wmgr_find_free_space(root, window, policy)
	Window_handle	root;
	Policy_handle	policy;
{
	struct rectlist	*rl;
	int		 flag;

	switch (policy->obstructors) {
	    case Cur_windows:
		flag = WFS_CURRENT;
		break;
	    case Icons:
		flag = WFS_ICONS;
		break;
	    case Open:
		flag = WFS_OPEN;
		break;
	    case All_windows:
		flag = WFS_BOTH;
		break;
	    default:
	    	flag = WFS_CURRENT;
	    	break;
	}
	rl = wmgr_findspace(root, flag, window);
	return rl;
}

static int
wmgr_test_slots(candidate, root_rect, rl, policy)
	Policy_handle	 policy;
	struct rect	*root_rect;
	struct rect	*candidate;
	struct rectlist	*rl;
{
	for (wmgr_place_first_icon(policy->gravity, root_rect, candidate);
	     rect_includesrect(root_rect, candidate);
	     wmgr_advance_icon(policy->gravity, root_rect, candidate)  )  {
		if (wmgr_test_rect(candidate, rl, policy))  {
			return TRUE;
		}
	}
	return FALSE;
}

/*ARGSUSED*/
int
wmgr_init_icon_position(root)
	Window_handle	root;
{
}

/*ARGSUSED*/
int
wmgr_acquire_next_icon_rect(rect)
	struct rect	rect;
{
}

/*ARGSUSED*/
int
wmgr_inquire_next_icon_rect(rect)
	struct rect	rect;
{
}

int
wmgr_set_icon_rect(rootfd, window, icon_rect)	/* ASSUMES window is OPEN  */
	Window_handle	 rootfd, window;
	struct rect	*icon_rect;
{
	struct rect	 dummy_rect;

	if (icon_rect == NULL)  {
		icon_rect = &dummy_rect;
		(void)win_getsavedrect(window, icon_rect);
	}
	if (icon_rect->r_left == WMGR_SETPOS)  {
		wmgr_find_icon_slot(rootfd, window, icon_rect, FALSE);
	} else {
		wmgr_find_icon_slot(rootfd, window, icon_rect, TRUE);
	}
}

static enum win_enumerator_result
wmgr_find_boundary(window, parms)
	Window_handle	 window;
	Level_parms	*parms;
{
	int		link;

	link = win_fdtonumber(window);
	if (link == parms->self)  {
		return Enum_Normal;
	}
	if (wmgr_iswindowopen(window))  {
		parms->next = link;
		return Enum_Succeed;
	}
	parms->prev = link;
	return Enum_Normal;
}

static void
wmgr_findlevel(rootfd, window, parms)
	Window_handle	 rootfd, window;
	Level_parms	*parms;
{
	int		 self;
	
	self = win_fdtonumber(window);
	switch (parms->level)  {
	  case Back:	parms->prev = WIN_NULLLINK;
			parms->next = win_getlink(rootfd, WL_BOTTOMCHILD);
			if (parms->next == self)  {
			    parms->next = win_getlink(window, WL_COVERING);
			}
			return;
	  case Front:	parms->next = WIN_NULLLINK;
			parms->prev = win_getlink(rootfd, WL_TOPCHILD);
			if (parms->prev == self)  {
			    parms->prev = win_getlink(window, WL_COVERED);
			}
			return;
	  case Boundary: parms->self = win_fdtonumber(window);
			parms->prev = parms->next = WIN_NULLLINK;
			(void)win_enumerate_children(rootfd,
				     wmgr_find_boundary, (caddr_t)parms);
			return;
	}
}

void
wmgr_set_icon_level(rootfd, windowfd)
	Window_handle	rootfd, windowfd;
{
	Policy_handle	policy;
	Level_parms	parms;

	policy = wmgr_get_icon_policy();
	if (policy->close_level == Cur_level)  {
		/* Force repaint */
		(void)win_remove(windowfd);
		(void)win_insert(windowfd);
		return;
	}
	parms.level = policy->close_level;
	wmgr_findlevel(rootfd, windowfd, &parms);
	wmgr_setlevel(windowfd, parms.prev, parms.next);
}

static void
wmgr_get_trimmed_bounds(root, bounds)
	Window_handle	 root;
	struct rect	*bounds;
{
	Policy_handle	 policy;

	policy = wmgr_get_icon_policy();
	(void)win_getrect(root, bounds);
	switch (policy->gravity) {
	  case North:
		bounds->r_top += TOOL_ICONHEIGHT;	/*  FALL THROUGH  */
	  case South:
		bounds->r_height -= TOOL_ICONHEIGHT;
		break;
	  case West:
		bounds->r_left += TOOL_ICONWIDTH;	/*  FALL THROUGH  */
	  case East:
		bounds->r_width -= TOOL_ICONWIDTH;
		break;
	}
}

static void
wmgr_adjust_tool(bounds, candidate)
	register struct rect	*bounds, *candidate;
{
	register int		 delta;

#ifdef NOTDEF
	Policy_handle 	policy;
	
	policy = wmgr_get_icon_policy();
		switch (policy->gravity) {
	    case North:
		bounds->r_top -= TOOL_ICONHEIGHT;	/*  FALL THROUGH  */
	    case South:
		bounds->r_height += TOOL_ICONHEIGHT;
		break;
	    case West:
		bounds->r_left -= TOOL_ICONWIDTH;	/*  FALL THROUGH  */
	    case East:
		bounds->r_width += TOOL_ICONWIDTH;
		break;
	}
#endif
	if (candidate->r_width > bounds->r_width)
		candidate->r_width = bounds->r_width;
	if (candidate->r_height > bounds->r_height)
		candidate->r_height = bounds->r_height;

	delta = rect_right(bounds) - rect_right(candidate);
	if (delta < 0)
		candidate->r_left += delta;
	if (candidate->r_left < bounds->r_left)
		candidate->r_left = bounds->r_left;

	delta = rect_bottom(bounds) - rect_bottom(candidate);
	if (delta < 0)
		candidate->r_top += delta;
	if (candidate->r_top < bounds->r_top)
		candidate->r_top = bounds->r_top;
}

static void
wmgr_advance_tool(root, bounds, candidate)
	Window_handle	 root;
	struct rect	*bounds, *candidate;
{ /*  This routine is a bit hairy.  Its logic is:
   *	since we've been asked to, bump the tool position.
   *	If that moves off the left edge of the screen,
   *	it's time to start a new diagonal line at the top,
   *	over to the right a ways.
   *	Similarly, if this window extends off the bottom,
   *	but it could fit from a reachable position, start a new
   *	diagonal rank.
   *	If that new rank would put the candidate's right edge out of
   *	bounds (and it could fit), come back to the start of the
   *	first rank.
   *	Whatever position we arrive at, stash it in the rect,
   *	and save any new "next" info in the placeholders tool rect.
   */
	struct rect	 next_pos, dummy;

	(void)wmgr_get_placeholders(&next_pos, &dummy);
	next_pos.r_top += WMGR_ADVANCE_TOOL_Y;
	next_pos.r_left += WMGR_ADVANCE_TOOL_X;
	if ((next_pos.r_left < bounds->r_left) ||
	    ((next_pos.r_top + candidate->r_height
		>  bounds->r_top + bounds->r_height) &&
	      candidate->r_height <= bounds->r_height )  )  {
		next_pos.r_left = next_pos.r_width;
		next_pos.r_top = next_pos.r_height;
		next_pos.r_width += WMGR_ADVANCE_TOOL_XRANK;
		next_pos.r_height += WMGR_ADVANCE_TOOL_YRANK;
	}
	if ( next_pos.r_left + candidate->r_width
		    > bounds->r_left + bounds->r_width )  {
		if (candidate->r_width <= bounds->r_width)  {
			(void)wmgr_init_tool_position(root);
			(void)wmgr_get_placeholders(&next_pos, &dummy);
			next_pos.r_top += WMGR_ADVANCE_TOOL_Y;
			next_pos.r_left += WMGR_ADVANCE_TOOL_X;
		}
	}
	(void)wmgr_set_placeholders(&next_pos, &dummy);
	if (candidate->r_left == WMGR_SETPOS)  {
		candidate->r_left = next_pos.r_left;
	}
	if (candidate->r_top == WMGR_SETPOS)  {
		candidate->r_top = next_pos.r_top;
	}
}

int
wmgr_init_tool_position(root)
	Window_handle	 root;
{
	struct rect	bounds, rect, dummy;

	wmgr_get_trimmed_bounds(root, &bounds);
	(void)wmgr_get_placeholders(&rect, &dummy);
	rect.r_left = bounds.r_width / 8 + bounds.r_left - WMGR_ADVANCE_TOOL_X;
	rect.r_top = bounds.r_top - WMGR_ADVANCE_TOOL_Y;;
	rect.r_width = rect.r_left + WMGR_ADVANCE_TOOL_XRANK;
	rect.r_height = bounds.r_top + WMGR_ADVANCE_TOOL_YRANK;
	(void)wmgr_set_placeholders(&rect, &dummy);
}

int
wmgr_acquire_next_tool_rect(rect)
	struct rect	*rect;
{
	struct rect	dummy;

	(void)wmgr_get_placeholders(rect, &dummy);
}

int
wmgr_inquire_next_tool_rect(rect)
	struct rect	rect;
{
	struct rect	dummy;

	(void)wmgr_get_placeholders(&rect, &dummy);
}

int
wmgr_set_tool_rect(root, window, candidate)
	Window_handle	 root, window;
	struct rect	*candidate;
{
	struct rect	 root_rect, dummy_rect;

	if (candidate == NULL)  {
		candidate = &dummy_rect;
		(void)win_getrect(window, candidate);
	}
	if (candidate->r_width	== WMGR_SETPOS ||
	    candidate->r_height	== WMGR_SETPOS)  {
		(void) pw_pfsysopen();
		if (candidate->r_width == WMGR_SETPOS)  {
			candidate->r_width = tool_widthfromcolumns(80);
		}
		if (candidate->r_height == WMGR_SETPOS)  {
			candidate->r_height = tool_heightfromlines(34,TRUE);
		}
		(void)pw_pfsysclose();
	}
	if (candidate->r_left == WMGR_SETPOS ||
	    candidate->r_top == WMGR_SETPOS)  {
		Policy_handle	policy;

		policy = wmgr_get_icon_policy();
		wmgr_get_trimmed_bounds(root, &root_rect);
		if (candidate->r_width > root_rect.r_width) {
			switch (policy->gravity) {
			    case East:
			    	candidate->r_left -= TOOL_ICONWIDTH;
			    case West:
				candidate->r_width += TOOL_ICONWIDTH;
			}
		}
		if (candidate->r_height > root_rect.r_height) {
			switch (policy->gravity) {
			    case North:
			    	candidate->r_top -= TOOL_ICONHEIGHT;
			    case South:
				candidate->r_height += TOOL_ICONHEIGHT;
			}
		}
		wmgr_advance_tool(root, &root_rect, candidate);
	} else {
		(void)win_getrect(root, &root_rect);
	}
	if (!rect_includesrect(&root_rect, candidate))  {
		wmgr_adjust_tool(&root_rect, candidate);
	}
	(void)win_setrect(window, candidate);
}

void
wmgr_set_tool_level(rootfd, windowfd)
	Window_handle	rootfd, windowfd;
{
	Policy_handle	policy;
	Level_parms	parms;

	policy = wmgr_get_icon_policy();
	if (policy->close_level == Cur_level)  {
		/* Force repaint */
		(void)win_remove(windowfd);
		(void)win_insert(windowfd);
		return;
	}
	parms.level = Front;
	wmgr_findlevel(rootfd, windowfd, &parms);
	wmgr_setlevel(windowfd, parms.prev, parms.next);
}
