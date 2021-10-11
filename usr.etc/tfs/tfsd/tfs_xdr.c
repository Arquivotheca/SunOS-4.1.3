#ifndef lint
static char sccsid[] = "@(#)tfs_xdr.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include "headers.h"
#include "vnode.h"
#include "tfs.h"
#include "subr.h"

#ifdef TFSDEBUG
char           *xdropnames[] = {"encode", "decode", "free"};
#endif

#ifdef SUN_OS_4
#define RROK_MAP rrok_map
#else
#define RROK_MAP rrok_bp
#endif

/*
 * These are the XDR routines used to serialize and deserialize
 * the various structures passed as parameters accross the network
 * between NFS clients and servers.
 */



/*
 * Arguments to remote write and writecache
 */
bool_t
xdr_writeargs(xdrs, wa)
	XDR            *xdrs;
	struct nfswriteargs *wa;
{
	if (xdr_fhandle(xdrs, &wa->wa_fhandle) &&
	    xdr_long(xdrs, (long *) &wa->wa_begoff) &&
	    xdr_long(xdrs, (long *) &wa->wa_offset) &&
	    xdr_long(xdrs, (long *) &wa->wa_totcount) &&
	    xdr_bytes(xdrs, &wa->wa_data, (u_int *) & wa->wa_count,
		      NFS_MAXDATA)) {
#ifdef TFSDEBUG
		dprint(tfsdebug, 6, "xdr_writeargs: %s off %d ct %d\n",
		       xdropnames[(int) xdrs->x_op],
		       wa->wa_offset, wa->wa_totcount);
#endif
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_writeargs: %s FAILED\n",
	       xdropnames[(int) xdrs->x_op]);
#endif
	return (FALSE);
}


/*
 * File attributes
 */
bool_t
xdr_fattr(xdrs, na)
	XDR            *xdrs;
	register struct nfsfattr *na;
{
	register long  *ptr;

#ifdef TFSDEBUG
	dprint(tfsdebug, 6, "xdr_fattr: %s\n",
	       xdropnames[(int) xdrs->x_op]);
#endif
	if (xdrs->x_op == XDR_ENCODE) {
		ptr = XDR_INLINE(xdrs, 17 * BYTES_PER_XDR_UNIT);
		if (ptr != NULL) {
			IXDR_PUT_ENUM(ptr, na->na_type);
			IXDR_PUT_LONG(ptr, na->na_mode);
			IXDR_PUT_LONG(ptr, na->na_nlink);
			IXDR_PUT_LONG(ptr, na->na_uid);
			IXDR_PUT_LONG(ptr, na->na_gid);
			IXDR_PUT_LONG(ptr, na->na_size);
			IXDR_PUT_LONG(ptr, na->na_blocksize);
			IXDR_PUT_LONG(ptr, na->na_rdev);
			IXDR_PUT_LONG(ptr, na->na_blocks);
			IXDR_PUT_LONG(ptr, na->na_fsid);
			IXDR_PUT_LONG(ptr, na->na_nodeid);
			IXDR_PUT_LONG(ptr, na->na_atime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_atime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_mtime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_mtime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_ctime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_ctime.tv_usec);
			return (TRUE);
		}
	} else {
		ptr = XDR_INLINE(xdrs, 17 * BYTES_PER_XDR_UNIT);
		if (ptr != NULL) {
			na->na_type = IXDR_GET_ENUM(ptr, enum nfsftype);
			na->na_mode = IXDR_GET_LONG(ptr);
			na->na_nlink = IXDR_GET_LONG(ptr);
			na->na_uid = IXDR_GET_LONG(ptr);
			na->na_gid = IXDR_GET_LONG(ptr);
			na->na_size = IXDR_GET_LONG(ptr);
			na->na_blocksize = IXDR_GET_LONG(ptr);
			na->na_rdev = IXDR_GET_LONG(ptr);
			na->na_blocks = IXDR_GET_LONG(ptr);
			na->na_fsid = IXDR_GET_LONG(ptr);
			na->na_nodeid = IXDR_GET_LONG(ptr);
			na->na_atime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_atime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_mtime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_mtime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_ctime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_ctime.tv_usec = IXDR_GET_LONG(ptr);
			return (TRUE);
		}
	}
	if (xdr_enum(xdrs, (enum_t *) & na->na_type) &&
	    xdr_u_long(xdrs, &na->na_mode) &&
	    xdr_u_long(xdrs, &na->na_nlink) &&
	    xdr_u_long(xdrs, &na->na_uid) &&
	    xdr_u_long(xdrs, &na->na_gid) &&
	    xdr_u_long(xdrs, &na->na_size) &&
	    xdr_u_long(xdrs, &na->na_blocksize) &&
	    xdr_u_long(xdrs, &na->na_rdev) &&
	    xdr_u_long(xdrs, &na->na_blocks) &&
	    xdr_u_long(xdrs, &na->na_fsid) &&
	    xdr_u_long(xdrs, &na->na_nodeid) &&
	    xdr_timeval(xdrs, &na->na_atime) &&
	    xdr_timeval(xdrs, &na->na_mtime) &&
	    xdr_timeval(xdrs, &na->na_ctime)) {
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_fattr: %s FAILED\n",
	       xdropnames[(int) xdrs->x_op]);
#endif
	return (FALSE);
}

