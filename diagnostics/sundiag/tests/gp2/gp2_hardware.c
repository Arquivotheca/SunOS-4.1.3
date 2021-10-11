#ifndef lint
static	char sccsid[] = "@(#)gp2_hardware.c 1.1 92/07/30 Copyright Sun Micro";
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
#include "gp2_hardware.h"
#include "sdrtns.h"

extern char *sprintf();

extern short *gp1_shmem;
extern int ioctlfd;

/*
 *************************************************************************
 *      Test XP Local ram
 *
 *	Test xp local ram, test all but 4 of the 64k of ram. The stack 
 *	lives in the upper portions of local ram. This is done by sending
 *	commands to the GP2 runing GPCI micro code to test local ram.
 *	If an error is detected it will be reported.
 *************************************************************************
 */
test_xp_local_ram(shmptr, offset)
    short *shmptr;
    int offset;
{
    int i, *error, err_cnt = 0;
    u_long addr, size;
    u_long data, data1;

    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\tTesting XP Local Ram\n");
    addr = 0x0;
    size = 0xf000;
    data = 0x00000001;
    for (i = 0; i < 32; i++) {
        error = (int *)post_mem_test(XPMEMTST,addr,size,data,shmptr,offset);
        data1 = *(error+4);
        if (data1 != 0 )
            gp_send_message(-DATA_ERROR, FATAL, xp_local_ram_msg1);
        if (*error != 0) {
            err_cnt++;
	    (void) sprintf(msg, xp_local_ram_msg2, *(error+1));
            gp_send_message(0, ERROR, msg);
	    (void) sprintf(msg, data_err_msg1, *(error+2),*(error+3),*error);
            gp_send_message(-DATA_ERROR, FATAL, msg);
        }
        data = data << 1;
    }
    return(err_cnt);
}

/*
 **********************************************************************
 *      Test XP Shared ram
 **********************************************************************
 */
test_xp_shared_ram(shmptr, offset)
    short *shmptr;
    int offset;
{
    int i, *error, err_cnt = 0;
    u_long addr, size;
    u_long data, data1;

    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\tTesting XP Shared Ram\n");
    addr = 0x70000;
    size = 0x0fb00;
    data = 1;
    for (i = 0; i < 32; i++) {
        error = (int *)post_mem_test(XPMEMTST,addr,size,data,shmptr,offset);
        data1 = *(error+4);
        if (data1 != 0 )
            gp_send_message(-DATA_ERROR, FATAL, xp_shared_ram_msg1);
        data1 = *error;
        if (data1 != 0) {
            err_cnt++;
	    (void) sprintf(msg, xp_shared_ram_msg2, *(error+1));
            gp_send_message(0, ERROR, msg);
	    (void) sprintf(msg, data_err_msg1, *(error+2),*(error+3),*error);
            gp_send_message(-DATA_ERROR, FATAL, msg);
        }
        data = data << 1;
    }
    return(err_cnt);
}

/*
 *********************************************************************
 *      Test XP sequencer
 *
 *	Test the XP sequencer to determine if it processes the condition
 *	codes correctly.
 *********************************************************************
 */

test_xp_sequencer(shmptr, offset)
    short *shmptr;
    int offset;
{
    int i, *error, err_cnt = 0;
    u_long data1;

    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\tTesting XP Sequencer\n");
    for (i = 0; i < 12; i++) {
        error = (int *)post_alu_test(XPSEQ,seq_tst[i][0],seq_tst[i][1],
            seq_tst[i][2],shmptr,offset);
        data1 = *(error+4);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, xp_sequencer_msg1);
        if (*error != seq_tst[i][3]) {
            err_cnt++;
	    (void) sprintf(msg, xp_sequencer_msg2, seq_tst[i][3], *error, i);
            gp_send_message(-DATA_ERROR, FATAL, msg);
        }
    }
    return(err_cnt);
}

/*
 *********************************************************************
 *      Test XP Alu
 *
 *	Simple ALU test to determine if the basic arithmetic functions
 *	work.
 *********************************************************************
 */
