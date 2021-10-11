#ifndef lint
static	char sccsid[] = "@(#)molecule.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
/* Molecule - display a molecule description file using shaded, colored
 *            spheres
 *
 * June 1984 Philippe Lacroute
 * idea for shading algorithm from Mike Pique's zbs program from UNC
   July 1984 Martin Weiss minor changes to make marketing happy
 */

#include "defs.h"
#include <pixrect/pixrect_hs.h>
#include <suntool/gfxsw.h>
#include <ctype.h>

#define sqrt(c)	sqrttbl[c]
#define TBLSZ	1024
#define UNITY	fracti(1)

short sqrttbl[TBLSZ] = {
#include "sqrttbl.h"
};

#define MAXRADIUS	64
int delay = 15;	/* seconds to delay between rounds */
int sweep = 30;	/* degrees to sweep in one round */
int MAXSCRX,MAXSCRY;
struct pixrect *zbpr, *atompr;
short *zbptr;
unsigned char *atomptr;
short circle[40];
struct rect r;
fract xfmmatrix[4][3] = {	/* transformation matrix */
	UNITY, 0, 0,
	0, UNITY, 0,
	0, 0, UNITY,
	0, 0, 0
};


main(argc, argv)
int argc;
char **argv;
{
	int angle;

/*
 *  initialization
 */

        if ((moleculewin = gfxsw_init(0,argv)) == (struct gfxsubwindow *) 0)
                exit(1);
        MAXSCRX = ((moleculewin->gfx_pixwin)->pw_pixrect)->pr_size.x;
        MAXSCRY = ((moleculewin->gfx_pixwin)->pw_pixrect)->pr_size.y;
	zbpr = mem_create( MAXSCRX, MAXSCRY, 16);
        loadcmap();
	first = 1;	/* see dospheres below */
	atompr = mem_create( MAXRADIUS*2+1, MAXRADIUS*2+1, 8);
	if ( !zbpr || !atompr) {
		printf("Need more swap space or virtual memory space.\n");
		printf("Try a smaller window or kill some processes.\n");
		exit( 0);
	}

/*
 * The main loop
 * Get a random angle; rotate the lines from that angle through the
 * sweep angle displaying the transformed lines
 * at intervals to give 3D effect; stop and draw the spheres; sleep;
 * repeat
 */


    while (1) {
	if (first) {		/* get the spheres the first time through */
		srand(getpid());
		clearscreen();
		readdata();
		sortspheres();
		sortlines();
		first = 0;
	} else {
		clearscreen();
	}
	angle = rand() % 360;
	displaylines(angle-sweep, angle);
	rotate(angle);
	xfmspheres();
	displayspheres();
	zbclear();
        sleep(delay);
    }
}

#define MAXINTEN	15
#define NINTENBITS	4
loadcmap()
{

/* Set up the color map
 * 8 bits are used per color
 * the 3 high bits define the color
 * the 5 low bits define the intensity
 * Bits 7-5 are for Red, Green, and Blue */

#define RBIT 0x40
#define GBIT 0x20
#define BBIT 0x10
#define BACKGRR 127 /* Red level (of 255) */
#define BACKGRG 127 /* Grn level (of 255) */
#define BACKGRB 127 /* Blu level (of 255) */
#define NCMAP	128

	register int i, rval, gval, bval;
	unsigned char r[NCMAP], g[NCMAP], b[NCMAP];

	r[0]=BACKGRR; g[0]=BACKGRG; b[0] = BACKGRB;
	for(i=1;i<NCMAP;i++) {
		rval = ( (i&RBIT)==0) ? 0 : (i&MAXINTEN)<<(8-NINTENBITS);
		gval = ( (i&GBIT)==0) ? 0 : (i&MAXINTEN)<<(8-NINTENBITS);
		bval = ( (i&BBIT)==0) ? 0 : (i&MAXINTEN)<<(8-NINTENBITS);
		if(rval==0 && gval==0 && bval==0) 
		   rval=gval=bval= (i&MAXINTEN)<<(8-NINTENBITS);
		r[i] = rval;
		g[i] = gval;
		b[i] = bval;
	}
        pw_setcmsname(moleculewin->gfx_pixwin, "molecules");
	pw_putcolormap(moleculewin->gfx_pixwin, 0, NCMAP, r, g, b);
}

