/*
 * This module prints out Sun RPC packets in a verbose form
 *
 * @(#)rpcprint.c 1.1 92/07/30
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/errno.h>

#include <netinet/in.h>
#include <netdb.h>

#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/pmap_clnt.h>
#include <rpc/svc.h>
#include <rpcsvc/yp_prot.h>

#include "nfs_prot.h"

extern char *sprintf();

	/* define some well-known program numbers */

#define		FIRST_RPC_PROG	((u_long)100000)
#define		PM_PROG		((u_long)100000)
#define		RSTATD_PROG	((u_long)100001)
#define		RUSERSD_PROG	((u_long)100002)
#define		NFS_PROG	((u_long)100003)
#define		YPSERV_PROG	((u_long)100004)
#define		MOUNTD_PROG	((u_long)100005)
#define		YPBIND_PROG	((u_long)100007)
#define		WALLD_PROG	((u_long)100008)
#define		YPPASSWD_PROG	((u_long)100009)
#define		ETHERSTATD_PROG	((u_long)100010)
#define		RQUOTAD_PROG	((u_long)100011)
#define		SPRAYD_PROG	((u_long)100012)
#define		MAP_3270_PROG	((u_long)100013)
#define		RJE_MAPPER_PROG	((u_long)100014)
#define		SELECTION_SVC	((u_long)100015)
#define		DATABASE_SVC	((u_long)100016)
#define		REXD_PROG	((u_long)100017)
#define		ALIS_PROG	((u_long)100018)
#define		SCHED_PROG	((u_long)100019)
#define		LLOCKMGR	((u_long)100020)
#define		NLOCKMGR	((u_long)100021)
#define		X25_INR		((u_long)100022)
#define		STATMON		((u_long)100023)
#define		STATUS		((u_long)100024)
#define		BOOTPARAMPROG	((u_long)100026)
#define		LAST_RPC_PROG	((u_long)100026)

  /*
   * The number of RPC program names cached in the following table:
   */
#define		MAX_RPC_PROG	((u_long)128)

char	*prog_names[] = {		/* table of RPC program names */
	"portmapper",		/* 100000 */
	"rstatd",		/* 100001 */
	"rusersd",		/* 100002 */
	"nfs",			/* 100003 */
	"ypserv",		/* 100004 */
	"mountd",		/* 100005 */
	NULL,
	"ypbind",		/* 100007 */
	" walld ",		/* 100008 */
	" yppasswdd ",		/* 100009 */
	" etherstatd ",		/* 100010 */
	" rquotad ",		/* 100011 */
	" sprayd ",		/* 100012 */
	" 3270_mapper ",	/* 100013 */
	" rje_mapper ",		/* 100014  */
	" selection_svc ",	/* 100015 */
	" database_svc ",	/* 100016 */
	" rexd ",		/* 100017 */
	" alis ",		/* 100018 */
	" sched ",		/* 100019 */
	" llockmgr ",		/* 100020 */
	" nlockmgr ",		/* 100021 */
	" x25.inr ",		/* 100022 */
	" statmon ",		/* 100023 */
	" status ",		/* 100024 */
	NULL,
	" bootparams "		/* 100026 */
	};

char	*nfs_procs[] = {		/* NFS procedure names */
	" Null ",		/* 0 */
	" Getattr ",		/* 1 */
	" Setattr ",		/* 2 */
	" Root ",		/* 3 */
	" Lookup ",		/* 4 */
	" Read link ",		/* 5 */
	" Read ",		/* 6 */
	" Write cache ",	/* 7 */
	" Write ",		/* 8 */
	" Create ",		/* 9 */
	" Remove ",		/* 10 */
	" Rename ",		/* 11 */
	" Link ",		/* 12 */
	" Symlink ",		/* 13 */
	" Make dir ",		/* 14 */
	" Remove dir ",		/* 15 */
	" Read dir ",		/* 16 */
	" Stat FS ",		/* 17 */
	" Nproc "		/* 18 */
	};

