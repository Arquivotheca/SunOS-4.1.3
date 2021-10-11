#ifndef lint
static char sccsid[] = "@(#)tfs_server.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <nse/util.h>
#include "headers.h"
#include "vnode.h"
#include "tfs.h"
#include "subr.h"
#include <sys/socketvar.h>
#include <nse/tfs_calls.h>


struct svstat {
	int		ncalls;
	int		nbadcalls;
	int		old_reqs[TFS_OLD_USER_MAXPROC + 1];
	int		tfs_reqs[TFS_USER_NPROC];
} svstat;

extern bool_t	svc_finished;
extern char	tfsd_log_name[];
extern char	tfsd_lock_name[];
extern bool_t	alarm_went_off;

/*
 * Buffers for arguments and results.  Allocated in init_dispatch_buffers().
 */
static char	*disp_args;
static char	*disp_res;
char		*read_result_buffer;
char		*readdir_buffer;
bool_t		servicing_req;
struct sockaddr_in *client_addr;

int		tfs_error();
int		tfs_null();
int		tfs_getattr();
int		tfs_setattr_1();
int		tfs_setattr_2();
int		tfs_lookup_1();
int		tfs_lookup_2();
int		tfs_readlink();
int		tfs_read();
int		tfs_write();
int		tfs_create_1();
int		tfs_create_2();
int		tfs_remove();
int		tfs_rename();
int		tfs_link();
int		tfs_symlink();
int		tfs_mkdir_1();
int		tfs_mkdir_2();
int		tfs_rmdir();
int		tfs_readdir();
int		tfs_statfs();
int		tfs_unwhiteout();
int		tfs_get_wo_entries();
int		tfs_mount_1();
int		tfs_mount_2();
int		tfs_unmount_1();
int		tfs_unmount_2();
int		tfs_getname();
int		tfs_flush();
int		tfs_sync();
int		tfs_push();
int		tfs_set_searchlink();
int		tfs_clear_front();
int		tfs_wo_readonly_files();
int		tfs_pull();
int		tfs_translate();

/*
 * tfs dispatch table
 */

enum fh_type {FH_NONE, FH_NFS, FH_TFS};

struct tfsdisp {
	int             (*dis_proc) ();	/* proc to call */
	xdrproc_t       dis_xdrargs;	/* xdr routine to get args */
	int             dis_argsz;	/* sizeof args */
	xdrproc_t       dis_xdrres;	/* xdr routine to put results */
	int             dis_ressz;	/* size of results */
	enum fh_type	dis_fh_type;	/* type of fhandle in args */
};

/*
 * Version 1 TFS procedures.  Includes NFS protocol procedures (1..17)
 * and version 1 user-callable TFS procedures.
 */
