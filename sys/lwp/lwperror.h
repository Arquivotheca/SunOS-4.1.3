/* @(#)lwperror.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */

#ifndef _lwp_lwperror_h
#define _lwp_lwperror_h

typedef enum lwp_err_t {
	LE_NOERROR		=0,	/* No error */
	LE_NONEXIST		=1,	/* use of nonexistent object */
	LE_TIMEOUT		=2,	/* receive timed out */
	LE_INUSE		=3,	/* attempt to access object in use */
	LE_INVALIDARG		=4,	/* argument to primitive is invalid */
	LE_NOROOM		=5,	/* can't get room to create object */
	LE_NOTOWNED		=6,	/* object use without owning resource */
	LE_ILLPRIO		=7,	/* use of illegal priority */
	LE_REUSE		=8,	/* possible reuse of existing object */
	LE_NOWAIT		=9,	/* attempt to use barren object */
} lwp_err_t;

extern	char **lwp_errstr();	/* list of error messages */
extern	void lwp_perror();	/* print error message */

#ifndef _lwp_process_h
/* some defs a client may want */

extern	lwp_err_t lwp_geterr();	/* returns error for current lwp */
#endif /*!_lwp_process_h*/

#endif /*!_lwp_lwperror_h*/
