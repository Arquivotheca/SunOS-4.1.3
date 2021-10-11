#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)ntfy_fd_op.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif
#include <sys/types.h>

/* AND op on two fds */
int
ntfy_fd_cmp_and(a, b)
fd_set *a, *b;
{
	register int i;

	for( i = 0; i < howmany(FD_SETSIZE, NFDBITS) ; i++) 
		if (a->fds_bits[i] & b->fds_bits[i]) return(1);
	return(0);
}


/* OR op on two fds */
int
ntfy_fd_cmp_or(a, b)
fd_set *a, *b;
{
	register int i;

	for( i = 0; i < howmany(FD_SETSIZE, NFDBITS) ; i++) 
		if (a->fds_bits[i] | b->fds_bits[i]) return(1);
	return(0);
}


/* Are any of the bits set */
int
ntfy_fd_anyset(a)
fd_set *a;
{
	register int i;

	for( i = 0; i < howmany(FD_SETSIZE, NFDBITS) ; i++) 
		if (a->fds_bits[i]) return(1);
	return(0);
}


/* Return OR of two fd's */
fd_set
*ntfy_fd_cpy_or(a, b)
fd_set *a, *b;
{
	register int i;

	for( i = 0; i < howmany(FD_SETSIZE, NFDBITS) ; i++) 
		a->fds_bits[i] =  a->fds_bits[i] | b->fds_bits[i];
	return(a);
}


/* Return AND of two fd's */
fd_set
*ntfy_fd_cpy_and(a, b)
fd_set *a, *b;
{
	register int i;

	for( i = 0; i < howmany(FD_SETSIZE, NFDBITS) ; i++) 
		a->fds_bits[i] =  a->fds_bits[i] & b->fds_bits[i];
	return(a);
}




/* Return XOR of two fd's */
fd_set
*ntfy_fd_cpy_xor(a, b)
fd_set *a, *b;
{
	register int i;

	for( i = 0; i < howmany(FD_SETSIZE, NFDBITS) ; i++) 
		a->fds_bits[i] =  a->fds_bits[i] ^ b->fds_bits[i];
	return(a);
}
