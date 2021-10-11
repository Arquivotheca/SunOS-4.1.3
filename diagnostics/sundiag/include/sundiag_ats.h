/*      @(#)sundiag_ats.h 1.1 92/07/30 SMI          */
 
/* 
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#define SUNDIAG_ATS_ACK             200	/* xdr_void */
#define SUNDIAG_ATS_UP              201 /* xdr_int */
#define SUNDIAG_ATS_FAILURE         202 /* xdr_failure */
#define SUNDIAG_ATS_ERROR           203 /* xdr_failure */
#define SUNDIAG_ATS_START           204 /* xdr_test */
#define SUNDIAG_ATS_STOP            205 /* xdr_failure */
#define	SUNDIAG_ATS_PONG	    206 /* xdr_int */
