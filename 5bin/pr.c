/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)pr.c 1.1 92/07/30 SMI"; /* from S5R3 1.7 */
#endif

/*
 *	PR command (print files in pages and columns, with headings)
 *	2+head+2+page[56]+5
 */

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>

#define ESC	'\033'
#define LENGTH	66
#define LINEW	72
#define NUMW	5
#define MARGIN	10
#define DEFTAB	8
#define NFILES	10

#define STDINNAME()	nulls
#define PROMPT()	putc('\7', stderr) /* BEL */
#define NOFILE	nulls
#define TABS(N,C)	if ((N = intopt(argv, &C)) < 0) N = DEFTAB
#define ETABS	(Inpos % Etabn)
#define ITABS	(Itabn > 0 && Nspace >= (nc = Itabn - Outpos % Itabn))
#define NSEPC	'\t'
#define HEAD	"%12.12s %4.4s  %s Page %d\n\n\n", date+4, date+20, head, Page
#define cerror(S)	fprintf(stderr, "pr: %s", S)
#define done()	if (Ttyout) chmod(Ttyout, Mode)

typedef char CHAR;
typedef int ANY;
typedef unsigned UNS;
typedef struct { FILE *f_f; char *f_name; int f_nextc; } FILS;

ANY *getspace();

FILS *Files;
FILE *fopen(), *mustopen();

int Mode;
int Multi = 0, Nfiles = 0, Error = 0, onintr();

char nulls[] = "";
char *ttyname(), *Ttyout, obuf[BUFSIZ];

extern	void	exit();
extern	void	_exit();
extern	char	*malloc();
extern	time_t	time();

void
fixtty()
{
	struct stat sbuf;

	setbuf(stdout, obuf);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN) signal(SIGINT, onintr);
	if (Ttyout= ttyname(fileno(stdout))) {		/* is stdout a tty? */
		stat(Ttyout, &sbuf);
		Mode = sbuf.st_mode&0777;		/* save permissions */
		chmod(Ttyout, 0600);
	}
	return;
}

char *GETDATE() /* return date file was last modified */
{
	char *ctime();
	static char *now = NULL;
	static struct stat sbuf, nbuf;

	if (Nfiles > 1 || Files->f_name == nulls) {
		if (now == NULL)
			{ time(&nbuf.st_mtime); now = ctime(&nbuf.st_mtime); }
		return (now);
	} else {
		stat(Files->f_name, &sbuf);
		return (ctime(&sbuf.st_mtime));
	}
}


char *ffiler(s) char *s;
{
	static char buf[100];

	sprintf(buf, "can't open %s", s);
	return (buf);
}


main(argc, argv) char *argv[];
{
	FILS fstr[NFILES];
	int nfdone = 0;

	setlocale(LC_ALL, "");
	Files = fstr;
	for (argc = findopt(argc, argv); argc > 0; --argc, ++argv)
		if (Multi == 'm') {
			if (Nfiles >= NFILES - 1) die("too many files");
			if (mustopen(*argv, &Files[Nfiles++]) == NULL)
				++nfdone; /* suppress printing */
		} else {
			if (print(*argv))
				fclose(Files->f_f);
			++nfdone;
		}
	if (!nfdone) /* no files named, use stdin */
		print(NOFILE); /* on GCOS, use current file, if any */
	errprint(); /* print accumulated error reports */
	exit(Error);	/*NOTREACHED*/
}


long Lnumb = 0;
FILE *Ttyin = stdin;
int Dblspace = 1, Fpage = 1, Formfeed = 0,
	Length = LENGTH, Linew = 0, Offset = 0, Ncols = 1, Pause = 0, Sepc = 0,
	Colw, Plength, Margin = MARGIN, Numw, Nsepc = NSEPC, Report = 1,
	Etabn = 0, Etabc = '\t', Itabn = 0, Itabc = '\t';
char *Head = NULL;
CHAR *Buffer = NULL, *Bufend;
typedef struct { CHAR *c_ptr, *c_ptr0; long c_lno; } *COLP;
COLP Colpts;
					/* findopt() returns eargc modified to be    */
					/* the number of explicitly supplied         */
					/* filenames, including '-', the explicit    */
					/* request to use stdin.  eargc == 0 implies */
					/* that no filenames were supplied and       */
					/* stdin should be used.		     */
