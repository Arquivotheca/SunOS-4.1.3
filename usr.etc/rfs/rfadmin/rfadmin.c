/*      @(#)rfadmin.c 1.1 92/07/30 SMI      */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rfadmin:rfadmin.c	1.3.3.1"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <rfs/rfsys.h>
#include <rfs/sema.h>
#include <rfs/rfs_misc.h>
#include <rfs/comm.h>
#include <rfs/rdebug.h>
#include <stdio.h>
#include <ctype.h>
#include <rfs/nserve.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define	EQ(s1,s2)	(strcmp(s1, s2) == 0)

#define	PRIM_RTN	1
#define	REMOVE		2
#define	ADD		4
#define	QUERY		8
#define OPTION	       16

#define MINLENGTH 	6

#define	YES		1
#define	NO		0

#define	PRIMARY		1
#define SECONDARY	2

#define SEM_FILE	"/etc/.rfadmin"

extern	char	*optarg;
extern	int	optind;
extern	int	ns_errno;

extern	char	*namepart();
extern	char	*dompart();

static	char	*cmd_name;
static	char	*getdname();

main( argc, argv )
int   argc;
char *argv[];
{
	int	flag = 0;
	int	error = 0;
	int	c;
	char	*host;
	char	*optname;

	cmd_name = argv[0];

	/*
	 *	Process arguments.
	 */

	while ((c = getopt(argc, argv, "pqa:r:o:")) != EOF) {
		switch (c) {
			case 'p':
				if (flag)
					error = 1;
				else
					flag |= PRIM_RTN;
				break;
			case 'q':
				if (flag)
					error = 1;
				else
					flag |= QUERY;
				break;
			case 'r':
				if (flag)
					error = 1;
				else {
					flag |= REMOVE;
					host = optarg;
				}
				break;
			case 'a':
				if (flag)
					error = 1;
				else {
					flag |= ADD;
					host = optarg;
				}
				break;
/*
			case 'o':
				if (flag)
					error = 1;
				else {
					flag |= OPTION;
					optname = optarg;
				}
				break;
*/
			case '?':
				error = 1;
		}
	}

	if (optind < argc) {
		fprintf(stderr, "%s: extra arguments given\n", cmd_name);
		error = 1;
	}

	if (error) {
		fprintf(stderr, "%s: usage: %s [-p]\n", cmd_name, cmd_name);
		fprintf(stderr, "                %s [-q]\n", cmd_name);
		fprintf(stderr, "                %s [-a domain.host]\n", cmd_name);
		fprintf(stderr, "                %s [-r domain.host]\n", cmd_name);
		fprintf(stderr, "                %s [-o optionname]\n", cmd_name);
		exit(1);
	}

	/*
	 *	If the "-q" option was specified, print out a message
	 *	stating if RFS is running or not (do not have to be
	 *	root in this case).
	 */

	if (flag & QUERY)
		exit(query_rtn());

	/*
	 *	Handle -o options.  Super-user restrictions must be done by
	 *	each option individually.
	 */

/*
	if (flag & OPTION)
		exit(handle_opt(optname));
*/

	/*
	 *	At this point, all other options require super-user
	 *	priviledges.
	 */

	if (geteuid() != 0) {
		fprintf(stderr, "%s: must be super user\n", cmd_name);
		exit(1);
	}

	/*
	 *	if no arguments given, print out name of primary
	 */

	if (flag == 0)
		exit(pr_primary());

	if (flag & ADD)
		exit(update_passwd(host, ADD));

	if (flag & REMOVE)
		exit(update_passwd(host, REMOVE));

	if (flag & PRIM_RTN)
		exit(primary_rtn());

	exit(0);
	/* NOTREACHED */
}

static
query_rtn()
{
	int rtn;

	if ((rtn = rfsys(RF_RUNSTATE)) < 0) {
		perror(cmd_name);
		return(2);
	}

	if (rtn == DU_UP) {
		printf("RFS is running\n");
		return(0);
	}

	printf("RFS is not currently running\n");
	return(1);
}

