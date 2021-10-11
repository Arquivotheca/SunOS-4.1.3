#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)setruid.c 1.1 92/07/30 SMI"; /* from UCB 4.1 83/06/30 */
#endif

setruid(ruid)
	int ruid;
{

	return (setreuid(ruid, -1));
}
