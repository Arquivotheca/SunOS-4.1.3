#ifndef lint
static	char sccsid[] = "@(#)gp2_polygon.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixrect_hs.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/cg2reg.h>
#include <pixrect/gp1cmds.h>
#include <pixrect/gp1var.h>
#include <sun/gpio.h>
#include <suntool/sunview.h>
#include <suntool/fullscreen.h>
#include <suntool/gfx_hs.h>
#include "gp2test.h"
#include "gp2test_msgs.h"
#include "gp2_polygon.h"
#include "sdrtns.h"

extern char *sprintf();

extern struct pixrect *screen;
extern int CgTWO;
extern short *gp1_shmem;
extern int ioctlfd;

u_long pix_chk();

/*
 *************************************************************************
 *	create_screen
 *
 *	this routine will ask pixrect's for host memory the size of
 *	the screen. It will retry 200 times to get enough memory.
 *	If it doesn't get it the program will be exited with a error
 *	message.
 *************************************************************************
 */

Pixrect *
create_screen()
{
    int i;
    Pixrect *mem_screen;

    for(i = 0; i < 200; i++) {
        if ((mem_screen = mem_create(screen->pr_width,screen->pr_height,screen->pr_depth)) == 0 ) {
            sleep(1);
        } else {
            break;
        }  
    }
    if (i >= 199) {
    	gp_send_message(-NO_MEMORY, FATAL, create_screen_msg);
    }
    return(mem_screen);
}

/*
 *************************************************************************
 *	loadmap
 *
 *	load an linear color map into the CG5 color board.
 *************************************************************************
 */

loadmap() /* load the colormap */
{
    int i;
    static unsigned char rmap[256], gmap[256], bmap[256];

    for (i = 0; i < 256; i++) {
        rmap[i] = (i & RED_BIT) ? 255 : 0;
        gmap[i] = (i & GREEN_BIT) ? 255 : 0;
        bmap[i] = (i & BLUE_BIT) ? 255 : 0;
    }

    pr_putcolormap(screen, 0, 256, rmap, gmap, bmap);
}
/*
 *************************************************************************
 *	paint_polygons
 *
 *	1).  build the polygons structure to be posted to GP2
 *	2).  clear z buffer.
 *	3).  call routine to post the polygons. (gp2poly3)
 *	4).  calculate the check sum for paint 1.
 *	5).  copy the screen contents to mem_screen1.
 *	6).  call routine to post the polygons.
 *	7).  calculate the check sum for paint 2.
 *	8).  save the screen contents to mem_screen2.
 *	9).  compare check sums. if no match report error, exit.
 *	10). XOR mem_screen1 with mem_screen2.
 *	11). check screen for all blank (0x0), if not report error, exit.
 *	12). if no errors return to calling routine.
 *************************************************************************
 */

paint_polygons(hsr, mem_screen, mem_screen1)
    char *hsr;
    Pixrect *mem_screen;
    Pixrect *mem_screen1;
{
    int index, index_end = 4;
    int points_per_poly, hsr_on = FALSE;
    u_long sav1_chksum=0,sav2_chksum=0;

    if ( !CgTWO )
       Crane_Init();

    if ((strcmp(hsr, "on")) == 0)
	hsr_on = TRUE;
    for (index = 0; index <= index_end; index++) { 
        points_per_poly = bldpolygons(poly_list[index]);
/*
 *      paint 1st screen 
 */
        clrzb();
	pr_rop ( screen, 0, 0, screen->pr_width, screen->pr_height, PIX_SRC,
			0, 0, 0 );
        if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
	    gp_send_message(-SYNC_ERROR, FATAL, paint_polygons_msg1);
        gp1poly3(polylst, npoly, points_per_poly);
        if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
	    gp_send_message(-SYNC_ERROR, FATAL, paint_polygons_msg2);
        if (hsr_on)             /* check sum hsr mode */
    	    sav1_chksum = pix_chk(screen);
        pr_rop(mem_screen,0,0,screen->pr_width,screen->pr_height,PIX_SRC,screen,0,0);
/*         
 *      paint second screen
 */
        clrzb();
	pr_rop ( screen, 0, 0, screen->pr_width, screen->pr_height, PIX_SRC,
			0, 0, 0 );
        if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
	    gp_send_message(-SYNC_ERROR, FATAL, paint_polygons_msg3);
        gp1poly3(polylst, npoly, points_per_poly);
        if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
	    gp_send_message(-SYNC_ERROR, FATAL, paint_polygons_msg4);
        pr_rop(mem_screen1,0,0,screen->pr_width,screen->pr_height,PIX_SRC,screen,0,0);
	if (hsr_on) {
    	    sav2_chksum = pix_chk(screen);  /* check sum hsr mode */
	    if (sav1_chksum != sav2_chksum) {
                errors++;
		(void) sprintf(msg, paint_polygons_msg5,
		    sav1_chksum, sav2_chksum, index, hsr);
                gp_send_message(DATA_ERROR, ERROR, msg);
            }
	}
/*          
 *      xor 1st paint with 2nd paint and check for any pixels left
 *      on the screen
 */
	pr_rop(screen,0,0,screen->pr_width,screen->pr_height,(PIX_SRC ^ PIX_DST),mem_screen,0,0);
        (void) pix_clr(screen, index, hsr);
        if (errors) {
            display(mem_screen, mem_screen1);
            clean_up();
            exit(DATA_ERROR);
        }
    }
}
/*
 *************************************************************************
 *	bldpolygons
 *
 *	Build the polygon structure for the points of the polygon to be
 *	posted to the GP2 board.
 *************************************************************************
 */

