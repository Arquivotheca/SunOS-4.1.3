/*      @(#)pp_test.h 1.1 92/07/30 SMI          */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */


#define DEVICE_NAME     "pp0"
#define TOTAL_PASS      7     /* number of test loops */
#define ERROR_LIMIT     5     /* max number of errors allowed if run_on_error */
#define Default_pass    100   /* default # times Parallel test is run  */

#define ININ_mask         0x04
#define ININ_one          0x04
#define ININ_zero         0xFB

#define FALSE           0
#define TRUE            ~FALSE
 
/* Error return code definitions - for send_message()  */

#define NEWTEST_ERROR   1
#define TOO_MANY_ERRORS 2
#define OPEN_ERROR      8
#define NO_WRITE        10
#define NO_READ         11
#define TEST_ERR        12
#define NONZERO         13
