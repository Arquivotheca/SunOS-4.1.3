#ifndef lint
static char sccsid[] = "@(#)prot_share.c 1.1 92/07/30 SMI";
#endif

	/*
	 * Copyright (c) 1986 by Sun Microsystems, Inc.
	 */

	/*
	 * prot_share.c consists of subroutines that implement the
	 * DOS-compatible file sharing services for PC-NFS
	 */

#include <stdio.h>
#include <sys/file.h>
#include "prot_lock.h"
#include "priv_prot.h"


extern int debug;
extern int report_sharing_conflicts;
extern int grace_period;
extern char *xmalloc();
extern void xfree();
extern bool_t obj_cmp();
char *malloc();

bool_t compatible();

#define SH_HASH	29

/*
 * File sharing records are organized as follows:
 *
 *                     fh (file handle)
 *                      |
 *                      v
 *                    hash()
 *                      |
 *                      +-------------+
 *                                    |
 *                                    v
 *               +-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     fh_to_sx[]| | | | | | | | | | |*| | |
 *               +-+-+-+-+-+-+-+-+-+-+|+-+-+
 *                                    |
 *      +-----------------------------+
 *      |
 *      +-->  +---+   +---+   +---+   +---+
 *   sx       |FWD--->|FWD--->|FWD--->| 0 |   (head of chains for files
 *            +---+   +---+   +---+   +---+    whose handle hash to the
 *            | 0 |<---BCK|<---BCK|<---BCK|    same value)
 *            +---+   +---+   +---+   +---+
 *            |sxp|   |sxp|   |sxp|   |sxp|
 *            +---+   +-|-+   +---+   +---+
 *                      |
 *      +---------------+
 *      |
 *      +-->  +---+   +---+   +---+   +---+
 *   sxx      |FWD--->|FWD--->|FWD--->| 0 |   (multiple shared access to
 *            +---+   +---+   +---+   +---+    the same file)
 *            | 0 |<---BCK|<---BCK|<---BCK|
 *            +---+   +---+   +---+   +---+
 *            | s |   | s |   | s |   | s |	 (share: the structure
 *            | h |   | h |   | h |   | h |	  passed	in the RPC)
 *            | a |   | a |   | a |   | a |
 *            | r |   | r |   | r |   | r |
 *            | e |   | e |   | e |   | e |
 *            +---+   +---+   +---+   +---+
 */

struct	sxx {
	struct sxx	*fwd;
	struct sxx	*bck;
	nlm_share	sh;
};

struct sx {
	struct sx	*fwd;
	struct sx	*bck;
	struct sxx	*sxxp;
};

struct sx *fh_to_sx[SH_HASH];
struct sxx *new_sxx();


/*
 *	There are three entry points into the prot_share
 *	subsystem:
 *
 *	init_nlm_share
 *		called at the beginning of time to set up the hash table
 *
 *	proc_nlm_share
 *		called out of nlm_prog() in prot_main.c to process a
 *		a SHARE or UNSHARE request
 *
 *	destroy_client_shares
 *		called whenever a client has been declared dead by the
 *		status monitor. We throw away all sharing records
 *		for the client
 */

void *
init_nlm_share()
{
	int i;

	if (debug)
		printf("init_nlm_share\n");
	for (i = 0; i < SH_HASH; i++)
		fh_to_sx[i] = NULL;
}

