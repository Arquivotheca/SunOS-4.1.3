#include <stdio.h>

char	*sccsname();

int	sccsflag = 1;

main(argc, argv)
	int	argc;
	char	**argv;
{
	++argv, --argc;

	while( argc-- )
	{
		if( strcmp("-s", *argv) == 0 )
			sccsflag = 1;
		else if( strcmp("-r", *argv) == 0 )
			sccsflag = 0;
		else
			(void)printf("%s\n", sccsname(NULL, *argv, sccsflag));
		++argv;
	}
	return(0);
}

nomem()
{
	extern int errno;
	
	perror("out of memory");
	exit(errno);
}
