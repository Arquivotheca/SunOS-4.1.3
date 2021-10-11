/*	@(#)tool_impl.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Overview: Definitions PRIVATE to the implement the abstraction
 *		defined in tool.h.  Nothing in this file is supported or
 *		public.
 */


#define	TOOL_ATTR_MAX	101	/* number of attribute value slots supported */

#define	TOOL_DEFAULT_CMS	TOOL_FIRSTPRIV	/* use tool's colormap
						   segment as default */
#define	TOOL_REPAINT_LOCK	(TOOL_FIRSTPRIV<<1)	/* Disable repainting */
/*	TOOL_LAYOUT_LOCK	(TOOL_FIRSTPRIV<<2)	must be in tool.h */
#define	TOOL_DYNAMIC_STORAGE	(TOOL_FIRSTPRIV<<3)	/* most tool struct data
							  dynamically allocated 
							 */
#define	TOOL_NOTIFIER		(TOOL_FIRSTPRIV<<4)	/* Using notifier for
							 * tool specific
							 * notifications */
#define	TOOL_DESTROY		(TOOL_FIRSTPRIV<<5)	/* Trying to destroy.
							 * May be vetoed. */
#define	TOOL_NO_CONFIRM		(TOOL_FIRSTPRIV<<6)	/* Don't put up std tool
							 * confirmation msg. */

/*
 * Definitions to support the default user interface.
 */
#define	TOP_KEY		ACTION_FRONT
#define UN_TOP_KEY	ACTION_BACK
#define	OPEN_KEY	ACTION_OPEN
#define UN_OPEN_KEY	ACTION_CLOSE
#define DELETE_KEY      ACTION_CUT
#define PROPS_KEY	ACTION_PROPS

extern	char *tool_copy_attr();
extern	tool_debug_attr;

/*
 * Structure used to expand toolsw structure with tool implementation
 * private data.
 */
typedef struct	toolsw_priv {
	int dummy;		/* Placeholder */
	bool have_kbd_focus;	/* TRUE if toolsw has kbd focus */
} Toolsw_priv;
#define	TOOLSW_PRIV_NULL	((Toolsw_priv *)0)

extern	Toolsw * tool_sw_from_client();	/* (Tool *tool, Notify_client client) */
extern  Notify_value tool_input();


