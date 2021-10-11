/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)rasterfileio.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#include <rasterfile.h>

/*
 * Save a raster to a file, with optional replication.
 */
raster_to_file(raster, map, rasfid, n)
    rast_type *raster;
    colormap_type *map;
    int rasfid, n;
{
    int i;
    char *funcname;
    int linesize, replsize;
    unsigned char *linepointer, *locline;
    unsigned short lastmask;
    struct rasterfile hdr;

    funcname = "raster_to_file";
    /* copy the raster */
    hdr.ras_magic = RAS_MAGIC;
    hdr.ras_width = raster->width;
    hdr.ras_height = raster->height;
    hdr.ras_depth = raster->depth;
    hdr.ras_type = 1;
    hdr.ras_maptype = map->type;
    hdr.ras_maplength = map->nbytes;

    if (hdr.ras_depth == 1) {
	linesize = ((hdr.ras_width + 15) >> 4) * 2;	/* bytes in raster */
	replsize = ((hdr.ras_width * 2 + 15) >> 4) * 2;	/* bytes in raster */
	lastmask = ~((1 << (16 - (hdr.ras_width & 0xF))) - 1);
    } else if (hdr.ras_depth == 8) {
	linesize = (hdr.ras_width + 1) & ~1;
	replsize = hdr.ras_width * 2;
	lastmask = (linesize == hdr.ras_width) ? 0xFF : 0xF0;
    } else {			/* must be 1 or 8 bit pixels */
	_core_errhand(funcname, 86);
	return (86);
    }
    switch (n) {		/* handle pixel replication */
	default:
	case 1:
	    hdr.ras_length = hdr.ras_height * linesize;
	    write(rasfid, (char *) &hdr, sizeof(hdr));	/* write image header */
	    write(rasfid, map->data, map->nbytes);	/* write colormap data   */
	    linepointer = (unsigned char *) raster->bits;
	    *((short *) (linepointer + linesize - 2)) &= lastmask;
	    for (i = 0; i < hdr.ras_height; i++) {
		write(rasfid, (char *) linepointer, linesize);	/* write image data   */
		linepointer += linesize;
		*((short *) (linepointer + linesize - 2)) &= lastmask;
	    }
	    break;
	case 2:
	    hdr.ras_width *= 2;
	    hdr.ras_height *= 2;
	    hdr.ras_length = hdr.ras_height * replsize;
	    write(rasfid, (char *) &hdr, sizeof(hdr));	/* write image header */
	    write(rasfid, map->data, map->nbytes);	/* write colormap data   */
	    linepointer = (unsigned char *) raster->bits;
	    *((short *) (linepointer + linesize - 2)) &= lastmask;
	    locline = (unsigned char *) malloc((unsigned int) replsize);
	    for (i = 0; i < hdr.ras_height / 2; i++) {	/* write image data   */
		if (hdr.ras_depth == 1)
		    replicatebit(linepointer, (unsigned short *) locline, replsize);
		else
		    replicatebyte(linepointer, (unsigned short *) locline, replsize);
		write(rasfid, (char *) locline, replsize);
		write(rasfid, (char *) locline, replsize);
		linepointer += linesize;
		*((short *) (linepointer + linesize - 2)) &= lastmask;
	    }
	    free((char *) locline);
	    break;
    }
    return (0);
}

/*
 * Replication table
 */