clearscreen()
{
	pw_writebackground(moleculewin->gfx_pixwin,0,0,MAXSCRX,MAXSCRY,PIX_SET);
}

#define zbcover(x, y, u, v, depth, color)				\
	zbptr = (short*)mprd8_addr( mpr_d(zbpr), x,y, zbpr->pr_depth);	\
	atomptr = mprd8_addr( mpr_d(atompr), u, v, atompr->pr_depth);	\
	if (*zbptr < depth) {						\
		*zbptr = depth;						\
		*atomptr = color;					\
	}

/*
 * Make a parabaloid over the visible circle; at each point, the
 * sqrt of the height of the parabaloid is the height of the sphere
 */

#define SDERIV	-2

drawsphere(x, y, z, radius, colorcode)
short x, y, z, radius, colorcode;
{
	int stheight;			/* starting height */
	register ifderiv, ofderiv;	/* derivatives of parab */
	register short u,v;		/* coordinates of pts on circle */
	register height;		/* height**2 of current pixel */
	register short sheight;		/* actual height of pixel */
	register int color;
	int inten, r2;
	short x1, x2, x3, x4, y1, y2, y3, y4;
	short u1, u2, u3, u4, v1, v2, v3, v4;

	ofderiv = -1;
	stheight = radius * radius;
	makecircle(radius, x, y);
	pw_lock(moleculewin->gfx_pixwin, &r);
	r2 = radius * 2 + 1;
	pw_read( atompr, 0, 0, r2, r2, PIX_SRC,
		moleculewin->gfx_pixwin, x-radius, y-radius);
	for (v = 0; v <= radius; v++) {
		height = stheight;
		ifderiv = -1;
		stheight += ofderiv;
		ofderiv += SDERIV;
		x2 = x+v; x4 = x-v; y2 = y+v; y4 = y-v;
		u2 = v2 = radius+v; u4 = v4 = radius-v;
		for (u = 0; u <= v; u++) {
			if (v > circle[u])
				break;
			else if (height <= 0)
				sheight = 0;
			else
				sheight = sqrt(height);
			inten = (sheight * MAXINTEN) / radius;
			color = colorcode | inten;

			sheight += z;

			x1 = x+u; x3 = x-u; y1 = y+u; y3 = y-u;
			u1 = v1 = radius+u;
			u3 = v3 = radius-u;

			zbcover(x1, y2, u1, v2, sheight, color);
			zbcover(x2, y1, u2, v1, sheight, color);
			if (u != 0) {
				zbcover(x3, y2, u3, v2, sheight, color);
				zbcover(x2, y3, u2, v3, sheight, color);
			}
			if (v != 0) {
				zbcover(x1, y4, u1, v4, sheight, color);
				zbcover(x4, y1, u4, v1, sheight, color);
			}
			if (u != 0 && v != 0) {
				zbcover(x3, y4, u3, v4, sheight, color);
				zbcover(x4, y3, u4, v3, sheight, color);
			}
			height += ifderiv;
			ifderiv += SDERIV;
		}
	}
	pw_write( moleculewin->gfx_pixwin, x-radius, y-radius,
		r2, r2, PIX_SRC, atompr, 0, 0);
	pw_unlock(moleculewin->gfx_pixwin);
}

displayspheres()
{
	int counter;

	for (counter = 0; counter < numspheres; counter++) {
		drawsphere(roundfr(saddr[counter]->tx),
			   roundfr(saddr[counter]->ty),
			   roundfr(saddr[counter]->tz),
			   saddr[counter]->radius,
			   saddr[counter]->color);
	}
}

zbclear()
{
	pr_rop( zbpr, 0,0, MAXSCRX,MAXSCRY,PIX_SRC,0,0,0);
}

makecircle(rad, x0, y0)
short   rad;
{
	int x, y, d;

	x = 0;
	y = rad;
	d = 3 - 2 * rad;
	while (x < y) {
		circle[x] = y;
		circle[y] = x;
		if (d < 0) {
			d = d + 4 * x + 6;
		} else {
			d = d + 4 * (x - y) + 10;
			y = y - 1;
		}
		x += 1;
	}
	if (x == y)
		circle[x] = y;
	else
		circle[x] = 0;
}

struct sphere *spheres = NULL;
struct line *lines = NULL;

