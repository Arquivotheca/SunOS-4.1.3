/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)cu.c 1.1 92/07/30"	/* from SVR3.2 cu:cu.c 2.23 */

/********************************************************************
 *cu [-sspeed] [-lline] [-h] [-t] [-d] [-n] [-o|-e] telno | systemname
 *
 *	legal baud rates: 300, 1200, 2400, 4800, 9600.
 *
 *	-l is for specifying a line unit from the file whose
 *		name is defined in /usr/lib/uucp/Devices.
 *	-h is for half-duplex (local echoing).
 *	-t is for adding CR to LF on output to remote (for terminals).
 *	-d can be used  to get some tracing & diagnostics.
 *	-o or -e is for odd or even parity on transmission to remote.
 *	-n will request the phone number from the user.
 *	Telno is a telephone number with `=' for secondary dial-tone.
 *	If "-l dev" is used, speed is taken from /usr/lib/uucp/Devices.
 *	Only systemnames that are included in /usr/lib/uucp/Systems may
 *	be used.
 *
 *	Escape with `~' at beginning of line.
 *	Silent output diversions are ~>:filename and ~>>:filename.
 *	Terminate output diversion with ~> alone.
 *	~. is quit, and ~![cmd] gives local shell [or command].
 *	Also ~$ for local procedure with stdout to remote.
 *	Both ~%put from [to]  and  ~%take from [to] invoke built-ins.
 *	Also, ~%break or just ~%b will transmit a BREAK to remote.
 *	~%nostop toggles on/off the DC3/DC1 input control from remote,
 *		(certain remote systems cannot cope with DC3 or DC1).
 *	~%debug(~%b) toggles on/off the program tracing and diagnostics output;
 *	cu should no longer be compiled with #ifdef ddt.
 *
 *	Cu no longer uses dial.c to reach the remote.  Instead, cu places
 *	a telephone call to a remote system through the uucp conn() routine
 *	when the user picks the systemname option or through altconn()--
 *	which bypasses /usr/lib/uucp/Systems -- if a telno or direct
 *	line is chosen. The line termio attributes are set in fixline(),
 *	before the remote connection is made.  As a device-lockout semaphore
 *	mechanism, uucp creates an entry in /usr/spool/locks whose name is
 *	LCK..dev where dev is the device name from the Devices file.
 *	When cu terminates, for whatever reason, cleanup() must be
 *	called to "release" the device, and clean up entries from
 *	the locks directory.  Cu runs with uucp ownership, and thus provides
 *	extra insurance that lock files will not be left around.	
# ******************************************************************/

#include "uucp.h"
#include <string.h>

#define MID	BUFSIZ/2	/* mnemonic */
#define	RUB	'\177'		/* mnemonic */
#define	XON	'\21'		/* mnemonic */
#define	XOFF	'\23'		/* mnemonic */
#define	TTYIN	0		/* mnemonic */
#define	TTYOUT	1		/* mnemonic */
#define	TTYERR	2		/* mnemonic */
#define	YES	1		/* mnemonic */
#define	NO	0		/* mnemonic */
#define IOERR	4		/* exit code */
#define	MAXPATH	100
#define	NPL	50

int Sflag=0;
int Cn;				/*fd for remote comm line */
static char *_Cnname;			/*to save associated ttyname */
jmp_buf Sjbuf;			/*needed by uucp routines*/

/*	io buffering	*/
/*	Wiobuf contains, in effect, 3 write buffers (to remote, to tty	*/
/*	stdout, and to tty stderr) and Riobuf contains 2 read buffers	*/
/*	(from remote, from tty).  [WR]IOFD decides which one to use.	*/
/*	[RW]iop holds current position in each.				*/
#define WIOFD(fd)	(fd == TTYOUT ? 0 : (fd == Cn ? 1 : 2))
#define RIOFD(fd)	(fd == TTYIN ? 0 : 1)
#define WRIOBSZ 256
static char Riobuf[2*WRIOBSZ];
static char Wiobuf[3*WRIOBSZ];
static int Riocnt[2]={0,0};
static char *Riop[2]; 
static char *Wiop[3]; 

extern int
	errno,			/* supplied by system interface */
	optind;			/* variable in getopt() */


extern char
	*optarg;

static struct call {		/*NOTE-also included in altconn.c--> */
				/*any changes must be made in both places*/
	char *speed;		/* transmission baud rate */
	char *line;		/* device name for outgoing line */
	char *telno;		/* ptr to tel-no digit string */
	char *class;		/* call class */
	}Cucall;
	
static int Saved_tty;		/* was TCGETAW of _Tv0 successful?	*/
static struct termio _Tv, _Tv0;	/* for saving, changing TTY atributes */
static struct termio _Lv;	/* attributes for the line to remote */

static char
	_Cxc,			/* place into which we do character io*/
	_Tintr,			/* current input INTR */
	_Tquit,			/* current input QUIT */
	_Terase,		/* current input ERASE */
	_Tkill,			/* current input KILL */
	_Teol,			/* current secondary input EOL */
	_Myeof;			/* current input EOF */

int
	Terminal=0,		/* flag; remote is a terminal */
	Oddflag = 0,		/*flag- odd parity option*/
	Evenflag = 0,		/*flag- even parity option*/
	Echoe,			/* save users ECHOE bit */
	Echok,			/* save users ECHOK bit */
	Child,			/* pid for receive process */
	Intrupt=NO,		/* interrupt indicator */
	Duplex=YES,		/* Unix= full duplex=YES; half = NO */ 
	Sstop=YES,		/* NO means remote can't XON/XOFF */
	Rtn_code=0,		/* default return code */
	Takeflag=NO,		/* indicates a ~%take is in progress */
	w_char(),		/* local io routine */
	r_char();		/* local io routine */

