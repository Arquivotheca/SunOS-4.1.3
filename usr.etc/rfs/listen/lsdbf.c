/*	@(#)lsdbf.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:lsdbf.c	1.8.1.1"

/*
 * data base routines for the network listener process
 * Once initialization has completed, check_dbf should be called before
 * data base access to possibly re-read the data base file.
 * Currently, the listener does this in listen() before forking.
 */

/* system include files	*/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

/* listener include files */

#include "lsparam.h"		/* listener parameters		*/
#include "lsfiles.h"		/* listener files info		*/
#include "lserror.h"		/* listener error codes		*/
#include "lsdbf.h"		/* data base file stuff		*/

/* static data		*/

static char *dbfinitmsg = "Using data base file: %s";
static char *dbfvaldmsg = "Validating data base file: %s";
static char *dbfopenmsg = "Trouble opening data base file";
static char *dbfrderror = "Error reading data base file: line %d";
static char *dbfbadlmsg = "Data base file: Error on line %d";
static char *dbfdupcmsg = "Data base file: Duplicate service code: <%s>";
static char *dbfunknown = "Unknown error reading data base file: line %d";
static char *dbfsvccmsg = "Data base file: Illegal service code: <%s>";
static char *dbfignomsg = "Data base file ignored -- 0 entries";
static char *dbfstatmsg = "Trouble checking data base file.  (Ignored)";
static char *dbfnewdmsg = "Using new data base file";
static char *dbfcorrupt = "Data base file has been corrupted";

static int   Dbflineno;		/* current line number in dbf		*/
static unsigned Dbfentries;	/* number of non-comment lines		*/

static dbf_t *Dbfhead;		/* Dbfentries (when allocated)		*/
static char  *Server_cmd_lines; /* contains svc_code, cmd_line, mod_list */

static time_t Dbfmtime;		/* from stat(2)				*/

/*
 * init_dbf: called at initialization, to read the data base file
 *
 * check_dbf: called each time the data base is accessed (before access)
 *	      to check if the data base has been modified, and if it has
 *	      to re-read the data base.
 */

int
init_dbf()
{
	return(read_dbf(0));
}


int
check_dbf()
{
	int ret = 0;
	struct stat statbuf;

	if (stat(DBFNAME, &statbuf))  {
		logmessage(dbfstatmsg);
		sys_error(E_DBF_IO, CONTINUE);
		return(-1);
	}
	if (statbuf.st_mtime != Dbfmtime)  {
		Dbfmtime = statbuf.st_mtime;
		if (!(ret = read_dbf(1)))
			logmessage(dbfnewdmsg);
	}
	return(ret);
}

/*
 * read_dbf:
 *
 * read the data base file into internal structures
 *
 * all data base routines under read_dbf log there own errors and return -1
 * in case of an error.
 *
 * if 're_read' is non-zero, this stuff is being called to read a new 
 * data base file after the listener's initialization.
 */

