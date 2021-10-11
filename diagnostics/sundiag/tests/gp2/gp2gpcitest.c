#ifndef lint
static	char sccsid[] = "@(#)gp2gpcitest.c 1.1 92/07/30 Copyright Sun Micro";
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
#include <pixrect/cg2reg.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/gp1cmds.h>
#include <sun/gpio.h>
#include <suntool/sunview.h>
#include <suntool/fullscreen.h>
#include <suntool/gfx_hs.h>
#include "gp2test.h"
#include "gp2test_msgs.h"
#include "sdrtns.h"

#ifndef GP2_XF_PGON_FLT_3D_RGB
#define GP2_XF_PGON_FLT_3D_RGB          (83 << 8)
#define PIXPG_24BIT_COLOR 5
#define GP2_RGB_COLOR_PACK 2
#define GP2_SET_DITHER                  (90 << 8)
#define GP2_RGB_COLOR_TRIPLE        1
#endif

extern char *sprintf();

extern struct pixrect *screen;
extern int CgTWO;

short *gp1_shmem;
short statblk;
int ioctlfd;
char minordev;
float xscale, xoffset, yscale, yoffset, zscale, zoffset;
Pixrect *mem_screen;
Pixrect *mem_screen1;
/* 
 ************************************************************************* 
 *	Open the gp2 device, get the ioctl file discripter, and get
 *	pointer to shared ram.
 *************************************************************************
 */
opengp()
{
    int i;

    for (i = 0; i < 200 ;i++) {
        if ((screen = pr_open(GP_DEV)) == 0)
            sleep(1);
        else
            break;
    }  
    if ( !CgTWO )
       pr_set_plane_group( screen, PIXPG_24BIT_COLOR );
    if(i >= 199) {
	(void) sprintf(msg, opengp_msg1, GP_DEV);
        gp_send_message(-DEV_NOT_OPEN, FATAL, msg);
    }
    gp1_shmem = (short *) (gp1_d(screen)->gp_shmem);
    ioctlfd = gp1_d(screen)->ioctl_fd;
    minordev = gp1_d(screen)->minordev;
    if ((statblk = gp1_get_static_block(ioctlfd)) == -1)
        gp_send_message(-DEV_NO_STATBLK, FATAL, opengp_msg2);
}
/* 
 ************************************************************************* 
 *	1). get a gp2 context block from shared ram.
 *	2). set color to white.
 *	3). set to rop.
 *	4). set line width, and line text.
 *	5). view port to 3d and whole screen size.
 *	6). set clip list to whole screen.
 *************************************************************************
 */
init_static_blk()
{
    unsigned int        bitvec;
    register int        offset;
    register short     *shmptr;
    int                 planesmask;
    float               ftemp;

    offset = 0;
    offset = gp1_alloc(gp1_shmem, 2, &bitvec, minordev, ioctlfd);
    if (offset == 0)
         gp_send_message(-DEV_NO_STATBLK, FATAL, init_blk_msg1);
    shmptr = &((short *) gp1_shmem)[offset];
    *shmptr++ = GP1_USE_CONTEXT | statblk;
    *shmptr++ = GP1_SET_COLOR | WHITE_COLOR;
    *shmptr++ = GP1_SET_FB_NUM | gp1_d(screen)->cg2_index;
    *shmptr++ = GP1_SET_ROP;
    *shmptr++ = PIX_SRC;
    pr_getattributes(screen, &planesmask);
    if ( CgTWO )
       *shmptr++ = GP1_SET_FB_PLANES | planesmask;
    *shmptr++ = GP1_SET_CLIP_PLANES | 0x3F;
    *shmptr++ = GP1_SET_MAT_NUM | 0;
    *shmptr++ = GP1_SET_LINE_WIDTH;
    *shmptr++ = 1;
    *shmptr++ = 0;
    *shmptr++ = GP1_SET_LINE_TEX;
    *shmptr++ = 0;
    *shmptr++ = 0;
    *shmptr++ = 0;
    *shmptr++ = GP1_SET_VWP_3D;
    ftemp = 100.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 576.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 100.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 450.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    *shmptr++ = GP1_SET_CLIP_LIST;
    *shmptr++ = 1;
    *shmptr++ = 0;
    *shmptr++ = 0;
    *shmptr++ = 1152;
    *shmptr++ = 900;
    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
    GP1_PUT_I(shmptr, bitvec);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
        gp_send_message(-POST_ERROR, FATAL, init_blk_msg2);
}

