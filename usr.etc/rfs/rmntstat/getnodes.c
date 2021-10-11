/*	@(#)getnodes.c 1.1 92/07/30 SMI	*/
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rmntstat:getnodes.c	1.20.4.1"
		/* this module is used in the fumount, fusage
		   and rmntstat  commands */

		/* getnodes() retrieves the necessary tables from the Kernel,
		   builds a table of nodes (clients) that have the
		   resource mounted including block transfer counts */

		/* getcount() returns the block transfer count for a
		   mounted resource.  It is included here because it
		   requires the same kernel access routines used by getnodes */

#include <nlist.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/sema.h>
#include <sys/comm.h>
#include "fumount.h"
#include <sys/nserve.h>
#include <sys/cirmgr.h>
#include <sys/idtab.h>
#include <sys/var.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/adv.h>

/* symbol names */
#define V_STR "v"
#define MAXADV_STR "nadvertise"
#define ADVTAB_STR "advertise"
#define NRCVD_STR "nrcvd"
#define RCVD_STR "rcvd"
#define MNT_STR "mount"
#define NSRMNT_STR "nsrmount"
#define SRMNT_STR "srmount"
#define GDP_STR "gdp"
#define MGDP_STR "maxgdp"

struct nlist nl[] = {
	{V_STR},
	{MAXADV_STR},
	{ADVTAB_STR},
	{NRCVD_STR},
	{RCVD_STR},
	{MNT_STR},
	{NSRMNT_STR},
	{SRMNT_STR},
	{GDP_STR},
	{MGDP_STR},
	{""}
};

#if u3b15 || u3b2
#define MEMF "/dev/kmem"
#endif

#define SYSTEM "/unix"
#define NLDATA "/etc/nl_data"

void perror();
char nullptr[] = "";
extern char *malloc();
extern struct clnts *client;

int mem;
int NLload;			/* nlist load flag */
int maxadv, nsrmnt, gdpsiz;
unsigned v_adr, adv_adr, maxadv_adr;
unsigned mnt_adr, srmnt_adr, nsrmnt_adr, gdp_adr, mgdp_adr;
struct var v;
struct advertise *Advtab;
struct mount *Mount;
struct srmnt *Srmount;
struct rcvd Rcvd;
struct gdp *gdpp;

