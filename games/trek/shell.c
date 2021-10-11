#ifndef lint
static	char sccsid[] = "@(#)shell.c 1.1 92/07/30 SMI"; /* from UCB 4.2 05/09/83 */
#endif

/*
**  CALL THE SHELL
*/

shell()
{
	int		i;
	register int	pid;
	register int	sav2, sav3;

	if (!(pid = fork()))
	{
		setuid(getuid());
		nice(0);
		execl("/bin/csh", "-", 0);
		syserr("cannot execute /bin/csh");
	}
	sav2 = signal(2, 1);
	sav3 = signal(3, 1);
	while (wait(&i) != pid) ;
	signal(2, sav2);
	signal(3, sav3);
}