char	*mount_procs[] = {		/* mount procedure names */
	" MNTPROC_NULL ",	/* 0 */
	" MNTPROC_MNT ",	/* 1 */
	" MNTPROC_DUMP ",	/* 2 */
	" MNTPROC_UMNT ",	/* 3 */
	" MNTPROC_UMNTALL ",	/* 4 */
	" MNTPROC_EXPORT ",	/* 5 */
	};

char	*pm_procs[] = {			/* Portmapper procedure names */
	" PMAPPROC_NULL ",	/* 0 */
	" PMAPPROC_SET ",	/* 1 */
	" PMAPPROC_UNSET ",	/* 2 */
	" PMAPPROC_GETPORT ",	/* 3 */
	" PMAPPROC_DUMP ",	/* 4 */
	" PMAPPROC_CALLIT "	/* 5 */
	};

char	*rs_procs[] = {			/* Remote status daemon names */
	" RSTATPROC_NULL ",	/* 0 */
	" RSTATPROC_STATS ",	/* 1 */
	" RSTATPROC_HAVEDISK "	/* 2 */
	};

char	*yp_procs[] = {			/* NIS procedure names */
	" NULL ",		/* 0 */
	" Domain ",		/* 1 */
	" Domain_NONACK ", 	/* 2 */
	" Match ",		/* 3 */
	" First ",		/* 4 */
	" Next ",		/* 5 */
	" XFR ",		/* 6 */
	" Clear ",		/* 7 */
	" All ",		/* 8 */
	" Master ",		/* 9 */
	" Order ",		/* 10 */
	" Map List "		/* 11 */
	};

char	*bootparam_procs[] = {		/* Boot Param procedures */
	" BOOTPARAMPROC_NULL ",		/* 0 */
	" BOOTPARAMPROC_WHOAMI ",	/* 1 */
	" BOOTPARAMPROC_GETFILE "	/* 2 */
	};


char	*accept_stats[] = {		/* RPC reply statuses */
	" Success ",		/* 0 */
	" PROG_UNAVAIL ",	/* 1 */
	" PROG_MISMATCH ",	/* 2 */
	" PROC_UNAVAIL ",	/* 3 */
	" GARBAGE_ARGS ",	/* 4 */
	" SYSTEM_ERR "		/* 5 */
	};

char	*auth_stats[] = {		/* RPC authorization statuses */
	" auth OK (invalid with rejection) ",		/* AUTH_OK=0 */
	" bad credentials ",				/* AUTH_BADCRED=1 */
	" rejected credentials: begin new session ",	/* AUTH_REJECTEDCRED=2 */
	" bad verifier ",				/* AUTH_BADVERF=3 */
	" expired verifier ",				/* AUTH_REJECTEDVERF=4 */
	" security rejection ",				/* AUTH_TOOWEAK=5 */
	" bad response verifier (local only!) ",	/* AUTH_INVALIDRESP=6 */
	" unknown rejection "				/* AUTH_FAILED=7 */
	};


#define	XID_CACHE_SIZE	32
struct cache_struct	{
	int	xid_number;
	struct	timeval	xid_time;
} xid_cache[XID_CACHE_SIZE];
struct cache_struct	*xid_cache_ptr = &xid_cache[0];

extern int tflag;		/* turn on RPC transaction timing */
struct timeval start_time, *tvp;/* globals from ?? */

/*
 * Return true if this is a valid RPC packet
 */
valid_rpc(rpc, length)
	struct	rpc_msg *rpc;
{
	XDR	xdrm;
	int	rm_xid;
	int	rm_direction;

	if (length < sizeof(struct rpc_msg))
		return (FALSE);

