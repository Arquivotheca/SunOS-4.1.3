/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acctdusg.c 1.1 92/07/30 SMI" /* from S5R3 acct:acctdusg.c 1.4 */
/*
 *	disk [-u file] [-p file] > dtmp-file
 *	-u	file for names of files not charged to anyone
 *	-p	get password info from file
 *	reads input (normally from find / -print)
 *	and compute disk resource consumption by login
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#define NAMESZ 8
 
struct	disk{
	char	dsk_name[NAMESZ];	/* login name */
	unsigned dsk_uid;		/* user id of login name */
	int	dsk_dirsz;	/* # letters in pathname of login directory */
	char	*dsk_logdir;	/* ptr to path name of login directory */
	int	dsk_ns;			/* no of slashes in path name */
	long	dsk_du;			/* disk usage */
	struct	disk *dsk_np;		/* ptr to next struct */

};
char	*pfile = NULL;
char	*afile, *nfile;
 
struct disk	*Dp;	/* ptr to 1st entry in dirlist */
struct stat	statb;

FILE	*pswd, *nchrg;
FILE	*names ={stdin};
FILE	*acct  ={stdout};

char	fbuf[BUFSIZ];
char	*calloc();

struct passwd *getpwent(), *fgetpwent();

main(argc, argv)
char	**argv;

{
	register struct passwd *pw;

	while(--argc > 0){
		++argv;
		if(**argv == '-') switch((*argv)[1]) {
		case 'u':
			if (--argc <= 0)
				break;
			if ((nchrg = fopen(*(++argv),"w")) == NULL)
				openerr(*argv);
			chmod(*argv, 0644);
			continue;
		case 'p':
			if (--argc <= 0)
				break;
			pfile = *(++argv);
			continue;
		}
		fprintf(stderr,"Invalid argument: %s\n", *argv);
		exit(1);
	}
	if (pfile != NULL) {
		if ((pswd = fopen(pfile, "r")) == NULL)
			openerr(pfile);
	}

	while ((pw = (pfile == NULL ? getpwent() : fgetpwent(pswd))) != NULL) {
		makdlst(pw);	/* make a list of home directory names
				for every entry in password file */
	}
	if (pfile != NULL)
		fclose(pswd);

	dsort();

	while( fgets(fbuf, sizeof fbuf, names) != NULL) {
		fbuf[strndx(fbuf, '\n') ] = '\0';
		clean(fbuf);
		charge(fbuf);
	}
	if (names != stdin)
		fclose(names);

	output();

	if (acct != stdout)
		fclose(acct);
	if (nchrg)
		fclose(nchrg);
#ifdef DEBUG
		pdisk();
#endif

	exit(0);
}
openerr(file)
char	*file;
{
	fprintf(stderr, "Cannot open %s\n", file);
	exit(1);
}

output()
{

	register struct disk *dp;

	for(dp = Dp; dp != NULL; dp=dp->dsk_np) {
		if(dp->dsk_du)
			fprintf(acct,
				"%05u\t%-8.8s\t%7lu\n",
				dp->dsk_uid,
				dp->dsk_name,
				dp->dsk_du);
	}
}

strndx(str, chr)
register char *str;
register char chr;
{

	register index;

	for (index=0; *str; str++,index++)
		if (*str == chr)
			return index;
	return -1;
}

/*
 *	make a list of home directory names
 *	for every entry in password file
 */

makdlst(pw)
register struct passwd *pw;
{

	static struct	disk *dl = {NULL};
	struct disk	*dp;
	int    i;

	if( (dp = (struct disk *)calloc(sizeof(struct disk), 1)) == NULL) {
	nocore:
		fprintf(stderr, "out of core\n");
		exit(2);
	}
	strncpy(dp->dsk_name, pw->pw_name, NAMESZ);
	dp->dsk_uid = pw->pw_uid;

	dp->dsk_dirsz = strlen(pw->pw_dir); /* length of path name */
	if((dp->dsk_logdir = calloc(dp->dsk_dirsz + 1, 1)) == NULL)
		goto nocore;

	strcpy(dp->dsk_logdir, pw->pw_dir, dp->dsk_dirsz);

	if(stat(dp->dsk_logdir,&statb)== -1 ||
			(statb.st_mode & S_IFMT) != S_IFDIR) {
		cfree(dp->dsk_logdir);
		cfree(dp);
		return;
	}
	for(i=0; dp->dsk_logdir[i]; i++)
		if(dp->dsk_logdir[i] == '/')
			dp->dsk_ns++; /* count # of slashes */

	if(dl == NULL) { /* link ptrs */
		Dp = dl = dp;
	} else {
		dl->dsk_np = dp;
		dl = dp;
	}
	return;
}

