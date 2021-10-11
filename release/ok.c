#include <stdio.h>
main(argc,argv)
int	argc;
char	**argv;
{
	char line[80];
	register char	*p;
	int	all = 0;

	setlinebuf(stdout);
	setlinebuf(stderr);

	while( --argc )
	{
		fprintf(stderr, "%s%s", *++argv, all ? "\n" : "? ");
		if( !all && fgets(p = line, sizeof(line), stdin) == NULL )
			break;
		for( ; *p && ( *p == ' ' || *p == '\t' ) ; ++p )
			continue;
		if( *p == 'q' || *p == 'Q' )
			break;
		if( *p == 'a' || *p == 'A' )
			all = 1;
		if( all )
			p = "y";
		if( *p == 'y' || *p == 'Y' )
			puts(*argv);
	}
}
