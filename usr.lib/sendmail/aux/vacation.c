/*
**  Vacation
**  Copyright (c) 1983  Eric P. Allman
**  Berkeley, California
**
**  Copyright (c) 1983 Regents of the University of California.
**  All rights reserved.  The Berkeley software License Agreement
**  specifies the terms and conditions for redistribution.
*/

#ifndef lint
static char	SccsId[] = "@(#)vacation.c 1.1 92/07/30 SMI"; /* from UCB 5.3 7/1/85 */
#endif not lint

# include <pwd.h>
# include <stdio.h>
# include <sysexits.h>
# include <ctype.h>
# include "useful.h"
# include "userdbm.h"

/*
**  VACATION -- return a message to the sender when on vacation.
**
**	This program could be invoked as a message receiver
**	when someone is on vacation.  It returns a message
**	specified by the user to whoever sent the mail, taking
**	care not to return a message too often to prevent
**	"I am on vacation" loops.
**
**	For best operation, this program should run setuid to
**	root or uucp or someone else that sendmail will believe
**	a -f flag from.  Otherwise, the user must be careful
**	to include a header on his .vacation.msg file.
**
**	Positional Parameters:
**		the user to collect the vacation message from.
**
**	Flag Parameters:
**		-I	initialize the database.
**		-d	turn on debugging.
**		-tT	set the timeout to T.  messages arriving more
**			often than T will be ignored to avoid loops.
**
**	Side Effects:
**		A message is sent back to the sender.
**
**	Author:
**		Eric Allman
**		UCB/INGRES
*/

# define MAXLINE	256	/* max size of a line */

# define ONEWEEK	(60L*60L*24L*7L)
# define MsgFile "/.vacation.msg"

time_t	Timeout = ONEWEEK;	/* timeout between notices per user */

struct dbrec
{
	long	sentdate;
};

bool	Debug = FALSE;
bool	AnswerAll = FALSE;	/* default to answer if in To:/Cc: only */
char	*myname;		/* name of person "on vacation" */
char	*homedir;		/* home directory of said person */
char	*Subject = "";		/* subject in message header */
char	*AliasList[MAXLINE];	/* list of aliases to allow */
int	AliasCount = 0;

main(argc, argv)
	char **argv;
{
	char *from;
	register char *p;
	struct passwd *pw;
	char *shortfrom;
	char buf[MAXLINE];
	extern struct passwd *getpwnam();
	extern char *newstr();
	extern char *getfrom();
	extern bool knows();
	extern bool junkmail();
	extern time_t convtime();

	if (argc == 1) {
		AutoInstall();
		exit (EX_OK);
	}

	/* process arguments */
	while (--argc > 0 && (p = *++argv) != NULL && *p == '-')
	{
		switch (*++p)
		{
		  case 'a':	/* answer all mail, even if not in To/Cc */
			AliasList[AliasCount++] = argv[1];
			if (argc > 0) {
				argc--; argv++;
			}
			break;

		  case 'd':	/* debug */
			Debug = TRUE;
			break;

		  case 'I':	/* initialize */
			initialize();
			exit(EX_OK);

		  case 'j':	/* answer all mail, even if not in To/Cc */
			AnswerAll = TRUE;
			break;

		  case 't':	/* set timeout */
			Timeout = convtime(++p);
			break;

		  default:
			usrerr("Unknown flag -%s", p);
			exit(EX_USAGE);
		}
	}

	/* verify recipient argument */
	if (argc != 1)
	{
		usrerr("Usage: vacation [-j] [-a alias] [-tN] username  (or)  vacation -I");
		exit(EX_USAGE);
	}

	myname = p;

	/* find user's home directory */
	pw = getpwnam(myname);
	if (pw == NULL)
	{
		usrerr("Unknown user %s", myname);
		exit(EX_NOUSER);
	}
	homedir = newstr(pw->pw_dir);

	(void) strcpy(buf, homedir);
	(void) strcat(buf, "/.vacation");
	dbminit(buf);

	/* read message from standard input (just from line) */
	from = getfrom(&shortfrom);

	/* check if junk mail or this person is already informed */
	if (!junkmail(shortfrom) && !knows(shortfrom))
	{
		/* mark this person as knowing */
		setknows(shortfrom);

		/* send the message back */
		(void) strcpy(buf, homedir);
		(void) strcat(buf, MsgFile);
		if (Debug)
			printf("Sending %s to %s\n", buf, from);
		else
		{
			sendmessage(buf, from, myname);
			/*NOTREACHED*/
		}
	}
	exit (EX_OK);
}
/*
**  GETFROM -- read message from standard input and return sender
**
**	Parameters:
**		none.
**
**	Returns:
**		pointer to the sender address.
**
**	Side Effects:
**		Reads first line from standard input.
*/

