#ifndef lint
static	char sccsid[] = "@(#)gp2_matrix.c 1.1 92/07/30 Copyright Sun Micro";
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
#include "gp2_matrix.h"
#include "sdrtns.h"

extern char *sprintf();

extern short *gp1_shmem;
extern int ioctlfd;
extern short statblk;
double depth=9;
double eye=4.0;
/*
 *************************************************************************
 *      mat_ident 
 *
 *	Initialize the Idenity map to be used through out the Test.
 *************************************************************************
 */

mat_ident(mat)
    float mat[4][4];
{
    mat[0][0] = 1.0;
    mat[0][1] = 0.0;
    mat[0][2] = 0.0;
    mat[0][3] = 0.0;
    mat[1][0] = 0.0;
    mat[1][1] = 1.0;
    mat[1][2] = 0.0;
    mat[1][3] = 0.0;
    mat[2][0] = 0.0;
    mat[2][1] = 0.0;
    mat[2][2] = 1.0;
    mat[2][3] = 0.0;
    mat[3][0] = 0.0;
    mat[3][1] = 0.0;
    mat[3][2] = 0.0;
    mat[3][3] = 1.0;
}
/*
 ************************************************************************* 
 *      test_translation 
 *
 *	Move the matrix to be used in the test to the mat structure
 *	so it can be posted to the GP2.
 *	Then call the post routine.
 ************************************************************************* 
 */ 
test_translation(mat, shmptr, offset, dest)
    float mat[4][4];
    short *shmptr;
    int offset;
    int dest;
{
    int i, j;

    if ((!exec_by_sundiag) || (debug))
        (void) printf("\tTesting Translation for x, y, z: Matrix 1\n");
    for(i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            mat[i][j] = mat_tst1[i][j];
        }  
    }  
    /* set the matrix in to sram */
    set_mat_in_ram(mat, shmptr, offset, dest);

    if ((!exec_by_sundiag) || (debug))
        (void) printf("\tTesting Translation for x, y, z: Matrix 2\n");
    for(i = 0; i < 4; i++) {
        for(j = 0; j < 4; j++) {
            mat[i][j] = mat_tst2[i][j];
        }  
    }
}
/*
 *************************************************************************
 *	test_rotate
 *
 *	set rotate angle, replace GP2 matix the test matrix.
 *************************************************************************
 */

test_rotate(axis, mat)
    char axis;
    float mat[4][4];
{
    double angle;
    int i, j;

    if ((!exec_by_sundiag) || (debug)) {
        (void) printf("\n\tTesting Rotate about (%c) axis:\n", axis);
        (void) printf("\tangle in degrees: 90\n");
    }
    angle=90;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4;j ++) {
            mat[i][j] = mat_tst1[i][j];
        }
    }
    rotmat(axis, angle, mat);
}
/*
 *************************************************************************
 *	rotmat
 *
 *	Take the angle and rotate the matrix around the axis sent to 
 *	the routine, and build a new matrix to be posted to the GP2.
 *************************************************************************
 */

rotmat(axis, angle, mat)
    char axis;
    double angle;
    float mat[4][4];
{
    double c, s;

    if (isupper(axis))
        axis = tolower(axis);
    angle *= M_PI / 180.0;
    c = cos(angle);
    s = sin(angle);
    if (axis == 'x') {
        mat[1][1] = c;
        mat[2][2] = c;
        mat[1][2] = s;
        mat[2][1] = -s;
    } else if (axis == 'y') {
        mat[0][0] = c;
        mat[2][2] = c;
        mat[0][2] = s;
        mat[2][0] = -s;
    } else if (axis == 'z') {
        mat[0][0] = c;
        mat[1][1] = c;
        mat[0][1] = -s;
        mat[1][0] = s;
    } else {   
	(void) sprintf(msg, rotmat_msg, axis);
        gp_send_message(-DATA_ERROR, FATAL, msg);
    }
}
/*
 *************************************************************************
 *	test_scale
 *
 *	move the test_scale matrix to the temp matrix then post the 
 *	new matrix to the GP2.
 *************************************************************************
 */

test_scale(mat)
    float mat[4][4];
{
    int i, j;

    if ((!exec_by_sundiag) || (debug))
        (void) printf("\n\tTesting Scale factor x,  y,  z:\n");
    for (i = 0; i < 4; i++) {
        for(j = 0; j < 4; j++) {
            mat[i][j] = mat_tst1[i][j];
        }
    }
    mat[0][0] = 1.1;
    mat[1][1] = 1.1;
    mat[2][2] = 1.4;
}
/*
 *************************************************************************
 *      test_eye
 *
 *      move the test_eye matrix to the temp matrix then post the
 *      new matrix to the GP2.
 *************************************************************************
 */

test_eye(mat)
    float mat[4][4];
{
    int i, j;

    if ((!exec_by_sundiag) || (debug))
        (void) printf("\n\tTesting Eye distance, depth:\n");
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4;j ++) {
            mat[i][j] = mat_tst1[i][j];
        }
    }
    mat[0][0] = 1.1;
    mat[1][1] = 1.1;
    mat[2][2] = (1. / eye) + (1. / depth);
    mat[2][3] = 1. / eye;
}
/*
 *************************************************************************
 *      set_mat_in_ram 
 *
 *	1). use same context block.
 *	2). set GP1_GET_MAT command, and move shared ram ptr to new matrix.
 *	3). post new matrix.
 *************************************************************************
 */

