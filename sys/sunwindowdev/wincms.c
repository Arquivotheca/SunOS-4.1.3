#ifndef lint
static	char sccsid[] = "@(#)wincms.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * SunWindows Desktop colormap sharing code.
 */

#include <sunwindowdev/wintree.h>
#include <sys/kmem_alloc.h>

int	winenforcecmstd = 1;
void	dtop_set_bkgnd();

int
dtop_cmsalloc(dtop, size)
	register struct desktop *dtop;
	long size;
{
	int	addr = cms_alloc((struct map *)dtop->dt_rmp, size);

	if (addr == 0)
		return (-1);
	else
		return (addr - CRM_ADDROFFSET);
}

dtop_cmsfree(dtop, w)
	register struct desktop *dtop;
	register struct window *w;
{
	struct window *dtop_cmsmatch();

	/*
	 * Releasing color map segment.
	 */
	if (dtop->dt_cmsize && w->w_cms.cms_size &&
	    (dtop_cmsmatch(dtop, w, &w->w_cms) == NULL)) {
		if (w->w_flags & WF_CMSHOG)
			win_freecmapdata(w);
		else {
			/*
			 * Guard against the case in which w was allocated,
			 * a cms segment was assigned, the dtop was closed,
			 * the w->w_pid was left hung, a new dtop was created,
			 * w->w_pid was waken to end up on the new dtop,
			 * w is eventually closed, w's previously assigned
			 * cms segment (now invalid) is about to be freed
			 * (which would cause a panic.
			 */
			if (!(dtop->dt_rootwin &&
			    dtop->dt_rootwin->w_pid > w->w_pid))
				rmfree((struct map *)dtop->dt_rmp,
				    (long)w->w_cms.cms_size,
				    (u_long)(w->w_cms.cms_addr + CRM_ADDROFFSET));
			/* Reset addr 0 because special for fullscreen access */
			if (w->w_cms.cms_addr == 0)
				dtop_set_bkgnd(dtop);
		}
	}
	w->w_flags &= ~WF_CMSHOG;
	bzero((caddr_t)&w->w_cms, sizeof (struct colormapseg));
	bzero((caddr_t)&w->w_cmap, sizeof (struct cms_map));
	w->w_cmapdata = 0;
}

struct	window *
dtop_cmsmatch(dtop, w, cms)
	struct desktop *dtop;
	struct window *w;
	struct colormapseg *cms;
{
	register int	wi;
	register struct	window *wtmp;

	/*
	 * Must search all open windows instead of enumerating dtop because
	 * need to match cms's whose windows are not in the tree currently.
	 */
	for (wi = 0; wi != win_nwindows; wi++) {
		wtmp = winbufs[wi];
		if (wtmp != NULL && wtmp != w &&
		    (wtmp->w_flags & WF_OPEN) &&
		    wtmp->w_desktop == dtop &&
		    strncmp(cms->cms_name, wtmp->w_cms.cms_name,
			CMS_NAMESIZE) == 0)
				return (wtmp);
	}
	return (NULL);
}

caddr_t
dtop_alloccmapdata(dtop, cmap, size)
	struct	desktop *dtop;
	register struct cms_map *cmap;
	int	size;
{
	caddr_t	cmapdata = new_kmem_zalloc((u_int)3*size, KMEM_SLEEP);

	if (cmapdata == 0) {
		printf("Couldn't allocate colormap buffer\n");
		cms_freecmapdata(cmapdata, cmap, size);
	} else {
		cmap->cm_red = (u_char *)cmapdata;
		cmap->cm_green = cmap->cm_red + size;
		cmap->cm_blue = cmap->cm_green + size;
		dtop_stdizecmap(dtop, cmap, 0, size);
	}
	return (cmapdata);
}

