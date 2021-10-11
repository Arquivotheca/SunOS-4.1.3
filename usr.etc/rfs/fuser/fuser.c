/*	@(#)fuser.c 1.1 92/07/30 SMI	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fuser:fuser.c	1.27.2.1"

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*			The last swap based fuser for the 3B5
			and the 3B2 is rev 1.17 */
#include <nlist.h>
#include <kvm.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/signal.h>
#define KERNEL
#include <sys/file.h>
#undef KERNEL
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/sysmacros.h>
#include <sys/ucred.h>
#include <machine/vm_hat.h>
#include <vm/as.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_vn.h>
#include <rfs/sema.h>
#include <rfs/rfs_misc.h>
#include <rfs/nserve.h>
#include <rfs/comm.h>
#include <rfs/rfs_mnt.h>

/* symbol names */
#define NSNDD_STR "_nsndd"
#define SNDD_STR  "_sndd"
#define NRCVD_STR "_nrcvd"
#define RCVD_STR  "_rcvd"
#define RFSFST_STR "_rfs_vnodeops"
#define RFSMNT_STR "_rfsmount"
#define NRFSMNT_STR "_nrfsmount"
#define BOOTSTATE_STR "_bootstate"
#define SEGVN_STR "_segvn_ops"


#define copyrval(STR, PTR)	pcopyrval(STR, PTR, sizeof (* (PTR)))

void exit(), perror();
extern char *malloc();
int gun = 0;
int usrid = 0;
struct vnodeops *rfs_vnodeops = NULL;
struct seg_ops *segvn_ops = NULL;
kvm_t *kd;
char *cmd;
int mypid;

struct nlist nl[] = {
	{NSNDD_STR},
	{SNDD_STR},
	{NRCVD_STR},
	{RCVD_STR},
	{RFSFST_STR},
	{RFSMNT_STR},
	{NRFSMNT_STR},
	{BOOTSTATE_STR},
	{SEGVN_STR},
	{""}
};

