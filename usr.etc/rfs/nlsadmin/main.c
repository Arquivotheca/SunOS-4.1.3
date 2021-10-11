/*	@(#)main.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlsadmin:main.c	1.18.2.1"

#include "sys/types.h"
#include "sys/param.h"
#include "tiuser.h"
#include "sys/stat.h"
#include "dirent.h"
#include "stdio.h"
#include "ctype.h"
#include "signal.h"
#include "string.h"
#include "fcntl.h"
#include "nlsadmin.h"
#include "errno.h"
#include "pwd.h"
#include "grp.h"

char    *arg0;
int Errno;

char serv_str[16];
char id_str[MAXPATHLEN];
char res_str[MAXPATHLEN];
char flag_str[10];
char mod_str[MAXPATHLEN];
char path_str[MAXPATHLEN];
char cmnt_str[MAXPATHLEN];

char homedir[MAXPATHLEN];
char tempfile[MAXPATHLEN];

#ifdef S4
static char *tempname = "/usr/tmp/.nlstmp";
#else
static char *tempname = "/.nlstmp";
#endif

FILE *tfp;
int nlsuid, nlsgid;
int uid;
char *Netspec;

extern char *erropen;
extern char *errclose;
extern char *errmalloc;
extern char *errchown;
extern char *errscode;
extern char *errperm;
extern char *errdbf;
extern char *errsvc;
extern char *errarg;
extern char *init[];

extern char *malloc();


main(argc, argv)
int   argc;
char *argv[];
{
	register int     flag = 0;
	register int     c;
	register char    *net_spec = NULL;
	register char    *serv_code = NULL;
	int     qflag = 0;
	int     nflag = 0;
	int     errflg = 0;
	int     adrflg = 0;
	int	mflag = 0;
	char    *laddr = NULL;
	char    *taddr = NULL;
	char	*id = NULL;
	char	*res = NULL;
	char    *pathname = NULL;
	char    *command = NULL;
	char    *comment = NULL;
	char	*modules = NULL;
	struct passwd *pwdp;
	struct group *grpp;
	extern struct passwd *getpwnam();
	extern struct group *getgrnam();
	extern void exit(), endpwent(), endgrent();

#ifdef	S4
	char	*starname = NULL;
	char	*pstarname = NULL;
	extern char *nlsname();
	extern int _nlslog;
#endif

	arg0 = argv[0];
	Errno = NLSOK;

	if (argc == 1)
		usage();

	uid = getuid();
	if (!(grpp = getgrnam(LSGRPNAME)))  {	/* get group entry	*/
		fprintf(stderr, "%s: no group entry for %s?\n",arg0,LSGRPNAME);
		exit(NLSNOGRP);
	}
	nlsgid = grpp->gr_gid;
	endgrent();

	/* get password entry for name LSUIDNAME.  homedir = LSUIDNAME's home */

	if (!(pwdp = getpwnam(LSUIDNAME)) || !(strlen(pwdp->pw_dir)))  {
		fprintf(stderr,"%s: could not find correct password entry for %s\n",arg0,LSUIDNAME);
		exit(NLSNOPASS);
	}
	nlsuid = pwdp->pw_uid;
	strcpy(homedir, HOMEDIR);
	endpwent();

	umask(022);

	/*
	 *      Process arguments.
	 */

