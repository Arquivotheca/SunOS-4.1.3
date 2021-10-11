#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)setegid.c 1.1 92/07/30 SMI"; /* from UCB 4.1 83/06/30 */
#endif

setegid(egid)
	int egid;
{

	return (setregid(-1, egid));
}
