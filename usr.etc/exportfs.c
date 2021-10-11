#ifndef lint
static char sccsid[] = "@(#)exportfs.c 1.1 92/07/30 SMI";
#endif

/*
 * Export directories (or files) to NFS clients Copyright (c) 1987 Sun
 * Microsystems, Inc.
 */
#include <stdio.h>
#include <netdb.h>
#include <strings.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <nfs/export.h>
#include <exportent.h>
#include <pwd.h>

extern char *malloc();
extern char *realloc();
extern char *strpbrk();
extern char *strdup();
extern char *sprintf();
extern int errno;

/*
 * options to command
 */
#define AFLAG 0x01
#define UFLAG 0x02
#define VFLAG 0x04
#define IFLAG 0x08

#define MAXROOTS EXMAXADDRS
#define MAXWRITE EXMAXADDRS
#define MAXLINELEN 4096
#define MAXNAMELEN 256
#define NETGROUPINCR 32

#define streq(a,b) (strcmp(a, b) == 0)

char *getline();
char *accessjoin();
char *strtoken();
char *check_malloc();
char *check_realloc();
void out_of_memory();

char anon_opt[] = ANON_OPT;
char ro_opt[] = RO_OPT;
char rw_opt[] = RW_OPT;
char root_opt[] = ROOT_OPT;
char access_opt[] = ACCESS_OPT;
char secure_opt[] = SECURE_OPT;
char window_opt[] = WINDOW_OPT;

char EXPORTFILE[] = "/etc/exports";

struct options {
	int anon;
	int window;
	int flags;
	int auth;
	char *hostlist[MAXROOTS];
	int nhosts;
	char **accesslist;
	int accesslistsize;
	int naccess;
	char *writelist[MAXWRITE];
	int nwrites;
};


main(argc, argv)
	int argc;
	char *argv[];

{
	int i;
	char *options = NULL;
	int Argc;
	char **Argv;
	int flags;
	int retstatus = 0;

	if (!parseargs(argc, argv, &flags, &options, &Argc, &Argv)) {
		(void) fprintf(stderr,
			"usage: %s [-auv] [-o options] [dir] ...\n", argv[0]);
		exit(1);
	}
	if (Argc == 0) {
		if (flags & AFLAG) {
			if (flags & UFLAG) {
				retstatus = (unexportall(flags & VFLAG) < 0);
			} else {
				retstatus = (exportall(flags & VFLAG, NULL) 
					     < 0);
			}
		} else {
			printexports();
		}
	} else if (flags & UFLAG) {
		if (options) {
			(void) fprintf(stderr,
				  "exportfs: options ignored for unexport\n");
		}
		for (i = 0; i < Argc; i++) {
			retstatus = (unexport(Argv[i], flags & VFLAG) < 0);
		}
	} else {
		for (i = 0; i < Argc; i++) {
			if (flags & IFLAG) {
				retstatus = (export(Argv[i], options, 
						    flags & VFLAG) < 0);
				
			} else {
				retstatus = (exportall(flags & VFLAG, Argv[i])
					     < 0);
			}
		}
	}

	exit(retstatus);
	/* NOTREACHED */
}

/*
 * Print all exported pathnames
 */
printexports()
{
	struct exportent *xent;
	FILE *f;
	int maxlen;

	f = setexportent();
	if (f == NULL) {
		(void) printf("nothing exported\n");
		return;
	}
	maxlen = 0;
	while (xent = getexportent(f)) {
		if (strlen(xent->xent_dirname) > maxlen) {
			maxlen = strlen(xent->xent_dirname);
		}
	}
	rewind(f);
	while (xent = getexportent(f)) {
		printexport(xent, maxlen + 1);
	}
	if (maxlen == 0) {
		(void) printf("nothing exported\n");
	}
	endexportent(f);
}

/*
 * Print just one exported pathname
 */
printexport(xent, col)
	struct exportent *xent;
	int col;
{
	int i;

	(void) printf("%s", xent->xent_dirname);

	if (xent->xent_options) {
		for (i = strlen(xent->xent_dirname); i < col; i++) {
			(void) putchar(' ');
		}
		(void) printf("-%s", xent->xent_options);
	}
	(void) printf("\n");
}

/*
 * Unexport everything in the export tab file
 */
