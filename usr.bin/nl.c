#ifndef lint
static	char sccsid[] = "@(#)nl.c 1.1 92/07/30 SMI"; /* from S5R2 1.25 */
#endif

/*	NLSID
*/

#include <stdio.h>	/* Include Standard Header File */

/*	THE FOLLOWING LINES ARE NEEDED FOR
	THE REGEXP(7) ROUTINE			*/

#define INIT		register char *ptr = instring;
#define GETC()		(*ptr++)
#define PEEKC()		(*ptr)
#define UNGETC(c)	(--ptr)
#define RETURN(c)	return;
#define ERROR(c)	regerr(c);
#define EXPSIZ		128

#include <regexp.h>	/* Include Regexp source */

#ifdef u370
	int nbra, sed;	/* u370 - not used in nl.c, but extern in regexp.h */
#endif
	int width = 6;	/* Declare default width of number */
	char nbuf[100];	/* Declare bufsize used in convert/pad/cnt routines */
	char bexpbuf[EXPSIZ];	/* Declare the size of the regexp buf */
	char hexpbuf[EXPSIZ];	/* Declare the size of the regexp buf */
	char fexpbuf[EXPSIZ];	/* Declare the size of the regexp buf */
	int bsave;
	int hsave;
	int fsave;
	char delim1 = '\\'; char delim2 = ':';	/* Default delimiters. */
	char pad = ' ';	/* Declare the default pad for numbers */
	char *s;	/* Declare the temp array for args */
	char s1[EXPSIZ];	/* Declare the conversion array */
	char format = 'n';	/* Declare the format of numbers to be rt just */
	int q = 2;	/* Initialize arg pointer to drop 1st 2 chars */
	int k;	/* Declare var for return of convert */
	int r;	/* Declare the arg array ptr for string args */