/*
 ***********************************************************************
 *      This routine will test the gp2 hardware
 *      by calling the diagnostic tests that are in
 *      the micro code on the GP2 board.
 ***********************************************************************
 */
test_hardware()
{
    short *shmptr;
    short *shmptr1;
    int offset;
    unsigned int bitvec;

    if ((!exec_by_sundiag) || (debug))
    	gp_send_message(0, DEBUG, "\n\tTesting Hardware\n");
    offset = 0;
    offset = gp1_alloc(gp1_shmem, 2, &bitvec, minordev, ioctlfd);
    if (offset == 0)
        gp_send_message(-DEV_NO_STATBLK, FATAL, hardware_msg1);
    shmptr = &((short *) gp1_shmem)[offset];

    errors += test_xp_local_ram(shmptr, offset);
    errors += test_xp_shared_ram(shmptr, offset);
    errors += test_xp_sequencer(shmptr, offset);
    errors += test_xp_alu(shmptr, offset);
    errors += test_xp_rp_fifo(shmptr, offset);
    errors += test_rp_local_ram(shmptr, offset);
    errors += test_rp_shared_ram(shmptr, offset);
    errors += test_rp_sequencer(shmptr, offset);
    errors += test_rp_alu(shmptr, offset);
    errors += test_rp_pp_fifo(shmptr, offset);
    errors += test_pp_ldx_ago(shmptr, offset);
    errors += test_pp_ady_ago(shmptr, offset);
    errors += test_pp_adx_ago(shmptr, offset);
    errors += test_pp_sequencer(shmptr, offset);
    errors += test_pp_alu(shmptr, offset);
    errors += test_pp_rw_zbuf(shmptr, offset);
    errors += test_pp_zbuf(shmptr, offset);

    *shmptr = GP1_EOCL | GP1_FREEBLKS;
    shmptr1 = shmptr+1;
    GP1_PUT_I(shmptr1, bitvec);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) !=0)
        gp_send_message(-POST_ERROR, FATAL, hardware_msg2);
    if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
        gp_send_message(-SYNC_ERROR, FATAL, hardware_msg3);
}
/*
 ********************************************************************
 * 	test_mul_matrix
 *	   Tests the GPCI commands:
 *         	GP1_SET_MAT_3D
 *		GP1_GET_MAT_3D
 *		GP1_MUL_MAT_3D
 *
 *	This tests the above commands to make sure the XP floating 
 *	point chip works properly.
 *	1). test by multiplying 2 matrixes together and have correct results.
 *	2). test rotate around "X" axis, check for correct results.
 *	3). test rotate around "Y" axis, check for correct results.
 *	4). test rotate around "Z" axis, check for correct results.
 *	5). test multiplying depth correctly.
 ********************************************************************
 */
test_mul_matrix()
{
    short *shmptr;
    int offset;
    unsigned int bitvec;
    float  mat[4][4];
    int dest, k;
    char axis;

    if ((!exec_by_sundiag) || (debug))
    	gp_send_message(0, DEBUG, "\n\tTesting 3D matrix commands\n");
    offset = 0;
    offset = gp1_alloc(gp1_shmem, 2, &bitvec, minordev, ioctlfd);
    if (offset == 0)
         gp_send_message(-DEV_NO_STATBLK, FATAL, mul_matrix_msg1);
    shmptr = &((short *) gp1_shmem)[offset];
    mat_ident(mat);
    for(k = 0; k < MATRIX_TESTS; k++) {
        switch(k) {
            case 0:
		dest = 0;
		test_translation(mat, shmptr, offset, dest);
		dest = 1;
		break;
            case 1:
		dest = 0;
	 	axis = 'x';
		test_rotate(axis, mat);
		break;
            case 2:
		dest = 0;
	 	axis = 'y';
		test_rotate(axis, mat);
		break;
            case 3:
		dest = 0;
	 	axis = 'z';
		test_rotate(axis, mat);
		break;
	    case 4:
		dest = 0;
		test_scale(mat);
		break;
	    case 5:
		dest = 0;
		test_eye(mat);
		break;
	}
	shmptr = &((short *) gp1_shmem)[offset];
        set_mat_in_ram(mat, shmptr, offset, dest);
        multiply_matrices(mat, shmptr, offset);
    }
    shmptr = &((short *) gp1_shmem)[offset];
    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
    GP1_PUT_I(shmptr, bitvec);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
        gp_send_message(-POST_ERROR, FATAL, mul_matrix_msg2);
}

