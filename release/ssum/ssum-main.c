#include <stdio.h>
#include <sys/param.h>
#include <ctype.h>
#include "ssum.h"

#ifdef DEBUG
int	dumpser = 0,
	mflag = 0,
	debug = 0;
FILE	*dumpfile;
#endif DEBUG

char	*calloc(),
	*sprintf(),
	*sccsname();

char	*myname;
extern int errno,
	abortflag;
int	absline;	/* absolute line number */

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	errors = 0;


	abortflag = 0;
	myname = *argv++;
	--argc;

	if( !argc )
	{
		(void)fprintf(stderr, "%s: no path specified\n", myname);
		exit(255);
	}

	while(argc--)
	{
		if( strcmp(*argv, "-") == 0 )
		{
			char	ifname[MAXPATHLEN];

			while( gets(ifname) != NULL )
				if( !do_file(ifname) )
					exit(errno == 0 ? 255 : errno);
		}
		else if( **argv == '-' )
			setoptions(*argv++);
		else
			if( !do_file(*argv++) )
				++errors;
	}
	exit(errors);
}

void
printsums(array, fname)
	delta_t	**array;
	char	*fname;
{
	delta_t	*dptr;
	int	i,
		maxdelta;

	if( !array )
		return;

	/* print everything out */
	maxdelta = (long)array[0];
	(void)fflush(stderr);
	(void)fflush(stdout);
	for( i = 1; i <= maxdelta; ++i )
	{
		if( (dptr = array[i]) == NULL )
			continue;

		(void)printf("%05u\t%d\t%s\t%s\n",
			dptr->sum, dptr->bytes,
			sidstr(&array[i]->sid), fname);

		if( dptr->lines == dptr->maxlines )
			continue;
		(void)fflush(stdout);
		(void)fprintf(stderr,
			"%s: warning: line count for %s rev %s, expected %d, counted %d.\n",
			myname, fname, sidstr(&array[i]->sid),
			dptr->maxlines, dptr->lines);
	}
}

/* for now we will assume that we are getting s.name */
do_file(fname)
	char	*fname;
{
	FILE	*file;
	delta_t	*tree = NULL;
	register delta_t	**array;
	char	*sname;
	int	r = 1;

	if(	(file = fopen(sname=sccsname((char *)NULL, fname, 1), "r")) == NULL
	     && (file = fopen(sname=sccsname((char *)NULL, fname, 0), "r")) == NULL )
	{
		(void)fprintf(stderr, "%s: cannot open %s\n", myname, sname);
		perror(myname);
		return(0);
	}

	/* read all sccs delta headers */
	array = (delta_t **)0;
	if((tree = readhead(file, sname)) && (array = buildtree(&tree, sname)))
	{
		(void)readtext(file, tree, sname, 0, array);
		printsums(array, sname);
		freearray(array);
	}
	else
		r = 0;
	(void)fclose(file);
	return(r);
}

setoptions(string)
	register char	*string;
{
	++string;	/* skip '-' */
	while( *string )
	{
		switch( *string++ )
		{
#ifdef DEBUG
		case 'd':
			++debug;
			break;
		case 'm':
			++mflag;
			break;
		case 'a':
			++abortflag;
			break;
		case 's':
			dumpser = atoi(string);
			string = "";
			if( dumpser > 0 )
				dumpfile = fopen("dumpfile", "w");
			break;
#endif DEBUG
		default:
#ifdef DEBUG
			(void)fprintf(stderr, "%s: invalid option %s\n",
				myname, --string);
			(void)fprintf(stderr,
				"Usage: %s [-d] [-m] [-s#] [-] [files...]\n",
				myname);
#else DEBUG
			(void)fprintf(stderr, "%s: no options supported\n",
				myname);
#endif DEBUG
			exit(255);
			break;
		}
	}
}

nomem()
{
	(void)fprintf(stderr, "%s: out of memory\n", myname);
	exit(255);
}
