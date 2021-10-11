#ifndef lint
static	char sccsid[] = "@(#)main.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:main.c	1.18.2.2"
/*
 * This file contains code for the crash functions:  ?,  help,  redirect,
 * and quit,  as well as the command interpreter.
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "signal.h"
#include "sys/types.h"
#include "sys/user.h"
#include "setjmp.h"
#include "crash.h"

#define NARGS 25		/* number of arguments to one function */
#define LINESIZE 256		/* size of function input line */

int mem;			/* file descriptor for dump file */
char *namelist = "/vmunix";
char *dumpfile = "/dev/mem";
char *corfile;
struct user *ubp;			/* pointer to ublock buffer */
FILE *fp;				/* output file pointer */
FILE *rp;				/* redirect file pointer */
int opipe = 0;				/* open pipe flag */
int Procslot;				/* current process slot */
int Virtmode = 1;			/* virtual or physical mode flag*/
long pipesig;				/* pipe signal catch */

struct nlist *File, *Inode, *Mount, *Proc,  *V, *Panic,
	*Curproc, *Streams,  *Start;	/* namelist symbol pointers */

int active;	/* Flag set if crash is examining an active system */
jmp_buf	jmp, syn;	/* labels for jump */
void exit();

/* function calls */
extern int getadv(), getgdp(), getrcvd(), getsndd(), getsrmount(),
	getbufhdr(), getbuffer(), getcallout(), getdblock(),
	getinode(), getvnode(), getkfp(), getpage(), getdblk(),
	getmap(), getmess(), getmount(), getvfs(), getnm(), getod(), getpcb(),
	getproc(), getqrun(), getqueue(), getquit(),
	getas(),  getseg(),  getsegdata(),  getpmgrp(),  getctx(),  getpment(),
	getstack(), getstat(), getstream(), getstrstat(), gettrace(), getsymbol(),
	getuser(), getvtop(), getfuncs(), getbase(), gethelp(),
	getsearch(), getdbfree(), getmbfree(), getsearch(), getfile(), getdefproc(),
	getmode(), getredirect(), getsize(),
	getfindaddr(), getlinkblk();

/* function definition */
struct func {
	char *name;
	char *syntax;
	int (*call)();
	char *description;
};

