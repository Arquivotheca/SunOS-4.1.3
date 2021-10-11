/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)stty.c 1.1 92/07/30 SMI"; /* from S5R3 1.11 */
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <ctype.h>
#include <locale.h>

extern void exit();
extern void perror();

struct
{
	char	*string;
	int	speed;
} speeds[] = {
	"0",		B0,
	"50",		B50,
	"75",		B75,
	"110",		B110,
	"134",		B134,
	"134.5",	B134,
	"150",		B150,
	"200",		B200,
	"300",		B300,
	"600",		B600,
	"1200",		B1200,
	"1800",		B1800,
	"2400",		B2400,
	"4800",		B4800,
	"9600",		B9600,
	"19200",	B19200,
	"19.2",		B19200,
	"exta",		EXTA,
	"38400",	B38400,
	"38.4",		B38400,
	"extb",		EXTB,
	0,
};
struct mds {
	char	*string;
	unsigned long	set;
	unsigned long	reset;
};
						/* Control Modes */
struct mds cmodes[] = {
	"-parity", CS8, PARENB|CSIZE,
	"-evenp", CS8, PARENB|CSIZE,
	"-oddp", CS8, PARENB|PARODD|CSIZE,
	"parity", PARENB|CS7, PARODD|CSIZE,
	"evenp", PARENB|CS7, PARODD|CSIZE,
	"oddp", PARENB|PARODD|CS7, CSIZE,
	"parenb", PARENB, 0,
	"-parenb", 0, PARENB,
	"parodd", PARODD, 0,
	"-parodd", 0, PARODD,
	"cs8", CS8, CSIZE,
	"cs7", CS7, CSIZE,
	"cs6", CS6, CSIZE,
	"cs5", CS5, CSIZE,
	"cstopb", CSTOPB, 0,
	"-cstopb", 0, CSTOPB,
	"stopb", CSTOPB, 0,
	"-stopb", 0, CSTOPB,
	"hupcl", HUPCL, 0,
	"hup", HUPCL, 0,
	"-hupcl", 0, HUPCL,
	"-hup", 0, HUPCL,
	"clocal", CLOCAL, 0,
	"-clocal", 0, CLOCAL,
	"nohang", CLOCAL, 0,
	"-nohang", 0, CLOCAL,
#if 0		/* this bit isn't supported */
	"loblk", LOBLK, 0,
	"-loblk", 0, LOBLK,
#endif
	"cread", CREAD, 0,
	"-cread", 0, CREAD,
	"crtscts", CRTSCTS, 0,
	"-crtscts", 0, CRTSCTS,
	"litout", CS8, (CSIZE|PARENB),
	"-litout", (CS7|PARENB), CSIZE,
	"pass8", CS8, (CSIZE|PARENB),
	"-pass8", (CS7|PARENB), CSIZE,
	"raw", CS8, (CSIZE|PARENB),
	"-raw", (CS7|PARENB), CSIZE,
	"cooked", (CS7|PARENB), CSIZE,
	"sane", (CS7|PARENB|CREAD), (CSIZE|PARODD|CLOCAL),
	0
};
						/* Input Modes */
struct mds imodes[] = {
	"ignbrk", IGNBRK, 0,
	"-ignbrk", 0, IGNBRK,
	"brkint", BRKINT, 0,
	"-brkint", 0, BRKINT,
	"ignpar", IGNPAR, 0,
	"-ignpar", 0, IGNPAR,
	"parmrk", PARMRK, 0,
	"-parmrk", 0, PARMRK,
	"inpck", INPCK, 0,
	"-inpck", 0,INPCK,
	"istrip", ISTRIP, 0,
	"-istrip", 0, ISTRIP,
	"inlcr", INLCR, 0,
	"-inlcr", 0, INLCR,
	"igncr", IGNCR, 0,
	"-igncr", 0, IGNCR,
	"icrnl", ICRNL, 0,
	"-icrnl", 0, ICRNL,
	"-nl", ICRNL, (INLCR|IGNCR),
	"nl", 0, ICRNL,
	"iuclc", IUCLC, 0,
	"-iuclc", 0, IUCLC,
	"lcase", IUCLC, 0,
	"-lcase", 0, IUCLC,
	"LCASE", IUCLC, 0,
	"-LCASE", 0, IUCLC,
	"ixon", IXON, 0,
	"-ixon", 0, IXON,
	"ixany", IXANY, 0,
	"-ixany", 0, IXANY,
	"decctlq", 0, IXANY,
	"-decctlq", IXANY, 0,
	"ixoff", IXOFF, 0,
	"-ixoff", 0, IXOFF,
	"tandem", IXOFF, 0,
	"-tandem", 0, IXOFF,
	"imaxbel", IMAXBEL, 0,
	"-imaxbel", 0, IMAXBEL,
	"pass8", 0, ISTRIP,
	"-pass8", ISTRIP, 0,
	"raw", 0, -1,
	"-raw", (BRKINT|IGNPAR|ISTRIP|ICRNL|IXON|IMAXBEL), 0,
	"cooked", (BRKINT|IGNPAR|ISTRIP|ICRNL|IXON), 0,
	"sane", (BRKINT|IGNPAR|ISTRIP|ICRNL|IXON|IMAXBEL),
		(IGNBRK|PARMRK|INPCK|INLCR|IGNCR|IUCLC|IXOFF),
	0
};
						/* Local Modes */
