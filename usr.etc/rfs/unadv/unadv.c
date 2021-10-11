/*	@(#)unadv.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unadv:unadv.c	1.13"
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>
#include  <unistd.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <sys/errno.h>
#include  <rfs/nserve.h>

#define   MASK      00644
#define   BUFSIZE   512
#define   SAME      0
#define   ADVLOCK   "/etc/adv.lck"  /* lock file for /etc/advtab */

void	exit();
static	char	*resource;
static	char	*cmd;

extern	int	errno;
extern	int	ns_errno;
extern	char	*dompart();
extern	char	*namepart();

main(argc,argv)
int	argc;
char	*argv[];
{

	int	sys_err = 0;
	int	temprec;
	char	*usage = "unadv  resource";

	/*
	 *	There must be one argument, the resource
	 *	being unadvertised.
	 */

	cmd = argv[0];

	if (argc != 2) {
		fprintf(stderr,"Usage: %s\n",usage);
		exit(1);
	}
	verify_name(argv[1]);

	if (geteuid() != 0) {
		fprintf(stderr,"%s: must be super-user\n",cmd);
		exit(1);
	}

	/*
	 *	Lock a temporary file to prevent many advertises, or
	 *	unadvertises from updating "/etc/advtab" at the same time.
	 */

	if ((temprec = creat(ADVLOCK, 0600)) == -1 ||
	     lockf(temprec, F_LOCK, 0L) < 0) {
		fprintf(stderr,"%s: warning: cannot lock temp file <%s>\n",cmd,ADVLOCK);
	}

	/*
	 *	Unadvertise the resource by invoking the unadvfs() 
	 *	system call, and send resource name to the 
	 *	name server.
	 */

	if (unadvfs(resource) == -1)  {
		if (errno != ENODEV) {
			rpterr(resource);
			exit(1);
		} else
			sys_err = 1;
	}

	update_entry(resource,sys_err);

#ifdef NAMESERVER
	if (ns_unadv(argv[1]) == RFS_FAILURE) {
		if (sys_err) {
			if (ns_errno == R_PERM)
				fprintf(stderr, "%s: <%s> not advertised by this host\n", cmd, resource);
			else
				nserror(cmd);
			exit(1);
		}
	}
#endif
	exit(0);

	/* NOTREACHED */
}

static
verify_name(name)
char	*name;
{

	char	*domain;
	int	qname = 0;


	if (*(domain = dompart(name)) != '\0') {
		qname = 1;
		if (strlen(domain) > SZ_DELEMENT) {
			fprintf(stderr,"%s: domain name in <%s> exceeds <%d> characters\n",cmd,name,SZ_DELEMENT);
			exit(1);
		}

		if (v_dname(domain) != 0) {
			fprintf(stderr,"%s: domain name in <%s> contains invalid characters\n",cmd,name);
			exit(1);
		}
	}

	if (*(resource = namepart(name)) == '\0') {
		fprintf(stderr,"%s: resource name must be specified\n",cmd);
		exit(1);
	}

	if (v_resname(resource) != 0) {
		fprintf(stderr,"%s: resource name %s<%s> contains invalid characters\n",cmd,qname ? "in ":"",name);
		exit(1);
	}

	if (strlen(resource) > SZ_RES) {
		resource[SZ_RES] = '\0';
		if (qname)
			name[strlen(domain) + SZ_RES + 1] = '\0';
		else
			name[SZ_RES] = '\0';
		fprintf(stderr,"%s: warning: resource name truncated to <%s>\n",cmd,resource);
	}
}

static
update_entry(res,sys_err)
char	*res;
int	sys_err;
{

	register FILE  *fp, *fp1;
	int	found = 0;
	char	*rname;
	char	*rem;
	char	advbuf[BUFSIZE];
	struct	stat	stbuf;


	if ((fp = fopen(ADVFILE, "r")) == NULL) {
		fprintf(stderr,"%s: warning: <%s> does not exist\n",cmd,ADVFILE);
		return;
	}
	stat(ADVFILE,&stbuf);

	if ((fp1 = fopen(TEMPADV, "w")) == NULL) {
		fprintf(stderr,"%s: cannot create temporary advertise file <%s>\n",cmd,TEMPADV);
		exit(1);
	}

	/*
	 *	Update the local advertise file.
	 */

	while (fgets(advbuf,BUFSIZE,fp)) {
		rname = advbuf;
		rem = strpbrk(advbuf," ");
		*rem++ = '\0';
		rem += strspn(rem," ");
		if (strcmp(res,rname) != SAME)
			fprintf(fp1,"%s  %s",rname,rem);
		else
			found = 1;
	}

	if (!found && !sys_err)
		fprintf(stderr,"%s: warning: <%s> not in <%s>\n",cmd,res,ADVFILE);

	fclose(fp);
	fclose(fp1);
	unlink(ADVFILE);
	link(TEMPADV,ADVFILE);
	chmod(ADVFILE,MASK);
	chown(ADVFILE,(int)stbuf.st_uid,(int)stbuf.st_gid);
	unlink(TEMPADV);
}

static
ns_unadv(res)
char	*res;
{


	struct	nssend	send;
	struct	nssend	*ns_getblock();

	/*
	 *	Initialize structure with the resource name to be 
	 *	unadvertised by the name server.
	 */

	send.ns_code = NS_UNADV;
	send.ns_type = 1;
	send.ns_flag = 0;
	send.ns_name = res;
	send.ns_path = NULL;
	send.ns_desc = NULL;
	send.ns_mach = NULL;

	/*
	 *	Send the structure using the name server function
	 *	ns_getblock().
	 */

	if (ns_getblock(&send) == NULL)
		return(RFS_FAILURE);

	return(RFS_SUCCESS);
}

static
rpterr(res)
char	*res;
{

	switch (errno) {
	case EPERM:
		fprintf(stderr,"%s: must be super-user\n",cmd);
		break;
	case ENONET:
		fprintf(stderr,"%s: machine not on the network\n",cmd);
		break;
	case EINVAL:
		fprintf(stderr,"%s: invalid resource name\n",cmd);
		break;
	case EFAULT:
		fprintf(stderr,"%s: bad user address\n",cmd);
		break;
	default:
		fprintf(stderr,"%s: errno <%d>, cannot unadvertise <%s>\n",cmd,errno,res);
		break;
	}
}
