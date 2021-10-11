#ifndef lint
static char sccsid[] = "@(#)openprom.c 1.1 92/07/30 SMI";
#endif

/*
 * Open Boot Prom eeprom utility
 * Copyright (c) 1989 Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sundev/openpromio.h>
#include <stdio.h>
#include <fcntl.h>
#include <strings.h>

/*
 * Usage:  % eeprom [-f filename] [-] 
 *	   % eeprom [-f filename] field[=value] ...
 */

/*
 * 64 is the size of the largest (currently) property name
 * 512 is the size of the largest (currently) property value, viz. oem-message
 * the sizeof(u_int) is from struct openpromio
 */

#define MAXPROPSIZE	64
#define MAXVALSIZE	512
#define BUFSIZE		(MAXPROPSIZE + MAXVALSIZE + sizeof(u_int))
typedef union {
	char buf[BUFSIZE];
	struct openpromio opp;
} Oppbuf;

int _error();
#define	NO_PERROR	((char *) 0)

static int prom_fd;
static char *promdev;
int verbose;

static void do_var(), dump_all(), print_one(), set_one(),
	promopen(), promclose();

static getpropval(), setpropval();

typedef	void (*func)();

#define	e(n, i, o)	{n, i, o}

/* We have to special-case two properties related to security */
void i_secure();
void i_passwd(), o_passwd();
void i_oemlogo();

/*
 * It's unfortunate that we have to know the names of certain properties
 * in this program (the whole idea of openprom was to avoid it), but at
 * least we can isolate them to these defines here.
 */
#define PASSWORD_PROPERTY "security-password"
#define MODE_PROPERTY "security-mode"
#define LOGO_PROPERTY "oem-logo"
#define PW_SIZE 8

/*
 * Unlike the old-style eeprom command, where every property needed an
 * i_foo and an o_foo function, we only need them when the default case
 * isn't sufficient.
 */
struct	eevar {
	char	*name;
	func	in;
	func	out;
} eevar[] = {
 	e(MODE_PROPERTY,	i_secure,	(func)NULL),
 	e(PASSWORD_PROPERTY,	i_passwd,	o_passwd),
	e(LOGO_PROPERTY,	i_oemlogo,	(func)NULL),
	{ (char *)NULL, (func)NULL, (func)NULL}
};


main(argc,argv)
register int argc;
register char **argv;
{
	int c;
	extern char *optarg;
	extern int optind;

	promdev = "/dev/openprom";

	while ((c = getopt(argc, argv, "cif:v")) != -1)
		switch (c) {
		case 'c':
		case 'i':
			/* ignore for openprom */
			break;
		case 'v':
			verbose++;
			break;
		case 'f':
			promdev = optarg;
			break;
		default:
			exit(_error(NO_PERROR,
		"Usage: %s [-v] [-f prom-device] [variable[=value] ...]",
				argv[0]));
		}
		
	setprogname(argv[0]);

	/*
	 * If no arguments, dump all fields.
	 */
	if (optind >= argc) {
		dump_all();
		exit(0);
	}

	while (optind < argc) {
		/*
		 * If "-" specified, read variables from stdin.
		 */
		if (strcmp(argv[optind], "-") == 0) {
			int c;
			char *nl, line[BUFSIZE];

			while (fgets(line, sizeof line, stdin) != NULL) {
				/* zap newline if present */
				if (nl = index(line, '\n'))
					*nl = 0;
				/* otherwise discard rest of line */
				else
					while ((c = getchar()) != '\n' &&
						c != EOF)
						/* nothing */ ;

				do_var(line);
			}
			clearerr(stdin);
		}
		/*
		 * Process each argument as a variable print or set request.
		 */
		else
			do_var(argv[optind]);

		optind++;
	}
}

/*
 * Print or set an EEPROM field.
 */
static void
do_var(var)
	char *var;
{
	register char *val;

	val = index(var, '=');

	if (val == NULL) {
		/*
		 * print specific property
		 */
		promopen(O_RDONLY);
		print_one(var);
	}
	else {
		/*
		 * set specific property to value
		 */
		*val++ = '\0';

		promopen(O_RDWR);
		set_one(var, val);
	}
	promclose();
}

/*
 * Print all properties and values
 */
