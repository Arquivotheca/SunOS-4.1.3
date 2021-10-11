#ifndef lint
static char sccsid[] = "@(#)mknetid.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif
/*
 * Copyright (C) 1990, Sun Microsystems, Inc.
 */

/*
 * Network name to unix credential database generator.
 * Uses /etc/passwd, /etc/group, /etc/hosts and /etc/netid to
 * create the database.
 *
 * If some user appears in passwd, they get an entry like:
 *	sun.<uid>@<domainname>	<uid>:<gid1>,<gid2>,...
 * If some host appears in hosts, it gets an entry like:
 *	sun.<hostname>@<domainname>	0:<hostname>
 *
 * The file /etc/netid is used to add other domains (possibly non-unix)
 * to the database.
 */
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <sys/param.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>


#define MAXNAMELEN 256
#define MAXLINELEN 1024
#define MAXDOMAINLEN 32

#define GRPTABSIZE	256		/* size of group table */
#define PRNTABSIZE 4096		/* size of printed item table */

#define NUMGIDS	(NGROUPS + 1)	/* group-access-list + gid */

extern char **getline();
extern char *malloc();
extern char *strcpy();

/*
 * The group list
 * Store username and groups to which he/she belongs
 */
struct group_list {
	char *user;
	int group_len;
	int groups[NUMGIDS];
	struct group_list *next;
};

/*
 * General purpose list of strings
 */
struct string_list {
	char *str;
	struct string_list *next;
};

static FILE *openfile();
static char *scanargs();
static int atoi();

static char *cmdname;	/* name of this program */
static int quietmode;	/* quiet mode: don't print error messages */
static char *curfile;	/* name of file we are parsing */
static int curline;		/* current line parsed in this file */

static struct group_list *groups[GRPTABSIZE];	/* group table */
static struct string_list *printed[PRNTABSIZE];	/* printed item table */
static char domain[MAXDOMAINLEN];	/* name of our domain */

static char PASSWD[] = "/etc/passwd";	/* default passwd database */
static char IDMAP[] = "/etc/idmap";		/* default net-id map database */
static char GROUP[] = "/etc/group";		/* default group database */
static char HOSTS[] = "/etc/hosts";		/* default hosts database */

static char *pwdfile = PASSWD;	/* password file to parse */
static char *grpfile = GROUP;	/* group file */
static char *hostfile = HOSTS;	/* hosts file */
static char *mapfile = IDMAP;	/* network id file */

/*
 * Various separaters
 */
static char WHITE[] = "\t ";	
static char COLON[] = ":";
static char COMMA[] = ",";

main(argc, argv)
	int argc;
	char *argv[];
{
	FILE *pf, *mf, *gf, *hf;

	cmdname = argv[0];
	if (!parseargs(argc, argv)) {
		(void) fprintf(stderr,
			"usage: %s [-q] [-pghm filename]\n", cmdname);
		exit(1);
	}
	(void) getdomainname(domain, sizeof(domain));

	pf = openfile(pwdfile);
	gf = openfile(grpfile);
	hf = openfile(hostfile);
	mf = fopen(mapfile, "r");


	if (mf != NULL) {
		domapfile(mapfile, mf);
	}
	dogrpfile(grpfile, gf);
	dopwdfile(pwdfile, pf);
	dohostfile(hostfile, hf);

	exit(0);
	/* NOTREACHED */
}

/*
 * Parse the network id mapping file
 */
domapfile(mapfile, mf)
	char *mapfile;
	FILE *mf;
{
	char **lp;
	char line[MAXLINELEN];
	char name[MAXNAMELEN];
	int uid, gid;

	curfile = mapfile;
	curline = 0;
	while (lp = getline(line, sizeof(line), mf, &curline, "#")) {
		check_getname(lp, name, WHITE, WHITE, "#");
		if (wasprinted(name)) {
			multdef(name);
			continue;
		}
		put_s(name); 
		(void) putchar(' ');
		check_getname(lp, name, WHITE, COLON, "#");
		uid = Atoi(name);
		put_d(uid); 
		(void) putchar(':');
		if (uid == 0) {
			check_getname(lp, name, WHITE, "\t\n ", "#");
			put_s(name); 
			(void) putchar(' ');
		} else {
			check_getname(lp, name, WHITE, ",\n", "#");
			gid = Atoi(name);
			put_d(gid);
			while (getname(name, sizeof(name), WHITE, ",\n", lp, 
				       "#") >= 0) {
				gid = Atoi(name);
				(void) putchar(','); 
				put_d(gid);
			}
		}
		(void) putchar('\n');
	}
}