unexportall(verbose)
	int verbose;
{
	struct exportent *xent;
	FILE *f;
	register int retcode = 0;

	f = setexportent();
	if (f == NULL) {
		return (-1);
	}
	while (xent = getexportent(f)) {
		(void) remexportent(f, xent->xent_dirname);
		if (exportfs(xent->xent_dirname, (struct export *) NULL) < 0) {
			(void) fprintf(stderr, "exportfs: ");
			if (errno == EPERM) {
				(void) fprintf(stderr,
					  "must be superuser to unexport\n");
				endexportent(f);
				return (-1);
			}
			perror(xent->xent_dirname);
			retcode = -1;
		} else {
			if (verbose) {
				(void) printf("unexported %s\n", xent->xent_dirname);
			}
		}
	}
	endexportent(f);
	return (retcode);
}

/*
 * Unexport just one directory
 */
unexport(dirname, verbose)
	char *dirname;
	int verbose;
{
	FILE *f;

	f = setexportent();
	if (f == NULL) {
		return (-1);
	}
	(void) remexportent(f, dirname);
	if (exportfs(dirname, (struct export *) NULL) < 0) {
		(void) fprintf(stderr, "exportfs: ");
		if (errno == EPERM) {
			(void) fprintf(stderr, "must be superuser to unexport\n");
		} else {
			perror(dirname);
		}
		endexportent(f);
		return (-1);
	}
	endexportent(f);
	if (verbose) {
		(void) printf("unexported %s\n", dirname);
	}
	return (0);
}

/*
 * Export everything in /etc/exports
 */
exportall(verbose, which)
	int verbose;
	char *which;
{
	char line[MAXLINELEN];
	char *name;
	FILE *f;
	char *lp;
	char *dirname;
	char *options;
	char **grps;
	int grpsize;
	int ngrps;
	register int i;
	int exported;

	f = fopen(EXPORTFILE, "r");
	if (f == NULL) {
		(void) fprintf(stderr, "exportfs: Can't open ");
		perror(EXPORTFILE);
		return (-1);
	}
	grps = NULL;
	grpsize = 0;
	exported = 0;
	while ((which == NULL || !exported) &&
	       getline(line, sizeof(line), f)) {
		lp = line;
		dirname = NULL;
		options = NULL;
		ngrps = 0;
		while ((name = strtoken(&lp, " \t")) != NULL) {
			if (dirname == NULL) {
				dirname = name;
			} else if (options == NULL && name[0] == '-') {
				if ((options = strdup(name + 1)) == NULL)
					out_of_memory();
			} else {
				grpsize = addtogrouplist(&grps, grpsize,
							 ngrps, name);
				ngrps++;
			}
		}
		if (ngrps > 0) {
			options = accessjoin(options, ngrps, grps);
			for (i = 0; i < ngrps; i++)
				free(grps[i]);
		}
		if (dirname != NULL && (which == NULL || 
					strcmp(dirname, which) == 0)) {
			if (export(dirname, options, verbose) < 0) {
				return (-1);
			}
			exported++;
		}
		if (options != NULL) {
			free(options);
		}
	}
	if (which != NULL && !exported) {
		fprintf(stderr, "%s not found in %s\n", which, EXPORTFILE);
		return (-1);
	}
	return (0);
}

int
addtogrouplist(grplistp, grplistsize, index, group)
	char ***grplistp;
	int grplistsize;
	int index;
	char *group;
{
	if (index == grplistsize) {
		grplistsize += NETGROUPINCR;
		if (*grplistp == NULL)
			*grplistp =
				(char **) check_malloc(((unsigned) grplistsize * sizeof(char *)));
		else
			*grplistp = (char **) check_realloc((char *) *grplistp,
				   (unsigned) (grplistsize * sizeof(char *)));
	}
	(*grplistp)[index] = group;
	return (grplistsize);
}

/*
 * Export just one directory
 */
export(dirname, options, verbose)
	char *dirname;
	char *options;
	int verbose;
{
	struct export ex;
	struct options opt;
	FILE *f;
	int redo = 0;

	if (!interpret(dirname, &opt, options)) {
		return (0);
	}
	fillex(&ex, &opt);
	f = setexportent();
	if (f == NULL) {
		return (-1);
	}
	if (insane(f, dirname)) {
		endexportent(f);
		return (0);
	}
	if (xtab_test(f, dirname)) {
		(void) remexportent(f, dirname);
		redo = 1;
	}
	if (exportfs(dirname, &ex) < 0) {
		(void) fprintf(stderr, "exportfs: ");
		if (errno == EPERM) {
			(void) fprintf(stderr, "must be superuser to export\n");
		} else {
			perror(dirname);
		}
		endexportent(f);
		if (errno == EPERM) {
			return (-1);
		} else {
			return (0);
		}
	}
	addexportent(f, dirname, options);
	endexportent(f);
	if (verbose) {
		if (redo) {
			(void) printf("re-exported %s\n", dirname);
		} else {
			(void) printf("exported %s\n", dirname);
		}
	}
	return (0);
}

