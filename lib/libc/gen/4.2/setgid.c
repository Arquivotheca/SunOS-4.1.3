#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)setgid.c 1.1 92/07/30 SMI"; /* from UCB 4.1 83/06/30 */
#endif

/*
 * Backwards compatible setgid.
 */
setgid(gid)
	int gid;
{

	return (setregid(gid, gid));
}
