/*	showfh.h	1.1	92/07/30	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * showfh - show file handle client/server definitions
 */
#ifndef _rpcsvc_showfh_h
#define _rpcsvc_showfh_h

#include <sys/types.h>

#define MAXNAMELEN 	1024
#define SHOWFH_SUCCESS	0
#define SHOWFH_FAILED	1
#define FHSIZE		8

struct res_handle {
	char *file_there;	/* Name of the file or error message */
	int result;		/* succeeded or failed ? */
};
typedef struct res_handle res_handle;
bool_t xdr_res_handle();

struct nfs_handle {
	struct {
		u_int cookie_len;
		u_int *cookie_val;
	} cookie;
};
typedef struct nfs_handle nfs_handle;
bool_t xdr_nfs_handle();

#define FILEHANDLE_PROG ((u_long)100043)
#define FILEHANDLE_VERS ((u_long)1)
#define GETRHANDLE ((u_long)1)
extern res_handle *getrhandle_1();

#endif /*!_rpcsvc_showfh_h*/
