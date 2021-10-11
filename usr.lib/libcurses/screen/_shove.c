#ifndef lint
static	char sccsid[] = "@(#)_shove.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * Shove right in body as much as necessary.
 * Note that we give the space the same attributes as the upcoming
 * character, to force the cookie to be placed on the space.
 */
/* ARGSUSED */
_shove(body, len, lno)
register chtype *body;
register int len, lno;
{
	register int j, k, prev = 0;
	register int curscol = SP->virt_x, cursincr = 0, shoved = 0;
	static chtype buf[256];

#ifdef DEBUG
	if(outf) fprintf(outf, "_shove('");
	_prstr(body, len);
	if(outf) fprintf(outf, "', %d, %d), SP->virt_x %d\n", len, lno, SP->virt_x);
#endif
	for (j=0, k=0; j<len; ) {
		if ((body[j]&A_ATTRIBUTES) != prev) {
			shoved++;
			if ((body[j]&A_CHARTEXT) == ' ') {
				/* Using an existing space */
				buf[j] = ' ' | body[j+1]&A_ATTRIBUTES;
			} else if ((body[j-1]&A_CHARTEXT) == ' ') {
				/* Using previous existing space */
				buf[j-1] = ' ' | body[j]&A_ATTRIBUTES;
			} else {
				/* A space is inserted here. */
				buf[k++] = ' ' | body[j]&A_ATTRIBUTES;
				if (j < curscol)
					cursincr++;
			}
		}
#ifdef DEBUG
		if(outf) fprintf(outf, "j %d, k %d, prev %o, new %o\n",
			j, k, prev, body[j] & A_ATTRIBUTES);
#endif
		prev = body[j] & A_ATTRIBUTES;
		buf[k++] = body[j++];
	}
	if (shoved) {
		/* k is 1 more than the last column of the line */
		if (k > columns)
			k = columns;
		if (buf[k-1]&A_ATTRIBUTES) {
			if (k < columns)
				k++;
			buf[k-1] = ' ';	/* All attributes off */
		}
		for (j=0; j<k; j++)
			body[j] = buf[j];
		len = k;
	}
	if (cursincr && lno == SP->virt_y+1)
		SP->virt_x += cursincr;
#ifdef DEBUG
	if(outf) fprintf(outf, "returns '");
	_prstr(body, len);
	if(outf) fprintf(outf, "', len %d, SP->virt_x %d\n", len, SP->virt_x);
#endif
	return len;
}

#ifdef DEBUG
static
_prstr(result, len)
chtype *result;
int len;
{
	register chtype *cp;

	for (cp=result; *cp && cp < result+len; cp++)
		if (*cp >= ' ' && *cp <= '~') {
			if(outf) fprintf(outf, "%c", *cp);
		} else {
			if(outf) fprintf(outf, "<%o,%c>",
				*cp&A_ATTRIBUTES, *cp&A_CHARTEXT);
		}
}
#endif