#ifndef	S4
	while ((c = getopt(argc, argv, "a:c:d:e:ikl:mnp:qr:st:vw:xy:z:")) != EOF) {
#else
	while ((c = getopt(argc, argv, "a:c:d:e:ikl:mnqr:st:vw:xy:z:S:")) != EOF) {
#endif
		switch (c) {
			case 'a':
				flag |= ADD;
				if (*optarg != '-')
					serv_code = optarg;
				break;
			case 'c':
				flag |= ADD;
				if (*optarg != '-')
					command = optarg;
				break;
			case 'd':
				flag |= DISBL;
				if (*optarg != '-')
					serv_code = optarg;
				break;
			case 'e':
				flag |= ENABL;
				if (*optarg != '-')
					serv_code = optarg;
				break;
			case 'i':
				flag |= INIT;
				break;
			case 'k':
				flag |= KILL;
				break;
			case 'l':
				flag |= ADDR;
				adrflg |= LISTEN;
				if (*optarg != '-')
					laddr = optarg;
				else if (strlen(optarg) != 1)
					errflg = 1;
				break;
			case 'm':
				mflag = 1;
				break;
			case 'n':
				nflag = 1;
				break;
#ifndef	S4
			case 'p':
				flag |= ADD;
				if (*optarg != '-')
					modules = optarg;
				break;
#endif
			case 'q':
				qflag = 1;
				break;
			case 'r':
				flag |= REM;
				if (*optarg != '-')
					serv_code = optarg;
				break;
			case 's':
				flag |= START;
				break;
			case 't':
				flag |= ADDR;
				adrflg |= TTY;
				if (*optarg != '-')
					taddr = optarg;
				else if (strlen(optarg) != 1)
					errflg = 1;
				break;
			case 'v':
				flag |= VERBOSE;
				break;
			case 'w':
				flag |= ADD;
				if (*optarg != '-')
					id = optarg;
				break;
			case 'x':
				flag |= ALL;
				break;
			case 'y':
				flag |= ADD;
				if (*optarg != '-')
					comment = optarg;
				break;
			case 'z':
				flag |= ZCODE;
				if (*optarg != '-')
					serv_code = optarg;
				break;
#ifdef S4
			case 'S':
				flag |= STARLAN;
				if (*optarg != '-')
					starname = optarg;
				break;
#endif
			case '?':
				errflg = 1;
		}
	}
	if (errflg)
		usage();
	if (optind < argc)
		Netspec = net_spec = argv[optind++];
	if (optind < argc)
		usage();

	/*
	 * validate service code, if given.
	 * code follows algorithm in listen:lsdbf.c:get_dbf().
	 */

	if (serv_code) {
		c = strlen(serv_code);
		if ((c == 0) || (c >= SVC_CODE_SZ))
			error(errsvc, NLSSERV);
		if ((c < 5) && isnum(serv_code)) {
			c = atoi(serv_code);
			if (c == 0)
				error(errsvc, NLSSERV);
			if (flag == ADD) {
				if (mflag) {
					if (c > N_DBF_ADMIN)
						error(errsvc, NLSSERV);
				} else {
					if (c < MIN_REG_CODE)
						error(errsvc, NLSSERV);
				}
			}
		} else {
			if ((flag == ADD) && mflag)
				error(errsvc, NLSSERV);
		}
	}

	if (!net_spec) {
		if (flag != ALL)
			usage();
	} else {
		if (!ok_netspec(net_spec))
			usage();
		if ((pathname = (char *)malloc(strlen(homedir)+strlen(net_spec)+strlen(DBFILE)+3)) == NULL)
			error(errmalloc, NLSMEM);
		sprintf(pathname, "%s/%s/%s", homedir, net_spec, DBFILE);
	}

	if (net_spec) {
#ifdef S4
		(void) strcpy(tempfile, tempname);
#else
		(void) strcpy(tempfile, homedir);
		(void) strcat(tempfile, "/");
		(void) strcat(tempfile, net_spec);
		(void) strcat(tempfile, tempname);
#endif
	}

	if ((flag != ALL) && (flag != INIT) && check_version(pathname))
		error("database file is not current version, please convert", NLSVER);

	switch (flag) {
		case ALL:
			if (mflag || qflag || nflag || adrflg || net_spec)
				usage();
			print_all(homedir);
			exit(Errno);
		case INIT:
			if (mflag || qflag || nflag || adrflg || !net_spec)
				usage();
			if (uid != 0)
				error(errperm, NLSPERM);
			if (access(pathname, 0) < 0) {
/*
				if (strchr(net_spec, '\n'))
					error(errarg, NLSCMD);
*/
				if (!make_db(pathname, net_spec))
					error("error in making database file", Errno);
			} else {
				error("net_spec already initialized",NLSINIT);
			}
			exit(NLSOK);
		case ADD:
			if (qflag || nflag || adrflg || !net_spec)
				usage();
			if (!serv_code || !command || !comment)
				usage();
			if (uid != 0)
				error(errperm, NLSPERM);
/*
			if (strchr(serv_code, '\n') || strchr(modules, '\n') ||
			    strchr(command, '\n') || strchr(comment, '\n') ||
			    strchr(id, '\n') || strchr(res, '\n'))
				error(errarg, NLSCMD);
*/
			if (!add_nls(pathname,serv_code,id,res,modules,command,comment,mflag))
				error("could not add service", Errno);
			break;
		case REM:
			if (mflag || qflag || nflag || adrflg || !net_spec)
				usage();
			if (!serv_code)
				usage();
			if (uid != 0)
				error(errperm, NLSPERM);
			if (!rem_nls(pathname, serv_code))
				error("could not remove service",Errno);
			break;
		case ENABL:
			if (mflag || qflag || nflag || adrflg || !net_spec)
				usage();
			if (!serv_code)
				usage();
			if (uid != 0)
				error(errperm, NLSPERM);
			if (!enable_nls(pathname, serv_code))
				error("could not enable service",Errno);
			break;
		case DISBL:
			if (mflag || qflag || nflag || adrflg || !net_spec)
				usage();
			if (!serv_code)
				usage();
			if (uid != 0)
				error(errperm, NLSPERM);
			if (!disable_nls(pathname, serv_code))
				error("could not disable service",Errno);
			break;
		case START:
			if (mflag || qflag || adrflg || !net_spec)
				usage();
			if (uid != 0)
				error(errperm, NLSPERM);
			start_nls(net_spec, nflag);
			/* will exit by itself */
			break;
		case KILL:
			if (mflag || qflag || nflag || adrflg || !net_spec)
				usage();
			if (uid != 0)
				error(errperm, NLSPERM);
			if (!kill_nls(net_spec))
				exit(Errno);
			exit(NLSOK);
		case NONE:
			if (mflag || nflag || adrflg || !net_spec)
				usage();
			if (qflag)
				exit(!isactive(net_spec));
			else
				printf("%s\t%s\n",net_spec,isactive(net_spec)?"ACTIVE":"INACTIVE");
			exit(NLSOK);
		case VERBOSE:
			if (mflag || qflag || nflag || adrflg || !net_spec)
				usage();
			print_spec(pathname);
			exit(NLSOK);
		case ADDR:
			if (mflag || qflag || nflag || !net_spec)
				usage();
			if ((laddr || taddr) && (uid != 0))
				error(errperm, NLSPERM);
/*
			if (strchr(laddr, '\n') || strchr(taddr, '\n'))
				error(errarg, NLSCMD);
*/
			if (!chg_addr(net_spec, laddr, taddr, adrflg))
				error("error in accessing address file",Errno);
			exit(NLSOK);
#ifdef S4
		case STARLAN:
			if (mflag || qflag || nflag || adrflg || !net_spec)
				usage();
			if (!starname)
				usage();
			if (uid != 0)
				error(errperm, NLSPERM);
			/*
			 * Nlsname returns null if pstarname will be longer 
			 * than NAMEBUFSZ (currently 64)
			 */

			_nlslog = 1;	/* allows nlsname to use stderr */
			if (!(pstarname = nlsname(starname)))
				error("error permuting name", NLSADRF);
			if (!chg_addr(net_spec, pstarname, starname, LISTEN|TTY))
				error("error in accessing address file", Errno);
			exit(NLSOK);
#endif
		case ZCODE:
			if (mflag || nflag || adrflg || !net_spec)
				usage();
			if (!serv_code)
				usage();
			print_serv(pathname, serv_code, qflag);
			/* will exit by itself */
			break;
		default:
			usage();
	}
	exit(Errno);
	/* NOTREACHED */
}


