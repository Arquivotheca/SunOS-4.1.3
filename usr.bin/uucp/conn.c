/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)conn.c 1.1 92/07/30"	/* from SVR3.2 uucp:conn.c 2.17 */

#include "uucp.h"

static char _ProtoStr[40] = "";	/* protocol string from Systems file entry */
extern jmp_buf Sjbuf;

int	Modemctrl, ioctlok;
#ifdef ATTSVTTY
struct termio	ttybuf;
#endif

/* Parity control during login procedure */
#define	P_ZERO	0
#define	P_ONE	1
#define	P_EVEN	2
#define	P_ODD	3

char	par_tab[128];

int alarmtr();
static void getProto();
/* static getto(), finds();   PUT this back after altconn is included */
extern getto();
static finds();

static notin(), ifdate(), classmatch();

extern struct caller caller[];

/* Needed for cu for force which line will be used */
#ifdef STANDALONE
extern char *Myline;
#endif /* STANDALONE */

/*
 * conn - place a telephone call to system and login, etc.
 *
 * return codes:
 *	FAIL - connection failed
 *	>0  - file no.  -  connect ok
 * When a failure occurs, Uerror is set.
 */

conn(system)
char *system;
{
	int nf, fn;
	char *flds[F_MAX+1];
	extern void sysreset();

	CDEBUG(4, "conn(%s)\n", system);
	Uerror = 0;
	while ((nf = finds(system, flds, F_MAX)) > 0) {
		fn = getto(flds);
		CDEBUG(4, "getto ret %d\n", fn);
		if (fn < 0)
		    continue;

#ifdef TIOCSPGRP
		{
		int pgrp = getpgrp(0);
		ioctl(fn, TIOCSPGRP, &pgrp);
		}
#endif
#ifdef STANDALONE
		sysreset();
		return(fn);
#else /* !STANDALONE */

		if (chat(nf - F_LOGIN, flds + F_LOGIN, fn,"","") == SUCCESS) {
			sysreset();
			return(fn); /* successful return */
		}

		/* login failed */
		DEBUG(6, "close caller (%d)\n", fn);
		close(fn);
		if (Dc[0] != NULLCHAR) {
			DEBUG(6, "delock(%s)\n", Dc);
			delock(Dc);
		}
#endif /* STANDALONE */
	}

	/* finds or getto failed */
	sysreset();
	CDEBUG(1, "Call Failed: %s\n", UERRORTEXT);
	return(FAIL);
}

/*
 * getto - connect to remote machine
 *
 * return codes:
 *	>0  -  file number - ok
 *	FAIL  -  failed
 */

getto(flds)
char *flds[];
{
	char *dev[D_MAX+2], devbuf[BUFSIZ];
	register int status;
	register int dcf = -1;
	int reread = 0;
	int tries = 0;	/* count of call attempts - for limit purposes */
	extern void devreset();

	CDEBUG(1, "Device Type %s wanted\n", flds[F_TYPE]);
	Uerror = 0;
	while (tries < TRYCALLS) {
		if ((status=rddev(flds[F_TYPE], dev, devbuf, D_MAX)) == FAIL) {
			if (tries == 0 || ++reread >= TRYCALLS)
				break;
			devreset();	/* reset devices file(s) */
			continue;
		}
		/* check class, check (and possibly set) speed */
		if (classmatch(flds, dev) != SUCCESS)
			continue;
		if ((dcf = processdev(flds, dev)) >= 0)
			break;

		switch(Uerror) {
		case SS_CANT_ACCESS_DEVICE:
		case SS_DEVICE_FAILED:
		case SS_LOCKED_DEVICE:
		case SS_CHAT_FAILED:
			break;
		default:
			tries++;
			break;
		}
	}
	devreset();	/* reset devices file(s) */
	if (status == FAIL && !Uerror) {
		CDEBUG(1, "Requested Device Type Not Found\n", 0);
		Uerror = SS_NO_DEVICE;
	}
	return(dcf);
}

