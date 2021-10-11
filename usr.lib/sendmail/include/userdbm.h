/*
**  USERDBM.H -- user-level definitions for the DBM library
**
**	Version:
**		@(#)userdbm.h	1.1	92/07/30	SMI; from UCB 3.1 10/13/82
*/

typedef struct
{
	char	*dptr;
	int	dsize;
} DATUM;

extern DATUM fetch();