/*
 * Arguments to remote read
 */
bool_t
xdr_readargs(xdrs, ra)
	XDR            *xdrs;
	struct nfsreadargs *ra;
{

	if (xdr_fhandle(xdrs, &ra->ra_fhandle) &&
	    xdr_long(xdrs, (long *) &ra->ra_offset) &&
	    xdr_long(xdrs, (long *) &ra->ra_count) &&
	    xdr_long(xdrs, (long *) &ra->ra_totcount)) {
#ifdef TFSDEBUG
		dprint(tfsdebug, 6, "xdr_readargs: %s off %d ct %d\n",
		       xdropnames[(int) xdrs->x_op],
		       ra->ra_offset, ra->ra_totcount);
#endif
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_raedargs: %s FAILED\n",
	       xdropnames[(int) xdrs->x_op]);
#endif
	return (FALSE);
}


/*
 * Status OK portion of remote read reply
 */
bool_t
xdr_rrok(xdrs, rrok)
	XDR            *xdrs;
	struct nfsrrok *rrok;
{

	if (xdr_fattr(xdrs, &rrok->rrok_attr)) {
		if (xdrs->x_op == XDR_ENCODE && rrok->RROK_MAP) {
		} else {		/* client side */
			if (xdr_bytes(xdrs, &rrok->rrok_data,
				 (u_int *) & rrok->rrok_count, NFS_MAXDATA)) {
#ifdef TFSDEBUG
				dprint(tfsdebug, 6, "xdr_rrok: %s %d addr %x\n",
				       xdropnames[(int) xdrs->x_op],
				       rrok->rrok_count, rrok->rrok_data);
#endif
				return (TRUE);
			}
		}
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_rrok: %s FAILED\n",
	       xdropnames[(int) xdrs->x_op]);
#endif
	return (FALSE);
}

struct xdr_discrim rdres_discrim[2] = {
				       {(int) NFS_OK, xdr_rrok},
				       {__dontcare__, NULL_xdrproc_t}
};

/*
 * Reply from remote read
 */
bool_t
xdr_rdresult(xdrs, rr)
	XDR            *xdrs;
	struct nfsrdresult *rr;
{

#ifdef TFSDEBUG
	dprint(tfsdebug, 6, "xdr_rdresult: %s\n", xdropnames[(int) xdrs->x_op]);
#endif
	if (xdr_union(xdrs, (enum_t *) & (rr->rr_status),
		      (caddr_t) & (rr->rr_ok), rdres_discrim, xdr_void)) {
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_rdresult: %s FAILED\n",
	       xdropnames[(int) xdrs->x_op]);
#endif
	return (FALSE);
}

/*
 * File attributes which can be set
 */
bool_t
xdr_sattr(xdrs, sa)
	XDR            *xdrs;
	struct nfssattr *sa;
{

	if (xdr_u_long(xdrs, &sa->sa_mode) &&
	    xdr_u_long(xdrs, &sa->sa_uid) &&
	    xdr_u_long(xdrs, &sa->sa_gid) &&
	    xdr_u_long(xdrs, &sa->sa_size) &&
	    xdr_timeval(xdrs, &sa->sa_atime) &&
	    xdr_timeval(xdrs, &sa->sa_mtime)) {
#ifdef TFSDEBUG
		dprint(tfsdebug, 6,
		       "xdr_sattr: %s mode %o uid %d gid %d size %d\n",
		       xdropnames[(int) xdrs->x_op], sa->sa_mode, sa->sa_uid,
		       sa->sa_gid, sa->sa_size);
#endif
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_sattr: %s FAILED\n",
	       xdropnames[(int) xdrs->x_op]);
#endif
	return (FALSE);
}