static void
	_onintrpt(),		/* interrupt routines */
	_rcvdead(),
	_quit(),
	_bye();

extern void	cleanup();
extern void     tdmp();

static void
	_flush(),
	_shell(),
	_dopercen(),
	_receive(),
	_mode(),
	_w_str();

char *Myline = NULL;  /* flag to force the requested line to be used  */
		      /* by rddev() in uucp conn.c		    */

char *P_USAGE= "USAGE:%s [-s speed] [-l line] [-h] [-n] [-t] [-d] [-o|-e] telno | systemname\n";
char *P_CON_FAILED = "Connect failed: %s\r\n";
char *P_Ct_OPEN = "Cannot open: %s\r\n";
char *P_LINE_GONE = "Remote line gone\r\n";
char *P_Ct_EXSH = "Can't execute shell\r\n";
char *P_Ct_DIVERT = "Can't divert %s\r\n";
char *P_STARTWITH = "Use `~~' to start line with `~'\r\n";
char *P_CNTAFTER = "after %ld bytes\r\n";
char *P_CNTLINES = "%d lines/";
char *P_CNTCHAR = "%ld characters\r\n";
char *P_FILEINTR = "File transmission interrupted\r\n";
char *P_Ct_FK = "Can't fork -- try later\r\n";
char *P_Ct_SPECIAL = "r\nCan't transmit special character `%#o'\r\n";
char *P_TOOLONG = "\nLine too long\r\n";
char *P_IOERR = "r\nIO error\r\n";
char *P_USECMD = "Use `~$'cmd \r\n"; 
char *P_USEPLUSCMD ="Use `~+'cmd \r\n";
char *P_NOTERMSTAT = "Can't get terminal status\r\n";
char *P_3BCONSOLE = "Sorry, you can't cu from a 3B console\r\n";
char *P_PARITY  = "Parity option error\r\n";
char *P_TELLENGTH = "Telno cannot exceed 58 digits!\r\n";

/***************************************************************
 *	main: get command line args, establish connection, and fork.
 *	Child invokes "receive" to read from remote & write to TTY.
 *	Main line invokes "transmit" to read TTY & write to remote.
 ***************************************************************/