/*
 * The following is the rough flow for proc_nlm_share:
 *
 *	hv = hash(fh)
 *	sxh = fh_to_sx[hv];
 *	if (sxh!=NULL) {
 *		scan the chain looking for sxh->sxxp.sh.fh==fh
 *	}
 *	if (sxh==NULL) {
 *		SHARE: allocate new sx and sxx and chain to fh_to_sx[hv]
 *		UNSHARE: give up
 *	}
 *	sxxh = sxh->sxxp
 *	scan the chain looking for (sxxh->sh.caller_mame==caller_name &&
 *		sxxh->sh.oh == oh) [i.e. the matching share structure]
 *	if (sxxh!=NULL) {
 *		SHARE: return {SUCCESS} [assume repeat request]
 *		UNSHARE: release *sxxh, which may free sxh, which may lead
 *		        to the fh_to_sx entry being reset
 *	}
 *	[there is no existing entry]
 *	UNSHARE: give up
 *	SHARE: scan chain starting at sxxxh->sxxp to see if the sharing
 *		is legal. If not, fail. Otherwise allocate new
 *		sxx, initialize it and add it to the list.
 *		Return {SUCCESS}
 */
	
void *
proc_nlm_share(Rqstp, Transp)
	struct svc_req *Rqstp;
	SVCXPRT *Transp;
{
	nlm_shareargs *req;
	struct	sx	*sxh;
	struct	sxx	*sxxh;
	int	hv;
	enum nlm_stats	result;
	bool_t	SHARE_REQ;
	register unsigned char *c;
	register int i;

/*
 * Remember what kind of request this was
 */

	SHARE_REQ = (Rqstp->rq_proc == NLM_SHARE);
#define UNSHARE_REQ (!SHARE_REQ)

/*
 * Allocate space for arguments and decode them
 */

	req = (nlm_shareargs *)xmalloc(sizeof(nlm_shareargs));
	if (!svc_getargs(Transp, xdr_nlm_shareargs, req)) {
		free_sreq(req);
		svcerr_decode(Transp);
		return;
	}

	if (debug) {
		printf("proc_nlm_share: %s from %s\n",
			SHARE_REQ?"share":"unshare",
			req->share.caller_name);
		printf("oh="); pr_fh(&req->share.oh);
		printf("\nfh="); pr_fh(&req->share.fh);
		printf("\n");
		if (SHARE_REQ)
			printf("share: mode=%d, access=%d\n",
				req->share.mode, req->share.access);
	}


/*
 * We only allow reclaims during the grace period
 */
	if (grace_period > 0 && !(req->reclaim)) {
		result = nlm_denied_grace_period;
		goto common_exit;
	}

/*
 * Compute the hash index and get the start of the sxx chain
 */
	hv = 0;
	i = req->share.fh.n_len;
	if (i > 20) i = 20;
	for (c = (unsigned char *) req->share.fh.n_bytes; --i;)
		hv += *c++;
	hv %= SH_HASH;

	sxh = fh_to_sx[hv];

/*
 *	If sxh != NULL, scan down looking for the fh we are dealing with
 */
	while (sxh != NULL) {
		if (cmp_fh(&req->share.fh, &sxh->sxxp->sh.fh))
			break;
		sxh = sxh->fwd;
	}

/*
 * If sxh is null, we're not currently holding a sharing state for
 * this file. If it's a SHARE request, allocate new sx & sxx structures
 */
	if (sxh == NULL) {
		if (SHARE_REQ) {
			if (debug)
				printf("...null sxh - new file\n");
			sxh = (struct sx *)malloc(sizeof(struct sx));
			sxh->bck = NULL;
			sxh->fwd = fh_to_sx[hv];
			fh_to_sx[hv] = sxh;
			sxh->sxxp = new_sxx(&req->share);
		}
		result = nlm_granted; /* either way.... */
		goto common_exit;
	}

/*
 * If sxh is non-null, we have sharing state for this file.
 * Let's try and find an sx which corresponds exactly to this
 * request.
 */
	if (debug)
		printf("...sxh @0x%x\n", sxh);
	sxxh = sxh->sxxp;
	while (sxxh != NULL) {
		if (!strcmp(sxxh->sh.caller_name, req->share.caller_name) &&
			obj_cmp(&sxxh->sh.oh, &req->share.oh))
			break;
		sxxh = sxxh->fwd;
	}

/*
 * If sxxh is non-null, we have found an entry corresponding to
 * the request. For SHARE, we just reply "nlm_granted" -- presumably
 * this is an artefact of UDP-based RPC's. For UNSHARE, we must
 * throw away the sxx, and maybe the sx.
 */
	if (sxxh != NULL) {
		if (debug)printf("...matching sxx @0x%x\n", sxxh);
		if (UNSHARE_REQ) {
/*
 * Routine list manipulation follows....
 */
			if (sxxh->bck == NULL)
				sxh->sxxp = sxxh->fwd;
			else
				sxxh->bck->fwd = sxxh->fwd;
			if (sxxh->fwd != NULL)
				sxxh->fwd->bck = sxxh->bck;
			free_sxx(sxxh);
/*
 * Check to see if we need to eliminate the sxx as well
 */
			if (sxh->sxxp == NULL) {
				if (sxh->bck == NULL)
					fh_to_sx[hv] = sxh->fwd;
				else
					sxh->bck->fwd = sxh->fwd;
				if (sxh->fwd != NULL)
					sxh->fwd->bck = sxh->bck;
				free((char *)sxh);
			}			
		}
		result = nlm_granted; /* for SHARE or UNSHARE */
		goto common_exit;
	}

/*
 * We've now established that there's no existing sx corresponding
 * to this request. For UNSHARE, we just give "ok".... For
 * SHARE we've got to validate this sharing request. If it's o.k.
 * we build a new sx and add it to the list. [Current thinking
 * is that there's no special ordering here, so we can just stick it
 * on the front.]
 */
	if (SHARE_REQ) {
		if (!compatible(sxh->sxxp, &req->share)) {
			if (debug)
				printf("...not compatible\n");
			result = nlm_denied;
			goto common_exit;
		}
		sxxh = new_sxx(&req->share);
		if (debug)printf("...compatible. new sxx @0x%x\n", sxxh);
		sxh->sxxp->bck = sxxh;
		sxxh->fwd = sxh->sxxp;
		sxh->sxxp = sxxh;
	}
	result = nlm_granted;	/* SHARE/UNSHARE. Now we fall through to... */
/*
 *	Common exit - issue reply with appropriate result code,
 * free request structure and quit
 */
common_exit:
	share_reply(Transp, req, result);
	free_sreq(req);
}

