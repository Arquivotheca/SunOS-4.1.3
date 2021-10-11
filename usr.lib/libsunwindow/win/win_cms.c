#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)win_cms.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Pw_cms.c: Implement the colormap segment sharing aspect of
 *		the pixwin.h|cms.h|cms*.h interfaces.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>

/*
 * Colormap segment manager operations.
 */
win_setcms(windowfd, cms, cmap)
	int	windowfd;
	struct	colormapseg *cms;
	struct	cms_map *cmap;
{
	struct	cmschange cmschange;

	cmschange.cc_cms = *cms;
	cmschange.cc_map = *cmap;
	(void)werror(ioctl(windowfd, WINSETCMS, &cmschange), WINSETCMS);
	*cms = cmschange.cc_cms;
}

win_getcms(windowfd, cms, cmap)
	int	windowfd;
	struct	colormapseg *cms;
	struct	cms_map *cmap;
{
	struct	cmschange cmschange;

	cmschange.cc_cms = *cms;
	cmschange.cc_map = *cmap;
	(void)werror(ioctl(windowfd, WINGETCMS, &cmschange), WINGETCMS);
	*cms = cmschange.cc_cms;
}

win_clearcms(windowfd)
	int	windowfd;
{
	struct	colormapseg cms;
	struct	cms_map cmap;

	cms.cms_size = 0;
	cms.cms_addr = 0;
	cms.cms_name[0] = '\0';
	cmap.cm_red = 0;
	cmap.cm_green = 0;
	cmap.cm_blue = 0;
	(void)win_setcms(windowfd, &cms, &cmap);
}

