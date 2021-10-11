#ifndef lint
static char sccsid[] = "@(#)trace.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 *  Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/syscall.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <setjmp.h>

#include "trace.h"

#define uoffset(x) ((char *)&((struct user *)0)->x)
#define MAXSTR 32		/* Max length of string args */

extern char *errname[];
extern struct sysinfo sysinfo[];
extern int nsysent;
extern char *sigids[];
extern int nsig;

char *sprintf();
char *strcpy();
char *strncpy();
char *sprintf();
time_t time();
char * calloc();

void printcounts();
void quit();
void trace();
void before_syscall();
void after_syscall();
char *sigid();
void print();
char *argtostr();
void printstr();
void prtime();
int call_ptrace();

int detach;			/* Detach flag       */
int pid;			/* Traced process id */
int pflag;			/* Tracing a process id */
int cflag;			/* Counts only */
int tflag;			/* Print timestamps */
FILE *tracefile;		/* Trace output */
int *syscounts;			/* Sys call counts */
int *sigcounts;			/* Signal counts */
jmp_buf env;			/* for setjmp */

static struct save {		/* Holds args to system call */
    int arg;			/* Integer arg */
    char str[MAXSTR + 2];	/* String arg */
}   save[7];

main(argc, argv)
    int argc;
    char *argv[];
{
    extern char *optarg;
    extern int optind;
    int c;
    int errflg = 0;		/* Command line error flag */

    tracefile = fdopen(dup(2), "w");	/* stderr */

    while ((c = getopt(argc, argv, "o:p:sct")) != EOF)
	switch (c) {
	case 'o':		/* specify output file */
	    if (freopen(optarg, "w", tracefile) == NULL) {
		perror(optarg);
		exit(1);
	    }
	    break;

	case 'p':		/* trace process id */
	    pflag = 1;
	    pid = atoi(optarg);
	    break;

	case 'c':		/* count signals and/or system calls */
	    cflag = 1;
	    if ((syscounts = (int *) calloc((unsigned) nsysent, sizeof(int))) == 0
		|| (sigcounts = (int *) calloc((unsigned) nsig, sizeof(int))) == 0) {
		perror("malloc");
		exit(1);
	    }
	    break;

	case 't':		/* print timestamps */
	    tflag = 1;
	    break;

	default:
	    errflg++;
	}

    /* Want either pid or command - not both */

    if ((pflag == 0) == (argc == optind))
	errflg++;

    if (errflg) {
	(void) fprintf(stderr,
	  "Usage: %s [-t] [-c] [-o file]  [-p pid] | [command]\n", argv[0]);
	exit(1);
    }
    if (pflag) {		/* trace -p <process id> */
	if (call_ptrace(PTRACE_ATTACH, pid, (char *)0, 0, (char *)0) < 0) {
	    perror(argv[0]);
	    exit(1);
	}
	(void) fprintf(stderr, "Process %d attached - interrupt to quit\n", pid);
	trace(1);
	if (detach)
	    (void) fprintf(stderr, "\nProcess %d detached\n", pid);
    } else {			/* trace prog [args..] */
	switch (pid = fork()) {
	case 0:		/* child */
	    (void) call_ptrace(PTRACE_TRACEME, 0, (char *)0, 0, (char *)0);
	    (void) fclose(tracefile);
	    execvp(argv[optind], &argv[optind]);
	    perror(argv[optind]);
	    exit(1);

	case -1:		/* error */
	    perror("Couldn't fork");
	    exit(1);

	default:		/* parent */
	    trace(0);
	    break;
	}
    }

    if (cflag)
	printcounts();
    
    exit(0);
    /* NOTREACHED */
}

void
printcounts()
 /*
  * Print system call and signal counts. 
  */
{
    register int inx, max, i;

    (void) fprintf(tracefile, "\n");
    for (;;) {
	for (max = 0, i = 0; i < nsysent; i++)
	    if (syscounts[i] > max) {
		max = syscounts[i];
		inx = i;
	    }
	if (max == 0)
	    break;
	(void) fprintf(tracefile, "%8d %s\n", max, sysinfo[inx].name);
	syscounts[inx] = 0;
    }

    (void) fprintf(tracefile, "\n");
    for (;;) {
	for (max = 0, i = 0; i < nsig; i++)
	    if (sigcounts[i] > max) {
		max = sigcounts[i];
		inx = i;
	    }
	if (max == 0)
	    break;
	(void) fprintf(tracefile, "%8d %s\n", max, sigid(inx));
	sigcounts[inx] = 0;
    }
}

void
quit()
 /**
    *  User interrupted.
    *  Detach traced process.
    */
{
    if (pflag) {
	detach = 1;
	longjmp(env, 1);
    } else
	(void) kill(pid, SIGTERM);	/* suggest child exit */
}