main(argc, argv)
char *argv[];
{
	extern void setservice();
	extern int sysaccess();
	struct stat buff;
	struct stat bufsave;
	char s[MAXPH];
	char *string;
	int i, j =0;
	int cflag=0;
	int errflag=0;
	int lflag=0;
	int nflag=0;
	int systemname = 0;

	Riop[0] = &Riobuf[0];
	Riop[1] = &Riobuf[WRIOBSZ];
	Wiop[0] = &Wiobuf[0];
	Wiop[1] = &Wiobuf[WRIOBSZ];
	Wiop[2] = &Wiobuf[2*WRIOBSZ];

	Verbose = 1;		/*for uucp callers,  dialers feedback*/
	strcpy(Progname,"cu");
	setservice(Progname);
	if ( sysaccess(EACCESS_SYSTEMS) != 0 ) {
		(void)fprintf(stderr, "cu: cannot read Systems files\n");
		exit(1);
	}
	if ( sysaccess(EACCESS_DEVICES) != 0 ) {
		(void)fprintf(stderr, "cu: cannot read Devices files\n");
		exit(1);
	}
	if ( sysaccess(EACCESS_DIALERS) != 0 ) {
		(void)fprintf(stderr, "cu: cannot read Dialers files\n");
		exit(1);
	}

	Cucall.speed = "Any";       /*default speed*/
	Cucall.line = NULL;
	Cucall.telno = NULL;
	Cucall.class = NULL;
		
/*Flags for -h, -t, -e, and -o options set here; corresponding line attributes*/
/*are set in fixline() in culine.c before remote connection is made           */

	while((i = getopt(argc, argv, "dhteons:l:c:")) != EOF)
		switch(i) {
			case 'd':
				Debug = 9; /*turns on uucp debugging-level 9*/
				break;
			case 'h':
				Duplex  = NO;
				Sstop = NO;
				break;
			case 't':
				Terminal = YES;
				break;
			case 'e':
				if ( Oddflag ) {
					(void)fprintf(stderr, "%s: cannot have both even and odd parity\n", argv[0]);
					exit(1);
				}
				Evenflag = 1;
				break;
			case 'o':
				if ( Evenflag ) {
					(void)fprintf(stderr, "%s: cannot have both even and odd parity\n", argv[0]);
					exit(1);
				}
				Oddflag = 1;
				break;
			case 's':
				Sflag++;
				Cucall.speed = optarg;
			  	break;
			case 'l':
				lflag++;
				Cucall.line = optarg;
				break;
#ifdef forfutureuse
/* -c specifies the class-selecting an entry type from the Devices file */
			case 'c':
				Cucall.class = optarg;
				break;
#endif
			case 'n':
				nflag++;
				printf("Please enter the number: ");
				gets(s);
				break;
			case '?':
				++errflag;
		}

#ifdef  u3b
	if(fstat(TTYIN, &buff) < 0) {
		VERBOSE(P_NOTERMSTAT,"");
		exit(1);
	} else if ( (buff.st_mode & S_IFMT) == S_IFCHR && buff.st_rdev == 0 ) {
		VERBOSE(P_3BCONSOLE,"");
		exit(1);
		}
#endif

	if((optind < argc && optind > 0) || (nflag && optind > 0)) {  
		if(nflag) 
			string=s;
		else
			string = argv[optind];
		if((strlen(string) == strspn(string, "0123456789=-*#")) ||
		   (Cucall.class != NULL)) {
			Cucall.telno = string;
			if(strlen(Cucall.telno) >= (MAXPH - 1)) {
				VERBOSE(P_TELLENGTH,"");
				exit(0);
			}
		} else { 		/*if it's not a legitimate telno,  */
					/*then it should be a systemname   */
			if ( nflag ) {
				(void)fprintf(stderr,
				"%s: bad phone number %s\nPhone numbers may contain only the digits 0 through 9 and the special\ncharacters =, -, * and #.\n",
				argv[0], string );
				exit(1);
			}
			systemname++;
		}
	} else
		if(Cucall.line == NULL)   /*if none of above, must be direct */
			++errflag;
	
	if(errflag) {
		VERBOSE(P_USAGE, argv[0]);
		exit(1);
	}

	/* save initial tty state */
	Saved_tty = ( ioctl(TTYIN, TCGETA, &_Tv0) == 0 );
	_Tintr = _Tv0.c_cc[VINTR]? _Tv0.c_cc[VINTR]: '\377';
	_Tquit = _Tv0.c_cc[VQUIT]? _Tv0.c_cc[VQUIT]: '\377';
	_Terase = _Tv0.c_cc[VERASE]? _Tv0.c_cc[VERASE]: '\377';
	_Tkill = _Tv0.c_cc[VKILL]? _Tv0.c_cc[VKILL]: '\377';
	_Teol = _Tv0.c_cc[VEOL]? _Tv0.c_cc[VEOL]: '\377';
	_Myeof = _Tv0.c_cc[VEOF]? _Tv0.c_cc[VEOF]: '\04';
	Echoe = _Tv0.c_lflag & ECHOE;
	Echok = _Tv0.c_lflag & ECHOK;

	(void)signal(SIGHUP, cleanup);
	(void)signal(SIGQUIT, cleanup);
	(void)signal(SIGINT, cleanup);

/* place call to system; if "cu systemname", use conn() from uucp
   directly.  Otherwise, use altconn() which dummies in the
   Systems file line.
*/

	if(systemname) {
		if ( lflag )
			(void)fprintf(stderr,
			"%s: warning: -l flag ignored when system name used\n",
			argv[0]);
		if ( Sflag )
			(void)fprintf(stderr,
			"%s: warning: -s flag ignored when system name used\n",
			argv[0]);
		Cn = conn(string);
	} else
		Cn = altconn(&Cucall);

	_Cnname = ttyname(Cn);
	if(_Cnname != NULL) {
		struct stat Cnsbuf;
		if ( fstat(Cn, &Cnsbuf) == 0 )
			Dev_mode = Cnsbuf.st_mode;
		else
			Dev_mode = R_DEVICEMODE;
		chmod(_Cnname, M_DEVICEMODE);
	}

	Euid = geteuid();
	if(setuid(getuid()) && setgid(getgid()) < 0) {
		VERBOSE("Unable to setuid/gid\n","");
		cleanup();
		}

	if(Cn < 0) {

		VERBOSE(P_CON_FAILED,UERRORTEXT);
		cleanup(-Cn);
	}


	if(Debug) tdmp(Cn); 

	/* At this point succeeded in getting an open communication line */
	/* Conn() takes care of closing the Systems file                       */

	(void)signal(SIGINT,_onintrpt);
	_mode(1);			/* put terminal in `raw' mode */
	VERBOSE("Connected\007\r\n","");	/*bell!*/

	/*	must catch signals before fork.  if not and if _receive()	*/
	/*	fails in just the right (wrong?) way, _rcvdead() can be		*/
	/*	called and do "kill(getppid(),SIGUSR1);" before parent		*/
	/*	has done calls to signal() after recfork().			*/
	(void)signal(SIGUSR1, _bye);
	(void)signal(SIGHUP, cleanup);
	(void)signal(SIGQUIT, _onintrpt);

	recfork();		/* checks for child == 0 */

	if(Child > 0) {
		Rtn_code = transmit();
		_quit(Rtn_code);
	} else {
		cleanup(Cn);
	}
}

/*
 *	Kill the present child, if it exists, then fork a new one.
 */

recfork()
{
	if (Child)
		kill(Child, SIGKILL);
	Child = dofork();
	if(Child == 0) {
		(void)signal(SIGUSR1, SIG_DFL);
		(void)signal(SIGHUP, _rcvdead);
		(void)signal(SIGQUIT, SIG_IGN);
		(void)signal(SIGINT, SIG_IGN);

		_receive();	/* This should run until killed */
		/*NOTREACHED*/
	}
}

/***************************************************************
 *	transmit: copy stdin to remote fd, except:
 *	~.	terminate
 *	~!	local login-style shell
 *	~!cmd	execute cmd locally
 *	~$proc	execute proc locally, send output to line
 *	~%cmd	execute builtin cmd (put, take, or break)
 ****************************************************************/
#ifdef forfutureuse
 /*****************************************************************
  *	~+proc	execute locally, with stdout to and stdin from line.
  ******************************************************************/
#endif

