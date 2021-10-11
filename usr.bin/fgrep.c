#ifndef lint
static	char sccsid[] = "@(#)fgrep.c 1.1 92/07/30 SMI"; /* from UCB 4.3 5/30/85 */
#endif
/*
 * fgrep -- print all lines containing any of a set of keywords
 *
 *	status returns:
 *		0 - ok, and some matches
 *		1 - ok, but no matches
 *		2 - some error
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/stat.h>

#define BLKSIZE 8192
#define	MAXSIZ 6000
#define QSIZE 400
struct words {
	char 	inp;
	char	out;
	struct	words *nst;
	struct	words *link;
	struct	words *fail;
} w[MAXSIZ], *smax, *q;

long	lnum;
int	bflag, cflag, fflag, lflag, nflag, vflag, xflag, yflag, eflag;
int	hflag	= 1;
int	sflag;
int	retcode = 0;
int	nfile;
long	blkno;
int	nsucc;
long	tln;
FILE	*wordf;
char	*wordfname;
unsigned char	*argptr;
extern	char *optarg;
extern	int optind;
void exit();

main(argc, argv)
char **argv;
{
	register c;
	char *usage;
	int errflg = 0;
	usage = "[ -bchilnsvx ] [ -e exp ] [ -f file ] [ strings ] [ file ] ...";

	while(( c = getopt(argc, argv, "shbciye:f:lnvx")) != EOF)
		switch(c) {

		case 's':
			sflag++;
			continue;

		case 'h':
			hflag = 0;
			continue;

		case 'b':
			bflag++;
			continue;

		case 'c':
			cflag++;
			continue;

		case 'e':
			eflag++;
			argptr = (unsigned char *) optarg;
			continue;

		case 'f':
			if (fflag) {
				fprintf(stderr, "fgrep: Only one \"-f\" option allowed\n");
				exit(2);
			}
			fflag++;
			wordfname = optarg;
			continue;

		case 'l':
			lflag++;
			continue;

		case 'n':
			nflag++;
			continue;

		case 'v':
			vflag++;
			continue;

		case 'x':
			xflag++;
			continue;

		case 'i':		/* Berkeley, S5 */
		case 'y':		/* V7 */
			yflag++;
			continue;

		case '?':
			errflg++;
	}

	argc -= optind;
	if (errflg || ((argc <= 0) && !fflag && !eflag)) {
		printf("usage: fgrep %s\n",usage);
		exit(2);
	}

	if (fflag) {
		wordf = fopen(wordfname, "r");
		if (wordf==NULL) {
			fprintf(stderr, "fgrep: ");
			perror(wordfname);
			exit(2);
		}
	} else {
		if (!eflag) {
			argptr = (unsigned char *) argv[optind];
			optind++;
			argc--;
		}
	}

	cgotofn();
	cfail();
	nfile = argc;
	argv = &argv[optind];
	if (argc<=0) {
		if (lflag) exit(1);
		execute((char *)NULL);
	}
	else while (--argc >= 0) {
		execute(*argv);
		argv++;
	}
	exit(retcode != 0 ? retcode : nsucc == 0);
	/* NOTREACHED */
}

# define ccomp(a,b) (yflag ? lca(a)==lca(b) : a==b)
# define lca(x) (isupper(x) ? tolower(x) : x)
execute(file)
char *file;
{
	register struct words *c;
	register ccount;
	register char ch;
	register char *p;
	static char *buf;
	static int blksize;
	struct stat stb;
	int f;
	int failed;
	char *nlp;
	if (file) {
		if ((f = open(file, 0)) < 0) {
			fprintf(stderr, "fgrep: ");
			perror(file);
			retcode = 2;
			return;
		}
	}
	else f = 0;
	if (buf == NULL) {
		if (fstat(f, &stb) > 0 && stb.st_blksize > 0)
			blksize = stb.st_blksize;
		else
			blksize = BLKSIZE;
		buf = (char *)malloc(2*blksize);
		if (buf == NULL) {
			fprintf(stderr, "fgrep: no memory for %s\n", file);
			retcode = 2;
			return;
		}
	}
	ccount = 0;
	failed = 0;
	lnum = 1;
	tln = 0;
	blkno = 0;
	p = buf;
	nlp = p;
	c = w;
	for (;;) {
		if (--ccount <= 0) {
			if (p == &buf[2*blksize]) p = buf;
			if (p > &buf[blksize]) {
				if ((ccount = read(f, p, &buf[2*blksize] - p)) <= 0) break;
			}
			else if ((ccount = read(f, p, blksize)) <= 0) break;
			blkno += ccount;
		}
		nstate:
			if (ccomp(c->inp, *p)) {
				c = c->nst;
			}
			else if (c->link != 0) {
				c = c->link;
				goto nstate;
			}
			else {
				c = c->fail;
				failed = 1;
				if (c==0) {
					c = w;
					istate:
					if (ccomp(c->inp ,  *p)) {
						c = c->nst;
					}
					else if (c->link != 0) {
						c = c->link;
						goto istate;
					}
				}
				else goto nstate;
			}
		if (c->out) {
			while (*p++ != '\n') {
				if (--ccount <= 0) {
					if (p == &buf[2*blksize]) p = buf;
					if (p > &buf[blksize]) {
						if ((ccount = read(f, p, &buf[2*blksize] - p)) <= 0) break;
					}
					else if ((ccount = read(f, p, blksize)) <= 0) break;
					blkno += ccount;
				}
			}
			if (ccount < 0)
				goto done;
			if ( (vflag && (failed == 0 || xflag == 0)) || (vflag == 0 && xflag && failed) )
				goto nomatch;
	succeed:	nsucc = 1;
			if (cflag) tln++;
			else if (sflag)
				;	/* ugh */
			else if (lflag) {
				printf("%s\n", file);
				close(f);
				return;
			}
			else {
				if (nfile > 1 && hflag) printf("%s:", file);
				if (bflag) printf("%ld:", (blkno-ccount-1)/DEV_BSIZE);
				if (nflag) printf("%ld:", lnum);
				if (p <= nlp) {
					while (nlp < &buf[2*blksize]) putchar(*nlp++);
					nlp = buf;
				}
				while (nlp < p) putchar(*nlp++);
			}
	nomatch:	lnum++;
			nlp = p;
			c = w;
			failed = 0;
			continue;
		}
		if (*p++ == '\n')
			if (vflag) goto succeed;
			else {
				lnum++;
				nlp = p;
				c = w;
				failed = 0;
			}
	}
done:
	if (ccount < 0) {
		fprintf(stderr, "fgrep: Read error on ");
		perror(file ? file : "standard input");
		retcode = 2;
		close(f);
		return;
	}
	close(f);
	if (cflag) {
		if (nfile > 1)
			printf("%s:", file);
		printf("%ld\n", tln);
	}
}