	xdrmem_create(&xdrm, rpc, length, XDR_DECODE);
	if (!(xdr_int(&xdrm, &rm_xid))) {
		return (FALSE);
	}
	if (!(xdr_int(&xdrm, &rm_direction)))	{
		return(FALSE);
	}
	if ((rm_direction != 0) && (rm_direction != 1)) {
		return(FALSE);
	}
	return (TRUE);
}


cached_xid(id)
	int	id;
{
	struct cache_struct	*p = xid_cache_ptr;

	while (p != &xid_cache[0]) {
		if (p->xid_number == id) {
			if (tflag) {
				tvp->tv_sec = 
					tvp->tv_sec - p->xid_time.tv_sec 
						+ start_time.tv_sec;
				tvp->tv_usec = 
					tvp->tv_usec - p->xid_time.tv_usec 
						+ start_time.tv_usec;
			}
			return (TRUE);
		}
		p--;
	}

	p = &xid_cache[XID_CACHE_SIZE - 1];
	while (p != xid_cache_ptr) {
		if (p->xid_number == id) {
			if (tflag) {
				tvp->tv_sec = 
					tvp->tv_sec - p->xid_time.tv_sec 
						+ start_time.tv_sec;
				tvp->tv_usec = 
					tvp->tv_usec - p->xid_time.tv_usec 
						+ start_time.tv_usec;
			}
			return (TRUE);
		}
		p--;	
	}
	return (FALSE);
}

valid_nfs(rpc, length)
	struct  rpc_msg *rpc;
{
	XDR	xdrm;
	int	rm_xid;
	int	rm_direction;

	if (length < sizeof(struct rpc_msg))
		return (FALSE);

	xdrmem_create(&xdrm, rpc, length, XDR_DECODE);
	if (!(xdr_int(&xdrm, &rm_xid))) {
		display (0, "valid_rpc: can't decode transaction ident\n");
		return (FALSE);
	}
	if (!(xdr_int(&xdrm, &rm_direction)))	{
		display (0, "valid_rpc: can't decode direction\n");
		return(FALSE);
	}
	if ((rm_direction != 0) && (rm_direction != 1)) {
		return(FALSE);
	}
	if (rpc->rm_call.cb_prog == NFS_PROG) {
		xid_cache_ptr->xid_number = rm_xid;
		if (tflag) {
			xid_cache_ptr->xid_time.tv_sec = tvp->tv_sec;
			xid_cache_ptr->xid_time.tv_usec = tvp->tv_usec;
		}
		if (xid_cache_ptr == &xid_cache[XID_CACHE_SIZE - 1])
			xid_cache_ptr = &xid_cache[0];
		else
			xid_cache_ptr++;
		return (TRUE);
	}

	return (cached_xid (rm_xid));
}

/*
 * Returns the string name of the given RPC program number
 */
char *rpc_progname(prog)
	u_long prog;
{
    static char buf[128];
    int index;
    struct rpcent *rp, *getrpcbynumber();

    index = prog - FIRST_RPC_PROG;

    if (index < MAX_RPC_PROG && prog_names[index])
    	return(prog_names[index]);

    rp = NULL;
    if (index < MAX_RPC_PROG)
        rp = getrpcbynumber(prog);
    if (rp) {
	    prog_names[index] = malloc(strlen(rp->r_name)+1);
	    if (prog_names[index]) {
	    	strcpy(prog_names[index], rp->r_name);
	    	return(prog_names[index]);
	    }
    }
    sprintf(buf, "prog %d", prog);
    if (index < MAX_RPC_PROG) {
	    prog_names[index] = malloc(strlen(buf)+1);
	    if (prog_names[index])
	    	strcpy(prog_names[index], buf);
    }
    return(buf);
}


/*
 * Returns the string name of the given RPC procedure number
 */