struct mds lmodes[] = {
	"isig", ISIG, 0,
	"-isig", 0, ISIG,
	"noisig", 0, ISIG,
	"-noisig", ISIG, 0,
	"iexten", IEXTEN, 0,
	"-iexten", 0, IEXTEN,
	"icanon", ICANON, 0,
	"-icanon", 0, ICANON,
	"cbreak", 0, ICANON,
	"-cbreak", ICANON, 0,
	"xcase", XCASE, 0,
	"-xcase", 0, XCASE,
	"lcase", XCASE, 0,
	"-lcase", 0, XCASE,
	"LCASE", XCASE, 0,
	"-LCASE", 0, XCASE,
	"echo", ECHO, 0,
	"-echo", 0, ECHO,
	"echoe", ECHOE, 0,
	"-echoe", 0, ECHOE,
	"crterase", ECHOE, 0,
	"-crterase", 0, ECHOE,
	"echok", ECHOK, 0,
	"-echok", 0, ECHOK,
	"lfkc", ECHOK, 0,
	"-lfkc", 0, ECHOK,
	"echonl", ECHONL, 0,
	"-echonl", 0, ECHONL,
	"noflsh", NOFLSH, 0,
	"-noflsh", 0, NOFLSH,
	"tostop", TOSTOP, 0,
	"-tostop", 0, TOSTOP,
	"echoctl", ECHOCTL, 0,
	"-echoctl", 0, ECHOCTL,
	"ctlecho", ECHOCTL, 0,
	"-ctlecho", 0, ECHOCTL,
	"echoprt", ECHOPRT, 0,
	"-echoprt", 0, ECHOPRT,
	"prterase", ECHOPRT, 0,
	"-prterase", 0, ECHOPRT,
	"echoke", ECHOKE, 0,
	"-echoke", 0, ECHOKE,
	"crtkill", ECHOKE, 0,
	"-crtkill", 0, ECHOKE,
#if 0		/* this bit isn't supported yet */
	"defecho", DEFECHO, 0,
	"-defecho", 0, DEFECHO,
#endif
	"raw", 0, (ISIG|ICANON|XCASE|IEXTEN),
	"-raw", (ISIG|ICANON|IEXTEN), 0,
	"cooked", (ISIG|ICANON), 0,
	"sane", (ISIG|ICANON|ECHO|ECHOE|ECHOK|ECHOCTL|ECHOKE),
		(XCASE|ECHOPRT|ECHONL|NOFLSH),
	0,
};
						/* Output Modes */
struct mds omodes[] = {
	"opost", OPOST, 0,
	"-opost", 0, OPOST,
	"nopost", 0, OPOST,
	"-nopost", OPOST, 0,
	"olcuc", OLCUC, 0,
	"-olcuc", 0, OLCUC,
	"lcase", OLCUC, 0,
	"-lcase", 0, OLCUC,
	"LCASE", OLCUC, 0,
	"-LCASE", 0, OLCUC,
	"onlcr", ONLCR, 0,
	"-onlcr", 0, ONLCR,
	"-nl", ONLCR, (OCRNL|ONLRET),
	"nl", 0, ONLCR,
	"ocrnl", OCRNL, 0,
	"-ocrnl",0, OCRNL,
	"onocr", ONOCR, 0,
	"-onocr", 0, ONOCR,
	"onlret", ONLRET, 0,
	"-onlret", 0, ONLRET,
	"fill", OFILL, OFDEL,
	"-fill", 0, OFILL|OFDEL,
	"nul-fill", OFILL, OFDEL,
	"del-fill", OFILL|OFDEL, 0,
	"ofill", OFILL, 0,
	"-ofill", 0, OFILL,
	"ofdel", OFDEL, 0,
	"-ofdel", 0, OFDEL,
	"cr0", CR0, CRDLY,
	"cr1", CR1, CRDLY,
	"cr2", CR2, CRDLY,
	"cr3", CR3, CRDLY,
	"tab0", TAB0, TABDLY,
	"tabs", TAB0, TABDLY,
	"tab1", TAB1, TABDLY,
	"tab2", TAB2, TABDLY,
	"-tabs", XTABS, TABDLY,
	"tab3", XTABS, TABDLY,
	"nl0", NL0, NLDLY,
	"nl1", NL1, NLDLY,
	"ff0", FF0, FFDLY,
	"ff1", FF1, FFDLY,
	"vt0", VT0, VTDLY,
	"vt1", VT1, VTDLY,
	"bs0", BS0, BSDLY,
	"bs1", BS1, BSDLY,
#if 0		/* these bits aren't supported yet */
	"pageout", PAGEOUT, 0,
	"-pageout", 0, PAGEOUT,
	"wrap", WRAP, 0,
	"-wrap", 0, WRAP,
#endif
	"litout", 0, OPOST,
	"-litout", OPOST, 0,
	"raw", 0, OPOST,
	"-raw", OPOST, 0,
	"cooked", OPOST, 0,
	"33", CR1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"tty33", CR1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"tn", CR1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"tn300", CR1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"ti", CR2, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"ti700", CR2, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"05", NL1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"vt05", NL1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"tek", FF1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"37", (FF1|VT1|CR2|TAB1|NL1), (NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY),
	"tty37", (FF1|VT1|CR2|TAB1|NL1), (NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY),
	"sane", (OPOST|ONLCR), (OLCUC|OCRNL|ONOCR|ONLRET|OFILL|OFDEL|
			NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY),
	0,
};