char *
getfrom(shortp)
char **shortp;
{
	static char line[MAXLINE];
	register char *p, *start, *at, *bang;
	char saveat;

	/* read the from line */
	if (fgets(line, sizeof line, stdin) == NULL ||
	    strncmp(line, "From ", 5) != NULL)
	{
		usrerr("No initial From line");
		exit(EX_USAGE);
	}

	/* find the end of the sender address and terminate it */
	start = &line[5];
	p = index(start, ' ');
	if (p == NULL)
	{
		usrerr("Funny From line '%s'", line);
		exit(EX_USAGE);
	}
	*p = '\0';

	/*
	 * Strip all but the rightmost UUCP host
	 * to prevent loops due to forwarding.
	 * Start searching leftward from the leftmost '@'.
	 *	a!b!c!d yields a short name of c!d
	 *	a!b!c!d@e yields a short name of c!d@e
	 *	e@a!b!c yields the same short name
	 */
#ifdef VDEBUG
printf("start='%s'\n", start);
#endif VDEBUG
	*shortp = start;			/* assume whole addr */
	if ((at = index(start, '@')) == NULL)	/* leftmost '@' */
		at = p;				/* if none, use end of addr */
	saveat = *at;
	*at = '\0';
	if ((bang = rindex(start, '!')) != NULL) {	/* rightmost '!' */
		char *bang2;
		*bang = '\0';
		if ((bang2 = rindex(start, '!')) != NULL) /* 2nd rightmost '!' */
			*shortp = bang2 + 1;		/* move past ! */
		*bang = '!';
	}
	*at = saveat;
#ifdef VDEBUG
printf("place='%s'\n", *shortp);
#endif VDEBUG

	/* return the sender address */
	return start;
}
/*
**  JUNKMAIL -- read the header and tell us if this is junk/bulk mail.
**
**	Parameters:
**		from -- the Return-Path of the sender.  We assume that
**			anything from "*-REQUEST@*" is bulk mail.
**
**	Returns:
**		TRUE -- if this is junk or bulk mail (that is, if the
**			sender shouldn't receive a response).
**		FALSE -- if the sender deserves a response.
**
**	Side Effects:
**		May read the header from standard input.  When this
**		returns the position on stdin is undefined.
*/

bool
junkmail(from)
	char *from;
{
	register char *p;
	char buf[MAXLINE+1];
	extern char *index();
	extern char *rindex();
	extern char *strtok();
	extern bool sameword();
	bool inside, foundto, onlist;

	/* test for inhuman sender */
	p = rindex(from, '@');
	if (p != NULL)
	{
		*p = '\0';
		if (sameword(&p[-8],  "-REQUEST") ||
		    sameword(&p[-10], "Postmaster") ||
		    sameword(&p[-13], "MAILER-DAEMON"))
		{
			*p = '@';
			return (TRUE);
		}
		*p = '@';
	}

# define Delims " \t\n,:;()<>@.!"

	/* read the header looking for "interesting" lines */
	inside = FALSE;
	onlist = FALSE;
	foundto = FALSE;
	while (fgets(buf, MAXLINE, stdin) != NULL && buf[0] != '\n')
	{
		if (buf[0]!=' ' && buf[0] != '\t' && index(buf,':') == NULL)
			return(FALSE);			/* no header found */

		p = strtok(buf, Delims);
		if (p==NULL)
			continue;

		if (sameword(p, "To") || sameword(p, "Cc"))
		{
			inside = TRUE;
			p = strtok(NULL, Delims);
			if (p==NULL)
				continue;

		}
		else				/* continuation line? */
		    if (inside)			
		    	inside =  (buf[0]==' ' || buf[0]=='\t');

		if (inside) {
		    int i;

		    do {
			if (sameword(p,myname))
				onlist = TRUE;		/* I am on the list */

			for (i=0;i<AliasCount;i++)
			    if (sameword(p,AliasList[i]))
				onlist = TRUE;		/* alias on list */

		    } while (p = strtok(NULL, Delims));
		    foundto = TRUE;
		    continue;
		}
		
		if (sameword(p, "Precedence"))
		{
			/* find the value of this field */
			p = strtok(NULL, Delims);
			if (p == NULL)
				continue;

			/* see if it is "junk" or "bulk" */
			p[4] = '\0';
			if (sameword(p, "junk") || sameword(p, "bulk"))
				return (TRUE);
		}

		if (sameword(p, "Subject"))
		{
			Subject = newstr(buf+9);
			if (p = rindex(Subject,'\n'))
				*p = '\0';
			if (Debug) 
				printf("Subject=%s\n", Subject);
		}
	}
	if (AnswerAll)
		return (FALSE);
	else
		return (foundto && !onlist);
}
/*
**  KNOWS -- predicate telling if user has already been informed.
**
**	Parameters:
**		user -- the user who sent this message.
**
**	Returns:
**		TRUE if 'user' has already been informed that the
**			recipient is on vacation.
**		FALSE otherwise.
**
**	Side Effects:
**		none.
*/

