/*	@(#)tutor.h 1.1 92/07/30 SMI; from UCB 4.1 82/05/11	*/

struct situatn  {
	int	brd[26];
	int	roll1;
	int	roll2;
	int	mp[4];
	int	mg[4];
	int	new1;
	int	new2;
	char	*(*com[8]);
};