/*
 *************************************************************************
 *	Test_mul_point
 *	   Tests the GPCI commands:
 *		GP1_MUL_POINT_FLT_3D
 *		GP1_MUL_POINT_INT_3D
 *
 *	Transforms the the 3d points using the selected transform matrix.
 *	read back the points contained in the command block and verify
 *	that they were transformed correctly. The host generates the points
 *	that should be correct.
 *************************************************************************
 */
test_mul_point()
{
    short *shmptr;
    int offset;
    unsigned int bitvec;
    float  mat[4][4];
    int k, dest = 0;
    float points[4 * MAXPTS];
    float *fp2;

    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\n\tTesting MUL_POINT_xxx_3D commands\n");
    offset = 0;
    offset = gp1_alloc(gp1_shmem, 2, &bitvec, minordev, ioctlfd);
    if (offset == 0)
         gp_send_message(-DEV_NO_STATBLK, FATAL, mul_point_msg1);
/*
 * The following code needs to be here if you want to call
 * the test_mul_point() function without calling the prior test,
 * test_mul_matrix() to initialize your matrix, AND you want your
 * results to be identical to those of the older version of this test.
 */ 
    mat_ident(mat);
    test_eye(mat);
    shmptr = &((short *) gp1_shmem)[offset];
    set_mat_in_ram(mat, shmptr, offset, dest);
    multiply_matrices(mat, shmptr, offset);
/*
    shmptr = &((short *) gp1_shmem)[offset];
    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
    GP1_PUT_I(shmptr, bitvec);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
       gp_send_message(-POST_ERROR, FATAL, "Error Mul point 2: post failed");
*/
/* 
 * end of setup
 */
    shmptr = &((short *) gp1_shmem)[offset];
    get_matrix(mat, shmptr, offset, dest);
    for (k = 0; k < POINT_TESTS; k++) {
        get_pts(k, points);
	fp2 = set_pts_in_ram(points, shmptr, offset);
	errors += multiply_pts(mat, points, fp2);
    }
    shmptr = &((short *) gp1_shmem)[offset];
    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
    GP1_PUT_I(shmptr,bitvec);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
        gp_send_message(-POST_ERROR, FATAL, mul_point_msg2);
}
/*
 ********************************************************************
 *	test_proc_line
 *	  tests the GPCI commands:
 *		GP1_PROC_LINE
 *	
 *	Tests the ability of the GP2 to take 3D coordinates and 
 *	clip them to the view port and check that screen coordinates
 *	that are calculated are correct.
 ********************************************************************
 */
test_proc_line()
{
    short *shmptr;
    int offset;
    unsigned int bitvec;
    float  mat[4][4];
    int k, dest = 3;
    float fpoint0[4];
    float fpoint1[4];
    float points[8 * MAXLINES];
    int *ip_r_start, *ip_r_end;

    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\n\tTesting PROC_LINE_xxx_3D commands\n");
    offset = 0;
    offset = gp1_alloc(gp1_shmem, 2, &bitvec, minordev, ioctlfd);
    if (offset == 0)
    	gp_send_message(-DEV_NO_STATBLK, FATAL, proc_line_msg1);
    shmptr = &((short *) gp1_shmem)[offset];
    mat_ident(mat);
    shmptr = set_view_port(fpoint0, fpoint1, shmptr, dest);
    shmptr = set_matrix(mat, shmptr, offset, dest);
    for (k = 0; k < LINE_TESTS; k++) {
	get_line(k, points);
        shmptr = &((short *) gp1_shmem)[offset];
        set_line(shmptr, offset, points, &ip_r_start, &ip_r_end);
        multiply_line(mat, fpoint0, fpoint1, points, ip_r_start, ip_r_end);
        check_line(k, fpoint0, fpoint1, ip_r_start, ip_r_end);
    }
    shmptr = &((short *) gp1_shmem)[offset];
    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
    GP1_PUT_I(shmptr,bitvec);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) !=0)
    	gp_send_message(-POST_ERROR, FATAL, proc_line_msg2);
}
/*
 ********************************************************************
 *	test_polygons
 *
 *	This routine calls the functions that will display polygons
 *	on the screen that are generated by passing parameters to the
 *	gp2's context blocks. 
 *
 *	First create 2, 1 meg screen save areas in memory then test
 *	with hsr mode turned on, then hsr mode turned off.  
 *	The mem_screens are used to save the 1st and 2nd paints of
 *	each pattern and compare them each other. If they don't match
 *	and error will be displayed.
 ********************************************************************
 */