#ifdef S4

static char *usagemsg = "\
%s: usage: %s -x  |  %s [ options ] net_spec\n\
\twhere [ options ] are as follows:\n\n\
status:		[-q]  |  [-v]  |\n\
start/stop:	[-s -n]  |  [-k]  |\n\
initialize:	[-i]  | \n\
set addresses:	[ [-l address|-]  [-t address|-] ]  |\n\
		[-S STARLAN_NETWORK_name]  |\n\
query servers:	[[-q] -z service_code]  |\n\
add server:	[ [-m] -a service_code -c \"command\" [-w id] -y \"comment\" ]  |\n\
remove server:	[-r service_code]  |\n\
enable/disable:	[-d service_code]  |  [-e service_code]\n\
";

#else

static char *usagemsg = "\
%s: usage: %s -x   or   %s [ options ] net_spec\n\
\twhere [ options ] are:\n\
[-q]  |  [-v]  |  [-s]  |  [-k]  |  [-i]  | \n\
[ [-l address|-]  [-t address|-] ]  |\n\
[ [-m] -a svc_code -c \"cmd\" -y \"comment\"  [-p module_list] [-w id]]  |\n\
[[-q] -z svc_code]  |  [-r svc_code]  |  [-d svc_code]  |  [-e svc_code]\n\
";

#endif


usage()
{
	fprintf(stderr, usagemsg, arg0, arg0, arg0);
	exit(NLSCMD);
}


error(s, ec)
char *s;
int ec;
{

	fprintf(stderr, "%s: %s\n", arg0, s);
	exit(ec);
}


/*
 * add an entry to the database, res is reserved for future use, ignore for now
 */

add_nls(fname, serv, id, res, mods, path, comm, flag)
char *fname;
char *serv;
char *id;
char *res;
char *mods;
register char *path;
char *comm;
int flag;
{
	register FILE *fp = NULL;
#ifndef S4
	struct stat sbuf;
#endif


	for (; *path ; path++)		/* clean up beginning white space */
		if (!isspace(*path))
			break;
	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, erropen, arg0, Netspec);
		Errno = NLSOPEN;
		return(0);
	}

	if (strlen(serv) > (sizeof(serv_str)-1))
		error("service code too long", NLSSERV);
	if (find_serv(fp, serv) != -1) {
		fprintf(stderr,"%s: service code %s already exists in %s\n",arg0,serv,fname);
		Errno = NLSEXIST;
		return(0);
	}
	(void) fclose(fp);
	if ((fp = fopen(fname, "a")) == NULL) {
		fprintf(stderr, erropen, arg0, Netspec);
		Errno = NLSOPEN;
		return(0);
	}
	if (!id)
		id = DEFAULTID;
	if (mods)
		fprintf(fp,"%s:%s:%s:reserved:NULL,%s,:%s\t#%s\n",serv,flag?"na":"n",id,mods,path,comm);
	else
		fprintf(fp,"%s:%s:%s:reserved:NULL,:%s\t#%s\n",serv,flag?"na":"n",id,path,comm);
	if (fclose(fp) != 0) {
		fprintf(stderr,errclose,arg0,fname,errno);
		Errno = NLSCLOSE;
		return(0);
	}