read_dbf(re_read)
int	re_read;	/* zero means first time	*/
{
	register unsigned size;
	int exit_flag = EXIT | NOCORE;
	register int n;
	register dbf_t *dbf_p;
	register char  *cmd_p;
	struct stat statbuf;
	unsigned scan_dbf();
	extern char *calloc();
	extern char *Home;
	char buf[128];

	DEBUG((9,"in read_dbf"));

	/*
	 * if first time, stat the data base file so we can save the
	 * modification time for later use.
	 */

	if (check_version())
		error(E_BADVER, EXIT | NOCORE);

	if (re_read)
		exit_flag = CONTINUE;
	else   {
		if (stat(DBFNAME, &statbuf))
			sys_error(E_DBF_IO, exit_flag);
		Dbfmtime = statbuf.st_mtime;
	}

	/*
	 * note: data base routines log their own error messages
	 */

	sprintf(buf,"%s/%s",Home,DBFNAME);
	if ( (size = scan_dbf(buf)) == (unsigned)(-1) )
		error( E_SCAN_DBF, exit_flag | NO_MSG );

	DEBUG((5,"read_dbf: scan complete: non-commented lines: %u, size: %u",
		Dbfentries, size));

	if (!size)  {
		logmessage(dbfignomsg);
		return(0);
	}

	/*
	 * allocate enough space for Dbfentries of 'size' bytes (total)
	 * include enough space for default administrative entries in case
	 * they aren't mentioned in the data base.
	 */

	if (!(dbf_p = (dbf_t *)calloc(Dbfentries+N_DBF_ADMIN+1,sizeof(dbf_t)))
	   || !(cmd_p = calloc(size, 1)))  {
		DEBUG((1,"cannot calloc %u + %u bytes", size,
			(Dbfentries+N_DBF_ADMIN+1)*(unsigned)sizeof(dbf_t)));
		error( E_DBF_ALLOC, exit_flag);	/* if init, exit	*/ 

		/* if still here, this is a re-read		*/

		if (dbf_p)
			free(dbf_p);
		if (cmd_p)
			free(cmd_p);
		return(-1);
	}

	if (get_dbf(dbf_p, cmd_p))  {
		error(E_DBF_IO, exit_flag | NO_MSG);

		/* if still here, this is a re_read */

		free(dbf_p);
		free(cmd_p);
		return(-1);
	}

	if (re_read)  {
		if (Dbfhead)
			free(Dbfhead);
		if (Server_cmd_lines)
			free(Server_cmd_lines);
	}

	Dbfhead = dbf_p;
	Server_cmd_lines = cmd_p;

#ifdef	DEBUGMODE
	DEBUG((7,"read_dbf: data base dump..."));
	if (Dbfhead)
		for (dbf_p = Dbfhead; dbf_p->dbf_svc_code; ++dbf_p)
			DEBUG((7, "svc code <%s>; id: %s; reserved: %s; modules: %s; cmd line: %s", 
			dbf_p->dbf_svc_code, dbf_p->dbf_id, dbf_p->dbf_reserved, dbf_p->dbf_modules, dbf_p->dbf_cmd_line));
#endif	/* DEBUGMODE */

	return(0);
}


/*
 * get_dbf: read the file and fill the structures
 *	    checking for duplicate entries as we go
 */

get_dbf(dbf_p, cmd_p)
register dbf_t *dbf_p;
register char *cmd_p;
{
	dbf_t *dbfhead = dbf_p;
	register unsigned size;
	register int n, i;
	char buf[DBFLINESZ];
	register char *p = buf;
	char scratch[128];
	FILE *dbfilep;
	char *cmd_line_p;
	int flags;
	char *svc_code_p;
	char *id_p;
	char *res_p;
	char *module_p;
	register dbf_t *tdbf_p;
	extern int isdigits(), atoi();

	Dbflineno = 0;

	if (!(dbfilep = fopen(DBFNAME,"r")))  {
		logmessage(dbfopenmsg);
		error(E_DBF_IO, EXIT | NOCORE | NO_MSG);
	}

	while (n = rd_dbf_line(dbfilep,p,&svc_code_p,&flags,&id_p,&res_p,&module_p,&cmd_line_p))  {

		if (n == -1)  {			/* read error	*/
			fclose(dbfilep);
			return(-1);
		}

		/* make sure service code is legal 			*/

		i = strlen(svc_code_p);
		if ( (i == 0) || (i >= SVC_CODE_SZ) )
			goto reject;

		if (isdigits(svc_code_p))  {
			i = atoi(svc_code_p);
			if (i == 0)
				goto reject;
			if (flags & DBF_ADMIN)   {
				if (i > N_DBF_ADMIN)
					goto reject;
			} else if (i < MIN_REG_CODE)
				goto reject;
		}  else if (flags & DBF_ADMIN)
			goto reject;

		/* check for duplicate service code			*/

		tdbf_p = dbfhead;
		while (tdbf_p->dbf_svc_code)  {	/* duplicate svc code?	*/
			if (!strcmp(svc_code_p, tdbf_p->dbf_svc_code))  {
				sprintf(scratch, dbfdupcmsg, svc_code_p);
				logmessage(scratch);
				return(-1);
			}
			++tdbf_p;
		}

		/*
		 * legal, non-duplicate entry: copy it into internal data base
		 */

		dbf_p->dbf_flags = flags;
		strcpy(cmd_p, svc_code_p);
		dbf_p->dbf_svc_code = cmd_p;
		cmd_p += strlen(svc_code_p) + 1;
		strcpy(cmd_p, cmd_line_p);	/* copy temp to alloc'ed buf */
		dbf_p->dbf_cmd_line = cmd_p;
		cmd_p += strlen(cmd_line_p) + 1;
		strcpy(cmd_p, id_p);
		dbf_p->dbf_id = cmd_p;
		cmd_p += strlen(id_p) + 1;	/* user id + null char */
		strcpy(cmd_p, res_p);
		dbf_p->dbf_reserved = cmd_p;
		cmd_p += strlen(res_p) + 1;	/* not used, a place holder */
		strcpy(cmd_p, module_p);
		dbf_p->dbf_modules = cmd_p;
		cmd_p += strlen(module_p) + 1;	/* cmd line + null char */
		++dbf_p;

	}

	fclose(dbfilep);
	return(0);

reject:
	DEBUG((9, "svc_code <%s> failed validation test", svc_code_p));
	sprintf(scratch, dbfsvccmsg, svc_code_p);
	logmessage(scratch);
	return(-1);
}