test_polygons()
{
    char *hsr;
    int k;
    int planes = GP2_BUFSIZ - 1;

    if ( !CgTWO )
       planes = 0xFFFFFF;

    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\n\tTesting Polygons Commands\n");
    mem_screen = create_screen();
    mem_screen1 = create_screen();
    loadmap();
    for (k = 0; k < 2; k++) {
        switch(k) {
            case 0:
		hsr = "on";
		init_static_blk1();
		break;
            case 1:
		hsr = "off";
		init_static_blk2();
		break;
        }
	pr_putattributes(screen, &planes);    
	paint_polygons(hsr, mem_screen, mem_screen1);
    }
}

/*
 ********************************************************************
 *      test_arbiter
 *
 *      This routine calls the functions that will test the arbiter
 *      by writing into the cg9 from the p2 and vme simultaneously.
 *
 *      First create 2, 1 meg screen save areas in memory then test
 *      with hsr mode turned on, then hsr mode turned off.
 *      The mem_screens are used to save the 1st and 2nd paints of
 *      each pattern and compare them each other. If they don't match
 *      and error will be displayed.
 ********************************************************************
 */
test_arbiter()   
{
    int k;
    int planes = 0xFFFFFF;

    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\n\tTesting Arbitration \n");
    loadmap();
    for (k = 0; k < 2; k++)
    {
        init_arbiter(k);
        pr_putattributes(screen, &planes);
        do_arbitration(k);
    }
}
 
/*
 ********************************************************************
 *      init_arbiter
 *
 *      1). set color to blue
 *      2). set to rop mode
 *      3). set hidden serface removal on.
 *      4). set width and line text.
 *      5). set view port whole screen.
 *      6). set to normal transform matrix.
 *      7). set clip list to whole screen.
 *      8). if option == 1 then set dither on
 ********************************************************************
 */
init_arbiter(dither_on_off)
int dither_on_off;
{
    unsigned int bitvec;
    int offset;
    int planesmask;
    short *shmptr;
    float ftemp;
 
    offset = 0;
    offset = gp1_alloc(gp1_shmem, 1, &bitvec, minordev, ioctlfd);
    if(offset == 0)
         gp_send_message(-DEV_NO_STATBLK, FATAL, init_arbiter_msg1);
 
    shmptr = &((short *) gp1_shmem)[offset];
    *shmptr++ = GP1_USE_CONTEXT | statblk;
    *shmptr++ = GP1_SET_FB_NUM | gp1_d(screen)->cg2_index;
    *shmptr++ = GP1_SET_ROP;
    *shmptr++ = PIX_SRC;
    pr_getattributes(screen, &planesmask);
    *shmptr++ = GP1_SET_CLIP_PLANES | 0x3F;
    *shmptr++ = GP1_SET_HIDDEN_SURF | GP1_ZBHIDDENSURF;
    *shmptr++ = GP1_SET_MAT_NUM | 0;
    *shmptr++ = GP1_SET_LINE_WIDTH;
    *shmptr++ = 1;
    *shmptr++ = 0;
    *shmptr++ = GP1_SET_LINE_TEX;
    *shmptr++ = 0;
    *shmptr++ = 0;
    *shmptr++ = 0;
    *shmptr++ = GP1_SET_VWP_3D;
    ftemp = 576.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 576.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = -450.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 450.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 30000.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    *shmptr++ = GP1_SET_MAT_3D | 0;
    ftemp = 1.0;
    GP1_PUT_F(shmptr, ftemp);

    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 1.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 1.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 1.0;
    GP1_PUT_F(shmptr, ftemp);
    *shmptr++ = GP1_SET_CLIP_LIST;
    *shmptr++ = 1;
    *shmptr++ = 0;
    *shmptr++ = 0;
    *shmptr++ = 1152;
    *shmptr++ = 900;
    *shmptr++ = GP2_SET_DITHER | dither_on_off;
    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
    GP1_PUT_I(shmptr, bitvec);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
        gp_send_message(-SYNC_ERROR, FATAL, init_arbiter_msg2);
}