#ifndef S4

	(void) strtok(path, DBFWHITESP);

	if (access(path, 0) == 0) {
		(void) stat(path, &sbuf);
		if (!(sbuf.st_mode & 0111))
			fprintf(stderr,"%s: warning - %s not executable\n", arg0, path);
		if (!(sbuf.st_mode & S_IFREG))
			fprintf(stderr, "%s: warning - %s not a regular file\n", arg0, path);
	} else {
		fprintf(stderr,"%s: warning - %s not found\n", arg0, path);
	}

#endif

	return(1);
}


rem_nls(fname, serv_code)	       /* remove an entry from the database */
char *fname;
char *serv_code;
{
	register FILE *fp;
	int n;

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, erropen, arg0, Netspec);
		Errno = NLSOPEN;
		return(0);
	}
	if ((n = find_serv(fp, serv_code)) == -1) {
		fprintf(stderr,errscode, arg0, serv_code, fname);
		Errno = NLSNOCODE;
		return(0);
	}
	if (!open_temp())
		return(0);
	if (n != 0)
		copy_file(fp, 0, n-1);
	copy_file(fp, n+1, -1);
	if (!close_temp(fname))
		return(0);
	return(1);
}


enable_nls(fname, serv_code)	    /* enable a specific service code */
char *fname;
char *serv_code;
{
	register FILE *fp;
	register char *to, *from;
	register char *limit;
	char *bp, *copy;
	int n;

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, erropen, arg0, Netspec);
		Errno = NLSOPEN;
		return(0);
	}
	if ((n = find_serv(fp, serv_code)) == -1) {
		fprintf(stderr,errscode,arg0,serv_code,fname);
		Errno = NLSNOCODE;
		return(0);
	}
	if (!open_temp())
		return(0);
	if (n != 0)
		copy_file(fp, 0, n-1);
	else
		fseek(fp, 0L, 0);
	if ((bp = (char *)malloc(DBFLINESZ)) == NULL)
		error(errmalloc, NLSMEM);
	if ((copy = (char *)malloc(DBFLINESZ)) == NULL)
		error(errmalloc, NLSMEM);
	if (!fgets(bp,DBFLINESZ,fp)) {
		(void) unlink(tempfile);
		error(errdbf, NLSRDFIL);
	}
	limit = strchr(bp, (int)':');
	for (to = copy, from = bp; from < limit; )
		*to++ = *from++;
	*to++ = *from++;	/* copy ":" */
	limit = strchr(from, (int)':');
	for ( ; from < limit ; ) {
		if (*from == 'x' || *from == 'X') {
			from++;
			continue;
		}
		*to++ = *from++;
	}
	for ( ; *from ; )
		*to++ = *from++;
	*to = '\0';
	fprintf(tfp, "%s", copy);
	free(bp);
	free(copy);
	copy_file(fp, n+1, -1);
	(void) fclose(fp);
	if (!close_temp(fname))
		return(0);
	return(1);
}


disable_nls(fname, serv_code)	   /* disable a specific service code */
char *fname;
char *serv_code;
{
	register FILE *fp;
	register char *to, *from;
	register char *limit;
	char *bp, *copy;
	int n;

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, erropen, arg0, Netspec);
		Errno = NLSOPEN;
		return(0);
	}
	if ((n = find_serv(fp, serv_code)) == -1) {
		fprintf(stderr,errscode,arg0,serv_code,fname);
		Errno = NLSNOCODE;
		return(0);
	}
	if (!open_temp())
		return(0);
	if (n != 0)
		copy_file(fp, 0, n-1);
	else
		fseek(fp, 0L, 0);
	if ((bp = (char *)malloc(DBFLINESZ)) == NULL)
		error(errmalloc, NLSMEM);
	if ((copy = (char *)malloc(DBFLINESZ)) == NULL)
		error(errmalloc, NLSMEM);
	if (!fgets(bp,DBFLINESZ,fp)) {
		(void) unlink(tempfile);
		error(errdbf, NLSRDFIL);
	}
	limit = strchr(bp, (int)':');
	for (to = copy, from = bp; from < limit; )
		*to++ = *from++;
	*to++ = *from++;	/* copy ":" */
	limit = strchr(from, (int)':');
	for ( ; from < limit ; ) {
		if (*from == 'x' || *from == 'X') {
			from++;
			continue;
		}
		*to++ = *from++;
	}
	*to++ = 'x';
	for ( ; *from ; )
		*to++ = *from++;
	*to = '\0';
	fprintf(tfp, "%s", copy);
	free(bp);
	free(copy);
	copy_file(fp, n+1, -1);
	(void) fclose(fp);
	if (!close_temp(fname))
		return(0);
	return(1);
}


isdir(s)		/* check to see if a directory exists */
char *s;
{
	struct stat sbuf;

	if (stat(s, &sbuf) != 0) {
		fprintf(stderr, "%s: could not stat %s, errno = %d\n",arg0,s,errno);
		Errno = NLSSTAT;
		return(0);
	}
	if ((sbuf.st_mode & FILETYPE) == DIRECTORY)
		return(1);
	else
		return(0);
}