/*
 * classmatch - process 'Any' in Devices and Systems and
 * 	determine the correct speed, or match for ==
 */

static int
classmatch(flds, dev)
char *flds[], *dev[];
{
	/* check class, check (and possibly set) speed */
	if (EQUALS(flds[F_CLASS], "Any")
	   && EQUALS(dev[D_CLASS], "Any")) {
		dev[D_CLASS] = DEFAULT_BAUDRATE;
		return(SUCCESS);
	} else if (EQUALS(dev[D_CLASS], "Any")) {
		dev[D_CLASS] = flds[F_CLASS];
		return(SUCCESS);
	} else if (EQUALS(flds[F_CLASS], "Any") ||
	EQUALS(flds[F_CLASS], dev[D_CLASS]))
		return(SUCCESS);
	else
		return(FAIL);
}


/***
 *	rddev - find and unpack a line from device file for this caller type 
 *	lines starting with whitespace of '#' are comments
 *
 *	return codes:
 *		>0  -  number of arguments in vector - succeeded
 *		FAIL - EOF
 ***/

rddev(type, dev, buf, devcount)
char *type;
char *dev[];
char *buf;
{
	char *commap, d_type[BUFSIZ];
	int na;

	while (getdevline(buf, BUFSIZ)) {
		if (buf[0] == ' ' || buf[0] == '\t'
		    ||  buf[0] == '\n' || buf[0] == '\0' || buf[0] == '#')
			continue;
		na = getargs(buf, dev, devcount);
		ASSERT(na >= D_CALLER, "BAD LINE", buf, na);

		if ( strncmp(dev[D_LINE],"/dev/",5) == 0 ) {
			/* since cu (altconn()) strips off leading */
			/* "/dev/",  do the same here.  */
			strcpy(dev[D_LINE], &(dev[D_LINE][5]) );
		}

		/* may have ",M" subfield in D_LINE */
		if ( (commap = strchr(dev[D_LINE], ',')) != (char *)NULL ) {
			if ( strcmp( commap, ",M") == SAME )
				Modemctrl = TRUE;
			*commap = '\0';
		}

/* For cu -- to force the requested line to be used */
#ifdef STANDALONE
		if ((Myline != NULL) && (!EQUALS(Myline, dev[D_LINE])) )
		    continue;
#endif /* STANDALONE */

		bsfix(dev);	/* replace \X fields */

		/*
		 * D_TYPE field may have protocol subfield, which
		 * must be pulled off before comparing to desired type.
		 */
		(void)strcpy(d_type, dev[D_TYPE]);
		if ((commap = strchr(d_type, ',')) != (char *)NULL )
			*commap = '\0';
		if (EQUALS(d_type, type)) {
			getProto( dev[D_TYPE] );
			return(na);
		}
	}
	return(FAIL);
}


/*
 * finds	- set system attribute vector
 *
 * input:
 *	fsys - open Systems file descriptor
 *	sysnam - system name to find
 * output:
 *	flds - attibute vector from Systems file
 *	fldcount - number of fields in flds
 * return codes:
 *	>0  -  number of arguments in vector - succeeded
 *	FAIL - failed
 * Uerror set:
 *	0 - found a line in Systems file
 *	SS_BADSYSTEM - no line found in Systems file
 *	SS_TIME_WRONG - wrong time to call
 */