/*
 * read input data lines
 * each line begins with a command character from the set [sltq] for
 * sphere, line, transform or quit respectively; the remainder
 * of the line contains the appropriate data for the command: 3 
 * coordinates of the center, radius, and color; 3 coordinates each
 * for the 2 endpoints, and color; 3 translations; and nothing respectively
 */

readdata()
{
	struct sphere *s;
	struct line *l;
	char line[512], *lptr;
	int x, y, z, x2, y2, z2;;

	while (gets(line) != NULL) {
		switch (line[0]) {
		case 's':	/* a sphere */
			s = (struct sphere *) malloc(sizeof(struct sphere));
			if (s == NULL) {
				printf("Out of sphere memory\n");
				exit(1);
			}
			sscanf(&line[1], "%d %d %d %hd %hd",
			       &x, &y, &z, &s->radius, &s->color);
			if (s->radius > MAXRADIUS)
				s->radius = MAXRADIUS;
			s->color <<= NINTENBITS;
			s->ox = fracti(x);
			s->oy = fracti(y);
			s->oz = fracti(z);
			s->next = spheres;
			spheres = s;
			numspheres++;
			break;
		case 'l':	/* a line */
			l = (struct line *) malloc(sizeof(struct line));
			if (l == NULL) {
				printf("Out of line memory\n");
				exit(1);
			}
			sscanf(&line[1], "%d %d %d %d %d %d %hd",
				&x, &y, &z, &x2, &y2, &z2, &l->l_color);
			l->l_color <<= NINTENBITS;
			l->ox1 = fracti(x);
			l->oy1 = fracti(y);
			l->oz1 = fracti(z);
			l->ox2 = fracti(x2);
			l->oy2 = fracti(y2);
			l->oz2 = fracti(z2);
			l->ex1 = l->ex2 = l->ey1 = l->ey2 = 0;
			l->l_next = lines;
			lines = l;
			numlines++;
			break;
		case 't':
			sscanf(&line[1], "%d %d %d", &x, &y, &z);
			xfmmatrix[3][0] = fracti(x);
			xfmmatrix[3][1] = fracti(y);
			xfmmatrix[3][2] = fracti(z);
			break;
		case 'q':
			return;
		case '#':	/* comment line */
			break;
		default:
			if (!isspace(line[0]))
				printf("Illegal control character %c\n", line[0]);
			break;
		}
	}
}

/*
 * simple selection sort to place closer spheres at beginning of list
 * thus reducing the number of pixels that will be written repeatedly
 */

sortspheres()
{
	register int counter, counter2, index;
	struct sphere *s = spheres;

	saddr = (struct sphere **) malloc(numspheres*sizeof(struct sphere *));
	for (counter = 0; counter < numspheres; counter++) {
		saddr[counter] = s;
		s = s->next;
	}
	for (counter = 0; counter < numspheres - 1; counter++) {
		index = counter;
		s = saddr[counter];
		for (counter2 = counter+1; counter2 < numspheres; counter2++)
			if (saddr[counter2]->oz > saddr[index]->oz) {
				index = counter2;
				s = saddr[counter2];
			}
		saddr[index] = saddr[counter];
		saddr[counter] = s;
	}
}

/* Do lines on right first */

sortlines()
{
	register int counter, counter2, index;
	struct line *l = lines;

	laddr = (struct line **) malloc(numspheres*sizeof(struct line *));
	for (counter = 0; counter < numlines; counter++) {
		laddr[counter] = l;
		l = l->l_next;
	}
	for (counter = 0; counter < numlines - 1; counter++) {
		index = counter;
		l = laddr[counter];
		for (counter2 = counter+1; counter2 < numlines; counter2++)
			if (laddr[counter2]->ox1 > laddr[index]->ox1) {
				index = counter2;
				l = laddr[counter2];
			}
		laddr[index] = laddr[counter];
		laddr[counter] = l;
	}
}

xfmspheres()
{
	int counter;

	for (counter = 0; counter < numspheres; counter++)
		xfmsphere(saddr[counter]);
}


/* apply the transformation in xfmmatrix to a sphere */

