#ifndef lint
static	char sccsid[] = "@(#)strend.c 1.1 92/07/30 SMI"; /* from System III 3.1 */
#endif

char *strend(p)
register char *p;
{
	while (*p++)
		;
	return(--p);
}