main(argc, argv)
int argc;
char **argv;
{
	int newfile = 0, errcnt = 0;
	register i, j, k;
	unsigned rfsmnt_adr = NULL, nrfsmount = 0;
	struct rfsmnt *rfsmount;
	unsigned rcvd_adr = NULL;
	int nrcvd = 0;
	struct rcvd *rcvd = NULL, *rdp;
	struct user *u = NULL;
	struct proc *p = NULL;
	int bootstate = 0;
	struct vnode vna, vnb;		
	int size;
	int mntpt = 0;

	/* once only code */
	if(argc < 2) {
		fprintf(stderr, "Usage:  %s { [-[ku]] file } ... \n", argv[0]);
		exit(1);
	}
	cmd = argv[0];
	mypid = getpid();

	/* open kvm stuff */
	if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, argv[0])) == NULL) {
		fprintf(stderr, "%s: can't do kvm_open\n", argv[0]);
		exit(1);
	}
	/* get values of system variables */
	if (kvm_nlist(kd, nl) == -1) {
		exit(-1);
	}
	/* Get address of segvn_ops, to look for vnodes attached via VM */
	if (copylval(SEGVN_STR, &segvn_ops)) {
		printf("%s: did not find '%s'\n", argv[0], SEGVN_STR);
		exit(1);
	}
	/* Determine if RFS is configured in and running */
	if (copyrval(BOOTSTATE_STR, &bootstate))
		bootstate = DU_DOWN;

	/* If RFS is up, read in relevant data structures */
	if (bootstate == DU_UP) {
		/* Rfs mount table */
		if (copyrval(RFSMNT_STR, &rfsmnt_adr) || 
	    	    copyrval(NRFSMNT_STR, &nrfsmount)) {
			fprintf(stderr, "%s: can't read rfs mount table\n", 
				argv[0]);
			exit(-1);
		}
		size =	nrfsmount * sizeof (struct rfsmnt);
		rfsmount = (struct rfsmnt *) malloc(size);

		if (kvm_read(kd, rfsmnt_adr, rfsmount, size) < 0) {
			fprintf(stderr, "%s: can't read rfs mount table\n", 
				argv[0]);
			exit(-1);
		}

		/* Receive descriptors */
		if (copyrval(RCVD_STR, &rcvd_adr) || 
	    	    copyrval(NRCVD_STR, &nrcvd)) {
			fprintf(stderr, "%s: can't read rcvd table\n", argv[0]);
			exit(-1);
		}
		size = nrcvd * sizeof (struct rcvd);
		rcvd = (struct rcvd *) malloc(size);

		if (kvm_read(kd, rcvd_adr, rcvd, size) < 0) {
			fprintf(stderr, "%s: can't read rfs mount table\n", 
				argv[0]);
			exit(-1);
		}

		/* RFS filesystem type v_ops vector */
		if (copylval(RFSFST_STR, &rfs_vnodeops)) {
			fprintf(stderr, "%s: can't read rfs vnodeops\n", 
				argv[0]);
			exit(-1);
		}
	}

	/* for each argunent on the command line */
	for (i = 1; i < argc; i++) {
		if(argv[i][0] == '-') {
			/* options processing */
			if(newfile) {
				gun = 0;
				usrid = 0;
				mntpt = 0;
			}
			newfile = 0;
			for(j = 1; argv[i][j] != '\0'; j++)
			switch(argv[i][j]) {
			case 'k':
				gun++; 
				break;
			case 'u':
				usrid++; 
				break;
			case 'm':
				mntpt++;
				break;
			default:
				fprintf(stderr,
					"Illegal option %c ignored.\n",
						argv[i][j]);
			}
			continue;
		} else
			newfile = 1;
		fflush(stdout);
	
		/* First print file name on stderr (so stdout (pids) can
		 * be piped to kill) */
		fprintf(stderr, "%s: ", argv[i]);
		fflush(stderr);

		/* then convert the path into a vnode */
		if (path_to_vnode(argv[i], &vna) == 0) {
			if (mntpt)
				vna.v_type = VBAD;
		} else {
			/* No such file, must be RFS resource, get root vnode */
			if (rsrc_to_vnode(argv[i], &vna, 
						rfsmount, nrfsmount) == -1) {
				fprintf(stderr," not found\n");
				errcnt++;
				continue;
			}
		}
		/* For each process check its files against the file  */
		for (kvm_setproc(kd); p = kvm_nextproc(kd); ) {
			if (p->p_stat == 0 || p->p_stat == SZOMB || 
			    p->p_stat == SIDL || p->p_pid == mypid) 
				continue;
			if ((u = kvm_getu(kd, p)) == NULL)
				continue;

			/* check current directory */
			if (!read_vnode(u->u_cdir, &vnb)) {
				if (ckuse("c", &vna, &vnb, p) == -1) {
					errcnt++;
					continue;
				}
			}	
			/* check chroot directory */
			if (!read_vnode(u->u_rdir, &vnb)) {
				if (ckuse("r", &vna, &vnb, p) == -1) {
					errcnt++;
					continue;
				}
			}
			/* Check each open file */
			for (k = 0; k < u->u_lastfile; k++) {
				if (file_to_vnode(u, k, &vnb))
					continue;
				if (ckuse("", &vna, &vnb, p) == -1) {
					errcnt++;
					break;
				}
			}
			/* Check files attached via VM */
			vm_check(p, &vna);
		}
	
		/* RFS - check receive descriptors for server holding vnodes.
		   No specific process is holding the vnode in this case. */
		for (rdp = rcvd; rdp < &rcvd[nrcvd]; rdp++) {
			if ((rdp->rd_stat & RDUNUSED) || 
			    ! (rdp->rd_qtype & GENERAL))
				continue;
			if (!read_vnode(rdp->rd_vnode, &vnb)) {
				if (ckuse("U", &vna, &vnb, NULL) == -1) {
					errcnt++;
					continue;
				}
			}	
		}
		vn_rele(&vna);
		printf("\n");
	}
	exit(errcnt);
}

/* 
 * Return the root vnode of a mounted RFS resource. The v_type field is
 * set to VBAD to flag this as a root vnode. 
 */
