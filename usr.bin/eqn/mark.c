#ifndef lint
static	char sccsid[] = "@(#)mark.c 1.1 92/07/30 SMI"; /* from UCB 4.2 83/08/11 */
#endif

#include "e.h"

mark(p1) int p1; {
	markline = 1;
	printf(".ds %d \\k(97\\*(%d\n", p1, p1);
	yyval = p1;
	if(dbg)printf(".\tmark %d\n", p1);
}

lineup(p1) {
	markline = 1;
	if (p1 == 0) {
		yyval = oalloc();
		printf(".ds %d \\h'|\\n(97u'\n", yyval);
	}
	if(dbg)printf(".\tlineup %d\n", p1);
}