bldpolygons(model)
    struct  polygon *model;
{
    int                 i, j, k, l, ncol, nrow;
    struct point       *ptr, *modelptr;
    float               xoff, yoff;
    int                *nvptr;
    int points_per_poly;


    if ( !CgTWO ) bld24bitcolor();
    ncol = (int) (2.0 / model->x_max);
    nrow = (int) (2.0 / model->y_max);
    ptr = coordlst;
    xoff = -1.0;
    nvptr = nvert;
    npoly = 0;
    for (i = 0; i < ncol; i++) {
        yoff = -1.0;
        for (j = 0; j < nrow; j++) {
            points_per_poly = 0;
            polylst[npoly].nbnds = model->nbnds;
            polylst[npoly].nvptr = nvptr;
            polylst[npoly].coordptr = ptr;
            modelptr = model->coordptr;
            for (k = 0; k < model->nbnds; k++) {
                *nvptr++ = model->nvptr[k];
                for (l = 0; l < model->nvptr[k]; l++) {
                    ptr->x = modelptr->x + xoff;
                    ptr->y = modelptr->y + yoff;
                    ptr->z = modelptr->z;
                    ptr->i = modelptr->i;
                    *ptr++;
                    *modelptr++;
                    points_per_poly++;
                }
            }    
            yoff += model->y_max;
            npoly++;
        }
        xoff += model->x_max;
    }
    return(points_per_poly);
}

/*
 ******************************************************************
 *      read the frame buffer and generate a check sum value from
 *      the screen.
 *
 *      Returns: checked sum value
 ******************************************************************
 */
u_long
pix_arbiter_chk(memptr,x,y,width,height)
    Pixrect *memptr;
    int x,y,width,height;
{
    u_long d_add = 0,temp = 0;
    u_long *addr;
    int i = 0,j = 0,sizex = 0;
    register struct mprp_data *prd;
 
    check_input();
#ifdef cg9var_DEFINED
    prd =  &(gp1_d(memptr)->cgpr.cg9pr.mprp);
#endif
    sizex = memptr->pr_size.x;
    if(memptr->pr_depth == 1)
    {
        width = width/32;
        x = x/32;
        sizex = sizex/32;
    }
    else if(memptr->pr_depth == 8)
    {
         width = width/4;
         x = x/4;
        sizex = sizex/4;
    }
    d_add = 0;
    for (i = y; i < (((y+height-1)<=memptr->pr_size.y)?y+height:memptr->pr_size.y); i++) {
        addr = (u_long *) prd->mpr.md_image;
        addr += (i*sizex)+x;
        for (j = x; j < (((x+width-1)<=sizex)?x+width:sizex); j++) {
                temp = *addr++;
                if(memptr->pr_depth == 32) temp = temp & 0x00ffffff;
                d_add += temp;
                /*d_add += *addr++;*/
        }
    }
    return(d_add);
}

/*
 ******************************************************************
 *      read the frame buffer and generate a check sum value from
 *      the screen.
 *
 *      Returns: checked sum value
 ******************************************************************
 */
u_long
pix_chk(memptr)
    Pixrect *memptr;
{
    u_long d_add;
    u_long *addr;
    int i;

    check_input();
    if ( CgTWO ) {
       register struct cg2fb *fb;
       register struct cg2pr *prd;
       prd = (struct cg2pr *) memptr->pr_data;
       fb = prd->cgpr_va;
       fb->ppmask.reg = 0xFF;
       fb->status.reg.ropmode = SRWPIX;
       addr = (u_long *) fb->ropio.roppixel.pixel;
    }
#ifdef cg9var_DEFINED
    else {
       register struct mprp_data *prd;
       prd = &(gp1_d( memptr)->cgpr.cg9pr.mprp);
       addr = (u_long *) prd->mpr.md_image;
    }
#endif

    d_add = 0;
    for (i = 0; i < 0x3f480; i++) {
    	d_add += *addr;
     	addr++;
    }
    return(d_add);
}

