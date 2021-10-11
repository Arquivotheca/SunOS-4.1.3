/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)uucpdefs.c 1.1 92/07/30"	/* from SVR3.2 uucp:uucpdefs.c 2.6 */

#include "uucp.h"

int	Ifn, Ofn;
int	Debug = 0;
int	Uid, Euid;		/* user-id and effective-uid */
int	Ulimit;
ushort	Dev_mode;		/* save device mode here */
char	Progname[NAMESIZE];
char	Pchar;
char	Rmtname[MAXFULLNAME];
char	RemSpool[MAXFULLNAME];	/* spool subdirectory for remote system */
char	User[MAXFULLNAME];
char	Uucp[NAMESIZE];
char	Loginuser[NAMESIZE];
char	Myname[MAXBASENAME+1];
char	Wrkdir[MAXFULLNAME];
char	Logfile[MAXFULLNAME];
char	*Spool = SPOOL;
char	*Pubdir = PUBDIR;
char	**Env;

extern int	read();
extern int	write();
extern int	ioctl();

long	Retrytime = 0;		/* default is to use exponential backoff */
struct	nstat Nstat;
char	Dc[50];			/* line name				*/
int	Seqn;			/* sequence #				*/
int	Role;
char	*Bnptr;			/* used when BASENAME macro is expanded */
char	Jobid[NAMESIZE] = "";	/* Jobid of current C. file */
int	Uerror;			/* global error code */
char	MaxGrade;		/* MASTER may specify maximum grade */
struct nvlist *Nvhead = 0; /* head of name/value list, initially empty */

int	(*genbrk)();

#ifdef STANDALONE
int	Verbose = 0;	/* for cu and ct only */
#endif

/* used for READANY and READSOME macros */
struct stat __s_;

/* messages */
char	*Ct_OPEN =	"CAN'T OPEN";
char	*Ct_WRITE =	"CAN'T WRITE";
char	*Ct_READ =	"CAN'T READ";
char	*Ct_CREATE =	"CAN'T CREATE";
char	*Ct_ALLOCATE =	"CAN'T ALLOCATE";
char	*Ct_LOCK =	"CAN'T LOCK";
char	*Ct_STAT =	"CAN'T STAT";
char	*Ct_CHOWN =	"CAN'T CHOWN";
char	*Ct_CHMOD =	"CAN'T CHMOD";
char	*Ct_LINK =	"CAN'T LINK";
char	*Ct_CHDIR =	"CAN'T CHDIR";
char	*Ct_UNLINK =	"CAN'T UNLINK";
char	*Wr_ROLE =	"WRONG ROLE";
char	*Ct_CORRUPT =	"CAN'T MOVE TO CORRUPTDIR";
char	*Ct_CLOSE =	"CAN'T CLOSE";
char	*Ct_FORK =	"CAN'T FORK";
char	*Fl_EXISTS =	"FILE EXISTS";

char *UerrorText[] = {
  /* SS_OK			0 */ "SUCCESSFUL",
  /* SS_NO_DEVICE		1 */ "NO DEVICES AVAILABLE",
  /* SS_TIME_WRONG		2 */ "WRONG TIME TO CALL",
  /* SS_INPROGRESS		3 */ "TALKING",
  /* SS_CONVERSATION		4 */ "CONVERSATION FAILED",
  /* SS_SEQBAD			5 */ "BAD SEQUENCE CHECK",
  /* SS_LOGIN_FAILED		6 */ "LOGIN FAILED",
  /* SS_DIAL_FAILED		7 */ "DIAL FAILED",
  /* SS_BAD_LOG_MCH		8 */ "BAD LOGIN/MACHINE COMBINATION",
  /* SS_LOCKED_DEVICE		9 */ "DEVICE LOCKED",
  /* SS_ASSERT_ERROR		10 */ "ASSERT ERROR",
  /* SS_BADSYSTEM		11 */ "SYSTEM NOT IN Systems FILE",
  /* SS_CANT_ACCESS_DEVICE	12 */ "CAN'T ACCESS DEVICE",
  /* SS_DEVICE_FAILED		13 */ "DEVICE FAILED",
  /* SS_WRONG_MCH		14 */ "WRONG MACHINE NAME",
  /* SS_CALLBACK		15 */ "CALLBACK REQUIRED",
  /* SS_RLOCKED			16 */ "REMOTE HAS A LCK FILE FOR ME",
  /* SS_RUNKNOWN		17 */ "REMOTE DOES NOT KNOW ME",
  /* SS_RLOGIN			18 */ "REMOTE REJECT AFTER LOGIN",
  /* SS_UNKNOWN_RESPONSE	19 */ "REMOTE REJECT, UNKNOWN MESSAGE",
  /* SS_STARTUP			20 */ "STARTUP FAILED",
  /* SS_CHAT_FAILED		21 */ "CALLER SCRIPT FAILED",
};
