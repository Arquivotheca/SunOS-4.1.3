#if !defined(lint) && defined(SCCSIDS) 
static  char sccsid[] = "@(#)execv.c 1.1 92/07/30 SMI"; 
#endif
/*
 *	execv(file, argv)
 *
 *	where argv is a vector argv[0] ... argv[x], NULL
 *	last vector element must be NULL
 *	environment passed automatically
 */

execv(file, argv)
	char	*file;
	char	**argv;
{
	extern	char	**environ;

	return(execve(file, argv, environ));
}