/*
 *	sort by decreasing # of levels in login
 *	pathname and then by increasing uid
 */
dsort()
{

	register struct disk *dp1, *dp2, *pdp;
	int	change;

	if(Dp == NULL || Dp->dsk_np == NULL)
		return;

	change = 0;
	pdp = NULL;

	for(dp1 = Dp; ;) {
		if((dp2 = dp1->dsk_np) == NULL) {
			if(!change)
				break;
			dp1 = Dp;
			pdp = NULL;
			change = 0;
			continue;
		}
		if((dp1->dsk_ns < dp2->dsk_ns) ||
		   (dp1->dsk_ns==dp2->dsk_ns && dp1->dsk_uid > dp2->dsk_uid)) {
			swapd(pdp, dp1, dp2);
			change = 1;
			dp1 = dp2;
			continue;
		}
		pdp = dp1;
		dp1 = dp2;
	}
}

swapd(p,d1,d2)

register struct disk *p, *d1, *d2;
{
	struct disk *t;

	if (p != NULL) {
		p->dsk_np = d2;
		t = d2->dsk_np;
		d2->dsk_np = d1;
		d1->dsk_np = t;
	} else {
		t = d2->dsk_np;
		d2->dsk_np = d1;
		d1->dsk_np = t;
		Dp = d2;
	}
}

charge(n)
register char *n;
{
	register struct disk *dp;
	register i;
	long	blks;

	if(stat(n,&statb) == -1)
		return;

	i = strlen(n);
	for(dp = Dp; dp != NULL; dp = dp->dsk_np) {
		if(i < dp->dsk_dirsz)
			continue;
		if(strncmp(dp->dsk_logdir, n, dp->dsk_dirsz) == 0 &&
		   (n[dp->dsk_dirsz] == '/' || n[dp->dsk_dirsz] == '\0'))
			break;
	}

	blks = statb.st_blocks;
	if(dp == NULL) {
		if(nchrg && (statb.st_size) &&
		   (statb.st_mode&S_IFMT) == S_IFDIR)
			fprintf(nchrg, "%5u\t%7lu\t%s\n",
			statb.st_uid, blks, n);
		return;
	}

	dp->dsk_du += (statb.st_mode&S_IFMT) == S_IFDIR ? blks
		:((statb.st_mode&S_IFMT)==S_IFREG ?
			(blks / statb.st_nlink) : 0L);
}

#ifdef DEBUG
pdisk()
{
	register struct disk *dp;

	for(dp=Dp; dp!=NULL; dp=dp->dsk_np)
		printf("%.8s\t%5u\t%7lu\t%5u\t%5u\t%s\n",
			dp->dsk_name,
			dp->dsk_uid,
			dp->dsk_du,
			dp->dsk_dirsz,
			dp->dsk_ns,
			dp->dsk_logdir);
}
#endif

clean(p)
register char *p;
{
	register char *s1, *s2;

	for(s1=p; *s1; ) {
		s2 = s1;
		while(*s1 == '/')
			s1++;
		s1 = s1<= s2 ? s2 : s1-1;
		if(s1 != s2) {
			strcpy(s2,s1);
			s1 = s2;
		}
		if(*++s2 == '.') 
		switch(*++s2) {
		case '/':
			strcpy(s1,s2);
			continue;
		case '.':
			if(*++s2 != '/')
				break;
			if(s1 > p)
				while(*--s1 != '/' && s1 > p)
					;
			strcpy(s1,s2);
			continue;
		}
		while(*s2 && *++s2 != '/')
			;
		s1 = s2;
	}
	return;
}
