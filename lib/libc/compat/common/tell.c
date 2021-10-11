#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)tell.c 1.1 92/07/30 SMI"; /* from UCB 4.1 80/12/21 */
#endif

/*
 * return offset in file.
 */

long	lseek();

long tell(f)
{
	return(lseek(f, 0L, 1));
}
