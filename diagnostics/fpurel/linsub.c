/*
 *	@(#)linsub.c 1.1 7/30/92  Copyright Sun Microsystems
 *
 *	linsub.c:  stub program to replace linsub.F incase compile 
 *		   machine has no Fortran compiler.
 */

#ifdef S
slinsub()
{
	return(0);
}
#endif

#ifdef D
dlinsub()
{
	return(0);
}
#endif