/*
 * The following is the rough flow for destroy_client_shares:
 *
 *	for (hv = 0; hv < SH_HASH; hv++){
 *	sxh = fh_to_sx[hv];
 *	while (sxh!=NULL) {
 *		sxxh = sxh->sxxp
 *		while (sxxh != NULL) {
 *			if (!strcmp(sxxh->sh.caller_name,client))
 *				blow away the entry (may longjump out of loops)
 *			sxxh=sxxh->fwd
 *		}
 *		sxh=sxh->fwd;
 *	}
 */
	
void *
destroy_client_shares(client)
	char *client;
{
	struct	sx	*sxh;
	struct	sx	*next_sx;
	struct	sxx	*sxxh;
	struct	sxx	*next_sxx;
	int	hv;

	if (debug)
		printf("destroy_client_shares for %s\n", client);

	for (hv = 0; hv < SH_HASH; hv++) {
		sxh = fh_to_sx[hv];

/*
 *	If sxxh != NULL, scan down looking for the fh we are dealing with
 */
		while (sxh != NULL) {
			sxxh = sxh->sxxp;
			next_sx = sxh->fwd;
			while (sxxh != NULL) {
				next_sxx = sxxh->fwd;
				if (!strcmp(sxxh->sh.caller_name, client)) {
/*
 * Blow this one away....
 */
					if (debug)
						printf("... zap sxx @0x%x\n", sxxh);
					if (sxxh->bck == NULL)
						sxh->sxxp = sxxh->fwd;
					else
						sxxh->bck->fwd = sxxh->fwd;
					if (sxxh->fwd != NULL)
						sxxh->fwd->bck = sxxh->bck;
					free_sxx(sxxh);
/*
 * Check to see if we need to eliminate the sxx as well
 */
					if (sxh->sxxp == NULL) {
					if (debug)
						printf("...  &  sx @0x%x\n", sxh);
						if (sxh->bck == NULL)
							fh_to_sx[hv] = sxh->fwd;
						else
							sxh->bck->fwd = sxh->fwd;
						if (sxh->fwd != NULL)
							sxh->fwd->bck = sxh->bck;
						free((char *)sxh);
						goto	skip_to_next_sx;
					}			
				}	/* ... of "if (strcmp..." */
				sxxh = next_sxx;
			} /* of "while (sxxh..." */
skip_to_next_sx:
			sxh = next_sx;
		} /* ... of "while (sxh..." */
	} /* ... of	"for (hv... " */
}


