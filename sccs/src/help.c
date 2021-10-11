# include	"../hdr/defines.h"

SCCSID(@(#)help.c 1.1 92/07/30 SMI); /* from System III 5.1 */

/*
	Program to locate helpful info in an ascii file.
	The program accepts a variable number of arguments.

	The file to be searched is determined from the argument. If the
	argument does not contain numerics, the search
	will be attempted on '/usr/sccs/helpdir/cmds', with the search key
	being the whole argument.
	If the argument begins with non-numerics but contains
	numerics (e.g, zz32) the search will be attempted on
	'/usr/sccs/helpdir/<non-numeric prefix>', (e.g,/usr/lib/help/zz),
	with the search key being <remainder of arg>, (e.g., 32).
	If the argument is all numeric, or if the file as
	determined above does not exist, the search will be attempted on
	'/usr/sccs/helpdir/default' with the search key being
	the entire argument.
	In no case will more than one search per argument be performed.

	File is formatted as follows:

		* comment
		* comment
		-str1
		text
		-str2
		text
		* comment
		text
		-str3
		text

	The "str?" that matches the key is found and
	the following text lines are printed.
	Comments are ignored.

	If the argument is omitted, the program requests it.
*/
char	dftfile[]   =   "/usr/sccs/helpdir/default";
char	helpdir[]   =   "/usr/sccs/helpdir/";
char	hfile[64];
FILE	*iop;
char	line [LINESIZE];


main(argc,argv)
int argc;
char *argv[];
{
	register int i;
	extern int Fcnt;

	/*
	Tell 'fatal' to issue messages, clean up, and return to its caller.
	*/
	Fflags = FTLMSG | FTLCLN | FTLJMP;

	if (argc == 1)
		findprt(ask());		/* ask user for argument */
	else
		for (i = 1; i < argc; i++)
			findprt(argv[i]);

	exit(Fcnt ? 1 : 0);
	/* NOTREACHED */
}


findprt(p)
char *p;
{
	register char *q;
	char key[50];
	FILE *fdfopen();

	if (setjmp(Fjmp))		/* set up to return here from */
		return;			/* 'fatal' and return to 'main' */

	if (size(p) > 50)
		fatal("argument too long (he2)");

	q = p;

	while (*q && !numeric(*q))
		q++;

	if (*q == '\0') {		/* all alphabetics */
		copy(p,key);
		cat(hfile,helpdir,"cmds",0);
		if (!exists(hfile))
			copy(dftfile,hfile);
	}
	else
		if (q == p) {		/* first char numeric */
			copy(p,key);
			copy(dftfile,hfile);
		}
	else {				/* first char alpha, then numeric */
		copy(p,key);		/* key used as temporary */
		*(key + (q - p)) = '\0';
		cat(hfile,helpdir,key,0);
		copy(q,key);
		if (!exists(hfile)) {
			copy(p,key);
			copy(dftfile,hfile);
		}
	}

	iop = xfopen(hfile,0);

	/*
	Now read file, looking for key.
	*/
	while ((q = fgets(line,sizeof(line),iop)) != NULL) {
		repl(line,'\n','\0');		/* replace newline char */
		if (line[0] == '-' && equal(&line[1],key))
			break;
	}

	if (q == NULL) {	/* endfile? */
		printf("\n");
		sprintf(Error,"%s not found (he1)",p);
		fatal(Error);
	}

	printf("\n%s:\n",p);

	while (fgets(line,sizeof(line),iop) != NULL && line[0] == '-')
		;
	do {
		if (line[0] != '*')
			printf("%s",line);
	} while (fgets(line,sizeof(line),iop) != NULL && line[0] != '-');

	fclose(iop);
}


ask()
{
	static char resp[51];

	iop = stdin;

	printf("msg number or comd name? ");
	fgets(resp,51,iop);
	return(repl(resp,'\n','\0'));
}


clean_up()
{
	fclose(iop);
}
