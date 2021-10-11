/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)msg.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.15 */
#endif

/*
 *	UNIX shell
 */


#include	"defs.h"
#include	"sym.h"

/*
 * error messages
 */
char	badopt[]	= "bad option(s)";
char	mailmsg[]	= "you have mail\n";
char	nospace[]	= "no space";
char	nostack[]	= "no stack space";
char	synmsg[]	= "syntax error";

char	badnum[]	= "bad number";
char	badparam[]	= "parameter null or not set";
char	unset[]		= "parameter not set";
char	badsub[]	= "bad substitution";
char	nofork[]	= "fork failed - too many processes";
char	noswap[]	= "cannot fork: no swap space";
char	restricted[]	= "restricted";
char	piperr[]	= "cannot make pipe";
char	coredump[]	= " - core dumped";
char	badexec[]	= "cannot execute";
char	notfound[]	= "not found";
char	badfile[]	= "bad file number";
char	badshift[]	= "cannot shift";
char	baddir[]	= "bad directory";
char	badtrap[]	= "bad trap";
char	wtfailed[]	= "is read only";
char	notid[]		= "is not an identifier";
char 	badulimit[]	= "bad ulimit";
char	badreturn[] 	= "cannot return when not in function";
char	badexport[] 	= "cannot export functions";
char	badunset[] 	= "cannot unset";
char	nohome[]	= "no home directory";
char 	badperm[]	= "execute permission denied";
char	longpwd[]	= "sh error: pwd too long";
char	mssgargn[]	= "missing arguments";
char	libacc[] 	= "can't access a needed shared library";
char	libbad[]	= "accessing a corrupted shared library";
char	libscn[]	= ".lib section in a.out corrupted";
char	libmax[]	= "attempting to link in too many libs";
char    nulldir[]       = "null directory";

/*
 * messages for 'builtin' functions
 */
char	btest[]		= "test";
char	badop[]		= "unknown operator ";
/*
 * built in names
 */
char	pathname[]	= "PATH";
char	cdpname[]	= "CDPATH";
char	homename[]	= "HOME";
char	mailname[]	= "MAIL";
char	ifsname[]	= "IFS";
char	ps1name[]	= "PS1";
char	ps2name[]	= "PS2";
char	mchkname[]	= "MAILCHECK";
char	acctname[]  	= "SHACCT";
char	mailpname[]	= "MAILPATH";

/*
 * string constants
 */
char	nullstr[]	= "";
char	sptbnl[]	= " \t\n";
char	defpath[]	= ":/usr/ucb:/bin:/usr/bin";
char	colon[]		= ": ";
char	minus[]		= "-";
char	endoffile[]	= "end of file";
char	unexpected[] 	= " unexpected";
char	atline[]	= " at line ";
char	devnull[]	= "/dev/null";
char	execpmsg[]	= "+ ";
char	readmsg[]	= "> ";
char	stdprompt[]	= "$ ";
char	supprompt[]	= "# ";
char	profile[]	= ".profile";
char	sysprofile[]	= "/etc/profile";

/*
 * tables
 */

struct sysnod reserved[] =
{
	{ "case",	CASYM	},
	{ "do",		DOSYM	},
	{ "done",	ODSYM	},
	{ "elif",	EFSYM	},
	{ "else",	ELSYM	},
	{ "esac",	ESSYM	},
	{ "fi",		FISYM	},
	{ "for",	FORSYM	},
	{ "if",		IFSYM	},
	{ "in",		INSYM	},
	{ "then",	THSYM	},
	{ "until",	UNSYM	},
	{ "while",	WHSYM	},
	{ "{",		BRSYM	},
	{ "}",		KTSYM	}
};

int no_reserved = 15;

char	*sysmsg[] =
{
	0,
	"Hangup",
	0,	/* Interrupt */
	"Quit",
	"Illegal instruction",
	"Trace/BPT trap",
	"Abort",
	"Emulator trap",
	"Arithmetic exception",
	"Killed",
	"Bus error",
	"Memory fault",
	"Bad system call",
	0,	/* Broken pipe */
	"Alarm call",
	"Terminated",
	"Urgent condition",
	"Stopped",
	"Stopped from terminal",
	"Continued",
	"Child terminated",
	"Stopped on terminal input",
	"Stopped on terminal output",
	"Asynchronous I/O",
	"Exceeded cpu time limit",
	"Exceeded file size limit",
	"Virtual time alarm",
	"Profiling time alarm",
	"Window changed",
	"Resource lost",
	"Signal 30",
	"Signal 31",
	"Signal 32",
};

char	export[] = "export";
char	duperr[] = "cannot dup";
char	readonly[] = "readonly";


struct sysnod commands[] =
{
	{ ".",		SYSDOT	},
	{ ":",		SYSNULL	},
	{ "[",		SYSTST },
	{ "break",	SYSBREAK },
	{ "cd",		SYSCD	},
	{ "chdir",	SYSCD	},
	{ "continue",	SYSCONT	},
	{ "echo",	SYSECHO },
	{ "eval",	SYSEVAL	},
	{ "exec",	SYSEXEC	},
	{ "exit",	SYSEXIT	},
	{ "export",	SYSXPORT },
	{ "getopts",	SYSGETOPT },
	{ "hash",	SYSHASH	},

	{ "login",	SYSLOGIN },
	{ "newgrp",	SYSNEWGRP },

	{ "pwd",	SYSPWD },
	{ "read",	SYSREAD	},
	{ "readonly",	SYSRDONLY },
	{ "return",	SYSRETURN },
	{ "set",	SYSSET	},
	{ "shift",	SYSSHFT	},
	{ "test",	SYSTST },
	{ "times",	SYSTIMES },
	{ "trap",	SYSTRAP	},
	{ "type",	SYSTYPE },


#ifndef RES		
	{ "ulimit",	SYSULIMIT },
#endif

	{ "umask",	SYSUMASK },
	{ "unset", 	SYSUNS },
	{ "wait",	SYSWAIT	}
};

int no_commands = sizeof commands / sizeof(struct sysnod);
