#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)valloc.c 1.1 92/07/30 SMI"; /* from UCB 4.3 83/07/01 */
#endif

extern	unsigned getpagesize();
extern	char	*memalign();

char *
valloc(size)
	unsigned size;
{
	static unsigned pagesize;
	if (!pagesize)
		pagesize = getpagesize();
	return memalign(pagesize, size);
}