/*
 *	------ Support routines -----
 */

share_reply(T,req,rc)
	SVCXPRT *T;
	nlm_shareargs *req;
	enum	nlm_stats rc;
{
	nlm_shareres res;

	res.cookie.n_len = req->cookie.n_len;
	res.cookie.n_bytes = req->cookie.n_bytes;
	res.stat = rc;

	if (!(svc_sendreply(T, xdr_nlm_shareres, &res)))
		svcerr_systemerr(T);
}

		

free_sxx(s)
	struct sxx *s;
{
	xfree(&s->sh.caller_name);
  	xfree(&s->sh.fh.n_bytes);
	xfree(&s->sh.oh.n_bytes);
	free(s);
}



free_sreq(r)
	nlm_shareargs *r;
{
	if (r) {
		xfree(&r->cookie.n_bytes);
		xfree(&r->share.caller_name);
		xfree(&r->share.fh.n_bytes);
		xfree(&r->share.oh.n_bytes);
	}
	free(r);
}



/*
 *	Allocate a new sxx structure based on that in the RPC. We make use
 * of the fact that the caller-name, fh and oh are dynamically
 * allocated, and that we won't need them in the original copy after
 * this point in time, so we just copy the pointers and lengths and
 * set the pointers to NULL (xfree will check them in free_sreq)
 */
struct sxx *
new_sxx(s)
	nlm_share *s;
{
	struct sxx *new;
	new = (struct sxx *)malloc(sizeof(struct sxx));

	new->fwd = NULL;
	new->bck = NULL;

	new->sh.caller_name = s->caller_name;
	s->caller_name = NULL;

	new->sh.fh.n_len = s->fh.n_len;
	new->sh.fh.n_bytes = s->fh.n_bytes;
	s->fh.n_bytes = NULL;

	new->sh.oh.n_len = s->oh.n_len;
	new->sh.oh.n_bytes = s->oh.n_bytes;
	s->oh.n_bytes = NULL;

	new->sh.mode = s->mode;
	new->sh.access = s->access;

	return(new);
}

/*
 * compatible - the guts of the whole thing. Takes a pointer to
 * the first sx structure in the list and one to the share structure
 * in the RPC. Returns TRUE if the SHARE is compatible with the
 * existing state.
 *
 * The checking relies on the bit encoding for fsh_mode and
 * fsh_access (see nlm_prot.h). Specifically,
 * if any existing access ANDed with the requested mode is non-zero, OR
 * if any existing mode ANDed with the requested access is non-zero,
 * we deny the SHARE.
 */
bool_t
compatible(ss,s)
	struct sxx *ss;
	nlm_share *s;
{
	while (ss != NULL) {
		if (((int)ss->sh.mode & (int)s->access) ||
				((int)ss->sh.access & (int)s->mode)) {
			if (report_sharing_conflicts)
				printf("\nFile sharing request from %s (mode %d, access %d) conflicts\nwith sharing for %s (mode %d, access %d)\n",
				s->caller_name, s->mode, s->access,
				ss->sh.caller_name, ss->sh.mode, ss->sh.access);
			return(FALSE);
		}
		ss = ss->fwd;
	}
	return(TRUE);
}
