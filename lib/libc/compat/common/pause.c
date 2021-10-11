#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)pause.c 1.1 92/07/30 SMI"; /* from UCB 4.1 83/06/09 */
#endif

/*
 * Backwards compatible pause.
 */
pause()
{

	sigpause(sigblock(0));
	return (-1);
}
