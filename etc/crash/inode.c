#ifndef lint
static	char sccsid[] = "@(#)inode.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:inode.c	1.14.4.4"
/*
 * This file contains code for the crash functions:  inode, file.
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "sys/sysmacros.h"
#include "sys/types.h"
#include "sys/mount.h"
#include "sys/conf.h"
#include "sys/stream.h"
#include "sys/time.h"
#include <sys/vnode.h>
#define KERNEL /* XXX */
#include "ufs/inode.h"
#include "sys/file.h"
#undef KERNEL
#include "crash.h"

struct inode ibuf;		/* buffer for inode */
unsigned nfiles, filev;
extern char *strtbl;			/* pointer to string table */

/* get arguments for inode function */
int
getinode()
{
	int slot = -1;
	int full = 0;
	int phys = 0;
	unsigned addr = -1;
	unsigned arg1 = -1;
	unsigned arg2 = -1;
	int c;
	union ihead *xihead, *axihead, *axitail, *ih;
	struct inode *ip, ibuf;
	char *heading = "MAJ/MIN  INUMB  LINK    UID   GID    SIZE   MODE FLAGS\n";
	extern struct nlist *symsrch();
	struct nlist *np;

	optind = 1;
	while ((c = getopt(argcnt, args, "fw:")) !=EOF) {
		switch(c) {
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (!symsrch("ihead")) {
                fprintf(fp, "no local inodes\n");
                return;
        }
	if (!full)
		fprintf(fp, "%s", heading);
	if (!(np = symsrch("ihead"))) {
		fprintf(fp, "can't find ihead in symbol table\n");
		return;
	}
	axihead = (union ihead *)np->n_value;
	axitail = axihead + sizeof (union  ihead) * INOHSZ;
	xihead = (union ihead *)calloc((unsigned)INOHSZ, sizeof(union ihead));
	readbuf(axihead, 0, 0, -1, xihead, INOHSZ * sizeof(union ihead),
		"inode hash table");

	for (ih = xihead; ih < &xihead[INOHSZ]; ih++) {
		for (ip = ih->ih_chain[0]; 
		     ip < (struct inode *) axihead || 
	 	      ip > (struct inode *) axitail;  
		     ip = ibuf.i_forw) {
			readbuf(ip, 0, 0, -1, &ibuf, sizeof(ibuf), "inode");
			prinode(full, &ibuf, heading);
		}
	}
}

/* print inode table */
int
prinode(full, ip, heading)
int full;
struct inode *ip;
char *heading;
{
	short mode;
	char ch;
	extern long lseek();
	register i;

	if (full)
		fprintf(fp, "%s", heading);
	fprintf(fp, " %3u, %-3u %5u  %4d %5d %5d %8ld",
		major(ip->i_dev),
		minor(ip->i_dev),
		ip->i_number,
		ip->i_nlink,
		ip->i_uid,
		ip->i_gid,
		ip->i_size);
	switch (ip->i_mode & IFMT) {
		case IFDIR: ch = 'd'; break;
		case IFCHR: ch = 'c'; break;
		case IFBLK: ch = 'b'; break;
		case IFREG: ch = 'f'; break;
		case IFIFO: ch = 'p'; break;
		case IFLNK: ch = 'l'; break;
		case IFSOCK: ch = 's'; break;
		default:    ch = '-'; break;
	}
	fprintf(fp, " %c", ch);
	mode = ip->i_mode;
	fprintf(fp, "%s%s%s%03o",
		mode & ISUID ? "u" : "-",
		mode & ISGID ? "g" : "-",
		mode & ISVTX ? "v" : "-",
		mode & 0777);
	fprintf(fp, "%s%s%s%s%s%s%s%s%s%s%s\n",
		ip->i_flag & ILOCKED ? " lk" : "",
		ip->i_flag & IUPD ? " up" : "",
		ip->i_flag & IACC ? " ac" : "",
		ip->i_flag & IMOD ? " mo" : "",
		ip->i_flag & IWANT ? " wt" : "",
		ip->i_flag & ISYNC ? " sy" : "",
		ip->i_flag & ICHG ? " ch" : "",
		ip->i_flag & ILWAIT ? " lw" : "",
		ip->i_flag & IREF ? " rf" : "",
		ip->i_flag & INOACC ? " na" : "",
		ip->i_flag & IMODTIME ? " mt" : "");
	if (!full)
		return;
	fprintf(fp, "vnode:\n");
	prvnode(&(ip->i_vnode));
	fprintf(fp, "dev vnode ptr: %8x fs ptr: %8x quota ptr: %8x\n",
		ip->i_devvp,
		ip->i_fs,
		ip->i_dquot);
	fprintf(fp, "owner: %4d count: %3d dir off: %4x\n",
		ip->i_owner,
		ip->i_count,
		ip->i_diroff);
	fprintf(fp, "read off/socket %8x blocks %4d generation %8x\n",
		ip->i_un,
		ip->i_blocks,
		ip->i_gen);
	fprintf(fp, "times:\n");
	fprintf(fp, "access  : %s", ctime(&(ip->i_atime)));
	fprintf(fp, "modified: %s", ctime(&(ip->i_mtime)));
	fprintf(fp, "changed : %s", ctime(&(ip->i_ctime)));
	fprintf(fp, "disk blocks:");
	for (i = 0; i < NDADDR; i++) {
		if ((i % 6) == 0)
			fprintf(fp, "\n");
		fprintf(fp, "%4d: ", i+1);
		fprintf(fp, "%6d", ip->i_db[i]);
	}
	fprintf(fp, "\nindirect blocks:\n");
	for (i = 0; i < NIADDR; i++) {
		fprintf(fp, "%4d: ", i+1);
		fprintf(fp, "%6d", ip->i_ib[i]);
	}
	fprintf(fp, "\n");
}

/* get arguments for vnode function */
int
getvnode()
{
	int phys = 0;
	unsigned addr = -1;
	unsigned arg1 = -1;
	unsigned arg2 = -1;
	struct vnode vbuf;
	int c;

	optind = 1;
	while ((c = getopt(argcnt, args, "pw:")) !=EOF) {
		switch(c) {
			case 'p' :	phys = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	
	if (args[optind]) {
		do {
			getargs(0xffffffff, &arg1, &arg2);
			if (arg1 != -1)  {
				addr = arg1;
				break;
			}
		} while (args[++optind]);
	}
	if (addr == -1) {
		prerrmes("no vnode address\n");
		return;
	}
	readmem(addr, !phys, -1, &vbuf, sizeof(struct vnode), "vnode structure");
	prvnode(&vbuf);

}
/* print out vnode */
prvnode(vp)
register struct vnode *vp;
{
	char tmp[20];
	extern struct nlist *findsym();
	struct nlist *np;

	fprintf(fp, "flags: ");
	fprintf(fp, "%s%s%s%s%s%s%s\n",
		vp->v_flag & VROOT ? " root" : "",
		vp->v_flag & VEXLOCK ? " exlock" : "",
		vp->v_flag & VSHLOCK ? " shlock" : "",
		vp->v_flag & VLWAIT ? " lwait" : "",
		vp->v_flag & VNOCACHE ? " nocache" : "",
		vp->v_flag & VNOMAP ? " nomap" : "",
		vp->v_flag & VISSWAP ? " swap" : "");
	fprintf(fp, "count: %4d shlockc: %4d exlockc: %4d\n",
		vp->v_count,
		vp->v_shlockc,
		vp->v_exlockc);
	fprintf(fp, "vfs mounted ptr: %8x vfs in: %8x\n",
		vp->v_vfsmountedhere,
		vp->v_vfsp);
	if (!(np = findsym((unsigned)vp->v_op)))
		sprintf(tmp, "%x", vp->v_op);
	else sprintf(tmp, "%15s", strtbl + np->n_un.n_strx);
	fprintf(fp, "ops: %15s socket/stream/page: %8x data: %8x\n",
		tmp,
		vp->v_s,
		vp->v_data);
	
	switch (vp->v_type) {
		case VNON:  sprintf(tmp, "none"); break;
		case VREG:  sprintf(tmp, "regular file"); break;
		case VDIR:  sprintf(tmp, "directory"); break;
		case VBLK:  sprintf(tmp, "block device"); break;
		case VCHR:  sprintf(tmp, "character device"); break;
		case VLNK:  sprintf(tmp, "symbolic link"); break;
		case VSOCK: sprintf(tmp, "socket"); break;
		case VBAD:  sprintf(tmp, "bad"); break;
		case VFIFO: sprintf(tmp, "fifo"); break;
		default:    sprintf(tmp, "?"); break;
	}
	fprintf(fp, "type: %s rdev: major: %d minor: %3d\n", 
		tmp,
		major(vp->v_rdev),
		minor(vp->v_rdev));
}	

/* get arguments for file function */
int
getfile()
{
	int slot = -1;
	int all = 0;
	int phys = 0;
	unsigned addr = -1;
	unsigned arg1 = -1;
	unsigned arg2 = -1;
	int c;

	optind = 1;
	while ((c = getopt(argcnt, args, "epw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	fprintf(fp, "FILE TABLE SIZE = %d\n", nfiles);
	fprintf(fp, "SLOT  TYPE RCNT   MSG            OPS     DATA   OFFSET     CRED FLAGS\n");
	if (args[optind]) {
		all = 1;
		do {
			getargs(nfiles, &arg1, &arg2);
			if (arg1 == -1) 
				continue;
			if (arg2 != -1)
				for(slot = arg1; slot <= arg2; slot++)
					prfile(all, slot, phys, addr);
			else {
				if (arg1 < nfiles)
					slot = arg1;
				else addr = arg1;
				prfile(all, slot, phys, addr);
			}
			slot = addr = arg1 = arg2 = -1;
		} while (args[++optind]);
	}
	else for (slot = 0; slot < nfiles; slot++)
		prfile(all, slot, phys, addr);
}

/* print file table */
int
prfile(all, slot, phys, addr)
int all, slot, phys;
unsigned addr;
{
	struct file fbuf;
	char tmp[20];
	extern struct nlist *findsym();
	struct nlist *np;

	readbuf(addr, filev + slot * sizeof fbuf, phys, -1,
		(char *)&fbuf, sizeof fbuf, "file table");
	if (!fbuf.f_count && !all)
		return;
	if (addr > -1) 
		slot = getslot(addr, filev, sizeof fbuf, phys,
			nfiles);
	if (slot == -1)
		fprintf(fp, "  - ");
	else fprintf(fp, "%4d", slot);
	fprintf(fp, "    %c %5d %5d",
		(fbuf.f_type == DTYPE_VNODE) ? 'V' : 'S',
		fbuf.f_count,
		fbuf.f_msgcount);
	if (!(np = findsym((unsigned)fbuf.f_ops)))
		sprintf(tmp, "%x", fbuf.f_ops);
	else sprintf(tmp, "%15s", strtbl + np->n_un.n_strx);
	fprintf(fp, "%15s %8x %8x %8x",
		tmp,
		fbuf.f_data,
		fbuf.f_offset,
		fbuf.f_cred);
	fprintf(fp, "%s%s%s%s%s%s%s%s%s%s%s%s\n",
		fbuf.f_flag & FREAD ? " read" : "",
		fbuf.f_flag & FWRITE ? " write" : "",  /* print the file flag */
		fbuf.f_flag & FNDELAY ? " ndelay" : "",
		fbuf.f_flag & FAPPEND ? " appen" : "",
		fbuf.f_flag & FASYNC ? " async" : "",
		fbuf.f_flag & FMARK ? " mark" : "",
		fbuf.f_flag & FSHLOCK ? " shlock" : "",
		fbuf.f_flag & FEXLOCK ? " exlock" : "",
		fbuf.f_flag & FCREAT ? " creat" : "",
		fbuf.f_flag & FTRUNC ? " trunc" : "",
		fbuf.f_flag & FEXCL ? " excl" : "",
		fbuf.f_flag & FSYNC ? " sync" : "");
}
