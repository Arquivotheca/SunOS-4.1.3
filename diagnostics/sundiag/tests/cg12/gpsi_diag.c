
#ifndef lint
static  char sccsid[] = "@(#)gpsi_diag.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <esd.h>

/**********************************************************************/
void
gpsi_diag()
/**********************************************************************/
/* Evokes the built-in C30 test codes in the GPSI */
{
    int result;
    short sram_addr;
    short sram_length;
    short dram_addr;
    short dram_length;

    /* DSP chip test */
    tmessage("C30 DSP chip: ");
    result = post_diag_test(GP1_DIAG, DSP_TEST, 0, 0, 0, 0, 0, 0, 0, 0);
    if (result) {
	error_report(errmsg_list[16]);
    } else {
	pmessage("ok.\n");
    }

    /* SRAM test */
    tmessage("SRAM: ");
    result = post_diag_test(GP1_DIAG, SRAM_TEST, 0, 0, 0, 0, 0, 0, 0, 0);
    if (result) {
	error_report(errmsg_list[17]);
    } else {
	pmessage("ok.\n");
    }

    /* DRAM test */
    tmessage("DRAM: ");
    result = post_diag_test(GP1_DIAG, DRAM_TEST, 0, 0, 0, 0, 0, 0, 0, 0);
    if (result) {
	error_report(errmsg_list[17]);
    } else {
	pmessage("ok.\n");
    }

    /* BootPROM test */
/****************** NO BOOT PROM TEST *****************************
    tmessage("Boot PROM: ");
    result = post_diag_test(GP1_DIAG, BPROM_TEST, 0, 0, 0, 0, 0, 0, 0, 0);
    if (result) {
	error_report(errmsg_list[18]);
    } else {
	pmessage("ok.\n");
    }
*******************************************************************/
}