bool
knows(user)
	char *user;
{
	DATUM k, d;
	long now;
	auto long then;

	time(&now);
	k.dptr = user;
	k.dsize = strlen(user) + 1;
	d = fetch(k);
	if (d.dptr == NULL)
		return (FALSE);
	
	/* be careful on 68k's and others with alignment restrictions */
	bcopy((char *) &((struct dbrec *) d.dptr)->sentdate, (char *) &then, sizeof then);
	if (then + Timeout < now)
		return (FALSE);
	if (Debug)
		printf("User %s already knows\n");
	return (TRUE);
}
/*
**  SETKNOWS -- set that this user knows about the vacation.
**
**	Parameters:
**		user -- the user who should be marked.
**
**	Returns:
**		none.
**
**	Side Effects:
**		The dbm file is updated as appropriate.
*/

setknows(user)
	char *user;
{
	DATUM k, d;
	struct dbrec xrec;

	k.dptr = user;
	k.dsize = strlen(user) + 1;
	time(&xrec.sentdate);
	d.dptr = (char *) &xrec;
	d.dsize = sizeof xrec;
	store(k, d);
}
/*
**  SENDMESSAGE -- send a message to a particular user.
**
**	Parameters:
**		msgf -- filename containing the message.
**		user -- user who should receive it.
**
**	Returns:
**		none.
**
**	Side Effects:
**		sends mail to 'user' using /usr/lib/sendmail.
*/

sendmessage(msgf, user, myname)
	char *msgf;
	char *user;
	char *myname;
{
	FILE *f, *pipe;
	char line[MAXLINE];
	char *p;

	/* find the message to send */
	f = fopen(msgf, "r");
	if (f == NULL)
	{
		f = fopen("/usr/lib/vacation.def", "r");
		if (f == NULL)
			syserr("No message to send");
	}

	(void) sprintf(line, "/usr/lib/sendmail -eq -f %s %s", myname, user);
	pipe = popen(line, "w");
	if (pipe == NULL)
		syserr("Cannot exec /usr/lib/sendmail");
	while (fgets(line, MAXLINE, f)) {
		p = index(line,'$');
		if (p && strncmp(p,"$SUBJECT",8)==0) {
			*p = '\0';
			fputs(line, pipe);
			fputs(Subject, pipe);
			fputs(p+8, pipe);
			continue;
		}
		fputs(line, pipe);
	}
	pclose(pipe);
	fclose(f);
}
/*
**  INITIALIZE -- initialize the database before leaving for vacation
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Initializes the files .vacation.{pag,dir} in the
**		caller's home directory.
*/

initialize()
{
	char *homedir;
	char buf[MAXLINE];
	extern char *getenv();

	setgid(getgid());
	setuid(getuid());
	homedir = getenv("HOME");
	if (homedir == NULL)
		syserr("No home!");
	(void) strcpy(buf, homedir);
	(void) strcat(buf, "/.vacation.dir");
	if (close(creat(buf, 0644)) < 0)
		syserr("Cannot create %s", buf);
	(void) strcpy(buf, homedir);
	(void) strcat(buf, "/.vacation.pag");
	if (close(creat(buf, 0644)) < 0)
		syserr("Cannot create %s", buf);
}
/*
**  USRERR -- print user error
**
**	Parameters:
**		f -- format.
**		p -- first parameter.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

usrerr(f, p)
	char *f;
	char *p;
{
	fprintf(stderr, "vacation: ");
	_doprnt(f, &p, stderr);
	fprintf(stderr, "\n");
}
/*
**  SYSERR -- print system error
**
**	Parameters:
**		f -- format.
**		p -- first parameter.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

syserr(f, p)
	char *f;
	char *p;
{
	fprintf(stderr, "vacation: ");
	_doprnt(f, &p, stderr);
	fprintf(stderr, "\n");
	exit(EX_USAGE);
}
/*
**  NEWSTR -- copy a string
**
**	Parameters:
**		s -- the string to copy.
**
**	Returns:
**		A copy of the string.
**
**	Side Effects:
**		none.
*/

