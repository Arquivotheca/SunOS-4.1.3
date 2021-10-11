#ifndef lint
static	char sccsid[] = "@(#)gp2_arbiter.c 1.1 92/07/30 Copyright Sun Micro";
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
#include <sun/gpio.h>
#include <suntool/sunview.h>
#include <suntool/fullscreen.h>
#include <suntool/gfx_hs.h>
#include "gp2test.h"
#include "gp2test_msgs.h"
#include "sdrtns.h"

#ifndef PIXPG_24BIT_COLOR
#define PIXPG_24BIT_COLOR  5
#endif PIXPG_24BIT_COLOR
extern char *sprintf();

extern struct pixrect *screen;
extern short *gp1_shmem;
extern int ioctlfd;
/*struct pixrect *mem_screen3, *mem_screen4, *mem_screen5, *mem_screen6, *mem_screen7, *mem_screen8;;
struct pixrect *mem_screen;*/

/*
 *************************************************************************
 *	do_arbitration
 *
 *           IF OPTION == 0
 *	1).  clear entire screen, including overlay & overlay enable planes
 *      2).  fill the 4 windows to be used for arbitration testing
 *      2a.  compute checksums without arbitration happening
 *	3).  call routine to post the polygons. (gp2_arb_poly3)
 *      4).  do ROP loop and check sum after each rop
 *      5).  sync to wait for gp2 then compute checksum for entire screen
 *      6).  repeat process but do rops into overlay plane
 *           ELSE IF OPTION == 1
 *      1).  set 12 bit mode to true
 *      1a.  compute checksums without arbitration happening
 *	2).  call routine to post the polygons in buffer 1
 *      3).  do ROP loop buffer 1 and check sum after each rop
 *      4).  sync to wait for gp2 then compute checksum for entire screen 1
 *      5).  flip buffers and repeat 2 through 6
 *           ENDIF
 *	6).  if no errors return to calling routine.
 *************************************************************************
 */

#define GP2COUNT  30
#define HOSTCOUNT 2
do_arbitration(option)
int option; /* arbitration pass 0 or pass 1 */
{
    int index, index2;
    u_long chksum=0,chksum1=0,chksum2=0,chksum3=0,chksum4=0,chksum5=0,chksum6=0;
    u_long chksum7=0,chksum8=0;

    switch(option)
    {
        case 0:

           /*
            * Clear entire screen including overlay & enable.
            */

            Crane_Init();

           /*
            * Clear screen to white.
            */

            pr_rop(screen,0,0,screen->pr_size.x,screen->pr_size.y,PIX_SRC|PIX_COLOR(0xffffff),0,0,0);

           /*
            * Color the 4 windows to be used for arbitration.
            */

            window_layout(0xf2050f);

            for(index2 = 0; index2 < 2; index2++)
            {
                if(index2 == 0)
                {
                    pr_set_plane_group ( screen, PIXPG_24BIT_COLOR);
                }
                else
                {
                    pr_set_plane_group ( screen, PIXPG_OVERLAY_ENABLE );
                    pr_rop(screen,581,10,561,435,PIX_SRC|PIX_COLOR(0x1),0,0,0);
                    pr_set_plane_group ( screen, PIXPG_OVERLAY );
                }
           
               /*
                * Compute checksums for first pass.
                */

                if(index2) pr_rop(screen,581,10,561,435,PIX_SRC|PIX_COLOR(0x0),0,0,0);
                else pr_rop(screen,581,10,561,435,PIX_SRC|PIX_COLOR(0x00ff00),0,0,0);
                chksum1 =  pix_arbiter_chk(screen,581,10,561,435);
                if(index2) pr_rop(screen,581,10,561,435,PIX_SRC|PIX_COLOR(0x1),0,0,0);
                else pr_rop(screen,581,10,561,435,PIX_SRC|PIX_COLOR(0x0000ff),0,0,0);
                chksum2 =  pix_arbiter_chk(screen,581,10,561,435); 
                gp2_arb_poly3(2,0,0,576,450);
                sync();
    	        chksum3 = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                if(index2)
                {
                    pr_set_plane_group(screen, PIXPG_24BIT_COLOR);
                    chksum5 = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                    pr_set_plane_group(screen, PIXPG_OVERLAY_ENABLE );
                    chksum6 = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                    pr_set_plane_group(screen, PIXPG_OVERLAY );
                }
                gp2_arb_poly3(1,0,0,576,450);
                sync();
    	        chksum4 = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                if(index2)
                {
                    pr_set_plane_group(screen, PIXPG_24BIT_COLOR);
                    chksum7 = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                    pr_set_plane_group(screen, PIXPG_OVERLAY_ENABLE );
                    chksum8 = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                    pr_set_plane_group(screen, PIXPG_OVERLAY );
                }

               /*
                * Post polygons in upper left corner.
                */

                gp2_arb_poly3(GP2COUNT*2,0,0,576,450);

               /*
                * Do VME rops to upper right window while GP2 still running.
                * After each rop compute checksum and check against precalculated value.
                */
      
                if(index2) 
                {
                    do_vme_roploop(581,10,561,435,0x0,0x1,chksum1,chksum2,HOSTCOUNT*32);
                }
                else 
                {
                    do_vme_roploop(581,10,561,435,0x00ff00,0x0000ff,chksum1,chksum2,HOSTCOUNT);
                }
           
               /*
                * Wait for GP2 to finish.
                */

                sync();

               /*
                * Compute checksum for entire screen, check against value.
                */
    
    	        chksum = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                compare_sums(chksum,chksum3);
                if(index2)
                {
                    pr_set_plane_group(screen, PIXPG_24BIT_COLOR);
                    chksum = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                    compare_sums(chksum,chksum5);
                    pr_set_plane_group(screen, PIXPG_OVERLAY_ENABLE );
                    chksum = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                    compare_sums(chksum,chksum6);
                    pr_set_plane_group(screen, PIXPG_OVERLAY );
                }

                gp2_arb_poly3(1,0,0,576,450);
                sync();
                chksum = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                compare_sums(chksum,chksum4);
                if(index2)
                {
                    pr_set_plane_group(screen, PIXPG_24BIT_COLOR);
                    chksum = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                    compare_sums(chksum,chksum7);
                    pr_set_plane_group(screen, PIXPG_OVERLAY_ENABLE );
                    chksum = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                    compare_sums(chksum,chksum8);
                    pr_set_plane_group(screen, PIXPG_OVERLAY );
                }

                pr_set_plane_group(screen, PIXPG_OVERLAY);
                pr_rop(screen,581,10,561,435,PIX_SRC|PIX_COLOR(0x0),0,0,0);
                pr_set_plane_group (screen, PIXPG_OVERLAY_ENABLE );
                pr_rop(screen,581,10,561,435,PIX_SRC|PIX_COLOR(0x0),0,0,0);
                pr_set_plane_group(screen, PIXPG_24BIT_COLOR);
            }
            break;

        case 1:
            for(index = 0; index < 2; index++)
            {
                set_dblbf_attributes(index);

               /*
                * Compute checksums for 2nd pass.
                */

                pr_rop(screen,10,455,561,435,PIX_SRC|PIX_COLOR(0x00ff00),0,0,0);
                chksum1 =  pix_arbiter_chk(screen,10,455,561,435);
                pr_rop(screen,10,455,561,435,PIX_SRC|PIX_COLOR(0x0000ff),0,0,0);
                chksum2 =  pix_arbiter_chk(screen,10,455,561,435); 
                gp2_arb_poly3(2,575,449,576,450);
                sync();
    	        chksum3 = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                gp2_arb_poly3(1,575,449,576,450);
                sync();
    	        chksum4 = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
            
               /*
                * Post polygons in lower left corner.
                */

                gp2_arb_poly3(GP2COUNT*2,575,449,576,450);

               /*
                * Do VME rops to upper right window while GP2 still running & check sums.
                */
      
                do_vme_roploop(10,455,561,435,0x00ff00,0x0000ff,chksum1,chksum2,HOSTCOUNT);
           
               /*
                * Wait for GP2 to finish.
                */

                sync();

               /*
                * Compute checksum for entire screen, check against initial value.
                */

    	        chksum = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                compare_sums(chksum,chksum3);
 
               /*
                * Spit out polys, chksum screen & compare against initial value.
                */

                gp2_arb_poly3(1,575,449,576,450);
                sync();
    	        chksum = pix_arbiter_chk(screen,0,0,screen->pr_size.x,screen->pr_size.y);
                compare_sums(chksum,chksum4);
            }
            break;
    }
    return;
}