/* function table */
struct func functab[] = {
	"adv","[-e] [-p] [-wfilename] [tbl_entry[s]]",
		getadv,"advertise table",
	"as", "[-wfilename] [([-p] proc_entry | #procid)[s])]",
		getas, "process address spaces",
	"b", " ", getbuffer, "(buffer)",
	"base", "[-wfilename] number[s]",
		getbase, "base conversions",
	"buf", " ", getbufhdr, "(bufhdr)",
	"buffer", "[-wfilename] [-b|-c|-d|-x|-o|-i|-r|-s] ([-p] st_addr)",
		getbuffer, "buffer data",
	"bufhdr", "[-f] [-wfilename] [[-p] tbl_entry[s]]",
		getbufhdr, "buffer headers",
	"c", " ", getcallout, "(callout)",
	"callout", "[-wfilename]",
		getcallout, "callout table",
	"ctx", "[-wfilename] [[-p] tbl_entry[s]]",
		getctx, "context table",
	"dbfree", "[-wfilename]",
		getdbfree, "free data block headers",
	"dblock", "[-e] [-wfilename] [[-p] dblk_addr[s]]",
		getdblk, "allocated stream data block headers",
	"defproc", "[-wfilename] [-c | slot]",
		getdefproc, "set default process slot",
	"ds", "[-wfilename] virtual_address[es]",
		getsymbol, "data address namelist search",
	"f", " ", getfile, "(file)",
	"file", "[-e] [-wfilename] [[-p] tbl_entry[s]]",
		getfile, "file table",
	"findaddr", "[-wfilename] table slot",
		getfindaddr, "find address for given table and slot",
	"gdp","[-e] [-f] [-p] [-wfilename] [tbl_entry[s]]",
		getgdp,"gdp structure",
	"help", "[-wfilename] function[s]",
		gethelp, "help function",
	"i", " ", getinode, "(inode)",
	"inode", "[-f] [-wfilename]",
		getinode, "inode table",
	"kfp", "[-wfilename] [-sprocess] [-r | value]",
		getkfp, "frame pointer for start of stack trace",
	"linkblk", "[-e] [-wfilename] [[-p] tbl_entry[s]]",
		getlinkblk, "linkblk table",
	"m", " ", getmount, "(mount)",
	"map", "[-wfilename] mapname[s]",
		getmap, "map structures",
	"mbfree", "[-wfilename]",
		getmbfree, "free message block headers",
	"mblock", "[-e] [-wfilename] [[-p] mblk_addr[s]]",
		getmess, "allocated stream message block headers",
	"mode", "[-wfilename] [v | p]",
		getmode, "address mode",
	"mount", "[-wfilename] [[-p] tbl_entry[s]]",
		getmount, "mount table",
	"nm", "[-wfilename] symbol[s]",
		getnm, "name search",
	"od", "[-wfilename] [-c|-d|-x|-o|-a|-h] [-l|-t|-b] [-sprocess] [-p] st_addr [count]",
		getod, "dump symbol values",
	"p", " ", getproc, "(proc)",
	"page", "[-e] [-wfilename] [[-p] tbl_entry[s]]",
		getpage, "page structure",
	"pcb", "[-wfilename] [process]",
		getpcb, "process control block",
	"pment", "[-wfilename] [[-p] tbl_entry[s]]",
		getpment, "page map entry table",
	"pmgrp", "[-wfilename] [[-p] tbl_entry[s]]",
		getpmgrp, "page map entry group table",
	"proc", "[-f] [-wfilename] [([-p] tbl_entry | #procid)[s] | -r)]",
		getproc, "process table",
	"q", " ", getquit, "(quit)",
	"qrun", "[-wfilename]",
		getqrun, "list of servicable stream queues",
	"queue", "[-wfilename] [[-p] queue_addr[s]]",
		getqueue, "stream queues",
	"quit", " ",
		getquit, "exit",
	"rcvd","[-e] [-f] [-p] [-wfilename] [tbl_entry[s]]",
		getrcvd,"receive descriptor",
	"rd", " ", getod, "(od)",
	"redirect", "[-wfilename] [-c | filename]",
		getredirect, "output redirection",
	"s", " ", getstack, "(stack)",
	"search", "[-wfilename] [-mmask] [-sprocess] pattern [-p] st_addr length",
		getsearch, "memory search",
	"seg", "[-wfilename] [([-p] proc_entry | #procid)[s])]",
		getseg, "process segments",
	"segdata", "[-wfilename] [([-p] proc_entry | #procid)[s])]",
		getsegdata, "process segment data",
	"sndd","[-e] [-p] [-f] [-wfilename] [tbl_entry[s]]",
		getsndd,"send descriptor",
	"size", "[-x] [-wfilename] structurename[s]",
		getsize, "symbol size",
	"srmount","[-e] [-p] [-wfilename] [tbl_entry[s]]",
		getsrmount,"server mount table",
	"stack", "[-wfilename] [[-u | -k] [process] | -i [-p] st_addr]",
		getstack, "stack dump",
	"status", "[-wfilename]",
		getstat, "system status",
	"stream", "[-e] [-f] [-wfilename] [[-p] stream_addr[s]]",
		getstream, "allocated stream table slots",
	"strstat", "[-wfilename]",
		getstrstat, "streams statistics",
	"t", " ", gettrace, "(trace)",
	"trace", "[-wfilename] [[-r] [process] | -i [-p] st_addr]",
		gettrace, "kernel stack trace",
	"ts", "[-wfilename] virtual_address[es]",
		getsymbol, "text address namelist search",
	"u", " ", getuser, "(user)",
	"user", "[-f] [-wfilename] [process]",
		getuser, "uarea",
	"v", " ", getvnode, "(vnode)",
	"vfs", "[-wfilename] [[-p] tbl_entry[s]]",
		getvfs, "vfs table",
	"vnode", "[-wfilename] [[-p] addr]",
		getvnode, "vnode",
	"vtop", "[-wfilename] [-sprocess] st_addr[s]",
		getvtop, "virtual to physical address",
	"?", "[-wfilename]",
		getfuncs, "print list of available commands",
	"!cmd", " ", NULL, "escape to shell",
	NULL, NULL, NULL, NULL
};

char *args[NARGS];		/* argument array */
int argcnt;			/* argument count */
char outfile[100];		/* output file for redirection */
static int tabsize;		/* size of function table */

