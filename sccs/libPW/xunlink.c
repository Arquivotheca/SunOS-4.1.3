#ifndef lint
static	char sccsid[] = "@(#)xunlink.c 1.1 92/07/30 SMI"; /* from System III 3.1 */
#endif

/*
	Interface to unlink(II) which handles all error conditions.
	Returns 0 on success,
	fatal() on failure.
*/

xunlink(f)
{
	if (unlink(f))
		return(xmsg(f,"xunlink"));
	return(0);
}