/*
 * scan_dbf:	Take a quick pass through the data base file to figure out
 *		approx. how many items in the file we'll need to 
 *		allocate storage for.  Essentially, returns the number
 *		of non-null, non-comment lines in the data base file.
 *
 *		return: -1 == error.
 *			other == number of non-comment characters.
 *			Dbfentries set.
 */

unsigned
scan_dbf(path)
register char *path;
{
	register unsigned int size = 0;
	register int n;
	register FILE *dbfilep;
	char buf[DBFLINESZ];
	register char *p = buf;
	char *svc_code_p;
	int flags;
	char *cmd_line_p;
	char *module_p;
	char *id_p;
	char *res_p;
	extern Validate;

	DEBUG((9, "In scan_dbf.  Scanning data base file %s.", path));

	sprintf(buf, (Validate ? dbfvaldmsg: dbfinitmsg), path);
	logmessage(buf);

	if (!(dbfilep = fopen(path,"r")))  {
		logmessage(dbfopenmsg);
		return(-1);
	}

	do {
		n = rd_dbf_line(dbfilep,p,&svc_code_p,&flags,&id_p,&res_p,&module_p,&cmd_line_p);
		if (n == -1)  {
			fclose(dbfilep);
			return(-1);
		}
		size += (unsigned)n;
		if (n)
			++Dbfentries;
	} while (n);

	fclose(dbfilep);
	return(size);
}


/*
 * rd_dbf_line:	Returns the next non-comment line into the
 *		given buffer (up to DBFLINESZ bytes).
 *		Skips 'ignore' lines.
 *
 *		Returns:	0 = done, -1 = error, 
 * 				other = cmd line size incl. terminating null.
 *				*svc_code_p = service code;
 *				*id_p = user id string
 *				*res_p = reserved for future use
 *				*flags_p = decoded flags;
 *				cnd_line_p points to null terminated cmd line;
 *
 * When logging errors, the extern Dbflineno is used.
 */

int
rd_dbf_line(fp, bp, svc_code_p, flags_p, id_p, res_p, module_p, cmd_line_p)
register FILE *fp;
register char *bp;
char **svc_code_p;
int *flags_p;
char **id_p;
char **res_p;
char **module_p;
char **cmd_line_p;
{
	register int length;
	register char *p;

	do {
		++Dbflineno;
		length = 0;

		if (!fgets(bp, DBFLINESZ, fp))  {
			if (feof(fp))  {
				return(0);	/* EOF	*/
			}
			if (ferror(fp))  {
				sprintf(bp,dbfrderror,Dbflineno);
				logmessage(bp);
				return(-1);
			}
			sprintf(bp,dbfunknown,Dbflineno);
			logmessage(bp);
			return(-1);		/* Unknown error (?)	*/
		}

		if (*(bp+strlen(bp)-1) != '\n')  {  /* terminating newline? */
			sprintf(bp, dbfbadlmsg, Dbflineno);
			logmessage(bp);
			return(-1);
		}

		*(bp + strlen(bp) -1) = (char)0; /* delete newline	*/

		if (strlen(bp) && (p = strchr(bp, DBFCOMMENT)))
			*p = (char)0;		/* delete comments	*/
		if (!strlen(bp))
			continue;

		p = bp + strlen(bp) - 1;	/* bp->start; p->end	*/
		while ((p != bp) && (isspace(*p)))  {
			*p = (char)0;		/* delete terminating spaces */
			--p;
		}

		while (*bp)			/* del beginning white space*/
			if (isspace(*bp))
				++bp;
			else
				break;

		if (strlen(bp)) {		/* anything left?	*/

		   if (!(length=scan_line(bp,svc_code_p,flags_p,id_p,res_p,module_p,cmd_line_p)) ||
		     (*flags_p & DBF_UNKNOWN))  {  /* if bad line...	*/

			DEBUG((1, "rd_dbf_line line %d, unknown flag character",
			  Dbflineno));
			sprintf(bp, dbfbadlmsg, Dbflineno);
			logmessage(bp);
			return(-1);
		    }
		}

	}  while (!length);		/* until we have something */

	DEBUG((5,"rd_dbf_line: line: %d,cmd line len: %d",Dbflineno, length+1));

	return(length + 6);

}