struct termios cb;
struct winsize size;

struct	special {
	char	*name;
	unsigned char	*cp;
	unsigned char	def;
} special[] = {
	"intr",		&cb.c_cc[VINTR],	CINTR,
	"quit",		&cb.c_cc[VQUIT],	CQUIT,
	"erase",	&cb.c_cc[VERASE],	CERASE,
	"kill",		&cb.c_cc[VKILL],	CKILL,
	"eof",		&cb.c_cc[VEOF],		CEOF,
	"eol",		&cb.c_cc[VEOL],		CEOL,
	"eol2",		&cb.c_cc[VEOL2],	CEOL2,
#if 0		/* this character isn't supported */
	"swtch",	&cb.c_cc[VSWTCH],	CSWTCH,
#endif
	"start",	&cb.c_cc[VSTART],	CSTART,
	"stop",		&cb.c_cc[VSTOP],	CSTOP,
	"susp",		&cb.c_cc[VSUSP],	CSUSP,
	"dsusp",	&cb.c_cc[VDSUSP],	CDSUSP,
	"rprnt",	&cb.c_cc[VREPRINT],	CRPRNT,
	"flush",	&cb.c_cc[VDISCARD],	CFLUSH,
	"werase",	&cb.c_cc[VWERASE],	CWERASE,
	"lnext",	&cb.c_cc[VLNEXT],	CLNEXT,
	0
};

char	*arg;					/* arg: ptr to mode to be set */
int	match;
char	USAGE[] = "usage: stty [-ag] [modes]\n";
int	pitt = 0;

#ifdef S5EMUL
#define	ioctl_desc	0
#define	output		stdout
#else
#define	ioctl_desc	1
#define	output		stderr
#endif

