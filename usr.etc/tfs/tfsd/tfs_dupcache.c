#ifndef lint
static char sccsid[] = "@(#)tfs_dupcache.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1989 Sun Microsystems, Inc.
 */

#include <nse/param.h>
#include <nse/types.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <nse_impl/tfs.h>
#include "sysdep.h"

/*
 * Cache of recent CREATE and MKDIR requests.  These requests should only
 * be executed once by the tfsd, because executing a CREATE request more
 * than once can lead to problems, and executing a MKDIR request more than
 * once will give an EEXIST error.  This cache should really be used for
 * all once-only requests, but this would require a TFS protocol change to
 * pass a unique ID with every once-only request.  (The other once-only
 * requests are remove, rmdir, rename, link, and symlink.)  CREATE and
 * MKDIR are the only requests cached here because there are some unused
 * fields in the CREATE and MKDIR args which can be used for the necessary
 * transaction ID.  (The sa_uid field is the field used.)
 *
 * NOTE:  Here is one situation that can occur if duplicate CREATE requests
 * aren't checked for.  A process creates a file, sending a CREATE request
 * to the tfsd.  The tfsd doesn't receive the first CREATE request, so the
 * process retries the request.  The tfsd receives the retry and creates
 * the file.  The process then starts writing to the file, and after a
 * number of writes have occurred, the "lost" first CREATE request arrives
 * at the tfsd and the tfsd truncates the file, losing the just-written
 * data.
 */


#define	MAXDUPREQS	10

/*
 * The client kernel will set the sa_uid field to an integer >= 1000000 if
 * it is using the sa_uid field as a transaction ID.
 */
#define IS_VALID_XID(xid)	(xid != -1 && xid >= 1000000)

/*
 * Note that we don't need to cache the {program #, version #} of the
 * requests because requests from a given client will be for the same
 * {program #, version #}.
 */
typedef struct Duprequest *Duprequest;

struct Duprequest {
	struct sockaddr_in clnt_addr;	/* Address of client */
	int		procnum;
	u_long		xid;
	Duprequest	next;
	Duprequest	prev;
};


/*
 * mru_request points to the head of a circular linked list in MRU order.
 * mru_request->prev == LRU element in list
 */
static Duprequest mru_request;

/*
 * Address of client, set in tfs_dispatch()
 */
extern struct sockaddr_in *client_addr;

void		dupreq_save();
bool_t		dupreq_is_duplicate();
void		init_dupreq_cache();


void
dupreq_save(proc, xid)
	int		proc;
	u_long		xid;
{
	if (!IS_VALID_XID(xid)) {
		return;
	}
	mru_request = mru_request->prev;
	mru_request->clnt_addr.sin_addr = client_addr->sin_addr;
	mru_request->clnt_addr.sin_port = client_addr->sin_port;
	mru_request->procnum = proc;
	mru_request->xid = xid;
}


bool_t
dupreq_is_duplicate(proc, xid)
	int		proc;
	u_long		xid;
{
	Duprequest	dp;

	if (!IS_VALID_XID(xid)) {
		return FALSE;
	}
	dp = mru_request;
	do {
		if (xid == dp->xid && proc == dp->procnum &&
		    client_addr->sin_port == dp->clnt_addr.sin_port &&
		    client_addr->sin_addr.s_addr ==
		     dp->clnt_addr.sin_addr.s_addr) {
			return TRUE;
		}
		dp = dp->next;
	} while (dp != mru_request);
	return FALSE;
}


/*
 * Allocate whole list at once so it will be in contiguous memory.
 */
void
init_dupreq_cache()
{
	Duprequest	dp;
	Duprequest	prev_dp = NULL;
	int		i;

	for (i = 0; i < MAXDUPREQS; i++) {
		dp = NSE_NEW(Duprequest);
		if (prev_dp) {
			prev_dp->next = dp;
			dp->prev = prev_dp;
		} else {
			mru_request = dp;
		}
		prev_dp = dp;
	}
	prev_dp->next = mru_request;
	mru_request->prev = prev_dp;
}