isnum(s)		/* check to see if s is a numeric string */
register char *s;
{
	if (!*s)
		return(0);
	while(*s)
		if (!isdigit(*s++))
			return(0);
	return(1);
}


isactive(name)	  /* is a listener active? */
char *name;
{
	char lockname[MAXPATH];
	int fd;
	struct stat sbuf;

	sprintf(lockname, "%s/%s", homedir, name);
	if (access(lockname, 0) < 0) {
		fprintf(stderr, erropen, arg0, name);
		exit(NLSNOTF);
	}
	strcat(lockname, "/");
	strcat(lockname, LOCKFILE);

	/* check to see if modes are okay */

	if (stat(lockname, &sbuf) != 0) {
		/* Errno = NLSSTAT */
		return(0);
	}
	if ((sbuf.st_mode & ALLPERM) != LOCKPERM) {
		if (uid == 0 || uid == nlsuid)
			chmod(lockname, LOCKPERM);
		else {
			fprintf(stderr,"%s: bad modes for file %s - retry as super user\n",arg0,lockname);
			exit(NLSPERM);
		}
	}

	/* open file with open(2) */

	if ((fd = open(lockname, O_RDWR)) < 0) {
		/* Errno = NLSOPEN */
		return(0);
	}
	if (LOCK(fd, 2, 0L) == -1) {
		close(fd);
		return(1);
	}
	close(fd);
	return(0);
}


make_db(fname, nspec)	   /* create the database file */
char *fname;
char *nspec;
{
	register FILE *fp = NULL;
	register FILE *afp = NULL;
	register int i;
	register char *ptr, *savptr;
	char addrname[MAXPATH];
	char newname[MAXPATH];
	char parent[MAXPATH];

	/* assume homedir already exists */

	/* make directory(s) for net_spec */

	umask(0);
	sprintf(addrname, "%s/%s", homedir, nspec);
	strcpy(parent, homedir);
	ptr = addrname+strlen(homedir)+1;
	do {
		savptr = ptr;
		ptr = strchr(ptr, (int)'/');
		if (ptr == savptr || !ptr)
			i = strlen(addrname);
		else
			i = ptr - addrname;
		strncpy(newname, addrname, i);
		newname[i] = (char)0;
		if ((access(newname,0) < 0) && (errno == ENOENT)){
			if (mkdir(newname, 0777) < 0) {
				fprintf(stderr, "%s: could not create %s\n",
					arg0, newname);
				perror("mkdir");
				Errno = NLSCREAT;
				return(0);
			}
		}
		strcpy(parent, newname);
	} while ((ptr != savptr) && (ptr++ != (char *)NULL));
	umask(022);

	/* create files */

	strcat(addrname, "/");
	strcat(addrname, ADDRFILE);
	if ((fp = fopen(fname, "w")) == NULL) {
		fprintf(stderr,"%s: could not create %s, errno = %d\n",arg0,fname,errno);
		Errno = NLSCREAT;
		return(0);
	}
	if ((afp = fopen(addrname, "w")) == NULL) {
		fprintf(stderr,"%s: could not create %s, errno = %d\n",arg0,addrname,errno);
		Errno = NLSCREAT;
		return(0);
	}

	init_db(fp);

	if (fclose(fp) != 0) {
		fprintf(stderr,errclose,arg0,fname,errno);
		Errno = NLSCLOSE;
		return(0);
	}
	if (chown(fname, nlsuid, nlsgid) < 0) {
		fprintf(stderr,errchown,arg0, fname, errno);
		Errno = NLSCHOWN;
		return(0);
	}
	fprintf(afp,"\n\n");
	if (fclose(afp) != 0) {
		fprintf(stderr,errclose,arg0,addrname,errno);
		Errno = NLSCLOSE;
		return(0);
	}
	if (chown(addrname, nlsuid, nlsgid) < 0) {
		fprintf(stderr,errchown,arg0, addrname, errno);
		Errno = NLSCHOWN;
		return(0);
	}
	umask(0);
	sprintf(addrname, "%s/%s/%s", homedir, nspec, LOCKFILE);
	if ((fp = fopen(addrname, "w")) == NULL) {
		fprintf(stderr,"%s: could not create %s, errno = %d\n",arg0,addrname,errno);
		Errno = NLSCREAT;
		return(0);
	}
	if (fclose(fp) != 0) {
		fprintf(stderr,errclose,arg0,addrname,errno);
		Errno = NLSCLOSE;
		return(0);
	}
	if (chown(addrname, nlsuid, nlsgid) < 0) {
		fprintf(stderr,errchown,arg0, addrname, errno);
		Errno = NLSCHOWN;
		return(0);
	}
	umask(022);
	return(1);
}


