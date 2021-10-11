#ifndef lint
static	char sccsid[] = "@(#)gp2_point.c 1.1 92/07/30 Copyright Sun Micro";
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
#include <pixrect/gp1cmds.h>
#include <sun/gpio.h>
#include <suntool/sunview.h>
#include <suntool/fullscreen.h>
#include <suntool/gfx_hs.h>
#include "gp2test.h"
#include "gp2test_msgs.h"
#include "sdrtns.h"

extern char *sprintf();

extern int debug, verbose;
extern short *gp1_shmem;
extern int ioctlfd;
extern short statblk;
extern float xscale, xoffset, yscale, yoffset, zscale, zoffset;

/*
 **************************************************************************
 *	Get the 3D matrix stored that we will be using.
 **************************************************************************
 */
get_matrix(mat, shmptr, offset, dest)
    float mat[4][4];
    short *shmptr;
    int offset;
    int dest;
{
    int i;
    float *fp1, *fp2;
 
    *shmptr++ = GP1_USE_CONTEXT | statblk;
    *shmptr++ = GP1_SET_MAT_NUM | dest;
    *shmptr++ = GP1_GET_MAT_3D | dest;
    *shmptr++ = -1;
    fp1 = (float *) shmptr;
    for (i = 0; i < 32; i++)
        *shmptr++ = -1;
    *shmptr++ = GP1_EOCL;
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) !=0)
        gp_send_message(-POST_ERROR, FATAL, get_matrix_msg1);
    if ((gp1_sync(gp1_shmem, ioctlfd)) !=0)
        gp_send_message(-SYNC_ERROR, FATAL, get_matrix_msg2);
    fp2 = (float *) mat;
    bcopy((char *)fp1, (char *)fp2, 16*sizeof(float));
}
/*
 *************************************************************************
 *	get_pts
 *
 *	Get the points that will be used in the transformation.
 *************************************************************************
 */
get_pts(k, points)
    int k;
    float points[4 * MAXPTS];
{
    float *fp1;

    switch(k) {
            case 0 :
                fp1 = points;
                fp1[0] = 1.0;
                fp1[1] = 1.0;
                fp1[2] = 1.0;
                fp1[3] = 1.0;
                break;
            case 1 :
                fp1 = points;
                fp1[0] = 0.000002;
                fp1[1] = 0.000004;
                fp1[2] = 0.000006;
                fp1[3] = 0.000008;
                break;
             case 2 :
                fp1 = points;
                fp1[0] = 999990.0;
                fp1[1] = 999991.0;
                fp1[2] = 999992.0;
                fp1[3] = 999993.0;
                break;
            case 3 :
                fp1 = points;
                fp1[0] = 791.0345;
                fp1[1] = 800.74050;
                fp1[2] = 8.00034;
                fp1[3] = 11.0;
                break;
    }
}
/*
 *************************************************************************
 *      set_pts_in_ram
 *
 *	Send the points fetch in the above function to the GP2 in a 
 *	context block. 
 *	Tell the GP2 to multiply the points with the matrix and put
 *	the result in the GP2 context block.
 *************************************************************************
 */
    
float *
set_pts_in_ram(points, shmptr, offset)
    float points[4 * MAXPTS];
    short *shmptr;
    int offset;
{
    float *fp1, *fp2;

    shmptr = &((short *) gp1_shmem)[offset];
    *shmptr++ = GP1_USE_CONTEXT | statblk;
    *shmptr++ = GP1_MUL_POINT_FLT_3D;
    *shmptr++ = 1;                 /* count */

    fp1 = points;
    fp2 = (float *) shmptr;
    GP1_PUT_F(shmptr, *fp1);
    GP1_PUT_F(shmptr, *(fp1 + 1));
    GP1_PUT_F(shmptr, *(fp1 + 2));
    GP1_PUT_F(shmptr, *(fp1 + 3));
    *shmptr++ = -1;                /* Flag */

    *shmptr++ = GP1_EOCL;
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
        gp_send_message(-POST_ERROR, FATAL, set_pts_msg1);
    if ((gp1_sync(gp1_shmem, ioctlfd)) !=0)
        gp_send_message(-SYNC_ERROR, FATAL, set_pts_msg2);
    return(fp2);
}
/*
 *************************************************************************
 *      multiply_pts 
 *
 *	This routine will take the points returned from get_pts routine
 *	and multiply them with the matrix returned from get_matrix routine
 *	and compare them with the transformed points in GP2 context block.
 *************************************************************************
 */

