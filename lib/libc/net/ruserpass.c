#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)ruserpass.c 1.1 92/07/30 SMI"; /* from UCB 4.2 82/10/10 */
#endif

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

char	*malloc(), *index(), *getenv(), *getpass(), *getlogin();

#define	DEFAULT	1
#define	LOGIN	2
#define	PASSWD	3
#define	NOTIFY	4
#define	WRITE	5
#define	YES	6
#define	NO	7
#define	COMMAND	8
#define	FORCE	9
#define	ID	10
#define	MACHINE	11

#define	MAXTOKEN  11
#define NTOKENS	(MAXTOKEN - 1 + 2 + 1)	/* two duplicates and null, minus id */

static struct ruserdata {
	char tokval[100];
	struct toktab {
		char *tokstr;
		int tval;
	} toktab[NTOKENS];
	FILE *cfile;
} *ruserdata, *_ruserdata();


static struct ruserdata *
_ruserdata()
{
	register struct ruserdata *d = ruserdata;
	struct toktab *t;

	if (d == 0) {
		if ((d = (struct ruserdata *) 
			calloc(1, sizeof(struct ruserdata))) == NULL) {
				return(NULL);
		}
		ruserdata = d;
		t = d->toktab;
		t->tokstr = "default";  t++->tval = DEFAULT;
		t->tokstr = "login";    t++->tval = LOGIN;
		t->tokstr = "password"; t++->tval = PASSWD;
		t->tokstr = "notify";   t++->tval = NOTIFY;
		t->tokstr = "write";    t++->tval = WRITE;
		t->tokstr = "yes";      t++->tval = YES;
		t->tokstr = "y";        t++->tval = YES;
		t->tokstr = "no";       t++->tval = NO;
		t->tokstr = "n";        t++->tval = NO;
		t->tokstr = "command";  t++->tval = COMMAND;
		t->tokstr = "force";    t++->tval = FORCE;
		t->tokstr = "machine";  t++->tval = MACHINE;
		t->tokstr = 0;          t->tval = 0;
	}
	return(d);
}


_ruserpass(host, aname, apass)
	char *host, **aname, **apass;
{

	if (*aname == 0 || *apass == 0)
		rnetrc(host, aname, apass);
	if (*aname == 0) {
		char *myname = getlogin();
		*aname = malloc(16);
		printf("Name (%s:%s): ", host, myname);
		fflush(stdout);
		if (read(2, *aname, 16) <= 0)
			exit(1);
		if ((*aname)[0] == '\n')
			*aname = myname;
		else
			if (index(*aname, '\n'))
				*index(*aname, '\n') = 0;
	}
	if (*aname && *apass == 0) {
		printf("Password (%s:%s): ", host, *aname);
		fflush(stdout);
		*apass = getpass("");
	}
}


static
rnetrc(host, aname, apass)
	char *host, **aname, **apass;
{
	register struct ruserdata *d = _ruserdata();
	char *hdir, buf[BUFSIZ];
	int t;
	struct stat stb;
	extern int errno;

	if (d == 0)
		return;

	hdir = getenv("HOME");
	if (hdir == NULL)
		hdir = ".";
	sprintf(buf, "%s/.netrc", hdir);
	d->cfile = fopen(buf, "r");
	if (d->cfile == NULL) {
		if (errno != ENOENT)
			perror(buf);
		return;
	}
next:
	while ((t = token())) switch(t) {

	case DEFAULT:
		(void) token();
		continue;

	case MACHINE:
		if (token() != ID || strcmp(host, d->tokval))
			continue;
		while ((t = token()) && t != MACHINE) switch(t) {

		case LOGIN:
			if (token())
				if (*aname == 0) { 
					*aname = malloc(strlen(d->tokval) + 1);
					strcpy(*aname, d->tokval);
				} else {
					if (strcmp(*aname, d->tokval))
						goto next;
				}
			break;
		case PASSWD:
			if (fstat(fileno(d->cfile), &stb) >= 0
			    && (stb.st_mode & 077) != 0) {
	fprintf(stderr, "Error - .netrc file not correct mode.\n");
	fprintf(stderr, "Remove password or correct mode.\n");
				exit(1);
			}
			if (token() && *apass == 0) {
				*apass = malloc(strlen(d->tokval) + 1);
				strcpy(*apass, d->tokval);
			}
			break;
		case COMMAND:
		case NOTIFY:
		case WRITE:
		case FORCE:
			(void) token();
			break;
		default:
	fprintf(stderr, "Unknown .netrc option %s\n", d->tokval);
			break;
		}
		goto done;
	}
done:
	fclose(d->cfile);
}

static
token()
{
	register struct ruserdata *d = _ruserdata();
	char *cp;
	int c;
	struct toktab *t;

	if (d == 0)
		return(0);

	if (feof(d->cfile))
		return (0);
	while ((c = getc(d->cfile)) != EOF &&
	    (c == '\n' || c == '\t' || c == ' ' || c == ','))
		continue;
	if (c == EOF)
		return (0);
	cp = d->tokval;
	if (c == '"') {
		while ((c = getc(d->cfile)) != EOF && c != '"') {
			if (c == '\\')
				c = getc(d->cfile);
			*cp++ = c;
		}
	} else {
		*cp++ = c;
		while ((c = getc(d->cfile)) != EOF
		    && c != '\n' && c != '\t' && c != ' ' && c != ',') {
			if (c == '\\')
				c = getc(d->cfile);
			*cp++ = c;
		}
	}
	*cp = 0;
	if (d->tokval[0] == 0)
		return (0);
	for (t = d->toktab; t->tokstr; t++)
		if (!strcmp(t->tokstr, d->tokval))
			return (t->tval);
	return (ID);
}