getnodes(resrc, advflg)
char *resrc;
int advflg;
{	
	int clx, advx, srmx, gdpx;
	struct rcvd *rcvdp;
	struct inode *inop;
	sysid_t sysid;

	if(nlload() != 0)
		return(-1);

/*	we have a resource name.  The advertise table has
**	a pointer to a receive descriptor.  The receive
**	descriptor has a pointer to the inode of the resource.
**
**	The srmount table also has an inode pointer.  We can
**	scan the srmount table for instances of this inode,
**	pick up the sysid, and match it in the gdp structure.
**	The gdp structure which contains (finally) the system name.
*/
		/* find inode pointer for this resource */
	for(advx = 0; advx < maxadv; advx++)
		if((strncmp(Advtab[advx].a_name, resrc, NMSZ) == 0)
	 	&& (Advtab[advx].a_flags & A_INUSE))
			break;	/* found it */
	if(advx >= maxadv) {
		return(1);	/* is this an error? */
	}

	rcvdp = Advtab[advx].a_queue;
	if(rread(mem, rcvdp, &Rcvd, sizeof(struct rcvd)))
		return(2);
	inop = Rcvd.rd_inode;	/* this is the thing to match */

			/* there may be mutiple srmount entries pointing
				to this inode */

	for(srmx = 0, clx = 0; srmx < nsrmnt; srmx++) {
		if(!(Srmount[srmx].sr_flags & MINUSE))
			continue;
		if(Srmount[srmx].sr_rootinode == inop) {
			sysid = Srmount[srmx].sr_sysid; /* sysid of client */
			for(gdpx = 0; gdpx < gdpsiz; gdpx++) {
				if((gdpp[gdpx].flag & GDPCONNECT)
				&& (gdpp[gdpx].sysid == sysid)) {
						/* load client list structure */
					strncpy(client[clx].node, 
				gdpp[gdpx].token.t_uname, 
						MAXDNAME);
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
	while(clx < (nsrmnt + 1))
		client[clx++].flags = EMPTY;

	if(client[0].flags == EMPTY)	/* nothing found */
		return(3);
	return(0);
}

getcount(fs)
char *fs;
{
	struct mount *mp;
	struct stat stb;
	int i;

			/* return the block io count from the kernel
			   mount table for the file system requested */

	if(nlload() != 0)
		return(-1);

	if(stat(fs,&stb) == -1) {
		perror("getcount");
		return(-1);
	}
	for (mp = Mount, i = 0; i < v.v_mount; mp++, i++) {
		if (mp->m_flags & MINUSE) {
			if(mp->m_dev == stb.st_dev) {
				return(mp->m_bcount);
			}
		}
	}
	return(-1);
}

nlload()
{
	int i, nlfd;
	struct stat nls, uxs;

	if(NLload)
		return(0);

	/* once only code */
	NLload++;

	/* open file to access memory */
	if((mem = open(MEMF, O_RDONLY)) == -1) {
		perror(MEMF);
		return(-1);
	}
	/* get values of system variables */

	if((stat(NLDATA,&nls) < 0)
	|| (stat(SYSTEM,&uxs) < 0)
	|| (uxs.st_mtime > nls.st_mtime)) {
getnl:
		if(nlist(SYSTEM, nl) == -1) {
			perror("nlist:");
			return(-1);
		}
		if((nlfd = open(NLDATA, O_WRONLY | O_CREAT)) >= 0 ) {
			chmod(NLDATA,0664);
			for(i = 0; i < sizeof(nl) / sizeof(struct nlist); i++)
				write(nlfd, &nl[i].n_value, sizeof(long));
			close(nlfd);
		}
	} else {
		if((nlfd = open(NLDATA, O_RDONLY)) >= 0) {
			for(i = 0; i < sizeof(nl) / sizeof(struct nlist); i++)
				if(read(nlfd, &nl[i].n_value, sizeof(long))
						!= sizeof(long))
					goto getnl;
			close(nlfd);
		} else {
			goto getnl;
		}
	}

	if(copylval(V_STR, &v_adr)
	|| copylval(MAXADV_STR, &maxadv_adr)
	|| copylval(GDP_STR, &gdp_adr)
	|| copylval(MGDP_STR, &mgdp_adr)
	|| copylval(ADVTAB_STR, &adv_adr)
	|| copylval(MNT_STR, &mnt_adr)
	|| copylval(NSRMNT_STR, &nsrmnt_adr)
	|| copylval(SRMNT_STR, &srmnt_adr))
		return(-1);

	if(rread(mem, v_adr, &v, sizeof(struct var)))
		return(-1);

	/* get space for advertise, mount, srmount, and gdp tables */

	if(rread(mem, maxadv_adr, &maxadv, sizeof(maxadv)))
		return(-1);
	Advtab = (struct advertise *)malloc(maxadv * sizeof(struct advertise));
	if(Advtab) {
		if(rread(mem,adv_adr,Advtab,maxadv * sizeof(struct advertise)))
			return(-1);
	} else {
		fprintf(stderr,"could not allocate space for advertise table\n");
		return(-1);
	}
	Mount = (struct mount *)malloc(v.v_mount * sizeof(struct mount));
	if(Mount)  {
		if(rread(mem,mnt_adr,Mount,v.v_mount * sizeof(struct mount)))
			return(-1);
	} else {
		fprintf(stderr,"could not allocate space for mount table\n");
		return(-1);
	}
	if(rread(mem, nsrmnt_adr, &nsrmnt, sizeof(nsrmnt)))
		return(-1);
	Srmount = (struct srmnt *)malloc(nsrmnt * sizeof(struct srmnt));
	if(Srmount)  {
		if(rread(mem,srmnt_adr,Srmount,nsrmnt * sizeof(struct srmnt)))
			return(-1);

	} else {
		fprintf(stderr,"could not allocate space for srmount table\n");
		return(-1);
	}
	if(rread(mem, mgdp_adr, &gdpsiz, sizeof(maxgdp)))
		return(-1);
	gdpp = (struct gdp *)malloc(gdpsiz * sizeof(struct gdp));
	if(gdpp)  {
		if(rread(mem, gdp_adr, gdpp, gdpsiz * sizeof(struct gdp)))
			return(-1);
	} else {
		fprintf(stderr,"could not allocate space for gdp table\n");
		return(-1);
	}
		/* also need space for the client list */
	client = (struct clnts *)malloc((nsrmnt + 1) * sizeof(struct clnts));
	if(client == 0) {
		fprintf(stderr,"could not allocate space for gdp\n");
		return(-1);
	}
	for(i = 0; i < (nsrmnt + 1); i++)
		client[i].flags = EMPTY;
	return(0);
}

/*	copylval(), and rread() were stolen from fuser.c */

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
				fprintf(stderr,"copylval: '%s' is undefined\n",symbol);
				return(-1);
			}
			*ptr = nl[i].n_value;
			return(0);
		}
		i++;
	}
	fprintf(stderr,"copylval cannot find '%s'\n",symbol);
	return(-1);
}

rread(device, position, buffer, count)
char *buffer;
int count, device;
long position;
{	/* Seeks to "position" on device "device" and reads "count"
	 * bytes into "buffer". Zeroes out the buffer on errors.
	 */
	int i;
	long lseek();
	static int p1 = 0;

	if(lseek(device, position, 0) == (long) -1) {
		fprintf(stderr, "Seek error in %s ",MEMF);
		perror("");
		for(i = 0; i < count; buffer++, i++) *buffer = '\0';
		return(-1);
	}
	if(read(device, buffer, (unsigned) count) == -1) {
		fprintf(stderr, "Read error in %s ",MEMF);
		perror("");
		for(i = 0; i < count; buffer++, i++) *buffer = '\0';
		return(-1);
	}
	return(0);
}