static
update_passwd(host, cmd)
char *host;
int   cmd;
{
	char domain[MAXDNAME], name[SZ_MACH];
	char filename[BUFSIZ];
	int sem;
	struct utsname uts;

	/*
	 *	Separate host name into machine name and domain.
	 */

	strncpy(name, namepart(host), SZ_MACH);
	name[SZ_MACH - 1] = '\0';
	strncpy(domain, dompart(host), MAXDNAME);
	domain[MAXDNAME - 1] = '\0';
	if (domain[0] == '\0' || name[0] == '\0') {
		fprintf(stderr, "%s: host name <%s> must be specified as domain.host\n", cmd_name, name);
		return(1);
	}

	/*
	 *	make sure that this machine is authorized to add or remove
	 *	hosts (i.e, it is primary name server).
	 */

	if (uname(&uts) == -1) {
		fprintf(stderr, "%s: cannot get current uname\n", cmd_name);
		return(1);
	}

	if (is_prime(uts.nodename, domain, PRIMARY) == NO) {
		fprintf(stderr, "%s: Not primary name server for the domain <%s>\n", cmd_name, domain);
		return(1);
	}

	/*
	 *	Verify that the name given is syntactically correct.
	 *	pv_[du]name prints out what's wrong withe the names
	 *	if anything.  The functions return 0 on success.
	 */

	if (pv_dname(cmd_name, domain, 0) != 0
	   || pv_uname(cmd_name, name, 0, "host") != 0) {
		return(1);
	}

	sprintf(filename, DOMPASSWD, domain);

	/*
	 *	Create and lock a temporary file.  This file will be
	 *	used as a semaphore so only one process updates the
	 *	password file at a time.
	 */

	if ((sem = creat(SEM_FILE, 0600)) == -1 ||
	     lockf(sem, F_LOCK, 0L) < 0) {
		fprintf(stderr, "%s: cannot create temp semaphore file <%s>\n", cmd_name, SEM_FILE);
		return(1);
	}

	if (cmd == ADD)
		return(add_rtn(filename, name));

	if (is_prime(name, domain, SECONDARY) == YES) {
		fprintf(stderr, "%s: removal of secondary name server <%s> disallowed\n", cmd_name, name);
		return(1);
	}

	if (is_prime(name, domain, PRIMARY) == YES) {
		fprintf(stderr, "%s: removal of primary name server disallowed\n", cmd_name);
		return(1);
	}

	if (adv_res(name) == YES)
		fprintf(stderr, "%s: warning: %s currently has resources advertised\n", cmd_name, name);

	return(remove_rtn(filename, name));
}

static
adv_res(name)
char   *name;
{
	char	cmd[512];
	FILE	*fp;
	int	rtn;

	/*
	 *	Determine if the host has resources advertised.
	 */

	sprintf(cmd, "nsquery -h %s 2>/dev/null", name);
	if ((fp = popen(cmd, "r")) == NULL)
		return(NO);

	/*
	 *	Get the first character.  This is done beacuse
	 *	a new-line will be present even if no resources
	 *	are advertised.
	 */

	fgetc(fp);
	if (fgetc(fp) == EOF)
		rtn = NO;
	else
		rtn = YES;

	pclose(fp);
	return(rtn);
}

static
add_rtn(filename, name)
char	filename[];
char	*name;
{
	FILE *fp;
	char	buf[BUFSIZ];
	char	*pw;
	char	*getnewpass();
	char	*m_name, *ptr;

	if (make_file(filename) < 0) {
		fprintf(stderr, "%s: error in creating the password file directory <%s>\n", cmd_name, filename);
		return(1);
	}

	if ((fp = fopen(filename, "a+")) == NULL) {
		fprintf(stderr, "%s: cannot open password file <%s>\n", cmd_name, filename);
		return(1);
	} else {
		/*
		 *	Check to make sure that the host is not already
		 *	in the password file.
		 */

		while(fgets(buf, sizeof(buf), fp) != NULL) {
			m_name = ptr = buf;
			while (*ptr != ':' && *ptr != '\0')
				ptr ++;
			*ptr = '\0';
			if (EQ(name, m_name)) {
				fprintf(stderr, "%s: host <%s> is already in domain\n", cmd_name, name);
				return(1);
			}
		}

		/*
		 *	Add the new entry for the specified host.
		 */

		if ((pw = getnewpass(name)) == (char *)NULL)
			return(1);

		sprintf(buf, "%s:%s\n", name, pw);
		if (fputs(buf, fp) == EOF) {
			fprintf(stderr, "%s: error in writing to password file <%s>\n", cmd_name, filename);
			return(1);
		}
		fclose(fp);
	}
	return(0);
}