struct xdr_discrim attrstat_discrim[2] = {
					  {(int) NFS_OK, xdr_fattr},
					  {__dontcare__, NULL_xdrproc_t}
};

/*
 * Reply status with file attributes
 */
bool_t
xdr_attrstat(xdrs, ns)
	XDR            *xdrs;
	struct nfsattrstat *ns;
{

	if (xdr_union(xdrs, (enum_t *) & (ns->ns_status),
		      (caddr_t) & (ns->ns_attr), attrstat_discrim, xdr_void)) {
#ifdef TFSDEBUG
		dprint(tfsdebug, 6, "xdr_attrstat: %s stat %d\n",
		       xdropnames[(int) xdrs->x_op], ns->ns_status);
#endif
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_attrstat: %s FAILED\n",
	       xdropnames[(int) xdrs->x_op]);
#endif
	return (FALSE);
}

/*
 * NFS_OK part of read sym link reply union
 */
bool_t
xdr_srok(xdrs, srok)
	XDR            *xdrs;
	struct nfssrok *srok;
{

	if (xdr_bytes(xdrs, &srok->srok_data, (u_int *) & srok->srok_count,
		      NFS_MAXPATHLEN)) {
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_srok: %s FAILED\n",
	       xdropnames[(int) xdrs->x_op]);
#endif
	return (FALSE);
}

struct xdr_discrim rdlnres_discrim[2] = {
					 {(int) NFS_OK, xdr_srok},
					 {__dontcare__, NULL_xdrproc_t}
};

/*
 * Result of reading symbolic link
 */
bool_t
xdr_rdlnres(xdrs, rl)
	XDR            *xdrs;
	struct nfsrdlnres *rl;
{

#ifdef TFSDEBUG
	dprint(tfsdebug, 6, "xdr_rdlnres: %s\n", xdropnames[(int) xdrs->x_op]);
#endif
	if (xdr_union(xdrs, (enum_t *) & (rl->rl_status),
		      (caddr_t) & (rl->rl_srok), rdlnres_discrim, xdr_void)) {
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_rdlnres: %s FAILED\n",
	       xdropnames[(int) xdrs->x_op]);
#endif
	return (FALSE);
}

/*
 * Arguments to readdir
 */
bool_t
xdr_rddirargs(xdrs, rda)
	XDR            *xdrs;
	struct nfsrddirargs *rda;
{

	if (xdr_fhandle(xdrs, &rda->rda_fh) &&
	    xdr_u_long(xdrs, &rda->rda_offset) &&
	    xdr_u_long(xdrs, &rda->rda_count)) {
#ifdef TFSDEBUG
		dprint(tfsdebug, 6,
		       "xdr_rddirargs: %s fh %d, off %d, cnt %d\n",
		       xdropnames[(int) xdrs->x_op],
		       TFS_FH(&rda->rda_fh)->fh_id, rda->rda_offset,
		       rda->rda_count);
#endif
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_rddirargs: %s FAILED\n",
	       xdropnames[(int) xdrs->x_op]);
#endif
	return (FALSE);
}

/*
 * Directory read reply:
 * union (enum status) {
 *	NFS_OK: entlist;
 *		boolean eof;
 *	default:
 * }
 *
 * Directory entries
 *	struct  direct {
 *		u_long  d_fileno;	       * inode number of entry *
 *		u_short d_reclen;	       * length of this record *
 *		u_short d_namlen;	       * length of string in d_name *
 *		char    d_name[MAXNAMLEN + 1];  * name no longer than this *
 *	};
 * are on the wire as:
 * union entlist (boolean valid) {
 * 	TRUE: struct dirent;
 *	      u_long nxtoffset;
 * 	      union entlist;
 *	FALSE:
 * }
 * where dirent is:
 * 	struct dirent {
 *		u_long	de_fid;
 *		string	de_name<NFS_MAXNAMELEN>;
 *	}
 */

#define	nextdp(udp)	((struct udirect *)((int)(udp) + (udp)->d_reclen))

/*
 * ENCODE ONLY
 */