main(argc, argv)
char	*argv[];
{
	register i;
	register struct special *sp;
#ifndef S5EMUL
	char obuf[BUFSIZ];
#endif

#ifndef S5EMUL
	setbuf(stderr, obuf);
#endif
	setlocale(LC_ALL, "");

	if (argc == 2) {
		/*
		 * "stty size", "stty speed", and "stty -g" are intended for
		 * use within backquotes; thus, they do the "fetch" "ioctl" 
		 * from "/dev/tty" and always print their result on the 
		 * standard output.
		 * Since their standard output is likely to be a pipe, they
		 * should not try to read the modes from the standard output.
		 */
		if (strcmp(argv[1], "size") == 0) {
			if ((i = open("/dev/tty", 0)) < 0) {
				perror("stty: Cannot open /dev/tty");
				exit(2);
			}
			if (ioctl(i, TIOCGWINSZ, &size) < 0) {
				perror("stty: TIOCGWINSZ");
				exit(2);
			}
			(void) printf("%d %d\n", size.ws_row, size.ws_col);
			exit(0);
		}
		else if (strcmp(argv[1], "speed") == 0) {
			if ((i = open("/dev/tty", 0)) < 0) {
				perror("stty: Cannot open /dev/tty");
				exit(2);
			}
			if (ioctl(i, TCGETS, &cb) < 0) {
				perror("stty: TCGETS");
				exit(2);
			}
			for(i=0; speeds[i].string; i++)
				if ((cb.c_cflag&CBAUD) == speeds[i].speed) {
					(void) printf("%s\n", speeds[i].string);
					exit(0);
				}
			(void) printf("unknown\n");
			exit(1);
		}
		else if (strcmp(argv[1], "-g") == 0) {
			if ((i = open("/dev/tty", 0)) < 0) {
				perror("stty: Cannot open /dev/tty");
				exit(2);
			}
			if (ioctl(i, TCGETS, &cb) < 0) {
				perror("stty: TCGETS");
				exit(2);
			}
			prencode();
			exit(0);
		}
	}
	if(ioctl(ioctl_desc, TCGETS, &cb) == -1) {
		perror("stty: TCGETS");
		exit(2);
	}
	if(ioctl(ioctl_desc, TIOCGWINSZ, &size) == -1) {
		perror("stty: TIOCGWINSZ");
		exit(2);
	}

	if (argc == 1) {
		prmodes(0);
		exit(0);
	}
	if (argc == 2 && strcmp(argv[1], "all") == 0) {
		prmodes(1);
		exit(0);
	}
	if (argc == 2 && strcmp(argv[1], "everything") == 0) {
		pramodes();
		exit(0);
	}
	if ((argc == 2) && (argv[1][0] == '-') && (argv[1][2] == '\0'))
	switch(argv[1][1]) {
		case 'a':
			pramodes();
			exit(0);
		default:
			(void) fprintf(stderr, "%s", USAGE);
			exit(2);
	}
	while(--argc > 0) {		/* set terminal modes for supplied options */

		arg = *++argv;
		match = 0;
		for (sp = special; sp->name; sp++) {
			if (eq(sp->name)) {
				if (--argc == 0) {
					(void) fprintf(stderr, "stty: No argument for \"%s\"\n", sp->name);
					exit(1);
				}
				*sp->cp = gct(*++argv);
				goto cont;
			}
		}
		if (eq("brk")) {	/* synonym for "eol" */
			if (--argc == 0) {
				(void) fprintf(stderr, "stty: No argument for \"brk\"\n");
				exit(1);
			}
			cb.c_cc[VEOL] = gct(*++argv);
			goto cont;
		}
		if (eq("ek")) {
			cb.c_cc[VERASE] = CERASE;
			cb.c_cc[VKILL] = CKILL;
		}
		else if (eq("min")) {
			if (--argc == 0) {
				(void) fprintf(stderr, "stty: No argument for \"min\"\n");
				exit(1);
			}
			cb.c_cc[VMIN] = atoi(*++argv);
		}
		else if (eq("time")) {
			if (--argc == 0) {
				(void) fprintf(stderr, "stty: No argument for \"time\"\n");
				exit(1);
			}
			cb.c_cc[VTIME] = atoi(*++argv);
		}
		else if (eq("new")) {
			;		/* do nothing, for now */
		}
		else if (eq("crt") || eq("newcrt")) {
			cb.c_lflag &= ~ECHOPRT;
			cb.c_lflag |= ECHOE|ECHOCTL;
			if ((cb.c_cflag&CBAUD) >= B1200)
				cb.c_lflag |= ECHOKE;
		}
		else if (eq("old")) {
			;		/* do nothing, for now */
		}
		else if (eq("dec")) {
			cb.c_cc[VERASE] = 0177;
			cb.c_cc[VKILL] = _CTRL(u);
			cb.c_cc[VINTR] = _CTRL(c);
			cb.c_lflag &= ~ECHOPRT;
			cb.c_lflag |= ECHOE|ECHOCTL;
			if ((cb.c_cflag&CBAUD) >= B1200)
				cb.c_lflag |= ECHOKE;
		}
		else if (eq("gspeed")) {
			cb.c_cflag &= ~(CIBAUD|CBAUD);
			cb.c_cflag |= (B300<<IBSHIFT)|B9600;
		}
		else if (eq("rows")) {
			if (--argc == 0) {
				(void) fprintf(stderr, "stty: No argument for \"rows\"\n");
				exit(1);
			}
			size.ws_row = atoi(*++argv);
		}
		else if (eq("cols") || eq("columns")) {
			if (--argc == 0) {
				(void) fprintf(stderr, "stty: No argument for \"cols\"\n");
				exit(1);
			}
			size.ws_col = atoi(*++argv);
		}
		else if (eq("line")) {
			if (--argc == 0) {
				(void) fprintf(stderr, "stty: No argument for \"line\"\n");
				exit(1);
			}
			cb.c_line = atoi(*++argv);
		}
		else if (eq("raw") || eq("cbreak")) {
			cb.c_cc[VMIN] = 1;
			cb.c_cc[VTIME] = 0;
		}
		else if (eq("-raw") || eq("-cbreak") || eq("cooked")) {
			cb.c_cc[VEOF] = CEOF;
			cb.c_cc[VEOL] = CNUL;
		}
		else if(eq("sane")) {
			for (sp = special; sp->name; sp++)
				*sp->cp = sp->def;
		}
		for(i=0; speeds[i].string; i++)
			if(eq(speeds[i].string)) {
				cb.c_cflag &= ~(CIBAUD|CBAUD);
				cb.c_cflag |= speeds[i].speed&CBAUD;
			}
		for(i=0; imodes[i].string; i++)
			if(eq(imodes[i].string)) {
				cb.c_iflag &= ~imodes[i].reset;
				cb.c_iflag |= imodes[i].set;
			}
		for(i=0; omodes[i].string; i++)
			if(eq(omodes[i].string)) {
				cb.c_oflag &= ~omodes[i].reset;
				cb.c_oflag |= omodes[i].set;
			}
		for(i=0; cmodes[i].string; i++)
			if(eq(cmodes[i].string)) {
				cb.c_cflag &= ~cmodes[i].reset;
				cb.c_cflag |= cmodes[i].set;
			}
		for(i=0; lmodes[i].string; i++)
			if(eq(lmodes[i].string)) {
				cb.c_lflag &= ~lmodes[i].reset;
				cb.c_lflag |= lmodes[i].set;
			}
		if(!match)
			if(!encode()) {
				(void) fprintf(stderr, "unknown mode: %s\n", arg);
				exit(2);
			}
	cont:
		;	
	}
	if(ioctl(ioctl_desc, TCSETSW, &cb) == -1) {
		perror("stty: TCSETSW");
		exit(2);
	}
	if(ioctl(ioctl_desc, TIOCSWINSZ, &size) == -1) {
		perror("stty: TIOCSWINSZ");
		exit(2);
	}
	exit(0);	/*NOTREACHED*/
}