getargc()
{
	register c;
	if (wordf) {
		c = getc(wordf);
		if (c == EOF) {
			if (ferror(wordf)) {
				fprintf(stderr, "fgrep: Read error on ");
				perror(wordfname);
				exit(1);
			}
			fclose(wordf);
		}
	} else {
		/*
		 * argptr must be (unsigned char *) so 8-bit chars
		 * don't sign-extend
		 */
		if ((c = *argptr++) == '\0')
			return(EOF);
	}
	return(c);
}

cgotofn() {
	register c;
	register struct words *s;

	s = smax = w;
nword:	for(;;) {
		c = getargc();
		if (c==EOF)
			return;
		if (c == '\n') {
			if (xflag) {
				for(;;) {
					if (s->inp == c) {
						s = s->nst;
						break;
					}
					if (s->inp == 0) goto nenter;
					if (s->link == 0) {
						if (smax >= &w[MAXSIZ -1]) overflo();
						s->link = ++smax;
						s = smax;
						goto nenter;
					}
					s = s->link;
				}
			}
			s->out = 1;
			s = w;
		} else {
		loop:	if (ccomp(s->inp, c)) {
				s = s->nst;
				continue;
			}
			if (s->inp == 0) goto enter;
			if (s->link == 0) {
				if (smax >= &w[MAXSIZ - 1]) overflo();
				s->link = ++smax;
				s = smax;
				goto enter;
			}
			s = s->link;
			goto loop;
		}
	}

	enter:
	do {
		s->inp = c;
		if (smax >= &w[MAXSIZ - 1]) overflo();
		s->nst = ++smax;
		s = smax;
	} while ((c = getargc()) != '\n' && c!=EOF);
	if (xflag) {
	nenter:	s->inp = '\n';
		if (smax >= &w[MAXSIZ -1]) overflo();
		s->nst = ++smax;
	}
	smax->out = 1;
	s = w;
	if (c != EOF)
		goto nword;
}

overflo() {
	fprintf(stderr, "fgrep: wordlist too large\n");
	exit(2);
}
cfail() {
	struct words *queue[QSIZE];
	struct words **front, **rear;
	struct words *state;
	int bstart;
	register char c;
	register struct words *s;
	s = w;
	front = rear = queue;
init:	if ((s->inp) != 0) {
		*rear++ = s->nst;
		if (rear >= &queue[QSIZE - 1]) overflo();
	}
	if ((s = s->link) != 0) {
		goto init;
	}

	while (rear!=front) {
		s = *front;
		if (front == &queue[QSIZE-1])
			front = queue;
		else front++;
	cloop:	if ((c = s->inp) != 0) {
			bstart = 0;
			*rear = (q = s->nst);
			if (front < rear)
				if (rear >= &queue[QSIZE-1])
					if (front == queue) overflo();
					else rear = queue;
				else rear++;
			else
				if (++rear == front) overflo();
			state = s->fail;
		floop:	if (state == 0) {
				state = w;
				bstart = 1;
			}
			if (state->inp == c) {
			qloop:	q->fail = state->nst;
				if ((state->nst)->out == 1) q->out = 1;
				if ((q = q->link) != 0) goto qloop;
			}
			else if ((state = state->link) != 0)
				goto floop;
			else if(bstart == 0){
				state = 0;
				goto floop;
			}
		}
		if ((s = s->link) != 0)
			goto cloop;
	}
}
