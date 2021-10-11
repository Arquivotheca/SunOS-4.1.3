#ifndef lint
static	char sccsid[] = "@(#)atrm.c 1.1 92/07/30 SMI"; /* from UCB 5.1 06/06/85 */
#endif
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint


/*
 *	synopsis: atrm [-f] [-i] [-] [[job #] [user] ...]
 *
 *
 *	Remove "at" jobs.
 *
 *	Author: Steve Wall
 *		Computer Systems Research Group
 *		University of California @ Berkeley
 *
 */

#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>
#include "cron.h"

extern time_t	num();
extern char	*errmsg();
extern int	errno;

#define SUPERUSER	0			/* is user super-user? */

int user;					/* person requesting removal */
int fflag = 0;					/* suppress announcements? */
int iflag = 0;					/* run interactively? */

char login[UNAMESIZE];

#define INVALIDUSER	"you are not a valid user (no entry in /etc/passwd)"
#define NOTALLOWED	"you are not authorized to use at.  Sorry."

main(argc,argv)
int argc;
char **argv;

{
	int i;				/* for loop index */
	int userid;			/* uid of owner of file */
	int isuname;			/* is a command line argv a user name?*/
	int numjobs;			/* # of jobs in spooling area */
	int usage();			/* print usage info and exit */
	int allflag = 0;		/* remove all jobs belonging to user? */
	int jobexists;			/* does a requested job exist? */
	extern int alphasort();		/* sort jobs by date of execution */
	char *pp;
	char *getuser();
	struct direct **namelist;	/* names of jobs in spooling area */
	struct stat **statlist;

	/*
	 * If job number, user name, or "-" is not specified, just print
	 * usage info and exit.
	 */
	if (argc < 2)
		usage();

	--argc; ++argv;

	pp = getuser((user=getuid()));
	if (pp == NULL)
		atabort(INVALIDUSER);
	(void) strcpy(login,pp);
	if (!allowed(login, ATALLOW, ATDENY))
		atabort(NOTALLOWED);

	/*
	 * Process command line flags.
	 * Special case the "-" option so that others may be grouped.
	 */
	while (argc > 0 && **argv == '-') {
		if (*(++(*argv)) == '\0') {
			++allflag;
		} else while (**argv) switch (*(*argv)++) {

			case 'f':	++fflag;
					break;
					
			case 'i':	++iflag;
					break;
					
			default:	usage();
		}
		++argv; --argc;
	}

	/*
	 * If all jobs are to be removed and extra command line arguments 
	 * are given, print usage info and exit.
	 */
	if (allflag && argc) 
		usage();

	/*
	 * If only certain jobs are to be removed and no job #'s or user
	 * names are specified, print usage info and exit.
	 */
	if (!allflag && !argc)
		usage();

	/*
	 * If interactive removal and quiet removal are requested, override
	 * quiet removal and run interactively.
	 */
	if (iflag && fflag)
		fflag = 0;

	/* 
	 * Get user id of person requesting removal.
	 */
	user = getuid();

	/*
	 * Move to spooling directory and get a list of the files in the
	 * spooling area.
	 */
	numjobs = getjoblist(&namelist,&statlist,alphasort);

	/*
	 * If all jobs belonging to the user are to be removed, compare
	 * the user's id to the owner of the file. If they match, remove
	 * the file. If the user is the super-user, don't bother comparing
	 * the id's. After all files are removed, exit (status 0).
	 */
	if (allflag) {
		for (i = 0; i < numjobs; ++i) { 
			if (user == SUPERUSER || user == statlist[i]->st_uid)
				(void) removentry(namelist[i]->d_name,
				    statlist[i], user);
		}
		exit(0);
	}

	/*
	 * If only certain jobs are to be removed, interpret each command
	 * line argument. A check is done to see if it is a user's name or
	 * a job number (inode #). If it's a user's name, compare the argument
	 * to the files owner. If it's a job number, compare the argument to
	 * the inode number of the file. In either case, if a match occurs,
	 * try to remove the file. (The function "isusername" scans the
	 * argument to see if it is all digits which we will assume means 
	 * that it's a job number (a fairly safe assumption?). This is done
	 * because we have to determine whether we are dealing with a user
	 * name or a job number. By assuming that only arguments that are
	 * all digits is a job number, we allow users to have digits in
	 * their login name i.e. "johndoe2").
	 */

	while (argc--) {
		jobexists = 0;
		isuname = isusername(*argv);
		for (i = 0; i < numjobs; ++i) {

			/* if the inode number is 0, this entry was removed */
			if (statlist[i]->st_ino == 0)
				continue;

			/* 
			 * if argv is a username, compare his/her uid to
			 * the uid of the owner of the file......
			 */
			if (isuname) {
				if (statlist[i]->st_uid != getid(*argv))
					continue;

			/*
			 * otherwise, we assume that the argv is a job # and
			 * thus compare argv to the inode (job #) of the file.
			 */
			} else {
				if (statlist[i]->st_ino != atoi(*argv)) 
					continue;
			}
			++jobexists;
			/*
			 * if the entry is ultimately removed, don't
			 * try to remove it again later.
			 */
			if (removentry(namelist[i]->d_name, statlist[i], user)) {
				statlist[i]->st_ino = 0;
			}
		}

		/*
		 * If a requested argument doesn't exist, print a message.
		 */
		if (!jobexists && !fflag && !isuname) {
			fprintf(stderr, "atrm: %s: no such job number\n", *argv);
		}
		++argv;
	}
	exit(0);
	/* NOTREACHED */
}

