/*	@(#)two.twostr.c 1.1 92/07/30 SMI	*/
/* Make a 2 letter code into an integer we can switch on easily */
#define	two( s1, s2 )	(s1 + 256 * s2 )
#define	twostr( str )	two( *str, str[ 1 ] )