#define NOBODY_UID_DEFAULT      (u_short) 65534
#define NOBODY_LOGIN            "nobody"

/*
 * Interpret a line of options
 */
interpret(dirname, opt, line)
	char *dirname;
	struct options *opt;
	char *line;
{
	char *name;
	struct passwd *pw, *getpwnam();

	/*
	 * Initialize and set up defaults
	 */
	if ((pw = getpwnam(NOBODY_LOGIN)) == NULL)
		opt->anon = (int) NOBODY_UID_DEFAULT;
	else
		opt->anon = (int) pw->pw_uid;
	opt->window = 30000;
	opt->flags = 0;
	opt->auth = AUTH_UNIX;
	opt->nhosts = 0;
	opt->accesslist = NULL;
	opt->accesslistsize = 0;
	opt->naccess = 0;
	opt->nwrites = 0;
	if (line == NULL) {
		return (1);
	}
	for (;;) {
		name = strtoken(&line, ",");
		if (name == NULL) {
			return (1);
		}
		if (!dooption(dirname, name, opt)) {
			return (0);
		}
	}
}

/*
 * Fill an export structure given the options selected
 */
fillex(ex, ops)
	struct export *ex;
	struct options *ops;
{
	ex->ex_flags = ops->flags;
	if (ex->ex_flags & EX_RDMOSTLY) {
		ex->ex_writeaddrs.addrvec = (struct sockaddr *)
			check_malloc(EXMAXROOTADDRS * sizeof(struct sockaddr));
		filladdrs(&ex->ex_writeaddrs, ops->writelist, ops->nwrites);
	}
	ex->ex_anon = ops->anon;
	ex->ex_auth = ops->auth;
	switch (ops->auth) {
	case AUTH_UNIX:
		ex->ex_unix.rootaddrs.naddrs = ops->nhosts;
		ex->ex_unix.rootaddrs.addrvec = (struct sockaddr *)
			check_malloc(EXMAXROOTADDRS * sizeof(struct sockaddr));
		filladdrs(&ex->ex_unix.rootaddrs, ops->hostlist, ops->nhosts);
		break;
	case AUTH_DES:
		ex->ex_des.window = ops->window;
		ex->ex_des.nnames = ops->nhosts;
		ex->ex_des.rootnames = (char **)
			check_malloc(EXMAXROOTNAMES * sizeof(char *));
		fillnames(&ex->ex_des, ops);
		break;
	}
}

/*
 * Fill in the root addresses
 */
filladdrs(addrs, names, nnames)
	struct exaddrlist *addrs;
	char **names;
	int nnames;
{
	struct hostent *h;
	struct sockaddr_in *sin;
	int i;

	addrs->naddrs = 0;
	for (i = 0; i < nnames; i++) {
		h = gethostbyname(names[i]);
		if (h != NULL) {
			bzero((char *) &addrs->addrvec[i],
			      sizeof(struct sockaddr));
			addrs->addrvec[i].sa_family = h->h_addrtype;
			switch (h->h_addrtype) {
			case AF_INET:
				sin = (struct sockaddr_in *)
					(addrs->addrvec + i);
				bcopy(h->h_addr, (char *) &sin->sin_addr,
				      h->h_length);
				addrs->naddrs++;
				break;
			default:
				(void) fprintf(stderr,
				       "exportfs: unknown address type: %d\n",
					       h->h_addrtype);
				break;
			}
		} else {
			(void) fprintf(stderr,
			    "exportfs: %s: unknown host\n", names[i]);
		}
	}
}

fillnames(dx, ops)
	struct desexport *dx;
	struct options *ops;
{
	int i;
	char netname[MAXNETNAMELEN + 1];

	dx->nnames = 0;
	for (i = 0; i < ops->nhosts; i++) {
		if (!host2netname(netname, ops->hostlist[i], NULL)) {
			(void) fprintf(stderr,
				       "exportfs: unknown host: %s\n",
				       ops->hostlist[i]);
		} else {
			if ((dx->rootnames[i] = strdup(netname)) == NULL)
				out_of_memory();
			dx->nnames++;
		}
	}
}