test_xp_alu(shmptr, offset)
    short *shmptr;
    int offset;
{
    int i, *error, err_cnt = 0;
    u_long data1;

    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\tTesting XP Alu\n");
    for (i = 0; i < 8; i++) {
        error = (int *)post_alu_test(XPALUTST,alu_tst[i][0],
                                alu_tst[i][1],alu_tst[i][2],shmptr, offset);
        data1 = *(error+4);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, xp_alu_msg1);
        if (*error != alu_tst[i][3]) {
            err_cnt++;
	    (void) sprintf(msg, xp_alu_msg2, alu_tst[i][3],*error);
            gp_send_message(-DATA_ERROR, FATAL, msg);
        }
    }
    return(err_cnt);
}
 
/*
 *********************************************************************
 *      XP -> RP fifo -> XP read back reg test
 *
 *	XP sends data to RP fifo. 
 *	RP read fifo data and sends to XP readback register
 *	XP reads readback register and checks data.
 *	Reports error if any.
 *********************************************************************
 */
test_xp_rp_fifo(shmptr, offset)
    short *shmptr;
    int offset;
{
    int i, *error, err_cnt = 0;
    u_long data, data1;

    data = 0x55555555;
    if ((!exec_by_sundiag) || (debug))
    	gp_send_message(0, DEBUG, "\tTesting XP -> RP Fifo\n");
    for (i = 0; i < 2; i++) {
        error = (int *)post_fifo(XPFIFO,data,shmptr,offset);
        data1 = *(error+5);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, xp_rp_fifo_msg1);
        if (*error != 0) {
            err_cnt++;
	    (void) sprintf(msg, xp_rp_fifo_msg2, *error);
            gp_send_message(0, ERROR, xp_rp_fifo_msg2);
	    (void) sprintf(msg, data_err_msg2, data,*(error-1));
            gp_send_message(-DATA_ERROR, FATAL, msg);
        }
        data = ~data;
    }
    return(err_cnt);
}

/*        
 ***********************************************************************
 *      RP Local Ram Test
 **********************************************************************
 */
test_rp_local_ram(shmptr, offset)
    short *shmptr;
    int offset;
{
    int i, *error, err_cnt = 0;
    u_long addr, size;
    u_long data, data1;

    if ((!exec_by_sundiag) || (debug))
    	gp_send_message(0, DEBUG, "\tTesting RP Local Ram\n");
    addr = 0x0;
    size = 0xf000;
    data = 1;
    for (i = 0; i < 32; i++) {
        error = (int *)post_mem_test(RPMEMTST,addr,size,data,shmptr,offset);
        data1 = *(error+4);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, rp_local_ram_msg1);
        data1 = *(error+5);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, rp_local_ram_msg2);
        if (*error != 0) {
            err_cnt++;
	    (void) sprintf(msg, rp_local_ram_msg3, *(error+1));
            gp_send_message(0, ERROR, msg);
	    (void) sprintf(msg, data_err_msg1, *(error+2),*(error+3),*error);
            gp_send_message(-DATA_ERROR, FATAL, msg);
        }
        data = data << 1;
    }
    return(err_cnt);
}

/*
 ***********************************************************************
 *      Test RP Shared ram
 ***********************************************************************
 */
test_rp_shared_ram(shmptr, offset)
    short *shmptr;
    int offset;
{
    int i, *error, err_cnt = 0;
    u_long addr, size;
    u_long data, data1;

    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\tTesting RP Shared Ram\n");
    addr = 0x70000;
    size = 0xfbc0;
    data = 1;
    for (i = 0; i < 32; i++) {
	error = (int *)post_mem_test(RPMEMTST,addr,size,data,shmptr,offset);
       	data1 = *(error+4);
       	if (data1 != 0 )
            gp_send_message(-DATA_ERROR, FATAL, rp_shared_ram_msg1);
        data1 = *(error+5);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, rp_shared_ram_msg2);
        data1 = *error;
        if (*error != 0) {
            err_cnt++;
	    (void) sprintf(msg, rp_shared_ram_msg3, *(error+1));
            gp_send_message(0, ERROR, msg);
	    (void) sprintf(msg, data_err_msg1, *(error+2),*(error+3),*error);
            gp_send_message(-DATA_ERROR, FATAL, msg);
        }
        data = data << 1;
    }
    return(err_cnt);
}

/*
 *********************************************************************
 *      Test RP sequencer
 *
 *	Test RP sequencer to determine if the condition codes and branch
 *	work.
 *********************************************************************
 */
