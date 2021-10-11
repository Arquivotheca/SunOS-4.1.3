/*      @(#)ats_sundiag.h 1.1 92/07/30 SMI          */
 
/* 
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#define OFF 	0
#define ON 	1
#define MAXPROMPTS	10

#define LOAD	1
#define STORE	2

#define ATS_SUNDIAG_ACK         	100 /* xdr_void */
#define ATS_SUNDIAG_START_TESTS 	101 /* xdr_void */
#define ATS_SUNDIAG_STOP_TESTS  	102 /* xdr_void */
#define ATS_SUNDIAG_SELECT_TEST 	103 /* xdr_strings */
#define ATS_SUNDIAG_DESELECT_TEST 	104 /* xdr_strings */
#define ATS_SUNDIAG_OPTION      	105 /* xdr_strings */
#define ATS_SUNDIAG_RESET       	106 /* xdr_void */
#define ATS_SUNDIAG_HALT		117 /* xdr_void */
#define ATS_SUNDIAG_INTERVEN		120 /* xdr_int */
#define ATS_SUNDIAG_OPT_COREFILE 	121 /* xdr_int */
#define ATS_SUNDIAG_OPTION_FILE 	122 /* xdr_option */
#define ATS_SUNDIAG_QUIT		123 /* xdr_void */
