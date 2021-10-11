/*	@(#)sputl.c 1.1 92/07/30 SMI	*/
/*
 * Provide machine independent transfer of longs.
 */

/*
 * The intent here is to provide a means to make the value of
 * bytes in an io-stream correspond to the value of the long
 * in the memory while doing the io a `long' at a time.
 * Files written and read in this way are machine-independent.
 *
 */
#include <values.h>

void sputl(l, buffer)
register long l;
register char *buffer;
{
      register int i = BITSPERBYTE * sizeof(long);

      while ((i -= BITSPERBYTE) >= 0)
	      *buffer++ = (char) (l >> i);
}