/*
 ********************************************************************
 *      gp2_arb_poly3
 *      
 *      This routine will draw some polys in a window and iterate
 *      given number of times switching colors.
 ********************************************************************
 */ 
 
gp2_arb_poly3(count,x,y,width,height)
int count,x,y,width,height;
{
    register int m, i, j, *nvp, overflow;
    unsigned int bitvec;
    register int offset, roomleft;
    register short *shmptr, *nvshmptr;
    register float *ptr;
    int needed;
    float xleft,xright,ytop,ybottom,ftemp,r,g,b;
    int itemp;
    int blocks_per_transfer = 6;
    
    xleft = (float) (((float)x)/((float)(screen->pr_size.x/2.0))-1.0);
    xright = (float)((float)(x+width)/(float)(screen->pr_size.x/2.0)-1.0);
    ytop   = (float)(-(y/(screen->pr_size.y/2.0))+1.0);
    ybottom = (float)(-((y+height)/(screen->pr_size.y/2.0))+1.0);
 
    offset = 0;
    offset = gp1_alloc(gp1_shmem, blocks_per_transfer, &bitvec, minordev, ioctlfd);
    if (offset == 0)
    {
         gp_send_message(-DEV_NO_STATBLK, FATAL, gp2_arb_poly3_msg1);
    }
    shmptr = &((short *) gp1_shmem)[offset];
    *shmptr++ = GP1_USE_CONTEXT | statblk;
 
    for(i = 0; i < count; i++)
    {
 
       /*
        * Clear the z buffer for the specified window.
        */
 
        *shmptr++ = GP1_SETZBUF;
        *shmptr++ = 0xFFFF;
        *shmptr++ = x;
        *shmptr++ = y;
        *shmptr++ = width;
        *shmptr++ = height;
 
        if(i%2)
        {
            b = .75;
            *shmptr++ = GP2_XF_PGON_FLT_3D_RGB | GP2_RGB_COLOR_TRIPLE;
            *shmptr++ = 1;
            *shmptr++ = 4;
            g = ftemp = xleft+.05*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(g < 0.0) g *= -1.0;
            r = ftemp = ytop-.05*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(r < 0.0) r *= -1.0;

            ftemp = 1.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
            g = ftemp = xleft+.5*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(g < 0.0) g *= -1.0;
            r = ftemp = ytop-.05*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(r < 0.0) r *= -1.0;
            ftemp = 1.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
            g = ftemp = xleft+.85*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(g < 0.0) g *= -1.0;
            r = ftemp = ytop-.9*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(r < 0.0) r *= -1.0;
            ftemp = 0.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
            g = ftemp = xleft+.35*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(g < 0.0) g *= -1.0;
            r = ftemp = ytop-.9*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(r < 0.0) r *= -1.0;
            ftemp = 0.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
            r = .75;
            *shmptr++ = GP2_XF_PGON_FLT_3D_RGB | GP2_RGB_COLOR_TRIPLE;
            *shmptr++ = 1;
            *shmptr++ = 4;
            g = ftemp = xleft+.45*(xright-xleft); GP1_PUT_F(shmptr,ftemp);

            if(g < 0.0) g *= -1.0;
            b = ftemp = ytop-.1*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(b < 0.0) b *= -1.0;
            ftemp = 0.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
            g = ftemp = xleft+.95*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(g < 0.0) g *= -1.0;
            b = ftemp = ytop-.1*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(b < 0.0) b *= -1.0;
            ftemp = 0.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
            g = ftemp = xleft+.65*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(g < 0.0) g *= -1.0;
            b = ftemp = ytop-.95*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(b < 0.0) b *= -1.0;
            ftemp = 1.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
            g = ftemp = xleft+.15*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(g < 0.0) g *= -1.0;
            b = ftemp = ytop-.95*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(b < 0.0) b *= -1.0;
            ftemp = 1.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
        }
        else

        {
            g = 0.8;
            *shmptr++ = GP2_XF_PGON_FLT_3D_RGB | GP2_RGB_COLOR_TRIPLE;
            *shmptr++ = 1;
            *shmptr++ = 4;
            r = ftemp = xleft+.05*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(r < 0.0) r *= -1.0;
            b = ftemp = ytop-.05*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(b < 0.0) b *= -1.0;
            ftemp = 1.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
            r = ftemp = xleft+.5*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(r < 0.0) r *= -1.0;
            b = ftemp = ytop-.05*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(b < 0.0) b *= -1.0;
            ftemp = 1.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
            r = ftemp = xleft+.85*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(r < 0.0) r *= -1.0;
            b = ftemp = ytop-.9*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(b < 0.0) b *= -1.0;
            ftemp = 0.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
            r = ftemp = xleft+.35*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(r < 0.0) r *= -1.0;
            b = ftemp = ytop-.9*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(b < 0.0) b *= -1.0;

            ftemp = 0.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
            r = .9;
            *shmptr++ = GP2_XF_PGON_FLT_3D_RGB | GP2_RGB_COLOR_TRIPLE;
            *shmptr++ = 1;
            *shmptr++ = 4;
            b = ftemp = xleft+.45*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(b < 0.0) b *= -1.0;
            g = ftemp = ytop-.1*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(g < 0.0) g *= -1.0;
            ftemp = 0.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
            b = ftemp = xleft+.95*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(b < 0.0) b *= -1.0;
            g = ftemp = ytop-.1*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(g < 0.0) g *= -1.0;
            ftemp = 0.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
            b = ftemp = xleft+.65*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(b < 0.0) b *= -1.0;
            g = ftemp = ytop-.95*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(g < 0.0) g *= -1.0;
            ftemp = 1.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
            b = ftemp = xleft+.15*(xright-xleft); GP1_PUT_F(shmptr,ftemp);
            if(b < 0.0) b *= -1.0;
            g = ftemp = ytop-.95*(ytop-ybottom); GP1_PUT_F(shmptr,ftemp);
            if(g < 0.0) g *= -1.0;
            ftemp = 1.0; GP1_PUT_F(shmptr,ftemp);
            ftemp = r; GP1_PUT_F(shmptr,ftemp);
            ftemp = g; GP1_PUT_F(shmptr,ftemp);
            ftemp = b; GP1_PUT_F(shmptr,ftemp);
        }
    }
    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
    GP1_PUT_I(shmptr, bitvec);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) !=0)
    {
        gp_send_message(-POST_ERROR, FATAL, gp1poly3_msg4);
    }
}


