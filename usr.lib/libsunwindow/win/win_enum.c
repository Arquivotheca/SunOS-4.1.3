#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)win_enum.c 1.1 92/07/30";
#endif
#endif

/*
 * (C) Copyright 1986 Sun Microsystems, Inc.
 */

#include <errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sunwindow/window_hs.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_enum.h>


static int	 win_open_number();

static enum win_enumerator_result
		 stash_node();

static int cmpscreen();
/*
 * Returns -1 if error enumerating, 0 if went smoothly and
 * 1 if func terminated enumeration.
 */
int
win_enumall(func)
    int             (*func) ();

{
    int             winnumber, windowfd;
    struct screen   screen;
    extern int      errno;

    for (winnumber = 0;; winnumber++) {
	/*
	 * Open window. 
	 */
	if ((windowfd = win_open_number(winnumber)) < 0) {
	    if (errno == ENXIO || errno == ENOENT)
		/*
		 * Last window passed. 
		 */
		break;
	    return (-1);
	}
	/*
	 * Try to get window's screen. 
	 */
	if (ioctl(windowfd, WINSCREENGET, &screen) && errno == ESPIPE) {
	    (void)close(windowfd);
	    continue;
	}
	/*
	 * Call function.  If returns non-zero value then return 1. 
	 */
	if ((*func) (windowfd)) {
	    (void)close(windowfd);
	    return (1);
	}
	(void)close(windowfd);
    }
    return (0);
}

/*
 * win_enumscreen accesses all windows on a specified device and applies a
 * specified function to each one.
 * Return codes like win_enumall.
 */

static	int (*callfunc)();
static	struct	screen *matchscreen;

win_enumscreen(screen, func)
    struct screen  *screen;
    int             (*func) ();

{
    callfunc = func;
    matchscreen = screen;
    (void)win_enumall(cmpscreen);
}

static int
cmpscreen(windowfd)
	int     windowfd; {
	struct  screen screen;

	(void)win_screenget(windowfd, &screen); /*
	 * Is this window on the same screen */ if
	(strncmp(screen.scr_fbname, matchscreen->scr_fbname,
		SCR_NAMESIZE) == 0) { return((*callfunc)(windowfd,
		&screen)); } return(0); }

enum win_enumerator_result
win_enumerate_children(window, proc, args)
    Window_handle   window;
    Enumerator      proc;
    caddr_t         args;
{
    int             fd, link;
    enum win_enumerator_result result;

    link = win_getlink(window, WL_OLDESTCHILD);
    while (link != WIN_NULLLINK) {
	if ((fd = win_open_number(link)) < 0) {
	    perror("win_enumerate: window tree malformed");
	    abort();
	}
	result = (*proc) (fd, args);
	if (result != Enum_Normal) {
	    (void)close(fd);
	    return result;
	}
	link = win_getlink(fd, WL_YOUNGERSIB);
	(void)close(fd);
    }
    return Enum_Normal;
}

enum win_enumerator_result
win_enumerate_subtree(window, proc, args)
    Window_handle   window;
    Enumerator      proc;
    caddr_t         args;
{
    int             fd, link;
    enum win_enumerator_result result;

    link = win_getlink(window, WL_OLDESTCHILD);
    while (link != WIN_NULLLINK) {
	if ((fd = win_open_number(link)) < 0) {
	    perror("win_enumerate: window tree malformed:");
	    abort();
	}
	result = (*win_enumerate_subtree) (fd, proc, args);
	if (result != Enum_Normal) {
	    (void)close(fd);
	    return result;
	}
	link = win_getlink(fd, WL_YOUNGERSIB);
	(void)close(fd);
    }
    return (*proc) (window, args);
}

int
win_get_tree_layer(parent, size, buffer)
    Window_handle   parent;
    u_int           size;
    char           *buffer;
{
    int             result;
    Win_tree_layer  layer;

    layer.bytecount = size;
    layer.buffer = (Win_enum_node *) buffer;
    result = ioctl(parent, WINGETTREELAYER, &layer);
    if (result == ENOTTY)
	goto old_style;
    if (result == 0)
	return layer.bytecount;
    else
	return -result;

old_style:{
	enum win_enumerator_result
	                e_result;
	int             sz;

	sz = layer.bytecount;
	e_result = win_enumerate_children(parent, stash_node, (caddr_t) &layer);
	switch (e_result) {
	  case Enum_Fail:
	    return -ENOTTY;
	  default:
	    return sz - layer.bytecount;
	}
    }
}

static int
win_open_number(number)
    int             number;
{
    char            name[WIN_NAMESIZE];

    (void)win_numbertoname(number, name);
    return open(name, O_RDONLY, 0);
}

static enum win_enumerator_result
stash_node(win, layer)
    Window_handle   win;
    Win_tree_layer *layer;
{
    register        Win_enum_node
    *               nodep;

    nodep = layer->buffer++;
    if (layer->bytecount < sizeof(Win_enum_node))  {
    	return Enum_Fail;
    }   else   {
    	layer->bytecount -= sizeof(Win_enum_node);
    }
    nodep->me = win_fdtonumber(win);
    nodep->parent = win_getlink(win, WL_PARENT);
    nodep->upper_sib = win_getlink(win, WL_YOUNGERSIB);
    nodep->lowest_kid = win_getlink(win, WL_BOTTOMCHILD);
    nodep->flags = win_getuserflags(win);
    if (nodep->flags & WUF_WMGR1) {
	(void)win_getsavedrect(win, &nodep->open_rect);
	(void)win_getrect(win, &nodep->icon_rect);
    } else {
	(void)win_getsavedrect(win, &nodep->open_rect);
	(void)win_getrect(win, &nodep->icon_rect);
    }
    if (nodep->upper_sib == (unsigned char)WIN_NULLLINK)
	return Enum_Succeed;
    else
	return Enum_Normal;
}
