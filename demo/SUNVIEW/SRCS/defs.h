#include <suntool/tool_hs.h>
#include <stdio.h>
#include "fract.h"

#define MINSCRX	0
#define MINSCRY	0

#define SPRSIDE		80
#define HSPRSIDE	40

#define pw_stencil(dpw, x, y, w, h, op, stpr, stx, sty, spr, sy, sx)	\
	(*(dpw)->pw_ops->pro_stencil)((dpw)->pw_opshandle,              \
	    x-(dpw)->pw_opsx, y-(dpw)->pw_opsy, w, h, op,		\
	    stpr, stx, sty, spr, sy, sx)

struct gfxsubwindow *moleculewin;

char *malloc();

struct sphere {
	fract ox, oy, oz;	/* original x, y, z coordinates */
	fract tx, ty, tz;	/* transformed coordinates */
	short radius;
	short color;
	struct sphere *next;
} **saddr;

struct line {
	fract ox1, oy1, oz1;
	fract ox2, oy2, oz2;
	fract tx1, ty1, tz1;
	fract tx2, ty2, tz2;
	fract ex1, ey1, ez1;
	fract ex2, ey2, ez2;
	short l_color;
	struct line *l_next;
} **laddr;

int numspheres;
int numlines;
int first;