struct tfsdisp olddisptab[TFS_OLD_USER_MAXPROC + 1] = {
	/* RFS_NULL = 0 not called */
	{tfs_error, xdr_void, 0,
	 xdr_void, 0, FH_NONE,
	},
	/* RFS_GETATTR = 1 */
	{tfs_getattr, xdr_fhandle, sizeof(fhandle_t),
	 xdr_attrstat, sizeof(struct nfsattrstat), FH_NFS,
	},
	/* RFS_SETATTR = 2 */
	{tfs_setattr_1, xdr_saargs, sizeof(struct nfssaargs),
	 xdr_attrstat, sizeof(struct nfsattrstat), FH_NFS,
	}, 
	/* RFS_ROOT = 3 *** NO LONGER SUPPORTED *** */
	{tfs_error, xdr_void, 0,
	 xdr_void, 0, FH_NONE,
	},
	/* RFS_LOOKUP = 4 */
	{tfs_lookup_1, xdr_diropargs, sizeof(struct nfsdiropargs),
	 xdr_diropres, sizeof(struct nfsdiropres), FH_NFS,
	},
	/* RFS_READLINK = 5 */
	{tfs_readlink, xdr_fhandle, sizeof(fhandle_t),
	 xdr_rdlnres, sizeof(struct nfsrdlnres), FH_NFS,
	},
	/* RFS_READ = 6 */
	{tfs_read, xdr_readargs, sizeof(struct nfsreadargs),
	 xdr_rdresult, sizeof(struct nfsrdresult), FH_NFS,
	},
	/* RFS_WRITECACHE = 7 *** NO LONGER SUPPORTED *** */
	{tfs_error, xdr_void, 0,
	 xdr_void, 0, FH_NONE,
	},
	/* RFS_WRITE = 8 */
	{tfs_write, xdr_writeargs, sizeof(struct nfswriteargs),
	 xdr_attrstat, sizeof(struct nfsattrstat), FH_NFS,
	},
	/* RFS_CREATE = 9 */
	{tfs_create_1, xdr_creatargs, sizeof(struct nfscreatargs),
	 xdr_diropres, sizeof(struct nfsdiropres), FH_NFS,
	},
	/* RFS_REMOVE = 10 */
	{tfs_remove, xdr_diropargs, sizeof(struct nfsdiropargs),
	 xdr_enum, sizeof(enum nfsstat), FH_NFS,
	},
	/* RFS_RENAME = 11 */
	{tfs_rename, xdr_rnmargs, sizeof(struct nfsrnmargs),
	 xdr_enum, sizeof(enum nfsstat), FH_NFS,
	},
	/* RFS_LINK = 12 */
	{tfs_link, xdr_linkargs, sizeof(struct nfslinkargs),
	 xdr_enum, sizeof(enum nfsstat), FH_NFS,
	},
	/* RFS_SYMLINK = 13 */
	{tfs_symlink, xdr_slargs, sizeof(struct nfsslargs),
	 xdr_enum, sizeof(enum nfsstat), FH_NFS,
	},
	/* RFS_MKDIR = 14 */
	{tfs_mkdir_1, xdr_creatargs, sizeof(struct nfscreatargs),
	 xdr_diropres, sizeof(struct nfsdiropres), FH_NFS,
	},
	/* RFS_RMDIR = 15 */
	{tfs_rmdir, xdr_diropargs, sizeof(struct nfsdiropargs),
	 xdr_enum, sizeof(enum nfsstat), FH_NFS,
	},
	/* RFS_READDIR = 16 */
	{tfs_readdir, xdr_rddirargs, sizeof(struct nfsrddirargs),
	 xdr_putrddirres, sizeof(struct nfsreaddirres), FH_NFS,
	},
	/* RFS_STATFS = 17 */
	{tfs_statfs, xdr_fhandle, sizeof(fhandle_t),
	 xdr_statfs, sizeof(struct nfsstatfs), FH_NFS,
	},

	/* Version 1 TFS calls */
	/* TFS_MOUNT = 18 */
	{tfs_mount_1, xdr_tfs_old_mount_args, sizeof(Tfs_old_mount_args_rec),
	 xdr_tfs_mount_res, sizeof(Tfs_mount_res_rec), FH_NONE,
	},
	/* TFS_UNMOUNT = 19 */
	{tfs_unmount_1, xdr_tfs_old_mount_args, sizeof(Tfs_old_mount_args_rec),
	 xdr_enum, sizeof(enum nfsstat), FH_NONE,
	},
	/* TFS_FLUSH = 20 */
	{tfs_flush, xdr_wrapstring, sizeof(char *),
	 xdr_enum, sizeof(enum nfsstat), FH_NONE,
	},
	/* TFS_SYNC = 21 */
	{tfs_sync, xdr_void, 0,
	 xdr_void, 0, FH_NONE,
	},
	/* TFS_UNWHITEOUT = 22 */
	{tfs_unwhiteout, xdr_tfs_name_args, sizeof(Tfs_name_args_rec),
	 xdr_enum, sizeof(enum nfsstat), FH_TFS,
	},
	/* TFS_GET_WO = 23 */
	{tfs_get_wo_entries, xdr_tfs_get_wo_args, sizeof(Tfs_get_wo_args_rec),
	 xdr_tfs_get_wo_res, sizeof(Tfs_get_wo_res_rec), FH_TFS,
	},
	/* TFS_GETNAME = 24 */
	{tfs_getname, xdr_tfs_fhandle, sizeof(Tfs_fhandle_rec),
	 xdr_tfs_getname_res, sizeof(Tfs_getname_res_rec), FH_TFS,
	},
	/* TFS_PUSH = 25 */
	{tfs_push, xdr_tfs_fhandle, sizeof(Tfs_fhandle_rec),
	 xdr_enum, sizeof(enum nfsstat), FH_TFS,
	},
	/* TFS_SET_SEARCHLINK = 26 */
	{tfs_set_searchlink,
	 xdr_tfs_searchlink_args, sizeof(Tfs_searchlink_args_rec),
	 xdr_enum, sizeof(enum nfsstat), FH_TFS,
	},
	/* TFS_CLEAR_FRONT = 27 */
	{tfs_clear_front, xdr_tfs_name_args, sizeof(Tfs_name_args_rec),
	 xdr_enum, sizeof(enum nfsstat), FH_TFS,
	},
	/* TFS_WO_READONLY_FILES = 28 */
	{tfs_wo_readonly_files, xdr_tfs_fhandle, sizeof(Tfs_fhandle_rec),
	 xdr_enum, sizeof(enum nfsstat), FH_TFS,
	},
};

