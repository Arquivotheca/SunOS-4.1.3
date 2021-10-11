#ifndef lint
static	char *sccsid = "@(#)main.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

#include "rcv.h"
#include <sys/stat.h>
#include <locale.h>

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Startup -- interface with user.
 */

jmp_buf	hdrjmp;

/*
 * Find out who the user is, copy his mail file (if exists) into
 * /tmp/Rxxxxx and set up the message pointers.  Then, print out the
 * message headers and read user commands.
 *
 * Command line syntax:
 *	Mail [ -i ] [ -r address ] [ -h number ] [ -f [ name ] ]
 * or:
 *	Mail [ -i ] [ -r address ] [ -h number ] people ...
 *
 * and a bunch of other options.
 */

main(argc, argv)
	char **argv;
{
	register char *ef;
	register int i, argp;
	int mustsend, f;
	void hdrstop(), (*prevint)();
	int fflag;		/* -f was specified */
	int loaded = 0;
#ifdef USG_TTY
	struct termio tbuf;
#else
	struct sgttyb tbuf;
#endif

	/*
 	 * Get locale environment.
	 */
	setlocale(LC_ALL, "");
#ifdef signal
	Siginit();
#endif

	/*
	 * Set up a reasonable environment.  We figure out whether we are
	 * being run interactively, set up all the temporary files, buffer
	 * standard output, and so forth.
	 */

	inithost();
	mypid = getpid();
	intty = isatty(0);
#ifdef USG_TTY
	if (ioctl(1, TCGETA, &tbuf)==0) {
		outtty = 1;
		baud = tbuf.c_cflag & CBAUD;
	}
	else
		baud = B9600;
#else
	if (ioctl(1, TIOCGETP, &tbuf)==0) {
		outtty = 1;
		baud = tbuf.sg_ospeed;
	}
	else
		baud = B9600;
#endif
	image = -1;

	/*
	 * Now, determine how we are being used.
	 * We successively pick off instances of -r, -h, -f, and -i.
	 * If called as "rmail" we note this fact for letter sending.
	 * If there is anything left, it is the base of the list
	 * of users to mail to.  Argp will be set to point to the
	 * first of these users.
	 */

	fflag = 0;
	ef = NOSTR;
	argp = -1;
	mustsend = 0;
	if (argc > 0 && **argv == 'r')
		rmail++;
	for (i = 1; i < argc; i++) {

		/*
		 * If current argument is not a flag, then the
		 * rest of the arguments must be recipients.
		 */

		if (*argv[i] != '-') {
			argp = i;
			break;
		}
		switch (argv[i][1]) {
		case 'e':
			/*
			 * exit status only
			 */
			exitflg++;
			break;

		case 'r':
			/*
			 * Next argument is address to be sent along
			 * to the mailer.
			 */
			if (i >= argc - 1) {
				fprintf(stderr, "Address required after -r\n");
				exit(1);
			}
			mustsend++;
			rflag = argv[i+1];
			i++;
			break;

		case 'T':
			/*
			 * Next argument is temp file to write which
			 * articles have been read/deleted for netnews.
			 */
			if (i >= argc - 1) {
				fprintf(stderr, "Name required after -T\n");
				exit(1);
			}
			Tflag = argv[i+1];
			if ((f = creat(Tflag, 0600)) < 0) {
				perror(Tflag);
				exit(1);
			}
			close(f);
			i++;
			/* fall through for -I too */

		case 'I':
			/*
			 * print newsgroup in header summary
			 */
			newsflg++;
			break;

		case 'u':
			/*
			 * Next argument is person to pretend to be.
			 */
			if (i >= argc - 1) {
				fprintf(stderr, "Missing user name for -u\n");
				exit(1);
			}
			strcpy(myname, argv[i+1]);
			i++;
			break;

		case 'i':
			/*
			 * User wants to ignore interrupts.
			 * Set the variable "ignore"
			 */
			assign("ignore", "");
			break;
		
		case 'U':
			UnUUCP++;
			break;

		case 'd':
			assign("debug", "");
			break;

		case 'h':
			/*
			 * Specified sequence number for network.
			 * This is the number of "hops" made so
			 * far (count of times message has been
			 * forwarded) to help avoid infinite mail loops.
			 */
			if (i >= argc - 1) {
				fprintf(stderr, "Number required for -h\n");
				exit(1);
			}
			mustsend++;
			hflag = atoi(argv[i+1]);
			if (hflag == 0) {
				fprintf(stderr, "-h needs non-zero number\n");
				exit(1);
			}
			i++;
			break;

		case 's':
			/*
			 * Give a subject field for sending from
			 * non terminal
			 */
			if (i >= argc - 1) {
				fprintf(stderr, "Subject req'd for -s\n");
				exit(1);
			}
			mustsend++;
			sflag = argv[i+1];
			i++;
			break;

		case 'f':
			/*
			 * User is specifying file to "edit" with mailx,
			 * as opposed to reading system mailbox.
			 * If no argument is given after -f, we read his
			 * mbox file in his home directory.
			 */
			fflag++;
			if (i < argc - 1)
				ef = argv[i + 1];
			i++;
			break;

		case 'F':
			Fflag++;
			mustsend++;
			break;

		case 'n':
			/*
			 * User doesn't want to source
			 *	/usr/lib/Mail.rc
			 */
			nosrc++;
			break;

		case 'N':
			/*
			 * Avoid initial header printing.
			 */
			noheader++;
			break;

		case 'H':
			/*
			 * Print headers and exit
			 */
			Hflag++;
			break;

		case 'v':
			/*
			 * Send mailer verbose flag
			 */
			assign("verbose", "");
			break;

		case 'B':
			/*
			 * Don't buffer output
			 */
			setlinebuf(stdout);	/* a good compromise */
			setlinebuf(stderr);
			break;

		case 't':
			/*
			 * Like sendmail -t, read headers from text
			 */
			tflag++;
			mustsend++;
			break;

		default:
			fprintf(stderr, "Unknown flag: %s\n", argv[i]);
			exit(1);
		}
	}

	/*
	 * Check for inconsistent arguments.
	 */

	if (newsflg && !fflag) {
		fprintf(stderr, "Need -f with -I flag\n");
		exit(1);
	}
	if (fflag && argp != -1) {
		fprintf(stderr, "Cannot give -f and people to send to.\n");
		exit(1);
	}
	if (exitflg && (mustsend || argp != -1))
		exit(1);	/* nonsense flags involving -e simply exit */
	if (tflag && argp != -1) {
		fprintf(stderr, "Ignoring recipients on command line with -t\n");
		argp = -1;
	} else if (!tflag && mustsend && argp == -1) {
		fprintf(stderr, "The flags you gave are used only when sending mail.\n");
		exit(1);
	}
	tinit();
	input = stdin;
	rcvmode = !tflag && argp == -1;
	if (!nosrc)
		load(MASTER);

	if (!rcvmode) {
		load(Getf("MAILRC"));
		if (tflag)
			tmail();
		else
			mail(&argv[argp]);

		/*
		 * why wait?
		 */

		exit(senderr);
	}

	/*
	 * Ok, we are reading mail.
	 * Decide whether we are editing a mailbox or reading
	 * the system mailbox, and open up the right stuff.
	 *
	 * Do this before sourcing the MAILRC, because there might be
	 * a 'chdir' there that breaks the -f option.  But if the
	 * file specified with -f is a folder name, go ahead and
	 * source the MAILRC anyway so that "folder" will be defined.
	 */

	if (fflag) {
		char *ename;

		edit++;
		if (ef == NOSTR || *ef == '+') {
			load(Getf("MAILRC"));
			loaded++;
			if (ef == NOSTR) {
				ef = (char *) calloc(1, strlen(Getf("MBOX"))+1);
				strcpy(ef, Getf("MBOX"));
			}
		}
		ename = expand(ef);
		if (ename != ef) {
			ef = (char *) calloc(1, strlen(ename) + 1);
			strcpy(ef, ename);
		}
		editfile = ef;
		strcpy(mailname, ef);
	}

	if (setfile(mailname, edit) < 0)
		exit(1);

	if (!loaded)
		load(Getf("MAILRC"));
	if (msgCount > 0 && !noheader && value("header") != NOSTR) {
		if (setjmp(hdrjmp) == 0) {
			if ((prevint = sigset(SIGINT, SIG_IGN)) != SIG_IGN)
				sigset(SIGINT, hdrstop);
			announce();
			fflush(stdout);
			sigset(SIGINT, prevint);
		}
	}
	if (Hflag || (!edit && msgCount == 0)) {
		if (!Hflag)
			fprintf(stderr, "No mail for %s\n", myname);
		fflush(stdout);
		exit(0);
	}
	commands();
	sigset(SIGHUP, SIG_IGN);
	sigset(SIGINT, SIG_IGN);
	sigset(SIGQUIT, SIG_IGN);
	if (!outtty)
		sigset(SIGPIPE, SIG_IGN);
	if (edit)
		edstop(0);
	else
		quit(0);
	exit(0);
	/* NOTREACHED */
}

/*
 * Interrupt printing of the headers.
 */
void
hdrstop()
{

	fflush(stdout);
	fprintf(stderr, "\nInterrupt\n");
# ifndef VMUNIX
	sigrelse(SIGINT);
# endif
	longjmp(hdrjmp, 1);
}