test_rp_sequencer(shmptr, offset)
    short *shmptr;
    int offset;
{
    int i, *error, err_cnt = 0;
    u_long data1;

    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\tTesting RP Sequencer\n");
    for (i = 0; i < 12; i++) {
        error = (int *)post_alu_test(RPSEQ,seq_tst[i][0],seq_tst[i][1],
                                seq_tst[i][2],shmptr,offset);
        data1 = *(error+4);
        if (data1 != 0 )
            gp_send_message(-DATA_ERROR, FATAL, rp_sequencer_msg1);
        data1 = *(error+5);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, rp_sequencer_msg2);
        if (*error != seq_tst[i][3]) {
            err_cnt++;
	    (void) sprintf(msg, rp_sequencer_msg3, 
                        seq_tst[i][0],seq_tst[i][3],*error);
            gp_send_message(-DATA_ERROR, FATAL, msg);
        }
    }
    return(err_cnt);
}

/*
 *********************************************************************
 *      Test RP Alu
 *
 *	Test bacic alu functions to determine if arithmetic functions
 *	work.
 *********************************************************************
 */
test_rp_alu(shmptr, offset)
    short *shmptr;
    int offset;
{
    int i, *error, err_cnt = 0;
    u_long data1;

    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\tTesting RP Alu\n");
    for (i = 0; i < 8; i++) {
        error = (int *)post_alu_test(RPALUTST,alu_tst[i][0],
                                alu_tst[i][1],alu_tst[i][2],shmptr,offset);
        data1 = *(error+4);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, rp_alu_msg1);
        data1 = *(error+5);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, rp_alu_msg2);
        if (*error != alu_tst[i][3]) {
            err_cnt++;
	    (void) sprintf(msg, rp_alu_msg3, alu_tst[i][3],*error);
            gp_send_message(-DATA_ERROR, FATAL, msg); 
        }
    }
    return(err_cnt);
}
/*
 *********************************************************************
 *      RP -> PP fifo -> RP read back reg test
 *
 *	RP sends data to PP fifo.
 *	PP read fifo.
 *	PP writes RP read back register.
 *	RP reads PP Read back register and checks data.
 *********************************************************************
 */

test_rp_pp_fifo(shmptr, offset)
    short *shmptr;
    int offset;
{
    int i, *error, err_cnt = 0;
    u_long data, data1;

    data = 0x55555555;
    for (i = 0; i < 2; i++) {
    	if ((!exec_by_sundiag) || (debug))
            gp_send_message(0, DEBUG, "\tTesting RP -> PP Fifo\n");
        error = (int *)post_fifo(RPFIFO,data,shmptr,offset);
        data1 = *(error+5);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, rp_pp_fifo_msg1);
        data1 = *(error+6);
        if (data1 != 0 )
            gp_send_message(-DATA_ERROR, FATAL, rp_pp_fifo_msg2);
        if (*error != 0) {
            err_cnt++;
            (void) sprintf(msg, rp_pp_fifo_msg3, *error);
            gp_send_message(0, ERROR, msg);
	    (void) sprintf(msg, data_err_msg2, data,*(error-1));
            gp_send_message(-DATA_ERROR, FATAL, data_err_msg2);
        }
    	data = ~data;
    }
    return(err_cnt);
}

/*
 **********************************************************************
 *      Test PP LDX AGO
 **********************************************************************
 */
test_pp_ldx_ago(shmptr, offset)
    short *shmptr;
    int offset;
{
    int *error, err_cnt = 0;
    u_long data, data1;

    data = 0x00067788;
    data = 1;
    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\tTesting PP LDX AGO\n");
    (void) post_ppld_reg(PP_LD_REG,(u_long)4,(u_long)4,shmptr,offset);
    error = (int *)post_pp_ago(PPLDXAGO,data,shmptr,offset);
    data1 = *(error+6);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_ldx_ago_msg1);
    data1 = *(error+7);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_ldx_ago_msg2);
    data1 = *(error+8);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_ldx_ago_msg3); 
/*
 *	srm	the folowing addition of the & with 0xfffff is because
 *	srm	the ago is only 20 bits wide and when read will include
 *	srm	two bits from the amd which are not predictable.
 */
    if ( (*error & 0xfffff) != data) {	/*	srm	*/
        err_cnt++;
	(void) sprintf(msg, pp_ldx_ago_msg4, data,*error);
        gp_send_message(-DATA_ERROR, FATAL, msg);
    }
    return(err_cnt);
}

