#ifndef lint
static	char sccsid[] = "@(#)prtmem.c 1.1 92/08/05 SMI"; /* from S5R2 1.2 03/28/83 */
#endif
 
#ifdef GETU
#define udsize uu[0]
#define ussize uu[1]
#endif


prtmem()
{
#ifdef GETU
#include "stdio.h"
#include "sys/param.h"
#include "sys/dir.h"
#include "sys/user.h"
	unsigned uu[2];
	register int i;

	if(getu( &((struct user *)0)->u_dsize, &uu, sizeof uu) > 0)
	{
		udsize *= 64;
		ussize *= 64;
		(void)printf("mem: data = %u(0%o) stack = %u(0%o)\n",
			udsize, udsize, ussize, ussize);
/*
 *	The following works only when `make' is compiled
 *	with I&D space separated (i.e. cc -i ...).
 *	(Notice the hard coded `65' below!)
 */
		udsize /= 1000;
		ussize /= 1000;
		(void)printf("mem:");
		for(i=1; i<=udsize;i++)
		{
			if((i%10) == 0)
				(void)printf("___");
			(void)printf("d");
		}
		for(;i<=65-ussize;i++)
		{
			if((i%10) == 0)
				(void)printf("___");
			(void)printf(".");
		}
		for(;i<=65;i++)
		{
			if((i%10) == 0)
				(void)printf("___");
			(void)printf("s");
		}
		(void)printf("\n");
		(void)fflush(stdout);
	}
#endif
}