main(argc,argv)
int argc;
char *argv[];
{
	register int j;
	register int i = 0;
	register char *p;
	register char header = 'n';
	register char body = 't';
	register char footer = 'n';
	char line[BUFSIZ];
	char tempchr;	/* Temporary holding variable. */
	char swtch = 'n';
	char cntck = 'n';
	char type;
	int cnt;	/* line counter */
	int pass1 = 1;	/* First pass flag. 1=pass1, 0=additional passes. */
	char sep[EXPSIZ];
	char pat[EXPSIZ];
	char *string;
	register char *ptr ;
	int startcnt=1;
	int increment=1;
	int blank=1;
	int blankctr = 0;
	int c;
	char last;
	FILE *iptr=stdin;
	FILE *optr=stdout;
	sep[0] = '\t';
	sep[1] = '\0';

/*		DO WHILE THERE IS AN ARGUMENT
		CHECK ARG, SET IF GOOD, ERR IF BAD	*/

for (j = 1; j < argc; j++) {
	if (argv[j][i] == '-' && (c = argv[j][i + 1])) {
		switch(c) {
			case 'h':
				switch(argv[j][i + 2]) {
					case 'n':
						header = 'n';
						break;
					case 't':
						header = 't';
						break;
					case 'a':
						header = 'a';
						break;
					case 'p':
						s=argv[j];
						q=3;
						r=0;
						while (s[q] != '\0'){
							pat[r] = s[q];
							r++;
							q++;
						}
						pat[r] = '\0';
						header = 'h';
						ptr = pat;
					compile(pat, hexpbuf, &hexpbuf[EXPSIZ], '\0');
					hsave = circf;
						break;
					case '\0':
						header = 'n';
						break;
					default:
						printf("HEADER: ");
						optmsg(argv[j]);
				}
				break;
			case 'b':
				switch(argv[j][i + 2]) {
					case 't':
						body = 't';
						break;
					case 'a':
						body = 'a';
						break;
					case 'n':
						body = 'n';
						break;
					case 'p':
						s=argv[j];
						q=3;
						r=0;
						while (s[q] != '\0'){
							pat[r] = s[q];
							r++;
							q++;
						}
						pat[r] = '\0';
						body = 'b';
						ptr = pat;
					compile(pat, bexpbuf, &bexpbuf[EXPSIZ], '\0');
					bsave = circf;
						break;
					case '\0':
						body = 't';
						break;
					default:
						printf("BODY: ");
						optmsg(argv[j]);
				}
				break;
			case 'f':
				switch(argv[j][i + 2]) {
					case 'n':
						footer = 'n';
						break;
					case 't':
						footer = 't';
						break;
					case 'a':
						footer = 'a';
						break;
					case 'p':
						s=argv[j];
						q=3;
						r=0;
						while (s[q] != '\0'){
							pat[r] = s[q];
							r++;
							q++;
						}
						pat[r] = '\0';
						footer = 'f';
						ptr = pat;
					compile(pat, fexpbuf, &fexpbuf[EXPSIZ], '\0');
					fsave = circf;
						break;
					case '\0':
						footer = 'n';
						break;
					default:
						printf("FOOTER: ");
						optmsg(argv[j]);
				}
				break;
			case 'p':
				if (argv[j][i+2] == '\0')
				cntck = 'y';
				else
				{
				optmsg(argv[j]);
				}
				break;
			case 'v':
				if (argv[j][i+2] == '\0')
				startcnt = 1;
				else
				startcnt = convert(argv[j]);
				break;
			case 'i':
				if (argv[j][i+2] == '\0')
				increment = 1;
				else
				increment = convert(argv[j]);
				break;
			case 'w':
				if (argv[j][i+2] == '\0')
				width = 6;
				else
				width = convert(argv[j]);
				break;
			case 'l':
				if (argv[j][i+2] == '\0')
				blank = 1;
				else
				blank = convert(argv[j]);
				break;
			case 'n':
				switch (argv[j][i+2]) {
					case 'l':
						if (argv[j][i+3] == 'n')
						format = 'l';
						else
				{
				optmsg(argv[j]);
				}
						break;
					case 'r':
						if (argv[j][i+3] == 'n' || argv[j][i+3] == 'z')
						format = argv[j][i+3];
						else
				{
				optmsg(argv[j]);
				}
						break;
					case '\0':
						format = 'n';
						break;
					default:
				optmsg(argv[j]);
					break;
				}
				break;
			case 's':
				s = argv[j];
				q = 2;
				r = 0;
				while (s[q] != '\0') {
					sep[r] = s[q];
					r++;
					q++;
				}
				sep[r] = '\0';
				break;
			case 'd':
				tempchr = argv[j][i+2];
				if(tempchr == '\0')break;
				delim1 = tempchr;

				tempchr = argv[j][i+3];
				if(tempchr == '\0')break;
				delim2 = tempchr;
				if(argv[j][i+4] != '\0')optmsg(argv[j]);
				break;
			default:
				optmsg(argv[j]);
			}
		continue; /* If it got here, a valid -xx option was found.
				Now, start next pass of FOR loop. */
		}
		else
			if ((iptr = fopen(argv[j],"r")) == NULL)  {
				printf("CANNOT OPEN FILE %s\n", argv[j]);
				exit(1);
			}
}		/* Closing brace of "for" (~ line 71). */

	/* ON FIRST PASS ONLY, SET LINE COUNTER (cnt) = startcnt &
		SET DEFAULT BODY TYPE TO NUMBER ALL LINES.	*/
	if(pass1){cnt = startcnt; type = body; last = 'b'; pass1 = 0;}

/*		DO WHILE THERE IS INPUT
		CHECK TO SEE IF LINE IS NUMBERED,
		IF SO, CALCULATE NUM, PRINT NUM,
		THEN OUTPUT SEPERATOR CHAR AND LINE	*/

	while (( p = fgets(line,sizeof(line),iptr)) != NULL) {
	if (p[0] == delim1 && p[1] == delim2) {
		if (p[2] == delim1 && p[3] == delim2 && p[4]==delim1 && p[5]==delim2 && p[6] == '\n') {
			if ( cntck != 'y')
				cnt = startcnt;
			type = header;
			last = 'h';
			swtch = 'y';
		}
		else {
			if (p[2] == delim1 && p[3] == delim2 && p[4] == '\n') {
				if ( cntck != 'y' && last != 'h')
				cnt = startcnt;
				type = body;
				last = 'b';
				swtch = 'y';
			}
			else {
				if (p[0] == delim1 && p[1] == delim2 && p[2] == '\n') {
				if ( cntck != 'y' && last == 'f')
				cnt = startcnt;
					type = footer;
					last = 'f';
					swtch = 'y';
				}
			}
		}
	}
	if (swtch == 'y') {
		swtch = 'n';
		fprintf(optr,"\n");
	}
	else {
		switch(type) {
			case 'n':
				npad(width,sep);
				break;
			case 't':
				if (p[0] != '\n') {
					pnum(cnt,sep);
					cnt+=increment;
				}
				else {
					npad(width,sep);
				}
				break;
			case 'a':
				if (p[0] == '\n') {
					blankctr++;
					if (blank == blankctr) {
						blankctr = 0;
						pnum(cnt,sep);
						cnt+=increment;
					}
					else npad(width,sep);
				}
				else {
					blankctr = 0;
					pnum(cnt,sep);
					cnt+=increment;
				}
				break;
			case 'b':
				circf = bsave;
				if (step(p,bexpbuf)) {
					pnum(cnt,sep);
					cnt+=increment;
				}
				else {
					npad(width,sep);
				}
				break;
			case 'h':
				circf = hsave;
				if (step(p,hexpbuf)) {
					pnum(cnt,sep);
					cnt+=increment;
				}
				else {
					npad(width,sep);
				}
				break;
			case 'f':
				circf = fsave;
				if (step(p,fexpbuf)) {
					pnum(cnt,sep);
					cnt+=increment;
				}
				else {
					npad(width,sep);
				}
				break;
		}
		fprintf(optr,"%s",line);

	}	/* Closing brace of "else" (~ line 307). */
	}	/* Closing brace of "while". */
	fclose(iptr);
	exit(0);
	/* NOTREACHED */
}

