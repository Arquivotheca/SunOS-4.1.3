/*	@(#)cms.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * A colormapseg is a segment of a colormap that was allocated
 * to a user process.  It is used to multiplex a colormap in a
 * shared resource environment, i.e., multi-windows & multi-processes.
 *
 * The cms_addr serves as a base address of a pixel.  Cms_addr is added
 * with pixel values to get the true value of the colormap index.
 * The value of a colormap entry is translated to the video signal that
 * is sent to a monitor for a single pixel.
 *
 * Cms_size and cms_addr can be used by low level routines to provide some
 * protection from using a part of the colormap not allocated to the cms
 * by doing a bounds check on the pixel that is to be written.
 *
 * Cms_map is a structure that has pointers to the bytes of rgb for
 * a corresponding colormapseg.  The choice of this format enables
 * fast read/write to most colormaps.
 */

#ifndef cms_DEFINED

#define	cms_DEFINED

#define	CMS_NAMESIZE	20

struct colormapseg {
	int	cms_size;	/* size of this segment of the map */
	int	cms_addr;	/* colormap addr of start of segment */
	char	cms_name[CMS_NAMESIZE]; /* name segment of map (for sharing)*/
};

struct	cms_map {
	unsigned char *cm_red, *cm_green, *cm_blue;
};

#endif