static void
dump_all()
{
	Oppbuf	oppbuf;
	register struct openpromio *opp = &(oppbuf.opp);

	promopen(O_RDONLY);
	/* get first prop by asking for null string */
	bzero(oppbuf.buf, BUFSIZE);
	while (1) {
		/*
		 * get property
		 */
		opp->oprom_size = MAXPROPSIZE;

		if (ioctl(prom_fd,OPROMNXTOPT,opp) < 0)
			exit(_error("OPROMNXTOPT"));

		if (opp->oprom_size == 0) {
			promclose();
			return;
		}
		print_one(opp->oprom_array);
	}
}

/*
 * Print one property and its value.
 */
static void
print_one(var)
	char	*var;
{
	Oppbuf	oppbuf;
	register struct openpromio *opp = &(oppbuf.opp);

	(void)strcpy(opp->oprom_array, var);
	if (getpropval(opp) || opp->oprom_size < 0)
		(void)printf("%s: data not available.\n", var);
	else {
		if (opp->oprom_size == 0)
			(void)printf("%s=%s\n", var, opp->oprom_array);
		else {
			/* If necessary, massage the output */
			register struct eevar *v;

			for (v = eevar; v->name; v++)
				if (strcmp(var, v->name) == 0)
					break;

			if (v->name && v->out)
				(*v->out)(v->name, opp->oprom_array);
			else
				(void)printf("%s=%s\n", var, opp->oprom_array);
		}
	}
}

/*
 * Set one property to the given value.
 */
static void
set_one(var, val)
	char *var;
	char *val;
{
	Oppbuf	oppbuf;
	register struct openpromio *opp = &(oppbuf.opp);
	register struct eevar *v;

	if (verbose) {
		(void)printf("old:");
		print_one(var);
	}

	/* If necessary, massage the input */

	for (v = eevar; v->name; v++)
		if (strcmp(var, v->name) == 0)
			break;

	if (v->name && v->in)
		(*v->in)(v->name, val, opp);
	else {
		int varlen = strlen(var) + 1;

		(void)strcpy(opp->oprom_array, var);
		(void)strcpy(opp->oprom_array + varlen, val);
		opp->oprom_size = varlen + strlen(val);
		if (setpropval(opp))
			(void)printf("%s: invalid property.\n", var);
	}

	if (verbose) {
		(void)printf("new:");
		print_one(var);
	}
}

static void
promopen(oflag)
register int oflag;
{
	if ((prom_fd = open(promdev,oflag)) < 0)
		exit(_error("cannot open %s", promdev));
}

static void
promclose()
{
	if (close(prom_fd) < 0)
		exit(_error("close error on %s", promdev));
}

static
getpropval(opp)
register struct openpromio *opp;
{
	opp->oprom_size = MAXVALSIZE;

	if (ioctl(prom_fd,OPROMGETOPT,opp) < 0)
		return (_error("OPROMGETOPT"));
	
	if (opp->oprom_size == 0)
		*opp->oprom_array = 0;

	return (0);
}

static
setpropval(opp)
register struct openpromio *opp;
{
	/* Caller must set opp->oprom_size */

	if (ioctl(prom_fd, OPROMSETOPT, opp) < 0)
		return (_error("OPROMSETOPT"));
	return (0);
}


/*
 * The next set of functions handle the special cases.
 */

static void
i_oemlogo(var, val, opp)
	char *var;
	char *val;
	struct openpromio *opp;
{
	int varlen = strlen(var) + 1;

	(void)strcpy(opp->oprom_array, var);

	if (loadlogo(val, 64, 64, opp->oprom_array + varlen))
		exit(1);
	opp->oprom_size = varlen + 512;
	if (ioctl(prom_fd, OPROMSETOPT2, opp) < 0)
		exit(_error("OPROMSETOPT2"));
}

/*
 * Set security mode.
 * If oldmode was none, and new mode is not none, get and set password,
 * too.
 * If old mode was not none, and new mode is none, wipe out old
 * password.
 */
static void
i_secure(var, val, opp)
	char *var;
	char *val;
	struct openpromio *opp;
{
	int secure;
	Oppbuf oppbuf;
	register struct openpromio *opp2 = &(oppbuf.opp);
	char pwbuf[PW_SIZE + 2];
	int varlen1, varlen2;

	(void)strcpy(opp2->oprom_array, var);
	if (getpropval(opp2) || opp2->oprom_size <= 0) {
		(void)printf("%s: data not available.\n", var);
		exit(1);
	}
	secure = strcmp(opp2->oprom_array, "none");

