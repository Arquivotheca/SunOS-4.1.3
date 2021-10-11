#ifndef lint
static	char sccsid[] = "@(#)balls.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 *	Colliding balls demo program
 */
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <sunwindow/cms.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <suntool/gfxsw.h>

main(argc, argv)
	int argc;
	char **argv;
{
	short	op = PIX_CLR;

	struct	gfxsubwindow *gfx = gfxsw_init(0, argv, 0);

        if (argv) {
                char    **args;

                /*
                 * determine if want balls or worms.
                 */
                for (args = argv;*args;args++) {
                        if (strcmp(*args, "-b") == 0)
                                op = PIX_CLR;
                        if (strcmp(*args, "-w") == 0)
                                op = PIX_SET;
                }
        }
	srand( 931);
	BallsWorms(gfx, op);
	gfxsw_done(gfx);
	}

BallsWorms(gfx, op)
	struct	gfxsubwindow *gfx;
	short op;
{
        /*
         * This gives you either colliding balls or worms, depending
         * on the parameter "op".
         * PIX_CLR gives you balls, PIX_SET gives you worms
         */
#define NUMBER 30
#define MAX_VEL 8
	struct	pixrect *mpr = (struct pixrect *) 0;
	int	Xmax, Ymax, Ymin = 0;
	int	N, RADIUS;
	register int	i, j, x0, y0;
	int	t, clipor;
	register int	x[NUMBER], y[NUMBER], vx[NUMBER], vy[NUMBER];

Restart:
    if (mpr) {
	pr_destroy(mpr);
	mpr = (struct pixrect *) 0;
	}
    Xmax = gfx->gfx_rect.r_width;
    Ymax = gfx->gfx_rect.r_height;
    if (Xmax < Ymax)
    	RADIUS = Xmax/29+1;
    else
    	RADIUS = Ymax/29+1;
    RADIUS = (RADIUS < 2)? 2: RADIUS;
    RADIUS = (RADIUS > 16)? 16: RADIUS;
    N = 2*RADIUS;
    /*
     * Clear screen
     */
    pw_writebackground(gfx->gfx_pixwin, 0, 0, Xmax, Ymax, PIX_SRC);
    /*
     * First we make a square with a circle in it off the screen
     */
    mpr = mem_create(N, N, gfx->gfx_pixwin->pw_pixrect->pr_depth);
    for (i=1; i<=RADIUS-1; i++) {
        j=sqroot(RADIUS*RADIUS-i*i);
	pr_rop(mpr, RADIUS-i, RADIUS-j, 2*i, 2*j, PIX_SET, 0, 0, 0);
	pr_rop(mpr, RADIUS-j, RADIUS-i, 2*j, 2*i, PIX_SET, 0, 0, 0);
	}
    for (i=0; i<NUMBER; i++) {
	x[i]=r(0, Xmax-N);
	y[i]=r(Ymin, Ymax-N);
	vx[i]=r(-MAX_VEL, MAX_VEL);
	vy[i]=r(-MAX_VEL, MAX_VEL);
	pw_write(gfx->gfx_pixwin, x[i], y[i], N, N, PIX_SRC^PIX_DST, mpr, 0, 0);
        }
    while (gfx->gfx_reps) {
	pw_lock(gfx->gfx_pixwin, &gfx->gfx_rect);
	if (pw_rectvisible(gfx->gfx_pixwin, &gfx->gfx_rect))
		clipor = PIX_DONTCLIP;
	else
		clipor = 0;
	if (gfx->gfx_flags&GFX_DAMAGED)
		gfxsw_handlesigwinch(gfx);
	/*
	 * If size changes between lock and here then will RESTART.
	 * Possible race condition if exchange order of operations.
	 */
	if (gfx->gfx_flags&GFX_RESTART) {
		gfx->gfx_flags &= ~GFX_RESTART;
		pw_unlock(gfx->gfx_pixwin);
		goto Restart;
	}
        for (i=0; i<NUMBER; i++) 
         {
            x0=x[i];
            y0=y[i];
            if ( abs(vx[i])>MAX_VEL ) 
              {
                printf( "Weird value (%d) for vx[%d]\n", vx[i], i );
                vx[i] = r(-MAX_VEL, MAX_VEL);
              }
            x[i]=x[i]+vx[i]; if (x[i]<0) 
             {				/* Bounce off the left wall */
                x[i]= 0;
                vx[i]= -vx[i];
            } else if (x[i]>Xmax-N) 
             { 				/* Bounce off the right wall */
                vx[i]= -vx[i];
                x[i]=x[i] + vx[i];
            }

            y[i]=y[i]+vy[i];
            if (y[i]>Ymax-N) 
             { 				/* Bounce off the top */
                y[i]=Ymax-N;
                vy[i]= -vy[i];
            }
           else if (y[i]<Ymin) 
              { 			/* Bounce off the bottom */
                y[i]=Ymin;
                vy[i]= -vy[i];
              }
	    pw_write(gfx->gfx_pixwin, x0, y0, N, N,
	        ((op == PIX_CLR)? PIX_SRC^PIX_DST: PIX_SRC|PIX_DST) | clipor,
		mpr, 0, 0);
	    pw_write(gfx->gfx_pixwin, x[i], y[i], N, N,
		(PIX_SRC^PIX_DST) | clipor, mpr, 0, 0);
          }
	pw_unlock(gfx->gfx_pixwin);
        for (i=0; i<NUMBER-1; i++)
         {
            for (j=i+1; j<NUMBER; j++)
              {
                  /*
                   * Check each pair of balls for collisions
                   */
                x0=x[i]-x[j];
                y0=y[i]-y[j];
                if ((abs(x0)<N) && (abs(y0)<N) && (x0*x0+y0*y0<N*N)) 
                 {
                    if (y0<0) 
                      {
                        y0= -y0;
                        x0= -x0;
                      }
                    if (abs(x0) > (y0)) 
                     { 			/* flip both horizontal velocities */
                        vx[i]= -vx[i];
                        vx[j]= -vx[j];
                     }
                    else if ((10*y0)>(12*abs(x0))) 
                      { 		/* Flip both vertical velocities */
                        vy[i]= -vy[i];
                        vy[j]= -vy[j];
                      }
                     else if (y0>0) 
                      { 		/* Flip both and transpose x & y */
                        t=vx[i];
                        vx[i]= -vy[i];
                        vy[i]= -t;
                        t=vx[j];
                        vx[j]= -vy[j];
                        vy[j]= -t;
                      } 
                    else 
                      { 		/* Flip horizontal, transpose x & y */
                        t=vx[i];
                        vx[i]= -vy[i];
                        vy[i]=t;
                        t=vx[j];
                        vx[j]= -vy[j];
                        vy[j]=t;
                      }
                 }

            }
        }
	if (--gfx->gfx_reps <= 0)
		break;
    }
}

/*-------------------------------*/
int r (i, j) int i, j;
{
    int k;

    k = rand() % (j-i+1);
    if (k<0) return(k+j+1);
    else return(k+i);
}

/*-------------------------------*/
int sqroot (x) int x;
{
    int v, i;
    v=0;
    i=128;
    while (i!=0) {
        if ((v+i<=180) && ((v+i)*(v+i)<=x))
            v=v+i;
        i=i/2;
    }
    return (v);
}

