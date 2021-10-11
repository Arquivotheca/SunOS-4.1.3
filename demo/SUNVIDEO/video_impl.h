
/*      @(#)video_impl.h 1.1 92/07/30 SMI      */

/*      @(#)video_impl.h 1.1 92/07/30 Copyr 1988 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sunwindow/window_hs.h>
#include <suntool/window.h>
#ifdef ecd.help
#include <suntool/help.h>
#endif
#include "video.h"

typedef void	(*Function)();

#define BIT_FIELD(field)        int field : 1

typedef struct {
    Pixrect	*pr;		/* Video pixrect */
    Rect	rect;		/* object left, top, width, height */
    Rect	win_rect;	/* cached window width & height */
    int		screen_x;	/* Current screen x */
    int		screen_y;	/* Current screen y */
#ifdef ecd.help
    caddr_t	help_data;	/* help data handle */
#endif
    Function 	resize_proc;
    struct {
	BIT_FIELD(first_resize);        /* resize proc called at least once */
    } status_bits;
} Video_info;

#define status(canvas, field)           ((canvas)->status_bits.field)
#define status_set(canvas, field)       status(canvas, field) = TRUE
#define status_reset(canvas, field)     status(canvas, field) = FALSE

#define	win_width(video)	((video)->win_rect.r_width)
#define	win_height(video)	((video)->win_rect.r_height)

#define	video_rect(video)	(&(video)->rect)

#define video_x_offset(video)	(video)->rect.r_left
#define video_y_offset(video)	(video)->rect.r_top
#define video_width(video)	(video)->rect.r_width
#define video_height(video)	(video)->rect.r_height

#define video_set_x_offset(video, val)	video_x_offset(video) = val
#define video_set_y_offset(video, val)	video_y_offset(video) = val
#define video_set_width(video, val)	video_width(video) = val
#define video_set_height(video, val)	video_height(video) = val

#define video_screen_x(video)	((video)->screen_x)
#define video_screen_y(video)	((video)->screen_y)
#define video_set_screen_x(video, val) (video_screen_x(video)=(val))
#define video_set_screen_y(video, val) (video_screen_y(video)=(val))

#define	video_pr(video)		((video->pr))
#define	video_set_pr(video, pr)	video_pr(video) = pr
