/*	@(#)ns_xdr.h 1.1 92/07/30 SMI 	*/

/* 
 * XDR stuff for name server and utilities.
 */

#ifndef _rfs_ns_xdr_h
#define _rfs_ns_xdr_h

#define tcanon(xdrproc,from,to)  xdrwrap_/**/xdrproc(from,to,XDR_ENCODE)
#define fcanon(xdrproc,from,to)  xdrwrap_/**/xdrproc(from,to,XDR_DECODE)

/* 
 * RFS data types for XDRizing -- passed to tcanon() and fcanon()
 * Lower word is number of bytes in XDR expansion. Upper word is string 
 * identifier 
 */
#define XR_NDATA	176
#define XR_PKTHD	20
#define XR_FIRSTMSG	524
#define XR_PNHDR	16
#define XR_LONG		4
#define XR_STRING	1024
/* XR_STRING = DATASIZE #defined in sys/message.h */

bool_t xdr_rfs_token();
bool_t xdr_rfs_ndata();
bool_t xdr_rfs_n_data();
bool_t xdr_rfs_pkt_hd();
bool_t xdr_rfs_first_msg();
bool_t xdr_rfs_pnhdr();

#endif /*!_rfs_ns_xdr_h*/
