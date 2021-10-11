/*	rotcube.c	1.1	92/07/30	*/

/*      spinning "cube" although now maybe we will allow it to
 *      be a parallelpiped.  hacked over by vaughn pratt to
 *      speed it up, then thrashed by djc to put in some "C"
 *      like constructs and make it work on Sun 2 video and
 *      in the window system.
 */
#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/bw1reg.h>
#include <pixrect/bw1var.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <suntool/gfxsw.h>

#define DEBUG Yes
#define UNCHANGED 0
#define CHANGED 1

int redo;
int cx, cy;
int x, y;
int x1, y1, x2, y2, x3, y3, x4, y4;
int nx1, ny1, nx2, ny2, nx3, ny3, nx4, ny4;
int px1, py1, px2, py2, px3, py3, px4, py4;
int pnx1, pny1, pnx2, pny2, pnx3, pny3, pnx4, pny4;
int yd;
int z, zsc;
int Xmax, Ymax, Xmin, Ymin;
struct gfxsubwindow *gfx;
struct pixrect *backup_window;
int clipor;

main(argc,argv)
char **argv;
{

	int sp;
	gfx = gfxsw_init(0, argv);
	while (1) /* Restart: */ {
	Restart:

		Xmin = 0; Ymin = 0;
		Xmax = gfx->gfx_rect.r_width;
		Ymax = gfx->gfx_rect.r_height;
		if (backup_window) {
			 mem_destroy(backup_window);
			 backup_window = (struct pixrect *) 0;
		}
		backup_window = mem_create(Xmax, Ymax, 1);
		clear_area(gfx->gfx_pixwin, Xmin, Ymin, Xmax,Ymax);

		zsc = 10;
		x = -(Xmax/5)<<16, y = (Ymax/5)<<16, z = ((Xmax+Ymax)/10)<<zsc;
       		x = -200<<16; y = 200<<16; z = 200 <<zsc;
		yd = 1200 * 800 / (Xmax > Ymax ? Ymax : Xmax);
		cx = Xmax/2, cy = Ymax/2;
		redo = 1;
      
		while (1) {
			for (sp = 10; sp > 0; sp--)
				if (spin( sp )) goto Restart;
			for (sp = 2; sp < 10; sp++)
				if (spin( sp )) goto Restart;
		}
	}
}

int spin(sp)
{

	register int i, squareval;
	for (i = 0; i < 50; i++) {
		if (squareval = square(x,y)) break;
		x = x-(y>>sp);
		y = y+(x>>sp);
	}
	return(squareval);
}

int square(x,y)
{

	register xx = x>>16, yy = y>>16;
	nx1 = xx;
	ny1 = yy;
	nx2 = yy;
	ny2 = - xx;
	nx3 = - xx;
	ny3 = - yy;
	nx4 = - yy;
	ny4 = xx;
	pnx1 = (nx1<<zsc)/(yd+ny1);
	pny1 = z/(yd+ny1);
	pnx2 = (nx2<<zsc)/(yd+ny2);
	pny2 = z/(yd+ny2);
	pnx3 = (nx3<<zsc)/(yd+ny3);
	pny3 = z/(yd+ny3);
	pnx4 = (nx4<<zsc)/(yd+ny4);
	pny4 = z/(yd+ny4);

	pw_lock(gfx->gfx_pixwin, &gfx->gfx_rect);
        if (gfx->gfx_flags&GFX_DAMAGED)
                gfxsw_handlesigwinch(gfx);
	/*
         * If size changes between lock and here then will RESTART.
         * Possible race condition if exchange order of operations.
         */
        if (gfx->gfx_flags&GFX_RESTART) {
                gfx->gfx_flags &= ~GFX_RESTART;
		pw_unlock(gfx->gfx_pixwin);
		return(CHANGED);
        }
	nvec(px1,py1,px1,-py1, PIX_NOT(PIX_SRC) & PIX_DST);
	nvec(pnx1,pny1,pnx1,-pny1, PIX_SET | clipor);
	nvec(px2,py2,px2,-py2, PIX_NOT(PIX_SRC) & PIX_DST); 
	nvec(pnx2,pny2,pnx2,-pny2, PIX_SET | clipor);
	nvec(px3,py3,px3,-py3, PIX_NOT(PIX_SRC) & PIX_DST);
	nvec(pnx3,pny3,pnx3,-pny3, PIX_SET | clipor);
	nvec(px4,py4,px4,-py4, PIX_NOT(PIX_SRC) & PIX_DST);
	nvec(pnx4,pny4,pnx4,-pny4, PIX_SET | clipor);
	nvec(px1,py1,px2,py2, PIX_NOT(PIX_SRC) & PIX_DST);
	nvec(pnx1,pny1,pnx2,pny2, PIX_SET | clipor);
	nvec(px2,py2,px3,py3, PIX_NOT(PIX_SRC) & PIX_DST);
	nvec(pnx2,pny2,pnx3,pny3, PIX_SET | clipor);
	nvec(px3,py3,px4,py4, PIX_NOT(PIX_SRC) & PIX_DST);
	nvec(pnx3,pny3,pnx4,pny4, PIX_SET | clipor);
	nvec(px4,py4,px1,py1, PIX_NOT(PIX_SRC) & PIX_DST);
	nvec(pnx4,pny4,pnx1,pny1, PIX_SET | clipor);
	nvec(px1,-py1,px2,-py2, PIX_NOT(PIX_SRC) & PIX_DST);
	nvec(pnx1,-pny1,pnx2,-pny2, PIX_SET | clipor);
	nvec(px2,-py2,px3,-py3, PIX_NOT(PIX_SRC) & PIX_DST);	
	nvec(pnx2,-pny2,pnx3,-pny3, PIX_SET | clipor);	
	nvec(px3,-py3,px4,-py4, PIX_NOT(PIX_SRC) & PIX_DST);	
	nvec(pnx3,-pny3,pnx4,-pny4, PIX_SET | clipor);	
	nvec(px4,-py4,px1,-py1, PIX_NOT(PIX_SRC) & PIX_DST);
	nvec(pnx4,-pny4,pnx1,-pny1, PIX_SET | clipor);

	pw_unlock(gfx->gfx_pixwin);

	x1 = nx1, y1 = ny1;
	x2 = nx2, y2 = ny2;
	x3 = nx3, y3 = ny3;
	x4 = nx4, y4 = ny4;
	px1 = pnx1, py1 = pny1;
	px2 = pnx2, py2 = pny2;
	px3 = pnx3, py3 = pny3;
	px4 = pnx4, py4 = pny4;

	return(UNCHANGED);
}

nvec(x1,y1,x2,y2,op)
{

/*	pr_vector(backup_window, cx+x1, cy+y1, cx+x2, cy+y2, op, 1);
 */
 	pw_vector(gfx->gfx_pixwin, cx+x1, cy+y1, cx+x2, cy+y2, op, 1);
}

clear_area(window, left, bottom, width, height)
struct pixwin *window;
int left, bottom, width, height; 
{
	pw_writebackground(window, left, bottom, width, height, PIX_SRC);
}	

/* now that we have created the new image, replace the old one */
display_new_image(from_rect, to_window)
struct pixrect *from_rect;
struct pixwin  *to_window;
{
	pw_write(to_window, Xmin, Ymin, Xmax, Ymax, PIX_SRC, from_rect, 0, 0);
	pr_rop(from_rect, 0, 0, Xmax, Ymax, PIX_SRC, NULL, 0, 0);
}  