findopt(argc, argv) char *argv[];
{
	char **eargv = argv;
	int eargc = 0, c;
	int mflg = 0, aflg = 0;
	void usage();
	fixtty();
	while (--argc > 0) {
		switch (c = **++argv) {
		case '-':
			if ((c = *++*argv) == '\0') break;
		case '+':
			do {
				if (isdigit(c)) {
					--*argv;
					Ncols = atoix(argv);
					}
				else
					switch (c) {
					case '+': if ((Fpage = atoix(argv)) < 1)
							Fpage = 1;
						  continue;
					case 'd': Dblspace = 2;
						  continue;
					case 'e': TABS(Etabn, Etabc);
						  continue;
					case 'f': ++Formfeed;
						  continue;
					case 'h': if (--argc > 0)
						  Head = argv[1];
						  continue;
					case 'i': TABS(Itabn, Itabc);
						  continue;
					case 'l': Length = atoix(argv);
						  continue;
					case 'a': aflg++;
						  Multi = c;
						  continue;
					case 'm': mflg++; 
						  Multi = c; 
						  continue;
					case 'o': Offset = atoix(argv);
						  continue;
					case 'p': ++Pause;
						  continue;
					case 'r': Report = 0;
						  continue;
					case 's': if ((Sepc = (*argv)[1]) != '\0')
							++*argv;
						  else
							Sepc = '\t';
						  continue;
					case 't': Margin = 0;
						  continue;
					case 'w': Linew = atoix(argv);
						  continue;
					case 'n': ++Lnumb;
						  if ((Numw = intopt(argv,&Nsepc)) <= 0)
							Numw = NUMW;
						  continue;
					default : 
						  fprintf(stderr,
						 	 "pr: unknown option, %c\n",
							 (char)c);
						  usage();
					}
							/* Advance over options    */
							/* until null is found.    */
							/* Allows clumping options */
							/* as in "pr -pm f1 f2".   */
			} while ((c = *++*argv) != '\0');

			if (Head == argv[1]) ++argv;
			continue;
		}
		*eargv++ = *argv;
		++eargc;	/* count the filenames */
	}

	if (mflg && (Ncols > 1)) {
		fprintf(stderr, "pr: only one of either -m or -column allowed\n");
		usage();
	}
	if (aflg && (Ncols < 2)) {
		fprintf(stderr, "pr: -a valid only with -column\n");
		usage();
	}
	if (Length == 0) Length = LENGTH;
	if (Length <= Margin) Margin = 0;
	Plength = Length - Margin/2;
	if (Multi == 'm') Ncols = eargc;
	switch (Ncols) {
	case 0:
		Ncols = 1;
	case 1:
		break;
	default:
		if (Etabn == 0) /* respect explicit tab specification */
			Etabn = DEFTAB;
		if (Itabn == 0)
			Itabn = DEFTAB;
	}
	if (Linew == 0) Linew = Ncols != 1 && Sepc == 0 ? LINEW : 512;
	if (Lnumb) {
		int numw;

		if (Nsepc == '\t') {
			if(Itabn == 0)
					numw = Numw + DEFTAB - (Numw % DEFTAB);
			else
					numw = Numw + Itabn - (Numw % Itabn);
		}else {
				numw = Numw + ((isprint(Nsepc)) ? 1 : 0);
		}
		Linew -= (Multi == 'm') ? numw : numw * Ncols;
	}
	if ((Colw = (Linew - Ncols + 1)/Ncols) < 1)
		die("width too small");
	if (Ncols != 1 && Multi == 0) {
		UNS buflen = ((UNS)(Plength/Dblspace + 1))*(Linew+1)*sizeof(CHAR);
		Buffer = (CHAR *)getspace(buflen);
		Bufend = &Buffer[buflen];
		Colpts = (COLP)getspace((UNS)((Ncols+1)*sizeof(*Colpts)));
	}
							/* is stdin not a tty? */
	if (Ttyout && (Pause || Formfeed) && !ttyname(fileno(stdin)))
		Ttyin = fopen("/dev/tty", "r");
	return (eargc);
}

intopt(argv, optp) char *argv[]; int *optp;
{
	int c;

	if ((c = (*argv)[1]) != '\0' && !isdigit(c)) { *optp = c; ++*argv; }
	return ((c = atoix(argv)) != 0 ? c : -1);
}

int Page, C = '\0', Nspace, Inpos;