/* main program with call to functions */
main(argc, argv)
int argc;
char **argv;
{
	struct func *a, *f;
	int c, i, found;
	extern int opterr;
	int arglength;

	corfile = dumpfile;
	if (setjmp(jmp))
		exit(1);
	fp = stdout;
	strcpy(outfile, "stdout");
	optind = 1;		/* remove in next release */
	opterr = 0;		/* suppress getopt error messages */

	for (tabsize = 0, f = functab; f->name; f++, tabsize++)
		;

	while ((c = getopt(argc, argv, "d:n:w:")) !=EOF) {
		switch(c) {
			case 'd' :	dumpfile = optarg;
				 	break;
			case 'n' : 	namelist = optarg;
					break;
			case 'w' : 	strncpy(outfile, optarg, ARGLEN);
					if (!(rp = fopen(outfile, "a")))
						fatal("unable to open %s\n",
							outfile);
					break;
			default  :	fatal("usage: crash [-d dumpfile] [-n namelist] [-w outfile]\n");
		}
	}
	/* backward compatible code */
	if (argv[optind]) {
		dumpfile = argv[optind++];
		if (argv[optind])
			namelist = argv[optind++];
		if (argv[optind])
			fatal("usage: crash [-d dumpfile] [-n namelist] [-w outfile]\n");
	}
	/* remove in SVnext release */
	if (rp)
		fprintf(rp, "dumpfile = %s,  namelist = %s,  outfile = %s\n", dumpfile, namelist, outfile);
	fprintf(fp, "dumpfile = %s,  namelist = %s,  outfile = %s\n", dumpfile, namelist, outfile);
	init();
#ifdef sparc
	stack_init();
#endif

	setjmp(jmp);

	for (;;) {
		getcmd();
		if (argcnt == 0)
			continue;
		if (rp) {
			fp = rp;
			fprintf(fp, "\n> ");
			for (i = 0;i<argcnt;i++)
				fprintf(fp, "%s ", args[i]);
			fprintf(fp, "\n");
		}
		found = 0;
		for (f = functab; f->name; f++) 
			if (!strcmp(f->name, args[0])) {
				found = 1;
				break;
			}
		if (!found) {
			arglength = strlen(args[0]);
			for (f = functab; f->name; f++) {
				if (!strncmp(f->name, args[0], arglength)) {
					found++;
					a = f;
				}
			}
			if (found) {
				if (found > 1)
					error("%s is an ambiguous function name\n", args[0]);
				else f = a;
			}	
		}
		if (found) {
			if (setjmp(syn)) {
				while (getopt(argcnt, args, "") != EOF);
				if (*f->syntax == ' ') {
					for (a = functab;a->name;a++)
						if ((a->call == f->call) &&
						(*a->syntax != ' '))
							error("%s: usage: %s %s\n", f->name, f->name, a->syntax);
				}
				else error("%s: usage: %s %s\n", f->name, f->name, f->syntax);
			}
			else (*(f->call))();
		}
		else prerrmes("unrecognized function name\n");
		fflush(fp);
		resetfp();
	}
}

/* returns argcnt,  and args contains all arguments */
int
getcmd()
{
	char *p;
	int i;
	static char line[LINESIZE+1];
	FILE *ofp;
	
	ofp = fp;
	printf("> ");
	fflush(stdout);
	if (fgets(line, LINESIZE, stdin) == NULL)
		exit(0);
	line[LINESIZE] = '\n';
	p = line;
	while (*p == ' ' || *p == '\t') {
		p++;
	}
	if (*p == '!') {
		system(p+1);
		argcnt = 0;
	}
	else {
		for (i = 0; i < NARGS; i++) {
			if (*p == '\n') {
				*p = '\0';
				break;
			}
			while (*p == ' ' || *p == '\t')
				p++;
			args[i] = p;
			if (strlen(args[i]) == 1)
				break;
			if (*p == '!') {
				p = args[i];
				if (strlen(++args[i]) == 1)
					error("no shell command after '!'\n");
				pipesig = (long)signal(SIGPIPE, SIG_IGN);
				if ((fp = popen(++p, "w")) == NULL) {
					fp = ofp;
					error("cannot open pipe\n");
				}
				if (rp != NULL)
					error("cannot use pipe with redirected output\n");
				opipe = 1;
				break;
			}
			if (*p == '(')
				while ((*p != ')') && (*p != '\n'))
					p++;
			while (*p != ' ' && *p != '\n')
				p++;
			if (*p == ' ' || *p == '\t')
				*p++ = '\0';
		}
		args[i] = NULL;
		argcnt = i;
	}
}