multiply_pts(mat, points, fp2)
    float mat[4][4];
    float points[4 * MAXPTS];
    float *fp2;
{
    float *fp1, error;
    float tfp2[4];
    float fpoint[4];
    int i, err_cnt = 0;

    fp1 = points;
    fpoint[0] = fp1[0] * mat[0][0] + fp1[1] * mat[1][0] +
                fp1[2] * mat[2][0] + fp1[3] * mat[3][0];
    fpoint[1] = fp1[0] * mat[0][1] + fp1[1] * mat[1][1] +
                fp1[2] * mat[2][1] + fp1[3] * mat[3][1];
    fpoint[2] = fp1[0] * mat[0][2] + fp1[1] * mat[1][2] +
                fp1[2] * mat[2][2] + fp1[3] * mat[3][2];
    fpoint[3] = fp1[0] * mat[0][3] + fp1[1] * mat[1][3] +
                fp1[2] * mat[2][3] + fp1[3] * mat[3][3];
    bcopy((char *)fp2, (char *)tfp2, 4*sizeof(float));
    if (debug) {		
        (void) sprintf(msg, "Original:  %f %f %f %f\n",
                   fp1[0], fp1[1], fp1[2], fp1[3]);
	gp_send_message(0, DEBUG, msg);

        (void) sprintf(msg, "Result:    %f %f %f %f\n",
                   tfp2[0], tfp2[1], tfp2[2], tfp2[3]);
        gp_send_message(0, DEBUG, msg);

        (void) sprintf(msg, "Should be: %f %f %f %f\n",
                   fpoint[0], fpoint[1], fpoint[2], fpoint[3]);
	gp_send_message(0, DEBUG, msg);
    }
    for (i = 0; i < 4; i++) {
        if (tfp2[i] != fpoint[i]) {
            error = ((fpoint[i] - tfp2[i]) / fpoint[i]);
            if ((error > TOLERANCE) || (error < -TOLERANCE)) {
                err_cnt++;
                (void) sprintf(msg, multiply_pts_msg,
                    fpoint[i],tfp2[i],i,
                    *((int *)&fpoint[i]),*((int *)&tfp2[i]));
                gp_send_message(-DATA_ERROR, FATAL, msg);
            }
        }
    }
    return(err_cnt);
}

/*
 *************************************************************************
 * set view port points
 * write view port points to gp2
 **************************************************************************
 */
short *
set_view_port(fpoint0, fpoint1, shmptr, dest)
    float fpoint0[4];
    float fpoint1[4];
    short *shmptr;
    int dest;
{
    *shmptr++ = GP1_USE_CONTEXT | statblk;
    *shmptr++ = GP1_SET_MAT_NUM | dest;
    fpoint0[0] = 100;
    fpoint0[1] = 200;
    fpoint0[2] = 300;
    fpoint1[0] = 400;
    fpoint1[1] = 500;
    fpoint1[2] = 600;
    *shmptr++ = GP1_SET_VWP_3D;
    xscale = (fpoint1[0] - fpoint0[0]) / 2.0;
    xoffset = fpoint0[0] + xscale;
    yscale = (fpoint1[1] - fpoint0[1]) / 2.0;
    yoffset = fpoint0[1] + yscale;
    zscale = (fpoint1[2] - fpoint0[2]);
    zoffset = fpoint0[2];

    GP1_PUT_F(shmptr, xscale);
    GP1_PUT_F(shmptr, xoffset);
    GP1_PUT_F(shmptr, yscale);
    GP1_PUT_F(shmptr, yoffset);
    GP1_PUT_F(shmptr, zscale);
    GP1_PUT_F(shmptr, zoffset);
    return(shmptr);
}
/*
 **************************************************************************
 *	set_matrix
 * 
 *	This routine will the matrix sent to it and post the matix
 *	to the GP2 using the same context block.
 **************************************************************************
 */
