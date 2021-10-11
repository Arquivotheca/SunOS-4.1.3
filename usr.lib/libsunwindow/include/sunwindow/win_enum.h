/*      @(#)win_enum.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef sunwindow_win_enum_DEFINED
#define sunwindow_win_enum_DEFINED	1

#include <sunwindow/rect.h>

enum win_enumerator_result { Enum_Normal, Enum_Succeed, Enum_Fail };

typedef enum win_enumerator_result
		(*Enumerator)();

typedef int	  Window_handle;


/*  For a fast window enumerator in user-space */

typedef struct win_enum_node	{
	unsigned char	me;
	unsigned char	parent;
	unsigned char	upper_sib;
	unsigned char	lowest_kid;
	unsigned int	flags;
#define WIN_NODE_INSERTED     0x1
#define WIN_NODE_OPEN         0X2
#define WIN_NODE_IS_ROOT      0x4
	Rect		open_rect;
	Rect		icon_rect;
}   Win_enum_node;

typedef struct win_tree_layer	{
	unsigned int	bytecount;
	Win_enum_node  *buffer;
}   Win_tree_layer;

#ifndef KERNEL
enum win_enumerator_result
win_enumerate_children(/* window, proc, args */);
/*	Window_handle	window;
 *	Enumerator	proc;
 *	caddr_t		args;
 *
 *	invoke (*proc)(fd, args) with an fd for each immediate descendant
 *	of "window" in oldest-to-youngest order, stopping when told to
 *	by proc or when all have been enumerated.
 *	Return the reason for stopping.
 */

int
win_get_tree_layer( /* window, size, buffer */ );
/*	Window_handle	window;
 *	int		size;
 *	Win_tree_layer	buffer;
 *
 *	fill buffer with Win_enum_node information (rects, user_flags, and
 *	minimal links) for the children of "window" in oldest-to-youngest
 *	order.  A much more efficient alternative to win_enumerate_children
 *	for window management.  Data definitions in win_struct.h.
 */


enum win_enumerator_result
win_enumerate_subtree(/* window, proc, args */);
/*	Window_handle	window;
 *	Enumerator	proc;
 *	caddr_t		args;
 *
 *	invoke (*proc)(fd, args) with an fd for "window" and each of
 *	its descendants in depth-first, oldest-to-youngest order,
 *	stopping when told to by proc or when all have been enumerated.
 *	Return the reason for stopping.
 
 *	CAUTION: can fail for lack of fd's on deeply-nested windows.
 */

/*
 *	Following routines are maintained for backwards compatibility
 */

int
win_enumall(/* proc */);
/*	int	(*proc)();
 *
 *	open each window on the system in numerical order (/dev/win0 ...)
 *	apply "proc" to it if it is currently in use.
 *	Return -1 if error enumerating, 0 if windows exhausted, 1 if
 *	"proc" returned non-0
 */

int
win_enumscreen(/* screen, proc */);
/*	struct screen	 *screen;
 *	int		(*proc)();
 *
 *	apply "proc" to each window in use on "screen"
 *
 *	CAUTION: not re-entrant -- uses static data.
 */
#endif !KERNEL

#endif sunwindow_win_enum_DEFINED