/*
 **********************************************************************
 *      Test PP ADY AGO  ldx + (ady * 1152)
 **********************************************************************
 */
test_pp_ady_ago(shmptr, offset)
    short *shmptr;
    int offset;
{
    int *error, err_cnt = 0;
    u_long data, data1;

    data = 0x00034455;
    data = 0;
    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\tTesting PP ADY AGO\n");
    (void) post_ppld_reg(PP_LD_REG,(u_long)4,(u_long)4,shmptr,offset);
    error = (int *)post_pp_ago(PPADYAGO,data,shmptr,offset);
    data1 = *(error+6);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_ady_ago_msg1);
    data1 = *(error+7);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_ady_ago_msg2);
    data1 = *(error+8);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_ady_ago_msg3); 
/*
 *	srm	the folowing addition of the & with 0xfffff is because
 *	srm	the ago is only 20 bits wide and when read will include
 *	srm	two bits from the amd which are not predictable.
 */
    data = 1;
    if ( (*error & 0xfffff) != data) {	/*	srm	*/
        err_cnt++;
	(void) sprintf(msg, pp_ady_ago_msg4, data,*error);
        gp_send_message(-DATA_ERROR, FATAL, msg);
    }
    return(err_cnt);
}

/*
 **********************************************************************
 *      Test PP ADX AGO  above ago + adx
 **********************************************************************
 */
test_pp_adx_ago(shmptr, offset)
    short *shmptr;
    int offset;
{
    int *error, err_cnt = 0;
    u_long data, data1;

    data = 0x000609f8;
    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\tTesting PP ADX AGO\n");
    (void) post_ppld_reg(PP_LD_REG,(u_long)4,(u_long)4,shmptr,offset);
    error = (int *)post_pp_ago(PPADXAGO,data,shmptr,offset);
    data1 = *(error+6);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_adx_ago_msg1);
    data1 = *(error+7);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_adx_ago_msg2); 
    data1 = *(error+8);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_adx_ago_msg3);
/*
 *	srm	the folowing addition of the & with 0xfffff is because
 *	srm	the ago is only 20 bits wide and when read will include
 *	srm	two bits from the amd which are not predictable.
 */
    data = 0x609f9;
    if ( (*error & 0xfffff) != data) {	/*	srm	*/
        err_cnt++;
	(void) sprintf(msg, pp_adx_ago_msg4, data,*error);
        gp_send_message(-DATA_ERROR, FATAL, msg);
    }
    return(err_cnt);
}

/*
 **********************************************************************
 *      Test PP Sequencer Test
 **********************************************************************
 */
test_pp_sequencer(shmptr, offset)
    short *shmptr;
    int offset;
{
    int i, *error, err_cnt = 0;
    u_long data1;

    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\tTesting PP Sequencer\n");
    for (i = 0; i < 12; i++) {
        error = (int *)post_alu_test(PPSEQ,ppseq_tst[i][0],ppseq_tst[i][1],
                                ppseq_tst[i][2],shmptr,offset);
        data1 = *(error+4);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, pp_sequencer_msg1);
        data1 = *(error+5);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, pp_sequencer_msg2); 
        data1 = *(error+6);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, pp_sequencer_msg3);
        if (*error != ppseq_tst[i][3]) {
            err_cnt++;
	    (void) sprintf(msg, pp_sequencer_msg4, 
                ppseq_tst[i][0],ppseq_tst[i][3],*error);
            gp_send_message(-DATA_ERROR, FATAL, msg);
        }
    }
    return(err_cnt);
}

/*
 **********************************************************************
 *      Test PP ALU Test
 **********************************************************************
 */
test_pp_alu(shmptr, offset)
    short *shmptr;
    int offset;
{
    int i, *error, err_cnt = 0;
    u_long data1;

    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\tTesting PP ALU \n");
    for (i = 0; i < 8; i++) {
        error = (int *)post_alu_test(PPALU,ppalu_tst[i][0],ppalu_tst[i][1],
                                ppalu_tst[i][2],shmptr,offset);
        data1 = *(error+4);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, pp_alu_msg1);
        data1 = *(error+5);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, pp_alu_msg2);
        data1 = *(error+6);
        if (data1 != 0)
            gp_send_message(-DATA_ERROR, FATAL, pp_alu_msg3);
        if (*error != ppalu_tst[i][3]) {
            err_cnt++;
	    (void) sprintf(msg, pp_alu_msg4,
                        ppalu_tst[i][0],ppalu_tst[i][3],*error);
            gp_send_message(-DATA_ERROR, FATAL, msg);
        }
    }
    return(err_cnt);
}