short *
set_matrix(mat, shmptr, offset, dest)
    float mat[4][4];
    short *shmptr;
    int offset;
    int dest;
{
    int i;

    *shmptr++ = GP1_GET_MAT_3D | dest;
    *shmptr++ = -1;
    for (i = 0; i < 32; i++)
         *shmptr++ = -1;
    *shmptr++ = GP1_SET_MAT_3D | dest;
    GP1_PUT_F(shmptr, mat[0][0]);
    GP1_PUT_F(shmptr, mat[0][1]);
    GP1_PUT_F(shmptr, mat[0][2]);
    GP1_PUT_F(shmptr, mat[0][3]);
    GP1_PUT_F(shmptr, mat[1][0]);
    GP1_PUT_F(shmptr, mat[1][1]);
    GP1_PUT_F(shmptr, mat[1][2]);
    GP1_PUT_F(shmptr, mat[1][3]);
    GP1_PUT_F(shmptr, mat[2][0]);
    GP1_PUT_F(shmptr, mat[2][1]);
    GP1_PUT_F(shmptr, mat[2][2]);
    GP1_PUT_F(shmptr, mat[2][3]);
    GP1_PUT_F(shmptr, mat[3][0]);
    GP1_PUT_F(shmptr, mat[3][1]);
    GP1_PUT_F(shmptr, mat[3][2]);
    GP1_PUT_F(shmptr, mat[3][3]);

    *shmptr++ = GP1_GET_MAT_3D | dest;
    *shmptr++ = -1;
    for (i = 0; i < 32; i++)
       *shmptr++ = -1;
    *shmptr++ = GP1_EOCL;
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) !=0)
    	gp_send_message(-POST_ERROR, FATAL, set_matrix_msg1);
    if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
    	gp_send_message(-SYNC_ERROR, FATAL, set_matrix_msg2);
    return(shmptr);
}
/*
 *************************************************************************
 *	get_line
 *
 *	This routine will get the start and end points for the line
 *	to be clip checked.
 *************************************************************************
 */