eq(string)
char *string;
{
	register i;

	if(!arg)
		return(0);
	i = 0;
loop:
	if(arg[i] != string[i])
		return(0);
	if(arg[i++] != '\0')
		goto loop;
	match++;
	return(1);
}

prmodes(moremodes)			/* print modes, no options, argc is 1 */
int moremodes;				/* or "stty all" if "moremodes" set */
{
	register struct special *sp;
	register m;
	int first = 1;

	m = cb.c_cflag;
	if ((m&CIBAUD) != 0 && ((m&CIBAUD)>>IBSHIFT) != (m&CBAUD)) {
		prspeed("input speed ", (m&CIBAUD)>>IBSHIFT);
		prspeed("output speed ", m&CBAUD);
	} else
		prspeed("speed ", m&CBAUD);
	if (moremodes)
		(void) fprintf(output, ", %d rows, %d columns",
		    size.ws_row, size.ws_col);
	(void) fprintf(output, "; ");
	if (m&PARENB) {
		if (m&PARODD)
			(void) fprintf(output, "oddp ");
		else
			(void) fprintf(output, "evenp ");
	}
	if(((m&PARENB) && !(m&CS7)) || (!(m&PARENB) && !(m&CS8)))
		(void) fprintf(output, "cs%c ",'5'+(m&CSIZE)/CS6);
	if (m&CSTOPB)
		(void) fprintf(output, "cstopb ");
	if (m&HUPCL)
		(void) fprintf(output, "hupcl ");
	if (!(m&CREAD))
		(void) fprintf(output, "cread ");
	if (m&CLOCAL)
		(void) fprintf(output, "clocal ");
#if 0		/* this bit isn't supported */
	if (m&LOBLK)
		(void) fprintf(output, "loblk");
#endif
	(void) fprintf(output, "\n");
	if(cb.c_line != 0) {
		(void) fprintf(output, "line = %d", cb.c_line);
		first = 0;
		pitt++;
	}
	if(!moremodes) {
		for (sp = special; sp->name; sp++) {
			if ((*sp->cp&0377) != (sp->def&0377)) {
				pit(*sp->cp, sp->name, first ? "" : "; ");
				first = 0;
			}
		}
	}
	if(pitt) (void) fprintf(output, "\n");
	if ((cb.c_lflag&ICANON)==0)
		(void) fprintf(output, "min %d, time %d\n", cb.c_cc[VMIN],
		    cb.c_cc[VTIME]);
	m = cb.c_iflag;
	if (m&IGNBRK)
		(void) fprintf(output, "ignbrk ");
	else if (!(m&BRKINT))
		(void) fprintf(output, "-brkint ");
	if (!(m&INPCK))
		(void) fprintf(output, "-inpck ");
	else if (!(m&IGNPAR))
		(void) fprintf(output, "-ignpar ");
	if (m&PARMRK)
		(void) fprintf(output, "parmrk ");
	if (!(m&ISTRIP))
		(void) fprintf(output, "-istrip ");
	if (m&INLCR)
		(void) fprintf(output, "inlcr ");
	if (m&IGNCR)
		(void) fprintf(output, "igncr ");
	if (!(m&ICRNL))
		(void) fprintf(output, "-icrnl ");
	if (m&IUCLC)
		(void) fprintf(output, "iuclc ");
	if (!(m&IXON))
		(void) fprintf(output, "-ixon ");
	else if (m&IXANY)
		(void) fprintf(output, "ixany ");
	if (m&IXOFF)
		(void) fprintf(output, "ixoff ");
	if (m&IMAXBEL)
		(void) fprintf(output, "imaxbel ");
	m = cb.c_oflag;
	if (!(m&OPOST))
		(void) fprintf(output, "-opost ");
	else {
		if (m&OLCUC)
			(void) fprintf(output, "olcuc ");
		if (!(m&ONLCR))
			(void) fprintf(output, "-onlcr ");
		if (m&OCRNL)
			(void) fprintf(output, "ocrnl ");
		if (m&ONOCR)
			(void) fprintf(output, "onocr ");
		if (m&ONLRET)
			(void) fprintf(output, "onlret ");
#if 0		/* these bits aren't supported yet */
		if (m&PAGEOUT)
			(void) fprintf(output, "pageout ");
		if (m&WRAP)
			(void) fprintf(output, "wrap ");
#endif
		if (m&OFILL)
			if (m&OFDEL)
				(void) fprintf(output, "del-fill ");
			else
				(void) fprintf(output, "nul-fill ");
		delay((m&CRDLY)/CR1, "cr");
		delay((m&NLDLY)/NL1, "nl");
		if ((m&TABDLY) == XTABS)
			(void) fprintf(output, "-tabs ");
		else
			delay((m&TABDLY)/TAB1, "tab");
		delay((m&BSDLY)/BS1, "bs");
		delay((m&VTDLY)/VT1, "vt");
		delay((m&FFDLY)/FF1, "ff");
	}
	(void) fprintf(output, "\n");
	m = cb.c_lflag;
	if (!(m&ISIG))
		(void) fprintf(output, "-isig ");
	if (m&IEXTEN)
		(void) fprintf(output, "iexten ");
	else
		(void) fprintf(output, "-iexten ");
	if (!(m&ICANON))
		(void) fprintf(output, "-icanon ");
	if (m&XCASE)
		(void) fprintf(output, "xcase ");
	if (!(m&ECHO))
		(void) fprintf(output, "-echo ");
	if (m&ECHOE) {
		if (m&ECHOKE)
			(void) fprintf(output, "crt ");
		else
			(void) fprintf(output, "echoe -echoke ");
	} else {
		if (!(m&ECHOPRT))
			(void) fprintf(output, "-echoprt ");
	}
	if (!(m&ECHOCTL))
		(void) fprintf(output, "-echoctl ");
	if (!(m&ECHOK))
		(void) fprintf(output, "-echok ");
	if (m&ECHONL)
		(void) fprintf(output, "echonl ");
	if (m&NOFLSH)
		(void) fprintf(output, "noflsh ");
	if (m&TOSTOP)
		(void) fprintf(output, "tostop ");
#if 0		/* this bit isn't supported yet */
	if (m&DEFECHO)
		(void) fprintf(output, "defecho ");
#endif
	(void) fprintf(output, "\n");
	if (moremodes)
		prachars();
}

