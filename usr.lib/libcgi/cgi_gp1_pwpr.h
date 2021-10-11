/*	@(#)cgi_gp1_pwpr.h 1.1 92/07/30 Copyr 1985-9 Sun Micro		*/

/*
 * Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc. 
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

/*
 * CGI's GP1 interface.  Users shouldn't make use of this file.
 * This file will be removed in a future release.
 */

typedef struct
{
    short           attrchg;
    int             attrcnt, clpcnt, resetcnt;
    int             clipid;
    int             org_x, org_y;
    int             statblkindx;
    short           fbindx;
    short          *gp1_addr;
    int             shm_base, offset, count;
    unsigned int    bitvec;
    float           xscale, xoffset, yscale, yoffset;
    short           op, color, pixplanes;
    short           width;
    short          *tex_pattern, tex_offset, tex_options;
    int             ncmd;
    u_char         *cmdver;
}               Gp1_attr;

/*
  attrchg: indicates whether attributes have changed since last sent to GP
  attrcnt: value of _cgi_gp1resetcnt when attributes last sent to GP
  clpcnt: value of _cgi_gp1resetcnt when clipping list last sent to GP
  clipid: value of clipid when clipping list last sent to GP
	  (pixwin viewsurfaces only)
  org_x, org_y: window origin in screen coordinates (useful for pixwin
		viewsurfaces only; set to 0 for pixrect viewsurfaces)
  statblkindx: obtained from the kernel, one per viewsurface; indicates where
	       static data and clipping list is stored for the viewsurface
  fbindx: index of cg2 color board for this viewsurface as known to the
	  GP microcode
  shm_base:	offset in GP memory of currently allocated block, 0 if none.
  offset:	additional offset past shm_base where we are now in block.
  xscale, xoffset, yscale, yoffset: NDC to screen coordinate transform
  op: rasterop to be used for primitives
  color: color to be used for primitives
  pixplanes: per plane mask to be used for primitives (needs to be set from
	     pr or pw attributes
  width: line width to be used for primitives
  tex_pattern: pointer to one of CGI's pattern arrays.  Assumed to be <= 16
	      in length
  tex_offset: pixel offset into pattern
  tex_options: bit vector for fat, poly, givenpattern, balanced, endpoint,
		startpoint
  ncmd: copy of value from gp1pr private data struct, gives number of
		commands this version of GP microcode recognizes.
  cmdver: malloc'd space containing copy of command version vector
		from gp1pr structure.
*/


extern int      _cgi_gp1resetcnt;	/* wds added _cgi_ 850712 */
