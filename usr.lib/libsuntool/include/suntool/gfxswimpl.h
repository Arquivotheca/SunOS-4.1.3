/*	@(#)gfxswimpl.h 1.1 92/07/30	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Overview: Definitions PRIVATE to the implement the abstraction
 *		defined in gfxsw.h.  Nothing in this file is supported or
 *		public.
 */

/*
 * Takeover window private data
 * Don't keep parentname fd open so that if the owner process of the parent
 * window goes away then the graphics process will too.
 */
struct	gfx_takeover {
	char	gt_parentname[WIN_NAMESIZE]; /* Parent of gfx->gfx_windowfd */
	struct	tool *gt_tool;		/* Used with gfxsw_select */
	int	(*gt_selected)();	/* Used with gfxsw_select */
	int	gt_rootfd;		/* Not -1 if created new screen */
};
struct	gfx_takeover nullgt;


#define	gfxisretained(g) ((g)->gfx_pixwin->pw_prretained)
#define	gfxgettakeover(g) ((struct gfx_takeover *)LINT_CAST((g)->gfx_takeoverdata))
#define	gfxmadescreen(gt) ((gt)->gt_rootfd != -1)