static
remove_rtn(filename, name)
char	filename[];
char	*name;
{
	FILE *fp, *tempfp;
	char	buf[BUFSIZ];
	char	tempfile[BUFSIZ];
	char	tchr;
	register char *m_name, *ptr;
	int 	found = 0;

	strcpy(tempfile, filename);
	strcat(tempfile, ".t");

	if ((fp = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "%s: cannot open password file <%s>\n", cmd_name, filename);
		return(1);
	}

	if ((tempfp = fopen(tempfile, "a+")) == NULL) {
		fprintf(stderr, "%s: cannot open temporary password file\n", cmd_name);
		return(1);
	}

	/*
	 *	Go through the password file and eliminate
	 *	the entry for the specified host.
	 */

	while(fgets(buf, sizeof(buf), fp) != NULL) {
		m_name = ptr = buf;
		while (*ptr != ':' && *ptr != '\0')
			ptr ++;
		tchr = *ptr;
		*ptr = '\0';
		if (EQ(name, m_name)) {
			found = 1;
			continue;
		} else {
			*ptr = tchr;
			if (fputs(buf, tempfp) == -1) {
				fprintf(stderr, "%s: error in writing to password file <%s>\n", cmd_name, filename);
				fclose(tempfp);
				unlink(tempfile);
				return(1);
			}
		}
	}

	fclose(fp);
	fclose(tempfp);

	if (!found) {
		fprintf(stderr, "%s: host <%s> not in password file\n", cmd_name, name);
		unlink(tempfile);
		return(1);
	}

	/*
	 *	Move the temporary password file back to the real
	 *	password file.
	 */

	if (unlink(filename) == -1 || link(tempfile, filename) == -1 ||
	    unlink(tempfile) == -1 || chmod(filename, 0600) == -1) {
		fprintf(stderr, "%s: error in creating new password file <%s>\n", cmd_name, filename);
		return(1);
	}

	return(0);
}

static
is_prime(mach, domain, flag)
char	*mach;
char	*domain;
int	flag;
{
	FILE *fp;
	char buf[BUFSIZ];
	char morebuf[BUFSIZ];
	char	*dname, *cmd, *mname;
	char	*ptr;
	int i, size;
	int line = 0;

	if ((fp = fopen(NETMASTER, "r")) == NULL) {
		fprintf(stderr, "%s: cannot open master file <%s>\n", cmd_name, NETMASTER);
		return(NO);
	}

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		line ++;
		size = strlen(buf);
		while (buf[size-2] == '\\' && buf[size-1] == '\n') {
			buf[size-2] = '\0';
			if (fgets(morebuf,  sizeof(morebuf), fp) == NULL) {
				break;
			}
			if ((size = (size - 2 + strlen(morebuf))) > BUFSIZ) {
				fprintf("%s: warning: line %d of rfmaster too long\n", cmd_name, line);
				break;
			}
			strcat(buf, morebuf);
		}
		if ((dname = strtok(buf, " \t")) == NULL)
			continue;
		if ((cmd  = strtok(NULL, " \t")) == NULL)
			continue;
		if ((mname = namepart(strtok(NULL, " \t\n"))) == NULL)
			continue;

		for (i = 0; cmd[i] != '\0'; i ++)
			cmd[i] = tolower(cmd[i]);

		if (flag == PRIMARY) {
			if ((EQ(cmd, "p") || EQ(cmd, "soa"))
			&& (EQ(dname, domain) && EQ(mname, mach)))
				return(YES);
		} else {
			if ((EQ(cmd, "s") || EQ(cmd, "ns"))
			&& (EQ(dname, domain) && EQ(mname, mach)))
				return(YES);
		}
	}
	return(NO);
}

