#ifndef lint
static	char sccsid[] = "@(#)fakcu.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*LINTLIBRARY*/
/*
 *       This is a dummy _cleanup routine to place at the end
 *       of the C library in case no other definition is found.
 *       If the standard I/O routines are used, they supply a
 *       real "_cleanup"  routine in file flsbuf.c
 */

void
_cleanup()
{
}