char *
newstr(s)
	char *s;
{
	char *p;
	extern char *malloc();

	p = malloc((unsigned)strlen(s) + 1);
	if (p == NULL)
	{
		syserr("newstr: cannot alloc memory");
		exit(EX_OSERR);
	}
	strcpy(p, s);
	return (p);
}
/*
**  SAMEWORD -- return TRUE if the words are the same
**
**	Ignores case.
**
**	Parameters:
**		a, b -- the words to compare.
**
**	Returns:
**		TRUE if a & b match exactly (modulo case)
**		FALSE otherwise.
**
**	Side Effects:
**		none.
*/

bool
sameword(a, b)
	register char *a, *b;
{
	char ca, cb;

	do
	{
		ca = *a++;
		cb = *b++;
		if (isascii(ca) && isupper(ca))
			ca = ca - 'A' + 'a';
		if (isascii(cb) && isupper(cb))
			cb = cb - 'A' + 'a';
	} while (ca != '\0' && ca == cb);
	return (ca == cb);
}

/*
 * When invoked with no arguments, we fall into an automatic installation
 * mode, stepping the user through a default installation.
 */
AutoInstall()
{
	char file[MAXLINE];
	char forward[MAXLINE];
	char cmd[MAXLINE];
	char line[MAXLINE];
	char *editor;
	FILE *f;

myname = getenv("USER");
homedir = getenv("HOME");
if (homedir == NULL)
	syserr("Home directory unknown");

printf("This program can be used to answer your mail automatically\n");
printf("when you go away on vacation.\n");
(void) strcpy(file, homedir);
(void) strcat(file, MsgFile);
do {
	f = fopen(file,"r");
	if (f) {
printf("You have a message file in %s.\n", file);
if (ask("Would you like to see it")) {
	sprintf(cmd, "/usr/ucb/more %s", file);
	system(cmd);
}
if (ask("Would you like to edit it"))
		f = NULL;
	}
	else {
printf("You need to create a message file in %s first.\n", file);
		f = fopen(file,"w");
		if (myname)
fprintf(f,"From: %s (via the vacation program)\n", myname);
fprintf(f,"Subject: away from my mail\n");
fprintf(f,"\nI will not be reading my mail for a while.\n");
fprintf(f,"Your mail regarding \"$SUBJECT\" will be read when I return.\n");
		fclose(f);
		f = NULL;
	}
	if (f == NULL) {
		editor = getenv("VISUAL");
		if (editor == NULL)
			editor = getenv("EDITOR");
		if (editor == NULL)
			editor = "/usr/ucb/vi";
		sprintf(cmd, "%s %s", editor, file);
printf("Please use your editor (%s) to edit this file.\n", editor);
		system(cmd);
	}
} while (f == NULL);
fclose(f);
(void) strcpy(forward, homedir);
(void) strcat(forward, "/.forward");
f = fopen(forward,"r");
if (f) {
printf("You have a .forward file in your home directory containing:\n");
while (fgets(line, MAXLINE, f))
	printf("    %s", line);
fclose(f);
if (!ask("Would you like to remove it and disable the vacation feature")) 
	exit(0);
if (unlink(forward))
	perror("Error removing .forward file:");
else
	printf("Back to normal reception of mail.\n");
exit(0);
}

printf("To enable the vacation feature a \".forward\" file is created.\n");
if (!ask("Would you like to enable the vacation feature")) {
	printf("OK, vacation feature NOT enabled.\n");
	exit(0);
}
f = fopen(forward,"w");
if (f==NULL) {
	perror("Error opening .forward file");
	exit(EX_USAGE);
}
fprintf(f,"\\%s, \"|/usr/ucb/vacation %s\"\n", myname, myname);
fclose(f);
printf("Vacation feature ENABLED. Please remember to turn it off when\n");
printf("you get back from vacation. Bon voyage.\n");

initialize();
}


/*
 * Ask the user a question until we get a reasonable answer
 */
ask(prompt)
	char *prompt;
{
	char line[MAXLINE];

	while (1) {
		printf("%s? ", prompt);
		fflush(stdout);
		gets(line);
		if (line[0]=='y' || line[0]=='Y') return(TRUE);
		if (line[0]=='n' || line[0]=='N') return(FALSE);
		printf("Please reply \"yes\" or \"no\" (\'y\' or \'n\')\n");
	}
	
}