char *rpc_procname(prog, proc)
	u_long prog, proc;
{
    static char buf[128];

    switch (prog)
    {
	case NFS_PROG:
	    if (proc <= 18) return(nfs_procs[proc]);
	    break;
	case MOUNTD_PROG:
	    if (proc <= 5) return(mount_procs[proc]);
	    break;
	case PM_PROG:
	    if (proc <= 5) return(pm_procs[proc]);
	    break;
	case RSTATD_PROG:
	    if (proc <= 2) return(rs_procs[proc]);
	    break;
	case YPSERV_PROG:
	    if (proc <= 12) return(yp_procs[proc]);
	    break;
	case BOOTPARAMPROG:
	    if (proc <= 2) return(bootparam_procs[proc]);
	    break;
    }
    sprintf(buf, "proc %d", proc);
    return(buf);
}

static char auth_buf[MAX_AUTH_BYTES];
static char verf_buf[MAX_AUTH_BYTES];

/*
 * Print an accepted RPC reply 
 */
rpc_printreply(index, msg, xdrp)
    int index;
    register struct rpc_msg *msg;
    XDR *xdrp;
{
    enum accept_stat ar_stat;

    msg->acpted_rply.ar_verf.oa_base = auth_buf;
    if (!(xdr_opaque_auth(xdrp, &msg->acpted_rply.ar_verf))) {
    	display(index, "Can't decode accepted reply\n");
	return;
    }
    if (!(xdr_enum(xdrp, &msg->acpted_rply.ar_stat))) {
    	display(index, "Can't decode accepted reply\n");
	return;
    }
    ar_stat = msg->acpted_rply.ar_stat;
    display(index," [%x] ", msg->rm_xid);
    switch (msg->acpted_rply.ar_verf.oa_flavor)
    {
	case AUTH_NULL:
		display(index,"AUTH_NULL");
		break;

	case AUTH_UNIX:
		display(index,"AUTH_UNIX");
		break;

	case AUTH_SHORT:
		display(index,"AUTH_SHORT");
		break;

	case AUTH_DES:
		display(index,"AUTH_DES");
		break;

	default:
		display(index,"auth %d ", msg->acpted_rply.ar_verf.oa_flavor);
    }

    if ((u_long ) ar_stat <= (u_long) SYSTEM_ERR)
    	display(index,accept_stats[(int)ar_stat]);

    switch (ar_stat)
    {
      case SUCCESS:
      case PROG_MISMATCH:
      case PROG_UNAVAIL:
      case PROC_UNAVAIL:
      case GARBAGE_ARGS:
      case SYSTEM_ERR:
      	   /*
	    * To be completed
	    */
      	break;
      default:
      	display(index," bad status %d", ar_stat);
    }
}