set_dblbf_attributes(buf)
int buf; /* 0 == buffer A, 1 == buffer B */
{
    int attributes[7];
    attributes[0] = PR_DBL_WRITE;
    attributes[2] = PR_DBL_READ;
    attributes[4] = PR_DBL_DISPLAY;
    attributes[6] = 0;
    if(buf == 0)
    {
        attributes[1] = attributes[3] = attributes[5] = PR_DBL_A;
    }
    else
    {
        attributes[1] = attributes[3] = attributes[5] = PR_DBL_B;
    }
    pr_dbl_set(screen,&attributes[0]);
    return;
}

do_vme_roploop(x,y,w,h,clr1,clr2,sum1,sum2,count)
int x,y,w,h,clr1,clr2;
u_long sum1,sum2;
int count;
{
    int index;
    u_long chksum;
    for(index = 0; index < count; index++)
    {
        pr_rop(screen,x,y,w,h,PIX_SRC|PIX_COLOR(clr1),0,0,0);
        chksum = pix_arbiter_chk(screen,x,y,w,h);
        compare_sums(chksum,sum1);
        pr_rop(screen,x,y,w,h,PIX_SRC|PIX_COLOR(clr2),0,0,0);
        chksum =  pix_arbiter_chk(screen,x,y,w,h);
        compare_sums(chksum,sum2);
    }
    return;
}

window_layout(color)
int color;
{
    pr_rop(screen,10,10,561,435,PIX_SRC|PIX_COLOR(color),0,0,0);   /* upper left */
    pr_rop(screen,581,10,561,435,PIX_SRC|PIX_COLOR(color),0,0,0);  /* upper right */
    pr_rop(screen,10,455,561,435,PIX_SRC|PIX_COLOR(color),0,0,0);  /* bottom left */
    pr_rop(screen,581,455,561,435,PIX_SRC|PIX_COLOR(color),0,0,0); /* bottom right */
    return;
}

sync()
{
    if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
    {
        gp_send_message(-SYNC_ERROR, FATAL, do_arbitration_msg1);
    }
    return;
}

compare_sums(sum1,sum2)
u_long sum1,sum2;
{
    if(sum1 != sum2)
    {
        printf("\n\t Comparing Checksum against previously computed value \n"); 
        printf("\t SUM1 = %d \n",sum1);
        printf("\t SUM2 = %d \n",sum2);
        gp_send_message(-DATA_ERROR, ERROR, do_arbitration_msg2);
    }
    return;
}