/*
 ****************************************************************
 *      Read the frame buffer memory and check to see if any
 *      pixels are left on the screen after the xor rop.
 *      If any pixels are left on the screen an error message
 *      will be diplayed up to 8 pixels can be bad then the
 *      diagnostic "gp2test" is exited.
 ****************************************************************
 */
pix_clr(memptr, index, hsr)
    Pixrect *memptr;
    int index;
    char *hsr;
{
    u_long *addr,*taddr,data;
    int i;

    check_input();
    if (CgTWO ) {
       register struct cg2fb *fb;
       register struct cg2pr *prd;
       prd = (struct cg2pr *) memptr->pr_data;
       fb = prd->cgpr_va;
       fb->ppmask.reg = 0xFF;
       fb->status.reg.ropmode = SRWPIX;
       addr = (u_long *) fb->ropio.roppixel.pixel;
    }
#ifdef cg9var_DEFINED
    else {
       register struct mprp_data *prd;
       prd = &(gp1_d( memptr)->cgpr.cg9pr.mprp);
       addr = (u_long *) prd->mpr.md_image;
    }
#endif

    taddr = addr;
    for (i = 0; i < 0x3f480; i++) {
    	data = *taddr;
        if (data != 0) {
            errors++;
	    (void) sprintf(msg, pix_clr_msg, taddr-addr,data,index,hsr);
	    gp_send_message(-DATA_ERROR, ERROR, msg);
        }
        if (errors >= 8)
       	    return(8);
        taddr++;
   }
   return(errors);
}

/*
 *****************************************************************
 *      display the memory rops on the screen.
 *      This routine is used as an error message to give visual
 *      indication of what the cause of the error looks like.
 *      Upon exit the diagnostic "gp2test" is exited.
 *****************************************************************
 */
display(screen1,screen2)
    Pixrect *screen1,*screen2;
{
    extern mon_color;       /* 0=console on cgtwo0, 1 mono /dev/fb */
    extern return_code;
 
    char test='0';
 
    if (return_code == CG_ONLY) { /* if console is color board redisplay */
    	unlock_devtop();
        clean_up();
    } else {
        reset_signals();
    }
    if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
	gp_send_message(-SYNC_ERROR, FATAL, display_msg1);
    while (test != 'Q') {
    	(void) printf("1   To display 1st screen:\n");
        (void) printf("2   To display 2nd screen:\n");
        (void) printf("3   To display screen1 XOR screen2:\n");
        (void) printf("c   To continue\n");
        (void) printf("Q   Quit:\n");
        
        test = getchar();      
        (void) getchar();             /* remove "\n" */
 
        if (test == 'Q')
            return;

        switch(test) {
            case '1' :
                setup_signals();
                setup_desktop();
                loadmap();
                pr_rop(screen,0,0,screen->pr_width,screen->pr_height,PIX_SRC,screen1,0,0);
                break;
            case '2' :
                setup_signals();
                setup_desktop();
                loadmap();
                pr_rop(screen,0,0,screen->pr_width,screen->pr_height,PIX_SRC,screen2,0,0);
                break;
            case '3' :
                setup_signals();
                setup_desktop();
                loadmap();
                pr_rop(screen,0,0,screen->pr_width,screen->pr_height,PIX_SRC,screen2,0,0);
                if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
	            gp_send_message(-SYNC_ERROR, FATAL, display_msg2);
                pr_rop(screen,0,0,screen->pr_width,screen->pr_height,(PIX_SRC ^ PIX_DST),screen1,0,0);
                break;
	}
        if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
            gp_send_message(-SYNC_ERROR, FATAL, display_msg3); 
        if (return_code == CG_ONLY) { /* if console is color board redisplay */
            while (test != 'c') {
                test = getchar();
                (void) getchar();
            }
            unlock_devtop();
            clean_up();
        } else {
            reset_signals();
        }
    }
}

bld24bitcolor()

{
	int	count;

	for ( count = 0; count < modelnvert_0[0]; count++ )
		modellst_0[count].i = cmodellst_0[count];

	for ( count = 0; count < modelnvert_1[0]; count++ )
		modellst_1[count].i = cmodellst_1[count];

	for ( count = 0; count < modelnvert_2[0]; count++ )
		modellst_2[count].i = cmodellst_2[count];

	for ( count = 0; count < modelnvert_3[0]; count++ )
		modellst_3[count].i = cmodellst_3[count];

	for ( count = 0; count < modelnvert_4[0]; count++ )
		modellst_4[count].i = cmodellst_4[count];

}