/*
 * Parse the groups file
 */
dogrpfile(grpfile, gf)
	char *grpfile;
	FILE *gf;
{
	char **lp;
	char line[MAXLINELEN];
	char name[MAXNAMELEN];
	int gid;

	curfile = grpfile;
	curline = 0;
	while (lp = getline(line, sizeof(line), gf, &curline, "")) {
		check_getname(lp, name, WHITE, COLON, "");
		if (name[0] == '+') {
			continue;
		}
		check_getname(lp, name, WHITE, COLON, "");	/* ignore passwd */
		check_getname(lp, name, WHITE, COLON, "");
		gid = Atoi(name);
		while (getname(name, sizeof(name), WHITE, COMMA, lp, 
			       "") >= 0) {
			storegid(gid, name);
		}
	}
}


/*
 * Parse the password file 
 */
dopwdfile(pwdfile, pf)
	char *pwdfile;
	FILE *pf;
{
	char **lp;
	char line[MAXLINELEN];
	char name[MAXNAMELEN];
	char user[MAXNAMELEN];
	int uid, gid;

	curfile = pwdfile;
	curline = 0;

	while (lp = getline(line, sizeof(line), pf, &curline, "")) {
		check_getname(lp, user, WHITE, COLON, ""); 	/* username */
		if (user[0] == '-' || user[0] == '+') {
			continue;	/* NIS entry */
		}
		check_getname(lp, name, WHITE, COLON, ""); /* ignore passwd */
		check_getname(lp, name, WHITE, COLON, ""); /* but get uid */
		uid = Atoi(name);
		user2netname(name, uid, domain);
		if (wasprinted(name)) {
			multdef(name);
			continue;
		}
		put_s(name); 
		(void) putchar(' ');
		check_getname(lp, name, WHITE, COLON, "");
		gid = Atoi(name);
		put_d(uid); 
		(void) putchar(':');
		printgroups(user, gid);
	}
}


/*
 * Parse the hosts file
 */
dohostfile(hostfile, hf)
	char *hostfile;
	FILE *hf;
{
	char **lp;
	char line[MAXLINELEN];
	char name[MAXNAMELEN];
	char netname[MAXNETNAMELEN];

	curfile = hostfile;
	curline = 0;
	while (lp = getline(line, sizeof(line), hf, &curline, "#")) {
		check_getname(lp, name, WHITE, WHITE, "#");
		if (getname(name, MAXNAMELEN, WHITE, WHITE, lp, "#") != 1) {
			continue;
		}
		host2netname(netname, name, domain);
		if (wasprinted(netname)) {
			multdef(netname);
			continue;
		}
		(void) printf("%s 0:%.*s\n", netname, sizeof(name), name);
	}
}

/*
 * Open a file, exit on failure
 */
static FILE *
openfile(fname)
	char *fname;
{
	FILE *f;

	f = fopen(fname, "r");
	if (f == NULL) {
		(void) fprintf(stderr, "%s: can't open %s\n", cmdname, fname);
		exit(1);
	}
	return (f);
}

/*
 * Print syntax error message, then exit
 */
syntaxerror()
{
	(void) fprintf(stderr, "%s: syntax error in file \"%s\", line %d\n", 
	    cmdname, curfile, curline);
	exit(1);
}


/*
 * an atoi() that prints a message and exits upong failure
 */
static int
Atoi(str)
	char *str;
{
	int res;

	if (!sscanf(str, "%d", &res)) {
		syntaxerror();
	}
	return (res);
}


/*
 * Attempt to get a token from a file, print a message and exit upon failure
 */
check_getname(lp, name, skip, term, com)
	char **lp;
	char *name;
	char *skip;
	char *term;
	char *com;
{
	if (getname(name, MAXNAMELEN, skip, term, lp, com) != 1) {
		syntaxerror();
	}
}

/*
 * Something was defined more than once
 */
multdef(name)
	char *name;
{
	if (!quietmode) {
		(void) fprintf(stderr, 
	    	"%s: %s multiply defined, other definitions ignored\n", 
	    	cmdname, name);
	}
}

