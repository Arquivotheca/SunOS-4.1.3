#ifndef lint
static	char sccsid[] = "@(#)vectors.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
/*	
 *	zoom.c
 *
 *	purpose:    zoom up on part written out by "display"
 */

#include <stdio.h>
#include <suntool/tool_hs.h>
#include <suntool/gfxsw.h>

#define POINT_PLOT_MODE     0x1c
#define VECTOR_MODE         0x1d
#define BUFSIZE             0xffff

int bufptr = BUFSIZE;
int mode = VECTOR_MODE,x,y;
int dark_vector = TRUE;
int fp;
int scale_factor,offset_x,offset_y;
int zoom_factor = 1;

char *cmsname = "veccolmap";
unsigned char red[] = {220,255,0,0},
		green[] = {220,0,0,0},
		blue[] = {220,0,0,0};

main(argc,argv)
int argc;
char *argv[];
{
	int quick_flag;
        struct rect rect;
        struct gfxsubwindow *gfx = gfxsw_init(0,argv);
	int last_input = -1,current_input = -1;
        int control;
        short rel_x,rel_y;
        float tmpx,tmpy;
	int lockcount = 0;
   
	quick_flag=quick_test(argc,argv);

        /* open the /tmp file for use by zoom */
        if (argc < 2) {
           printf(" no input datafile specified\n");
           exit(1);
        }
        fp = open(argv[1],"2");
        if (fp == -1) {
           printf("parts file '%s' does not exist\n",argv[1]);
           return(0);
        }

	pw_setcmsname( gfx->gfx_pixwin, cmsname);
	pw_putcolormap( gfx->gfx_pixwin,0,4,red,green,blue);
 

        /* get parameters of the window taken over for pw_lock */
        pw_exposed(gfx->gfx_pixwin);

        /*get window size and calculate scale and zoom factors */
        win_getsize(gfx->gfx_windowfd,&rect); 
        rel_x = rect.r_width - rect.r_left;
        rel_y = rect.r_height - rect.r_top;
        tmpx = 750.0/(float)rel_x+1.0;
        tmpy = 600.00/(float)rel_y+1.0;
        scale_factor = (int)tmpx; 
        if (tmpy > tmpx)
           scale_factor = (int)tmpy;
        offset_x = (rel_x - 1024/scale_factor)/2 - (rel_x - rel_x/zoom_factor);
        offset_y = (rel_y - 800/scale_factor)/2 - (rel_y - rel_y/zoom_factor);

        /* clear screen */
        pw_writebackground(gfx->gfx_pixwin,0,0,rect.r_width,rect.r_height,0);
        
	pw_lock(gfx->gfx_pixwin, &rect);
	while (TRUE) {
		control = get_xy(); /* get xy co-ordinates or control data */
		if (control == EOF)
			break;
		else if (control == 0) {
			draw(gfx->gfx_pixwin,&rect); 
	            	if (lockcount++ == 10) {
	            		pw_unlock(gfx->gfx_pixwin);
	            		lockcount = 0;
	            		pw_lock(gfx->gfx_pixwin, &rect);
	            	}
		}
	}

        pw_unlock(gfx->gfx_pixwin);
        close(fp);

        if (quick_flag)
            sleep(10);
        else
            sleep(100000);
}

/* draw subroutine */

/* purpose - draw vector on the screen */

draw(pixwin,rect)
struct pixwin *pixwin;
struct rect *rect;

{
        int scaled_x,scaled_y;
	static int last_x,last_y;

        scaled_x = (x*zoom_factor)/scale_factor+offset_x;
        scaled_y = (y*zoom_factor)/scale_factor+offset_y;

        if (mode == VECTOR_MODE) {
                if (dark_vector == TRUE) {
                    /* reposition without drawing a vector */ 
                    dark_vector = FALSE; 
                }
                else  {
                    pw_vector(pixwin,last_x,last_y,scaled_x,scaled_y,PIX_SRC,1);
                }
	}
	else {    /* point plot mode */
		pw_put(pixwin,scaled_x,scaled_y,1);
	}   
        last_y = scaled_y;
        last_x = scaled_x;
}

/* get_xy */

/* purpose - fetch an x and a y co-ordinate */

get_xy()
{
       int input;

           read_from_file(&input);

           if (input == EOF)
               return(EOF);
           else if (input == VECTOR_MODE) {
               if (mode == VECTOR_MODE)
                  dark_vector = TRUE;
               mode = VECTOR_MODE;
               return(VECTOR_MODE);
           }
           else if (input == POINT_PLOT_MODE) {
               mode = POINT_PLOT_MODE;
               return(POINT_PLOT_MODE);
           }
           else {
               y = y & 0x00ff;
               y = y | (input << 8 );

               read_from_file(&input);
               y = y & 0xff00;
               y = y | input;

               read_from_file(&input);
               x = x & 0x00ff;
               x = x | (input << 8 );

               read_from_file(&input);
               x = x & 0xff00;
               x = x | input;
           }
           return(0);
}

/* read_from_file */

/* purpose - read the /tmp file into a buffer */

read_from_file(output)
int *output;
{
        static char buf[BUFSIZE];
        static int size;

        if (bufptr == BUFSIZE) {
            size = read(fp,buf,BUFSIZE);
            bufptr = 0;
        }
        if (bufptr < size)
           *output =((int)buf[bufptr++]) & 0xff;
        else
           *output = EOF;
        return(0);
}

int quick_test(argc,argv) int argc; char *argv[];
	{
	while (--argc > 0) {
		if(!strncmp(argv[argc],"-q",2))
			return(TRUE);
		}
	return(FALSE);
	}
