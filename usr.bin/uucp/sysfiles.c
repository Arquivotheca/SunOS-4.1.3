/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)sysfiles.c 1.1 92/07/30"	/* from SVR3.2 uucp:sysfiles.c 1.9 */
#include <stdio.h>
#include <unistd.h>
#include "sysfiles.h"
#ifdef sun
#include <sys/file.h>	/* for R_OK */
#endif


/*
 * manage systems files (Systems, Devices, and Dialcodes families).
 *
 * also manage new file Devconfig, allows per-device setup.
 * present use is to specify what streams modules to push/pop for
 * AT&T TLI/streams network.  see tssetup() in interface.c for details.
 *
 * TODO:
 *    call bsfix()?
 *    combine the 3 versions of everything (sys, dev, and dial) into one.
 *    allow arbitrary classes of service.
 *    need verifysys() for uucheck.
 *    nameserver interface?
 *    pass sysname (or 0) to getsysline().  (might want reg. exp. or NS processing 
 */

/* private variables */
static void tokenize(), nameparse(), setfile(), setioctl(),
	scansys(), scancfg();
static int namematch(), nextdialers(), nextsystems(), getline();

/* pointer arrays might be dynamically allocated */
static char *Systems[64] = {0};	/* list of Systems files */
static char *Devices[64] = {0};	/* list of Devices files */
static char *Dialers[64] = {0};	/* list of Dialers files */
static char *Pops[64] = {0};	/* list of STREAMS modules to be popped */
static char *Pushes[64] = {0};	/* list of STREAMS modules to be pushed */

static int nsystems;		/* index into list of Systems files */
static int ndevices;		/* index into list of Devices files */
static int ndialers;		/* index into list of Dialers files */
static int npops;		/* index into list of STREAMS modules */
							/*to be popped */
static int npushes;		/* index into list of STREAMS modules */
							/*to be pushed */

static FILE *fsystems;
static FILE *fdevices;
static FILE *fdialers;

char errformat[BUFSIZ];

/* this might be dynamically allocated */
#define NTOKENS 16
static char *tokens[NTOKENS], **tokptr;

/* export these */
extern void sysreset(), devreset(), dialreset(), setdevcfg(), setservice();
extern char *strsave();

/* import these */
extern char *strcpy(), *strtok(), *strchr(), *malloc(), *strsave();
extern int errno;

/*
 * setservice init's Systems, Devices, Dialers lists from Sysfiles
 */
void
setservice(service)
	char *service;
{
	scansys(service);
	return;
}

/*
 * setdevcfg init's Pops, Pushes lists from Devconfig
 */

void
setdevcfg(service, device)
	char *service, *device;
{
	scancfg(service, device);
	return;
}

/*	administrative files access */
int
sysaccess(type)
	int type;
{
	switch (type) {

	case ACCESS_SYSTEMS:
		return(access(Systems[nsystems], R_OK));
	case ACCESS_DEVICES:
		return(access(Devices[ndevices], R_OK));
	case ACCESS_DIALERS:
		return(access(Dialers[ndialers], R_OK));
	case EACCESS_SYSTEMS:
		return(eaccess(Systems[nsystems], R_OK));
	case EACCESS_DEVICES:
		return(eaccess(Devices[ndevices], R_OK));
	case EACCESS_DIALERS:
		return(eaccess(Dialers[ndialers], R_OK));
	default:
		(void)sprintf(errformat, "bad access type %d", type);
		logent(errformat, "sysaccess");
		return(FAIL);
	}
}


/*
 * read Sysfiles, set up lists of Systems/Devices/Dialers file names.
 * allow multiple entries for a given service, allow a service
 * type to describe resources more than once, e.g., systems=foo:baz systems=bar.
 */