dooption(dirname, opstr, ops)
	char *dirname;
	char *opstr;
	struct options *ops;
{
#	define pstreq(a, b, blen) \
		((strncmp(a, b, blen) == 0) && (a[blen] == '='))

	if (streq(opstr, ro_opt)) {
		ops->flags |= EX_RDONLY;
	} else if (pstreq(opstr, rw_opt, sizeof(rw_opt) - 1)) {
		ops->flags |= EX_RDMOSTLY;
		if (!getstaticnamelist(&opstr[sizeof(rw_opt)],
				     &ops->nwrites, ops->writelist, MAXWRITE)){
			toomany("write", dirname, MAXWRITE);
			return (0);
		}
	} else if (streq(opstr, secure_opt)) {
		ops->auth = AUTH_DES;
	} else if (pstreq(opstr, anon_opt, sizeof(anon_opt) - 1)) {
		ops->anon = atoi(&opstr[sizeof(anon_opt)]);
	} else if (pstreq(opstr, window_opt, sizeof(window_opt) - 1)) {
		ops->window = atoi(&opstr[sizeof(window_opt)]);
	} else if (pstreq(opstr, root_opt, sizeof(root_opt) - 1)) {
		if (!getstaticnamelist(&opstr[sizeof(root_opt)],
				     &ops->nhosts, ops->hostlist, MAXROOTS)) {
			toomany("root", dirname, MAXROOTS);
			return (0);
		}
	} else if (pstreq(opstr, access_opt, sizeof(access_opt) - 1)) {
		if (!getdynamicnamelist(&opstr[sizeof(access_opt)],
		     &ops->naccess, &ops->accesslist, &ops->accesslistsize)) {
			out_of_memory();
		}
	} else {
		(void) fprintf(stderr, "exportfs: unknown option: %s\n", opstr);
		return (0);
	}
	return (1);
}

toomany(kind, dirname, max)
	char *kind;
	char *dirname;
	int max;
{
	(void) fprintf(stderr,
	      "exportfs: %s allows %s access to too many hosts (max %d)\n",
				       kind, dirname, max);
}

int
getstaticnamelist(list, nnames, namelist, max)
	char *list;
	int *nnames;
	char *namelist[];
	int max;

{
	char *name;

	for (;;) {
		name = strtoken(&list, ":");
		if (name == NULL) {
			return (1);
		}
		if (*nnames == max) {
			return (0);
		}
		namelist[*nnames] = name;
		(*nnames)++;
	}
}

int
getdynamicnamelist(list, nnames, namelist, namelistsize)
	char *list;
	int *nnames;
	char ***namelist;
	int *namelistsize;
{
	char *name;

	for (;;) {
		name = strtoken(&list, ":");
		if (name == NULL) {
			return (1);
		}
		if ((*namelistsize = addtogrouplist(namelist, *namelistsize,
						    *nnames, name)) == 0)
			return (0);
		(*nnames)++;
	}
}

char *
accessjoin(options, ngrps, grps)
	char *options;
	int ngrps;
	char **grps;
{
	register char *str;
	register int i;
	register unsigned int len;
	register char *p;

	if (options != NULL) {
		len = strlen(options);	/* <options> */
		if (len != 0)
			len++;	/* , */
	} else
		len = 0;
	len += (sizeof(access_opt) - 1) + 1;	/* <access_opt>= */
	for (i = 0; i < ngrps; i++) {
		len += strlen(grps[i]) + 1;	/* group: or group\0 */
	}
	str = check_malloc(len);
	if (options == NULL || *options == '\0') {
		(void) sprintf(str, "%s=%s", access_opt, grps[0]);
	} else {
		(void) sprintf(str, "%s,%s=%s", options, access_opt, grps[0]);
	}
	p = str;
	for (i = 1; i < ngrps; i++) {
		p += strlen(p);
		(void) sprintf(p, ":%s", grps[i]);
	}
	if (options != NULL)
		free(options);
	return (str);
}

parseargs(argc, argv, flags, options, nargc, nargv)
	int argc;
	char *argv[];
	int *flags;
	char **options;
	int *nargc;
	char ***nargv;