static
finds(sysnam, flds, fldcount)
char *sysnam, *flds[];
{
	static char info[BUFSIZ];
	int na;

	/* format of fields
	 *	0 name;
	 *	1 time
	 *	2 acu/hardwired
	 *	3 speed
	 *	etc
	 */
	if (sysnam == 0 || *sysnam == 0 ) {
		Uerror = SS_BADSYSTEM;
		return(FAIL);
	}

	while (getsysline(info, sizeof(info))) {
		if (*sysnam != *info || *info == '#')	/* speedup */
			continue;
		na = getargs(info, flds, fldcount);
		bsfix(flds);	/* replace \X fields */
		if ( !EQUALSN(sysnam, flds[F_NAME], SYSNSIZE))
			continue;
#ifdef STANDALONE
		*_ProtoStr = NULLCHAR;
		getProto(flds[F_TYPE]);
		Uerror = 0;
		return(na);	/* FOUND OK LINE */
#else /* !STANDALONE */
		if (ifdate(flds[F_TIME])) {
			/*  found a good entry  */
			*_ProtoStr = NULLCHAR;
			getProto(flds[F_TYPE]);
			Uerror = 0;
			return(na);	/* FOUND OK LINE */
		}
		CDEBUG(1, "Wrong Time To Call: %s\n", flds[F_TIME]);
		if (!Uerror)
			Uerror = SS_TIME_WRONG;
#endif /* STANDALONE */
	}
	if (!Uerror)
		Uerror = SS_BADSYSTEM;
	return(FAIL);
}

/*
 * getProto - get the protocol letters from the input string.
 * input:
 *	str - string from Systems file (flds[F_TYPE])--the ,
 *		delimits the protocol string
 *		e.g. ACU,g or DK,d
 * output:
 *	str - the , (if present) will be replaced with NULLCHAR
 *	global ProtoStr will be modified
 * return:  none
 */

static
void
getProto(str)
char *str;
{
	register char *p;
	if ( (p=strchr(str, ',')) != NULL) {
		*p++ = NULLCHAR;
		if( *_ProtoStr == NULLCHAR )
			(void) strcpy(_ProtoStr, p);
		else
			(void) strcat(_ProtoStr, p);
		DEBUG(7, "ProtoStr = %s\n", _ProtoStr);
	}
}

/*
 * check for a specified protocol selection string
 * return:
 *	protocol string pointer
 *	NULL if none specified for LOGNAME
 */
char *
protoString()
{
	return(_ProtoStr[0] == NULLCHAR ? NULL : _ProtoStr);
}

/*
 * chat -	do conversation
 * input:
 *	nf - number of fields in flds array
 *	flds - fields from Systems file
 *	fn - write file number
 *	phstr1 - phone number to replace \D
 *	phstr2 - phone number to replace \T
 *
 *	return codes:  0  |  FAIL
 */

chat(nf, flds, fn, phstr1, phstr2)
char *flds[], *phstr1, *phstr2;
int nf, fn;
{
	char *want, *altern;
	extern char *index();
	extern int	ioctlok;
	static int	did_ioctl;
	int k, ok;

#ifdef ATTSVTTY
	/* init ttybuf - used in sendthem() */
	if ( !did_ioctl ) {
		if ( (*Ioctl)(fn, TCGETA, &ttybuf) == 0 )
			ioctlok = 1;
		else
			DEBUG(7, "chat: TCGETA failed, errno %d\n", errno);
		did_ioctl = 1;
	}
#endif

	for (k = 0; k < nf; k += 2) {
		want = flds[k];
		ok = FAIL;
		while (ok != 0) {
			altern = index(want, '-');
			if (altern != NULL)
				*altern++ = NULLCHAR;
			ok = expect(want, fn);
			if (ok == 0)
				break;
			if (altern == NULL) {
				Uerror = SS_LOGIN_FAILED;
				logent(UERRORTEXT, "FAILED");
				return(FAIL);
			}
			want = index(altern, '-');
			if (want != NULL)
				*want++ = NULLCHAR;
			sendthem(altern, fn, phstr1, phstr2);
		}
		sleep(2);
		if (flds[k+1])
		    sendthem(flds[k+1], fn, phstr1, phstr2);
	}
	return(0);
}

#define MR 300

/***
 *	expect(str, fn)	look for expected string
 *	char *str;
 *
 *	return codes:
 *		0  -  found
 *		FAIL  -  lost line or too many characters read
 *		some character  -  timed out
 */

