/*	@(#)dname.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dname:dname.c	1.10"
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <rfs/nserve.h>
#include <rfs/rfsys.h>

#define	DOMAIN	1
#define	NETSPEC	2
#define	NSNET	"/usr/nserve/netspec"

extern char *optarg;
extern int   optind;

extern char *strpbrk();
char *getdname();

main(argc, argv)
int    argc;
char **argv;
{
	int	c, error = 0;
	int	printflag = 0;
	int	setflag = 0;
	char	*newdomain, *newnetspec;
	int	rtn = 0;

	/*
	 *	Parse the command line.
	 */

	while ((c = getopt(argc, argv, "dnaD:N:")) != EOF) {
		switch (c) {
			case 'd':
				printflag |= DOMAIN;
				break;
			case 'n':
				printflag |= NETSPEC;
				break;
			case 'a':
				printflag = DOMAIN | NETSPEC;
				break;
			case 'D':
				if (setflag & DOMAIN)
					error = 1;
				else {
					setflag |= DOMAIN;
					newdomain = optarg;
				}
				break;
			case 'N':
				if (setflag & NETSPEC)
					error = 1;
				else {
					setflag |= NETSPEC;
					newnetspec = optarg;
				}
				break;
			case '?':
				error = 1;
		}
	}

	if (argc == 1)
		printflag = DOMAIN;

	if (optind < argc) {
		fprintf(stderr, "dname: extra arguments given\n");
		error = 1;
	}

	if (error) {
		fprintf(stderr, "dname: usage: dname [-D domain] [-N netspec] [-dna]\n");
		exit(1);
	}

	if (setflag & DOMAIN)
		rtn |= setdomain(newdomain);

	if (setflag & NETSPEC)
		rtn |= setnetspec(newnetspec);

	if (printflag)
		rtn |= pr_info(printflag);

	exit(rtn);
	/* NOTREACHED */
}

static
setdomain(domain)
char	 *domain;
{
	FILE *fp;
	register char *ptr;
	int  inval_chr;

	if (geteuid() != 0) {
		fprintf(stderr, "dname: must be super user to change domain name\n");
		return(1);
	}

	/*
	 *	The new domain name must between 1 and 14 characters
	 *	and must be alphanumeric, "-",  or "_".
	 */

	if (strlen(domain) > 14 || strlen(domain) < 1) {
		fprintf(stderr, "dname: new domain name must be between 1 and 14 characters in length\n");
		return(1);
	}

	inval_chr = 0;
	for (ptr = domain; *ptr; ptr++) {
		if (isalnum(*ptr) || *ptr == '_' || *ptr == '-')
			continue;
		else
			inval_chr = 1;
	}

	if (inval_chr) {
		fprintf(stderr, "dname: invalid characters specified in <%s>\n", domain);
		fprintf(stderr, "dname: domain name must contain only alpha-numeric, '-', or '_' characters\n");
		return(1);
	}

	if (rfsys(RF_SETDNAME, domain, strlen(domain)+1) < 0) {
		if (errno == EEXIST) {
			fprintf(stderr, "dname: cannot change domain name while RFS is running\n");
		} else {
			perror("dname");
		}
		return(1);
	}

	if (((fp = fopen(NSDOM,"w")) == NULL)
	  || (fputs(domain,fp) == NULL)
	  || (chmod(NSDOM, 00644) < 0)
	  || (chown(NSDOM, 0, 3) < 0)) {
		fprintf(stderr, "dname: warning: error in writing domain name into <%s>\n", NSDOM);
	}

	/*
	 *	Remove the domain password file.
	 */

	unlink(PASSFILE);
	return(0);
}

static
setnetspec(netspec)
char *netspec;
{
	FILE *fp;
	char  dname[MAXDNAME];
	char  path[BUFSIZ];
	struct stat sbuf;

	if (geteuid() != 0) {
		fprintf(stderr, "dname: must be super user to change network specification\n");
		return(1);
	}

	/*
	 *	Check to see if RFS is running.  If it is, then
	 *	the netspec cannot be changed.
	 */

	if (rfs_up() == 0) {
		fprintf(stderr, "dname: cannot change network specification while RFS is running\n");
		return(1);
	}

	if (*netspec == '\0') {
		fprintf(stderr, "dname: network specification cannot have a null value\n");
		return(1);
	}

	sprintf(path, "/dev/%s", netspec);
	if (stat(path, &sbuf) < 0) {
		fprintf(stderr, "dname: WARNING: %s does not exist\n", path);
	}

	if (((fp = fopen(NSNET,"w")) == NULL)
	  || (fputs(netspec,fp) == NULL)
	  || (chmod(NSNET, 00644) < 0)
	  || (chown(NSNET, 0, 3) < 0)
	  || (fclose(fp) < 0)) {
		fprintf(stderr, "dname: error: error in writing network specification into <%s>\n", NSNET);
		return(1);
	}
	return(0);
}

static
pr_info(flag)
int flag;
{
	int rtn = 0;

	if (flag & DOMAIN)
		rtn |= printdomain();

	if (flag & NETSPEC) {
		if (rtn == 0 && (flag & DOMAIN))
			putchar(' ');
		rtn |= printnetspec();
	}
	putchar('\n');
	return(rtn);
}

static
printdomain()
{
	char *name;
	register char *ptr;
	int  inval_chr;

	if ((name = getdname()) == NULL) {
		fprintf(stderr, "dname: domain name not set; contact System Administrator\n");
		return(1);
	}

	inval_chr = 0;
	for (ptr = name; *ptr; ptr++) {
		if (isalnum(*ptr) || *ptr == '_' || *ptr == '-')
			continue;
		else
			inval_chr = 1;
	}

	if (strlen(name) > 14 || strlen(name) < 1 || inval_chr == 1) {
		fprintf(stderr, "dname: domain name <%s> corrupted; contact System Administrator\n", name);
		return(1);
	}

	printf("%s", name);
	return(0);
}

static
printnetspec()
{
	FILE *fp;
	char netspec[BUFSIZ];

	if (((fp = fopen(NSNET,"r")) == NULL)
	|| (fgets(netspec,BUFSIZ,fp) == NULL)) {
		fprintf(stderr, "dname: network specification not set; contact System Administrator\n");
		return(1);
	}

	/*
	 *	get rid of trailing newline, if there
	 */
	if (netspec[strlen(netspec)-1] == '\n')
		netspec[strlen(netspec)-1] = '\0';

	printf("%s", netspec);
	fclose(fp);
	return(0);
}

static
char	*
getdname()
{
	static char dname[MAXDNAME];
	FILE	*fp;

	/*
	 *	If the domain name cannot be obtained from the system,
	 *	get it from the domain file.
	 */

	if ((rfsys(RF_GETDNAME, dname, MAXDNAME) < 0) || (dname[0] == '\0')) {
		if (((fp = fopen(NSDOM,"r")) == NULL)
		|| (fgets(dname,MAXDNAME,fp) == NULL))
			return(NULL);
		/*
		 *	get rid of trailing newline, if there
		 */
		if (dname[strlen(dname)-1] == '\n')
			dname[strlen(dname)-1] = '\0';
		fclose(fp);
	}
	return(dname);
}