/*
 **********************************************************************
 *      Test PP Simple R/W of Zbuffer
 **********************************************************************
 */
test_pp_rw_zbuf(shmptr, offset)
    short *shmptr;
    int offset;
{
    int *error, err_cnt = 0;
    u_long data, data1;

    data = 1;
    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\tTesting PP one R/W Zbuffer \n");
    error = (int *)post_pp_ago(PPRW,data,shmptr,offset);
    data1 = *(error+6);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_rw_zbuf_msg1);
    data1 = *(error+7);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_rw_zbuf_msg2);
        data1 = *(error+8);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_rw_zbuf_msg3);
    if (*(error-1) != 0) {
        err_cnt++;
        switch(*(error-1)) {
            case 1 :
                data = 0;
                break;
            case 2 :
                data = 0xff;
                break;
            case 3 :
                data = 0xa5;
                break;
            case 4 :
                data = 0x5a;
                break;
        }
        (void) sprintf(msg, pp_rw_zbuf_msg4, data);
        gp_send_message(-DATA_ERROR, FATAL, msg);
    }
    return(err_cnt);
}

/*
 **********************************************************************
 *      Test PP Zbuffer
 **********************************************************************
 */
test_pp_zbuf(shmptr, offset)
    short *shmptr;
    int offset;
{
    int *error, err_cnt = 0;
    u_long data, data1;

    data = -1;
    if ((!exec_by_sundiag) || (debug))
        gp_send_message(0, DEBUG, "\tTesting PP ZBuffer\n");
    error = (int *)post_pp_ago(PPZBUF,data,shmptr,offset);
    data1 = *(error+6);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_zbuf_msg1);
    data1 = *(error+7);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_zbuf_msg2);
    data1 = *(error+8);
    if (data1 != 0)
        gp_send_message(-DATA_ERROR, FATAL, pp_zbuf_msg3);
    if (*error != 0) {
        err_cnt++;
	(void) sprintf(msg, pp_zbuf_msg4, *error);
        gp_send_message(-DATA_ERROR, FATAL, msg);
    }
    if (*(error-1) != 0) {
        err_cnt++;
	(void) sprintf(msg, pp_zbuf_msg5, *(error-1));
        gp_send_message(-DATA_ERROR, FATAL, msg);
    }
    return(err_cnt);
}

/*
 ************************************************************************
 *      Post memory test parameters to GP2
 *
 *      inputs:
 *              starting address
 *              size
 *      returns:
 *              *errors         number of errors
 *              *errors+1       failed address
 *              *errors+2       exp data
 *              *errors+3       obs data
 ************************************************************************
 */
post_mem_test(subcmd, addr, size, data, shmptr, offset)
    int subcmd;
    u_long addr,size,data;
    short *shmptr;
    int offset;
{
    int i;
    short *error;
    u_long value;

    value = GPCI_DIAG + subcmd;
    GP1_PUT_I(shmptr, value);
    GP1_PUT_I(shmptr, addr);
    GP1_PUT_I(shmptr, size);
    GP1_PUT_I(shmptr, data);
    error = shmptr;
    value = -1;
    GP1_PUT_I(shmptr, value);
    value = 0;
    for (i = 0; i < 3; i++)
        GP1_PUT_I(shmptr, value);
    value = -1;
    for (i = 0; i < 3; i++)
        GP1_PUT_I(shmptr, value);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
        gp_send_message(-POST_ERROR, ERROR, post_mem_msg1);
    for (i = 0; i < 90000; i++);
        if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
        gp_send_message(-SYNC_ERROR, ERROR, post_mem_msg2);
    return((int)error);
}