static void
scansys(service)
	char *service;
{	FILE *f;
	char *tok, buf[BUFSIZ];

	Systems[0] = Devices[0] = Dialers[0] = NULL;
	if ((f = fopen(SYSFILES, "r")) != 0) {
		while (getline(f, buf) > 0) { 
			/* got a (logical) line from Sysfiles */
			/* strtok's of this buf continue in tokenize() */
			tok = strtok(buf, " \t");
			if (namematch("service=", tok, service)) {
				tokenize();
				nameparse();
			}
		}
	}

	/* if didn't find entries in Sysfiles, use defaults */
	if (Systems[0] == NULL) {
		Systems[0] = strsave(SYSTEMS);
		ASSERT(Systems[0] != NULL, "CAN'T ALLOCATE", "scansys: Systems",
			Systems[0]);
		Systems[1] = NULL;
	}
	if (Devices[0] == NULL) {
		Devices[0] = strsave(DEVICES);
		ASSERT(Devices[0] != NULL, "CAN'T ALLOCATE", "scansys: Devices",
			Devices[0]);
		Devices[1] = NULL;
	}
	if (Dialers[0] == NULL) {
		Dialers[0] = strsave(DIALERS);
		ASSERT(Dialers[0] != NULL, "CAN'T ALLOCATE", "scansys: Dialers",
			Dialers[0]);
		Dialers[1] = NULL;
	}
	return;
}


/*
 * read Devconfig.  allow multiple entries for a given service, allow a service
 * type to describe resources more than once, e.g., push=foo:baz push=bar.
 */
static void
scancfg(service, device)
	char *service, *device;
{	FILE *f;
	char *tok, buf[BUFSIZ];

	npops = npushes = 0;
	Pops[0] = Pushes[0] = NULL;
	if ((f = fopen(DEVCONFIG, "r")) != 0) {
		while (getline(f, buf) > 0) {
			/* got a (logical) line from Devconfig */
			/* strtok's of this buf continue in tokenize() */
			tok = strtok(buf, " \t");
			if (namematch("service=", tok, service)) {
				tok = strtok((char *)0, " \t");
				if ( namematch("device=", tok, device)) {
					tokenize();
					nameparse();
				}
			}
		}
	}

	return;

}

/*
 *  given a file pointer and buffer, construct logical line in buffer
 *  (i.e., concatenate lines ending in '\').  return length of line
 *  ASSUMES that buffer is BUFSIZ long!
 */

static int
getline(f, line)
	FILE *f;
	char *line;
{	char *lptr, *lend;

	lptr = line;
	while (fgets(lptr, (line + BUFSIZ) - lptr, f) != NULL) {
		lend = lptr + strlen(lptr);
		if (lend == lptr || lend[-1] != '\n')	
			/* empty buf or line too long! */
			break;
		*--lend = '\0'; /* lop off ending '\n' */
		if ( lend == line ) /* empty line - ignore */
			continue;
		lptr = lend;
		if (lend[-1] != '\\')
			break;
		/* continuation */
		lend[-1] = ' ';
	}
	return(lptr - line);
}

/*
 * given a label (e.g., "service=", "device="), a name ("cu", "uucico"),
 *  and a line:  if line begins with the label and if the name appears
 * in a colon-separated list of names following the label, return true;
 * else return false
 */
static int
namematch(label, line, name)
	char *label, *line, *name;
{	char *lend;

	if (strncmp(label, line, strlen(label)) != SAME) {
		return(FALSE);	/* probably a comment line */
	}
	line += strlen(label);
	if (*line == '\0')
		return(FALSE);
	/*
	 * can't use strtok() in the following because scansys(),
	 * scancfg() do an initializing call to strtok() before
	 * coming here and then CONTINUE calling strtok() in tokenize(),
	 * after returning from namematch().
	 */
	while ((lend = strchr(line, ':')) != NULL) {
		*lend = '\0';
		if (strcmp(line, name) == SAME)
			return(TRUE);
		line = lend+1;
	}
	return(strcmp(line, name) == SAME);
}

/*
 * tokenize() continues pulling tokens out of a buffer -- the
 * initializing call to strtok must have been made before calling
 * tokenize() -- and starts stuffing 'em into tokptr.
 */