xfmsphere(s)
struct sphere *s;
{
	s->tx =   frmul(s->ox, xfmmatrix[0][0])
		+ frmul(s->oy, xfmmatrix[1][0])
		+ frmul(s->oz, xfmmatrix[2][0])
		+ xfmmatrix[3][0];
	s->ty =   frmul(s->ox, xfmmatrix[0][1])
		+ frmul(s->oy, xfmmatrix[1][1])
		+ frmul(s->oz, xfmmatrix[2][1])
		+ xfmmatrix[3][1];
	s->tz =   frmul(s->ox, xfmmatrix[0][2])
		+ frmul(s->oy, xfmmatrix[1][2])
		+ frmul(s->oz, xfmmatrix[2][2])
		+ xfmmatrix[3][2];
}

/* create a transformation which rotates image ang degrees about y axis */

rotate(ang)
int ang;
{
	double sin(), cos();
	fract s = fractf((float)sin((double)ang/180.0*3.14159)),
	      c = fractf((float)cos((double)ang/180.0*3.14159));

	xfmmatrix[0][0] = c; xfmmatrix[0][1] = 0; xfmmatrix[0][2] = s;
	xfmmatrix[1][0] = 0; xfmmatrix[1][1] = fracti(1); xfmmatrix[1][2] = 0;
	xfmmatrix[2][0] = -s; xfmmatrix[2][1] = 0; xfmmatrix[2][2] = c;
}

/* transform a line */

xfmline(l)
struct line *l;
{
	l->tx1 =   frmul(l->ox1, xfmmatrix[0][0])
		+ frmul(l->oy1, xfmmatrix[1][0])
		+ frmul(l->oz1, xfmmatrix[2][0])
		+ xfmmatrix[3][0];
	l->ty1 =   frmul(l->ox1, xfmmatrix[0][1])
		+ frmul(l->oy1, xfmmatrix[1][1])
		+ frmul(l->oz1, xfmmatrix[2][1])
		+ xfmmatrix[3][1];
	l->tz1 =   frmul(l->ox1, xfmmatrix[0][2])
		+ frmul(l->oy1, xfmmatrix[1][2])
		+ frmul(l->oz1, xfmmatrix[2][2])
		+ xfmmatrix[3][2];
	l->tx2 =   frmul(l->ox2, xfmmatrix[0][0])
		+ frmul(l->oy2, xfmmatrix[1][0])
		+ frmul(l->oz2, xfmmatrix[2][0])
		+ xfmmatrix[3][0];
	l->ty2 =   frmul(l->ox2, xfmmatrix[0][1])
		+ frmul(l->oy2, xfmmatrix[1][1])
		+ frmul(l->oz2, xfmmatrix[2][1])
		+ xfmmatrix[3][1];
	l->tz2 =   frmul(l->ox2, xfmmatrix[0][2])
		+ frmul(l->oy2, xfmmatrix[1][2])
		+ frmul(l->oz2, xfmmatrix[2][2])
		+ xfmmatrix[3][2];
}


displaylines(from, to)
int from, to;
{
	int ang;
	register counter;

        r.r_left=0;
	r.r_top=0;
	r.r_width=MAXSCRX;
	r.r_height=MAXSCRY;
	for (ang = from; ang <= to; ang += 2) {
		rotate(ang);
		for (counter = 0; counter < numlines; counter++)
			xfmline(laddr[counter]);
		pw_lock(moleculewin->gfx_pixwin, &r);
		for (counter = 0; counter < numlines; counter++) {
			pw_vector(moleculewin->gfx_pixwin,
				roundfr(laddr[counter]->ex1),
				roundfr(laddr[counter]->ey1),
				roundfr(laddr[counter]->ex2),
				roundfr(laddr[counter]->ey2),
				PIX_SET, 0);
			pw_vector(moleculewin->gfx_pixwin,
				roundfr(laddr[counter]->tx1),
				roundfr(laddr[counter]->ty1),
				roundfr(laddr[counter]->tx2),
				roundfr(laddr[counter]->ty2),
				PIX_SRC,
				laddr[counter]->l_color | MAXINTEN);
			laddr[counter]->ex1 = laddr[counter]->tx1;
			laddr[counter]->ey1 = laddr[counter]->ty1;
			laddr[counter]->ex2 = laddr[counter]->tx2;
			laddr[counter]->ey2 = laddr[counter]->ty2;
		}
		pw_unlock(moleculewin->gfx_pixwin);
	}
}
