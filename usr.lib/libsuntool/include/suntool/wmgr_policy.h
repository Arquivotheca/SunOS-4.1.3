/*      @(#)wmgr_policy.h 1.1 92/07/30 SMI      */

#ifndef wmgr_policy_DEFINED
#define wmgr_policy_DEFINED	1

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include <ctype.h>
#include <stdio.h>
#include <sunwindow/win_enum.h>
#include <suntool/tool_hs.h>
#include <suntool/wmgr.h>

#define WMGR_ICON_ALIGN_X	  4	/*  grid for placing icons	*/
#define WMGR_ICON_ALIGN_Y	  4
#define WMGR_ADVANCE_TOOL_X	-18	/*  offsets by which one tool	*/
#define WMGR_ADVANCE_TOOL_Y	 18	/*  follows another in a rank	*/
#define WMGR_ADVANCE_TOOL_XRANK	100	/*  offset from one diagonal    */
#define WMGR_ADVANCE_TOOL_YRANK	  3	/*  rank of tools to the next	*/

#define WFS_CURRENT		  1
#define WFS_ICONS		  2
#define WFS_OPEN		  3
#define WFS_BOTH		  4

enum wmp_gravity	{ North, East, South, West };
enum wmp_close_level	{ Front, Cur_level, Boundary, Back };
enum wmp_visibility	{ Total, Partial };
enum wmp_obstructors	{ Cur_windows, Icons, Open, All_windows };

typedef enum wmp_gravity	 Gravity;
typedef enum wmp_close_level	 Close_level; 
typedef enum wmp_visibility	 Visibility; 
typedef enum wmp_obstructors	 Obstructors;

struct wmgr_policy  {
	Gravity		gravity;
	Close_level	close_level;
	Visibility	visibility;
	Obstructors	obstructors;
	int		mobile;
};
typedef struct wmgr_policy	*Policy_handle;

Policy_handle	 wmgr_get_icon_policy();

int		 wmgr_is_child_inserted();

struct rectlist	*wmgr_findspace();



#endif