rpcprint( index, p, size )
    int index;
    char *p;
    int size;
{
    XDR xdrm;
    struct rpc_msg msg;

    xdrmem_create(&xdrm, p, size, XDR_DECODE);
    if (!(xdr_long(&xdrm, &msg.rm_xid)) ||
        !(xdr_long(&xdrm, &msg.rm_direction)))	{
		/*** Probably just not a real RPC after all ***/
		return;
    }
    
    switch (msg.rm_direction)
     {
       case CALL:
        if (!(xdr_long(&xdrm, &msg.rm_call.cb_rpcvers)))
		return;
	if (msg.rm_call.cb_rpcvers != RPC_MSG_VERSION)	{
		display(index, " bad version %d ", msg.rm_call.cb_rpcvers);
		break;
	}
	msg.rm_call.cb_cred.oa_base = auth_buf;
	msg.rm_call.cb_verf.oa_base = verf_buf;
	if (!(xdr_long(&xdrm, &msg.rm_call.cb_prog)) || 
	    !(xdr_long(&xdrm, &msg.rm_call.cb_vers)) ||
	    !(xdr_long(&xdrm, &msg.rm_call.cb_proc)) ||
	    !(xdr_opaque_auth(&xdrm, &msg.rm_call.cb_cred)) ||
	    !(xdr_opaque_auth(&xdrm, &msg.rm_call.cb_verf))) {
		display(index, "Can't decode RPC call\n");
		break;
	}
       	display(index,"\n RPC Call %s %s V%d [%x]",
		 rpc_progname(msg.rm_call.cb_prog),
		 rpc_procname(msg.rm_call.cb_prog, msg.rm_call.cb_proc),
		 msg.rm_call.cb_vers, msg.rm_xid );
	if (msg.rm_call.cb_prog == NFS_PROG)
		nfs_print(display, &xdrm, msg.rm_call.cb_proc);
	if (msg.rm_call.cb_prog == YPSERV_PROG && msg.rm_call.cb_vers==YPVERS)
		yp_print(display, &xdrm, msg.rm_call.cb_proc);
	break;

       case REPLY:  
	if (!(xdr_long(&xdrm, &msg.rm_reply.rp_stat))) {
		display(index, "can't decode reply\n");
		return;
	}
       	display(index,"\n RPC Reply ");
	switch (msg.rm_reply.rp_stat)
	{
	  case MSG_ACCEPTED:
		rpc_printreply(index, &msg, &xdrm);
	  	break;

	  case MSG_DENIED:
	  	if (!(xdr_rejected_reply(&xdrm, &msg.rjcted_rply))) {
			display(index, "can't decode rejected reply\n");
		}
	  	display(index,"denied (low %d hi %d) [%x]",
		    msg.rjcted_rply.rj_vers.low,
		    msg.rjcted_rply.rj_vers.high,
		    msg.rm_xid );
		if ((u_long)msg.rjcted_rply.rj_why <= (u_long) AUTH_FAILED)
			display(index,auth_stats[msg.rjcted_rply.rj_why]);
	  	break;

	  default:
	  	display(index,"\tillegal reply status, %d", 
				msg.rm_reply.rp_stat);
	}
	break;

       default:
          /*
	   * Probably just not an RPC, so quietly ignore it
	   */
       	return;
     }
}

char *fh_print(fh)
    struct nfs_fh *fh;
{
    static char buf[256];
    
    (void) sprintf(buf, "%d.%x", _getlong(fh->data), 
    			_getlong(&(fh->data[12])));
    return(buf);
}

