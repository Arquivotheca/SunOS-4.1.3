/*	@(#)getnodes.c 1.1 92/07/30 SMI	*/

/*	Copyright (c) 1987 Sun Microsystems			*/
/*	Ported from System V.3.1				*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rmntstat:getnodes.c	1.20.3.1"

/* 
 * This module is used in the fumount, fusage and rmntstat commands.
 * (Put this module in the rfs library?)
 *
 * getnodes() retrieves the necessary tables from the Kernel,
 * builds a table of nodes (clients) that have the resource mounted
 * including block transfer counts.
 *
 * getcount() returns the block transfer count for a mounted resource.
 * It is included here because it requires the same kernel access
 * routines used by getnodes.
 */

#include <kvm.h>
#include <nlist.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <rfs/rfs_misc.h>
#include <rfs/sema.h>
#include <rfs/comm.h>
#include <rfs/fumount.h>
#include <rfs/nserve.h>
#include <ufs/mount.h>
#include <rfs/rfs_mnt.h>
#include <sys/vfs.h>
#include <sys/vfs_stat.h>
#include <sys/stat.h>
#include <rfs/adv.h>
#include <rfs/cirmgr.h>

struct nlist nl[] = {
	{ "_mounttab" },
#define X_MOUNTTAB	0
	{ "_nadvertise" },
#define X_NADVERTISE	1
	{ "_advertise" },
#define X_ADVERTISE	2
	{ "_nsrmount" },
#define X_NSRMOUNT	3
	{ "_srmount" },
#define X_SRMOUNT	4
	{ "_gdp" },
#define X_GDP		5
	{ "_maxgdp" },
#define X_MAXGDP	6
	{""}
};

void perror();
extern char *malloc();
extern struct clnts *client;

kvm_t *kd;		/* kernel descriptor for kvm calls */
int NLload;		/* nlload load flag to let nlload run only once */
int nadvertise, nsrmount, maxgdp;
/*
 * advertise, mounttab, srmount, and gdp contain the kernel virtual
 * addresses of these tables.
 */
caddr_t advertise, mounttab, srmount, gdp;
struct advertise *Advtab;
struct srmnt *Srmount;
struct gdp *Gdp;
struct rcvd Rcvd;

/*
 * getnodes builds a client table which has the block transfer count
 * of the mounted resources.
 */
getnodes(resrc, advflg)
char *resrc;
int advflg;
{	
	int clx, advx, srmx, gdpx;
	struct rcvd *rcvdp;
	struct vnode *vnop;
	sysid_t sysid;

	if (nlload() != 0)
		return (-1);

	/*
	 * We have a resource name.  The advertise table has
	 * a pointer to a receive descriptor.  The receive
	 * descriptor has a pointer to the vnode of the resource.
	 *
	 * The srmount table also has an vnode pointer.  We can
	 * scan the srmount table for instances of this vnode,
	 * pick up the sysid, and match it in the gdp structure.
	 * The gdp structure which contains (finally) the system name.
	 */

	/* find vnode pointer for this resource */
	for (advx = 0; advx < nadvertise; advx++)
		if ((strncmp(Advtab[advx].a_name, resrc, NMSZ) == 0)
	 	    && (Advtab[advx].a_flags & A_INUSE))
			break;	/* found it */
	if (advx >= nadvertise)
		return (1);	/* is this an error? */

	rcvdp = Advtab[advx].a_queue;
	if (kvm_read(kd, rcvdp, &Rcvd, sizeof (struct rcvd)) !=
	    sizeof (struct rcvd)) {
		fprintf(stderr, "could not read rcvp from kernel\n");
		return (2);
	}
	vnop = Rcvd.rd_vnode;	/* this is the thing to match */

	/* there may be mutiple srmount entries pointing to this vnode */
	for (srmx = 0, clx = 0; srmx < nsrmount; srmx++) {
		if (!(Srmount[srmx].sr_flags & MINUSE))
			continue;
		if (Srmount[srmx].sr_rootvnode == vnop) {
			sysid = Srmount[srmx].sr_sysid; /* sysid of client */
			for (gdpx = 0; gdpx < maxgdp; gdpx++) {
				if ((Gdp[gdpx].flag & GDPCONNECT)
				    && (Gdp[gdpx].sysid == sysid)) {
					/* load client list structure */
					strncpy(client[clx].node, 
					    Gdp[gdpx].token.t_uname, MAXDNAME);
					client[clx].node[MAXDNAME] = '\0';
					client[clx].sysid = sysid;
					client[clx].bcount = 
					    Srmount[srmx].sr_bcount;
					client[clx++].flags = advflg | KNOWN;
					break;
				}
			}
		}
	}
	while (clx < (nsrmount + 1))
		client[clx++].flags = EMPTY;

	if (client[0].flags == EMPTY)	/* nothing found */
		return (3);
	return (0);
}

/*
 * Return the block io count from the vfs for the file system requested.
 * mounttab[].m_vfsp->vfs; vfs_stats->vs_count[VS_READ + VS_WRITE][VS_CALL]
 */