bool_t
xdr_putrddirres(xdrs, rd)
	XDR            *xdrs;
	struct nfsreaddirres *rd;
{
	struct udirect *udp;
	char           *name;
	int             size;
	int             xdrpos;
	u_int           namlen;
	u_long          offset;
	bool_t          true = TRUE;
	bool_t          false = FALSE;

#ifdef TFSDEBUG
	dprint(tfsdebug, 6, "xdr_putrddirres: %s size %d offset %d\n",
	       xdropnames[(int) xdrs->x_op], rd->rd_size, rd->rd_offset);
#endif
	if (xdrs->x_op != XDR_ENCODE) {
		return (FALSE);
	}
	if (!xdr_enum(xdrs, (enum_t *) & rd->rd_status)) {
		return (FALSE);
	}
	if (rd->rd_status != NFS_OK) {
		return (TRUE);
	}
	xdrpos = XDR_GETPOS(xdrs);
	for (offset = rd->rd_offset, size = rd->rd_size,
	     udp = (struct udirect *) rd->rd_entries;
	     size > 0;
	     size -= udp->d_reclen, udp = nextdp(udp)) {
		if (udp->d_reclen == 0 ||
		    UENTRYSIZ(udp->d_namlen) > udp->d_reclen) {
#ifdef TFSDEBUG
			dprint(tfsdebug, 10,
			       "xdr_putrddirres: entry %d %s(%d) %d %d %d\n",
			       udp->d_fileno, udp->d_name, udp->d_namlen, udp->d_offset,
			       udp->d_reclen, UENTRYSIZ(udp->d_namlen));
			dprint(tfsdebug, 2, "xdr_putrddirres: bad directory\n");
#endif
			return (FALSE);
		}
		offset = udp->d_offset;
#ifdef TFSDEBUG
		dprint(tfsdebug, 10,
		       "xdr_putrddirres: entry %d %s(%d) %d %d %d %d\n",
		       udp->d_fileno, udp->d_name, udp->d_namlen, offset,
		       udp->d_reclen, XDR_GETPOS(xdrs), size);
#endif
		if (udp->d_fileno == 0) {
			continue;
		}
		name = udp->d_name;
		namlen = udp->d_namlen;
		if (!xdr_bool(xdrs, &true) ||
		    !xdr_u_long(xdrs, &udp->d_fileno) ||
		    !xdr_bytes(xdrs, &name, &namlen, NFS_MAXNAMLEN) ||
		    !xdr_u_long(xdrs, &offset)) {
			return (FALSE);
		}
		if (XDR_GETPOS(xdrs) - xdrpos >= rd->rd_bufsize) {
			rd->rd_eof = FALSE;
			break;
		}
	}
	if (!xdr_bool(xdrs, &false)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &rd->rd_eof)) {
		return (FALSE);
	}
	return (TRUE);
}


/*
 * DECODE ONLY
 */
/*
bool_t
xdr_getrddirres(xdrs, rd)
	XDR            *xdrs;
	struct nfsreaddirres *rd;
{
	struct udirect *udp;
	int             size;
	bool_t          valid;
	u_long          offset = (u_long) - 1;
  
	if (!xdr_enum(xdrs, (enum_t *) & rd->rd_status)) {
		return (FALSE);
	}
	if (rd->rd_status != NFS_OK) {
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 6, "xdr_getrddirres: %s size %d\n",
	       xdropnames[(int) xdrs->x_op], rd->rd_size);
#endif
	size = rd->rd_size;
	udp = rd->rd_entries;
	for (;;) {
		if (!xdr_bool(xdrs, &valid)) {
			return (FALSE);
		}
		if (valid) {
			if (!xdr_u_long(xdrs, &udp->d_fileno) ||
			    !xdr_u_short(xdrs, &udp->d_namlen) ||
			    (ENTRYSIZ(udp->d_namlen) > size) ||
			    !xdr_opaque(xdrs, udp->d_name, (u_int) udp->d_namlen) ||
			    !xdr_u_long(xdrs, &offset)) {
#ifdef TFSDEBUG
				dprint(tfsdebug, 2,
				       "xdr_getrddirres: entry error\n");
#endif
				return (FALSE);
			}
			udp->d_reclen = ENTRYSIZ(udp->d_namlen);
			udp->d_name[udp->d_namlen] = '\0';
#ifdef TFSDEBUG
			dprint(tfsdebug, 10,
			       "xdr_getrddirres: entry %d %s(%d) %d %d\n",
			       udp->d_fileno, udp->d_name, udp->d_namlen,
			       udp->d_reclen, offset);
#endif
		} else {
			break;
		}
		size -= udp->d_reclen;
		if (size <= 0) {
			return (FALSE);
		}
		udp = nextdp(udp);
	}
	if (!xdr_bool(xdrs, &rd->rd_eof)) {
		return (FALSE);
	}
	rd->rd_size = (int) udp - (int) (rd->rd_entries);
	rd->rd_offset = offset;
#ifdef TFSDEBUG
	dprint(tfsdebug, 6,
	       "xdr_getrddirres: returning size %d offset %d eof %d\n",
	       rd->rd_size, rd->rd_offset, rd->rd_eof);
#endif
	return (TRUE);
}
 */