int
transmit()
{
	char b[BUFSIZ];
#ifdef notdef
	char prompt[sizeof (struct utsname)];
#else
	char prompt[MAXBASENAME+1];
#endif
	register char *p;
	register int escape;
	register int id = 0;  /*flag for systemname prompt on tilda escape*/

	CDEBUG(4,"transmit started\n\r","");
	sysname(prompt);

	/* In main loop, always waiting to read characters from  */
	/* keyboard; writes characters to remote, or to TTYOUT   */
	/* on a tilda escape                                     */

	while(TRUE) {
		p = b;
		while(r_char(TTYIN) == YES) {
			if(p == b)  	/* Escape on leading  ~    */
				escape = (_Cxc == '~');
			if(p == b+1)   	/* But not on leading ~~   */
				escape &= (_Cxc != '~');
			if(escape) {
			     if(_Cxc == '\n' || _Cxc == '\r' || _Cxc == _Teol) {
					*p = '\0';
					if(tilda(b+1) == YES)
						return(0);
					id = 0;
					break;
				}
				if(_Cxc == _Tintr || _Cxc == _Tkill || _Cxc
					 == _Tquit ||
					    (Intrupt && _Cxc == '\0')) {
					if(_Cxc == _Tkill) {
						if(Echok)
							VERBOSE("\r\n","");
					}
					else {
					_Cxc = '\r';
						if( w_char(Cn) == NO) {
						 	VERBOSE(P_LINE_GONE,"");
							return(IOERR);
						}
					id=0;
					}
					break;
				}
				if((p == b+1) && (_Cxc != _Terase) && (!id)) {
					id = 1;
					VERBOSE("[%s]", prompt);
				}
				if(_Cxc == _Terase) { 
					p = (--p < b)? b:p;
					if(p > b)
						if(Echoe)
							VERBOSE("\b \b", "");
						else 
						 	(void)w_char(TTYOUT);
				} else {
					(void)w_char(TTYOUT);
					if(p-b < BUFSIZ) 
						*p++ = _Cxc;
					else {
						VERBOSE(P_TOOLONG,"");
						break;
					}
				}
	/*not a tilda escape command*/
			} else {
				if(Intrupt && _Cxc == '\0') {
					CDEBUG(4,"got break in transmit\n\r","");
					Intrupt = NO;
					(*genbrk)(Cn);
					_flush();
					break;
				}
				if(w_char(Cn) == NO) {
					VERBOSE(P_LINE_GONE,"");
					return(IOERR);
				}
				if(Duplex == NO) {
					if((w_char(TTYERR) == NO) ||
					   (wioflsh(TTYERR) == NO))
						return(IOERR);
				}
				if ((_Cxc == _Tintr) || (_Cxc == _Tquit) ||
					 ( (p==b) && (_Cxc == _Myeof) ) ) {
					CDEBUG(4,"got a tintr\n\r","");
					_flush();
					break;
				}
				if(_Cxc == '\n' || _Cxc == '\r' ||
					_Cxc == _Teol || _Cxc == _Tkill) {
					id=0;
					Takeflag = NO;
					break;
				}
				p = (char*)0;
			}
		}
	}
}

/***************************************************************
 *	routine to halt input from remote and flush buffers
 ***************************************************************/
static void
_flush()
{
	(void)ioctl(TTYOUT, TCXONC, 0);	/* stop tty output */
	(void)ioctl(Cn, TCFLSH, 0);		/* flush remote input */
	(void)ioctl(TTYOUT, TCFLSH, 1);	/* flush tty output */
	(void)ioctl(TTYOUT, TCXONC, 1);	/* restart tty output */
	if(Takeflag == NO) {
		return;		/* didn't interupt file transmission */
	}
	VERBOSE(P_FILEINTR,"");
	(void)sleep(3);
	_w_str("echo '\n~>\n';mesg y;stty echo\n");
	Takeflag = NO;
}

/**************************************************************
 *	command interpreter for escape lines
 **************************************************************/
int
tilda(cmd)
register char	*cmd;
{

	VERBOSE("\r\n","");
	CDEBUG(4,"call tilda(%s)\r\n", cmd);

	switch(cmd[0]) {
		case '.':
			if(Cucall.telno == NULL)
				if(cmd[1] != '.') {
					_w_str("\04\04\04\04\04");
					if (Child)
						kill(Child, SIGKILL);
					(void) ioctl (Cn, TCGETA, &_Lv);
        				/* speed to zero for hangup */ 
					_Lv.c_cflag = 0;
        				(void) ioctl (Cn, TCSETAW, &_Lv);
        				(void) sleep (2);
				}
			return(YES);
		case '!':
			_shell(cmd);	/* local shell */
			VERBOSE("\r%c\r\n", *cmd);
			VERBOSE("(continue)","");
			break;
		case '$':
			if(cmd[1] == '\0') {
				VERBOSE(P_USECMD,"");
				VERBOSE("(continue)","");
			}
			else {
				_shell(cmd);	/*Local shell  */
				VERBOSE("\r%c\r\n", *cmd);
			}
			break;	

#ifdef forfutureuse
		case '+':
			if(cmd[1] == '\0') {
				VERBOSE(P_USEPLUSCMD, "");
				VERBOSE("(continue)","");
			}
			else {
				if (*cmd == '+')
					      /* must suspend receive to give*/
					      /*remote out to stdin of cmd */
					kill(Child, SIGKILL);
					 _shell(cmd);	/* Local shell */
				if (*cmd == '+')
					recfork();
				VERBOSE("\r%c\r\n", *cmd);
			}
			break;
#endif
		case '%':
			_dopercen(++cmd);
			break;
			
		case 't':
			tdmp(TTYIN);
			VERBOSE("(continue)","");
			break;
		case 'l':
			tdmp(Cn);
			VERBOSE("(continue)","");
			break;
		
		default:
			VERBOSE(P_STARTWITH,"");
			VERBOSE("(continue)","");
			break;
	}
	return(NO);
}