getcount(fs)
char *fs;
{
	register struct mount *mp;
	struct mount mount;
	struct stat stb;
	struct vfs *vfsp = NULL;
	struct vfs vfs_buf;
	struct vfsstats vfsstat_buf;

	if (nlload() != 0)
		return (-1);

	if (stat(fs,&stb) == -1) {
		perror("getcount");
		return (-1);
	}
	mp = (struct mount *)mounttab;
	while (mp != NULL) {
		if (kvm_read(kd, mp, &mount, sizeof (struct mount)) !=
		    sizeof (struct mount)) {
			fprintf(stderr, "mount entry read error\n");
			return (-1);
		}
		if ((mount.m_bufp != NULL) && (mount.m_dev == stb.st_dev)) {
			vfsp = mount.m_vfsp;
			break;
		} else
			mp = mount.m_nxt;
	}
	if (vfsp == NULL) {
		fprintf(stderr, "%s is not in mounttab\n", fs);
		return (-1);
	}
	if (kvm_read(kd, vfsp, &vfs_buf, sizeof (struct vfs)) !=
	    sizeof (struct vfs)) {
		fprintf(stderr, " kernel read error on vfs\n");
		return (-1);
	}
	if (kvm_read(kd, vfs_buf.vfs_stats, &vfsstat_buf,
	    sizeof (struct vfsstats)) != sizeof (struct vfsstats)) {
		fprintf(stderr, " kernel read error on vfsstat\n");
		return (-1);
	}
	return (vfsstat_buf.vs_counts[VS_READ][VS_CALL] + 
	    vfsstat_buf.vs_counts[VS_WRITE][VS_CALL]);
}

/*
 * nlload uses kvm routines to load Advtab[] (advertise table),
 * Srmount[] (srmount table), and
 * Gdp[] (gdp table) from the kernel.  It also allocates the client
 * table which is to be filled by getnodes.
 */
nlload()
{
	int i;

	if (NLload)
		return (0);
	NLload++;	 /* once only code */

	/* initialize KVM descriptors */
	if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, NULL)) == NULL ) {
		perror("kvm_open");
		return (-1);
	}
	if (kvm_nlist(kd, nl) != 0) {
		fprintf(stderr, "symbols missing from namelist\n");
		return (-1);
	}

	/*
	 * Get advertise, mount, srmount, and gdp tables.
	 */

	/* Get Advtab[] */
	if (readsym(X_ADVERTISE, &advertise, sizeof (unsigned long)))
		return (-1);
	if (readsym(X_NADVERTISE, &nadvertise, sizeof (int)))
		return (-1);
	Advtab = (struct advertise *)
	    malloc(nadvertise * sizeof(struct advertise));
	if (Advtab) {
		if ((i=kvm_read(kd, advertise, Advtab,
		    nadvertise * sizeof(struct advertise))) !=
		    nadvertise * sizeof(struct advertise)) {
			fprintf(stderr, "could not read advertise table from kernel\n");
			return (-1);
		}
	} else {
		fprintf(stderr,"could not allocate space for advertise table\n");
		return (-1);
	}
	/* Get mounttab */
	if (readsym(X_MOUNTTAB, &mounttab, sizeof (unsigned long)))
		return (-1);

	/* Get Srmount[] */
	if (readsym(X_SRMOUNT, &srmount, sizeof (unsigned long)))
		return (-1);
	if (readsym(X_NSRMOUNT, &nsrmount, sizeof (int)))
		return (-1);
	Srmount = (struct srmnt *)malloc(nsrmount * sizeof(struct srmnt));
	if (Srmount) {
		if (kvm_read(kd, srmount, Srmount,
		    nsrmount * sizeof (struct srmnt)) !=
		    nsrmount * sizeof (struct srmnt)) {
			fprintf(stderr, "could not read srmount from kernel\n");
			return (-1);
		}
	} else {
		fprintf(stderr,"could not allocate space for srmount table\n");
		return (-1);
	}
	/* Get Gdp[] */
	if (readsym(X_GDP, &gdp, sizeof (unsigned long)))
		return (-1);
	if (readsym(X_MAXGDP, &maxgdp, sizeof (int)))
		return (-1);
	Gdp = (struct gdp *)malloc(maxgdp * sizeof(struct gdp));
	if (Gdp)  {
		if (kvm_read(kd, gdp, Gdp, maxgdp * sizeof(struct gdp)) !=
		    maxgdp * sizeof(struct gdp)) {
			fprintf(stderr, "could not read gdp from kernel\n");
			return (-1);
		}
	} else {
		fprintf(stderr,"could not allocate space for gdp table\n");
		return (-1);
	}
		/* also need space for the client list */
	client = (struct clnts *)malloc((nsrmount + 1) * sizeof(struct clnts));
	if (client == 0) {
		fprintf(stderr,"could not allocate space for gdp\n");
		return (-1);
	}
	for (i = 0; i < (nsrmount + 1); i++)
		client[i].flags = EMPTY;
	return (0);
}

/*
 * readsym gets the value of i-th symbol in namelist and puts it to
 * addr.  The size of the value is size bytes.
 * There should be a call kd = kvm_open(3) and kvm_nlist(3) before
 * readsym is called.  i is a constant indexing to the name list nl[].
 */
readsym(i, addr, size)
	int i;
	caddr_t addr;
	int size;
{
	if (nl[i].n_type == 0) {
		fprintf(stderr, "vlaue of %s is not found in the namelist\n"
		    , nl[i].n_name);
		return (-1);
	}
	if (kvm_read(kd, nl[i].n_value, addr, size) != size) {
		fprintf(stderr, "could not read %s from kernel\n",
		    nl[i].n_name);
		return (-1);
	}
	return (0);
}
