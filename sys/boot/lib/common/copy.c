/* 
 * @(#)copy.c 1.1 92/07/30 Copyr 1985 Sun Micro
 * Byte zero and copy routines
 */

bzero(p, n)
	register char *p;
	register int n;
{
	register char zeero = 0;

	while (n > 0)
		*p++ = zeero, n--;	/* Avoid clr for 68000, still... */
}

bcopy(src, dest, count)
	register char *src, *dest;
	register int count;
{
	while (--count != -1)
		*dest++ = *src++;
}