dtop_stdizecmap(dtop, cmap, addr, size)
	register struct	desktop *dtop;
	register struct cms_map *cmap;
	int	addr, size;
{
	register struct	singlecolor *bkgnd = &dtop->dt_screen.scr_background;
	register struct	singlecolor *frgnd = &dtop->dt_screen.scr_foreground;
	struct	singlecolor *tmpgnd;
	register int fgnd_index = addr + size - 1, bgnd_index = addr;

	/*
	 * Standardize colormap by setting the foreground and background
	 * only if they are the same.  This is meant to reduce disasterous
	 * blanking of the screen by errant programs.
	 */
	if (winenforcecmstd && (size > 0) &&
	    cmap->cm_red[bgnd_index] == cmap->cm_red[fgnd_index] &&
	    cmap->cm_green[bgnd_index] == cmap->cm_green[fgnd_index] &&
	    cmap->cm_blue[bgnd_index] == cmap->cm_blue[fgnd_index]) {
		/*
		 * Use foreground and background from dtop.
		 * These were made sure to be different in user process
		 * that called WIN_SCREENNEW (see sunwindow/win_screen.c).
		 */
		if (dtop->dt_screen.scr_flags & SCR_SWITCHBKGRDFRGRD) {
			tmpgnd = bkgnd;
			bkgnd = frgnd;
			frgnd = tmpgnd;
		}
		cmap->cm_red[bgnd_index] = bkgnd->red;
		cmap->cm_green[bgnd_index] = bkgnd->green;
		cmap->cm_blue[bgnd_index] = bkgnd->blue;
		cmap->cm_red[fgnd_index] = frgnd->red;
		cmap->cm_green[fgnd_index] = frgnd->green;
		cmap->cm_blue[fgnd_index] = frgnd->blue;
	}
}

/*
 * Colormap specific routines
 */
cms_freecmapdata(cmapdata, cmap, size)
	register caddr_t cmapdata;
	register struct cms_map *cmap;
	int	size;
{
	cmap->cm_red = 0;
	cmap->cm_green = 0;
	cmap->cm_blue = 0;
	if (cmapdata == 0)
		return;
	kmem_free((caddr_t)cmapdata, (u_int)3*size);
}

int
cms_alloc(rmp, size)
	struct map *rmp;
	long size;
{
	register struct mapent *ep = (struct mapent *)(rmp + 1);
	register struct mapent *bp;
	register int expo, addr, addrlastok = -1;

	/*
	 * If size is not power of 2 then need to alloc at addr == 0.
	 */
	for (expo = 1; expo <= 8 /* 2**8==256 */; ++expo) {
		if (size == (1 << expo)) {
			/*
			 * Addr % size must be zero.
			 * Try to allocate from end of space in order to leave
			 * room at addr == 0 for large consumers.
			 */
			for (bp = ep; bp->m_size; bp++) {
				addr = (bp->m_addr - CRM_ADDROFFSET) +
				    bp->m_size - size;
				addr -= addr % size;
				if (addr >= (bp->m_addr - CRM_ADDROFFSET) &&
				    addr % size == 0)
					addrlastok = addr;
			}
			if (addrlastok != -1)
				return (rmget(rmp, size,
				    (u_long)(addrlastok + CRM_ADDROFFSET)));
			else
				return (0);
		}
	}
	return (rmget(rmp, size, (u_long)(0 + CRM_ADDROFFSET)));
}

void
dtop_set_bkgnd(dtop)
	register Desktop *dtop;
{
	register struct	singlecolor *bkgnd;

	if (dtop->dt_screen.scr_flags & SCR_SWITCHBKGRDFRGRD)
		bkgnd = &dtop->dt_screen.scr_foreground;
	else
		bkgnd = &dtop->dt_screen.scr_background;
	dtop->dt_cmap.cm_red[0] = bkgnd->red;
	dtop->dt_cmap.cm_green[0] = bkgnd->green;
	dtop->dt_cmap.cm_blue[0] = bkgnd->blue;
}

void
dtop_save_grnds(dtop)
	register Desktop *dtop;
{
	int fg = dtop->dt_cmsize - 1;

	dtop->dt_bkgnd_cache.red = dtop->dt_cmap.cm_red[0];
	dtop->dt_bkgnd_cache.green = dtop->dt_cmap.cm_green[0];
	dtop->dt_bkgnd_cache.blue = dtop->dt_cmap.cm_blue[0];
	dtop->dt_frgnd_cache.red = dtop->dt_cmap.cm_red[fg];
	dtop->dt_frgnd_cache.green = dtop->dt_cmap.cm_green[fg];
	dtop->dt_frgnd_cache.blue = dtop->dt_cmap.cm_blue[fg];
}

void
dtop_restore_grnds(dtop)
	register Desktop *dtop;
{
	int fg = dtop->dt_cmsize - 1;

	dtop->dt_cmap.cm_red[0] = dtop->dt_bkgnd_cache.red;
	dtop->dt_cmap.cm_green[0] = dtop->dt_bkgnd_cache.green;
	dtop->dt_cmap.cm_blue[0] = dtop->dt_bkgnd_cache.blue;
	dtop->dt_cmap.cm_red[fg] = dtop->dt_frgnd_cache.red;
	dtop->dt_cmap.cm_green[fg] = dtop->dt_frgnd_cache.green;
	dtop->dt_cmap.cm_blue[fg] = dtop->dt_frgnd_cache.blue;
}
