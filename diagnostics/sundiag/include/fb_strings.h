/*      @(#)fb_strings.h 1.1 92/07/30 SMI          */
 
/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc. 
 */

struct fb_desc {
	int fb_type;
	char *fb_name;
} frame_buffs[] = {
	{ FBTYPE_SUN1BW,	"Sun-1 black and white frame buffer" },
	{ FBTYPE_SUN1COLOR,	"Sun-1 color graphics interface" },
	{ FBTYPE_SUN2BW,	"Sun-3/Sun-2 black and white frame buffer" },
	{ FBTYPE_SUN2COLOR,	"Sun-3/Sun-2 color graphics interface" },
	{ FBTYPE_SUN2GP,	"Sun-3/Sun-2 graphics processor" },
	{ FBTYPE_SUN3COLOR,	"reserved for future Sun use" },
	{ FBTYPE_SUN4COLOR,	"Sun-3 prism color graphics interface" },
	{ FBTYPE_NOTSUN1,	"reserved for customer" },
	{ FBTYPE_NOTSUN2,       "reserved for customer" },
	{ FBTYPE_NOTSUN3,       "reserved for customer" },
	{ 0 }
};