expect(str, fn)
char *str;
int fn;
{
	static char rdvec[MR];
	char *rp = rdvec;
	register int kr, c;
	char nextch;
	extern	errno;

	*rp = 0;

	CDEBUG(4, "expect: (", 0);
	for (c=0; kr=str[c]; c++)
		if (kr < 040) {
			CDEBUG(4, "^%c", kr | 0100);
		} else
			CDEBUG(4, "%c", kr);
	CDEBUG(4, ")\n", 0);

	if (EQUALS(str, "\"\"")) {
		CDEBUG(4, "got it\n", 0);
		return(0);
	}
	if (setjmp(Sjbuf)) {
		return(FAIL);
	}
	(void) signal(SIGALRM, alarmtr);
	alarm(MAXEXPECTTIME);
	while (notin(str, rdvec)) {
		errno = 0;
		kr = (*Read)(fn, &nextch, 1);
		if (kr <= 0) {
			alarm(0);
			CDEBUG(4, "lost line errno - %d\n", errno);
			logent("LOGIN", "LOST LINE");
			return(FAIL);
		}
		c = nextch & 0177;
		CDEBUG(4, "%s", c < 040 ? "^" : "");
		CDEBUG(4, "%c", c < 040 ? c | 0100 : c);
		if ((*rp = nextch & 0177) != NULLCHAR)
			rp++;
		if (rp >= rdvec + MR) {
			CDEBUG(4, "enough already\n", 0);
			alarm(0);
			return(FAIL);
		}
		*rp = NULLCHAR;
	}
	alarm(0);
	CDEBUG(4, "got it\n", 0);
	return(0);
}


/***
 *	alarmtr()  -  catch alarm routine for "expect".
 */

alarmtr()
{
	CDEBUG(6, "timed out\n", 0);
	longjmp(Sjbuf, 1);
}




/***
 *	sendthem(str, fn, phstr1, phstr2)	send line of chat sequence
 *	char *str, *phstr;
 *
 *	return codes:  none
 */

#define FLUSH() {\
	if ((bptr - buf) > 0)\
		if (wrstr(fn, buf, bptr - buf, echocheck) != SUCCESS)\
			goto err;\
	bptr = buf;\
}