/***************************************************************
 *	The routine "shell" takes an argument starting with
 *	either "!" or "$", and terminated with '\0'.
 *	If $arg, arg is the name of a local shell file which
 *	is executed and its output is passed to the remote.
 *	If !arg, we escape to a local shell to execute arg
 *	with output to TTY, and if arg is null, escape to
 *	a local shell and blind the remote line.  In either
 *	case, '^D' will kill the escape status.
 **************************************************************/

#ifdef forfutureuse
/***************************************************************
 *	Another argument to the routine "shell" may be +.  If +arg,
 *	arg is the name of a local shell file which is executed with
 *	stdin from and stdout to the remote.
 **************************************************************/
#endif

static void
_shell(str)
char	*str;
{
	int	fk;
	void	(*xx)(), (*yy)();

	CDEBUG(4,"call _shell(%s)\r\n", str);
	fk = dofork();
	if(fk < 0)
		return;
	_mode(0);	/* restore normal tty attributes */
	xx = signal(SIGINT, SIG_IGN);
	yy = signal(SIGQUIT, SIG_IGN);
	if(fk == 0) {
		char *shell, *getenv();
		shell = getenv("SHELL");

		if(shell == NULL)
			shell = "/bin/sh";
					   /*user's shell is set if*/
					   /*different from default*/
		(void)close(TTYOUT);

		/***********************************************
		 * Hook-up our "standard output"
		 * to either the tty for '!' or the line
		 * for '$'  as appropriate
		 ***********************************************/
#ifdef forfutureuse

		/************************************************
		 * Or to the line for '+'.
		 **********************************************/
#endif

		(void)fcntl((*str == '!')? TTYERR:Cn,F_DUPFD,TTYOUT);

#ifdef forfutureuse
		/*************************************************
		 * Hook-up "standard input" to the line for '+'.
		 * **********************************************/
		if (*str == '+')
			{
			(void)close(TTYIN);
			(void)fcntl(Cn,F_DUPFD,TTYIN);
			}
#endif

		/***********************************************
		 * Hook-up our "standard input"
		 * to the tty for '!' and '$'.
		 ***********************************************/

		(void)close(Cn);   	/*parent still has Cn*/
		(void)signal(SIGINT, SIG_DFL);
		(void)signal(SIGHUP, SIG_DFL);
		(void)signal(SIGQUIT, SIG_DFL);
		(void)signal(SIGUSR1, SIG_DFL);
		if(*++str == '\0')
			(void)execl(shell,shell,(char*)0,(char*)0,0);
		else
			(void)execl(shell,"sh","-c",str,0);
		VERBOSE(P_Ct_EXSH,"");
		exit(0);
	}
	while(wait((int*)0) != fk);
	(void)signal(SIGINT, xx);
	(void)signal(SIGQUIT, yy);
	_mode(1);
}


/***************************************************************
 *	This function implements the 'put', 'take', 'break', and
 *	'nostop' commands which are internal to cu.
 ***************************************************************/

