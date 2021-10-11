# ifdef lint
static char sccsid[] = "@(#)unix_login.c 1.1 92/07/30 Copyr 1989 Sun Micro";
# endif lint

# include <rpc/types.h>
# include <sys/ioctl.h>
# include <sys/signal.h>
# include <sys/file.h>
# include <sys/label.h>
# include <sys/audit.h>
# include <pwd.h>
# include <pwdadj.h>
# include <errno.h>
# include <stdio.h>
# include <utmp.h>
# include <signal.h>

# include "rex.h"

/*
 * unix_login - hairy junk to simulate logins for Unix
 *
 * Copyright (c) 1989 Sun Microsystems, Inc.
 */

char Ttys[] = "/etc/ttys";	/* file to get index of utmp */
char Utmp[] = "/etc/utmp";	/* the tty slots */
char Wtmp[] = "/var/adm/wtmp";	/* the log information */

int Master, Slave;		/* sides of the pty */
int InputSocket, OutputSocket;	/* Network sockets */
int Helper1, Helper2;		/* pids of the helpers */
char UserName[256];		/* saves the user name for loging */
char HostName[256];		/* saves the host name for loging */
char PtyName[16] = "/dev/ttypn";/* name of the tty we allocated */
static int TtySlot;		/* slot number in Utmp */

extern fd_set svc_fdset;	/* master file descriptor set */
extern int child;		/* pid of the executed process */
extern int ChildDied;		/* flag */
extern int HasHelper;		/* flag */

  /*
   * The convention used in auditing is:
   *  Argument	Value
   * --------- --------
   *   0	rpc.rexd
   *   1	user name
   *   2	hostname
   *   3	command
   *   4	optional error string
   */
# define AuditCount 5	/* number of audit parameters */
static char *audit_argv[AuditCount] = {"rpc.rexd", "", "", "", ""}; 

/*
 * Check for user being able to run on this machine.
 * returns 0 if OK, TRUE if problem, error message in "error"
 * copies name of shell and home directory if user is valid.
 */
ValidUser(host, uid, error, shell, dir, cmd)
    char *host;		/* passed in */
    int uid;
    char *error;	/* filled in on return */
    char *shell;	/* filled in on return */
    char *cmd;		/* passed in */
{
    struct passwd *pw, *getpwuid();
    struct passwd_adjunct *apw, *getpwanam();
    audit_state_t audit_state;	/* determines auditing flags */
    
    if (issecure()) {
        setauid(0);
	audit_state.as_success = AU_LOGIN;
	audit_state.as_failure = AU_LOGIN;
        setaudit(&audit_state);
    }
    audit_argv[2] = host;
    if (cmd != NULL)
    	audit_argv[3] = cmd;
    if (uid == 0) {
    	errprintf(error,"rexd: root execution not allowed\n",uid);
	audit_argv[1] = "root";
	audit_note(1, error);
	return(1);
    }
    pw = getpwuid(uid);
    if (pw == NULL || pw->pw_name == NULL) {
    	errprintf(error,"rexd: User id %d not valid\n",uid);
	audit_note(1, error);
	return(1);
    }
    strncpy(UserName, pw->pw_name, sizeof(UserName)-1 );
    strncpy(HostName, host, sizeof(HostName)-1 );
    strcpy(shell,pw->pw_shell);
    strcpy(dir,pw->pw_dir);
    setproctitle(pw->pw_name, host);
      /*
       * User has been validated, now do some auditing work
       */
    audit_argv[1] = pw->pw_name;
    if (cmd == NULL)
    	audit_argv[3] = shell;
    setauid(pw->pw_uid);
    if (issecure() == 0)
	return(0);
    if ((apw = getpwanam(pw->pw_name)) != NULL) {
	audit_state.as_success = 0;
	audit_state.as_failure = 0;
	if ((getfauditflags(&apw->pwa_au_always, 
		 &apw->pwa_au_never, &audit_state)) == 0) {
		/*
		 * if we can't tell how to audit from the flags, 
		 * audit everything that's not never for this user.
		 */
		audit_state.as_success = apw->pwa_au_never.as_success ^ (-1);
		audit_state.as_failure = apw->pwa_au_never.as_success ^ (-1);
        }
    } else {
	audit_state.as_success = -1;
	audit_state.as_failure = -1;
    }
    setaudit(&audit_state);
    return(0);
}