/*
 * scan a non-white space line
 *		0 = error;
 *		other = length of cmd line;
 *
 *	non-null lines have the following format:
 *
 *	service_code: flags: id: reserved: modules: cmd_line # comments
 */

int
scan_line(bp, svc_code_p, flags_p, id_p, res_p, module_p, cmd_line_p)
register char *bp;
register char **svc_code_p;
register int *flags_p;
char **id_p;
char **res_p;
char **module_p;
register char **cmd_line_p;
{
	register char *p;
	int length;
	extern isdigits();

	*flags_p = 0;
	length = strlen(bp);

	if (!(p = strtok(bp, DBFTOKENS))) {	/* look for service code string */
		DEBUG((9,"scan_line failed 1st strtok"));
		return(0);
	}

	DEBUG((9,"scan_line: service code: <%s>", p));
	*svc_code_p = p;

	if (!(p = strtok((char *)0, DBFTOKENS)))  {
		DEBUG((9,"scan_line failed 2nd strtok"));
		return(0);
	}

	while (*p)  {
		switch (*p)  {
		case 'a':		/* administrative entry		*/
		case 'A':
			*flags_p |= DBF_ADMIN;
			break;

		case 'x':		/* service is turned off	*/
		case 'X':
			*flags_p |= DBF_OFF;
			break;

		case 'n':		/* null flag			*/
		case 'N':
			break;

		default:
			DEBUG((1,"scan_line: unknown flag char: 0x%x",*p));
			*flags_p = DBF_UNKNOWN;
			return(0);
		}
		++p;
	}

	if (!(p = strtok((char *)0, DBFTOKENS))) {
		DEBUG((9,"scan_line failed 3rd strtok"));
		return(0);
	}
	*id_p = p;

	if (!(p = strtok((char *)0, DBFTOKENS))) {
		DEBUG((9,"scan_line failed 4th strtok"));
		return(0);
	}
	*res_p = p;

	if (!(p = strtok((char *)0, DBFTOKENS))) {
		DEBUG((9,"scan_line failed 5th strtok"));
		return(0);
	}
	*module_p = p;

	p += strlen(p) + 1;			/* go past separator */

	if ((p - bp) >= length)
		return(0);

	DEBUG((9,"scan_line: modules: %s; line: %s; len: %d", *module_p, p, strlen(*module_p)+strlen(p)));

	*cmd_line_p = p;
	return(strlen(*svc_code_p)+strlen(*id_p)+strlen(*res_p)+strlen(*module_p)+strlen(p));
}


/*
 * getdbfentry:	Given a service code, return a pointer to the dbf_t
 *		entry.  Return NULL if the entry doesn't exist.
 */

dbf_t *
getdbfentry(svc_code_p)
register char *svc_code_p;
{
	register dbf_t 	*dbp = Dbfhead;

	if (!Dbfhead)		/* no services in database file */
		return((dbf_t *)0);
	if ((svc_code_p) && strlen(svc_code_p))
		for (dbp = Dbfhead;  dbp->dbf_svc_code;   ++dbp)
			if (!strcmp(dbp->dbf_svc_code, svc_code_p))
				return(dbp);

	return((dbf_t *)0);	/* svc code not in list	*/

}