get_line(k, points)
    int k;
    float points[8 * MAXLINES];
{
    float *fp_o_start, *fp_o_end;
    
    switch (k) {
          case 0 :              /* all in left side */
            fp_o_start = &points[0];
            fp_o_start[0] = 1;
            fp_o_start[1] = -1;
            fp_o_start[2] = 1;
            fp_o_end = &points[4];
            fp_o_end[0] = 1;
            fp_o_end[1] = 1;
            fp_o_end[2] = 1;
            break;
          case 1 :              /* all in top */
            fp_o_start = &points[0];
            fp_o_start[0] = -1;
            fp_o_start[1] = 1;
            fp_o_start[2] = 1;
            fp_o_end = &points[4];
            fp_o_end[0] = 1;
            fp_o_end[1] = 1;
            fp_o_end[2] = 1;
            break;
          case 2 :              /* all in right side */
            fp_o_start = &points[0];
            fp_o_start[0] = -1;
            fp_o_start[1] = 1;
            fp_o_start[2] = 1;
            fp_o_end = &points[4];
            fp_o_end[0] = -1;
            fp_o_end[1] = -1;
            fp_o_end[2] = 1;
            break;
          case 3 :              /* all in bottom */
            fp_o_start = &points[0];
            fp_o_start[0] = -1;
            fp_o_start[1] = -1;
            fp_o_start[2] = 1;
            fp_o_end = &points[4];
            fp_o_end[0] = 1;
            fp_o_end[1] = -1;
            fp_o_end[2] = 1;
            break;
          case 4 :              /* all points out left side */
            fp_o_start = &points[0];
            fp_o_start[0] = 1.005;
            fp_o_start[1] = -1.005;
            fp_o_start[2] = 1.005;
            fp_o_end = &points[4];
            fp_o_end[0] = 1.005;
            fp_o_end[1] = 1.005;
            fp_o_end[2] = 1.005;
            break;
          case 5 :              /* all points out top */
            fp_o_start = &points[0];
            fp_o_start[0] = 1.002;
            fp_o_start[1] = 1.002;
            fp_o_start[2] = 1.004;
            fp_o_end = &points[4];
            fp_o_end[0] = -1.008;
            fp_o_end[1] = 1.008;
            fp_o_end[2] = 1.008;
            break;
          case 6 :              /* all points out right side */
            fp_o_start = &points[0];
            fp_o_start[0] = -1.002;
            fp_o_start[1] = 1.002;
            fp_o_start[2] = 1.004;
            fp_o_end = &points[4];
            fp_o_end[0] = -1.008;
            fp_o_end[1] = -1.008;
            fp_o_end[2] = 1.008;
            break;
          case 7 :              /* all point out bottom */
            fp_o_start = &points[0];
            fp_o_start[0] = -1.002;
            fp_o_start[1] = -1.002;
            fp_o_start[2] = 1.002;
            fp_o_end = &points[4];
            fp_o_end[0] = 1.005;
            fp_o_end[1] = -1.005;
            fp_o_end[2] = 1.444;;
            break;
          case 8 :              /* start xout yin end in */
            fp_o_start = &points[0];
            fp_o_start[0] = 2;
            fp_o_start[1] = .001;
            fp_o_start[2] = .031;
            fp_o_end = &points[4];
            fp_o_end[0] = .9;
            fp_o_end[1] = .6;
            fp_o_end[2] = .003;
            break;
          case 9 :              /* xin yout, all end in */
            fp_o_start = &points[0];
            fp_o_start[0] = -1;
            fp_o_start[1] = 2;
            fp_o_start[2] = .031;
            fp_o_end = &points[4];
            fp_o_end[0] = .9;
            fp_o_end[1] = .6;
            fp_o_end[2] = .003;
            break;
          case 10 :             /* zout all else in */
            fp_o_start = &points[0];
            fp_o_start[0] = -.2;
            fp_o_start[1] = -.001;
            fp_o_start[2] = 2.031;
            fp_o_end = &points[4];
            fp_o_end[0] = .9;
            fp_o_end[1] = .6;
            fp_o_end[2] = .003;
            break;
          case 11 :              /* all start in, end xout yin zin */
            fp_o_start = &points[0];
            fp_o_start[0] = -.5;
            fp_o_start[1] = -.001;
            fp_o_start[2] = .031;
            fp_o_end = &points[4];
            fp_o_end[0] = -1.9;
            fp_o_end[1] = .6;
            fp_o_end[2] = .003;
            break;
          case 12 :              /* all start in, end xin yout zin */
            fp_o_start = &points[0];
            fp_o_start[0] = 0.0;
            fp_o_start[1] = -1.0;
            fp_o_start[2] = 0.0;
            fp_o_end = &points[4];
            fp_o_end[0] = 0.00002;
            fp_o_end[1] = 1.6;
            fp_o_end[2] = .003;
            break;
          case 13 :             /*all start in, end xin yin zout */
            fp_o_start = &points[0];
            fp_o_start[0] = -.2;
            fp_o_start[1] = -.001;
            fp_o_start[2] = 0.031;
            fp_o_end = &points[4];
            fp_o_end[0] = .9;
            fp_o_end[1] = .6;
            fp_o_end[2] = -2.003;
            break;
          case 14 :             /*all start in, end xin yout zout */
            fp_o_start = &points[0];
            fp_o_start[0] = -.2;
            fp_o_start[1] = -.001;
            fp_o_start[2] = 0.031;
            fp_o_end = &points[4];
            fp_o_end[0] = .1;
            fp_o_end[1] = -2.6;
            fp_o_end[2] = 1.003;
            break;
        }
}
/*
 *************************************************************************
 *	set_line
 *
 *	This routine will post the line point to the GP2 using the same
 *	context with the above matrix and view port.
 *************************************************************************
 */

