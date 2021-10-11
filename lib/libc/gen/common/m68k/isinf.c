#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)isinf.c 1.1 92/07/30 SMI"; /* from UCB X.X XX/XX/XX */
#endif

/*
 * Recognize an infinity or a NaN when one is presented.
 * This is for keeping various IO routines out of trouble 
 */


int
isinf( d0, d1 )
    unsigned d0,d1;
    /* a lie -- actually its a ``double'' */
{
    if (d1 != 0 ) return 0; /* nope -- low-order must be all zeros */
    if (d0 != 0x7ff00000 && d0 != 0xfff00000) return 0; /* nope */
    return 1;
}

int
isnan( d0,d1 )
    unsigned d0,d1;
    /* a lie -- actually its a ``double'' */
{
#define EXPONENT 0x7ff00000
#define SIGN     0x80000000
    if ((d0 & EXPONENT) != EXPONENT ) return 0; /* exponent wrong */
    if ((d0 & ~(EXPONENT|SIGN)) == 0 && d1 == 0 ) return 0; /* must have bits */
    return 1;
}
