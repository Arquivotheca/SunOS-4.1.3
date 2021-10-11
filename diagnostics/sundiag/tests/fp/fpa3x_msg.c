/******************************************************************************
	@(#)fpa3x_msg.c - Rev 1.1 - 7/30/92
	Copyright (c) 1988 by Sun Microsystems, Inc.
	Deyoung Hong

	FPA-3X message file.
******************************************************************************/


char


/* Test messages */
*tst_context =	"Testing FPA%s context %d",
*tst_status =	"FPA%s: pass = %d, error = %d",
*tst_open =	"Open context %d",
*tst_close =	"Close context %d",
*tst_spmath =	"Single precision math test",
*tst_dpmath =	"Double precision math test",
*tst_dptrig =	"Double precision trig test",
*tst_reg =	"%s register test",
*tst_datareg =	"Data register test",
*tst_shadow =	"Shadow RAM test",
*tst_nack =	"Negative acknowledgement test",
*tst_simpleins = "Simple instruction test",
*tst_pointer =	"Pointer %s test",
*tst_lock =	"Lock %s test",
*tst_jumpcond =	"Jump condition test",
*tst_tipath =	"TI data path %s precision test",
*tst_tiop =	"TI operation %s test",
*tst_tistat =	"TI %s status test",
*tst_timing =	"TI timing test",
*tst_linpack =	"Linpack %s %s precision test",


/* Error messages */
*er_opendev =	"Unable to open device %s",
*er_closedev =	"Unable to close device %s",
*er_nofpa =	"FPA board is not present",
*er_no68881 =	"MC68881 chip is not present",
*er_readfpa =	"Bus error occurs when read FPA address %08X",
*er_writefpa =	"Bus error occurs when write FPA address %08X",
*er_spmath =	"Single precision math (%s) failed, exp %1.7f, obs %1.7f",
*er_dpmath =	"Double precision math (%s) failed, exp %1.15f, obs %1.15f",
*er_dptrig =	"Double precision trig (%s) failed, exp %1.15f, obs %1.15f",
*er_reg =	"%s register test failed, write %08X, read %08X",
*er_wstatus =	"WSTATUS register test failed, write %08X, exp %08X, read %08X",
*er_datareg =	"Data register %d failed, write %08X.%08X, read %08X.%08X",
*er_shadow =	"Shadow register %d failed, write %08X.%08X, read %08X.%08X",
*er_simpleins =	"Simple instruction test failed, addr %08X, status exp %08X, obs %08X",
*er_nonack =	"Nack test failed, no negative acknowledgement for status %08X",
*er_nack =	"Nack test failed, IERR exp %08X, obs %08X",
*er_pointer =	"Pointer test failed, reg %d, exp %08X.%08X, obs %08X.%08X",
*er_nolock =	"Lock test failed, shadow reg %d is not locked",
*er_lock =	"Lock test failed, shadow reg %d is locked but should not",
*er_lockierr =	"Lock test failed, IERR exp %08X, obs %08X",
*er_jumpcond =	"Jump condition test failed, reg %d, exp %08X.%08X, obs %08X.%08X",
*er_tipath =	"TI data path test failed, exp %08X.%08X, obs %08X.%08X",
*er_tiop =	"TI Operation %08X test failed, exp %08X.%08X, obs %08X.%08X",
*er_tistat =	"TI status test failed, exp %04X, obs %04X",
*er_timing =	"TI timing test failed, exp %08X.%08X, obs %08X.%08X",
*er_linpack =	"Linpack %s precision test failed",
*er_usage =	"Usage: %s [sd/menu] [d] [lt] [re] [sp] [co] [h <hostname>] [np=<pass>]\n\n";


