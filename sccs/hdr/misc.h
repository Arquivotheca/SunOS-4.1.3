/*	@(#)misc.h 1.1 92/07/30 SMI; from System III	*/
/*
 * structure to access an
 * integer in bytes
 */
struct
{
	char	hibyte;
	char	lobyte;
};

/*
 * structure to access an integer
 */
struct
{
	int	integ;
};

/*
 * structure to access a long as integers
 */
struct {
	short	hiword;
	short	loword;
};

/*
 *	structure to access an unsigned
 */
struct {
	unsigned	unsignd;
};