/*
 * TFS version 2 program procedures.
 */
struct tfsdisp tfsdisptab[TFS_USER_NPROC] = {
	/* TFS_NULL = 0 */
	{tfs_error, xdr_void, 0,
	 xdr_void, 0, FH_NONE,
	},
	/* TFS_SETATTR = 1 */
	{tfs_setattr_2, xdr_saargs, sizeof(struct nfssaargs),
	 xdr_tfsdiropres, sizeof(struct tfsdiropres), FH_NFS,
	},
	/* TFS_LOOKUP = 2 */
	{tfs_lookup_2, xdr_diropargs, sizeof(struct nfsdiropargs),
	 xdr_tfsdiropres, sizeof(struct tfsdiropres), FH_NFS,
	},
	/* TFS_CREATE = 3 */
	{tfs_create_2, xdr_creatargs, sizeof(struct nfscreatargs),
	 xdr_tfsdiropres, sizeof(struct tfsdiropres), FH_NFS,
	},
	/* TFS_REMOVE = 4 */
	{tfs_remove, xdr_diropargs, sizeof(struct nfsdiropargs),
	 xdr_enum, sizeof(enum nfsstat), FH_NFS,
	},
	/* TFS_RENAME = 5 */
	{tfs_rename, xdr_rnmargs, sizeof(struct nfsrnmargs),
	 xdr_enum, sizeof(enum nfsstat), FH_NFS,
	},
	/* TFS_LINK = 6 */
	{tfs_link, xdr_linkargs, sizeof(struct nfslinkargs),
	 xdr_enum, sizeof(enum nfsstat), FH_NFS,
	},
	/* TFS_SYMLINK = 7 */
	{tfs_symlink, xdr_slargs, sizeof(struct nfsslargs),
	 xdr_enum, sizeof(enum nfsstat), FH_NFS,
	},
	/* TFS_MKDIR = 8 */
	{tfs_mkdir_2, xdr_creatargs, sizeof(struct nfscreatargs),
	 xdr_tfsdiropres, sizeof(struct tfsdiropres), FH_NFS,
	},
	/* TFS_RMDIR = 9 */
	{tfs_rmdir, xdr_diropargs, sizeof(struct nfsdiropargs),
	 xdr_enum, sizeof(enum nfsstat), FH_NFS,
	},
	/* TFS_READDIR = 10 */
	{tfs_readdir, xdr_rddirargs, sizeof(struct nfsrddirargs),
	 xdr_putrddirres, sizeof(struct nfsreaddirres), FH_NFS,
	},
	/* TFS_STATFS = 11 */
	{tfs_statfs, xdr_fhandle, sizeof(fhandle_t),
	 xdr_statfs, sizeof(struct nfsstatfs), FH_NFS,
	},
	/* TFS_TRANSLATE = 12 */
	{tfs_translate, xdr_tfstransargs, sizeof(struct tfstransargs),
	 xdr_tfsdiropres, sizeof(struct tfsdiropres), FH_NFS,
	},
	/* TFS_MOUNT = 13 */
	{tfs_mount_2, xdr_tfs_mount_args, sizeof(Tfs_mount_args_rec),
	 xdr_tfs_mount_res, sizeof(Tfs_mount_res_rec), FH_NONE,
	},
	/* TFS_UNMOUNT = 14 */
	{tfs_unmount_2, xdr_tfs_unmount_args, sizeof(Tfs_unmount_args_rec),
	 xdr_enum, sizeof(enum nfsstat), FH_NONE,
	},
	/* TFS_FLUSH = 15 */
	{tfs_flush, xdr_wrapstring, sizeof(char *),
	 xdr_enum, sizeof(enum nfsstat), FH_NONE,
	},
	/* TFS_SYNC = 16 */
	{tfs_sync, xdr_void, 0,
	 xdr_void, 0, FH_NONE,
	},
	/* TFS_UNWHITEOUT = 17 */
	{tfs_unwhiteout, xdr_tfs_name_args, sizeof(Tfs_name_args_rec),
	 xdr_enum, sizeof(enum nfsstat), FH_TFS,
	},
	/* TFS_GET_WO = 18 */
	{tfs_get_wo_entries, xdr_tfs_get_wo_args, sizeof(Tfs_get_wo_args_rec),
	 xdr_tfs_get_wo_res, sizeof(Tfs_get_wo_res_rec), FH_TFS,
	},
	/* TFS_GETNAME = 19 */
	{tfs_getname, xdr_tfs_fhandle, sizeof(Tfs_fhandle_rec),
	 xdr_tfs_getname_res, sizeof(Tfs_getname_res_rec), FH_TFS,
	},
	/* TFS_PUSH = 20 */
	{tfs_push, xdr_tfs_fhandle, sizeof(Tfs_fhandle_rec),
	 xdr_enum, sizeof(enum nfsstat), FH_TFS,
	},
	/* TFS_SET_SEARCHLINK = 21 */
	{tfs_set_searchlink,
	 xdr_tfs_searchlink_args, sizeof(Tfs_searchlink_args_rec),
	 xdr_enum, sizeof(enum nfsstat), FH_TFS,
	},
	/* TFS_CLEAR_FRONT = 22 */
	{tfs_clear_front, xdr_tfs_name_args, sizeof(Tfs_name_args_rec),
	 xdr_enum, sizeof(enum nfsstat), FH_TFS,
	},
	/* TFS_WO_READONLY_FILES = 23 */
	{tfs_wo_readonly_files, xdr_tfs_fhandle, sizeof(Tfs_fhandle_rec),
	 xdr_enum, sizeof(enum nfsstat), FH_TFS,
	},
	/* TFS_PULL = 24 */
	{tfs_pull, xdr_tfs_fhandle, sizeof(Tfs_fhandle_rec),
	 xdr_enum, sizeof(enum nfsstat), FH_TFS,
	},
};