nfs_print(index, xdrp, proc)
    int index;
    XDR *xdrp;
    int proc;
{
  struct nfs_fh fh;
  unsigned value;
  char *name;
  static char buf[256];
  struct sattr sa;
  struct fattr fa;

  switch (proc)
  {
    case NFSPROC_NULL:
		break;

    case NFSPROC_GETATTR:
    		(void) xdr_nfs_fh(xdrp, &fh);
		display(index, " %s", fh_print(&fh));
		break;

    case NFSPROC_SETATTR:
    		(void) xdr_nfs_fh(xdrp, &fh);
		display(index, " %s", fh_print(&fh));
		(void) xdr_sattr(xdrp, &sa);
		break;

    case NFSPROC_ROOT:
		break;

    case NFSPROC_LOOKUP:
    case NFSPROC_REMOVE:
    case NFSPROC_RMDIR:
    		(void) xdr_nfs_fh(xdrp, &fh);
		name = buf;
		(void) xdr_filename(xdrp, &name);
		display(index, " %s %s", fh_print(&fh), name);
		break;

    case NFSPROC_READLINK:
    		(void) xdr_nfs_fh(xdrp, &fh);
		display(index, " %s", fh_print(&fh));
		break;

    case NFSPROC_READ:
    		(void) xdr_nfs_fh(xdrp, &fh);
		display(index, " %s", fh_print(&fh));
		(void) xdr_long(xdrp, &value);
		display(index, " off=%d,", value);
		(void) xdr_long(xdrp, &value);
		display(index, " count=%d,", value);
		(void) xdr_long(xdrp, &value);
		display(index, " total=%d", value);
		break;

    case NFSPROC_WRITECACHE:
		break;

    case NFSPROC_WRITE:
    		(void) xdr_nfs_fh(xdrp, &fh);
		display(index, " %s", fh_print(&fh));
		(void) xdr_long(xdrp, &value);
		display(index, " begin=%d,", value);
		(void) xdr_long(xdrp, &value);
		display(index, " off=%d,", value);
		(void) xdr_long(xdrp, &value);
		display(index, " total=%d", value);
		(void) xdr_long(xdrp, &value);
		display(index, " count=%d", value);
		break;

    case NFSPROC_CREATE:
    case NFSPROC_MKDIR:
    		(void) xdr_nfs_fh(xdrp, &fh);
		name = buf;
		(void) xdr_filename(xdrp, &name);
		display(index, " %s %s", fh_print(&fh), name);
		break;

    case NFSPROC_RENAME:
    		(void) xdr_nfs_fh(xdrp, &fh);
		name = buf;
		(void) xdr_filename(xdrp, &name);
		display(index, " %s %s", fh_print(&fh), name);
    		(void) xdr_nfs_fh(xdrp, &fh);
		name = buf;
		(void) xdr_filename(xdrp, &name);
		display(index, " to %s %s", fh_print(&fh), name);
		break;

    case NFSPROC_LINK:
    		(void) xdr_nfs_fh(xdrp, &fh);
		display(index, " %s", fh_print(&fh));
    		(void) xdr_nfs_fh(xdrp, &fh);
		name = buf;
		(void) xdr_filename(xdrp, &name);
		display(index, " to %s %s", fh_print(&fh), name);
		break;

    case NFSPROC_SYMLINK:
    		(void) xdr_nfs_fh(xdrp, &fh);
		name = buf;
		(void) xdr_filename(xdrp, &name);
		display(index, " from %s %s", fh_print(&fh), name);
		name = buf;
		(void) xdr_filename(xdrp, &name);
		display(index, " to %s", name);
		(void) xdr_sattr(xdrp, &sa);
		break;

    case NFSPROC_READDIR:
    		(void) xdr_nfs_fh(xdrp, &fh);
		display(index, " %s", fh_print(&fh));
		(void) xdr_long(xdrp, &value);
		display(index, " cookie=%d", value);
		(void) xdr_long(xdrp, &value);
		display(index, " count=%d", value);
		break;

    case NFSPROC_STATFS:
    		(void) xdr_nfs_fh(xdrp, &fh);
		display(index, " %s", fh_print(&fh));
		break;

  }
}


yp_print(index, xdrp, proc)
    int index;
    XDR *xdrp;
    int proc;
{
  unsigned value;
  char *name;
  static char dom[256], map[256], key[256];
  struct ypreq_key ypk;
  struct ypreq_nokey ypnk;

  switch (proc)
  {
    case YPPROC_NULL:
		break;

    case YPPROC_DOMAIN:
    case YPPROC_DOMAIN_NONACK:
    case YPPROC_MAPLIST:
    		(void) xdr_ypdomain_wrap_string(xdrp, dom);
		display(index, " %s", dom);
		break;

    case YPPROC_MATCH:
    case YPPROC_NEXT:
    		ypk.domain = dom;
		ypk.map = map;
		ypk.keydat.dptr = key;
		(void) xdr_ypreq_key(xdrp, &ypk);
		key[ypk.keydat.dsize] = '\0';
		display(index, " domain %s map %s key len %d %s",
			dom, map, ypk.keydat.dsize, key);
		break;

    case YPPROC_FIRST:
    case YPPROC_ALL:
    case YPPROC_MASTER:
    case YPPROC_ORDER:
    		ypnk.domain = dom;
		ypnk.map = map;
		(void) xdr_ypreq_nokey(xdrp, &ypnk);
		key[ypk.keydat.dsize] = '\0';
		display(index, " domain %s map %s", dom, map);
		break;
  }
}