static void
tokenize()
{	char *tok;

	tokptr = tokens;
	while ((tok = strtok((char *) NULL, " \t")) != NULL) {
		*tokptr++ = tok;
		if (tokptr - tokens >= NTOKENS)
			break;
	}
	*tokptr = NULL;
}

/*
 * look at top token in array: should be line of the form
 *	name=item1:item2:item3...
 * if name is one we recognize, then call set[file|ioctl] to set up 
 * corresponding list.  otherwise, log bad name.
 */
static void
nameparse()
{	char **line, *equals;

	for (line = tokens; (line - tokens) < NTOKENS && *line; line++) {
		equals = strchr(*line, '=');
		if (equals == NULL)
			continue;	/* may be meaningful someday? */
		*equals = '\0';
		/* ignore entry with empty rhs */
		if (*++equals == '\0')
			continue;
		if (strcmp(*line, "systems") == SAME)
			setfile(Systems, equals);
		else if (strcmp(*line, "devices") == SAME)
			setfile(Devices, equals);
		else if (strcmp(*line, "dialers") == SAME)
			setfile(Dialers, equals);
		else if (strcmp(*line, "pop") == SAME)
			setioctl(Pops, equals);
		else if (strcmp(*line, "push") == SAME)
			setioctl(Pushes, equals);
		else {
			(void)sprintf(errformat,"unrecognized label %s",*line);
			logent(errformat, "Sysfiles|Devconfig");
		}
	}
}

/*
 * given the list for a particular type (systems, devices,...)
 * and a line of colon-separated files, add 'em to list
 */

static void
setfile(type, line)
	char **type, *line;
{	char **tptr, *tok;
	char expandpath[BUFSIZ];

	if (*line == 0)
		return;
	tptr = type;
	while (*tptr)		/* skip over existing entries to*/
		tptr++;		/* concatenate multiple entries */

	for (tok = strtok(line, ":"); tok != NULL;
	tok = strtok((char *) NULL, ":")) {
		expandpath[0] = '\0';
		if ( *tok != '/' )
			/* by default, file names are relative to SYSDIR */
			sprintf(expandpath, "%s/", SYSDIR);
		strcat(expandpath, tok);
		if (eaccess(expandpath, 4) != 0)
			/* if we can't read it, no point in adding to list */
			continue;
		*tptr = strsave(expandpath);
		ASSERT(*tptr != NULL, "CAN'T ALLOCATE", "setfile: tptr", *tptr);
		tptr++;
	}
}

/*
 * given the list for a particular ioctl (push, pop)
 * and a line of colon-separated modules, add 'em to list
 */

static void
setioctl(type, line)
	char **type, *line;
{	char **tptr, *tok;

	if (*line == 0)
		return;
	tptr = type;
	while (*tptr)		/* skip over existing entries to*/
		tptr++;		/* concatenate multiple entries */
	for (tok = strtok(line, ":"); tok != NULL;
	tok = strtok((char *) NULL, ":")) {
		*tptr = strsave(tok);
		ASSERT(*tptr != NULL, "CAN'T ALLOCATE", "setioctl: tptr", *tptr);
		tptr++;
	}
}



/*
 * reset Systems files
 */
void
sysreset()
{
	if (fsystems)
		fclose(fsystems);
	fsystems = NULL;
	nsystems = 0;
	devreset();
}

/*
 * reset Devices files
 */
void		
devreset()
{
	if (fdevices)
		fclose(fdevices);
	fdevices = NULL;
	ndevices = 0;
	dialreset();
}

/*
 * reset Dialers files
 */
void		
dialreset()
{
	if (fdialers)
		fclose(fdialers);
	fdialers = NULL;
	ndialers = 0;
}

/*
 * get next line from Systems file
 * return TRUE if successful, FALSE if not
 */