/*
 ********************************************************************
 *	init_static_blk1
 *
 *	1). set color to blue
 *	2). set to rop mode
 *	3). set hidden serface removal on.
 *	4). set width and line text.
 *	5). set view port whole screen.
 *	6). set to normal transform matrix.
 *	7). set clip list to whole screen.
 ********************************************************************
 */
init_static_blk1()
{
    unsigned int bitvec;
    int offset;
    int planesmask;
    short *shmptr;
    float ftemp;

    offset = 0;
    offset = gp1_alloc(gp1_shmem, 1, &bitvec, minordev, ioctlfd);
    if(offset == 0)
         gp_send_message(-DEV_NO_STATBLK, FATAL, init_blk1_msg1);

    shmptr = &((short *) gp1_shmem)[offset];
    *shmptr++ = GP1_USE_CONTEXT | statblk;
    *shmptr++ = GP1_SET_COLOR | BLUE_COLOR;
    *shmptr++ = GP1_SET_FB_NUM | gp1_d(screen)->cg2_index;
    *shmptr++ = GP1_SET_ROP;
    *shmptr++ = PIX_SRC;
    pr_getattributes(screen, &planesmask);
    if ( CgTWO )
       *shmptr++ = GP1_SET_FB_PLANES | planesmask;
    *shmptr++ = GP1_SET_CLIP_PLANES | 0x3F;
    *shmptr++ = GP1_SETHIDDENSURF | GP1_ZBHIDDENSURF;
    *shmptr++ = GP1_SET_MAT_NUM | 0;
    *shmptr++ = GP1_SET_LINE_WIDTH;
    *shmptr++ = 1;
    *shmptr++ = 0;
    *shmptr++ = GP1_SET_LINE_TEX;
    *shmptr++ = 0;
    *shmptr++ = 0;
    *shmptr++ = 0;
    *shmptr++ = GP1_SET_VWP_3D;
    ftemp = 576.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 576.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = -450.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 450.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 30000.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    *shmptr++ = GP1_SET_MAT_3D | 0;
    ftemp = 1.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 1.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 1.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 1.0;
    GP1_PUT_F(shmptr, ftemp);
    *shmptr++ = GP1_SET_CLIP_LIST;
    *shmptr++ = 1;
    *shmptr++ = 0;
    *shmptr++ = 0;
    *shmptr++ = 1152;
    *shmptr++ = 900;
    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
    GP1_PUT_I(shmptr, bitvec);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
        gp_send_message(-SYNC_ERROR, FATAL, init_blk1_msg2);
}
/*
 ********************************************************************
 *      init_static_blk2
 *
 *      1). set color to blue
 *      2). set to rop mode
 *      3). set hidden serface removal off.
 *      4). set width and line text.
 *      5). set view port whole screen.
 *      6). set to normal transform matrix.
 *      7). set clip list to whole screen.
 ********************************************************************
 */