rsrc_to_vnode(name, vn, rfsmount, nrfsmount)
char *name;
struct vnode *vn;
struct rfsmnt *rfsmount;
int nrfsmount;
{
	int i;
	struct rfsmnt *rmp;
	char tname[MAXDNAME];

	/* search RFS mount table for the resource name. */
	for (rmp=rfsmount; rmp < &rfsmount[nrfsmount]; rmp++) {
		if(rmp->rm_flags & MINUSE) {
			if (!strncmp(name, rmp->rm_name, strlen(name))) {
				if (!read_vnode(rmp->rm_rootvp, vn)) {
					vn->v_type = VBAD;
					return(0);
				}
			}
		}
	}
	return(-1);	/* resource not found in mount table */
}


/* 
 * Return 1 for match, -1 for matched and killed, 0 for no match.
 */
int
ckuse(fc, vna, vnb, p)
char *fc;
struct vnode *vna, *vnb;
struct proc *p;
{
	struct sndd sna, snb;
	int match = 0;
	/* 
	Either vnodes are the same, or the first is the root of an
	RFS mount point and the second resides beneath that mount point. 
        Some funny things: 1) check for equality of vnodes by checking to
	see if they have the same kernel address, which is stored 
	by read_vnode() in v_pages. (Really want to do the same check 
	that the VN_CMP() op does for each filesystem type, but can't
	do that, and currently every filesystem type but RFS does vp1 == vp2.)
	2) If the first vnode corresponds to a remote RFS mount point, then 
	its type is VBAD  and just want to check that the second is under
	the same mount point 3) If both vnodes are type RFS must do the
	RFS VOP_CMP() check for equality. 
	*/
	if (vna->v_pages == vnb->v_pages)
		match = 1;
	else if (vna->v_type == VBAD && vna->v_vfsp == vnb->v_vfsp)
		match = 1;
	else if (vna->v_op == rfs_vnodeops && vnb->v_op == rfs_vnodeops) {
		if (kvm_read(kd, vna->v_data, &sna,sizeof(struct sndd)) == -1) {
			fprintf(stderr, "ckuse: can't read sndd %x\n", 
				vna->v_data);
			exit(-1);
		}
		if (kvm_read(kd, vnb->v_data, &snb,sizeof(struct sndd)) == -1) {
			fprintf(stderr, "ckuse: can't read sndd %x\n", 
				vnb->v_data);
			exit(-1);
		}
		if (sna.sd_queue == snb.sd_queue && 
		    sna.sd_sindex == snb.sd_sindex)
			match = 1;
	}
			
		
	if (match) {
 		if(fc[0] == 'U') 
			fprintf(stdout,"REMOTE USER(S)");
		if (p)
			fprintf(stdout, " %7d", (int) p->p_pid);
		fflush(stdout);
		switch(fc[0]) {
		case 'c':
		case 'p':
		case 'r':
		case 'v':
		case 'S':
			fprintf(stderr,"%c", fc[0]);
			fflush(stderr);
		}
		if(usrid) 
			puname(p);
		if((fc[0] != 'S') && (gun) && p) {
			kill((int) p->p_pid, 9);
			return(-1);
		}
	}
	return(match);
}

int vfd;	/* File descriptor associated with the acquired vnode */

path_to_vnode(path, vn)
char *path;
struct vnode *vn;
{	/* Converts a path to vnode by opening it and digging into the
           the kernel */

	char *cmd = "path_to_vnode";
	struct proc *p;
	struct user *up;
	struct file *fp;

	if ((vfd = open(path, 0, 0)) == -1) {
		return (-1);
	}
	if ((p = kvm_getproc(kd, mypid)) == NULL) {
		perror(cmd);
		exit(-1);
	}
	if ((up = kvm_getu(kd, p)) == NULL) {
		perror(cmd);
		exit(-1);
	}
	if (file_to_vnode(up, vfd, vn) != 0) {
		perror(cmd);
		exit(-1);
	}
	return(0);
}

vn_rele(vn)
struct vnode *vn;
{
	/* Release the vnode acquired by a path_to_vnode() operation.
	   Just close the associated file descriptor. */
	close(vfd);
}

puname(p)
struct proc *p;
{	
	struct passwd *getpwuid(), *pid;
	struct ucred cred;

	if (!p)
		return;
	if (kvm_read(kd, p->p_cred, &cred, sizeof (struct ucred)) == -1) {
		fprintf(stderr, "puname: can't read cred structure\n");
		exit(-1);
	}
	if ((pid = getpwuid(cred.cr_uid)) == NULL)
		return;
	fprintf(stderr, "(%s)", pid->pw_name);
}