set_line(shmptr, offset, points, ip_r_start, ip_r_end)
    short *shmptr;
    int offset;
    float points[8 * MAXLINES];
    int **ip_r_start;
    int **ip_r_end;
{
    float *fp_o_start, *fp_o_end;

    *shmptr++ = GP1_USE_CONTEXT | statblk;
    *shmptr++ = GP1_PROC_LINE_FLT_3D;
    *shmptr++ = 1;                 /* count */

    *shmptr++ = 0;                 /* clip flag */
    *ip_r_start = (int *) shmptr;
    fp_o_start = &points[0];
    GP1_PUT_F(shmptr, *fp_o_start);
    GP1_PUT_F(shmptr, *(fp_o_start + 1));
    GP1_PUT_F(shmptr, *(fp_o_start + 2));
    *ip_r_end = (int *) shmptr;
    fp_o_end = &points[4];
    GP1_PUT_F(shmptr, *fp_o_end);
    GP1_PUT_F(shmptr, *(fp_o_end + 1));
    GP1_PUT_F(shmptr, *(fp_o_end + 2));
    *shmptr++ = -1;                /* Done Flag */

    *shmptr++ = GP1_EOCL;
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) !=0)
        gp_send_message(-POST_ERROR, FATAL, set_line_msg1);
    if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
        gp_send_message(-SYNC_ERROR, FATAL, set_line_msg2);
    if (*(shmptr-2) != 0)
        gp_send_message(-2, ERROR, set_line_msg3);
}
/*
 *************************************************************************
 *      multiply_line 
 *
 *	This routine takes copy of matrix posted to the GP2 and a copy of
 *	the points posted to the GP2 and multiplies them together.
 *	These are expected points. The GP2 should generate the same ones.
 *************************************************************************
 */

multiply_line(mat, fpoint0, fpoint1, points, ip_r_start, ip_r_end)
    float mat[4][4];
    float fpoint0[4];
    float fpoint1[4];
    float points[4];
    int *ip_r_start, *ip_r_end;
{
    float *fp_o_start, *fp_o_end;

    fp_o_start = &points[0];
    fp_o_end = &points[4];
    fpoint0[0] = fp_o_start[0] * mat[0][0] +
                 fp_o_start[1] * mat[1][0] +
                 fp_o_start[2] * mat[2][0] + mat[3][0];
    fpoint0[1] = fp_o_start[0] * mat[0][1] +
                 fp_o_start[1] * mat[1][1] +
                 fp_o_start[2] * mat[2][1] + mat[3][1];
    fpoint0[2] = fp_o_start[0] * mat[0][2] +
                 fp_o_start[1] * mat[1][2] +
                 fp_o_start[2] * mat[2][2] + mat[3][2];
    fpoint0[3] = fp_o_start[0] * mat[0][3] +
                 fp_o_start[1] * mat[1][3] +
                 fp_o_start[2] * mat[2][3] + mat[3][3];

    fpoint1[0] = fp_o_end[0] * mat[0][0] +
                 fp_o_end[1] * mat[1][0] +
                 fp_o_end[2] * mat[2][0] + mat[3][0];
    fpoint1[1] = fp_o_end[0] * mat[0][1] +
                 fp_o_end[1] * mat[1][1] +
                 fp_o_end[2] * mat[2][1] + mat[3][1];
    fpoint1[2] = fp_o_end[0] * mat[0][2] +
                 fp_o_end[1] * mat[1][2] +
                 fp_o_end[2] * mat[2][2] + mat[3][2];
    fpoint1[3] = fp_o_end[0] * mat[0][3] +
                 fp_o_end[1] * mat[1][3] +
                 fp_o_end[2] * mat[2][3] + mat[3][3];

    fpoint0[0] = (fpoint0[0] / fpoint0[3]) * xscale + xoffset;
    fpoint0[1] = (fpoint0[1] / fpoint0[3]) * yscale + yoffset;
    fpoint0[2] = (fpoint0[2] / fpoint0[3]) * zscale + zoffset;
    fpoint1[0] = (fpoint1[0] / fpoint1[3]) * xscale + xoffset;
    fpoint1[1] = (fpoint1[1] / fpoint1[3]) * yscale + yoffset;
    fpoint1[2] = (fpoint1[2] / fpoint1[3]) * zscale + zoffset;

    if (debug) {	/* remove verbose */
        (void) sprintf(msg, "Original:  %f %f %f\n",
                   fp_o_start[0], fp_o_start[1], fp_o_start[2]);
	gp_send_message(0, DEBUG, msg);

        (void) sprintf(msg, "           %f %f %f\n",
                   fp_o_end[0], fp_o_end[1], fp_o_end[2]);
        gp_send_message(0, DEBUG, msg);

        (void) sprintf(msg, "Should be: %f %f %f\n",
                   fpoint0[0], fpoint0[1], fpoint0[2]);
        gp_send_message(0, DEBUG, msg);

        (void) sprintf(msg, "           %f %f %f\n",
                   fpoint1[0], fpoint1[1], fpoint1[2]);
        gp_send_message(0, DEBUG, msg);

        (void) sprintf(msg, "Result:    %d %d %d\n",
                   ip_r_start[0], ip_r_start[1], ip_r_start[2]);
        gp_send_message(0, DEBUG, msg);

        (void) sprintf(msg, "           %d %d %d\n",
                   ip_r_end[0], ip_r_end[1], ip_r_end[2]);
        gp_send_message(0, DEBUG, msg);
    }
}
/*
 *************************************************************************
 *	check_line
 *
 *	Check to see if the clip flag was calculated correctly.
 *	Call the routine to check if the points were calculated correctly.
 *	then if an error was detected the test is exited.
 *************************************************************************
 */