print(name) char *name;
{
	static int notfirst = 0;
	char *date = NULL, *head = NULL;
	int c;

	if (Multi != 'm' && mustopen(name, &Files[0]) == NULL) return (0);
	if (Multi == 'm' && Nfiles == 0 && mustopen(name, &Files[0]) == NULL)
		die("cannot open stdin");
	if (Buffer) ungetc(Files->f_nextc, Files->f_f);
	if (Lnumb) Lnumb = 1;
	for (Page = 0; ; putpage()) {
		if (C == EOF) break;
		if (Buffer) nexbuf();
		Inpos = 0;
		if (get(0) == EOF) break;
		Inpos = 0;
		fflush(stdout);
		if (++Page >= Fpage) {
			if (Ttyout && (Pause || Formfeed && !notfirst++)) {
				if (Ttyin != NULL) {
					PROMPT(); /* prompt with bell and pause */
					while ((c = getc(Ttyin)) != EOF && c != '\n') ;
				}
			}
			if (Margin == 0) continue;
			if (date == NULL) date = GETDATE();
			if (head == NULL) head = Head != NULL ? Head :
				Nfiles < 2 ? Files->f_name : nulls;
			printf("\n\n");
			Nspace = Offset;
			putspace();
			printf(HEAD);
		}
	}
	C = '\0';
	return (1);
}

int Outpos, Lcolpos, Pcolpos, Line;

putpage()
{
	register int colno;

	for (Line = Margin/2; ; get(0)) {
		for (Nspace = Offset, colno = 0, Outpos = 0; C != '\f'; ) {
			if (Lnumb && C != EOF && ((colno == 0 && Multi == 'm') || Multi != 'm')) {
				if (Page >= Fpage) {
					putspace();
					printf("%*ld%c", Numw, Buffer ?
						Colpts[colno].c_lno++ : Lnumb, Nsepc);
				}
				++Lnumb;
			}
			for (Lcolpos = 0, Pcolpos = 0;
				C != '\n' && C != '\f' && C != EOF; get(colno))
					put(C);
			if (C == EOF || ++colno == Ncols ||
				C == '\n' && get(colno) == EOF) break;
			if (Sepc) put(Sepc);
			else if ((Nspace += Colw - Lcolpos + 1) < 1) Nspace = 1;
		}
		if (C == EOF) {
			if (Margin != 0) break;
			if (colno != 0) put('\n');
			return;
		}
		if (C == '\f') break;
		put('\n');
		if (Dblspace == 2 && Line < Plength) put('\n');
		if (Line >= Plength) break;
	}
	if (Formfeed) put('\f');
	else while (Line < Length) put('\n');
}

nexbuf()
{
	register CHAR *s = Buffer;
	register COLP p = Colpts;
	int j, c, bline = 0;

	for ( ; ; ) {
		p->c_ptr0 = p->c_ptr = s;
		if (p == &Colpts[Ncols]) return;
		(p++)->c_lno = Lnumb + bline;
		for (j = (Length - Margin)/Dblspace; --j >= 0; ++bline)
			for (Inpos = 0; ; ) {
				if ((c = getc(Files->f_f)) == EOF) {
					for (*s = EOF; p <= &Colpts[Ncols]; ++p)
						p->c_ptr0 = p->c_ptr = s;
					balance(bline);
					return;
				}
				if (isprint(c)) ++Inpos;
				if (Inpos <= Colw || c == '\n') {
					*s = c;
					if (++s >= Bufend)
						die("page-buffer overflow");
				}
				if (c == '\n') break;
				switch (c) {
				case '\b': if (Inpos == 0) --s;
				case ESC:  if (Inpos > 0) --Inpos;
				}
			}
	}
}

balance(bline) /* line balancing for last page */
{
	register CHAR *s = Buffer;
	register COLP p = Colpts;
	int colno = 0, j, c, l;

	c = bline % Ncols;
	l = (bline + Ncols - 1)/Ncols;
	bline = 0;
	do {
		for (j = 0; j < l; ++j)
			while (*s++ != '\n') ;
		(++p)->c_lno = Lnumb + (bline += l);
		p->c_ptr0 = p->c_ptr = s;
		if (++colno == c) --l;
	} while (colno < Ncols - 1);
}

get(colno)
{
	static int peekc = 0;
	register COLP p;
	register FILS *q;
	register int c;

	if (peekc)
		{ peekc = 0; c = Etabc; }
	else if (Buffer) {
		p = &Colpts[colno];
		if (p->c_ptr >= (p+1)->c_ptr0) c = EOF;
		else if ((c = *p->c_ptr) != EOF) ++p->c_ptr;
	} else if ((c = 
		(q = &Files[Multi == 'a' ? 0 : colno])->f_nextc) == EOF) {
		for (q = &Files[Nfiles]; --q >= Files && q->f_nextc == EOF; ) ;
		if (q >= Files) c = '\n';
	} else
		q->f_nextc = getc(q->f_f);
	if (Etabn != 0 && c == Etabc) {
		++Inpos;
		peekc = ETABS;
		c = ' ';
	} else if (isprint(c))
		++Inpos;
	else
		switch (c) {
		case '\b':
		case ESC:
			if (Inpos > 0) --Inpos;
			break;
		case '\f':
			if (Ncols == 1) break;
			c = '\n';
		case '\n':
		case '\r':
			Inpos = 0;
		}
	return (C = c);
}