read_vnode(vn_adr, vn)
struct vnode *vn_adr, *vn;
{	/* Takes a pointer to one of the system's vnode entries
	 * and reads the vnode into the structure pointed to by "vn".
	 * The kernel address of the vnode is stored in v_pages */

	if (!vn_adr)
		return (-1);
		
	if (kvm_read(kd, vn_adr, vn, sizeof (struct vnode)) == -1) {
		fprintf(stderr, "read_vnode: can't read vnode %x\n", vn_adr);
		exit(-1);
	}
	vn->v_pages = (struct page *) vn_adr;
	return(0);
}


copylval(symbol, ptr)
char *symbol;
unsigned *ptr;
{		/* Copies the lvalue of the UNIX symbol "symbol" into
		 * the variable pointed to by "ptr". The lvalue of
		 * "symbol" is read from SYSTEM.
		 */
	int i = 0;

	while(nl[i].n_name[0] != '\0') {
		if(!strcmp(symbol,nl[i].n_name)) {
			if(nl[i].n_value == 0) {
				return(-1);
			}
			*ptr = nl[i].n_value;
			return(0);
		}
		i++;
	}
	return(1);
}

pcopyrval(symbol, ptr, size)
char *symbol, *ptr;
int size;
{	/* Copies the rvalue of the UNIX symbol "symbol" into the structure
	 * pointed to by "ptr". The rvalue is read from memory at the location
	 * specified by the value of "symbol" in the name list file "nl_file".
	 */
	unsigned lval;

	if (copylval(symbol, &lval)) {
		return(1);
	}
	if (kvm_read(kd, lval, ptr, size) < 0) {
		fprintf(stderr, "copyrval can't read address %x\n", lval);
		exit(1);
	}
	return (0);
}


file_to_vnode(u, k, vn)
struct user *u;
int k;
struct vnode *vn;
{	/* Given the kth user file table entry in u_area u, copies
	 * the vnode  to which it refers into the area pointed to by "vn". 
	 * The vnode's kernel address is stored in v_pages. 
	 */
	struct file f;
	struct file *file_adr;
	int ret = 0;

	/* read the file table entry's address -- have to do this
	   because ofile is just a **,  not an array containing the addresses */
	if (kvm_read(kd, u->u_ofile+k, &file_adr, sizeof(struct file *)) < 0) {
	    fprintf(stderr, "file_to_vnode: can't read file addr %x\n", 
			u->u_ofile+k);
	    exit(1);
	}

	if (!file_adr)
		return(-1);

	/* read the file table entry */
	if (kvm_read(kd, file_adr, &f, sizeof (f)) < 0) {
	    fprintf(stderr, "file_to_vnode: can't read file %x\n", file_adr);
	    exit(1);
	}
	if (f.f_flag)
		return(read_vnode(f.f_data, vn));
	else 
		return(-1);
}

vm_check(p, vp)
struct proc *p;
struct vnode *vp;
{
	struct as as;
	struct seg seg, *sp;
	struct segvn_data svd;
	struct vnode vn;

	/* read the process's address space */
	if (!p->p_as)
		return;
	if (kvm_read(kd, p->p_as, &as, sizeof (as)) < 0) {
	    fprintf(stderr, "vm_check: can't read address space %x\n", p->p_as);
	    exit(1);
	}
	/* Read each segment in the address space, checking the
	   vnodes associated with those of type seg_vn. */
	sp = as.a_segs;
	do {
		if (kvm_read(kd, sp, &seg, sizeof (seg)) < 0) {
	    		fprintf(stderr, "vm_check: can't read seg %x\n", sp);
	    		exit(1);
		}
		if (seg.s_ops != segvn_ops)
			continue;
		if (kvm_read(kd, seg.s_data , &svd, sizeof (svd)) < 0) {
	    		fprintf(stderr, "vm_check: can't read svd %x\n", 
				seg.s_data);
	    		exit(1);
		}
		if (read_vnode(svd.vp, &vn))
			continue;
		if (ckuse("v", vp, &vn, p) == -1)
			return;	 /* Killed it */
	} while ((sp = seg.s_next) != as.a_segs);
}