pramodes()				/* print all modes, -a option */
{
	register m;

	m = cb.c_cflag;
	if ((m&CIBAUD) != 0 && ((m&CIBAUD)>>IBSHIFT) != (m&CBAUD)) {
		prspeed("input speed ", (m&CIBAUD)>>IBSHIFT);
		prspeed("output speed ", m&CBAUD);
	} else
		prspeed("speed ", m&CBAUD);
	(void) fprintf(output, ", %d rows, %d columns", size.ws_row,
	    size.ws_col);
	if (cb.c_line != 0)
		(void) fprintf(output, "; line = %d", cb.c_line);
	(void) fprintf(output, "\n");
	m = cb.c_cflag;
	(void) fprintf(output, "-parenb "+((m&PARENB)!=0));
	(void) fprintf(output, "-parodd "+((m&PARODD)!=0));
	(void) fprintf(output, "cs%c ",'5'+(m&CSIZE)/CS6);
	(void) fprintf(output, "-cstopb "+((m&CSTOPB)!=0));
	(void) fprintf(output, "-hupcl "+((m&HUPCL)!=0));
	(void) fprintf(output, "-cread "+((m&CREAD)!=0));
	(void) fprintf(output, "-clocal "+((m&CLOCAL)!=0));

#if 0		/* this bit isn't supported */
	(void) fprintf(output, "-loblk "+((m&LOBLK)!=0));
#endif
	(void) fprintf(output, "-crtscts "+((m&CRTSCTS)!=0));

	(void) fprintf(output, "\n");
	m = cb.c_iflag;
	(void) fprintf(output, "-ignbrk "+((m&IGNBRK)!=0));
	(void) fprintf(output, "-brkint "+((m&BRKINT)!=0));
	(void) fprintf(output, "-ignpar "+((m&IGNPAR)!=0));
	(void) fprintf(output, "-parmrk "+((m&PARMRK)!=0));
	(void) fprintf(output, "-inpck "+((m&INPCK)!=0));
	(void) fprintf(output, "-istrip "+((m&ISTRIP)!=0));
	(void) fprintf(output, "-inlcr "+((m&INLCR)!=0));
	(void) fprintf(output, "-igncr "+((m&IGNCR)!=0));
	(void) fprintf(output, "-icrnl "+((m&ICRNL)!=0));
	(void) fprintf(output, "-iuclc "+((m&IUCLC)!=0));
	(void) fprintf(output, "\n");
	(void) fprintf(output, "-ixon "+((m&IXON)!=0));
	(void) fprintf(output, "-ixany "+((m&IXANY)!=0));
	(void) fprintf(output, "-ixoff "+((m&IXOFF)!=0));
	(void) fprintf(output, "-imaxbel "+((m&IMAXBEL)!=0));
	(void) fprintf(output, "\n");
	m = cb.c_lflag;
	(void) fprintf(output, "-isig "+((m&ISIG)!=0));
	(void) fprintf(output, "-iexten "+((m&IEXTEN)!=0));
	(void) fprintf(output, "-icanon "+((m&ICANON)!=0));
	(void) fprintf(output, "-xcase "+((m&XCASE)!=0));
	(void) fprintf(output, "-echo "+((m&ECHO)!=0));
	(void) fprintf(output, "-echoe "+((m&ECHOE)!=0));
	(void) fprintf(output, "-echok "+((m&ECHOK)!=0));
	(void) fprintf(output, "-echonl "+((m&ECHONL)!=0));
	(void) fprintf(output, "-noflsh "+((m&NOFLSH)!=0));
	(void) fprintf(output, "-tostop "+((m&TOSTOP)!=0));
	(void) fprintf(output, "\n");
	(void) fprintf(output, "-echoctl "+((m&ECHOCTL)!=0));
	(void) fprintf(output, "-echoprt "+((m&ECHOPRT)!=0));
	(void) fprintf(output, "-echoke "+((m&ECHOKE)!=0));
#if 0		/* this bit isn't supported yet */
	(void) fprintf(output, "-defecho "+((m&DEFECHO)!=0));
#endif
	(void) fprintf(output, "\n");
	m = cb.c_oflag;
	(void) fprintf(output, "-opost "+((m&OPOST)!=0));
	(void) fprintf(output, "-olcuc "+((m&OLCUC)!=0));
	(void) fprintf(output, "-onlcr "+((m&ONLCR)!=0));
	(void) fprintf(output, "-ocrnl "+((m&OCRNL)!=0));
	(void) fprintf(output, "-onocr "+((m&ONOCR)!=0));
	(void) fprintf(output, "-onlret "+((m&ONLRET)!=0));
#if 0		/* these bits aren't supported yet */
	(void) fprintf(output, "-pageout "+((m&PAGEOUT)!=0));
	(void) fprintf(output, "-wrap "+((m&WRAP)!=0));
#endif
	(void) fprintf(output, "-ofill "+((m&OFILL)!=0));
	(void) fprintf(output, "-ofdel "+((m&OFDEL)!=0));
	delay((m&CRDLY)/CR1, "cr");
	delay((m&NLDLY)/NL1, "nl");
	if ((m&TABDLY) == XTABS)
		(void) fprintf(output, "-tabs ");
	else
		delay((m&TABDLY)/TAB1, "tab");
	delay((m&BSDLY)/BS1, "bs");
	delay((m&VTDLY)/VT1, "vt");
	delay((m&FFDLY)/FF1, "ff");
	(void) fprintf(output, "\n");
	prachars();
}