/*
 * Arguments for directory operations
 */
bool_t
xdr_diropargs(xdrs, da)
	XDR            *xdrs;
	struct nfsdiropargs *da;
{

	if (xdr_fhandle(xdrs, &da->da_fhandle) &&
	    xdr_string(xdrs, &da->da_name, NFS_MAXNAMLEN)) {
#ifdef TFSDEBUG
		dprint(tfsdebug, 6, "xdr_diropargs: %s fh %d '%s'\n",
		       xdropnames[(int) xdrs->x_op],
		       TFS_FH(&da->da_fhandle)->fh_id, da->da_name);
#endif
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_diropargs: FAILED\n");
#endif
	return (FALSE);
}

/*
 * NFS_OK part of directory operation result
 */
bool_t
xdr_drok(xdrs, drok)
	XDR            *xdrs;
	struct nfsdrok *drok;
{

	if (xdr_fhandle(xdrs, &drok->drok_fhandle) &&
	    xdr_fattr(xdrs, &drok->drok_attr)) {
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_drok: FAILED\n");
#endif
	return (FALSE);
}

struct xdr_discrim diropres_discrim[2] = {
					  {(int) NFS_OK, xdr_drok},
					  {__dontcare__, NULL_xdrproc_t}
};

/*
 * Results from directory operation 
 */
bool_t
xdr_diropres(xdrs, dr)
	XDR            *xdrs;
	struct nfsdiropres *dr;
{

	if (xdr_union(xdrs, (enum_t *) & (dr->dr_status),
		      (caddr_t) & (dr->dr_drok), diropres_discrim, xdr_void)) {
#ifdef TFSDEBUG
		dprint(tfsdebug, 6, "xdr_diropres: %s stat %d\n",
		       xdropnames[(int) xdrs->x_op], dr->dr_status);
#endif
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_diropres: FAILED\n");
#endif
	return (FALSE);
}

/*
 * Time structure
 */
bool_t
xdr_timeval(xdrs, tv)
	XDR            *xdrs;
	struct timeval *tv;
{

	if (xdr_long(xdrs, &tv->tv_sec) &&
	    xdr_long(xdrs, &tv->tv_usec)) {
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_timeval: FAILED\n");
#endif
	return (FALSE);
}

/*
 * arguments to setattr
 */
bool_t
xdr_saargs(xdrs, argp)
	XDR            *xdrs;
	struct nfssaargs *argp;
{

	if (xdr_fhandle(xdrs, &argp->saa_fh) &&
	    xdr_sattr(xdrs, &argp->saa_sa)) {
#ifdef TFSDEBUG
		dprint(tfsdebug, 6, "xdr_saargs: %s fh %d\n",
		       xdropnames[(int) xdrs->x_op],
		       TFS_FH(&argp->saa_fh)->fh_id);
#endif
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_saargs: FAILED\n");
#endif
	return (FALSE);
}

/*
 * arguments to create and mkdir
 */
bool_t
xdr_creatargs(xdrs, argp)
	XDR            *xdrs;
	struct nfscreatargs *argp;
{

	if (xdr_diropargs(xdrs, &argp->ca_da) &&
	    xdr_sattr(xdrs, &argp->ca_sa)) {
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_creatargs: FAILED\n");
#endif
	return (FALSE);
}

/*
 * arguments to link
 */
bool_t
xdr_linkargs(xdrs, argp)
	XDR            *xdrs;
	struct nfslinkargs *argp;
{

