#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)bcopy.c 1.1 92/07/30 SMI";
#endif

/*
 * Copy s1 to s2, always copy n bytes.
 * For overlapped copies it does the right thing.
 */
void
bcopy(s1, s2, len)
	register char *s1, *s2;
	int len;
{
	register int n;

	if ((n = len) <= 0)
		return;

	if ((s1 < s2) && (n > abs(s1 - s2))) {		/* overlapped */
		s1 += (n - 1);
		s2 += (n - 1);
		do
			*s2-- = *s1--;
		while (--n);
	} else {					/* normal */
		 do
			*s2++ = *s1++;
		while (--n);
	}
}