static void
_dopercen(cmd)
register char *cmd;
{
	char	*arg[5];
	char	*getpath, *getenv();
	char	mypath[MAXPATH];
	int	narg;

	blckcnt((long)(-1));

	CDEBUG(4,"call _dopercen(\"%s\")\r\n", cmd);

	arg[narg=0] = strtok(cmd, " \t\n");

		/* following loop breaks out the command and args */
	while((arg[++narg] = strtok((char*) NULL, " \t\n")) != NULL) {
		if(narg < 4)
			continue;
		else
			break;
	}

	/* ~%take file option */
	if(EQUALS(arg[0], "take")) {
		if(narg < 2 || narg > 3) {
			VERBOSE("usage: ~%%take from [to]\r\n","");
			VERBOSE("(continue)","");
			return;
		}
		if(narg == 2)
			arg[2] = arg[1];

		/*
		 * be sure that the remote file (arg[1]) exists before 
		 * you try to take it.   otherwise, the error message from
		 * cat will wind up in the local file (arg[2])
		 *
		 * what we're doing is:
		 *	stty -echo; \
		 *	if test -r arg1
		 *      then (echo '~'>:arg2; cat arg1; echo '~'>)
		 *	else echo can't open: arg1
		 *	fi; \
		 *	stty echo
		 *
		 */
		_w_str("stty -echo;if test -r ");
		_w_str(arg[1]);
		_w_str("; then (echo '~>':");
		_w_str(arg[2]);
		_w_str(";cat ");
		_w_str(arg[1]);
		_w_str(";echo '~>'); else echo cant\\'t open: ");
		_w_str(arg[1]);
		_w_str("; fi;stty echo\n");
		Takeflag = YES;
		return;
	}
	/* ~%put file option*/
	if(EQUALS(arg[0], "put")) {
		FILE	*file;
		char	ch, buf[BUFSIZ], spec[NCC+1], *b, *p, *q;
		int	i, j, len, tc=0, lines=0;
		long	chars=0L;

		if(narg < 2 || narg > 3) {
			VERBOSE("usage: ~%%put from [to]\r\n","");
			VERBOSE("(continue)","");
			return;
		}
		if(narg == 2)
			arg[2] = arg[1];

		if((file = fopen(arg[1], "r")) == NULL) {
			VERBOSE(P_Ct_OPEN, arg[1]);
			VERBOSE("(continue)","");
			return;
		}
		/*
		 * if cannot write into file on remote machine, write into
		 * /dev/null
		 *
		 * what we're doing is:
		 *	stty -echo
		 *	(cat - > arg2) || cat - > /dev/null
		 *	stty echo
		 */
		_w_str("stty -echo;(cat - >");
		_w_str(arg[2]);
		_w_str(")||cat - >/dev/null;stty echo\n");
		Intrupt = NO;
		for(i=0,j=0; i < NCC; ++i)
			if((ch=_Tv0.c_cc[i]) != '\0')
				spec[j++] = ch;
		spec[j] = '\0';
		_mode(2);	/*accept interrupts from keyboard*/
		(void)sleep(5);	/*hope that w_str info digested*/

/* Read characters line by line into buf to write to remote with character*/
/*and line count for blckcnt                                              */
		while(Intrupt == NO &&
				fgets(b= &buf[MID],MID,file) != NULL) {
/*worse case= each*/
/*char must be escaped*/
			len = strlen(b);
			chars += len;		/* character count */
			p = b;
			while(q = strpbrk(p, spec)) {
				if(*q == _Tintr || *q == _Tquit ||
							*q == _Teol) {
					VERBOSE(P_Ct_SPECIAL, *q);
					(void)strcpy(q, q+1);
					Intrupt = YES;
				}
				else {
				b = strncpy(b-1, b, q-b);
				*(q-1) = '\\';
				}
			p = q+1;
			}
			if((tc += len) >= MID) {
				(void)sleep(1);
				tc = len;
			}
			if(write(Cn, b, (unsigned)strlen(b)) < 0) {
				VERBOSE(P_IOERR,"");
				Intrupt = YES;
				break;
			}
			++lines;		/* line count */
			blckcnt((long)chars);
		}
		_mode(1);
		blckcnt((long)(-2));		/* close */
		(void)fclose(file);
		if(Intrupt == YES) {
			Intrupt = NO;
			VERBOSE(P_FILEINTR,"");
			_w_str("\n");
			VERBOSE(P_CNTAFTER, ++chars);
		} else
			VERBOSE(P_CNTLINES, lines);
			VERBOSE(P_CNTCHAR,chars);
		_w_str("\04");
		(void)sleep(3);
		return;
	}

		/*  ~%b or ~%break  */
	if(EQUALS(arg[0], "b") || EQUALS(arg[0], "break")) {
		(*genbrk)(Cn);
		return;
	}
		/*  ~%d or ~%debug toggle  */
	if(EQUALS(arg[0], "d") || EQUALS(arg[0], "debug")) {
		if(Debug == 0)
			Debug = 9;
		else
			Debug = 0;
		VERBOSE("(continue)","");
		return;
	}
		/*  ~%nostop  toggles start/stop input control  */
	if(EQUALS(arg[0], "nostop")) {
		(void)ioctl(Cn, TCGETA, &_Tv);
		if(Sstop == NO)
			_Tv.c_iflag |= IXOFF;
		else
			_Tv.c_iflag &= ~IXOFF;
		(void)ioctl(Cn, TCSETAW, &_Tv);
		Sstop = !Sstop;
		_mode(1);
		VERBOSE("(continue)","");
		return;
	}
		/* Change local current directory */
	if(EQUALS(arg[0], "cd")) {
		if (narg < 2) {
			getpath = getenv("HOME");
			strcpy(mypath, getpath);
			if(chdir(mypath) < 0) {
				VERBOSE("Cannot change to %s\r\n", mypath);
				VERBOSE("(continue)","");
				return;
			}
		}
		else if (chdir(arg[1]) < 0) {
			VERBOSE("Cannot change to %s\r\n", arg[1]);
			VERBOSE("(continue)","");
			return;
		}
		recfork();	/* fork a new child so it know about change */
		VERBOSE("(continue)","");
		return;
	}
	VERBOSE("~%%%s unknown to cu\r\n", arg[0]);
	VERBOSE("(continue)","");
}

/***************************************************************
 *	receive: read from remote line, write to fd=1 (TTYOUT)
 *	catch:
 *	~>[>]:file
 *	.
 *	. stuff for file
 *	.
 *	~>	(ends diversion)
 ***************************************************************/

