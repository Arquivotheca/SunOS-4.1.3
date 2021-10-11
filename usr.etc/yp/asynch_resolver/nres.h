/*static	char sccsid[] = "@(#)nres.h 1.1 92/07/30 1989";*/
#ifndef  _yp_nres_h
#define _yp_nres_h
#include "rpc_as.h"

#if PACKETSZ > 1024
#define MAXPACKET	PACKETSZ
#else
#define MAXPACKET	1024
#endif
#define REVERSE_PTR 1
#define REVERSE_A	2
struct nres {
	rpc_as		nres_rpc_as;
	char           *userinfo;
	void            (*done) ();
	int             h_errno;
	int             reverse;/* used for gethostbyaddr */
	struct in_addr  theaddr;/* gethostbyaddr */
	char            name[MAXDNAME + 1];	/* gethostbyame name */
	char            search_name[2 * MAXDNAME + 2];
	int             search_index;	/* 0 up as we chase path */
	char            question[MAXPACKET];
	char            answer[MAXPACKET];
	int             using_tcp;	/* 0 ->udp in use */
	int             udp_socket;
	int             tcp_socket;
	int             got_nodata;	/* if we get this then no_data rather
					 * than name_not_found */
	int             question_len;
	int             answer_len;
	int             current_ns;
	int             retries;
};
#define IFRESDEBUG if (_res.options & RES_DEBUG)
#endif
