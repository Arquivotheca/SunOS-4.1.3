#ifndef lint
static	char *sccsid = "@(#)diff.c 1.1 92/07/30 SMI; from UCB 4.6 86/04/03";
#endif

#include "diff.h"
/*
 * diff - driver and subroutines
 */

char	diff[] = DIFF;
char	diffh[] = DIFFH;
char	pr[] = PR;
char	sobuf[BUFSIZ];
char	chrtran[256];

main(argc, argv)
	int argc;
	char **argv;
{
	register char *argp;
	register int c;

	setlocale(LC_ALL, "");
	ifdef1 = "FILE1"; ifdef2 = "FILE2";
	status = 2;
	diffargv = argv;
	setbuf(stdout, sobuf);
	argc--, argv++;
	while (argc > 2 && argv[0][0] == '-') {
		argp = &argv[0][1];
		argv++, argc--;
		while (*argp) switch(*argp++) {

#ifdef notdef
		case 'I':
			opt = D_IFDEF;
			wantelses = 0;
			continue;
		case 'E':
			opt = D_IFDEF;
			wantelses = 1;
			continue;
		case '1':
			opt = D_IFDEF;
			ifdef1 = argp;
			*--argp = 0;
			continue;
#endif
		case 'D':
			/* -Dfoo = -E -1 -2foo */
			wantelses = 1;
			ifdef1 = "";
			/* fall through */
#ifdef notdef
		case '2':
#endif
			opt = D_IFDEF;
			ifdef2 = argp;
			*--argp = 0;
			continue;
		case 'e':
			opt = D_EDIT;
			continue;
		case 'f':
			opt = D_REVERSE;
			continue;
		case 'n':
			opt = D_NREVERSE;
			continue;
		case 'b':
			bflag = 1;
			continue;
		case 'w':
			wflag = 1;
			continue;
		case 'i':
			iflag = 1;
			continue;
		case 't':
			tflag = 1;
			continue;
		case 'c':
			opt = D_CONTEXT;
			if (isdigit(*argp)) {
				context = atoi(argp);
				while (isdigit(*argp))
					argp++;
				if (*argp) {
					fprintf(stderr,
					    "diff: -c: bad count\n");
					done();
				}
				argp = "";
			} else
				context = 3;
			continue;
		case 'h':
			hflag++;
			continue;
		case 'S':
			if (*argp == 0) {
				fprintf(stderr, "diff: use -Sstart\n");
				done();
			}
			start = argp;
			*--argp = 0;		/* don't pass it on */
			continue;
		case 'r':
			rflag++;
			continue;
		case 's':
			sflag++;
			continue;
		case 'l':
			lflag++;
			continue;
		default:
			fprintf(stderr, "diff: -%s: unknown option\n",
			    --argp);
			done();
		}
	}
	if (argc != 2) {
		fprintf(stderr, "diff: two filename arguments required\n");
		done();
	}
	file1 = argv[0];
	file2 = argv[1];
	if (hflag && opt) {
		fprintf(stderr,
		    "diff: -h doesn't support -e, -f, -n, -c, or -I\n");
		done();
	}
	if (!strcmp(file1, "-"))
		stb1.st_mode = S_IFREG;
	else if (stat(file1, &stb1) < 0) {
		fprintf(stderr, "diff: ");
		perror(file1);
		done();
	}
	if (!strcmp(file2, "-"))
		stb2.st_mode = S_IFREG;
	else if (stat(file2, &stb2) < 0) {
		fprintf(stderr, "diff: ");
		perror(file2);
		done();
	}
	if (iflag) {
		for (c = 0; c < 256; c++)
			chrtran[c] = isupper(c) ? tolower(c) : c;
	} else {
		for (c = 0; c < 256; c++)
			chrtran[c] = c;
	}
	if ((stb1.st_mode & S_IFMT) == S_IFDIR &&
	    (stb2.st_mode & S_IFMT) == S_IFDIR) {
		diffdir(argv);
	} else
		diffreg();
	done();
	/* NOTREACHED */
}

char *
savestr(cp)
	register char *cp;
{
	register char *dp = malloc(strlen(cp)+1);

	if (dp == 0) {
		fprintf(stderr, "diff: ran out of memory\n");
		done();
	}
	strcpy(dp, cp);
	return (dp);
}

min(a,b)
	int a,b;
{

	return (a < b ? a : b);
}

max(a,b)
	int a,b;
{

	return (a > b ? a : b);
}

void
done()
{
	if (tempfile)
		unlink(tempfile);
	exit(status);
}

char *
talloc(n)
{
	register char *p;

	if ((p = malloc((unsigned)n)) != NULL)
		return(p);
	noroom();
	/*NOTREACHED*/
}

char *
ralloc(p,n)
char *p;
{
	register char *q;
	char *realloc();

	if ((q = realloc(p, (unsigned)n)) == NULL)
		noroom();
	return(q);
}

noroom()
{
	fprintf(stderr, "diff: files too big, try -h\n");
	done();
}

#include <a.out.h>

/*
 * We consider a "binary" file to be one that:
 * 1) looks like a executable;
 * 2) contains a null character ("diff" doesn't handle them correctly, and
 *    neither do many other UNIX text-processing commands).
 * Characters with their 8th bit set do NOT make a file binary; they may be
 * legitimate text characters, or parts of same.
 */
isbinary(buf, cnt)
	char *buf;
	register int cnt;
{
	register char *cp;

	if (cnt >= sizeof (struct exec)) {
		struct exec hdr;
		hdr = *(struct exec *)buf;
		if (!N_BADMAG(hdr))
			return (1);
	}
	cp = buf;
	while (--cnt >= 0)
		if (*cp++ == '\0')
			return (1);
	return (0);
}