sendthem(str, fn, phstr1, phstr2)
char *str, *phstr1, *phstr2;
int fn;
{
	int sendcr = 1, echocheck = 0;
	register char	*sptr, *bptr;
	char	buf[BUFSIZ];
	extern int		ioctlok;
	static int p_init = 0;

	if (!p_init) {
		p_init++;
		bld_partab(P_EVEN);
	}

	/* should be EQUALS, but previous versions had BREAK n for integer n */
	if (PREFIX("BREAK", str)) {
		/* send break */
		CDEBUG(5, "BREAK\n", 0);
		(*genbrk)(fn);
		return;
	}

	if (PREFIX("STTY=", str)) {
#ifdef ATTSVTTY
		if (!ioctlok) {
			CDEBUG(5, "STTY ignored\n", 0);
			return;
		}
#endif
		CDEBUG(5, "STTY %s\n", str+5);
		setmode(str+5, fn);
		return;
	}

	if (EQUALS(str, "EOT")) {
		CDEBUG(5, "EOT\n", 0);
		bptr = buf;
		for (sptr = EOTMSG; *sptr; sptr++)
			*bptr++ = par_tab[*sptr&0177];
		(void) (*Write)(fn, buf, bptr - buf);
		return;
	}

	/* Set parity as needed */
	if (EQUALS(str, "P_ZERO")) {
		bld_partab(P_ZERO);
		return;
	}
	if (EQUALS(str, "P_ONE")) {
		bld_partab(P_ONE);
		return;
	}
	if (EQUALS(str, "P_EVEN")) {
		bld_partab(P_EVEN);
		return;
	}
	if (EQUALS(str, "P_ODD")) {
		bld_partab(P_ODD);
		return;
	}

	if (EQUALS(str, "\"\"")) {
		CDEBUG(5, "\"\"\n", 0);
		str += 2;
	}

	bptr = buf;
	CDEBUG(5, "sendthem (", 0);
	for (sptr = str; *sptr; sptr++) {
		if (*sptr == '\\') {
			switch(*++sptr) {

			/* adjust switches */
			case 'c':	/* no CR after string */
				if (sptr[1] == NULLCHAR) {
					CDEBUG(5, "<NO CR>", 0);
					sendcr = 0;
				} else
					CDEBUG(5, "<NO CR IGNORED>\n", 0);
				continue;

			/* stash in buf and continue */
			case 'D':	/* raw phnum */
				strcpy(bptr, phstr1);
				bptr += strlen(bptr);
				continue;
			case 'T':	/* translated phnum */
				strcpy(bptr, phstr2);
				bptr += strlen(bptr);
				continue;
			case 'N':	/* null */
				*bptr++ = 0;
				continue;
			case 's':	/* space */
				*bptr++ = ' ';
				continue;
			case '\\':	/* backslash escapes itself */
				*bptr++ = *sptr;
				continue;
			default:	/* send the backslash */
				*bptr++ = '\\';
				*bptr++ = *sptr;	
				continue;

			/* flush buf, perform action, and continue */
			case 'E':	/* echo check on */
				FLUSH();
				CDEBUG(5, "ECHO CHECK ON\n", 0);
				echocheck = 1;
				continue;
			case 'e':	/* echo check off */
				FLUSH();
				CDEBUG(5, "ECHO CHECK OFF\n", 0);
				echocheck = 0;
				continue;
			case 'd':	/* sleep briefly */
				FLUSH();
				CDEBUG(5, "DELAY\n", 0);
				sleep(2);
				continue;
			case 'p':	/* pause momentarily */
				FLUSH();
				CDEBUG(5, "PAUSE\n", 0);
				nap(HZ/4);	/* approximately 1/4 second */
				continue;
			case 'K':	/* inline break */
				FLUSH();
				CDEBUG(5, "BREAK\n", 0);
				(*genbrk)(fn);
				continue;
#ifdef ATTSVTTY
			case 'M':	/* modem control - set CLOCAL */
				FLUSH();
				if ( ! ioctlok ) {
					CDEBUG(5, ")\nset CLOCAL ignored\n", 0);
					continue;
				}
				CDEBUG(5, ")\nCLOCAL set\n", 0);
				ttybuf.c_cflag |= CLOCAL;
				if ( (*Ioctl)(fn, TCSETAW, &ttybuf) < 0 )
					CDEBUG(5, "TCSETAW failed, errno %d\n", errno);
				continue;
			case 'm':	/* no modem control - clear CLOCAL */
				FLUSH();
				if ( ! ioctlok ) {
					CDEBUG(5, ")\nclear CLOCAL ignored\n", 0);
					continue;
				}
				CDEBUG(5, ")\nCLOCAL clear\n", 0);
				ttybuf.c_cflag &= ~CLOCAL;
				if ( (*Ioctl)(fn, TCSETAW, &ttybuf) < 0 )
					CDEBUG(5, "TCSETAW failed, errno %d\n", errno);
				continue;
#endif
			}
		} else
			*bptr++ = *sptr;
	}
	if (sendcr)
		*bptr++ = '\r';
	if ( (bptr - buf) > 0 )
		(void) wrstr(fn, buf, bptr - buf, echocheck);

err:
	CDEBUG(5, ")\n", 0);
	return;
}

/*
 * generate parity table for use by sendthem.
 */
bld_partab(type)
int type;
{
	register int i, j, n;

	for (i = 0; i < 128; i++) {
		n = 0;
		for (j = i&0177; j; j = (j-1)&j)
			n++;
		par_tab[i] = i;
		if (type == P_ONE
		 || (type == P_EVEN && (n&01) != 0)
		 || (type == P_ODD && (n&01) == 0))
			par_tab[i] |= 0200;
	}
}

#undef FLUSH

