#ifndef lint
static char sccsid[] = "@(#)tfs_trace.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

#ifdef TFSDEBUG

#include <nse/util.h>
#include <nse/searchlink.h>
#include "headers.h"
#include "tfs.h"
#include "subr.h"


struct procinfo {
	char		*name;
	int		(*pargs)();
	int		(*pres)();
};


pr_void()
{
}

pr_saargs(saa)
	struct nfssaargs *saa;
{
	struct nfssattr *sa;

	sa = (struct nfssattr *) &saa->saa_sa;
	dprint(0, 0,
		" (mode 0%o  uid %d  gid %d  size %d  atime %d  mtime %d)",
		sa->sa_mode, sa->sa_uid, sa->sa_gid, sa->sa_size,
		sa->sa_atime.tv_sec, sa->sa_mtime.tv_sec);
}

pr_diropargs(da)
	struct nfsdiropargs *da;
{
	dprint(0, 0, " %s", da->da_name);
}

pr_readargs(ra)
	struct nfsreadargs *ra;
{
	dprint(0, 0, " (count %d  offset %d)", ra->ra_count, ra->ra_offset);
}

pr_writeargs(wa)
	struct nfswriteargs *wa;
{
	dprint(0, 0, " (count %d  offset %d)", wa->wa_count, wa->wa_offset);
}

pr_creatargs(ca)
	struct nfscreatargs *ca;
{
	dprint(0, 0, "  %s", ca->ca_da.da_name);
}

pr_rename_args(rna)
	struct nfsrnmargs *rna;
{
	dprint(0, 0, " %s ->  fh %d  %s", rna->rna_from.da_name,
		TFS_FH(&rna->rna_to.da_fhandle)->fh_id, rna->rna_to.da_name);
}

pr_link_args(la)
	struct nfslinkargs *la;
{
	dprint(0, 0, "  fh %d  /%s",
		TFS_FH(&la->la_to.da_fhandle)->fh_id, la->la_to.da_name);
}

pr_symlink_args(sla)
	struct nfsslargs *sla;
{
	dprint(0, 0, "  %s -> %s", sla->sla_from.da_name, sla->sla_tnm);
}

pr_readdir_args(rda)
	struct nfsrddirargs *rda;
{
	dprint(0, 0, "  (count %d offset 0x%x)",
		rda->rda_count, rda->rda_offset);
}

pr_mount_args(ma)
	Tfs_mount_args ma;
{
	dprint(0, 0, " %s on %s\n", ma->directory, ma->tfs_mount_pt);
	dprint(0, 0, "  (env %s  w_layers %d  b_owner %d  b_ro %d\n",
		ma->environ, ma->writeable_layers, ma->back_owner,
		ma->back_read_only);
	dprint(0, 0, "  default '%s'  cond '%s')",
		ma->default_view, ma->conditional_view);
}

pr_unmount_args(ua)
	Tfs_unmount_args ua;
{
	dprint(0, 0, " %s  (environ %s)", ua->tfs_mount_pt, ua->environ);
}

pr_old_mount_args(ma)
	Tfs_old_mount_args ma;
{
	dprint(0, 0, " %s %s  (environ %s)",
		ma->directory, ma->tfs_mount_pt, ma->environ);
}

pr_tfsname_args(na)
	Tfs_name_args	na;
{
	dprint(0, 0, " %s", na->name);
}

pr_flush_args(fa)
	char		**fa;
{
	dprint(0, 0, " (environ %s)", *fa);
}

pr_get_wo_args(ga)
	Tfs_get_wo_args	ga;
{
	dprint(0, 0, "  (count %d offset %d)", ga->nbytes, ga->offset);
}

pr_translate_args(ta)
	struct tfstransargs *ta;
{
	dprint(0, 0, " (writeable %d)", ta->tr_writeable);
}

pr_nfsdiropres(dr)
	struct nfsdiropres *dr;
{
	dprint(0, 0, "  fh %d", TFS_FH(&dr->dr_fhandle)->fh_id);
}

pr_readlink_res(rl)
	struct nfsrdlnres *rl;
{
	dprint(0, 0, "  '%s'", rl->rl_data);
}

pr_read_res(rr)
	struct nfsrdresult *rr;
{
	dprint(0, 0, ", count = %d", rr->rr_count);
}

pr_readdir_res(rd)
	struct nfsreaddirres *rd;
{
	dprint(0, 0, " (size %d eof %d)", rd->rd_size, rd->rd_eof);
}

pr_mount_res(mr)
	Tfs_mount_res mr;
{
	dprint(0, 0, " (root fh %d  port %d)",
		TFS_FH(&mr->fh)->fh_id, mr->port);
}

pr_get_wo_res(gw)
	Tfs_get_wo_res gw;
{
	dprint(0, 0, " (count %d offset %d eof %d)",
		gw->count, gw->offset, gw->eof);
}

pr_getname_res(gn)
	Tfs_getname_res	gn;
{
	dprint(0, 0, " %s", gn->path);
}

pr_tfsdiropres(tr)
	struct tfsdiropres *tr;
{
	dprint(0, 0, "  fh %d  path '%s'",
		TFS_FH(&tr->dr_fh)->fh_id, tr->dr_path);
}


