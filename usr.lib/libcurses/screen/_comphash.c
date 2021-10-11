#ifndef lint
static	char sccsid[] = "@(#)_comphash.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

_comphash (p)
register struct line   *p;
{
	register chtype  *c, *l;
	register int rc;

	if (p == NULL)
		return;
	if (p -> hash)
		return;
	rc = 1;
	c = p -> body;
	l = &p -> body[p -> length];
	while (--l > c && *l == ' ')
		;
	while (c <= l && *c == ' ')
		c++;
	p -> bodylen = l - c + 1;
	while (c <= l)
		rc = (rc<<1) + *c++;
	p -> hash = rc;
	if (p->hash == 0)
		p->hash = 123;
}