	if (xdr_fhandle(xdrs, &argp->la_from) &&
	    xdr_diropargs(xdrs, &argp->la_to)) {
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_linkargs: FAILED\n");
#endif
	return (FALSE);
}

/*
 * arguments to rename
 */
bool_t
xdr_rnmargs(xdrs, argp)
	XDR            *xdrs;
	struct nfsrnmargs *argp;
{

	if (xdr_diropargs(xdrs, &argp->rna_from) &&
	    xdr_diropargs(xdrs, &argp->rna_to)) {
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_rnmargs: FAILED\n");
#endif
	return (FALSE);
}

/*
 * arguments to symlink
 */
bool_t
xdr_slargs(xdrs, argp)
	XDR            *xdrs;
	struct nfsslargs *argp;
{

	if (xdr_diropargs(xdrs, &argp->sla_from) &&
	    xdr_string(xdrs, &argp->sla_tnm, (u_int) MAXPATHLEN) &&
	    xdr_sattr(xdrs, &argp->sla_sa)) {
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_slargs: FAILED\n");
#endif
	return (FALSE);
}

/*
 * NFS_OK part of statfs operation
 */
xdr_fsok(xdrs, fsok)
	XDR            *xdrs;
	struct nfsstatfsok *fsok;
{

	if (xdr_long(xdrs, (long *) &fsok->fsok_tsize) &&
	    xdr_long(xdrs, (long *) &fsok->fsok_bsize) &&
	    xdr_long(xdrs, (long *) &fsok->fsok_blocks) &&
	    xdr_long(xdrs, (long *) &fsok->fsok_bfree) &&
	    xdr_long(xdrs, (long *) &fsok->fsok_bavail)) {
#ifdef TFSDEBUG
		dprint(tfsdebug, 6,
		    "xdr_fsok: %s tsz %d bsz %d blks %d bfree %d bavail %d\n",
		       xdropnames[(int) xdrs->x_op], fsok->fsok_tsize,
		       fsok->fsok_bsize, fsok->fsok_blocks, fsok->fsok_bfree,
		       fsok->fsok_bavail);
#endif
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_fsok: FAILED\n");
#endif
	return (FALSE);
}

struct xdr_discrim statfs_discrim[2] = {
					{(int) NFS_OK, xdr_fsok},
					{__dontcare__, NULL_xdrproc_t}
};

/*
 * Results of statfs operation
 */
xdr_statfs(xdrs, fs)
	XDR            *xdrs;
	struct nfsstatfs *fs;
{

	if (xdr_union(xdrs, (enum_t *) & (fs->fs_status),
		      (caddr_t) & (fs->fs_fsok), statfs_discrim, xdr_void)) {
#ifdef TFSDEBUG
		dprint(tfsdebug, 6, "xdr_statfs: %s stat %d\n",
		       xdropnames[(int) xdrs->x_op], fs->fs_status);
#endif
		return (TRUE);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 2, "xdr_statfs: FAILED\n");
#endif
	return (FALSE);
}


bool_t
xdr_tfstransargs(xdrs, args)
	XDR *xdrs;
	struct tfstransargs *args;
{
	return (xdr_fhandle(xdrs, &args->tr_fh) &&
		xdr_bool(xdrs, &args->tr_writeable));
}


bool_t
xdr_tfsdiropres(xdrs, dr)
	XDR *xdrs;
	struct tfsdiropres *dr;
{
#ifdef KERNEL
	int		old_pathlen;
#endif KERNEL

	if (!xdr_enum(xdrs, (enum_t *)&dr->dr_status)) {
		return (FALSE);
	}
	if (dr->dr_status != NFS_OK) {
		return (TRUE);
	}
#ifdef KERNEL
	old_pathlen = dr->dr_pathlen;
#endif KERNEL
	if (!xdr_u_int(xdrs, &dr->dr_pathlen)) {
		return (FALSE);
	}
#ifdef KERNEL
	if (dr->dr_pathlen > old_pathlen) {
		kmem_free(dr->dr_path, (u_int)(old_pathlen + 1));
		dr->dr_path = (caddr_t)kmem_alloc((u_int)(dr->dr_pathlen + 1));
	}
#endif KERNEL
	return (xdr_fhandle(xdrs, &dr->dr_fh) &&
		xdr_u_long(xdrs, &dr->dr_nodeid) &&
		xdr_string(xdrs, &dr->dr_path, dr->dr_pathlen) &&
		xdr_bool(xdrs, &dr->dr_writeable) &&
		xdr_sattr(xdrs, &dr->dr_sattrs));
}