struct procinfo	old_procinfo[TFS_OLD_USER_MAXPROC + 1] = {
	{"nfs_null",		pr_void,	pr_void,},
	{"nfs_getattr",		pr_void,	pr_void,},
	{"nfs_setattr",		pr_saargs,	pr_void,},
	{"nfs_root (???)",	pr_void,	pr_void,},
	{"nfs_lookup",		pr_diropargs,	pr_nfsdiropres,},
/* 5*/	{"nfs_readlink",	pr_void,	pr_readlink_res,},
	{"nfs_read",		pr_readargs,	pr_read_res,},
	{"nfs_writecache (???)", pr_void,	pr_void,},
	{"nfs_write",		pr_writeargs,	pr_void,},
	{"nfs_create",		pr_creatargs,	pr_nfsdiropres,},
/*10*/	{"nfs_remove",		pr_diropargs,	pr_void,},
	{"nfs_rename",		pr_rename_args,	pr_void,},
	{"nfs_link",		pr_link_args,	pr_void,},
	{"nfs_symlink",		pr_symlink_args, pr_void,},
	{"nfs_mkdir",		pr_creatargs,	pr_nfsdiropres,},
/*15*/	{"nfs_rmdir",		pr_diropargs,	pr_void,},
	{"nfs_readdir",		pr_readdir_args, pr_readdir_res,},
	{"nfs_statfs",		pr_void,	pr_void,},
	{"tfs_mount_1",		pr_old_mount_args, pr_mount_res,},
	{"tfs_unmount_1",	pr_old_mount_args, pr_void,},
/*20*/	{"tfs_flush_1",		pr_flush_args,	pr_void,},
	{"tfs_sync_1",		pr_void,	pr_void,},
	{"tfs_unwhiteout_1",	pr_tfsname_args, pr_void,},
	{"tfs_get_wo_entries_1", pr_get_wo_args, pr_get_wo_res,},
	{"tfs_getname_1",	pr_void,	pr_getname_res,},
/*25*/	{"tfs_push_1",		pr_void,	pr_void,},
	{"tfs_set_searchlink_1", pr_tfsname_args, pr_void,},
	{"tfs_clear_front_1",	pr_tfsname_args, pr_void,},
	{"tfs_wo_readonly_files_1", pr_void,	pr_void,},
};


struct procinfo	tfs_procinfo[TFS_USER_NPROC] = {
	{"tfs_null",		pr_void,	pr_void,},
	{"tfs_setattr",		pr_saargs,	pr_tfsdiropres,},
	{"tfs_lookup",		pr_diropargs,	pr_tfsdiropres,},
	{"tfs_create",		pr_creatargs,	pr_tfsdiropres,},
	{"tfs_remove",		pr_diropargs,	pr_void,},
/* 5*/	{"tfs_rename",		pr_rename_args,	pr_void,},
	{"tfs_link",		pr_link_args,	pr_void,},
	{"tfs_symlink",		pr_symlink_args, pr_void,},
	{"tfs_mkdir",		pr_creatargs,	pr_tfsdiropres,},
	{"tfs_rmdir",		pr_diropargs,	pr_void,},
/*10*/	{"tfs_readdir",		pr_readdir_args, pr_readdir_res,},
	{"tfs_statfs",		pr_void,	pr_void,},
	{"tfs_translate",	pr_translate_args, pr_tfsdiropres,},
	{"tfs_mount",		pr_mount_args,	pr_mount_res,},
	{"tfs_unmount",		pr_unmount_args, pr_void,},
/*15*/	{"tfs_flush",		pr_flush_args,	pr_void,},
	{"tfs_sync",		pr_void,	pr_void,},
	{"tfs_unwhiteout",	pr_tfsname_args, pr_void,},
	{"tfs_get_wo_entries",	pr_get_wo_args,	pr_get_wo_res,},
	{"tfs_getname",		pr_void,	pr_getname_res,},
/*20*/	{"tfs_push",		pr_void,	pr_void,},
	{"tfs_set_searchlink",	pr_tfsname_args, pr_void,},
	{"tfs_clear_front",	pr_tfsname_args, pr_void,},
	{"tfs_wo_readonly_files", pr_void,	pr_void,},
	{"tfs_pull",		pr_void,	pr_void,},
};


void
dprint_args(program, version, proc, args, vp, fh, have_fhandle)
	int		program;
	int		version;
	int		proc;
	char		*args;
	struct vnode	*vp;
	struct tfsfh	*fh;
	bool_t		have_fhandle;
{
	struct procinfo	*procp;
	char		pn[NFS_MAXPATHLEN];

	if (program == NFS_PROGRAM) {
		procp = &old_procinfo[proc];
	} else if (version == TFS_VERSION) {
		procp = &tfs_procinfo[proc];
	} else {
		procp = &old_procinfo[proc];
	}
	dprint(0, 0, "%s", procp->name);
	if (have_fhandle) {
		dprint(0, 0, "  fh [%d %d]", fh->fh_parent_id, fh->fh_id);
		if (vp == NULL) {
			dprint(0, 0, " (no vnode)\n");
			return;
		}
		vtovn(vp, pn);
		dprint(0, 0, " (%s)", pn);
	}
	(*procp->pargs) (args);
	dprint(0, 0, "\n");
}


void
dprint_results(program, version, proc, error_code, results)
	int		program;
	int		version;
	int		proc;
	int		error_code;
	caddr_t		results;
{
	struct procinfo	*procp;

	if (program == NFS_PROGRAM) {
		procp = &old_procinfo[proc];
	} else if (version == TFS_VERSION) {
		procp = &tfs_procinfo[proc];
	} else {
		procp = &old_procinfo[proc];
	}
	if (tfsdebug == 4) {
		if (error_code) {
			dprint(0, 0, "%s: returning %d\n",
				procp->name, error_code);
		}
		return;
	}
	dprint(0, 0, "%s: returning %d", procp->name, error_code);
	(*procp->pres) (results);
	dprint(0, 0, "\n");
}
#endif TFSDEBUG