prachars()
{
	if ((cb.c_lflag&ICANON)==0)
		(void) fprintf(output, "min %d, time %d\n", cb.c_cc[VMIN],
		    cb.c_cc[VTIME]);
	(void) fprintf(output, "\
erase  kill   werase rprnt  flush  lnext  susp   intr   quit   stop   eof\
\n");
	pcol(cb.c_cc[VERASE], 0);
	pcol(cb.c_cc[VKILL], 0);
	pcol(cb.c_cc[VWERASE], 0);
	pcol(cb.c_cc[VREPRINT], 0);
	pcol(cb.c_cc[VDISCARD], 0);
	pcol(cb.c_cc[VLNEXT], 0);
	pcol(cb.c_cc[VSUSP], cb.c_cc[VDSUSP]);
	pcol(cb.c_cc[VINTR], 0);
	pcol(cb.c_cc[VQUIT], 0);
	pcol(cb.c_cc[VSTOP], cb.c_cc[VSTART]);
	if (cb.c_lflag&ICANON)
		pcol(cb.c_cc[VEOF], cb.c_cc[VEOL]);
	(void) fprintf(output, "\n");
#if 0		/* the SWITCH character isn't supported */
	if (cb.c_cc[VEOL2] != 0 || cb.c_cc[VSWTCH] != 0) {
		(void) fprintf(output, "\
eol2  swtch\
\n");
		pcol(cb.c_cc[VEOL2], 0);
		pcol(cb.c_cc[VSWTCH], 0);
		(void) fprintf(output, "\n");
	}
#else
	if (cb.c_cc[VEOL2] != 0) {
		(void) fprintf(output, "\
eol2\
\n");
		pcol(cb.c_cc[VEOL2], 0);
		(void) fprintf(output, "\n");
	}
#endif
}

				/* get pseudo control characters from terminal */
