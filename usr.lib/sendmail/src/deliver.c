/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

# include "sendmail.h"
# include <signal.h>
# include <sys/stat.h>
# include <errno.h>
# include <vfork.h>

SCCSID(@(#)deliver.c 1.1 92/07/30 SMI); /* from UCB 5.21 5/5/88 */

static struct mxinfo *mx;	/* "current" mx record information */
# ifndef MXDOMAIN
struct mxinfo dummy_mx;		/* dummy mx info when not used */
# endif MXDOMAIN

/*
**  DELIVER -- Deliver a message to a list of addresses.
**
**	This routine delivers to everyone on the same host as the
**	user on the head of the list.  It is clever about mailers
**	that don't handle multiple users.  It is NOT guaranteed
**	that it will deliver to all these addresses however -- so
**	deliver should be called once for each address on the
**	list.
**
**	Parameters:
**		e -- the envelope to deliver.
**		firstto -- head of the address list to deliver to.
**
**	Returns:
**		zero -- successfully delivered.
**		else -- some failure, see ExitStat for more info.
**
**	Side Effects:
**		The standard input is passed off to someone.
*/

deliver(e, firstto)
	register ENVELOPE *e;
	ADDRESS *firstto;
{
	char *host;			/* host being sent to */
	char *user;			/* user being sent to */
	char **pvp;
	int sizeleft;			/* Bytes of argv left to work with */
	int buflen;			/* Temp */
	register char **mvp;
	register char *p;
	register MAILER *m;		/* mailer for this recipient */
	ADDRESS *ctladdr;
	register ADDRESS *to = firstto;
	bool clever = FALSE;		/* running user smtp to this mailer */
	ADDRESS *tochain = NULL;	/* chain of users in this mailer call */
	int rcode;			/* response code */
	char *pv[MAXPV+1];
	char tobuf[MAXLINE-50];		/* text line of to people */
	char buf[MAXNAME];
	char tfrombuf[MAXNAME];		/* translated from person */
	extern bool checkcompat();
	extern ADDRESS *getctladdr();
	extern char *remotename();

	errno = 0;
	if (bitset(QDONTSEND, to->q_flags))
		return (0);

	m = to->q_mailer;
	host = to->q_host;

# ifdef DEBUG
	if (tTd(10, 1))
		printf("\n--deliver, mailer=%d, host=`%s', first user=`%s'\n",
			m->m_mno, host, to->q_user);
# endif DEBUG

	/*
	**  If this mailer is expensive, and if we don't want to make
	**  connections now, just mark these addresses and return.
	**	This is useful if we want to batch connections to
	**	reduce load.  This will cause the messages to be
	**	queued up, and a daemon will come along to send the
	**	messages later.
	**		This should be on a per-mailer basis.
	*/

	if (NoConnect && !QueueRun && bitnset(M_EXPENSIVE, m->m_flags) &&
	    !Verbose)
	{
		for (; to != NULL; to = to->q_next)
		{
			if (bitset(QDONTSEND, to->q_flags) || to->q_mailer != m)
				continue;
			to->q_flags |= QQUEUEUP|QDONTSEND;
			e->e_to = to->q_paddr;
			message(Arpa_Info, "queued");
			if (LogLevel > 4)
				logdelivery("queued");
		}
		e->e_to = NULL;
		return (0);
	}

	/*
	**  Do initial argv setup.
	**	Insert the mailer name.  Notice that $x expansion is
	**	NOT done on the mailer name.  Then, if the mailer has
	**	a picky -f flag, we insert it as appropriate.  This
	**	code does not check for 'pv' overflow; this places a
	**	manifest lower limit of 4 for MAXPV.
	**		The from address rewrite is expected to make
	**		the address relative to the other end.
	*/

	/* rewrite from address, using rewriting rules */
	expand("\001f", buf, &buf[sizeof buf - 1], e);
	(void) strcpy(tfrombuf, remotename(buf, m, TRUE, TRUE));

	define('g', tfrombuf, e);		/* translated sender address */
	define('h', host, e);			/* to host */
	Errors = 0;
	pvp = pv;
	sizeleft = m->m_argvsize - 10;		/* Leave some slop */
	*pvp++ = m->m_argv[0];
	sizeleft -= strlen(m->m_argv[0]) + 1;

	/* insert -f or -r flag as appropriate */
	if (FromFlag && (bitnset(M_FOPT, m->m_flags) || bitnset(M_ROPT, m->m_flags)))
	{
		if (bitnset(M_FOPT, m->m_flags))
			*pvp++ = "-f";
		else
			*pvp++ = "-r";
		expand("\001g", buf, &buf[sizeof buf - 1], e);
		*pvp++ = newstr(buf);
		sizeleft -= strlen(buf) + 1 + 2 + 1;
	}

	/*
	**  Append the other fixed parts of the argv.  These run
	**  up to the first entry containing "$u".  There can only
	**  be one of these, and there are only a few more slots
	**  in the pv after it.
	*/

	for (mvp = m->m_argv; (p = *++mvp) != NULL; )
	{
		while ((p = index(p, '\001')) != NULL)
			if (*++p == 'u')
				break;
		if (p != NULL)
			break;

		/* this entry is safe -- go ahead and process it */
		expand(*mvp, buf, &buf[sizeof buf - 1], e);
		*pvp++ = newstr(buf);
		sizeleft -= strlen(buf) + 1;
		if (pvp >= &pv[MAXPV - 3])
		{
			syserr("Too many parameters to %s before $u", pv[0]);
			return (-1);
		}
	}

	/*
	**  If we have no substitution for the user name in the argument
	**  list, we know that we must supply the names otherwise -- and
	**  SMTP is the answer!!
	*/

	if (*mvp == NULL)
	{
		/* running SMTP */
# ifdef SMTP
		clever = TRUE;
		*pvp = NULL;
# else SMTP
		/* oops!  we don't implement SMTP */
		syserr("SMTP style mailer");
		return (EX_SOFTWARE);
# endif SMTP
	}

	/*
	**  At this point *mvp points to the argument with $u.  We
	**  run through our address list and append all the addresses
	**  we can.  If we run out of space, do not fret!  We can
	**  always send another copy later.
	*/

	tobuf[0] = '\0';
	e->e_to = tobuf;
	ctladdr = NULL;
	for (; to != NULL; to = to->q_next)
	{
		/* avoid sending multiple recipients to dumb mailers */
		if (tobuf[0] != '\0' && !bitnset(M_MUSER, m->m_flags))
			break;

		/* if already sent or not for this host, don't send */
		if (bitset(QDONTSEND, to->q_flags) ||
		    strcmp(to->q_host, host) != 0 ||
		    to->q_mailer != firstto->q_mailer)
			continue;

		/* avoid overflowing tobuf */
		if (sizeof tobuf - (strlen(to->q_paddr) + strlen(tobuf) + 2) < 0)
			break;

# ifdef DEBUG
		if (tTd(10, 1))
		{
			printf("\nsend to ");
			printaddr(to, FALSE);
		}
# endif DEBUG

		/* compute effective uid/gid when sending */
		if (to->q_mailer == ProgMailer)
			ctladdr = getctladdr(to);

		user = to->q_user;
		e->e_to = to->q_paddr;

		/*
		**  Check to see that these people are allowed to
		**  talk to each other.
		*/

		if (m->m_maxsize != 0 && e->e_msgsize > m->m_maxsize)
		{
			NoReturn = TRUE;
			usrerr("Message is too large; %ld bytes max", m->m_maxsize);
			giveresponse(EX_UNAVAILABLE, m, e);
			to->q_flags |= QDONTSEND;
			continue;
		}
		if (!checkcompat(to))
		{
			giveresponse(EX_UNAVAILABLE, m, e);
			to->q_flags |= QDONTSEND;
			continue;
		}

		/*
		**  Strip quote bits from names if the mailer is dumb
		**	about them.
		*/

		if (bitnset(M_STRIPQ, m->m_flags))
		{
			stripquotes(user, TRUE);
			stripquotes(host, TRUE);
		}
		else
		{
			stripquotes(user, FALSE);
			stripquotes(host, FALSE);
		}

		/* hack attack -- delivermail compatibility */
		if (m == ProgMailer && *user == '|')
			user++;

		/*
		**  If an error message has already been given, don't
		**	bother to send to this address.
		**
		**	>>>>>>>>>> This clause assumes that the local mailer
		**	>> NOTE >> cannot do any further aliasing; that
		**	>>>>>>>>>> function is subsumed by sendmail.
		*/

		if (bitset(QBADADDR|QQUEUEUP, to->q_flags)) {
			to->q_flags |= QDONTSEND;
			continue;
		}

		/*
		**  Expand out this user and see if it fits in argv.
		**  By definition, the first user always fits, else
		**  we would loop here for rather a long time.
		*/
		define('u', user, e);		/* to user */
		define('z', to->q_home, e);	/* user's home */
		expand(*mvp, buf, &buf[sizeof buf - 1], e);
		buflen = strlen(buf);

		if (buflen > sizeleft && tobuf[0] != '\0')
			break;		/* Stop argv here */

		/*
		**  We have decided that we are sending the message to
		**  this user now.  First mark it to avoid resending
		**  later.
		*/

		to->q_flags |= QDONTSEND;

		/*
		**  See if this user name is "special".
		**	If the user name has a slash in it, assume that this
		**	is a file -- send it off without further ado.  Note
		**	that this type of addresses is not processed along
		**	with the others, so we fudge on the To person.
		*/

		if (m == LocalMailer)
		{
			if (user[0] == '/')
			{
				rcode = mailfile(user, getctladdr(to));
				giveresponse(rcode, m, e);
				markstats(e, to);
				continue;
			}
		}

		/*
		**  Add this user to mailer argv, and add it
		**  to the print list of recipients.
		*/

		/* link together the chain of recipients */
		to->q_tchain = tochain;
		tochain = to;

		/* create list of users for error messages */
		(void) strcat(tobuf, ",");
		(void) strcat(tobuf, to->q_paddr);

		if (!clever)
		{
			*pvp++ = newstr(buf);
			sizeleft -= buflen + 1;
			if (pvp >= &pv[MAXPV - 2])
			{
				/* allow some space for trailing parms */
				break;
			}
		}
	}

	/* see if any addresses still exist */
	if (tobuf[0] == '\0')
	{
		define('g', (char *) NULL, e);
		return (0);
	}

	/* print out messages as full list */
	e->e_to = tobuf + 1;

	/*
	**  Fill out any parameters after the $u parameter.
	**  Note that these are not checked against "sizeleft", thus are
	**  not counted in the mailer L= option.  The L= value just has
	**  to subtract off the max length of any trailing parameters,
	**  or figure out how to move them to the front instead.
	*/

	while (!clever && *++mvp != NULL)
	{
		expand(*mvp, buf, &buf[sizeof buf - 1], e);
		*pvp++ = newstr(buf);
		if (pvp >= &pv[MAXPV])
			syserr("deliver: pv overflow after $u for %s", pv[0]);
	}
	*pvp++ = NULL;

	/*
	**  Call the mailer.
	**	The argument vector gets built, pipes
	**	are created as necessary, and we fork & exec as
	**	appropriate.
	**	If we are running SMTP, we just need to clean up.
	*/

	message(Arpa_Info, "Connecting to %s via %s...", host, m->m_name);

	if (ctladdr == NULL)
		ctladdr = &e->e_from;
# ifdef SMTP
	if (clever)
	{
# ifdef MXDOMAIN
		extern struct mxinfo *getmxrr();
/*
 * Should make this dependent on some flag, or at least
 * if strcmp(m->m_mailer,"[TCP]") == 0 
 * or certainly host[0] != '\0'
 */
		mx = getmxrr(host, MyHostName, &rcode);
		if (mx->mx_number >= 0)
			rcode = EX_OK;
#else MXDOMAIN
		mx = &dummy_mx;
		mx->mx_number = 1;
		mx->mx_hosts[0] = host;
		rcode = EX_OK;
#endif
		/* send the initial SMTP protocol */
		if (rcode == EX_OK)
			rcode = smtpinit(m, pv);

		if (rcode == EX_OK)
		{
			/* send the recipient list */
			tobuf[0] = '\0';
			for (to = tochain; to != NULL; to = to->q_tchain)
			{
				int i;

				e->e_to = to->q_paddr;
				i = smtprcpt(to, m);
				if (i != EX_OK)
				{
					markfailure(e, to, i);
					giveresponse(i, m, e);
				}
				else
				{
					(void) strcat(tobuf, ",");
					(void) strcat(tobuf, to->q_paddr);
				}
			}

			/* now send the data */
			if (tobuf[0] == '\0')
				e->e_to = NULL;
			else
			{
				e->e_to = tobuf + 1;
				rcode = smtpdata(m, e);
			}

			/* now close the connection */
			smtpquit(m);
		}
	}
	else
# endif SMTP
		rcode = sendoff(e, m, pv, ctladdr);

	/*
	**  Do final status disposal.
	**	We check for something in tobuf for the SMTP case.
	**	If we got a temporary failure, arrange to queue the
	**		addressees.
	*/

	if (tobuf[0] != '\0')
		giveresponse(rcode, m, e);
	if (rcode != EX_OK)
	{
		for (to = tochain; to != NULL; to = to->q_tchain)
			markfailure(e, to, rcode);
	} else for (to = tochain; to != NULL; to = to->q_tchain)
		markstats(e, to);

	errno = 0;
	define('g', (char *) NULL, e);
	return (rcode);
}
/*
**  MARKFAILURE -- mark a failure on a specific address.
**
**	Parameters:
**		e -- the envelope we are sending.
**		q -- the address to mark.
**		rcode -- the code signifying the particular failure.
**
**	Returns:
**		none.
**
**	Side Effects:
**		marks the address (and possibly the envelope) with the
**			failure so that an error will be returned or
**			the message will be queued, as appropriate.
*/

markfailure(e, q, rcode)
	register ENVELOPE *e;
	register ADDRESS *q;
	int rcode;
{
	if (rcode == EX_OK)
		return;
	else if (rcode != EX_TEMPFAIL && rcode != EX_OSERR && 
	         rcode !=EX_IOERR)
		q->q_flags |= QBADADDR;
	else if (curtime() > e->e_ctime + TimeOut)
	{
		extern char *pintvl();
		char buf[MAXLINE];

		if (!bitset(EF_TIMEOUT, e->e_flags))
		{
			(void) sprintf(buf, "Cannot send message for %s",
				pintvl(TimeOut, FALSE));
			if (e->e_message != NULL)
				free(e->e_message);
			e->e_message = newstr(buf);
			message(Arpa_Info, buf);
		}
		q->q_flags |= QBADADDR;
		e->e_flags |= EF_TIMEOUT;
	}
	else
		q->q_flags |= QQUEUEUP;
}
/*
**  DOFORK -- do a fork, retrying a couple of times on failure.
**
**	This MUST be a macro, since after a vfork we are running
**	two processes on the same stack!!!
**
**	Parameters:
**		none.
**
**	Returns:
**		From a macro???  You've got to be kidding!
**
**	Side Effects:
**		Modifies the ==> LOCAL <== variable 'pid', leaving:
**			pid of child in parent, zero in child.
**			-1 on unrecoverable error.
**
**	Notes:
**		I'm awfully sorry this looks so awful.  That's
**		vfork for you.....
*/

# define NFORKTRIES	5
# ifdef VMUNIX
# define XFORK	vfork
# else VMUNIX
# define XFORK	fork
# endif VMUNIX

# define DOFORK(fORKfN) \
{\
	register int i;\
\
	for (i = NFORKTRIES; --i >= 0; )\
	{\
		pid = fORKfN();\
		if (pid >= 0)\
			break;\
		if (i > 0)\
			sleep((unsigned) NFORKTRIES - i);\
	}\
}
/*
**  DOFORK -- simple fork interface to DOFORK.
**
**	Parameters:
**		none.
**
**	Returns:
**		pid of child in parent.
**		zero in child.
**		-1 on error.
**
**	Side Effects:
**		returns twice, once in parent and once in child.
*/

dofork()
{
	register int pid;

	DOFORK(fork);
	return (pid);
}
/*
**  SENDOFF -- send off call to mailer & collect response.
**
**	Parameters:
**		e -- the envelope to mail.
**		m -- mailer descriptor.
**		pvp -- parameter vector to send to it.
**		ctladdr -- an address pointer controlling the
**			user/groupid etc. of the mailer.
**
**	Returns:
**		exit status of mailer.
**
**	Side Effects:
**		If exit status is -1, ExitStat and errno are set as from
**		openmailer().
*/

sendoff(e, m, pvp, ctladdr)
	register ENVELOPE *e;
	MAILER *m;
	char **pvp;
	ADDRESS *ctladdr;
{
	auto FILE *mfile;
	auto FILE *rfile;
	register int i;
	int pid;

	/*
	**  Create connection to mailer.
	*/

	pid = openmailer(m, pvp, ctladdr, FALSE, &mfile, &rfile);
	if (pid < 0)
		return (EX_TEMPFAIL);

	/*
	**  Format and send message.
	*/

	putfromline(mfile, m);
	(*e->e_puthdr)(mfile, m, e);
	putline("\n", mfile, m);
	(*e->e_putbody)(mfile, m, e);
	(void) fclose(mfile);

	i = endmailer(pid, pvp[0]);

	/* arrange a return receipt if requested */
	if (e->e_receiptto != NULL && bitnset(M_LOCAL, m->m_flags))
	{
		e->e_flags |= EF_SENDRECEIPT;
		/* do we want to send back more info? */
	}

	return (i);
}
/*
**  ENDMAILER -- Wait for mailer to terminate.
**
**	We should never get fatal errors (e.g., segmentation
**	violation), so we report those specially.  For other
**	errors, we choose a status message (into statmsg),
**	and if it represents an error, we print it.
**
**	Parameters:
**		pid -- pid of mailer.
**		name -- name of mailer (for error messages).
**
**	Returns:
**		exit code of mailer.
**
**	Side Effects:
**		none.
*/


char *enderr[] = {
	"Zero",
	"Hangup",
	"Interrupt",
	"Quit",
	"Illegal instruction",
	"Trace",
	"IOT",
	"EMT",
	"Floating point exception",
	"Kill",
	"Bus error",
	"Segmentation violation",
	"Bad argument to system call",
	"Broken pipe",
	"Alarm clock",
	"Software termination",
	"Urgent",
	"Stop",
	"Stop from keyboard",
	"Continue",
	"Child status",
	"Background read",
	"Background write",
	"I/O",
	"CPU time limit exceeded",
	"File size limit exceeded",
	"Virtual time alarm",
	"Profiling timer alarm",
	"Window change"
} ;

#define MAXENDERR	(sizeof(enderr)/sizeof(enderr[0]))

endmailer(pid, name)
	int pid;
	char *name;
{
	int st;

	/* in the IPC case there is nothing to wait for */
	if (pid == 0)
		return (EX_OK);

	if (tTd(38, 1))
		syslog(LOG_INFO, "Before the wait on pid %d\n", pid);

	/* wait for the mailer process to die and collect status */
	st = waitfor(pid);

	if (tTd(38, 1))
		syslog(LOG_INFO, "After the wait on pid %d\n", pid);

	if (st == -1)
	{
		syserr("endmailer %s: wait", name);
		return (EX_SOFTWARE);
	}

	/* see if it died a horrid death */
	if ((st & 0377) != 0)
	{
		if (st < 0 || st >= MAXENDERR)
		    message(Arpa_Info,
			"%s delivery program died with signal %d", name, st);
		else
		    message(Arpa_Info,
			"%s delivery program died with %s signal", name, 
				enderr[st]);
		ExitStat = EX_TEMPFAIL;
		return (EX_TEMPFAIL);
	}

	/* normal death -- return status */
	st = (st >> 8) & 0377;
	return (st);
}
/*
**  OPENMAILER -- open connection to mailer.
**
**	Parameters:
**		m -- mailer descriptor.
**		pvp -- parameter vector to pass to mailer.
**		ctladdr -- controlling address for user.
**		clever -- create a full duplex connection.
**		pmfile -- pointer to mfile (to mailer) connection.
**		prfile -- pointer to rfile (from mailer) connection.
**
**	Returns:
**		pid of mailer ( > 0 ).
**		-1 on error, with errno set appropriately.
**		zero on an IPC connection.
**
**	Side Effects:
**		creates a mailer in a subprocess.
**		Sets errno if it returns -1.
*/

openmailer(m, pvp, ctladdr, clever, pmfile, prfile)
	MAILER *m;
	char **pvp;
	ADDRESS *ctladdr;
	bool clever;
	FILE **pmfile;
	FILE **prfile;
{
	int pid;
	int mpvect[2];
	int rpvect[2];
	FILE *mfile;
	FILE *rfile;
	extern FILE *fdopen();
	extern int DtableSize;		/* main.c -- from getdtablesize() */

# ifdef DEBUG
	if (tTd(11, 1))
	{
		printf("openmailer:");
		printav(pvp);
	}
# endif DEBUG
	errno = 0;

	/*
	**  Deal with the special case of mail handled through an IPC
	**  connection.
	**	In this case we don't actually fork.  We must be
	**	running SMTP for this to work.  We will return a
	**	zero pid to indicate that we are running IPC.
	**  We also handle a debug version that just talks to stdin/out.
	*/

#ifdef DEBUG
	/* check for Local Person Communication -- not for mortals!!! */
	if (strcmp(m->m_mailer, "[LPC]") == 0)
	{
		*pmfile = stdout;
		*prfile = stdin;
		return (0);
	}
#endif DEBUG

	if (strcmp(m->m_mailer, "[IPC]") == 0 ||
	    strcmp(m->m_mailer, "[TCP]") == 0)
	{
#ifdef DAEMON
		register int i, j;
		register u_short port;

		CurHostName = pvp[1];
		if (!clever || mx == NULL)
			syserr("non-clever TCP");
		if (pvp[2] != NULL)
			port = atoi(pvp[2]);
		else
			port = 0;
 		for (j = 0; j < mx->mx_number; j++)
		{
			CurHostName = mx->mx_hosts[j];
			if (Verbose && !sameword(CurHostName, pvp[1]))
				printf("mail exchanger is %s\n", CurHostName);
			i = makeconnection(CurHostName, port, pmfile, prfile);
			if (i == EX_OK)
				return(0);
		}
		/* Note that errno is set for this case */
		ExitStat = i;
		return (-1);
#else DAEMON
		syserr("openmailer: no TCP");
		return (-1);
#endif DAEMON
	}

	/* create a pipe to shove the mail through */
	if (pipe(mpvect) < 0)
	{
		syserr("openmailer: pipe (to mailer)");
		return (-1);
	}
#ifdef SMTP
	/* if this mailer speaks smtp, create a return pipe */
	if (clever && pipe(rpvect) < 0)
	{
		syserr("openmailer: pipe (from mailer)");
		(void) close(mpvect[0]);
		(void) close(mpvect[1]);
		return (-1);
	}
#endif SMTP

	/*
	**  Actually fork the mailer process.
	**	DOFORK is clever about retrying.
	**
	**	Dispose of SIGCHLD signal catchers that may be laying
	**	around so that endmail will get it.
	*/

	if (CurEnv->e_xfp != NULL)
		(void) fflush(CurEnv->e_xfp);		/* for debugging */
	(void) fflush(stdout);
# ifdef SIGCHLD
	(void) signal(SIGCHLD, SIG_DFL);
# endif SIGCHLD
	DOFORK(XFORK);
	/* pid is set by DOFORK */
	if (pid < 0)
	{
		/* failure */
		syserr("openmailer: cannot fork");
		(void) close(mpvect[0]);
		(void) close(mpvect[1]);
#ifdef SMTP
		if (clever)
		{
			(void) close(rpvect[0]);
			(void) close(rpvect[1]);
		}
#endif SMTP
		return (-1);
	}
	else if (pid == 0)
	{
		int i;

		/* child -- set up input & exec mailer */
		/* make diagnostic output be standard output */
		(void) signal(SIGINT, SIG_IGN);
		(void) signal(SIGHUP, SIG_IGN);
		(void) signal(SIGTERM, SIG_DFL);

		/* arrange to filter standard & diag output of command */
		if (clever)
		{
			(void) close(rpvect[0]);
			(void) close(1);
			(void) dup(rpvect[1]);
			(void) close(rpvect[1]);
		}
		else if (OpMode == MD_SMTP || HoldErrs)
		{
			/* put mailer output in transcript */
			(void) close(1);
			(void) dup(fileno(CurEnv->e_xfp));
		}
		(void) close(2);
		(void) dup(1);

		/* arrange to get standard input */
		(void) close(mpvect[1]);
		(void) close(0);
		if (dup(mpvect[0]) < 0)
		{
			syserr("Cannot dup to zero!");
			_exit(EX_OSERR);
		}
		(void) close(mpvect[0]);
		if (!bitnset(M_RESTR, m->m_flags))
		{
			if (ctladdr == NULL || ctladdr->q_uid == 0)
			{
				(void) setgid(DefGid);
				(void) initgroups(DefUser, DefGid);
				(void) setuid(DefUid);
			}
			else
			{
				(void) setgid(ctladdr->q_gid);
				(void) initgroups(ctladdr->q_ruser?
					ctladdr->q_ruser: ctladdr->q_user,
					ctladdr->q_gid);
				(void) setuid(ctladdr->q_uid);
			}
		}

		/* arrange for all the files to be closed */
		for (i = 3; i < DtableSize; i++)
#ifdef FIOCLEX
			(void) ioctl(i, FIOCLEX, 0);
#else FIOCLEX
			(void) close(i);
#endif FIOCLEX

		/* try to execute the mailer */
		execve(m->m_mailer, pvp, UserEnviron);
#ifdef FIOCLEX
		syserr("Cannot exec %s", m->m_mailer);
#else FIOCLEX
		printf("Cannot exec '%s' errno=%d\n", m->m_mailer, errno);
		(void) fflush(stdout);
#endif FIOCLEX
		if (m == LocalMailer || errno == EIO || errno == EAGAIN ||
		    errno == ENOMEM || errno == EPROCLIM)
			_exit(EX_TEMPFAIL);
		else
			_exit(EX_UNAVAILABLE);
	}

	/*
	**  Set up return value.
	*/
	if (tTd(38, 1))
		syslog(LOG_INFO, "Fork with pid %d\n", pid);

	(void) close(mpvect[0]);
	mfile = fdopen(mpvect[1], "w");
	if (clever)
	{
		(void) close(rpvect[1]);
		rfile = fdopen(rpvect[0], "r");
	} else
		rfile = NULL;

	*pmfile = mfile;
	*prfile = rfile;

	return (pid);
}
/*
**  GIVERESPONSE -- Interpret an error response from a mailer
**
**	Parameters:
**		stat -- the status code from the mailer (high byte
**			only; core dumps must have been taken care of
**			already).
**		m -- the mailer descriptor for this mailer.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Errors may be incremented.
**		ExitStat may be set.
*/

giveresponse(stat, m, e)
	int stat;
	register MAILER *m;
	ENVELOPE *e;
{
	register char *statmsg;
	extern char *SysExMsg[];
	register int i;
	extern int N_SysEx;
	char buf[MAXLINE];

#ifdef lint
	if (m == NULL)
		return;
#endif lint

	/*
	**  Compute status message from code.
	*/

	i = stat - EX__BASE;
	if (stat == 0)
		statmsg = "250 Sent";
	else if (i < 0 || i > N_SysEx)
	{
		(void) sprintf(buf, "554 unknown mailer error %d", stat);
		stat = EX_UNAVAILABLE;
		statmsg = buf;
	}
	else if (stat == EX_TEMPFAIL)
	{
		extern char SmtpError[];

		if (AlreadyKnown)
			(void) strcpy(buf, "250 Skipped");
		else 
			(void) strcpy(buf, SysExMsg[i]);
		if (SmtpError[0] != '\0' && 
			(errno == 0 || errno == ECONNRESET) )
		{
				(void) strcat(buf, ": ");
				(void) strcat(buf, SmtpError);
		}
		else if (errno)
		{
			extern char *errstring();

			(void) strcat(buf, ": ");
			(void) strcat(buf, errstring(errno));
		}
		statmsg = buf;
	}
	else
	{
		statmsg = SysExMsg[i];
	}

	/*
	**  Print the message as appropriate
	** Note the "%s" is needed to prevent percent signs in recipient
	** names from being treated as formatting commands.
	*/

	if (stat == EX_OK || stat == EX_TEMPFAIL)
		message(Arpa_Info, "%s", &statmsg[4]);
	else
	{
		Errors++;
		usrerr(statmsg);
	}

	/*
	**  Final cleanup.
	**	Log a record of the transaction.  Compute the new
	**	ExitStat -- if we already had an error, stick with
	**	that. Save the new envelope message unless it is temporary
	**	and we already have a permanent error.
	*/

	if (LogLevel > ((stat == 0 || stat == EX_TEMPFAIL) ? 3 : 2))
		logdelivery(&statmsg[4]);

	if (stat != EX_TEMPFAIL)
		setstat(stat);
	if (stat != EX_OK && (stat != EX_TEMPFAIL ||
		 !bitset(EF_TIMEOUT|EF_FATALERRS, e->e_flags)))
	{
		if (e->e_message != NULL)
			free(e->e_message);
		e->e_message = newstr(&statmsg[4]);
	}
	errno = 0;
}
/*
**  LOGDELIVERY -- log the delivery in the system log
**
**	Parameters:
**		stat -- the message to print for the status
**
**	Returns:
**		none
**
**	Side Effects:
**		none
*/

logdelivery(stat)
	char *stat;
{
	extern char *pintvl();

# ifdef LOG
	if (AlreadyKnown)
		return;
	/*
	 * Syslog should really check boundaries instead but it doesn't.
	 */
#define MAX_TO_SYSLOG 800
	if (strlen(CurEnv->e_to) > MAX_TO_SYSLOG)
		CurEnv->e_to[MAX_TO_SYSLOG] = '\0';
	syslog(LOG_INFO, "%s: to=%s, delay=%s, stat=%s", CurEnv->e_id,
	       CurEnv->e_to, pintvl(curtime() - CurEnv->e_ctime, TRUE), stat);
# endif LOG
}
/*
**  PUTFROMLINE -- output a UNIX-style from line (or whatever)
**
**	This can be made an arbitrary message separator by changing $l
**
**	One of the ugliest hacks seen by human eyes is contained herein:
**	UUCP wants those stupid "remote from <host>" lines.  Why oh why
**	does a well-meaning programmer such as myself have to deal with
**	this kind of antique garbage????
**
**	Parameters:
**		fp -- the file to output to.
**		m -- the mailer describing this entry.
**
**	Returns:
**		none
**
**	Side Effects:
**		outputs some text to fp.
*/

putfromline(fp, m)
	register FILE *fp;
	register MAILER *m;
{
	char *template = "\001l\n";
	char buf[MAXLINE];

	if (bitnset(M_NHDR, m->m_flags))
		return;

# ifdef UGLYUUCP
	if (bitnset(M_UGLYUUCP, m->m_flags))
	{
		char *bang;
		char xbuf[MAXLINE];

		expand("\001g", buf, &buf[sizeof buf - 1], CurEnv);
		bang = index(buf, '!');
		if (bang == NULL)
		{
		    bang = index(buf, '@');
		    if (bang == NULL)
			(void) sprintf(xbuf, 
			    "From %s  \001d remote from \001w\n", buf);
		    else
		    {
			*bang++ = '\0';
			(void) sprintf(xbuf, 
			  "From %s!%s  \001d remote from \001w\n", bang, buf);
		    }
		}
		else
		{
			*bang++ = '\0';
			(void) sprintf(xbuf, 
			  "From %s  \001d remote from %s\n", bang, buf);
		}
		template = xbuf;
	}
# endif UGLYUUCP
	expand(template, buf, &buf[sizeof buf - 1], CurEnv);
	putline(buf, fp, m);
}
/*
**  PUTBODY -- put the body of a message.
**
**	Parameters:
**		fp -- file to output onto.
**		m -- a mailer descriptor to control output format.
**		e -- the envelope to put out.
**
**	Returns:
**		none.
**
**	Side Effects:
**		The message is written onto fp.
*/

putbody(fp, m, e)
	FILE *fp;
	MAILER *m;
	register ENVELOPE *e;
{
	char buf[MAXLINE];

	/*
	**  Output the body of the message
	*/

	if (e->e_dfp == NULL)
	{
		if (e->e_df != NULL)
		{
			e->e_dfp = fopen(e->e_df, "r");
			if (e->e_dfp == NULL)
				syserr("Cannot open %s", e->e_df);
		}
		else
			putline("<<< No Message Collected >>>", fp, m);
	}
	if (e->e_dfp != NULL)
	{
		rewind(e->e_dfp);
		while (!ferror(fp) && fgets(buf, sizeof buf, e->e_dfp) != NULL)
		{
			if (buf[0] == 'F' && bitnset(M_ESCFROM, m->m_flags) &&
			    strncmp(buf, "From", 4) == 0)
				(void) putc('>', fp);
#ifdef INTER
			putline_8bit(buf, fp, m);
#else
			putline(buf, fp, m);
#endif
		}

		if (ferror(e->e_dfp))
		{
			syserr("putbody: read error");
			ExitStat = EX_IOERR;
		}
	}

	(void) fflush(fp);
	if (ferror(fp) && errno != EPIPE)
	{
		syserr("putbody: write error");
		ExitStat = EX_IOERR;
	}
	errno = 0;
}
/*
**  MAILFILE -- Send a message to a file.
**
**	If the file has the setuid/setgid bits set, but NO execute
**	bits, sendmail will try to become the owner of that file
**	rather than the real user.  Obviously, this only works if
**	sendmail runs as root.
**
**	This could be done as a subordinate mailer, except that it
**	is used implicitly to save messages in ~/dead.letter.  We
**	view this as being sufficiently important as to include it
**	here.  For example, if the system is dying, we shouldn't have
**	to create another process plus some pipes to save the message.
**
**	Parameters:
**		filename -- the name of the file to send to.
**		ctladdr -- the controlling address header -- includes
**			the userid/groupid to be when sending.
**
**	Returns:
**		The exit code associated with the operation.
**
**	Side Effects:
**		none.
*/

mailfile(filename, ctladdr)
	char *filename;
	ADDRESS *ctladdr;
{
	register FILE *f;
	register int pid;
	ENVELOPE *e = CurEnv;

	/*
	**  Fork so we can change permissions here.
	**	Note that we MUST use fork, not vfork, because of
	**	the complications of calling subroutines, etc.
	** Also, make sure the body file is open BEFORE we setuid to someone
	** besides root.
	*/
	
	if (CurEnv->e_dfp == NULL && CurEnv->e_df != NULL)
		CurEnv->e_dfp = fopen(CurEnv->e_df, "r");

# ifdef SIGCHLD
	(void) signal(SIGCHLD, SIG_DFL);
# endif SIGCHLD
	DOFORK(fork);

	if (pid < 0)
		return (EX_OSERR);
	else if (pid == 0)
	{
		/* child -- actually write to file */
		struct stat stb;

		(void) signal(SIGINT, SIG_DFL);
		(void) signal(SIGHUP, SIG_DFL);
		(void) signal(SIGTERM, SIG_DFL);
		(void) umask(OldUmask);
		if (stat(filename, &stb) < 0)
		{
			errno = 0;
			stb.st_mode = 0666;
		}
		if (bitset(0111, stb.st_mode))
			exit(EX_CANTCREAT);
		if (ctladdr == NULL)
			ctladdr = &CurEnv->e_from;
		if (!bitset(S_ISGID, stb.st_mode) || setgid(stb.st_gid) < 0)
		{
			if (ctladdr->q_uid == 0) {
				(void) setgid(DefGid);
				(void) initgroups(DefUser, DefGid);
			} else {
				(void) setgid(ctladdr->q_gid);
				(void) initgroups(ctladdr->q_ruser?
					ctladdr->q_ruser: ctladdr->q_user,
					ctladdr->q_gid);
			}
		}
		if (!bitset(S_ISUID, stb.st_mode) || setuid(stb.st_uid) < 0)
		{
			if (ctladdr->q_uid == 0)
				(void) setuid(DefUid);
			else
				(void) setuid(ctladdr->q_uid);
		}
		f = dfopen(filename, "a");
		if (f == NULL)
			exit(EX_CANTCREAT);

		putfromline(f, ProgMailer);
		(*CurEnv->e_puthdr)(f, ProgMailer, CurEnv);
		putline("\n", f, ProgMailer);
		(*CurEnv->e_putbody)(f, ProgMailer, CurEnv);
		putline("\n", f, ProgMailer);
		(void) fclose(f);
		(void) fflush(stdout);

		/* reset ISUID & ISGID bits for paranoid systems */
		(void) chmod(filename, (int) stb.st_mode);
		exit(EX_OK);
		/*NOTREACHED*/
	}
	else
	{
		/* parent -- wait for exit status */
		int st;

		st = waitfor(pid);
		if ((st & 0377) != 0)
			return (EX_UNAVAILABLE);
		else
			return ((st >> 8) & 0377);
		/*NOTREACHED*/
	}
}
/*
**  SENDALL -- actually send all the messages.
**
**	Parameters:
**		e -- the envelope to send.
**		mode -- the delivery mode to use.  If SM_DEFAULT, use
**			the current SendMode.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Scans the send lists and sends everything it finds.
**		Delivers any appropriate error messages.
**		If we are running in a non-interactive mode, takes the
**			appropriate action.
*/

sendall(e, mode)
	ENVELOPE *e;
	char mode;
{
	register ADDRESS *q;
	bool oldverbose;
	int pid;
	int checkpointCount;

#ifdef DEBUG
	if (tTd(13, 1))
	{
		printf("\nSENDALL: mode %c, sendqueue:\n", mode);
		printaddr(e->e_sendqueue, TRUE);
	}
#endif DEBUG

	/*
	**  Do any preprocessing necessary for the mode we are running.
	**	Check to make sure the hop count is reasonable.
	**	Delete sends to the sender in mailing lists.
	*/

	CurEnv = e;

	if (e->e_hopcount > Maxhop)
	{
		extern char	Arpa_Usrerr[];
		e->e_to = e->e_sendqueue->q_paddr;
		message(Arpa_Usrerr,"Mail loop detected");
		syserr("sendall: too many hops (%d max)", Maxhop);
		return;
	}

	if (!MeToo)
	{
		extern ADDRESS *recipient();

		e->e_from.q_flags |= QDONTSEND;
		(void) recipient(&e->e_from, &e->e_sendqueue);
	}

	if (SendMode == SM_VERIFY)
	  {
	    mode = SM_VERIFY;
	  }
# ifdef QUEUE
	else if (!bitset(EF_INQUEUE, e->e_flags))
	      {
		  /*
		   * Add in extra penalty points for messages to lots of
		   * recipients (like big mailing lists).
		   */
	    	checkpointCount = 0;
		for (q = e->e_sendqueue; q != NULL; q = q->q_next)
		  {
		     if (!bitset(QDONTSEND, q->q_flags)) checkpointCount++;
		  }
		e->e_msgpriority += WKRECIPFACT*checkpointCount;
		  /*
		   * Determine actual delivery mode: 
		   * if we are over the threshold and the message is too "big"
		   * then we just queue it up.
		   * If message is too "small" then complain.
		   * Messages with no recipients are an error.
		   */
		if (RecipThreshold > 0 &&
		    checkpointCount > RecipThreshold &&
		    e->e_bodysize < REJECT_MIN)
		    {
		      syserr("Too many recipients for no message body");
		      return;
		    }
		if (checkpointCount == 0 && !ForceMail && mode != SM_DELIVER)
		  {
		    e->e_to = e->e_sendqueue->q_paddr;
		    message(Arpa_Usrerr,"Possible alias loop");
		    syserr("No valid recipients");
		    return;
		  }
		if (mode == SM_DEFAULT)
		  {
		    if (shouldqueue(e->e_msgpriority))
				mode = SM_QUEUE;
			else
				mode = SendMode;
		  }
		queueup(e, TRUE, mode == SM_QUEUE);
	      }
#endif QUEUE
	oldverbose = Verbose;
	switch (mode)
	{
	  case SM_VERIFY:
		Verbose = TRUE;
		break;

	  case SM_QUEUE:
		e->e_flags |= EF_INQUEUE|EF_KEEPQUEUE;
		return;

	  case SM_FORK:
		if (e->e_xfp != NULL)
			(void) fflush(e->e_xfp);
		pid = fork();
		if (pid < 0)
		{
			mode = SM_DELIVER;
			break;
		}
		else if (pid > 0)
		{
			/* be sure we leave the temp files to our child */
			e->e_id = e->e_df = NULL;
			return;
		}

		/* double fork to avoid zombies */
		if (fork() > 0)
			exit(EX_OK);

		/* be sure we are immune from the terminal */
		disconnect(FALSE);

		break;
	}

	/*
	**  Run through the list and send everything.
	*/

    	checkpointCount = 0;
	for (q = e->e_sendqueue; q != NULL; q = q->q_next)
	{
		if (mode == SM_VERIFY)
		{
			e->e_to = q->q_paddr;
			if (!bitset(QDONTSEND|QBADADDR, q->q_flags))
				message(Arpa_Info, "deliverable");
		}
		else
			(void) deliver(e, q);
		/*
		 * Checkpoint the queue every once in a while,
		 * so we don't get a lot of duplicate messages
		 * if we crash during delivery of a big list.
		 */
		if (++checkpointCount >= CheckPointLimit)
		  {
		    checkpoint(e,q->q_next);
		    checkpointCount = 0;
		  }
	}
	Verbose = oldverbose;

	/*
	**  Now run through and check for errors.
	*/

	if (mode == SM_VERIFY)
		return;

	for (q = e->e_sendqueue; q != NULL; q = q->q_next)
	{
		register ADDRESS *qq;

# ifdef DEBUG
		if (tTd(13, 3))
		{
			printf("Checking ");
			printaddr(q, FALSE);
		}
# endif DEBUG

		/* only send errors if the message failed */
		if (!bitset(QBADADDR|QQUEUEUP, q->q_flags))
			continue;

		/* we have an address that failed -- find the parent */
		for (qq = q; qq != NULL; qq = qq->q_alias)
		{
			char obuf[MAXNAME + 6];
			extern char *aliaslookup();

			/* we can only have owners for local addresses */
			if (!bitnset(M_LOCAL, qq->q_mailer->m_flags))
				continue;

			/* see if the owner list exists */
			(void) strcpy(obuf, "owner-");
			if (strncmp(qq->q_user, "owner-", 6) == 0)
				(void) strcat(obuf, "owner");
			else
				(void) strcat(obuf, qq->q_user);
			if (aliaslookup(obuf) == NULL)
				continue;

# ifdef DEBUG
			if (tTd(13, 4))
				printf("Errors to %s\n", obuf);
# endif DEBUG
			/* owner list exists -- add it to the error queue */
			sendtolist(obuf, (ADDRESS *) NULL, &e->e_errorqueue);
			ErrorMode = EM_MAIL;
			break;
		}
	}

	if (mode == SM_FORK)
		finis();
}