set_mat_in_ram(mat, shmptr, offset, dest)
    float mat[4][4];
    short *shmptr;
    int offset;
    int dest;
{
    int i;

    *shmptr++ = GP1_USE_CONTEXT | statblk;
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
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
        gp_send_message(-POST_ERROR, FATAL, set_mat_msg1);
    if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
        gp_send_message(-SYNC_ERROR, FATAL, set_mat_msg1); 
}
/*
 *************************************************************************
 *	multiply_matrices
 *
 *	multiply the pre matrix with the post matrix, The GP2 will store
 *	the results in the destination matrix.
 *************************************************************************
 */

multiply_matrices(mat, shmptr, offset)
    float mat[4][4];
    short *shmptr;
    int offset;
{
    int dest, pre, post, i;
    float *ptr_pre;
    float *ptr_post;
    float *ptr_dest_after;

    pre = 0;
    post= 1;
    dest = 2;
    *shmptr++ = GP1_USE_CONTEXT | statblk;
    *shmptr++ = GP1_GET_MAT_3D | pre;
    *shmptr++ = -1;
    ptr_pre = (float *) shmptr;
    for (i = 0; i < 32; i++)
        *shmptr++ = -1;
    *shmptr++ = GP1_GET_MAT_3D | post;
    *shmptr++ = -1;
    ptr_post = (float *) shmptr;
    for (i = 0; i < 32; i++)
        *shmptr++ = -1;
    *shmptr++ = GP1_GET_MAT_3D | dest;
    *shmptr++ = -1;
    for (i = 0; i < 32; i++)
        *shmptr++ = -1;
    *shmptr++ = GP1_MUL_MAT_3D;
    *shmptr++ = pre;
    *shmptr++ = post;
    *shmptr++ = dest;
    *shmptr++ = GP1_GET_MAT_3D | dest;
    *shmptr++ = -1;
    ptr_dest_after = (float *) shmptr;
    for (i = 0; i < 32; i++)
        *shmptr++ = -1;
    *shmptr++ = GP1_EOCL;
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
        gp_send_message(-POST_ERROR, FATAL, set_mat_msg1);
    if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
        gp_send_message(-SYNC_ERROR, FATAL, set_mat_msg2);
    mat_mat_mul(ptr_pre, ptr_post, mat);
    if (verbose) {
        dump_matrix("pre", ptr_pre);
        dump_matrix("post", ptr_post);
        dump_matrix("Should be", (float *)mat);
        dump_matrix("Result", ptr_dest_after);
    }
    chk_matrix(ptr_dest_after, mat);
}
/*
 *************************************************************************
 *	mat_mat_mul
 *
 *	Here the Host calculates the results of the Matrices multiply.
 *************************************************************************
 */

mat_mat_mul(mat1, mat2, mat3)
    float *mat1;
    float *mat2;
    float mat3[4][4];
{
    int i, j;
    float tmat[4][4];
    float tmat1[4][4];
    float tmat2[4][4];

    bcopy((char *)mat1, (char *)tmat1, 16*sizeof(float));
    bcopy((char *)mat2, (char *)tmat2, 16*sizeof(float));
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            tmat[i][j] = tmat1[i][0] * tmat2[0][j] +
                         tmat1[i][1] * tmat2[1][j] +
                         tmat1[i][2] * tmat2[2][j] +
                         tmat1[i][3] * tmat2[3][j];
        }
    }    
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            mat3[i][j] = tmat[i][j];
        }
    }    
}
/*
 *************************************************************************
 *	dump_matrix
 *
 *	display the matrix.
 *************************************************************************
 */

dump_matrix(string, mat)
    char *string;
    float *mat;
{
    float tmat[4][4];
 
    bcopy((char *)mat, (char *)tmat, 16*sizeof(float));
    (void) sprintf(msg, "%s:\n", string);          
    gp_send_message(0, CONSOLE, msg);

    (void) sprintf(msg, "\t%f\t%f\t%f\t%f\n",
           tmat[0][0], tmat[0][1], tmat[0][2], tmat[0][3]);
    gp_send_message(0, CONSOLE, msg);

    (void) sprintf(msg, "\t%f\t%f\t%f\t%f\n",
           tmat[1][0], tmat[1][1], tmat[1][2], tmat[1][3]);
    gp_send_message(0, CONSOLE, msg);

    (void) sprintf(msg, "\t%f\t%f\t%f\t%f\n",
           tmat[2][0], tmat[2][1], tmat[2][2], tmat[2][3]);
    gp_send_message(0, CONSOLE, msg);

    (void) sprintf(msg, "\t%f\t%f\t%f\t%f\n",
           tmat[3][0], tmat[3][1], tmat[3][2], tmat[3][3]);
    gp_send_message(0, CONSOLE, msg);
}
/*
 *************************************************************************
 *	chk_matrix
 *
 *	Check the GP2 Matrix against the Matrix calculated by host.
 *	IF the results are with in tolerance of floating point numbers
 *	then return. If not report and error and exit program. 
 *************************************************************************
 */

chk_matrix(mat1, mat2)
    float *mat1;
    float mat2[4][4];
{
    int i,j;
    float error;
    float tmat1[4][4];
    float tmat2[4][4];

    bcopy((char *)mat1, (char *)tmat1, 16*sizeof(float));
    bcopy((char *)mat2, (char *)tmat2, 16*sizeof(float));
    for(i = 0; i < 4; i++) {
        for(j = 0; j < 4; j++) {
            if(tmat1[i][j] != tmat2[i][j]) {
          	error =(( tmat2[i][j]-tmat1[i][j] ) / tmat2[i][j]);
          	if((error > TOLERANCE) || (error < -TOLERANCE)) {
             	    errors++;
		    (void) sprintf(msg, chk_matrix_msg,
                	tmat2[i][j],tmat1[i][j],i,j,
                	*((int *)&tmat2[i][j]),*((int *)&tmat1[i][j]),error);
             	    gp_send_message(-DATA_ERROR, FATAL, msg);
          	}
            }
      	}  
    }  
}