gct(cp)				/* and convert to internal representation      */
register char *cp;
{
	register c;

	c = *cp++;
	if (c == '^') {
		c = *cp;
		if (c == '?')
			c = 0177;		/* map '^?' to DEL */
		else if (c == '-')
			c = 0;			/* map '^-' to 0, i.e. undefined */
		else
			c &= 037;
	}
#ifndef S5EMUL
	else if (c == 'u')
		c = 0;	/* "undef" maps to 0 */
#endif
	return(c);
}

pcol(ch1, ch2)
	int ch1, ch2;
{
	int nout = 0;

	ch1 &= 0377;
	ch2 &= 0377;
	if (ch1 == ch2)
		ch2 = 0;
	for (; ch1 != 0 || ch2 != 0; ch1 = ch2, ch2 = 0) {
		if (ch1 == 0)
			continue;
		if (ch1 & 0200 && !isprint(ch1)) {
			(void) fprintf(output, "M-");
			nout += 2;
			ch1 &= ~ 0200;
		}
		if (ch1 == 0177) {
			(void) fprintf(output, "^");
			nout++;
			ch1 = '?';
		} else if (ch1 < ' ') {
			(void) fprintf(output, "^");
			nout++;
			ch1 += '@';
		}
		(void) fprintf(output, "%c", ch1);
		nout++;
		if (ch2 != 0) {
			(void) fprintf(output, "/");
			nout++;
		}
	}
	while (nout < 7) {
		(void) fprintf(output, " ");
		nout++;
	}
}

pit(what, itsname, sep)		/*print function for prmodes() */
	unsigned char what;
	char *itsname, *sep;
{

	pitt++;
	(void) fprintf(output, "%s%s", sep, itsname);
	if (what == 0) {
		(void) fprintf(output, " <undef>");
		return;
	}
	(void) fprintf(output, " = ");
	if ((what & 0200) && !isprint(what)) {
		(void) fprintf(output, "M-");
		what &= ~ 0200;
	}
	if (what == 0177) {
		(void) fprintf(output, "^?");
		return;
	} else if (what < ' ') {
		(void) fprintf(output, "^");
		what += '@';
	}
	(void) fprintf(output, "%c", what);
}

delay(m, s)
char *s;
{
	if(m)
		(void) fprintf(output, "%s%d ", s, m);
}

long	speed[] = {
	0,50,75,110,134,150,200,300,600,1200,1800,2400,4800,9600,19200,38400
};

prspeed(c, s)
char *c;
int s;
{

	(void) fprintf(output, "%s%d baud", c, speed[s]);
}

					/* print current settings for use with  */
prencode()				/* another stty cmd, used for -g option */
{
	/* Since the -g option is mostly used for redirecting to a file */
	/* We must print to stdout here, not stderr */

	(void) fprintf(stdout, "%lx:%lx:%lx:%lx:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n",
	cb.c_iflag,cb.c_oflag,cb.c_cflag,cb.c_lflag,
	cb.c_cc[VINTR],cb.c_cc[VQUIT],cb.c_cc[VERASE],cb.c_cc[VKILL],
	cb.c_cc[VEOF],cb.c_cc[VEOL],cb.c_cc[VEOL2],cb.c_cc[VSWTCH],
	cb.c_cc[VSTART],cb.c_cc[VSTOP],cb.c_cc[VSUSP],cb.c_cc[VDSUSP],
	cb.c_cc[VREPRINT],cb.c_cc[VDISCARD],cb.c_cc[VWERASE],cb.c_cc[VLNEXT],
	cb.c_cc[VSTATUS]);
}

encode()
{
	unsigned long iflag, oflag, cflag, lflag;
	unsigned short min, time;
	int grab[NCCS], i;
	i = sscanf(arg, "%lx:%lx:%lx:%lx:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n",
	&iflag,&oflag,&cflag,&lflag,
	&grab[VINTR],&grab[VQUIT],&grab[VERASE],&grab[VKILL],
	&grab[VEOF],&grab[VEOL],&grab[VEOL2],&grab[VSWTCH],
	&grab[VSTART],&grab[VSTOP],&grab[VSUSP],&grab[VDSUSP],
	&grab[VREPRINT],&grab[VDISCARD],&grab[VWERASE],&grab[VLNEXT],
	&grab[VSTATUS]);

	if(i != 21) return(0);

	cb.c_iflag = iflag;
	cb.c_oflag = oflag;
	cb.c_cflag = cflag;
	cb.c_lflag = lflag;

	for(i=0; i<NCCS; i++)
		cb.c_cc[i] = (char) grab[i];
	return(1);
}