static void
_receive()
{
	register silent=NO, file;
	register char *p;
	int	tic;
	char	b[BUFSIZ];
	long	lseek(), count;

	CDEBUG(4,"_receive started\r\n","");


	b[0] = '\0';
	file = -1;
	p = b;

	while(r_char(Cn) == YES) {
		if(silent == NO)	/* ie., if not redirecting to file*/
			if(w_char(TTYOUT) == NO) 
				_rcvdead(IOERR);	/* this will exit */
		/* remove CR's and fill inserted by remote */
		if(_Cxc == '\0' || _Cxc == RUB || _Cxc == '\r')
			continue;
		*p++ = _Cxc;
		if(_Cxc != '\n' && (p-b) < BUFSIZ)
			continue;
		/***********************************************
		 * The rest of this code is to deal with what
		 * happens at the beginning, middle or end of
		 * a diversion to a file.
		 ************************************************/
		if(b[0] == '~' && b[1] == '>') {
			/****************************************
			 * The line is the beginning or
			 * end of a diversion to a file.
			 ****************************************/
			if((file < 0) && (b[2] == ':' || b[2] == '>')) {
				/**********************************
				 * Beginning of a diversion
				 *********************************/
				int	append;

				*(p-1) = NULL; /* terminate file name */
				append = (b[2] == '>')? 1:0;
				p = b + 3 + append;
				if(append && (file=open(p,O_WRONLY))>0)
					(void)lseek(file, 0L, 2);
				else
					file = creat(p, 0666);
				if(file < 0) {
					VERBOSE(P_Ct_DIVERT, p);
					perror("cu: open|creat failed");
					(void)sleep(5); /* 10 seemed too long*/
				} else {
					silent = YES; 
					count = tic = 0;
				}
			} else {
				/*******************************
				 * End of a diversion (or queer data)
				 *******************************/
				if(b[2] != '\n')
					goto D;		/* queer data */
				if(silent = close(file)) {
					VERBOSE(P_Ct_DIVERT, b);
					perror("cu: close failed");
					silent = NO;
				}
				blckcnt((long)(-2));
				VERBOSE("~>\r\n","");
				VERBOSE(P_CNTLINES, tic);
				VERBOSE(P_CNTCHAR, count);
				file = -1;
			}
		} else {
			/***************************************
			 * This line is not an escape line.
			 * Either no diversion; or else yes, and
			 * we've got to divert the line to the file.
			 ***************************************/
D:
			if(file > 0) {
				(void)write(file, b, (unsigned)(p-b));
				count += p-b;	/* tally char count */
				++tic;		/* tally lines */
				blckcnt((long)count);
			}
		}
		p = b;
	}
	VERBOSE("\r\nLost Carrier\r\n","");
	_rcvdead(IOERR);
}

/***************************************************************
 *	change the TTY attributes of the users terminal:
 *	0 means restore attributes to pre-cu status.
 *	1 means set `raw' mode for use during cu session.
 *	2 means like 1 but accept interrupts from the keyboard.
 ***************************************************************/
static void
_mode(arg)
{
	CDEBUG(4,"call _mode(%d)\r\n", arg);
	if(arg == 0) {
		if ( Saved_tty )
			(void)ioctl(TTYIN, TCSETAW, &_Tv0);
	} else {
		(void)ioctl(TTYIN, TCGETA, &_Tv);
		if(arg == 1) {
			_Tv.c_iflag &= ~(INLCR | ICRNL | IGNCR |
						IXOFF | IUCLC);
			_Tv.c_iflag |= ISTRIP;
			_Tv.c_oflag |= OPOST;
			_Tv.c_oflag &= ~(OLCUC | ONLCR | OCRNL | ONOCR
						| ONLRET);
			_Tv.c_lflag &= ~(ICANON | ISIG | ECHO);
			if(Sstop == NO)
				_Tv.c_iflag &= ~IXON;
			else
				_Tv.c_iflag |= IXON;
			if(Terminal) {
				_Tv.c_oflag |= ONLCR;
				_Tv.c_iflag |= ICRNL;
			}
			_Tv.c_cc[VEOF] = '\01';
			_Tv.c_cc[VEOL] = '\0';
		}
		if(arg == 2) {
			_Tv.c_iflag |= IXON;
			_Tv.c_lflag |= ISIG;
		}
		(void)ioctl(TTYIN, TCSETAW, &_Tv);
	}
}


static int
dofork()
{
	register int x,i;

	for(i = 0; i < 6; ++i) {
		if((x = fork()) >= 0) {
			return(x);
		}
	}

	if(Debug) perror("dofork");

	VERBOSE(P_Ct_FK,"");
	return(x);
}

static int
r_char(fd)
{
	int rtn = 1, rfd;
	char *riobuf;

	/* find starting pos in correct buffer in Riobuf	*/
	rfd = RIOFD(fd);
	riobuf = &Riobuf[rfd*WRIOBSZ];

	if (Riop[rfd] >= &riobuf[Riocnt[rfd]]) {
		/* empty read buffer - refill it	*/

		/*	flush any waiting output	*/
		if ( (wioflsh(Cn) == NO ) || (wioflsh(TTYOUT) == NO) )
			return(NO);

		while((rtn = read(fd, riobuf, WRIOBSZ)) < 0){
			if(errno == EINTR) {
		/* onintrpt() called asynchronously before this line */
				if(Intrupt == YES) {
					/* got a BREAK */
					_Cxc = '\0';
					return(YES);
				} else {
					/*a signal other than interrupt*/ 
					/*received during read*/
					continue;
				}
			} else {
				CDEBUG(4,"got read error, not EINTR\n\r","");
				break;			/* something wrong */
			}
		}
		if (rtn > 0) {
			/* reset current position in buffer	*/
			/* and count of available chars		*/
			Riop[rfd] = riobuf;
			Riocnt[rfd] = rtn;
		}
	}

	if ( rtn > 0 ) {
		_Cxc = *(Riop[rfd]++) & 0177; 	/*must mask off parity bit*/
		return(YES);
	} else {
		_Cxc = '\0';
		return(NO);
	}
}