static unsigned short repltab[] = {
       0, 3, 12, 15, 48, 51, 60, 63, 192, 195, 204, 207, 240, 243, 252, 255,
     768, 771, 780, 783, 816, 819, 828, 831, 960, 963, 972, 975, 1008, 1011,
     1020, 1023, 3072, 3075, 3084, 3087, 3120, 3123, 3132, 3135, 3264, 3267,
     3276, 3279, 3312, 3315, 3324, 3327, 3840, 3843, 3852, 3855, 3888, 3891,
   3900, 3903, 4032, 4035, 4044, 4047, 4080, 4083, 4092, 4095, 12288, 12291,
12300, 12303, 12336, 12339, 12348, 12351, 12480, 12483, 12492, 12495, 12528,
12531, 12540, 12543, 13056, 13059, 13068, 13071, 13104, 13107, 13116, 13119,
13248, 13251, 13260, 13263, 13296, 13299, 13308, 13311, 15360, 15363, 15372,
15375, 15408, 15411, 15420, 15423, 15552, 15555, 15564, 15567, 15600, 15603,
15612, 15615, 16128, 16131, 16140, 16143, 16176, 16179, 16188, 16191, 16320,
16323, 16332, 16335, 16368, 16371, 16380, 16383, 49152, 49155, 49164, 49167,
49200, 49203, 49212, 49215, 49344, 49347, 49356, 49359, 49392, 49395, 49404,
49407, 49920, 49923, 49932, 49935, 49968, 49971, 49980, 49983, 50112, 50115,
50124, 50127, 50160, 50163, 50172, 50175, 52224, 52227, 52236, 52239, 52272,
52275, 52284, 52287, 52416, 52419, 52428, 52431, 52464, 52467, 52476, 52479,
52992, 52995, 53004, 53007, 53040, 53043, 53052, 53055, 53184, 53187, 53196,
53199, 53232, 53235, 53244, 53247, 61440, 61443, 61452, 61455, 61488, 61491,
61500, 61503, 61632, 61635, 61644, 61647, 61680, 61683, 61692, 61695, 62208,
62211, 62220, 62223, 62256, 62259, 62268, 62271, 62400, 62403, 62412, 62415,
62448, 62451, 62460, 62463, 64512, 64515, 64524, 64527, 64560, 64563, 64572,
64575, 64704, 64707, 64716, 64719, 64752, 64755, 64764, 64767, 65280, 65283,
65292, 65295, 65328, 65331, 65340, 65343, 65472, 65475, 65484, 65487, 65520,
				   65523, 65532, 65535};
static 
replicatebit(iptr, optr, bytes)
    register unsigned char *iptr;
    register unsigned short *optr;
    int bytes;
{
    register int i;

    for (i = 0; i < bytes / 2; i++) {	/* for all input bytes */
	*optr++ = repltab[*iptr++];	/* produce output word */
    }
}

static 
replicatebyte(iptr, optr, bytes)
    register unsigned char *iptr;
    register unsigned short *optr;
    int bytes;
{
    register int i;
    register unsigned short w;

    for (i = 0; i < bytes / 2; i++) {	/* for all input bytes */
	w = *iptr++;		/* produce output word */
	*optr++ = w | (w << 8);
    }
}
static char *mapptr = 0;

file_to_raster(rasfid, raster, map)
    int rasfid;
    rast_type *raster;
    colormap_type *map;
{
    int i, errnum;
    char *funcname;
    int linesize;
    char *linepointer;
    struct rasterfile hdr;

    errnum = 0;
    funcname = "file_to_raster";

    read(rasfid, (char *) &hdr, sizeof(hdr));	/* read the image file header */
    if (hdr.ras_magic != RAS_MAGIC)
	return (1);
    if (hdr.ras_type != 0 && hdr.ras_type != 1) {
	_core_errhand(funcname, 114);
	return (1);
    }
    raster->width = hdr.ras_width;
    raster->height = hdr.ras_height;
    raster->depth = hdr.ras_depth;
    map->type = hdr.ras_maptype;
    map->nbytes = hdr.ras_maplength;
    if (mapptr)
	free(mapptr);
    mapptr = (char *) malloc((unsigned int) map->nbytes);
    map->data = mapptr;

    if (hdr.ras_depth == 1) {	/* compute depth dependent data lengths */
	linesize = ((hdr.ras_width + 15) >> 4) * 2;	/* bitmap line padded to
							 * word */
    } else if (hdr.ras_depth == 8) {
	linesize = (hdr.ras_width + 1) & ~1;
    } else {
	_core_errhand(funcname, 86);
	return (1);
    }

    read(rasfid, map->data, map->nbytes);	/* read colormap data   */
    raster->bits = (short *) malloc((unsigned int) linesize * hdr.ras_height);
    if (raster->bits) {
	linepointer = (char *) raster->bits;
	for (i = 0; i < hdr.ras_height; i++) {
	    read(rasfid, linepointer, linesize);	/* read image data   */
	    linepointer += linesize;
	}
    }
    return (errnum);
}