{
	int i;
	int j;

	*flags = 0;
	for (i = 1; i < argc && argv[i][0] == '-'; i++) {
		if (argv[i][1] == 0) {
			return (0);
		}
		for (j = 1; argv[i][j] != 0; j++) {
			switch (argv[i][j]) {
			case 'a':
				*flags |= AFLAG;
				break;
			case 'u':
				*flags |= UFLAG;
				break;
			case 'v':
				*flags |= VFLAG;
				break;
			case 'i':
				*flags |= IFLAG;
				break;
			case 'o':
				if (j != 1 || argv[i][2] != 0) {
					return (0);
				}
				if (i + 1 >= argc) {
					return (0);
				}
				*options = argv[++i];
				goto breakout;
			default:
				return (0);
			}
		}
breakout:
		;
	}
	*nargc = argc - i;
	*nargv = argv + i;
	return (1);
}

xtab_test(f, dirname)
	FILE *f;
	char *dirname;
{
	struct exportent *xent;

	rewind(f);
	while (xent = getexportent(f)) {
		if (streq(xent->xent_dirname, dirname)) {
			return (1);
		}
	}
	return (0);
}


direq(dir1, dir2)
	char *dir1;
	char *dir2;
{
	struct stat st1;
	struct stat st2;

	if (stat(dir1, &st1) < 0) {
		return (0);
	}
	if (stat(dir2, &st2) < 0) {
		return (0);
	}
	return (st1.st_ino == st2.st_ino && st1.st_dev == st2.st_dev);
}


insane(f, dir)
	FILE *f;
	char *dir;
{
	struct exportent *xent;

	rewind(f);
	while (xent = getexportent(f)) {
		if (direq(xent->xent_dirname, dir)) {
			continue;
		}
		if (issubdir(xent->xent_dirname, dir)) {
			(void) fprintf(stderr,
			"exportfs: %s: sub-directory (%s) already exported\n",
				       dir, xent->xent_dirname);
			return (1);
		}
		if (issubdir(dir, xent->xent_dirname)) {
			(void) fprintf(stderr,
				       "exportfs: %s: parent-directory (%s) already exported\n",
				       dir, xent->xent_dirname);
			return (1);
		}
	}
	return (0);
}



#define iseol(c)	(c == '\0' || c == '\n' || c == '#')
#define issep(c)	(strchr(sep, c) != NULL)
#define isignore(c) (strchr(ignore, c) != NULL)

/*
 * getline() Read a line from a file, converting backslash-newline to space.
 * Returns it argument, or NULL on end-of-file.
 */
char *
getline(line, maxlinelen, f)
	char *line;
	int maxlinelen;
	FILE *f;
{
	char *p;

	do {
		if (!fgets(line, maxlinelen, f)) {
			return (NULL);
		}
	} while (iseol(line[0]));
	p = line;
	maxlinelen -= strlen(p);
	/*
	 * maxlinelen is always > 0 since fgets only reads maxlinelen-1
	 * characters (incl newline). The check for maxlinelen could be
	 * gotten rid of. The maximum line length restriction is still valid.
   	 */
	for (; maxlinelen ;) {
		while (!iseol(*p)) {
			p++;
		}
		if (*p == '\n' && *(p - 1) == '\\') {
			p--;
			maxlinelen += 2;
			if (fgets(p, maxlinelen, f) == NULL) {
				*p = 0;
				break;
			}
			else
			        maxlinelen -= strlen(p);
		} else {
			*p = 0;
			break;
		}
	}
	return (line);
}


/*
 * Like strtok(), but no static data
 */
char *
strtoken(string, sepset)
	char **string;
	char *sepset;
{
	char *p;
	char *q;
	char *r;
	char *res;

	p = *string;
	if (p == 0) {
		return (NULL);
	}
	q = p + strspn(p, sepset);
	if (*q == 0) {
		return (NULL);
	}
	r = strpbrk(q, sepset);
	if (r == NULL) {
		*string = 0;
		if ((res = strdup(q)) == NULL)
			out_of_memory();
	} else {
		res = check_malloc((unsigned) (r - q + 1));
		(void) strncpy(res, q, r - q);
		res[r - q] = 0;
		*string = ++r;
	}
	return (res);
}

char *
check_malloc(size)
	unsigned int size;
{
	register char *memoryp;

	if ((memoryp = malloc(size)) == NULL)
		out_of_memory();
	return (memoryp);
}

char *
check_realloc(memoryp, size)
	char *memoryp;
	unsigned int size;
{

	if ((memoryp = realloc(memoryp, size)) == NULL)
		out_of_memory();
	return (memoryp);
}

void
out_of_memory()
{
	(void) fprintf(stderr, "exportfs: Out of memory\n");
	exit(1);
}
