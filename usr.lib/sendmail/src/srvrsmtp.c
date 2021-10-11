/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *  Sendmail
 *  Copyright (c) 1983  Eric P. Allman
 *  Berkeley, California
 */

# include <errno.h>
# include "sendmail.h"
# include <signal.h>

# ifndef SMTP
SCCSID(@(#)srvrsmtp.c 1.1 92/07/30 SMI (no SMTP)); /* from UCB 5.21 3/13/88 */
# else SMTP

SCCSID(@(#)srvrsmtp.c 1.1 92/07/30 SMI); /* from UCB 5.18 1/5/86/28/85 */

/*
**  SMTP -- run the SMTP protocol.
**
**	Parameters:
**		none.
**
**	Returns:
**		never.
**
**	Side Effects:
**		Reads commands from the input channel and processes
**			them.
*/

struct cmd
{
	char	*cmdname;	/* command name */
	int	cmdcode;	/* internal code, see below */
};

/* values for cmdcode */
# define CMDERROR	0	/* bad command */
# define CMDMAIL	1	/* mail -- designate sender */
# define CMDRCPT	2	/* rcpt -- designate recipient */
# define CMDDATA	3	/* data -- send message text */
# define CMDRSET	4	/* rset -- reset state */
# define CMDVRFY	5	/* vrfy -- verify address */
# define CMDHELP	6	/* help -- give usage info */
# define CMDNOOP	7	/* noop -- do nothing */
# define CMDQUIT	8	/* quit -- close connection and die */
# define CMDHELO	9	/* helo -- be polite */
# define CMDVERB	10	/* verb -- go into verbose mode */
	/**  Others removed from BSD **/

# define CMDONEX	15	/* onex -- sending one transaction only */

static struct cmd	CmdTab[] =
{
	"mail",		CMDMAIL,
	"rcpt",		CMDRCPT,
	"data",		CMDDATA,
	"rset",		CMDRSET,
	"vrfy",		CMDVRFY,
	"expn",		CMDVRFY,
	"help",		CMDHELP,
	"noop",		CMDNOOP,
	"quit",		CMDQUIT,
	"helo",		CMDHELO,
	"verb",		CMDVERB,
	"onex",		CMDONEX,
	NULL,		CMDERROR,
};

bool	InChild = FALSE;		/* true if running in a subprocess */

#define EX_QUIT		22		/* special code for QUIT command */

/**
 ** GRABCOMMAND - parse one smtp command line
 **/
char *grabcommand( inp, stringp, cmdp)
    char *inp;				/* input line */
    char **stringp;			/* returns the command string */
    struct cmd **cmdp;			/* returns the command structure */
  {
	register struct cmd *c;
	register char *p;

	p = inp;
	/* clean up end of line */
	fixcrlf(p, TRUE);

	/* echo command to transcript */
	if (CurEnv->e_xfp != NULL)
		fprintf(CurEnv->e_xfp, "<<< %s\n", inp);

	/* break off command */
	for (p = inp; isspace(*p); p++)
		continue;
	*stringp = p;
	while (*++p != '\0' && !isspace(*p))
		continue;
	if (*p != '\0')
		*p++ = '\0';

	/* decode command */
	for (c = CmdTab; c->cmdname != NULL; c++)
	{
		if (!strcasecmp(c->cmdname, *stringp))
			break;
	}
	*cmdp = c;
	return p;
 }

smtp()
{
	register char *p;
	struct cmd *c;
	char *cmd;
	extern char *skipword();
	bool hasmail;			/* mail command received */
	bool wasquit;			/* one transaction (quit after .) */
	auto ADDRESS *vrfyqueue;
	ADDRESS *a;
	char inp[MAXLINE];
	extern char Version[];
	extern tick();
	extern bool iswiz();
	extern char *arpadate();
	extern char *macvalue();
	extern ADDRESS *recipient();
	extern ENVELOPE BlankEnvelope;
	extern ENVELOPE *newenvelope();

	hasmail = FALSE;
	if (OutChannel != stdout)
	{
		/* arrange for debugging output to go to remote host */
		(void) close(1);
		(void) dup(fileno(OutChannel));
	}

	/* open alias database */
	initaliases(AliasFile, FALSE);
	settime();
	if (RealHostName != NULL)
	{
		CurHostName = RealHostName;
		setproctitle("From %s", CurHostName);
	}
	else
	{
		/* this must be us!! */
		CurHostName = MyHostName;
	}

	expand("\001e", inp, &inp[sizeof inp], CurEnv);
	message("220", inp);
	SmtpPhase = "startup";
	for (;;)
	{
		/* arrange for backout */
		if (setjmp(TopFrame) > 0 && InChild)
			finis();
		QuickAbort = FALSE;
		HoldErrs = FALSE;

		/* setup for the read */
		CurEnv->e_to = NULL;
		Errors = 0;
		(void) fflush(stdout);

		/* read the input line */
		p = sfgets(inp, sizeof inp, InChannel);

		/* handle errors */
		if (p == NULL)
		{
			/* end of file, just die */
			message("421", "%s Lost input channel to %s",
				MyHostName, CurHostName);
			finis();
		}

		p = grabcommand( inp, &cmd, &c);

	     commandloop:

		/* process command */
		switch (c->cmdcode)
		{
		  case CMDHELO:		/* hello -- introduce yourself */
			SmtpPhase = "HELO";
			setproctitle("From %s: %s", CurHostName, inp);
			if (!strcasecmp(p, MyHostName))
			{
				/* connected to an echo server */
				message("553", "%s host name configuration error",
					MyHostName);
				break;
			}
			if (RealHostName != NULL)
			{
				char buf[MAXNAME];
				register char *a = CurHostName;
				register char *b = p;

				/*
				 * Verify that hostname matches, but accept
				 * a leading prefix on a domain boundary,
				 * since the network code doesn't know which
				 * domain the other guy is in.  E.g. if we are
				 * talking to "l5" then it can announce itself
				 * as "l5.sun.uucp" and we won't complain.
				 */
				while (lower(*a) == lower(*b)) {
					if (*a == '\0')
						break;
					a++, b++;
				}
				if (*a == '\0' &&
					(*b == '\0' || *b == '.'))
						 goto nameok;
				
				/*
				 * Didn't pass validation.  Use both names.
				 */
				(void) sprintf(buf, "%s (%s)", p, CurHostName);
				define('s', p = newstr(buf), CurEnv);
			}
			else
			{
		nameok:
				define('s', newstr(p), CurEnv);
			}
			message("250", "%s Hello %s, pleased to meet you",
				MyHostName, p);
			break;

		  case CMDMAIL:		/* mail -- designate sender */
			SmtpPhase = "MAIL";
			/* force a sending host even if no HELO given */
			if (RealHostName != NULL && macvalue('s', CurEnv) == NULL)
				define('s', RealHostName, CurEnv);

			/* check for validity of this command */
			if (hasmail)
			{
				message("503", "Sender already specified");
				break;
			}

			initsys();
			setproctitle("%s From %s: %s", CurEnv->e_id,
				CurHostName, inp);

			/* child -- go do the processing */
			p = skipword(p, "from");
			if (p == NULL)
				break;
			setsender(p);
			if (Errors == 0)
			{
				message("250", "Sender ok");
				hasmail = TRUE;
			}
			break;

		  case CMDRCPT:		/* rcpt -- designate recipient */
			SmtpPhase = "RCPT";
			setproctitle("%s From %s: %s", CurEnv->e_id,
				CurHostName, inp);
			if (setjmp(TopFrame) > 0)
			{
				CurEnv->e_flags &= ~EF_FATALERRS;
				break;
			}
			QuickAbort = TRUE;
			p = skipword(p, "to");
			if (p == NULL)
				break;
			a = parseaddr(p, (ADDRESS *) NULL, 1, '\0');
			if (a == NULL)
				break;
			a->q_flags |= QPRIMARY;
			a = recipient(a, &CurEnv->e_sendqueue);
			if (Errors != 0)
				break;

			/* no errors during parsing, but might be a duplicate */
			CurEnv->e_to = p;
			if (!bitset(QBADADDR, a->q_flags))
				message("250", "Recipient ok");
			else
			{
				/* punt -- should keep message in ADDRESS.... */
				message("550", "Addressee unknown");
			}
			CurEnv->e_to = NULL;
			break;

		  case CMDDATA:		/* data -- text of mail */
			SmtpPhase = "DATA";
			if (!hasmail)
			{
				message("503", "Need MAIL command");
				break;
			}
			else if (CurEnv->e_nrcpts <= 0)
			{
				message("503", "Need RCPT (recipient)");
				break;
			}

			/* collect the text of the message */
			SmtpPhase = "collect";
			setproctitle("%s From %s: %s", CurEnv->e_id,
				CurHostName, inp);
			collect(TRUE);
			SmtpPhase = "wait for quit";
			if (Errors != 0)
				break;

			/*
			**  Arrange to send to everyone.
			**	If sending to multiple people, mail back
			**		errors rather than reporting directly.
			**	In any case, don't mail back errors for
			**		anything that has happened up to
			**		now (the other end will do this).
			**	Truncate our transcript -- the mail has gotten
			**		to us successfully, and if we have
			**		to mail this back, it will be easier
			**		on the reader.
			**	Then send to everyone.
			**	Finally give a reply code.  If an error has
			**		already been given, don't mail a
			**		message back.
			**	We goose error returns by clearing error bit.
			*/

			HoldErrs = TRUE;
			ErrorMode = EM_MAIL;

			CurEnv->e_flags &= ~EF_FATALERRS;

			/* 
			 * read the next input line to determine what
			 * mode to send the mail in.  QUIT means close first
			 * and then deliver; otherwise background.
			 */
			message("250", "Mail accepted");
			p = sfgets(inp, sizeof inp, InChannel);
			if (p!=NULL)
			    p = grabcommand( inp, &cmd, &c);
			wasquit = (p == NULL || c->cmdcode == CMDQUIT );
			if (wasquit) 
			  {
			    message("221", "%s delivering mail", MyHostName);
			    fclose(InChannel);
			    fclose(OutChannel);
			  }

			CurEnv->e_xfp = freopen(queuename(CurEnv, 'x'), "w", CurEnv->e_xfp);
			/* send to all recipients */
			sendall(CurEnv, wasquit ? SM_FORK : SM_DELIVER);
			CurEnv->e_to = NULL;

			hasmail = 0;

			/* save statistics */
			markstats(CurEnv, (ADDRESS *) NULL);

			if (wasquit) 
				finis();

			dropenvelope(CurEnv);
			CurEnv = newenvelope(CurEnv);
			CurEnv->e_flags = BlankEnvelope.e_flags;
			QuickAbort = FALSE;
			HoldErrs = FALSE;
			goto commandloop;

		  case CMDRSET:		/* rset -- reset state */
		  	hasmail = 0;
			Errors = 0;
			CurEnv->e_nrcpts = 0;
			CurEnv->e_sendqueue = NULL;
			message("250", "Reset state");
			break;

		  case CMDVRFY:		/* vrfy -- verify address */
			setproctitle("From %s: %s", CurHostName, inp);
			vrfyqueue = NULL;
			QuickAbort = TRUE;
			sendtolist(p, (ADDRESS *) NULL, &vrfyqueue);
			while (vrfyqueue != NULL)
			{
				register ADDRESS *a = vrfyqueue->q_next;
				char *code;

				while (a != NULL && bitset(QDONTSEND|QBADADDR, a->q_flags))
					a = a->q_next;

				if (!bitset(QDONTSEND|QBADADDR, vrfyqueue->q_flags))
				{
					if (a != NULL)
						code = "250-";
					else
						code = "250";
					if (vrfyqueue->q_fullname == NULL)
						message(code, "<%s>", vrfyqueue->q_paddr);
					else
						message(code, "%s <%s>",
						    vrfyqueue->q_fullname, vrfyqueue->q_paddr);
				}
				else if (a == NULL)
					message("554", "Self destructive alias loop");
				vrfyqueue = a;
			}
			break;

		  case CMDHELP:		/* help -- give user info */
			if (*p == '\0')
				p = "SMTP";
			help(p);
			break;

		  case CMDNOOP:		/* noop -- do nothing */
			message("200", "OK");
			break;

		  case CMDQUIT:		/* quit -- leave mail */
			message("221", "%s closing connection", MyHostName);
			finis();

		  case CMDVERB:		/* set verbose mode */
			Verbose = TRUE;
			message("200", "Verbose mode");
			break;

		  case CMDONEX:		/* doing one transaction only */
			message("200", "Only one transaction");
			break;

		  case CMDERROR:	/* unknown command */
			message("500", "Command unrecognized");
			break;

		  default:
			syserr("smtp: unknown code %d", c->cmdcode);
			break;
		}
	}
}
/*
**  SKIPWORD -- skip a fixed word.
**
**	Parameters:
**		p -- place to start looking.
**		w -- word to skip.
**
**	Returns:
**		p following w.
**		NULL on error.
**
**	Side Effects:
**		clobbers the p data area.
*/

static char *
skipword(p, w)
	register char *p;
	char *w;
{
	register char *q;

	/* find beginning of word */
	while (isspace(*p))
		p++;
	q = p;

	/* find end of word */
	while (*p != '\0' && *p != ':' && !isspace(*p))
		p++;
	while (isspace(*p))
		*p++ = '\0';
	if (*p != ':')
	{
	  syntax:
		message("501", "Syntax error");
		Errors++;
		return (NULL);
	}
	*p++ = '\0';
	while (isspace(*p))
		p++;

	/* see if the input word matches desired word */
	if (strcasecmp(q, w))
		goto syntax;

	return (p);
}
/*
**  HELP -- implement the HELP command.
**
**	Parameters:
**		topic -- the topic we want help for.
**
**	Returns:
**		none.
**
**	Side Effects:
**		outputs the help file to message output.
*/

help(topic)
	char *topic;
{
	register FILE *hf;
	int len;
	char buf[MAXLINE];
	bool noinfo;

	if (HelpFile == NULL || (hf = fopen(HelpFile, "r")) == NULL)
	{
		/* no help */
		errno = 0;
		message("502", "HELP not implemented");
		return;
	}

	len = strlen(topic);
	makelower(topic);
	noinfo = TRUE;

	while (fgets(buf, sizeof buf, hf) != NULL)
	{
		if (strncmp(buf, topic, len) == 0)
		{
			register char *p;

			p = index(buf, '\t');
			if (p == NULL)
				p = buf;
			else
				p++;
			fixcrlf(p, TRUE);
			message("214-", p);
			noinfo = FALSE;
		}
	}

	if (noinfo)
		message("504", "HELP topic unknown");
	else
		message("214", "End of HELP info");
	(void) fclose(hf);
}
# endif SMTP