/*
 * Add an audit record with argv that was pre-set, plus the given string
 */
audit_note(retcode, s)
	int retcode;
	char *s;
{
	audit_argv[4] = s;
	audit_text(AU_LOGIN, retcode, retcode, AuditCount, audit_argv);
}

/*
 *  eliminate any controlling terminal that we have already
 */
NoControl()
{
    int devtty;

    devtty = open("/dev/tty",O_RDWR);
    if (devtty > 0) {
    	    ioctl(devtty, TIOCNOTTY, NULL);
	    close(devtty);
    }
}

/*
 * Allocate a pseudo-terminal
 * sets the global variables Master and Slave.
 * returns 1 on error, 0 if OK
 */
AllocatePtyMaster(socket0, socket1)
    int socket0, socket1;
{
# define Sequence "0123456789abcdef"
# define MajorPos 8	/* /dev/ptyXx */
# define MinorPos 9	/* /dev/ptyxX */
# define SidePos 5	/* /dev/Ptyxx */
    static char ptySequence[] = Sequence;
    char *maj, min;
    int pgrp;
    int on = 1;

    signal(SIGHUP,SIG_IGN);
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    for (maj = "pqrstuvwxyzPQRST"; *maj; maj++)
      for (min = 0; min < strlen(Sequence); min++ ) {
	    PtyName[MajorPos] = *maj;
	    PtyName[MinorPos] = ptySequence[min];
	    PtyName[SidePos] = 'p';
	    Master = open(PtyName, O_RDWR);
	    if (Master < 0) {
		continue;
	    }
	    NoControl();
	    LoginUser();
	    pgrp = getpid();
	    setpgrp(pgrp, pgrp);
	    InputSocket = socket0;
	    OutputSocket = socket1;
	    ioctl(Master, FIONBIO, &on);
	    FD_SET(InputSocket, &svc_fdset);
	    FD_SET(Master, &svc_fdset);
	    return(0);
	}
    /*
     * No pty found!
     */
    return(1);
}


AllocatePtySlave()
{
  PtyName[SidePos] = 't';
  Slave = open (PtyName, O_RDWR);
  if (Slave < 0) {
    perror (PtyName);
    exit (1);
  }
}



  /*
   * Special processing for interactive operation.
   * Given pointers to three standard file descriptors,
   * which get set to point to the pty.
   */
DoHelper(pfd0, pfd1, pfd2)
    int *pfd0, *pfd1, *pfd2;
{
    int pgrp;

    pgrp = getpid();
    setpgrp(pgrp, pgrp);
    ioctl(Slave, TIOCSPGRP, &pgrp);

    signal( SIGINT, SIG_IGN);
    close(Master);
    close(InputSocket);
    close(OutputSocket);

    *pfd0 = Slave;
    *pfd1 = Slave;
    *pfd2 = Slave;
}


/*
 * destroy the helpers when the executing process dies
 */
KillHelper(grp)
    int grp;
{
    close(Master);
    FD_CLR(Master, &svc_fdset);
    close(InputSocket);
    FD_CLR(InputSocket, &svc_fdset);
    close(OutputSocket);
    LogoutUser();
    if (grp) killpg(grp,SIGKILL);
}


/*
 * edit the Unix traditional data files that tell who is logged
 * into "the system"
 */
LoginUser()
{
  FILE *ttysFile;
  register char *last = PtyName + sizeof("/dev");
  char line[256];
  int count;
  int utf;
  struct utmp utmp;
  
  ttysFile = fopen(Ttys,"r");
  TtySlot = 0;
  count = 0;
  if (ttysFile != NULL) {
      while (fgets(line, sizeof(line), ttysFile) != NULL) {
        register char *lp;
	lp = line + strlen(line) - 1;
	if (*lp == '\n') *lp = '\0';
	count++;
	if (strcmp(last,line+2)==0) {
	  TtySlot = count;
	  break;
	}
      }
      fclose(ttysFile);
  }
  if (TtySlot > 0 && (utf = open(Utmp,O_WRONLY)) >= 0) {
      lseek(utf, TtySlot*sizeof(utmp), L_SET);
      strncpy(utmp.ut_line,last,sizeof(utmp.ut_line));
      strncpy(utmp.ut_name,UserName,sizeof(utmp.ut_name));
      strncpy(utmp.ut_host,HostName,sizeof(utmp.ut_host));
      time(&utmp.ut_time);
      write(utf, (char *)&utmp, sizeof(utmp));
      close(utf);
  }
  if (TtySlot > 0 && (utf = open(Wtmp,O_WRONLY)) >= 0) {
      lseek(utf, (long)0, L_XTND);
      write(utf, (char *)&utmp, sizeof(utmp));
      close(utf);
  }
}


