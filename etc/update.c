#ifndef lint
static	char sccsid[] = "@(#)update.c 1.1 92/07/30 SMI"; /* from UCB 4.2 10/16/80 */
#endif

/*
 * Update the file system every 30 seconds.
 */

#include <signal.h>

main()
{
	if(fork())
		exit(0);
	close(0);
	close(1);
	close(2);
	dosync();
	for(;;)
		pause();
}

dosync()
{
	sync();
	signal(SIGALRM, dosync);
	alarm(30);
}

/*
 * The standard exit routine from libc calls _cleanup
 * which drags in a lot of stdio code we don't need,
 * so we have this special exit.
 */
exit(n)
	int n;
{
	_exit(n);
}
