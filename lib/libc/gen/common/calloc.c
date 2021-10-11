#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)calloc.c 1.1 92/07/30 SMI"; /* from UCB 4.1 80/12/21 */
#endif

/*
 * calloc - allocate and clear memory block
 */
#define CHARPERINT (sizeof(int)/sizeof(char))
#define NULL 0
#ifdef	S5EMUL
#define	ptr_t	void*
#else
#define	ptr_t	char*
#endif

ptr_t
calloc(num, size)
	unsigned num, size;
{
	register ptr_t mp;
	ptr_t	malloc();

	num *= size;
	mp = malloc(num);
	if (mp == NULL)
		return(NULL);
	bzero(mp, num);
	return ((ptr_t)(mp));
}

cfree(p, num, size)
	ptr_t p;
	unsigned num, size;
{
	free(p);
}