check_line(k, fpoint0, fpoint1, ip_r_start, ip_r_end)
    int k;
    float fpoint0[4];
    float fpoint1[4];
    int *ip_r_start, *ip_r_end;
{
    short *shmptr;

    shmptr = (short *) ip_r_start;
    switch(k) {
        case 0 :            /* all points are in */
        case 1 :
        case 2 :
        case 3 :
            if ((*(shmptr - 1) & 0xFFFF) != 0 ) {
                errors++;
		(void) sprintf(msg, check_line_msg1, *(shmptr - 1) & 0xFFFF);
            	gp_send_message(-DATA_ERROR, FATAL, msg);
            }  
            cmp_clip(start_string,ip_r_start,fpoint0,k);
            cmp_clip(end_string,ip_r_end,fpoint1,k);
            break;
        case 4 :            /* all points are out */
        case 5 :
        case 6 :
        case 7 :
            if ((*(shmptr - 1) & 0xFFFF) != 0x8000 ) {
                errors++;
		(void) sprintf(msg, check_line_msg2, *(shmptr - 1) & 0xFFFF);
            	gp_send_message(-DATA_ERROR, FATAL, msg);
            }  
            cmp_clip("Proc Line Start",ip_r_start,fpoint0,k);
            cmp_clip("Proc Line End",ip_r_end,fpoint1,k);
            break;
        case 8 :            /* some start point out */
        case 9 :            /* all end point in */
        case 10 :
            if ((*(shmptr - 1) & 0xFFFF) != 1 ) {
                errors++;
		(void) sprintf(msg, check_line_msg3, *(shmptr - 1) & 0xFFFF);
            	gp_send_message(-DATA_ERROR, FATAL, msg);
            }
            cmp_clip(end_string,ip_r_end,fpoint1,k);
            break;
	case 11 :           /* start points in end points out */
        case 12 :
        case 13 :            
	case 14 :
            if ((*(shmptr - 1) & 0xFFFF) != 2 ) {
                errors++;
		(void) sprintf(msg, check_line_msg4, *(shmptr - 1) & 0xFFFF);
            	gp_send_message(-DATA_ERROR, FATAL, msg);
            }
            cmp_clip(start_string,ip_r_start,fpoint0,k);
            break;
	default :
	    (void) sprintf(msg, check_line_msg5, *(shmptr - 1) & 0xFFFF);
            gp_send_message(-DATA_ERROR, FATAL, msg);
            break;
        }
}
/*
 *************************************************************************
 *	cmp_clip
 * 
 *	Compare the GP2 points with the Host generated points (multiply_line)
 *	If the points are not with in tolarence of closness for the
 *	floating point numbers then post an error message and exit.
 *************************************************************************
 */
cmp_clip(string, ip_r_start, fpoint0, k)
    char *string;
    int *ip_r_start,k;
    float *fpoint0;
{
    int i;
    float error;

    for(i = 0; i < 3; i++) {
        if (ip_r_start[i] != fpoint0[i]) {
            error = ((fpoint0[i] - ip_r_start[i]) / fpoint0[i]);
            if ((error > TOLERANCE1) || (error < -TOLERANCE1)) {
                errors++;
		(void) sprintf(msg, cmp_clip_msg,
		    string,fpoint0[i],ip_r_start[i],i,k);
                gp_send_message(-DATA_ERROR, FATAL, msg);
            }
       }  
   }
}
