/*	@(#)rfs_xdr.h 1.1 92/07/30 SMI 	*/

/*
 * XDR related stuff for kernel
 */

#ifndef _rfs_rfs_xdr_h
#define _rfs_rfs_xdr_h

/*
 * Macros to call XDR conversion routines. The first argument is the
 * name of the data structure (defined below) to be converted. This
 * is concatenated with the common prefix "xdrwrap_" to generate the
 * the name of the XDR conversion routine.
 */
#define tcanon(xdrproc,from,to)  xdrwrap_/**/xdrproc(from,to,XDR_ENCODE)
#define fcanon(xdrproc,from,to)  xdrwrap_/**/xdrproc(from,to,XDR_DECODE)

/* RFS data types sizes for XDRizing -- these are the canonical sizes.  */
#define XR_LOCK		48
#define XR_STAT		88
#define XR_STATFS	52
#define XR_USTAT	32
#define XR_UTIMBUF	 8
#define XR_MESSAGE	32
#define XR_REQ		80


bool_t xdr_rfs_flock();
bool_t xdr_rfs_stat();
bool_t xdr_rfs_statfs();
bool_t xdr_rfs_ustat();
bool_t xdr_rfs_dirent();
bool_t xdr_rfs_utimbuf();
bool_t xdr_rfs_message();
bool_t xdr_rfs_common();
bool_t xdr_rfs_request();

u_int xdrwrap_rfs_flock();
u_int xdrwrap_rfs_stat();
u_int xdrwrap_rfs_statfs();
u_int xdrwrap_rfs_ustat();
u_int xdrwrap_rfs_dirent();
u_int xdrwrap_rfs_utimbuf();
u_int xdrwrap_rfs_message();
u_int xdrwrap_rfs_request();
u_int xdrwrap_rfs_string();

#endif /*!_rfs_rfs_xdr_h*/
