#ifndef lint
static	char sccsid[] = "@(#)label.c 1.1 92/07/30 SMI"; /* from UCB 4.1 6/27/83 */
#endif

#include <stdio.h>
label(s)
char *s;
{
	int i;
	putc('t',stdout);
	for(i=0;s[i];)putc(s[i++],stdout);
	putc('\n',stdout);
}