wrstr(fn, buf, len, echocheck)
char *buf;
{
	int 	i;
	char dbuf[BUFSIZ], *dbptr = dbuf;

	if (echocheck)
		return(wrchr(fn, buf, len));

	if (Debug >= 5) {
		if (sysaccess(ACCESS_SYSTEMS) == 0) { /* Systems file access ok */
			for (i = 0; i < len; i++) {
				*dbptr = buf[i];
				if (*dbptr < 040) {
					*dbptr++ = '^';
					*dbptr = buf[i] | 0100;
				}
				dbptr++;
			}
			*dbptr = 0;
		} else
			strcpy(dbuf, "????????");
		CDEBUG(5, dbuf, 0);
	}
	dbptr = dbuf;
	for (i = 0; i < len; i++)
		*dbptr++ = par_tab[buf[i]&0177];
	if ((*Write)(fn, dbuf, len) != len)
		return(FAIL);
	return(SUCCESS);
}

wrchr(fn, buf, len)
register int fn;
register char *buf;
{
	int 	i, saccess;
	char	cin, cout;

	saccess = (sysaccess(ACCESS_SYSTEMS) == 0); /* protect Systems file */
	if (setjmp(Sjbuf))
		return(FAIL);
	(void) signal(SIGALRM, alarmtr);

	for (i = 0; i < len; i++) {
		cout = buf[i]&0177;
		if (saccess) {
			CDEBUG(5, "%s", cout < 040 ? "^" : "");
			CDEBUG(5, "%c", cout < 040 ? cout | 0100 : cout);
		} else
			CDEBUG(5, "?", 0);
		if (((*Write)(fn, &par_tab[cout], 1)) != 1)
			return(FAIL);
		do {
			(void) alarm(MAXEXPECTTIME);
			if ((*Read)(fn, &cin, 1) != 1)
				return(FAIL);
			(void) alarm(0);
			cin &= 0177;
			if (saccess) {
				CDEBUG(5, "%s", cin < 040 ? "^" : "");
				CDEBUG(5, "%c", cin < 040 ? cin | 0100 : cin);
			} else
				CDEBUG(5, "?", 0);
		} while (cout != cin);
	}
	return(SUCCESS);
}


/***
 *	notin(sh, lg)	check for occurrence of substring "sh"
 *	char *sh, *lg;
 *
 *	return codes:
 *		0  -  found the string
 *		1  -  not in the string
 */

static
notin(sh, lg)
char *sh, *lg;
{
	while (*lg != NULLCHAR) {
		if (PREFIX(sh, lg))
			return(0);
		else
			lg++;
	}
	return(1);
}


/*******
 *	ifdate(s)
 *	char *s;
 *
 *	ifdate  -  this routine will check a string (s)
 *	like "MoTu0800-1730" to see if the present
 *	time is within the given limits.
 *
 *	SIDE EFFECT - Retrytime is set to number following ";"
 *	SIDE EFFECT - MaxGrade is set to character following "/"
 *
 *	if a grade is specified, iswrk() is consulted, so that we don't
 *	place calls when there's only low priority work.  this will appear
 *	as a "wrong time to call" in the status file.  sorry.
 *
 *	String alternatives:
 *		Wk - Mo thru Fr
 *		zero or one time means all day
 *		Any - any day
 *
 *	return codes:
 *		0  -  not within limits, or grade too low
 *		1  -  within limits
 */

static
ifdate(s)
char *s;
{
	char	*r, *m, grade;
	struct tm	*tp;
	time_t	clock;
	int	t__now;

	time(&clock);
	tp = localtime(&clock);
	t__now = tp->tm_hour * 100 + tp->tm_min;	/* "navy" time */

	/*
	 *	pick up retry time for failures and max grade
	 *	global variables Retrytime and MaxGrade are set here
	 */
	r = strrchr(s, ';');
	m = strrchr(s, '/');

	/* set retry time */
	if (r != NULL) {
	    if (isdigit(r[1])) {
		if (sscanf(r+1, "%d", &Retrytime) < 1)
			Retrytime = 5;	/* 5 minutes is error default */
		DEBUG(5, "Retry time set to %d minutes\n", Retrytime);
		Retrytime *= 60;	/* convert to seconds */
		*r = NULLCHAR;		/* blow away retry time field */
	    }
	} else
	    Retrytime = 0;	/* use exponential backoff */

	/* set grade */
	MaxGrade = NULLCHAR;			/* default */
	if (m != NULL) {
	    if (isalnum(m[1]))
		MaxGrade = m[1];	/* you asked for it! */
	    *m = NULLCHAR;		/* blow away max grade field */
	    DEBUG(5, "Max Grade set to %c\n", MaxGrade);
	}

	/* test grade */
	if (MaxGrade != NULLCHAR) {
	    grade = iswrk(CNULL);
	    if (grade == NULLCHAR || MaxGrade < grade) {
		DEBUG(4, "No work of grade %c -- no call\n", MaxGrade);
		return(0);
	    }
	}


	while (checkdate(s, tp, t__now) == 0) {
		s = strchr(s, ',');
		if (s == CNULL)
			return(0);
		s++;
	}
	return(1);
}