/*
 * Print usage info and exit.
 */
usage()
{
	fprintf(stderr,"usage: atrm [-f] [-i] [-] [[job #] [user] ...]\n");
	exit(1);
}

/*
 * Is a command line argument a username? As noted above we will assume 
 * that an argument that is all digits means that it's a job number, not
 * a user's name. We choose to determine whether an argument is a user name
 * in this manner because then it's ok for someone to have digits in their 
 * user name.
 */
isusername(string)
char *string;
{
	char *ptr;			/* pointer used for scanning string */

	ptr = string;
	while (isdigit(*ptr))
		++ptr;
	return((*ptr == '\0') ? 0 : 1);
}

/*
 * Remove an entry from the queue. The access of the file is checked for
 * write permission (since all jobs are mode 644). If access is granted,
 * unlink the file. If the fflag (suppress announcements) is not set,
 * print the job number that we are removing and the result of the access
 * check (either "permission denied" or "removed"). If we are running 
 * interactively (iflag), prompt the user before we unlink the file. If 
 * the super-user is removing jobs, inform him/her who owns each file before 
 * it is removed.  Return TRUE if file removed, else FALSE.
 */
int
removentry(filename,statptr,user)
char *filename;
register struct stat *statptr;
int user;
{

	if (!fflag)
		printf("%6d: ",statptr->st_ino);

	if (user != statptr->st_uid && user != SUPERUSER) {

		if (!fflag) {
			printf("permission denied\n");
		}
		return (0);

	} else {
		if (iflag) {
			if (user == SUPERUSER) {
				printf("\t(owned by ");
				powner(filename);
				printf(") ");
			}
			printf("remove it? ");
			if (!yes())
				return (0);
		}
		sendmsg(DELETE,login,filename);
		if (unlink(filename) < 0) {
			if (!fflag) {
				fputs("could not remove\n", stdout);
				(void) fprintf(stderr, "atrm: %s: %s\n",
				    filename, errmsg(errno));
			}
			return (0);
		}
		if (!fflag && !iflag)
			printf("removed\n");
		return (1);
	}
}

/*
 * Print the owner of the job. This is the owner of the spoolfile.
 * If we run into trouble getting the name, we'll just print "???".
 */
powner(file)
char *file;
{
	struct stat statb;
	char *getname();

	if (stat(file,&statb) < 0) {
		printf("%s","???");
		(void) fprintf(stderr,"atrm: Couldn't stat spoolfile %s: %s\n",
		    file, errmsg(errno));
		return(0);
	}

	printf("%s",getname(statb.st_uid));
}

/*
 * Get answer to interactive prompts, eating all characters beyond the first
 * one. If a 'y' is typed, return 1.
 */
yes()
{
	register char ch;			/* dummy variable */
	register char ch1;			/* dummy variable */

	ch = ch1 = getchar();
	while (ch1 != '\n' && ch1 != EOF)
		ch1 = getchar();
	if (isupper(ch))
		ch = tolower(ch);
	return(ch == 'y');
}

/*
 * Get the uid of a person using his/her login name. Return -1 if no
 * such account name exists.
 */
getid(name)
char *name;
{

	register struct passwd *pwdinfo;	/* password info structure */
	
	if ((pwdinfo = getpwnam(name)) == 0)
		return(-1);

	return(pwdinfo->pw_uid);
}

/*
 * Get the full login name of a person using his/her user id.
 */
char *
getname(uid)
int uid;
{
	register struct passwd *pwdinfo;	/* password info structure */
	

	if ((pwdinfo = getpwuid(uid)) == 0)
		return("???");
	return(pwdinfo->pw_name);
}

aterror(msg)
	char *msg;
{
	fprintf(stderr,"atrm: %s\n",msg);
}

atperror(msg)
	char *msg;
{
	fprintf(stderr,"atrm: %s: %s\n", msg, errmsg(errno));
}

atabort(msg)
	char *msg;
{
	aterror(msg);
	exit(1);
}

atabortperror(msg)
	char *msg;
{
	atperror(msg);
	exit(1);
}
