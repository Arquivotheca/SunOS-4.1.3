/*	@(#)rquota.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * remote quota inquiry protocol
 */

#ifndef _rpcsvc_rquota_h
#define _rpcsvc_rquota_h

#define RQUOTAPROG	100011
#define RQUOTAVERS_ORIG	1
#define RQUOTAVERS	1

/*
 * inquire about quotas for uid (assume AUTH_UNIX)
 *	input - getquota_args			(xdr_getquota_args)
 *	output - getquota_rslt			(xdr_getquota_rslt)
 */
#define RQUOTAPROC_GETQUOTA		1	/* get quota */
#define RQUOTAPROC_GETACTIVEQUOTA	2	/* get only active quotas */

/*
 * args to RQUOTAPROC_GETQUOTA and RQUOTAPROC_GETACTIVEQUOTA
 */
struct getquota_args {
	char *gqa_pathp;		/* path to filesystem of interest */
	u_int gqa_uid;			/* inquire about quota for uid */
};

/*
 * remote quota structure
 */
struct rquota {
	int rq_bsize;			/* block size for block counts */
	bool_t rq_active;		/* indicates whether quota is active */
	u_long rq_bhardlimit;		/* absolute limit on disk blks alloc */
	u_long rq_bsoftlimit;		/* preferred limit on disk blks */
	u_long rq_curblocks;		/* current block count */
	u_long rq_fhardlimit;		/* absolute limit on allocated files */
	u_long rq_fsoftlimit;		/* preferred file limit */
	u_long rq_curfiles;		/* current # allocated files */
	u_long rq_btimeleft;		/* time left for excessive disk use */
	u_long rq_ftimeleft;		/* time left for excessive files */
};	

enum gqr_status {
	Q_OK = 1,			/* quota returned */
	Q_NOQUOTA = 2,			/* noquota for uid */
	Q_EPERM = 3			/* no permission to access quota */
};

struct getquota_rslt {
	enum gqr_status gqr_status;	/* discriminant */
	struct rquota gqr_rquota;	/* valid if status == Q_OK */
};

extern bool_t xdr_getquota_args();
extern bool_t xdr_getquota_rslt();
extern bool_t xdr_rquota();

#endif /*!_rpcsvc_rquota_h*/