/* worker function for ifdate() */
checkdate(s, tp, t__now)
char *s;
struct tm	*tp;
int	t__now;
{
	static char *days[] = {
		"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa", 0
	};
	int	i;

	/*
	 * check day of week
	 */

	while (isalpha(*s)) {
		if (PREFIX("Any", s))
			return(checktime(s, t__now));

		if (PREFIX("Wk", s) && tp->tm_wday >= 1 && tp->tm_wday <= 5)
			return(checktime(s, t__now));

		for (i = 0; days[i]; i++)
			if (PREFIX(days[i], s) && tp->tm_wday == i)
				return(checktime(s, t__now));
		s++;
	}

	return(0);	/* day match failed */
}

/* day match ok -- check time */
checktime(s, t__now)
char *s;
int	t__now;
{
	int	t__low, t__high;

	while (isalpha(*s))	/* flush day stuff */
		s++;

	if ((sscanf(s, "%d-%d", &t__low, &t__high) < 2))
		return(1);	/* time match ok (default) */

	if (t__low == t__high)
		return(1);

	/* 0000 crossover? */
	if (t__low < t__high) {
		if (t__low <= t__now && t__now <= t__high)
			return(1);
	} else {
		if (t__low <= t__now || t__now <= t__high)
			return(1);
	}

	return(0);
}

/***
 *	char *
 *	fdig(cp)	find first digit in string
 *
 *	return - pointer to first digit in string or end of string
 */

char *
fdig(cp)
char *cp;
{
	char *c;

	for (c = cp; *c; c++)
		if (*c >= '0' && *c <= '9')
			break;
	return(c);
}


#ifdef FASTTIMER
/*	Sleep in increments of 60ths of second.	*/
nap (time)
register int time;
{
	static int fd;

	if (fd == 0)
		fd = open (FASTTIMER, 0);

	(*Read) (fd, 0, time);
}

#endif /* FASTTIMER */

#ifdef BSD4_2

	/* nap(n) -- sleep for 'n' ticks of 1/60th sec each. */
	/* This version uses the select system call */


nap(n)
unsigned n;
{
	struct timeval tv;
	int rc;

	if (n==0)
		return;
	tv.tv_sec = n/60;
	tv.tv_usec = ((n%60)*1000000L)/60;
	rc = select(32, 0, 0, 0, &tv);
}

#endif /* BSD4_2 */

#ifdef NONAP

/*	nap(n) where n is ticks
 *
 *	loop using n/HZ part of a second
 *	if n represents more than 1 second, then
 *	use sleep(time) where time is the equivalent
 *	seconds rounded off to full seconds
 *	NOTE - this is a rough approximation and chews up
 *	processor resource!
 */

nap(n)
unsigned n;
{
	struct tms	tbuf;
	long endtime;
	int i;

	if (n > HZ) {
		/* > second, use sleep, rounding time */
		sleep( (int) (((n)+HZ/2)/HZ) );
		return;
	}

	/* use timing loop for < 1 second */
	endtime = times(&tbuf) + 3*n/4;	/* use 3/4 because of scheduler! */
	while (times(&tbuf) < endtime) {
	    for (i=0; i<1000; i++, i*i)
		;
	}
	return;
}


#endif /* NONAP */