/*		REGEXP ERR ROUTINE		*/

regerr(c)
int c;
{
printf("%d This is the error code\n",c);
printf("Illegal Regular Expression\n");
exit(1);
}

/*		CALCULATE NUMBER ROUTINE	*/

pnum(n,sep)
int	n;
char *	sep;
{
	register int	i;

		if (format == 'z') {
			pad = '0';
		}
	for ( i = 0; i < width; i++)
		nbuf[i] = pad;
		num(n,width - 1);
	if (format == 'l') {
		while (nbuf[0]==' ') {
			for ( i = 0; i < width; i++)
				nbuf[i] = nbuf[i+1];
			nbuf[width-1] = ' ';
		}
	}
		printf("%s%s",nbuf,sep);
}

/*		IF NUM > 10, THEN USE THIS CALCULATE ROUTINE		*/

num(v,p)
int v,p;
{
	if (v < 10)
		nbuf[p] = v + '0' ;
	else {
		nbuf[p] = (v % 10) + '0' ;
		if (p>0) num(v / 10,p - 1);
	}
}

/*		CONVERT ARG STRINGS TO STRING ARRAYS	*/

convert(argv)
char **argv;
{
	s = (char*)argv;
	q=2;
	r=0;
	while (s[q] != '\0') {
		if (s[q] >= '0' && s[q] <= '9')
		{
		s1[r] = s[q];
		r++;
		q++;
		}
		else
				{
				optmsg(argv);
				}
	}
	s1[r] = '\0';
	k = atoi(s1);
	return(k);
}

/*		CALCULATE NUM/TEXT SEPRATOR		*/

npad(width,sep)
	int	width;
	char *	sep;
{
	register int i;

	pad = ' ';
	for ( i = 0; i < width; i++)
		nbuf[i] = pad;
	printf("%s",nbuf);

	for(i=0; i < strlen(sep); i++)
		printf(" ");
}
/* ------------------------------------------------------------- */
optmsg(option)
char *option;
{
	printf("INVALID OPTION (%s) - PROCESSING TERMINATED\n",option);
	exit(1);
}
