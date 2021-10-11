#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)bzero.c 1.1 92/07/30 SMI";
#endif

/*
 * Set an array of n chars starting at sp to zero.
 */
void
bzero(sp, len)
	register char *sp;
	int len;
{
	register int n;

	if ((n = len) <= 0)
		return;
	do
		*sp++ = 0;
	while (--n);
}