chg_addr(fname, la, ta, flag)
char *fname;
char *la;
char *ta;
register int flag;
{
	register FILE *fp = NULL;
	char addrname[MAXPATH];
	char adr1[NAMEBUFSZ];
	char adr2[NAMEBUFSZ];
	char *p;

	adr1[0] = (char)0;
	adr2[0] = (char)0;
	sprintf(addrname, "%s/%s/%s", homedir, fname, ADDRFILE);
	if ((fp = fopen(addrname, "r")) == NULL) {
		fprintf(stderr, erropen, arg0, Netspec);
		Errno = NLSOPEN;
		return(0);
	}
	(void) fgets(adr1, NAMEBUFSZ, fp);
	(void) fgets(adr2, NAMEBUFSZ, fp);
	if (ferror(fp)) {
		fprintf(stderr,"%s: error in reading address file\n",arg0);
		(void) fclose(fp);
		Errno = NLSADRF;
		return(0);
	}
	(void) fclose(fp);
	if (adr1[0] == (char)0)
		adr1[0] = '\n';
	if (adr2[0] == (char)0)
		adr2[0] = '\n';
	if(p = strrchr(adr1, (int)'\n'))
		*p = (char)0;
	if(p = strrchr(adr2, (int)'\n'))
		*p = (char)0;
	fp = NULL;

/*
 * check to make sure the addresses are different before we zap the addr file
 * 3 cases exist, we are changing both, just la, or just ta
 */

	if ((la && ta && !strcmp(la, ta)) || (la && !ta && !strcmp(la, adr2)) || (!la && ta && !strcmp(adr1, ta))) {
		fprintf(stderr, "%s: listening addresses not unique\n", arg0);
		Errno = NLSNOTUNIQ;

/*
 * this little piece allows stuff like "nlsadmin -l addr -t- netspec"
 * to still print out the 'other' address even if the update fails.  Note
 * that fp will be NULL after this.
 */

		if (la) {
			flag &= ~LISTEN;
			la = NULL;
		}
		if (ta) {
			flag &= ~TTY;
			ta = NULL;
		}
	}
	if (!la) {
		if (flag & LISTEN)
			printf("%s\n",adr1);
	} else {
		if ((fp = fopen(addrname, "w")) == NULL) {
			fprintf(stderr, erropen, arg0, Netspec);
			Errno = NLSOPEN;
			return(0);
		}
		fprintf(fp, "%s\n", la);
	}
	if (!ta) {
		if (flag & TTY) {
			if (adr2[0] != (char)0)
				printf("%s\n",adr2);
			else
				fprintf(stderr, "Terminal login service address not configured\n");
		}
		if (fp)
			fprintf(fp, "%s\n", adr2);
	} else {
		if (!fp) {
			if ((fp = fopen(addrname, "w")) == NULL) {
				fprintf(stderr, erropen, arg0, Netspec);
				Errno = NLSOPEN;
				return(0);
			}
			fprintf(fp, "%s\n", adr1);
		}
		fprintf(fp, "%s\n", ta);
	}
	if (fp) {
		if (fclose(fp) != 0) {
			fprintf(stderr,errclose,arg0,addrname,errno);
			Errno = NLSCLOSE;
			return(0);
		}
		if (chown(addrname, nlsuid, nlsgid) < 0) {
			fprintf(stderr,errchown,arg0, addrname, errno);
			Errno = NLSCHOWN;
			return(0);
		}
	}
	if (Errno == NLSNOTUNIQ)
		return(0);
	else
		return(1);
}


init_db(fp)	     /* initialize the database file */
FILE *fp;
{
	register int i;

	for ( i=0; init[i]; i++)
		fprintf(fp,"%s\n",init[i]);
}


copy_file(fp, start, finish)
register FILE *fp;
register int start;
register int finish;
{
	register int i;
	char dummy[DBFLINESZ];

	fseek(fp,0L,0);
	if (start != 0) {
		for (i = 0; i < start; i++)
			if (!fgets(dummy,DBFLINESZ,fp)) {
				(void) unlink(tempfile);
				error(errdbf, NLSRDFIL);
			}
	}
	if (finish != -1) {
		for (i = start; i <= finish; i++) {
			if (!fgets(dummy, DBFLINESZ, fp)) {
				(void) unlink(tempfile);
				error(errdbf, NLSRDFIL);
			}
			if (fputs(dummy,tfp) == EOF) {
				(void) unlink(tempfile);
				error("error in writing tempfile", NLSDBF);
			}
		} /* for */
	} else {
		for ( ; ; ) {
			if (fgets(dummy, DBFLINESZ, fp) == NULL) {
				if (feof(fp)) {
					break;
				} else {
					(void) unlink(tempfile);
					error(errdbf, NLSRDFIL);
				}
			}
			if (fputs(dummy,tfp) == EOF) {
				(void) unlink(tempfile);
				error("error in writing tempfile", NLSDBF);
			}
		} /* for */
	} /* if */
}