/*
 **********************************************************************
 *      post alu parameters to GP2
 *
 *      inputs:
 *              gpci subcmd
 *              alu function:  or       seq function:
 *                      0 = add         0 = eqz
 *                      1 = sub         1 = nez
 *                      2 = xor         2 = gez
 *                      3 = xnor        3 = ltz
 *                      4 = and         4 = gtz
 *                      5 = or          5 = lez
 *                      6 = nor         6 = nov
 *                      7 = not         7 = ovf
 *              1st data
 *              2nd data
 *      returns:
 *              *results        results of function
 ************************************************************************
 */
post_alu_test(subcmd, function, data1, data2, shmptr, offset)
    int subcmd;
    u_long function,data1,data2;
    short *shmptr;
    int offset;
{
    int                i;
    short              *results;
    u_long             value;

    value = GPCI_DIAG + subcmd;
    GP1_PUT_I(shmptr, value);
    GP1_PUT_I(shmptr, function);
    GP1_PUT_I(shmptr, data1);
    GP1_PUT_I(shmptr, data2);
    results = shmptr;
    value = -1;
    for (i = 0; i < 6; i++)
        GP1_PUT_I(shmptr, value);
    GP1_PUT_I(shmptr, value);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
        gp_send_message(-POST_ERROR, ERROR, post_alu_msg1);
    if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
        gp_send_message(-SYNC_ERROR, ERROR, post_alu_msg2);
    return((int)results);
}

/*
 *************************************************************************
 *      post fifo parameters to GP2
 *
 *      inputs:
 *              subcmd
 *              data
 *      returns:
 *              *error
 ************************************************************************
 */
post_fifo(subcmd,data,shmptr,offset)
int subcmd;
u_long data;
short *shmptr;
int offset;
{
    int i;
    short *error;
    u_long value;

    value = GPCI_DIAG + subcmd;
    GP1_PUT_I(shmptr, value);
    GP1_PUT_I(shmptr, data);
    value = -1;
    GP1_PUT_I(shmptr, value);
    error = shmptr;
    value = -1;
    for (i = 0; i < 7; i++)
    	GP1_PUT_I(shmptr, value);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
        gp_send_message(-POST_ERROR, ERROR, post_fifo_msg1);
    for (i = 0; i < 9000; i++);
    if ((gp1_sync(gp1_shmem, ioctlfd)) != 0 )
        gp_send_message(-SYNC_ERROR, ERROR, post_fifo_msg2);
    return((int)error);
}

/*
 *************************************************************************
 *      post_pp_ago parameters to GP2
 *
 *      inputs:
 *              subcmd
 *              data
 *      returns:
 *              *error
 ************************************************************************
 */

post_pp_ago(subcmd,data,shmptr,offset)
    int subcmd;
    u_long data;
    short *shmptr;
    int offset;
{
    int                i;
    short              *error;
    u_long             value;

    value = GPCI_DIAG + subcmd;
    GP1_PUT_I(shmptr, value);
    GP1_PUT_I(shmptr, data);
    error = shmptr;
    value = -1;
    for (i = 0; i < 8; i++)
        GP1_PUT_I(shmptr, value);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0 )
        gp_send_message(-POST_ERROR, ERROR, post_pp_ago_msg1);
    if ((gp1_sync(gp1_shmem, ioctlfd)) != 0)
        gp_send_message(-SYNC_ERROR, ERROR, post_pp_ago_msg2);
    return((int)error);
}
/*
 *************************************************************************
 *      post_ppld_reg parameters to GP2
 *
 *      inputs:
 *              subcmd
 *		reg
 *              data
 *      returns:
 *              *error
 ************************************************************************
 */

post_ppld_reg(subcmd,reg,data,shmptr,offset)
    int subcmd;
    u_long reg,data;
    short *shmptr;
    int offset;
{
    u_long             value;
    short              *error;
    int                i;

    value = GPCI_DIAG + subcmd;
    GP1_PUT_I(shmptr, value);
    GP1_PUT_I(shmptr,reg);
    GP1_PUT_I(shmptr,data);
    error = shmptr;
    value = -1;
    for (i = 0; i < 8; i++)
    	GP1_PUT_I(shmptr, value);
    if ((gp1_post(gp1_shmem, offset, ioctlfd)) != 0)
    	gp_send_message(-POST_ERROR, ERROR, post_ppld_msg1);
    if ((gp1_sync(gp1_shmem, ioctlfd)) !=0)
    	gp_send_message(-SYNC_ERROR, ERROR, post_ppld_msg2);
    return((int)error);
}