/*
 * mkdbfargv:	Given a pointer to a dbf_t, construct argv
 *		for an exec system call.
 *		Warning: returns a pointer to static data which are
 *			 overritten by each call.
 *
 *		There is a practical limit of 50 arguments (including argv[0])
 *
 *		Warning: calling mkdbfargv destroys the data (by writing null
 *		characters via strtok) pointed to by dbp->dbf_cmd_line.
 */

static char *dbfargv[50];
static char *delim = " \t'\"";		/* delimiters */

char **
mkdbfargv(dbp)
register dbf_t	*dbp;
{
	register char **argvp = dbfargv;
	register char *p = dbp->dbf_cmd_line;
	char delch;
	register char *savep;
	register char *tp;
	char scratch[BUFSIZ];
	char *strpbrk();
#ifdef	DEBUGMODE
	register int i = 0;
#endif

	*argvp = 0;
	savep = p;
	while (p && *p) {
		if (p = strpbrk(p, delim)) {
			switch (*p) {
			case ' ':
			case '\t':
				/* "normal" cases */
				*p++ = '\0';
				*argvp++ = savep;
				DEBUG((9, "argv[%d] = %s", i++, savep));
				/* zap trailing white space */
				while (isspace(*p))
					p++;
				savep = p;
				break;
			case '"':
			case '\'':
				/* found a string */
				delch = *p; /* remember the delimiter */
				savep = ++p;

/*
 * We work the string in place, embedded instances of the string delimiter,
 * i.e. \" must have the '\' removed.  Since we'd have to do a compare to
 * decide if a copy were needed, it's less work to just do the copy, even
 * though it is most likely unnecessary.
 */

				tp = p;
				for (;;) {
					if (*p == '\0') {
						sprintf(scratch, "invalid command line, non-terminated string for service code %s", dbp->dbf_svc_code);
						logmessage(scratch);
						exit(2); /* server, don't log */
					}
					if (*p == delch) {
						if (*(tp - 1) == '\\') { /* \delim */
							*(tp - 1) = *p;
							p++;
						}
						else { /* end of string */
							*tp = 0;
							*argvp++ = savep;
							DEBUG((9, "argv[%d] = %s", i++, savep));
							p++;
							/* zap trailing white space */
							while (isspace(*p))
								p++;
							savep = p;
							break;
						}
					}
					else {
						*tp++ = *p++;
					}
				}
				break;
			default:
				logmessage("Internal error in parse routine");
				exit(2); /* server, don't log */
			}
		}
		else {
			*argvp++ = savep;
			DEBUG((9, "argv[%d] = %s", i++, savep));
		}
	}
	*argvp = 0;
	return(dbfargv);
}


#define VERSIONSTR	"# VERSION="

check_version()
{
	FILE *fp;
	char *line, *p, *tmp;
	int version;

	if ((fp = fopen(DBFNAME, "r")) == NULL) {
		logmessage(dbfopenmsg);
		error(E_DBF_IO, EXIT | NOCORE | NO_MSG);
	}
	if ((line = (char *) malloc(DBFLINESZ)) == NULL)
		error(E_DBF_ALLOC, EXIT | NOCORE);
	p = line;
	while (fgets(p, DBFLINESZ, fp)) {
		if (!strncmp(p, VERSIONSTR, strlen(VERSIONSTR))) {
			/* pitch the newline */
			tmp = strchr(p, '\n');
			if (tmp)
				*tmp = '\0';
			else {
				logmessage(dbfcorrupt);
				error(E_DBF_CORRUPT, EXIT | NOCORE);
			}
			p += strlen(VERSIONSTR);
			if (*p)
				version = atoi(p);
			else {
				logmessage(dbfcorrupt);
				error(E_DBF_CORRUPT, EXIT | NOCORE);
			}
			free(line);
			fclose(fp);
			if (version != VERSION)
				return(1);	/* wrong version */
			else
				return(0);	/* version ok */
		}
		p = line;
	}
	logmessage(dbfcorrupt);
	error(E_DBF_CORRUPT, EXIT | NOCORE);
}