void
trace(attached)
    int attached;
 /**
    *  Trace system calls of process with pid.
    *  Loop comprises: wait, do things to stopped process, restart.
    *  Any signal from the process will satisfy the wait.
    *  TRAP signals are from system calls.
    *  Print other signals & pass back to the process.
    *  First stop is ignored.  It's either the exec trap
    *  or the SIGSTOP after attach.  Non-zero system call code comes
    *  from trap before the system call.  Trap following system
    *  call has a zero system call code.
    *  Exit call is handled as a special case - don't want any
    *  traps when it exits into the kernel.
    */
{
    union wait status;
    int stops = 0;
    enum ptracereq request;
    int r;
    int sig;
    int code, bcode = 0;

    request = PTRACE_SYSCALL;

    signal(SIGINT, quit);
    signal(SIGTERM, quit);
    signal(SIGHUP, quit);

    if (setjmp(env)) {
        if (detach)
            (void) call_ptrace(PTRACE_DETACH, pid, (char *)1, 0, (char *)0);
        return;
    }

    for (;;) {

	while ((r = wait(&status)) != pid)	/* wait for child */
	    ;

	if (r < 0 || !(WIFSTOPPED(status)))	/* going or gone ? */
	    break;

	if (stops++ == 0) {	/* child's exec-trap or attach-stop */
	    (void) call_ptrace(request, pid, (char *)1, 0, (char *)0);
	    continue;
	}
	if ((sig = status.w_stopsig) != SIGTRAP) {	/* A signal */
	    if (cflag)		/* counts only */
		sigcounts[sig < nsig ? sig : 0]++;
	    else {
		if (tflag)	/* Print time stamp */
		    prtime(tracefile);
		(void) fprintf(tracefile, "- %s\n", sigid(sig));
	    }

	    (void) call_ptrace(request, pid, (char *)1, sig, (char *)0);
	    continue;
	}
	code = call_ptrace(PTRACE_PEEKUSER, pid, uoffset(u_arg[7]), 0, (char *)0);

	if (cflag) {		/* count calls */
	    if (code > 0)
		syscounts[code >= nsysent ? 0 : code]++;
	} else {		/* full trace */
	    if (code > 0)
		before_syscall(bcode = code);
	    else
		after_syscall(bcode);
	}

	if (code == SYS_exit) {	/* child called exit() */
	    if (!cflag)
	        fprintf(tracefile, "?\n");
	    request = attached ? PTRACE_DETACH : PTRACE_CONT;
	    (void) call_ptrace(request, pid, (char *)1, 0, (char *)0);
	    break;
	}
	(void) call_ptrace(request, pid, (char *)1, 0, (char *)0);	/* continue */
    }
}

void
before_syscall(code)
    int code;
 /**
    *  Process is stopped having just made a system call.
    *  Kernel has dumped system call code into slot for
    *  8th arg to system call.  No system calls have 8 args
    *  (fortunately). Use the code as an index to system call
    *  info and determine number and types of args.
    *  Copy args from u-area into save array. String args
    *  are dereferenced and MAXSTR+1 bytes copied from data segment.
    */
{
    int i, arg;
    enum argtype t;

    if (code >= nsysent)	/* unknown sys call ? */
	return;

    for (i = 0; i < 7; i++) {
	if ((t = sysinfo[code].args[i]) == V)
	    break;
	arg = call_ptrace(PTRACE_PEEKUSER, pid, uoffset(u_arg[i]), 0, (char *)0);
	save[i].arg = arg;
	if (t == S || t == W)	/* string pointer ? */
	    (void) call_ptrace(PTRACE_READDATA, pid, (char *)arg, MAXSTR+1, save[i].str);
    }
    print(1, code, 0, 0, save);
}

void
after_syscall(code)
    int code;
 /**
    *  Process is stopped having just completed a system call.
    *  Copy result and errno from u-area. Note: result isn't
    *  -1 for error here!  Set result = -1 if errno > 0
    *  Read buffer pointers are dereferenced and MAXSTR+1 bytes
    *  copied from data segment.
    *  Invoke print to format and print sys call info.
    */
{
    int i, err, result;
    enum argtype t;

    if (code <= 0)
	return;
    result = call_ptrace(PTRACE_PEEKUSER, pid, uoffset(u_r.r_val1), 0, (char *)0);
    err = call_ptrace(PTRACE_PEEKUSER, pid, uoffset(u_error), 0, (char *)0);
    if (err >>= 24)
	result = -1;

    if (code < nsysent)
	for (i = 0; i < 7; i++) {
	    if ((t = sysinfo[code].args[i]) == V)
		break;
	    if (t == R)		/* read buffer pointer ? */
		(void) call_ptrace(PTRACE_READDATA, pid, (char *)save[i].arg, MAXSTR+1, save[i].str);
	}

    print(0, code, result, err, save);
}

char *
sigid(signo)
    int signo;
{
    static char str[16];

    return sprintf(str, "%s (%d)",
	    signo >= nsig ? "Signal" : sigids[signo], signo);
}

