#include <stdio.h>

static char *sccsid = "@(#)relpath.c	1.1";

extern char
	**pathopt(),
	**split();

int many = 0;

char *
massagepath(from, to)
	char	*from;
	char	*to;
{
	register char	**fromv,
			**tov;
	register char	**fv,
			**tv;
	static char	newpath[1024];

	if( *from != '/' || *to != '/' )
		return(from);
	
	fv = fromv = pathopt(split(from));
	tv = tov = pathopt(split(to));

	while( *fv && *tv && strcmp(*fv, *tv) == 0 )
	{
		++fv;
		++tv;
	}
	fromv = fv;
	tov = tv;
	
	newpath[0] = '\0';

	while( *tv && (many || *(tv+1)) )
	{
		strcat(newpath, "../");
		++tv;
	}
	while( *fv )
	{
		strcat(newpath, *fv++ );
		if( *fv )
			strcat(newpath, "/");
	}
	return(newpath);
}

main(argc, argv)
	int	argc;
	char	**argv;
{
	register int	i,last;

	++argv, --argc;

	if( argc < 2 )
	{
		fprintf("relpath: need more arguments\n");
		exit(-1);
	}
	if( argc > 2 )
		many = 1; /* should be based on argv[last] being a dir */

	last = argc-1;
	i = 0;
	while( i < last )
		printf("%s ", massagepath(argv[i++], argv[last]));
	puts(argv[last]);
}

extern int errno;

nomem()
{
	perror("cannot malloc");
	exit(errno);
}