int
getsysline(buf, len)
	char *buf;
{
	if (Systems[0] == NULL)
		/* not initialized via setservice() - use default */
		setservice("uucico");

	/* initialize devices and dialers whenever a new line is read */
	/* from systems */
	devreset();
	if (fsystems == NULL)
		if (nextsystems() == FALSE)
			return(FALSE);
	for(;;) {
		if (fgets(buf, len, fsystems) != NULL)
			return(TRUE);
		if (nextsystems() == FALSE)
			return(FALSE);
	}
}

/*
 * move to next systems file.  return TRUE if successful, FALSE if not
 */
static int
nextsystems()
{
	devreset();

	if (fsystems != NULL) {
		(void) fclose(fsystems);
		nsystems++;
	} else {
		nsystems = 0;
	}
	for ( ; Systems[nsystems] != NULL; nsystems++)
		if ((fsystems = fopen(Systems[nsystems], "r")) != NULL)
			return(TRUE);
	return(FALSE);
}
		
/*
 * get next line from Devices file
 * return TRUE if successful, FALSE if not
 */
int
getdevline(buf, len)
	char *buf;
{
	if (Devices[0] == NULL)
		/* not initialized via setservice() - use default */
		setservice("uucico");

	if (fdevices == NULL)
		if (nextdevices() == FALSE)
			return(FALSE);
	for(;;) {
		if (fgets(buf, len, fdevices) != NULL)
			return(TRUE);
		if (nextdevices() == FALSE)
			return(FALSE);
	}
}

/*
 * move to next devices file.  return TRUE if successful, FALSE if not
 */
static int
nextdevices()
{
	if (fdevices != NULL) {
		(void) fclose(fdevices);
		ndevices++;
	} else {
		ndevices = 0;
	}
	for ( ; Devices[ndevices] != NULL; ndevices++)
		if ((fdevices = fopen(Devices[ndevices], "r")) != NULL)
			return(TRUE);
	return(FALSE);
}

		
/*
 * get next line from Dialers file
 * return TRUE if successful, FALSE if not
 */

int
getdialline(buf, len)
	char *buf;
{
	if (Dialers[0] == NULL)
		/* not initialized via setservice() - use default */
		setservice("uucico");

	if (fdialers == NULL)
		if (nextdialers() == FALSE)
			return(FALSE);
	for(;;) {
		if (fgets(buf, len, fdialers) != NULL)
			return(TRUE);
		if (nextdialers() == FALSE)
			return(FALSE);
	}
}

/*
 * move to next dialers file.  return TRUE if successful, FALSE if not
 */
static int
nextdialers()
{
	if (fdialers) {
		(void) fclose(fdialers);
		ndialers++;
	} else {
		ndialers = 0;
	}
	
	for ( ; Dialers[ndialers] != NULL; ndialers++)
		if ((fdialers = fopen(Dialers[ndialers], "r")) != NULL)
			return(TRUE);
	return(FALSE);
}

/*
 * get next module to be popped
 * return TRUE if successful, FALSE if not
 */
int
getpop(buf, len, optional)
	char *buf;
	int len, *optional;
{
	int slen;

	if ( Pops[0] == NULL || Pops[npops] == NULL )
		return(FALSE);

	/*	if the module name is enclosed in parentheses,	*/
	/*	is optional. set flag & strip parens		*/
	slen = strlen(Pops[npops]) - 1;
	if ( Pops[npops][0] == '('  && Pops[npops][slen] == ')' ) {
		*optional = 1;
		--slen;
		strncpy(buf, &(Pops[npops++][1]), (slen <= len ? slen : len) );
	} else {
		*optional = 0;
		strncpy(buf, Pops[npops++], len);
	}
	return(TRUE);
}

/*
 * get next module to be pushed
 * return TRUE if successful, FALSE if not
 */
int
getpush(buf, len)
	char *buf;
	int len;
{
	if ( Pushes[0] == NULL || Pushes[npushes] == NULL )
		return(FALSE);
	strncpy(buf, Pushes[npushes++], len);
	return(TRUE);
}