static
hash(str, size)
	unsigned char *str;
	int size;
{
	unsigned val;
	int flip;

	val = 0;
	flip = 0;
	while (*str) {
		if (flip) {
			val ^= (*str++ << 6);
		} else {
			val ^= *str++;
		}
		flip = !flip;
	}
	return (val % size);
}


/*
 * Check if an item has been printed
 * If not, store the item into the printed item table
 */
static 
wasprinted(name)
	char *name;
{
	struct string_list *s;
	int val;

	val = hash((unsigned char *) name, PRNTABSIZE);
	for (s = printed[val]; s != NULL && strcmp(s->str, name) ; s = s->next)
		;
	if (s != NULL) {
		return (1);
	}
	s = (struct string_list *) malloc(sizeof(struct string_list));
	s->str = malloc((unsigned) strlen(name) + 1);
	(void) strcpy(s->str, name);
	s->next = printed[val];
	printed[val] = s;
	return (0);
}

/*
 * Add gid to the list of a user's groups
 */
storegid(gid, user)
	int gid;
	char *user;
{
	struct group_list *g;
	int i;
	int val;

	val = hash((unsigned char *) user, GRPTABSIZE);
	for (g = groups[val]; g != NULL && strcmp(g->user, user); g = g->next)
		;
	if (g == NULL) {
		g = (struct group_list *) malloc(sizeof(struct group_list));
		g->user = malloc((unsigned) strlen(user) + 1);
		(void) strcpy(g->user, user);
		g->group_len = 1;
		g->groups[0] = gid;
		g->next = groups[val];
		groups[val] = g;
	} else {
		if (g->group_len > NUMGIDS) {
			(void) fprintf(stderr, "%s: %s's groups exceed %d\n",
				cmdname, user, NUMGIDS);
			return;
		}
		for (i = 0; i < g->group_len; i++) {
			if (g->groups[i] == gid) {
				return;
			}
		}
		g->groups[g->group_len++] = gid;
	}
}

/*
 * print out a user's groups
 */
printgroups(user, gid)
	char *user;
	int gid;
{
	struct group_list *g;
	int i;
	int val;

	val = hash((unsigned char *) user, GRPTABSIZE);
	for (g = groups[val]; g != NULL && strcmp(g->user, user); g = g->next)
		;
	put_d(gid);
	if (g != NULL) {
		for (i = 0; i < g->group_len; i++) {
			if (gid != g->groups[i]) {
				(void) putchar(','); 
				put_d(g->groups[i]);
			}
		}
	}
	(void) putchar('\n');
}


/*
 * Parse command line arguments
 */
parseargs(argc, argv)
	int argc;
	char *argv[];
{
	int i;
	int j;
	static struct {
		char letter;
		char *standard;
		char **filename;
	} whattodo[] = {
		{ 'p', PASSWD, &pwdfile },
		{ 'g', GROUP, &grpfile },
		{ 'm', IDMAP, &mapfile },
		{ 'h', HOSTS, &hostfile }
	};	

#define TABSIZE  sizeof(whattodo)/sizeof(whattodo[0])

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][2] != 0) {
				return (0);
			}
			if (argv[i][1] == 'q') {
				quietmode = 1;
				continue;
			}
			for (j = 0; j < TABSIZE; j++) {
				if (whattodo[j].letter == argv[i][1]) {
					if (*whattodo[j].filename != whattodo[j].standard) {
						return (0);
					}
					if (++i == argc) {
						return (0);
					}
					*whattodo[j].filename = argv[i];
					break;
				}
			}
			if (j == TABSIZE) {
				return (0);
			}
		}
	}
	return (1);
}

/*
 * Print a string, quickly
 */
put_s(s)
	char *s;
{
	(void) fwrite(s, strlen(s), 1, stdout);
}

/*
 * Print an integer, quickly
 */
put_d(d)
	int d;
{
	char buf[20];
	char *p;

	if (d == 0) {
		(void) putchar('0');
		return;
	}
	if (d < 0) {
		(void) putchar('-');
		d = -d;
	}
	p = buf + sizeof(buf);
	*--p = 0;
	while (d > 0) {
		*--p = (d % 10) + '0';
		d /= 10;
	}
	put_s(p);
}