static int
w_char(fd)
{
	int wfd;
	char *wiobuf;

	/* find starting pos in correct buffer in Wiobuf	*/
	wfd = WIOFD(fd);
	wiobuf = &Wiobuf[wfd*WRIOBSZ];

	if (Wiop[wfd] >= &wiobuf[WRIOBSZ]) {
		/* full output buffer - flush it */
		if ( wioflsh(fd) == NO )
			return(NO);
	}
	*(Wiop[wfd]++) = _Cxc;
	return(YES);
}

/* wioflsh	flush output buffer	*/
static int
wioflsh(fd)
int fd;
{
	int rtn, wfd;
	char *wiobuf;

	/* find starting pos in correct buffer in Wiobuf	*/
	wfd = WIOFD(fd);
	wiobuf = &Wiobuf[wfd*WRIOBSZ];

	if (Wiop[wfd] > wiobuf) {
		/* there's something in the buffer */
		while((rtn = write(fd, wiobuf, (Wiop[wfd] - wiobuf))) < 0) {
			if(errno == EINTR)
				if(Intrupt == YES) {
					VERBOSE("\ncu: Output blocked\r\n","");
					_quit(IOERR);
				} else
					continue;	/* alarm went off */
			else {
				Wiop[wfd] = wiobuf;
				return(NO);			/* bad news */
			}
		}
	}
	Wiop[wfd] = wiobuf;
	return(YES);
}


static void
_w_str(string)
register char *string;
{
	int len;

	len = strlen(string);
	if ( write(Cn, string, (unsigned)len) != len )
		VERBOSE(P_LINE_GONE,"");
}

static void
_onintrpt()
{
	(void)signal(SIGINT, _onintrpt);
	(void)signal(SIGQUIT, _onintrpt);
	Intrupt = YES;
}

static void
_rcvdead(arg)	/* this is executed only in the receive process */
int arg;
{
	CDEBUG(4,"call _rcvdead(%d)\r\n", arg);
	(void)kill(getppid(), SIGUSR1);
	exit((arg == SIGHUP)? SIGHUP: arg);
	/*NOTREACHED*/
}

static void
_quit(arg)	/* this is executed only in the parent process */
int arg;
{
	CDEBUG(4,"call _quit(%d)\r\n", arg);
	(void)kill(Child, SIGKILL);
	_bye(arg);
	/*NOTREACHED*/
}

static void
_bye(arg)	/* this is executed only in the parent proccess */
int arg;
{
	int status;

	CDEBUG(4,"call _bye(%d)\r\n", arg);

	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)wait(&status);
	VERBOSE("\r\nDisconnected\007\r\n","");
	cleanup((arg == SIGUSR1)? (status >>= 8): arg);
	/*NOTREACHED*/
}



void
cleanup(code) 	/*this is executed only in the parent process*/
int code;	/*Closes device; removes lock files          */
{

	CDEBUG(4,"call cleanup(%d)\r\n", code);

	(void) setuid(Euid);
	if(Cn > 0) {
		chmod(_Cnname, Dev_mode);
		(void)close(Cn);
	}


	rmlock((char*) NULL);	/*uucp routine in ulockf.c*/	
	_mode(0);		/*which removes lock files*/
	exit(code);		/* code=negative for signal causing disconnect*/
}



void
tdmp(arg)
{

	struct termio xv;
	int i;

	VERBOSE("\rdevice status for fd=%d\n", arg);
	VERBOSE("\F_GETFL=%o,", fcntl(arg, F_GETFL,1));
	if(ioctl(arg, TCGETA, &xv) < 0) {
		char	buf[100];
		i = errno;
		(void)sprintf(buf, "\rtdmp for fd=%d", arg);
		errno = i;
		perror(buf);
		return;
	}
	VERBOSE("iflag=`%o',", xv.c_iflag);
	VERBOSE("oflag=`%o',", xv.c_oflag);
	VERBOSE("cflag=`%o',", xv.c_cflag);
	VERBOSE("lflag=`%o',", xv.c_lflag);
	VERBOSE("line=`%o'\r\n", xv.c_line);
	VERBOSE("cc[0]=`%o',",  xv.c_cc[0]);
	for(i=1; i<8; ++i) {
		VERBOSE("[%d]=", i);
		VERBOSE("`%o' , ",xv.c_cc[i]);
	}
	VERBOSE("\r\n","");
}



sysname(name)
char * name;
{
#ifdef notdef

	register char *s;
	struct utsname utsn; 

	if(uname(&utsn) < 0)
		s = "Local";
	else
		s = utsn.nodename;

	strncpy(name,s,strlen(s) + 1);
	*(name + strlen(s) + 1) = '\0';
	return;
#else
	uucpname(name);
#endif
}



blckcnt(count)
long count;
{
	static long lcharcnt = 0;
	register long c1, c2;
	register int i;
	char c;

	if(count == (long) (-1)) {       /* initialization call */
		lcharcnt = 0;
		return;
	}
	c1 = lcharcnt/BUFSIZ;
	if(count != (long)(-2)) {	/* regular call */
		c2 = count/BUFSIZ;
		for(i = c1; i++ < c2;) {
			c = '0' + i%10;
			write(2, &c, 1);
			if(i%NPL == 0)
				write(2, "\n\r", 2);
		}
		lcharcnt = count;
	}
	else {
		c2 = (lcharcnt + BUFSIZ -1)/BUFSIZ;
		if(c1 != c2)
			write(2, "+\n\r", 3);
		else if(c2%NPL != 0)
			write(2, "\n\r", 2);
		lcharcnt = 0;
	}
}

void logent(){}		/* so we can load ulockf() */