find_pid(name)
char *name;
{
	char pidname[MAXPATH];
	char pidchar[10];
	FILE *fp;

	sprintf(pidname, "%s/%s/%s", homedir, name, PIDFILE);
	if ((fp = fopen(pidname, "r")) == NULL)
		return(0);
	if (fscanf(fp, "%s", pidchar) != 1) {
		(void) fclose(fp);
		return(0);
	}
	(void) fclose(fp);
	return((int)strtol(pidchar, (char **)NULL, 10));
}


start_nls(name, flag)
char *name;
int flag;
{
	register char *lstr, *tstr;
	char addrname[MAXPATH];
	char buff[100];
	FILE *fp;
	struct netbuf *lbuf = NULL;
	struct netbuf *tbuf = NULL;
	int taddr = 0;	/* true if remote tty address specified */
	extern struct netbuf *stoa();
	extern void nlsaddr2c();

	if (isactive(name))
		error("listener already active",NLSACTIV);

	if (flag)
		sprintf(buff, "%s/%s/%s", homedir, name, LISTENER);
	else
		sprintf(buff, "%s/%s", homedir, LISTENER);

	/* read addr file */

	sprintf(addrname, "%s/%s/%s", homedir, name, ADDRFILE);
	if ((fp = fopen(addrname, "r")) == NULL) {
		fprintf(stderr, erropen, arg0, Netspec);
		exit(NLSOPEN);
	}
	if ((lstr = (char *)malloc(NAMEBUFSZ))==NULL)
		error(errmalloc, NLSMEM);
	if ((tstr = (char *)malloc(NAMEBUFSZ))==NULL)
		error(errmalloc, NLSMEM);
	*lstr = (char)0;
	*tstr = (char)0;
	(void) fgets(lstr, NAMEBUFSZ, fp);
	(void) fgets(tstr, NAMEBUFSZ, fp);
	if (*lstr == '\n') {
		fprintf(stderr,"%s: address not initialized for %s\n",arg0,name);
		(void) fclose(fp);
		exit(NLSADRF);
	}
	if (*lstr == (char)0 || *tstr == (char)0) {
		fprintf(stderr,"%s: error in reading, or bad address file\n",arg0);
		(void) fclose(fp);
		exit(NLSADRF);
	}
	*(lstr + strlen(lstr) -1) = (char)0;
	*(tstr + strlen(tstr) -1) = (char)0;
	if (*tstr)
		taddr = 1;

	/* call stoa - convert from rfs address to netbuf */

	if ((lbuf = stoa(lstr, lbuf)) == NULL)
		error(errmalloc, NLSMEM);
	if (taddr) {
		if ((tbuf = stoa(tstr, tbuf)) == NULL)
			error(errmalloc, NLSMEM);
	}

	/* call nlsaddr2c - convert from netbuf to listener format */

	free(lstr);
	free(tstr);
	if ((lstr = (char *)malloc(NAMEBUFSZ*2+1))==NULL)
		error(errmalloc, NLSMEM);
	if (taddr) {
		if ((tstr = (char *)malloc(NAMEBUFSZ*2+1))==NULL)
			error(errmalloc, NLSMEM);
	}
	nlsaddr2c(lstr, lbuf->buf, lbuf->len);
	if (taddr)
		nlsaddr2c(tstr, tbuf->buf, tbuf->len);

	(void) fclose(fp);
	if (taddr)
		execl( buff, LISTENER, "-n", name, "-l", lstr, "-r", tstr, 0);
	else
		execl( buff, LISTENER, "-n", name, "-l", lstr, 0);
	error("could not exec listener", NLSEXEC ); 
}


kill_nls(name)
char *name;
{
	int pid;

	if (!isactive(name)) {
		fprintf(stderr,"%s: there is no listener active for %s\n",arg0,name);
		Errno = NLSDEAD;
		return(0);
	}
	if((pid = find_pid(name)) == 0) {
		fprintf(stderr, "%s: could not find pid to send SIGTERM\n",arg0);
		Errno = NLSFINDP;
		return(0);
	}
	if (kill(pid, SIGTERM) != 0) {
		fprintf(stderr, "%s: could not send SIGTERM to listener, errno = %d\n",arg0,errno);
		Errno = NLSSIG;
		return(0);
	}
	return(1);
}


close_temp(fname)
char *fname;
{
	if (fclose(tfp) == EOF) {
		fprintf(stderr,errclose,arg0,errno);
		Errno = NLSCLOSE;
		(void) unlink(tempfile);
		return(0);
	}
	if (unlink(fname)) {
		fprintf(stderr,"%s: cannot unlink %s, errno = %d\n", arg0, fname,errno);
		Errno = NLSULINK;
		(void) unlink(tempfile);
		return(0);

	}
	if (link(tempfile, fname)) {
		fprintf(stderr,"%s: cannot link %s to %s, errno = %d\n", arg0, tempfile, fname,errno);
		Errno = NLSLINK;
		(void) unlink(tempfile);
		return(0);
	}
	if (unlink(tempfile)) {
		fprintf(stderr,"%s: cannot unlink %s, errno = %d\n",arg0,tempfile,errno);
		Errno = NLSULINK;
		return(0);
	}
	if (chown(fname, nlsuid, nlsgid) < 0) {
		fprintf(stderr,errchown,arg0, fname, errno);
		Errno = NLSCHOWN;
		return(0);
	}
	return(1);
}