/*
 * edit the Unix traditional data files that tell who is logged
 * into "the system".
 */
LogoutUser()
{
  int utf;
  register char *last = PtyName + sizeof("/dev");
  struct utmp utmp;

  if (TtySlot > 0 && (utf = open(Utmp,O_RDWR)) >= 0) {
      lseek(utf, TtySlot*sizeof(utmp), L_SET);
      read(utf, (char *)&utmp, sizeof(utmp));
      if (strncmp(last,utmp.ut_line,sizeof(utmp.ut_line))==0) {
	lseek(utf, TtySlot*sizeof(utmp), L_SET);
	strcpy(utmp.ut_name,"");
	strcpy(utmp.ut_host,"");
	time(&utmp.ut_time);
	write(utf, (char *)&utmp, sizeof(utmp));
      }
      close(utf);
  }
  if (TtySlot > 0 && (utf = open(Wtmp,O_WRONLY)) >= 0) {
      lseek(utf, (long)0, L_XTND);
      strncpy(utmp.ut_line,last,sizeof(utmp.ut_line));
      write(utf, (char *)&utmp, sizeof(utmp));
      close(utf);
  }
  TtySlot = 0;
}


/*
 * set the pty modes to the given values
 */
SetPtyMode(mode)
    struct rex_ttymode *mode;
{
    int ldisc = NTTYDISC;
    
    PtyName[SidePos] = 't';
    Slave = open (PtyName, O_RDWR);
    if (Slave < 0) {
      perror (PtyName);
      exit (1);
    }
    ioctl(Slave, TIOCSETD, &ldisc);
    ioctl(Slave, TIOCSETN, &mode->basic);
    ioctl(Slave, TIOCSETC, &mode->more);
    ioctl(Slave, TIOCSLTC, &mode->yetmore);
    ioctl(Slave, TIOCLSET, &mode->andmore);
    close (Slave);
}

/*
 * set the pty window size to the given value
 */
SetPtySize(size)
    struct ttysize *size;
{
    int pgrp;

    PtyName[SidePos] = 't';
    Slave = open (PtyName, O_RDWR);
    if (Slave < 0) {
      perror (PtyName);
      exit (1);
    }
    (void) ioctl(Slave, TIOCSSIZE, size);
    SendSignal(SIGWINCH);
    close (Slave);
}


/*
 * send the given signal to the group controlling the terminal
 */
SendSignal(sig)
    int sig;
{
    int pgrp;

    if (ioctl(Slave, TIOCGPGRP, &pgrp) >= 0)
    	(void) killpg( pgrp, sig);
}


/*
 * called when the main select loop detects that we might want to
 * read something.
 */
HelperRead(fdp)
	fd_set *fdp;
{
    char buf[128];
    int cc;
    extern int errno;
    int mask;

    mask = sigsetmask (sigmask (SIGCHLD));
    if (FD_ISSET(Master, fdp)) {
	FD_CLR(Master, fdp);
    	cc = read(Master, buf, sizeof buf);
	if (cc > 0)
		(void) write(OutputSocket, buf, cc);
	else {
	  	shutdown(OutputSocket, 1);
		FD_CLR(Master, &svc_fdset);
		if (cc < 0 && errno != EINTR && errno != EWOULDBLOCK &&
		    errno != EIO)
			perror("pty read");
		if (ChildDied) {
		  KillHelper (child);
		  HasHelper = 0;
		  if (FD_ISSET(InputSocket, fdp))
		    FD_CLR(InputSocket, fdp);
		  goto done;
		}
	}
    }
    if (FD_ISSET(InputSocket, fdp)) {
	FD_CLR(InputSocket, fdp);
    	cc = read(InputSocket, buf, sizeof buf);
	if (cc > 0)
		(void) write(Master, buf, cc);
	else {
		if (cc < 0 && errno != EINTR && errno != EWOULDBLOCK)
			perror("socket read");
		FD_CLR(InputSocket, &svc_fdset);
	}
    }
  done:
    sigsetmask (mask);
}