void		tfs_dispatch();
void		init_dispatch_buffers();


void
tfs_dispatch(req, xprt)
	struct svc_req *req;
	register SVCXPRT *xprt;
{
	int		which;
	struct tfsdisp	*disp = NULL;
	struct tfsfh	*fh;
	struct tfsfh	fhandle;
	struct vnode	*vp;
	int		error_code = 0;
	extern time_t	tfsd_timestamp;

#ifdef TFSDEBUG
	if (tfsdebug == 7) {
		log_start_req();
	}
#endif TFSDEBUG
	svstat.ncalls++;
	which = req->rq_proc;
	client_addr = &xprt->xp_raddr;
	if (which == RFS_NULL) {
		svc_sendreply(xprt, xdr_void, (char *) NULL);
#ifdef TFSDEBUG
		if (req->rq_vers == TFS_VERSION && tfsdebug > 0) {
			tfs_null();
		}
#endif TFSDEBUG
		return;
	}
	if (req->rq_prog == TFS_PROGRAM && req->rq_vers == TFS_VERSION) {
		if (which > 0 && which < TFS_USER_NPROC) {
			disp = &tfsdisptab[which];
			svstat.tfs_reqs[which]++;
		}
	} else if (req->rq_prog == NFS_PROGRAM &&
		   req->rq_vers == NFS_VERSION) {
		if (which < RFS_NPROC) {
			disp = &olddisptab[which];
			svstat.old_reqs[which]++;
		}
	} else if (req->rq_prog == TFS_PROGRAM &&
		   req->rq_vers == TFS_OLD_VERSION) {
		if (which >= TFS_OLD_USER_MINPROC &&
		    which <= TFS_OLD_USER_MAXPROC) {
			disp = &olddisptab[which];
			svstat.old_reqs[which]++;
		}
	}
	if (!disp) {
#ifdef TFSDEBUG
		dprint(tfsdebug, 2,
		     "tfs_dispatch: bad request (prog %u  vers %u  proc %u)\n",
		       req->rq_prog, req->rq_vers, which);
#endif TFSDEBUG
		svcerr_noproc(req->rq_xprt);
		svstat.nbadcalls++;
		goto done;
	}

	if (disp->dis_argsz) {
		BZERO(disp_args, disp->dis_argsz);
	}
	if (!SVC_GETARGS(xprt, disp->dis_xdrargs, disp_args)) {
		svcerr_decode(xprt);
		svstat.nbadcalls++;
		goto done;
	}
	if (disp->dis_ressz) {
		BZERO(disp_res, disp->dis_ressz);
	}
	/*
	 * Check for unix style credentials 
	 */
	if (req->rq_cred.oa_flavor != AUTH_UNIX ||
	    set_user_id( (struct authunix_parms *) req->rq_clntcred)) {
		svcerr_weakauth(xprt);
		goto error;
	}
	servicing_req = TRUE;
	switch (disp->dis_fh_type) {
	case FH_NFS:
		fh = (struct tfsfh *) disp_args;
		/* XXX portable? */
		vp = fhtovp(fh);
		if (vp == NULL) {
			error_code = ESTALE;
		}
		break;
	case FH_TFS:
		fhandle.fh_id = ((Tfs_fhandle) disp_args)->nodeid;
		fhandle.fh_parent_id = ((Tfs_fhandle) disp_args)->parent_id;
		fhandle.fh_timestamp = tfsd_timestamp;
		fh = &fhandle;
		vp = fhtovp(fh);
		if (vp == NULL) {
			error_code = NSE_TFS_NOT_UNDER_MOUNTPT;
		}
		break;
	default:
		vp = NULL;
	}
#ifdef TFSDEBUG
	if (tfsdebug > 0 && (tfsdebug == 4 || tfsdebug >= 14)) {
		dprint_args((int) req->rq_prog, (int) req->rq_vers, which,
			    disp_args, vp, fh, disp->dis_fh_type != FH_NONE);
	}
#endif TFSDEBUG

	/*
	 * Call service routine with arg struct and results struct.
	 */
	if (!error_code) {
		error_code = (*disp->dis_proc) (vp, disp_args, disp_res);
	}

	*(enum nfsstat *) disp_res = puterrno(error_code);
#ifdef TFSDEBUG
	if (tfsdebug > 0 &&
	    (tfsdebug == 4 || tfsdebug == 5 || tfsdebug >= 15)) {
		dprint_results((int) req->rq_prog, (int) req->rq_vers, which,
			       error_code, disp_res);
	}
#endif TFSDEBUG
	/*
	 * Serialize results struct
	 */
	if (!svc_sendreply(xprt, disp->dis_xdrres, disp_res)) {
#ifdef TFSDEBUG
		dprint(tfsdebug, 1, "nfs: can't encode reply\n");
#endif
	}
error:
	/*
	 * Free arguments and results
	 */
	if (disp->dis_argsz) {
		if (!SVC_FREEARGS(xprt, disp->dis_xdrargs, disp_args)) {
#ifdef TFSDEBUG
			dprint(tfsdebug, 1, "nfs: can't free arguments\n");
#endif
		}
	}
	servicing_req = FALSE;
	if (alarm_went_off) {
		alarm_handler(0, 0, (struct sigcontext *) NULL);
	}
	if (svc_finished) {
#ifdef TFSDEBUG
		struct stat	statb;

		/*
		 * chdir to /tmp so that gprof will dump gmon.out there
		 * (and not in whatever random directory the tfsd just
		 * fchdir'ed to!)
		 */
		(void) chdir("/tmp");
		if (LSTAT(tfsd_log_name, &statb) == 0) {
			if (statb.st_size == 0) {
				(void) unlink(tfsd_log_name);
			}
		}
#endif TFSDEBUG
		(void) unlink(tfsd_lock_name);
		exit (0);
	}
done:
#ifdef TFSDEBUG
	if (tfsdebug == 7) {
		log_finish_req();
	}
#endif TFSDEBUG
}


