/*	@(#)sgetl.c 1.1 92/07/30 SMI	*/
/*
 * Provide machine independent transfer of longs.
 */

/*
 * The intent here is to provide a means to make the value of
 * bytes in an io-buffer correspond to the value of a long
 * in the memory while doing the io a `long' at a time.
 * Files written and read in this way are machine-independent.
 *
 */
#include <values.h>

long  sgetl(buffer)
register char *buffer;
{
	 register long l = 0;
	 register int i = BITSPERBYTE * sizeof(long);

	 while ((i -= BITSPERBYTE) >= 0)
			 l |= (short) ((unsigned char) *buffer++) << i;
	 return  l;
 }

