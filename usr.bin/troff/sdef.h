/*	@(#)sdef.h 1.1 92/07/30 SMI; from UCB 4.1 06/07/82	*/

struct s {
	int nargs;
	struct s *pframe;
	filep pip;
	int pnchar;
	int prchar;
	int ppendt;
	int *pap;
	int *pcp;
	int pch0;
	int pch;
	};