/* get arguments for ? function */
int
getfuncs()
{
	int c;

	while ((c = getopt(argcnt, args, "w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	prfuncs();
}

/* print all function names in columns */
int
prfuncs()
{
	int i, j, len;
	struct func *ff;
	char tempbuf[20];

	len = (tabsize + 3) / 4;
	for (i = 0; i < len; i++) {
		ff = functab + i;
		for (j = 0; j < 4; j++) {
			if (*ff->description != '(')
				fprintf(fp, "%-15s", ff->name);
			else {
				tempbuf[0] = 0;
				strcat(tempbuf, ff->name);
				strcat(tempbuf, " ");
				strcat(tempbuf, ff->description);
				fprintf(fp, "%-15s", tempbuf);
			}
			ff += len;
			if ((ff - functab) >= tabsize)
				break;
		}
		fprintf(fp, "\n");
	}
	fprintf(fp, "\n");
}

/* get arguments for help function */
int
gethelp()
{
	int c;

	optind = 1;
	while ((c = getopt(argcnt, args, "w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (args[optind]) {
		do {
			prhelp(args[optind++]);
		}while (args[optind]);
	}
	else prhelp("help");
}

/* print function information */
int
prhelp(string)
char *string;
{
	int found = 0;
	struct func *ff, *a, *aa;

	for (ff=functab;ff->name;ff++) {
		if (!strcmp(ff->name, string)){
			found = 1;
			break;
		}
	}
	if (!found)
		error("%s does not match in function list\n", string);
	if (*ff->description == '(') {
		for (a = functab;a->name != NULL;a++)
			if ((a->call == ff->call) && (*a->description != '('))
					break;
		fprintf(fp, "%s %s\n", ff->name, a->syntax);
		if (findstring(a->syntax, "tbl_entry"))
			fprintf(fp, "\ttbl_entry = slot number | address | symbol | expression | range\n");
		if (findstring(a->syntax, "st_addr"))
			fprintf(fp, "\tst_addr = address | symbol | expression\n");
		fprintf(fp, "%s\n", a->description);
	}
	else {
		fprintf(fp, "%s %s\n", ff->name, ff->syntax);
		if (findstring(ff->syntax, "tbl_entry"))
			fprintf(fp, "\ttbl_entry = slot number | address | symbol | expression | range\n");
		if (findstring(ff->syntax, "st_addr"))
			fprintf(fp, "\tst_addr = address | symbol | expression\n");
		fprintf(fp, "%s\n", ff->description);
	}
	fprintf(fp, "alias: ");
	for (aa = functab;aa->name != NULL;aa++)
		if ((aa->call == ff->call) && (strcmp(aa->name, ff->name)) &&
			strcmp(aa->description, "NA"))
				fprintf(fp, "%s ", aa->name);
	fprintf(fp, "\n");
	fprintf(fp, "\tacceptable aliases are uniquely identifiable initial substrings\n");
}

/* find tbl_entry or st_addr in syntax string */
int
findstring(syntax, substring)
char *syntax;
char *substring;
{
	char string[81];
	char *token;

	strcpy(string, syntax);
	token = strtok(string, "[] ");
	while (token) {
		if (!strcmp(token, substring))
			return(1);
		token = strtok(NULL, "[] ");
	}
	return(0);
}

/* terminate crash session */
int
getquit()
{
	if (rp)
		fclose(rp);
	exit(0);
}

/* get arguments for redirect function */
int
getredirect()
{
	int c;
	int close = 0;

	optind = 1;
	while ((c = getopt(argcnt, args, "w:c")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'c' :	close = 1;
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (args[optind]) 
		prredirect(args[optind], close);
	else prredirect(NULL, close);
}

/* print results of redirect function */
int
prredirect(string, close)
char *string;
int close;
{
	if (close)
		if (rp) {
			fclose(rp);
			rp = NULL;
			strcpy(outfile, "stdout");
			fp = stdout;
		}
	if (string) {
		if (rp) {
			fclose(rp);
			rp = NULL;
		}
		if (!(rp = fopen(string, "a")))
			error("unable to open %s\n", string);
		fp = rp;
		strncpy(outfile, string, ARGLEN);
	}
	fprintf(fp, "outfile = %s\n", outfile);
	if (rp)
		fprintf(stdout, "outfile = %s\n", outfile);
}
