#ifndef lint
static	char sccsid[] = "@(#)mount.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:mount.c	1.14.1.1"
/*
 * This file contains code for the crash functions:  mount,  srmount, vfs.
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "sys/sysmacros.h"
#include "sys/types.h"
#include "ufs/mount.h"
#include "sys/time.h"
#include "sys/vfs.h"
#include <sys/vnode.h>
#include "rfs/rfs_misc.h"
#include "rfs/nserve.h"
#include "rfs/rfs_mnt.h"
#include <ufs/inode.h>
#include "crash.h"

unsigned mountv, srmountv;
extern localfiles;
extern char *strtbl;			/* pointer to string table */
static struct nlist *Nsrmount;

/* get arguments for mount function */
int
getmount()
{
	int phys = 0;
	unsigned addr = -1;
	int c;
	unsigned arg1 = -1;
	unsigned arg2 = -1;

	optind = 1;
	while ((c = getopt(argcnt, args, "pw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (!localfiles || !mountv) {
		fprintf(fp, "no local mounts\n");
		return;
	}

	fprintf(fp,  "    ADDR      VFS MAJ/MIN    DEVVP   SBLOCK    QINOD  QFS  BTM  FTM      NXT\n");
	if (args[optind]) {
		do {
			getargs(0xffffffff, &arg1, &arg2);
			if (arg1 == -1)
				continue;
			else addr = arg1;
			prmount(phys, addr);
			addr = arg1 = arg2 = -1;
		} while (args[++optind]);
	}
	else prmount(phys, addr);
}

/* print mount table */
int
prmount(phys, addr)
int phys;
unsigned addr;
{
	struct mount mbuf;
	int all = 0;

	if (addr == -1) {
		all = 1;
		addr = mountv;
	}
	while (1) {
		readbuf(addr, addr, phys, -1,
			(char *)&mbuf, sizeof mbuf, "mount table");
		fprintf(fp, "%8x %8x %3d,%3d %8x %8x %8x %4x %4d %4d %8x\n",
			addr,
			mbuf.m_vfsp,
			major(mbuf.m_dev),
			minor(mbuf.m_dev),
			mbuf.m_devvp,
			mbuf.m_bufp,
			mbuf.m_qinod,
			mbuf.m_qflags,
			mbuf.m_btimelimit,
			mbuf.m_ftimelimit,
			mbuf.m_nxt);
		if (!all) break;
		addr = (int)mbuf.m_nxt;
		if (!addr) break;
	}
}

/* get arguments for srmount function */
int
getsrmount()
{
	int slot = -1;
	int all = 0;
	int phys = 0;
	long addr = -1;
	int c;
	int nsrmount;
	long arg1 = -1;
	long arg2 = -1;

	if (!Nsrmount)
		if (!(Nsrmount = symsrch("nsrmount"))) 
			error("cannot determine size of server mount table\n");
	readsym("srmount", &srmountv, sizeof(int));
	optind = 1;
	while((c = getopt(argcnt,args,"epw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	checkboot();
	readmem((long)Nsrmount->n_value, 1, -1, (char *)&nsrmount,
		sizeof nsrmount, "size of server mount table");
	fprintf(fp, "SERVER MOUNT TABLE SIZE = %d\n",nsrmount);
	fprintf(fp, "SLOT SYSID     RVNO MNDX RCNT DOTDOT BCOUNT SCNT RD FLAGS \n");
	if (args[optind]) {
		all = 1;
		do {
			getargs(nsrmount, &arg1, &arg2);
			if (arg1 == -1) 
				continue;
			if (arg2 != -1)
				for (slot = arg1; slot <= arg2; slot++)
					prsrmount(all, slot, phys, addr);
			else {
				addr = arg1;
				prsrmount(all, slot, phys, addr);
			}
			slot = addr = arg1 = arg2 = -1;
		} while (args[++optind]);
	}
	else for (slot = 0; slot < nsrmount; slot++)
		prsrmount(all, slot, phys, addr);
}

/* print server mount table */
int
prsrmount(all, slot, phys, addr)
int all, slot, phys;
long addr;
{
	struct srmnt srmntbuf;

	readbuf(addr, (long)(srmountv + slot * sizeof srmntbuf), phys, -1,
		(char *)&srmntbuf, sizeof srmntbuf, "server mount table");
	if ((srmntbuf.sr_flags == MFREE) && !all)
		return;
	if (addr > -1) 
		slot = getslot(addr, (long)srmountv, sizeof srmntbuf, phys);
	fprintf(fp, "%4d  %4x %8x %4u %4u %6u %6u %4u",
		slot,
		srmntbuf.sr_sysid,
		srmntbuf.sr_rootvnode,
		srmntbuf.sr_mntindx,
		srmntbuf.sr_refcnt,
		srmntbuf.sr_dotdot,
		srmntbuf.sr_bcount,
		srmntbuf.sr_slpcnt);
	fprintf(fp, " %s", (srmntbuf.sr_flags & MRDONLY) ? "ro" : "rw");
	fprintf(fp, "%s%s%s%s\n",
		(srmntbuf.sr_flags & MINUSE) ? " inuse" : "",
		(srmntbuf.sr_flags & MLINKDOWN) ? " ldown" : "",
		(srmntbuf.sr_flags & MFUMOUNT) ? " fumnt" : "",
		(srmntbuf.sr_flags & MINTER) ? " inter" : "");
}

/* get arguments for vfs function */
int
getvfs()
{
	int phys = 0;
	unsigned addr = -1;
	int c;
	unsigned arg1 = -1;
	unsigned arg2 = -1;

	optind = 1;
	while ((c = getopt(argcnt, args, "pw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (!localfiles || !mountv) {
		fprintf(fp, "no local mounts\n");
		return;
	}

	fprintf(fp,  "    ADDR     NEXT             OPS    COVER BSIZE FSID    STATS     DATA FLAGS\n");
	if (args[optind]) {
		do {
			getargs(0xffffffff, &arg1, &arg2);
			if (arg1 == -1)
				continue;
			else addr = arg1;
			prvfs(phys, addr);
			addr = arg1 = arg2 = -1;
		} while (args[++optind]);
	}
	else prvfs(phys, addr);
}

/* print vfs table */
int
prvfs(phys, addr)
int phys;
unsigned addr;
{
	struct vfs vbuf;
	struct mount mbuf;
	int all = 0;
	char tmp[20];
	extern struct nlist *findsym();
	struct nlist *np;
	unsigned int specvfsops;

	if (addr == -1) {
		all = 1;
		readbuf(mountv, mountv, phys, -1,
			(char *)&mbuf, sizeof mbuf, "mount table");
		addr = (unsigned)mbuf.m_vfsp;
	}
	specvfsops = symsrch("spec_vfsops")->n_value;
	while (1) {
		readbuf(addr, addr, phys, -1,
			(char *)&vbuf, sizeof vbuf, "vfs table");
		if (!(np = findsym((unsigned)vbuf.vfs_op)))
			sprintf(tmp, "%x", vbuf.vfs_op);
		else sprintf(tmp, "%12s", strtbl + np->n_un.n_strx);
		fprintf(fp, "%8x %8x %15s %8x",
			addr,
			vbuf.vfs_next,
			tmp,
			vbuf.vfs_vnodecovered);
		if ((int)vbuf.vfs_op != specvfsops) fprintf(fp, " %5d %4x %8x",
			vbuf.vfs_bsize,
			vbuf.vfs_fsid,
			vbuf.vfs_stats);
		else fprintf(fp, "                    ");
		fprintf(fp, " %8x",
			vbuf.vfs_data);
		fprintf(fp, "%s%s%s%s%s%s%s%s\n",
			vbuf.vfs_flag & VFS_RDONLY ? " ro" : "",
			vbuf.vfs_flag & VFS_MLOCK ? " mlk" : "",
			vbuf.vfs_flag & VFS_MWAIT ? " mw" : "",
			vbuf.vfs_flag & VFS_NOSUID ? " nosu" : "",
			vbuf.vfs_flag & VFS_GRPID ? " gid" : "",
			vbuf.vfs_flag & VFS_NOSUB ? " nosb" : "",
			vbuf.vfs_flag & VFS_REMOUNT ? " re" : "",
			vbuf.vfs_flag & VFS_MULTI ? " mult" : "");
		if (!all) break;
		addr = (int)vbuf.vfs_next;
		if (!addr) break;
	}
}