open_temp()
{
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	umask(0333);
	if (access(tempfile,0) >= 0) {
		fprintf(stderr, "%s: tempfile busy; try again later\n", arg0);
		Errno = NLSBUSY;
		return(0);
	}
	if ((tfp = fopen(tempfile, "w")) == NULL) {
		fprintf(stderr, "%s: cannot create tempfile, errno = %d\n", arg0,errno);
		Errno = NLSCREAT;
		return(0);
	}
	umask(022);
	return(1);
}


find_serv(fp, code)	     /* find a service routine */
FILE *fp;
char *code;
{
	register int n;
	register int total = -1;
	extern int read_dbf();
/*
 *	read_dbf() returns a line number relative to the file pointer.
 *	total is the absolute file offset.
 */

	while ((n = read_dbf(fp)) >= 0) {
		if (total == -1)
			total = n;
		else
			total = total + n + 1;
		if (strncmp(serv_str, code, SVC_CODE_SZ) == 0)
			return(total);
	}
	if (n == -2)
		error("error in reading database file", Errno);
	return(-1);
}


print_spec(fname)
char *fname;
{
	FILE *fp;
	register int n;
	int ignore;

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, erropen, arg0, Netspec);
		Errno = NLSOPEN;
		return;
	}
	while ((n = read_dbf(fp)) >= 0) {
		ignore = (int)strchr(flag_str, (int)'x') +
			 (int)strchr(flag_str, (int)'X');

#ifdef S4

		printf("%s\t%s\t%s\t%s\t%s\n",serv_str, ignore?"DISABLED":"ENABLED ",id_str,path_str,cmnt_str);

#else

		if (mod_str[0] == (char)0)
			printf("%s\t%s\t%s\tNOMODULES\t%s\t%s\n",serv_str, ignore?"DISABLED":"ENABLED ",id_str,path_str,cmnt_str);
		else
			printf("%s\t%s\t%s\t%s\t%s\t%s\n",serv_str, ignore?"DISABLED":"ENABLED ",id_str,mod_str,path_str,cmnt_str);

#endif /* S4 */

	}
	if (n == -2)
		error("error in reading database file", Errno);
	(void) fclose(fp);
}


print_all(dir)		/*  CAUTION: recursive routine */
char *dir;
{
	register char dirname[MAXNAMLEN];
	char *nspec;
	struct dirent *dp;
	DIR *dfp;

	if ((dfp = opendir(dir)) == (DIR *) NULL)
		error(errdbf, NLSRDFIL);
	while ((dp = readdir(dfp)) != (struct dirent *) NULL) {
		sprintf(dirname,"%s/%s",dir,dp->d_name);
		if (!strcmp(dp->d_name, ".") || ! strcmp(dp->d_name, ".."))
			continue;
		if (isdir(dirname)) {
			print_all(dirname);
		} else {
			if (strcmp(dp->d_name, DBFILE) == 0) {
				nspec = dir + strlen(homedir) + 1;
				printf("%s\t%s\n",nspec,isactive(nspec)?"ACTIVE":"INACTIVE");
			}
		}
	}
	closedir(dfp);
}


print_serv(fname, code, flag)
char *fname;
char *code;
int flag;
{
	register FILE *fp;
	register int ignore;

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, erropen, arg0, Netspec);
		exit(NLSOPEN);
	}
	if (find_serv(fp, code) == -1) {
		fprintf(stderr,errscode, arg0, code, fname);
		exit(NLSNOCODE);
	}
	(void) fclose(fp);
	ignore = (int)strchr(flag_str, (int)'x') + (int)strchr(flag_str, (int)'X');
	if (ignore)
		ignore = 1;
	if (flag)
		exit(ignore);

#ifdef S4

		printf("%s\t%s\t%s\t%s\t%s\n",serv_str, ignore?"DISABLED":"ENABLED ",id_str,path_str,cmnt_str);

#else

		if (mod_str[0] == (char)0)
			printf("%s\t%s\t%s\tNOMODULES\t%s\t%s\n",serv_str, ignore?"DISABLED":"ENABLED ",id_str,path_str,cmnt_str);
		else
			printf("%s\t%s\t%s\t%s\t%s\t%s\n",serv_str, ignore?"DISABLED":"ENABLED ",id_str,mod_str,path_str,cmnt_str);

#endif /* S4 */

	exit(NLSOK);
}


ok_netspec(nspec)		/* validates a net_spec name */
char *nspec;
{
	register int size;
	register char *cp, *n;

	if (!nspec)
		return(0);
	if (*nspec == '/')
		return(0);
	for (cp = nspec; *cp; cp += size) {
		if (*cp == '.')
			return(0);
		if ((n = strchr(cp, (int)'/')) == NULL)
			size = strlen(cp);
		else
			size = n - cp + 1;
	}
	return(1);
}
