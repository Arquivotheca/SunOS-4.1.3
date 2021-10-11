#ifndef lint
static	char sccsid[] = "@(#)userexit.c 1.1 92/07/30 SMI"; /* from System III 3.1 */
#endif

/*
	Default userexit routine for fatal and setsig.
	User supplied userexit routines can be used for logging.
*/

userexit(code)
{
	return(code);
}
