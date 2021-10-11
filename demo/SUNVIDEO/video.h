/*	@(#)video.h 1.1 92/07/30 Copyr 1988 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef video_DEFINED
#define video_DEFINED

#include <sunwindow/attr.h>
#include <sun/tvio.h>

typedef caddr_t	Video;

#define	VIDEO_ATTR(type, ordinal)	ATTR(ATTR_PKG_VIDEO, type, ordinal)

typedef enum {
    VIDEO_PIXRECT		= VIDEO_ATTR(ATTR_PIXRECT_PTR, 1),
    VIDEO_X_OFFSET		= VIDEO_ATTR(ATTR_X, 2),
    VIDEO_Y_OFFSET		= VIDEO_ATTR(ATTR_Y, 3),
    VIDEO_RESIZE_PROC	= VIDEO_ATTR(ATTR_FUNCTION_PTR, 4),
    VIDEO_LIVE			= VIDEO_ATTR(ATTR_INT, 5),
    VIDEO_INPUT			= VIDEO_ATTR(ATTR_INT, 6),
    VIDEO_SYNC			= VIDEO_ATTR(ATTR_INT, 7),
    VIDEO_CHROMA_SEPARATION	= VIDEO_ATTR(ATTR_INT, 8),
    VIDEO_DEMOD			= VIDEO_ATTR(ATTR_INT, 9),
    VIDEO_R_GAIN		= VIDEO_ATTR(ATTR_INT, 10),
    VIDEO_G_GAIN		= VIDEO_ATTR(ATTR_INT, 11),
    VIDEO_B_GAIN		= VIDEO_ATTR(ATTR_INT, 12),
    VIDEO_R_LEVEL		= VIDEO_ATTR(ATTR_INT, 13),
    VIDEO_G_LEVEL		= VIDEO_ATTR(ATTR_INT, 14),
    VIDEO_B_LEVEL		= VIDEO_ATTR(ATTR_INT, 15),
    VIDEO_CHROMA_GAIN	= VIDEO_ATTR(ATTR_INT, 16),
    VIDEO_LUMA_GAIN		= VIDEO_ATTR(ATTR_INT, 17),
    VIDEO_OUTPUT		= VIDEO_ATTR(ATTR_INT, 18),
    VIDEO_COMPRESSION	= VIDEO_ATTR(ATTR_INT, 19),
    VIDEO_SYNC_ABSENT	= VIDEO_ATTR(ATTR_BOOLEAN, 20),
    VIDEO_BURST_ABSENT	= VIDEO_ATTR(ATTR_BOOLEAN, 21),
    VIDEO_INPUT_STABLE	= VIDEO_ATTR(ATTR_BOOLEAN, 22),
    VIDEO_GENLOCK		= VIDEO_ATTR(ATTR_BOOLEAN, 23),
    VIDEO_COMP_OUT		= VIDEO_ATTR(ATTR_INT, 24)
} Video_attribute;

/* useful macros
 */
#define	video_pixrect(video)	(Pixrect *) (LINT_CAST(window_get(video, VIDEO_PIXRECT)))

/* video creation routine */
#define	VIDEO			video_window_object
extern caddr_t			video_window_object();

/* video window type */
#define	VIDEO_TYPE	ATTR_PKG_VIDEO

/* Values for sync and video formats */
#define VIDEO_NTSC  TVIO_NTSC
#define VIDEO_YC    TVIO_YC
#define VIDEO_RGB   TVIO_RGB
#define	VIDEO_YUV   TVIO_YUV

/* Values for Zooming */
#define VIDEO_2X	0, VIDEO_Y_OFFSET, 29
#define VIDEO_1X	1, VIDEO_Y_OFFSET, 29
#define VIDEO_1_2	2, VIDEO_Y_OFFSET, 15
#define VIDEO_1_4	3, VIDEO_Y_OFFSET, 7
#define VIDEO_1_8	4, VIDEO_Y_OFFSET, 4
#define VIDEO_1_16	5, VIDEO_Y_OFFSET, 2

/* Values for Chroma demodulation */
#define VIDEO_DEMOD_AUTO    TVIO_AUTO
#define VIDEO_DEMOD_ON	    TVIO_ON
#define VIDEO_DEMOD_OFF	    TVIO_OFF

/* Video output to the output connectors or to the Workstation */
#define VIDEO_VIDEO_OUT	    0
#define VIDEO_ONSCREEN	    1

#endif not video_DEFINED