put(c)
{
	int move;

	switch (c) {
	case ' ':
		if(Ncols < 2 || Lcolpos < Colw) {
			++Nspace;
			++Lcolpos;
		}
		return;
	case '\t':
		if(Itabn == 0)
			break;
		if(Lcolpos < Colw) {
			move = Itabn - ((Lcolpos + Itabn) % Itabn);
			move = (move < Colw-Lcolpos) ? move : Colw-Lcolpos;
			Nspace += move;
			Lcolpos += move;
		}
		return;
	case '\b':
		if (Lcolpos == 0) return;
		if (Nspace > 0) { --Nspace; --Lcolpos; return; }
		if (Lcolpos > Pcolpos) { --Lcolpos; return; }
	case ESC:
		move = -1;
		break;
	case '\n':
		++Line;
	case '\r':
	case '\f':
		Pcolpos = 0; Lcolpos = 0; Nspace = 0; Outpos = 0;
	default:
		move = (isprint(c) != 0);
	}
	if (Page < Fpage) return;
	if (Lcolpos > 0 || move > 0) Lcolpos += move;
	putspace();
	if (Ncols < 2 || Lcolpos <= Colw) {
		putchar(c);
		Outpos += move;
		Pcolpos = Lcolpos;
	}
}

putspace()
{
	int nc;

	for ( ; Nspace > 0; Outpos += nc, Nspace -= nc)
		if (ITABS)
			putchar(Itabc);
		else {
			nc = 1;
			putchar(' ');
		}
}

atoix(p) register char **p;
{
	register int n = 0, c;

	while (isdigit(c = *++*p)) n = 10*n + c - '0';
	--*p;
	return (n);
}

/* Defer message about failure to open file to prevent messing up
   alignment of page with tear perforations or form markers.
   Treat empty file as special case and report as diagnostic.
*/
#define EMPTY	14	/* length of " -- empty file" */
typedef struct err { struct err *e_nextp; char *e_mess; } ERR;
ERR *Err = NULL, *Lasterr = (ERR *)&Err;

FILE *mustopen(s, f) char *s; register FILS *f;
{
	if (*s == '\0') {
		f->f_name = STDINNAME();
		f->f_f = stdin;
	} else if ((f->f_f = fopen(f->f_name = s, "r")) == NULL) {
		char *strcpy();
		s = ffiler(f->f_name);
		s = strcpy((char *)getspace((UNS)(strlen(s) + 1)), s);
	}
	if (f->f_f != NULL) {
		if ((f->f_nextc = getc(f->f_f)) != EOF || Multi == 'm')
			return (f->f_f);
		sprintf(s = (char *)getspace((UNS)(strlen(f->f_name) + 1 + EMPTY)),
			"%s -- empty file", f->f_name);
		fclose(f->f_f);
	}
	Error = 1;
	if (Report)
		if (Ttyout) { /* accumulate error reports */
			Lasterr = Lasterr->e_nextp = (ERR *)getspace((UNS)sizeof(ERR));
			Lasterr->e_nextp = NULL;
			Lasterr->e_mess = s;
		} else { /* ok to print error report now */
			cerror(s);
			putc('\n', stderr);
		}
	return ((FILE *)NULL);
}

ANY *getspace(n) UNS n;
{
	ANY *t;

	if ((t = (ANY *)malloc(n)) == NULL) die("out of space");
	return (t);
}

die(s) char *s;
{
	++Error;
	errprint();
	cerror(s);
	putc('\n', stderr);
	exit(1);
}

onintr()
{
	++Error;
	errprint();
	_exit(1);
}

errprint() /* print accumulated error reports */
{
	fflush(stdout);
	for ( ; Err != NULL; Err = Err->e_nextp) {
		cerror(Err->e_mess);
		putc('\n', stderr);
	}
	done();
}
void
usage()
{
char *usage2 = "[-column [-wwidth] [-a]] [-ect] [-ict] [-drtfp] [+page] [-nsk]";
char *usage3 = "[-ooffset] [-llength] [-sseparator] [-h header] [file ...]";
char *usage4 = "[-m      [-wwidth]]      [-ect] [-ict] [-drtfp] [+page] [-nsk]";
char *usage5 = "[-ooffset] [-llength] [-sseperator] [-h header] file1 file2 ...";

	fprintf(stderr,
		"usage: pr %s  \\\n          %s\n\n       pr %s  \\\n          %s\n",
		usage2, usage3, usage4, usage5);
	exit(1);
} 