static
primary_rtn()
{
	struct nssend send;
	struct nssend *rtn;
	struct nssend *ns_getblock();

	/*
	 *	Initialize the information structure to send to the
	 *	name server.
	 */

	send.ns_code = NS_REL;
	send.ns_name = NULL;
	send.ns_flag = 0;
	send.ns_type = 0;
	send.ns_desc = NULL;
	send.ns_path = NULL;
	send.ns_addr = NULL;
	send.ns_mach = NULL;

	/*
	 *	ns_getblock withe the given structure will tell the
	 *	name server to relinquish primary responsibility.
	 */

	if ((rtn = ns_getblock(&send)) == (struct nssend *)NULL) {
		if (ns_errno == R_PERM) {
			fprintf(stderr, "%s: not currently primary name server\n", cmd_name);
			return(1);
		}
		if (ns_errno == R_NONAME) {
			fprintf(stderr, "%s: could not relinquish primary responsibilities\n", cmd_name);
			fprintf(stderr, "%s: possible cause: no secondary name servers running\n", cmd_name);
			return(2);
		}
		nserror(cmd_name);
		return(1);
	}
	return(0);
}

static
pr_primary()
{
	struct nssend send;
	struct nssend *rtn;
	struct nssend *ns_getblock();

	char   key[MAXDNAME+1];
	char  *dname = getdname();

	/*
	 *	Determine if RFS is running.
	 */

	if (rfs_up() != 0) {
		perror(cmd_name);
		return(1);
	}

	/*
	 *	Initialize the information structure to send to the
	 *	name server.
	 */

	if (dname == NULL) {
		fprintf(stderr, "%s: cannot obtain the domain name\n", cmd_name);
		return(1);
	}

	sprintf(key, "%s%s", dname, ".");
	send.ns_code = NS_FINDP;
	send.ns_name = key;
	send.ns_flag = 0;
	send.ns_type = 0;
	send.ns_desc = NULL;
	send.ns_path = NULL;
	send.ns_addr = NULL;
	send.ns_mach = NULL;

	/*
	 *	Get the name of the primary from the name server.
	 */

	if ((rtn = ns_getblock(&send)) != (struct nssend *)NULL) {
		printf("the acting name server for domain %s is %s\n", dname, *rtn->ns_mach);
		return(0);
	}

	if (ns_errno == R_INREC) {
		nserror(cmd_name);
	} else {
		fprintf(stderr, "%s: cannot obtain the name of the primary name server for domain <%s>\n", cmd_name, dname);
	}
	return(1);
}