void
print(before, code, result, err, save)
    int before, code, result, err;
    struct save *save;
 /**
    *  Print the system call info in readable form.
    *  System call code is converted to a name.
    *  Arguments are printed in a suitable format 
    *  according to their description in sysinfo.
    *  Error name and description are printed if
    *  result is -1.
    */
{
    struct sysinfo *info;
    enum argtype t;
    extern int sys_nerr;
    extern char *sys_errlist[];
    static int done_equal, i;

    if (before) {
        i = 0;
        done_equal = 0;
	if (tflag)
	    prtime(tracefile);			/* print timestamp */
    }

    if (code >= nsysent) {
	if (before)				/* unknown sys call */
		(void) fprintf(tracefile, "#%d (", code);
	info = &sysinfo[0];
    } else {
	info = &sysinfo[code];
	if (before)
		(void) fprintf(tracefile, "%s (", info->name);

	for (; i < 7; i++) {
	    if ((t = info->args[i]) == V)
		break;

	    switch (t) {
	    case S:
		printstr(save[i].str, -1);
		break;
	    case R:
		if (before) {
        	    fflush(tracefile);
		    return;
		}
		if (result == -1)
		    (void) fprintf(tracefile, "%s", argtostr(X, save[i].arg));
		else
		    printstr(save[i].str, result);
		break;
	    case W:
		printstr(save[i].str, save[i + 1].arg);
		break;
	    default:
		(void) fprintf(tracefile, "%s", argtostr(t, save[i].arg));
		break;
	    }
	    if (info->args[i+1] != V)
		(void) fprintf(tracefile, ", ");	/* arg separator */
	}
    }

    if (!done_equal) {
        (void) fprintf(tracefile, ") = ");
	done_equal++;
    }

    if (before) {
        fflush(tracefile);
	return;
    }

    if (result == -1) {
	(void) fprintf(tracefile, "-1");	/* result */
	if (err >= 0 && err < sys_nerr)
	    (void) fprintf(tracefile, " %s (%s)", errname[err], sys_errlist[err]);
	else
	    (void) fprintf(tracefile, " errno=%d", err);
    } else
	(void) fprintf(tracefile, "%s", argtostr(info->result, result));
    (void) fprintf(tracefile, "\n");
    (void) fflush(tracefile);
}

char *
argtostr(type, arg)
    enum argtype type;
    int arg;
 /*
  * Print arg according to format for type. 
  */
{
    static char str[32];

    switch (type) {
    case D: (void) sprintf(str, "%d", arg); break;
    case X: (void) sprintf(str, "%#x", arg); break;
    case O: (void) sprintf(str, "%#o", arg); break;
    case U: (void) sprintf(str, "%u", arg); break;
    case L: (void) sprintf(str, "%ld", arg); break;
    default: (void) sprintf(str, "?", arg); break;
    }
    return str;
}

void
printstr(str, len)
    char *str;
    int len;
 /** 
    *   Prints len bytes from str enclosed in quotes.
    *   If len is negative, length is taken from strlen(str).
    *   No more than MAXSTR bytes will be printed.  Longer
    *   strings are flagged with ".." after the closing quote.
    *   Non-printing characters are converted to C-style escape
    *   codes or octal digits.
    *   The string is not printed if more than 50% of the bytes
    *   are non-printing.  A null string followed by two dots
    *   is printed instead i.e.  ""..
    */
{
    static char tbuff[128];
    char *p, *pp;
    int printable = 0;
    int c, slen;

    str[MAXSTR+1] = '\0';
    if (len < 0)
	len = strlen(str);

    slen = MIN(len, MAXSTR);

    for (p = str, pp = tbuff; (p - str) < slen; p++)
	switch (c = *p & 0xFF) {
	case '\n': (void) strcpy(pp, "\\n"); pp += 2; break;
	case '\b': (void) strcpy(pp, "\\b"); pp += 2; break;
	case '\t': (void) strcpy(pp, "\\t"); pp += 2; break;
	case '\r': (void) strcpy(pp, "\\r"); pp += 2; break;
	case '\f': (void) strcpy(pp, "\\f"); pp += 2; break;
	default:
	    if (isascii(c) && isprint(c)) {
		*pp++ = c;
		printable++;
	    } else {
		(void) sprintf(pp, isdigit(*(p + 1)) ? "\\%03o" : "\\%o", c);
		pp += strlen(pp);
	    }
	    break;
	}

    *pp = '\0';
    if (printable < slen / 2)
	(void) fprintf(tracefile, "\"\"..");
    else {
	(void) fprintf(tracefile, "\"%s\"", tbuff);
	if (len > MAXSTR)
	    (void) fprintf(tracefile, "..");
    }
}

void
prtime(f)
    FILE *f;
{
    long tod;
    static char buff[32];

    tod = (long) time((long *)0);
    (void) fprintf(f, "%s ", strncpy(buff, (char *) ctime(&tod) + 11, 8));
}

int
call_ptrace(request, pid, addr, data, addr2)
    enum ptracereq request;
    int pid;
    char *addr;
    int data;
    char *addr2;
 /*
  * Call ptrace and check for errors. 
  */
{
    register int r;
    extern int errno;

    errno = 0;

    r = ptrace(request, pid, addr, data, addr2);

    if (errno > 0) {
	if (request == PTRACE_READDATA && errno == EIO) {	/* addr err */
	    (void) strcpy(addr2, "????");
	    return r;
	}
	perror("ptrace");
	exit(1);
    }
    return r;
}