init_static_blk2()
{
    unsigned int bitvec;
    int offset;
    int planesmask;
    short *shmptr;
    float ftemp;
 
    offset = 0;
    offset = gp1_alloc(gp1_shmem, 2, &bitvec, minordev, ioctlfd);
    if(offset == 0)
         gp_send_message(-DEV_NO_STATBLK, FATAL, init_blk2_msg1);

    shmptr = &((short *) gp1_shmem)[offset];
    *shmptr++ = GP1_USE_CONTEXT | statblk;
    *shmptr++ = GP1_SET_COLOR | BLUE_COLOR;
    *shmptr++ = GP1_SET_FB_NUM | gp1_d(screen)->cg2_index;
    *shmptr++ = GP1_SET_ROP;
    *shmptr++ = PIX_SRC;
    pr_getattributes(screen, &planesmask);
    if ( CgTWO )
       *shmptr++ = GP1_SET_FB_PLANES | planesmask;
    *shmptr++ = GP1_SET_CLIP_PLANES | 0x3F;
    *shmptr++ = GP1_SETHIDDENSURF | GP1_NOHIDDENSURF;
    *shmptr++ = GP1_SET_MAT_NUM | 0;
    *shmptr++ = GP1_SET_LINE_WIDTH;
    *shmptr++ = 1;
    *shmptr++ = 0;
    *shmptr++ = GP1_SET_LINE_TEX;
    *shmptr++ = 0;
    *shmptr++ = 0;
    *shmptr++ = 0;
    *shmptr++ = GP1_SET_VWP_3D;
    ftemp = 576.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 576.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = -450.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 450.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 30000.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    *shmptr++ = GP1_SET_MAT_3D | 0;
    ftemp = 1.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 1.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 1.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 0.0;
    GP1_PUT_F(shmptr, ftemp);
    ftemp = 1.0;
    GP1_PUT_F(shmptr, ftemp);
    *shmptr++ = GP1_SET_CLIP_LIST;
    *shmptr++ = 1;
    *shmptr++ = 0;
    *shmptr++ = 0;
    *shmptr++ = 1152;
    *shmptr++ = 900;
    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
    GP1_PUT_I(shmptr, bitvec);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
        gp_send_message(-SYNC_ERROR, FATAL, init_blk2_msg2);
}


/*
 ********************************************************************
 *	clrzb
 *
 *	This routine will clear the z (or depth buffer) for the 
 *	entire screen.
 ********************************************************************
 */
clrzb()
{
    unsigned int bitvec;
    int offset;
    short *shmptr;

    offset = 0;
    offset = gp1_alloc(gp1_shmem, 2, &bitvec, minordev, ioctlfd);
    if (offset == 0)
         gp_send_message(-DEV_NO_STATBLK, FATAL, clrzb_msg1);
    shmptr = &((short *) gp1_shmem)[offset];
    *shmptr++ = GP1_USE_CONTEXT | statblk;
    *shmptr++ = GP1_PR_ROP_NF | 255;
        *shmptr++ = 0;
        *shmptr++ = PIX_SRC | PIX_COLOR(0);
        *shmptr++ = 0;
        *shmptr++ = 0;
        *shmptr++ = 1152;
        *shmptr++ = 900;
        *shmptr++ = 0;
        *shmptr++ = 0;
        *shmptr++ = 1152;
        *shmptr++ = 900;
    *shmptr++ = GP1_SETZBUF;
    *shmptr++ = 0xFFFF;
    *shmptr++ = 0;
    *shmptr++ = 0;
    *shmptr++ = 1152;
    *shmptr++ = 900;
    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
    GP1_PUT_I(shmptr, bitvec);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
    	gp_send_message(-POST_ERROR, FATAL, clrzb_msg2);
}
/*
 ********************************************************************
 *	gp1poly3
 *	
 *	This routine will take the points that were generated by
 *	the bldpolygons routine and post them to the gp2 context
 *	blocks.  If all context blocks have been used it will free
 *	some up that are no longer needed and post some polygons
 *	to the GP2. All of the polygons are Gouraud shaded.
 ********************************************************************
 */
