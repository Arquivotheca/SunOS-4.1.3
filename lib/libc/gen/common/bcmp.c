#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)bcmp.c 1.1 92/07/30 SMI"; /* from UCB X.X XX/XX/XX */
#endif

bcmp(s1, s2, len)
	register char *s1, *s2;
	register int len;
{

	while (len--)
		if (*s1++ != *s2++)
			return (1);
	return (0);
}