static
char *
getdname()
{
	static char dname[MAXDNAME];
	FILE	*fp;

	/*
	 *	If the domain name cannot be obtained from the system,
	 *	get it from the domain file.
	 */

	if (rfsys(RF_GETDNAME, dname, MAXDNAME) < 0) {
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

static
make_file(filename)
char	filename[];
{
	char	buf[512];
	char	cmd[512];
	char	*ptr;
	struct stat sbuf;

	/*
	 *	This routine takes a file name of the form /x/y/z/w/v
	 *	and creates and executes the command
	 *		mkdir /x /x/y /x/y/z /x/y/z/w 2>/dev/null
	 */

	strncpy(buf, filename, sizeof(buf));
	ptr = buf;

	strcpy(cmd, "mkdir ");

	while (*ptr != '\0') {
		if (*ptr == '/') {
			*ptr = '\0';
			strcat(cmd, buf);
			strcat(cmd, " ");
			*ptr = '/';
		}
		ptr ++;
	}
	strcat(cmd, " 2>/dev/null");

	system(cmd);
	if (stat(filename, &sbuf) == -1)
		close(creat(filename, 0600));
	return(stat(filename, &sbuf));
}

static
char *
getnewpass(name)
char	 *name;
{
	char	buf[10];
	char	saltc[2];
	long	salt;
	int	flags;
	int	insist = 0;
	int	typerror = 0;
	char 	u_newpass[10];
	char	mess[80];
	int	c, i;
	char	*p;

	char	*crypt();

	for (;;) {
		if (insist >= 3) {
			fprintf(stderr, "too many failures - try again\n");
			return((char *)NULL);
		}
		if (typerror >= 3) {
			fprintf(stderr, "too many tries - try again\n");
			return((char *)NULL);
		}

		sprintf(mess, "Enter password for %s:", name);
		strncpy(u_newpass, getpass(mess), 10);

		/*
		 *	The empty string is a valid password in this
		 *	situation.
		 */

		if (u_newpass[0] == '\0') {
			sprintf(mess, "Re-enter password for %s:", name);
			strncpy(buf, getpass(mess), 10);
			if (buf[0] == '\0') {
				time(&salt);
				salt += getpid();

				saltc[0] = salt & 077;
				saltc[1] = (salt >> 6) & 077;
				for (i=0; i<2; i++) {
					c = saltc[i] + '.';
					if (c>'9') c += 7;
					if (c>'Z') c += 6;
					saltc[i] = c;
				}
				return(crypt(u_newpass, saltc));
			}
			fprintf(stderr, "They don't match; try again.\n");
			typerror ++;
			continue;
		}

		/*
	 	 *	Make sure new password is long enough
	 	 */

		if (strlen(u_newpass) < MINLENGTH) { 
			fprintf(stderr, "Password is too short - must be at least %d digits\n", MINLENGTH);
			insist ++;
			continue;
		}

		/*
		 *	Insure passwords contain at least two alpha
		 *	characters and one numeric or special character
		 */               

		flags = 0;
		p = u_newpass;
		while (c = *p++) {
			if (isalpha(c)) {
				if (flags & 1)
					flags |= 2;
				else
					flags |= 1;
			} else 
				flags |= 4;
		}

		if (flags != 7) {
			fprintf(stderr,"Password must contain at least two alphabetic characters and\n");
			fprintf(stderr,"at least one numeric or special character.\n");
			insist ++;
			continue;
		}

		/*
		 *	Insure password was typed correctly, user gets
		 *	three chances
		 */

		sprintf(mess, "Re-enter password for %s:", name);
		strncpy(buf, getpass(mess), 10);
		if (!EQ(buf, u_newpass)) {
			fprintf(stderr, "They don't match; try again.\n");
			typerror ++;
			continue;
		}

		/*
		 *	Construct salt, then encrypt the new password
		 */

		time(&salt);
		salt += getpid();

		saltc[0] = salt & 077;
		saltc[1] = (salt >> 6) & 077;
		for (i=0; i<2; i++) {
			c = saltc[i] + '.';
			if (c>'9') c += 7;
			if (c>'Z') c += 6;
			saltc[i] = c;
		}
		return(crypt(u_newpass, saltc));
	}
}

/*
 *	Handle -o options here.
 *	NOTE:  Individual options are required to validate that the user
 *	is super user, if necessary.
 */

/*
handle_opt(opt)
int	*opt;
{
	int	tmp;

	if (strcmp(opt, "loopback") == 0) {
		if (geteuid() != 0) {
			fprintf(stderr, "%s: option \"%s\": must be super user\n",cmd_name, opt);
			return(1);
		}
		rdebug(DB_LOOPBCK);
	}
	else if (strcmp(opt, "noloopback") == 0) {
		if (geteuid() != 0) {
			fprintf(stderr, "%s: option \"%s\": must be super user\n",cmd_name, opt);
			return(1);
		}
		tmp = rdebug(-2);
		rdebug(0);
		rdebug(tmp & ~DB_LOOPBCK);
	}
	else if (strcmp(opt, "loopmode") == 0) {
		if (geteuid() != 0) {
			fprintf(stderr, "%s: option \"%s\": must be super user\n",cmd_name, opt);
			return(1);
		}
		tmp = rdebug(-2); */	/* -2 returns dudebug level */
/*
		if (tmp & DB_LOOPBCK)
			printf("on\n");
		else
			printf("off\n");
	}
	else {
		fprintf(stderr,"%s: option \"%s\" not known\n",cmd_name,opt);
		return(1);
	}

	return(0);
}
*/