gp1poly3(plst, n, points_per_poly)
    struct polygon plst[];
    int n;
    int points_per_poly;
{
    register int m, i, j, *nvp, overflow;
    unsigned int bitvec;
    register int offset, roomleft;
    register short *shmptr, *nvshmptr;
    register float *ptr;
    int needed;
    float ftemp;
    int itemp;
    int blocks_per_transfer = 1;

    offset = 0;
    offset = gp1_alloc(gp1_shmem, blocks_per_transfer, &bitvec,
                 minordev, ioctlfd);
    if (offset == 0)
         gp_send_message(-DEV_NO_STATBLK, FATAL, gp1poly3_msg1);

/* Allows at most two bands. */
#ifdef GOURAUD
    needed = 4 + 8 * points_per_poly;
#else
    needed = 4 + 6 * points_per_poly;
#endif

    shmptr = &((short *) gp1_shmem)[offset];
    *shmptr++ = GP1_USE_CONTEXT | statblk;
    roomleft = 512 * blocks_per_transfer - 2;
    for (i = 0; i < n; i++) {
        if (roomleft < needed) {
            *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
            GP1_PUT_I(shmptr, bitvec);

PRE_POST_SYNC(gp1_shmem, ioctlfd);
            if ((gp1_post(gp1_shmem, offset, ioctlfd)) !=0)
                gp_send_message(-POST_ERROR, FATAL, gp1poly3_msg2);
POST_POST_SYNC(gp1_shmem, ioctlfd);

            offset = 0;
            offset = gp1_alloc(gp1_shmem, blocks_per_transfer, &bitvec,
                         minordev, ioctlfd);
            if (offset == 0)
                gp_send_message(-DEV_NO_STATBLK, FATAL, gp1poly3_msg1);
            shmptr = &((short *) gp1_shmem)[offset];
            *shmptr++ = GP1_USE_CONTEXT | statblk;
            roomleft = 512 * blocks_per_transfer - 2;
        }


        nvp = plst[i].nvptr;
        ptr = (float *) plst[i].coordptr;
#ifdef GOURAUD
	if ( CgTWO )
		*shmptr++ = GP1_XF_PGON_FLT_3D | GP1_SHADE_GOURAUD;
	else
        	*shmptr++ = GP2_XF_PGON_FLT_3D_RGB | GP2_RGB_COLOR_PACK;
#else
        *shmptr++ = GP1_XF_PGON_FLT_3D | GP1_SHADE_CONSTANT;
#endif
        *shmptr++ = plst[i].nbnds;
        nvshmptr = shmptr;
        shmptr += plst[i].nbnds;
        roomleft -= (plst[i].nbnds + 2);
        overflow = 0;
        for (j = 0; j < plst[i].nbnds; j++) {
            m = *nvp++;
            if (roomleft >= 3 + 8 * m) {
                *nvshmptr++ = m;
                roomleft -= 8 * m;
                while (m--) {
                    ftemp = *ptr++;
                    GP1_PUT_F(shmptr, ftemp);
                    ftemp = *ptr++;
                    GP1_PUT_F(shmptr, ftemp);
                    ftemp = *ptr++;
                    GP1_PUT_F(shmptr, ftemp);
#ifdef GOURAUD
                    itemp = *((int *) ptr)++;
                    GP1_PUT_I(shmptr, itemp);
#else
                    ((int *) ptr)++;
#endif
                }
            }
            else
                overflow = 1;
        }
        if (overflow) {
            shmptr = &((short *) gp1_shmem)[offset];
            gp_send_message(-2, ERROR, gp1poly3_msg3);
            return;
        }
#ifdef ONE_PGON_PER_POST
        roomleft = 0;
#endif
    }
      
    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
    GP1_PUT_I(shmptr, bitvec);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) !=0)
        gp_send_message(-POST_ERROR, FATAL, gp1poly3_msg4);
}