/*
 * Allocate buffers for the arguments and results of tfs_dispatch, and for
 * the results of read requests.
 */
void
init_dispatch_buffers()
{
	union {
		fhandle_t	a;
		struct nfssaargs b;
		struct nfsdiropargs c;
		struct nfsreadargs d;
		struct nfswriteargs e;
		struct nfscreatargs f;
		struct nfsrnmargs g;
		struct nfslinkargs h;
		struct nfsslargs i;
		struct nfsrddirargs j;
		struct tfstransargs k;
		Tfs_mount_args_rec l;
		Tfs_unmount_args_rec m;
		Tfs_name_args_rec n;
		Tfs_get_wo_args_rec o;
		Tfs_fhandle_rec	p;
		Tfs_searchlink_args_rec q;
	} args;
	union {
		struct nfsattrstat a;
		struct nfsdiropres b;
		struct nfsrdlnres c;
		struct nfsrdresult d;
		struct nfsreaddirres e;
		struct nfsstatfs f;
		struct tfsdiropres g;
		Tfs_mount_res_rec i;
		Tfs_get_wo_res_rec j;
		Tfs_getname_res_rec k;
	} res;

	disp_args = nse_malloc(sizeof args);
	disp_res = nse_malloc(sizeof res);
	read_result_buffer = nse_malloc((unsigned) NFS_MAXDATA);
	readdir_buffer = nse_malloc((unsigned) NFS_MAXDATA);
}