	/* Set up opp for mode */
	(void)strcpy(opp->oprom_array, var);
	varlen1 = strlen(opp->oprom_array) + 1;
	(void)strcpy(opp->oprom_array + varlen1, val);
	opp->oprom_size = varlen1 + strlen(val);

	/* Set up opp2 for password */
	(void)strcpy(opp2->oprom_array, PASSWORD_PROPERTY);
	varlen2 = strlen(opp2->oprom_array) + 1;

	if ((strcmp(val, "full") == 0) || (strcmp(val, "command") == 0)) {
		if (! secure) {
			/* no password yet, get one */
			if (get_password(pwbuf)) {
				strcpy(opp2->oprom_array + varlen2, pwbuf);
				opp2->oprom_size = varlen2 + strlen(pwbuf);
				/* set password first */
				if (setpropval(opp2) || setpropval(opp))
					exit(1);
			} else
				exit(1);
		} else {
			if (setpropval(opp))
				exit(1);
		}
	} else if (strcmp(val, "none") == 0) {
		if (secure) {
			(void) bzero(opp2->oprom_array + varlen2, PW_SIZE);
			opp2->oprom_size = varlen2 + PW_SIZE;
			/* set mode first */
			if (setpropval(opp) || setpropval(opp2))
				exit(1);
		} else {
			if (setpropval(opp))
				exit(1);
		}
	} else {
		(void) printf("Invalid security mode, mode unchanged.\n");
		exit(1);
	}
}

/*
 * Set password.
 * We must be in a secure mode in order to do this.
 */
static void
i_passwd(var, val, opp)
	char *var;
	char *val;
	struct openpromio *opp;
{
	int secure;
	Oppbuf oppbuf;
	register struct openpromio *opp2 = &(oppbuf.opp);
	char pwbuf[PW_SIZE + 2];
	int varlen;

	(void)strcpy(opp2->oprom_array, MODE_PROPERTY);
	if (getpropval(opp2) || opp2->oprom_size <= 0) {
		(void)printf("%s: data not available.\n", opp2->oprom_array);
		exit(1);
	}
	secure = strcmp(opp2->oprom_array, "none");

	if (! secure) {
		printf("Not in secure mode\n");
		exit(1);
	}

	/* Set up opp for password */
	(void)strcpy(opp->oprom_array, var);
	varlen = strlen(opp->oprom_array) + 1;

	if (get_password(pwbuf)) {
		strcpy(opp->oprom_array + varlen, pwbuf);
		opp->oprom_size = varlen + strlen(pwbuf);
		if (setpropval(opp))
			exit(1);
	} else
		exit(1);
}

static void
o_passwd(var, val)
	char *var, *val;
{
	/* Don't print the password */
}

get_password(pw_dest)
	char *pw_dest;
{
	int i, insist = 0, ok, flags;
	int c, pwlen;
	char *p;
	char pwbuf[10];

tryagain:
	(void) printf("Changing PROM password:\n");
	(void) strcpy(pwbuf, getpass("New password:"));
	pwlen = strlen(pwbuf);
	if (pwlen == 0) {
		(void) printf("Password unchanged.\n");
		return(0);
	}
	/*
	 * Insure password is of reasonable length and
	 * composition.  If we really wanted to make things
	 * sticky, we could check the dictionary for common
	 * words, but then things would really be slow.
	 */
	ok = 0;
	flags = 0;
	p = pwbuf;
	while (c = *p++) {
		if (c >= 'a' && c <= 'z')
			flags |= 2;
		else if (c >= 'A' && c <= 'Z')
			flags |= 4;
		else if (c >= '0' && c <= '9')
			flags |= 1;
		else
			flags |= 8;
	}
	if (flags >= 7 && pwlen >= 4)
		ok = 1;
	if ((flags == 2 || flags == 4) && pwlen >= 6)
		ok = 1;
	if ((flags == 3 || flags == 5 || flags == 6) && pwlen >= 5)
		ok = 1;
	if (!ok && insist < 2) {
	(void) printf("Please use %s.\n", flags == 1 ?
			"at least one non-numeric character" :
			"a longer password");
		insist++;
		goto tryagain;
	}
	if (strcmp(pwbuf, getpass("Retype new password:")) != 0) {
		(void) printf("Mismatch - password unchanged.\n");
		return(0);
	}
	(void) strncpy(pw_dest, pwbuf, PW_SIZE);
	return(1);
}
