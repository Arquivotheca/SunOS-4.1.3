/*      @(#)scsi_com.h 1.1 92/07/30 SMI   */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Common definitions for SCSI routines.
 */

/*
 * Possible error levels.
 */
#define ERRLVL_COR	1	/* corrected error */
#define ERRLVL_RETRY	2	/* retryable error */
#define ERRLVL_FAULT	3	/* drive faulted */
#define ERRLVL_FATAL	4	/* fatal error */

