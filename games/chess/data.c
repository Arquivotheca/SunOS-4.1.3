#ifndef lint
static  char sccsid[] = "%Z%%M% %I% %E% SMI";
#endif

short	center[] =
{
	2,3,4,4,4,4,3,2,
	3,6,8,8,8,8,6,3,
	4,8,12,12,12,12,8,4,
	4,8,12,14,14,12,8,4,
	4,8,12,14,14,12,8,4,
	4,8,12,12,12,12,8,4,
	3,6,8,8,8,8,6,3,
	2,3,4,4,4,4,3,2
};

short wheur1(), wheur2(), wheur3(), wheur4(), wheur5(), wheur6();

short	(*wheur[])() =
{
	wheur1,
	wheur2,
	wheur3,
	wheur4,
	wheur5,
	wheur6,
	0
};

short bheur1(), bheur2(), bheur3(), bheur4(), bheur5(), bheur6();

short	(*bheur[])() =
{
	bheur1,
	bheur2,
	bheur3,
	bheur4,
	bheur5,
	bheur6,
	0
};

short	ipval[] =
{
	-3000, -900, -500, -300, -300, -100,
	0,
	100, 300, 300, 500, 900, 3000
};

short	moveno	= 1;
short	depth	= 2;
short	qdepth	= 8;
short	mdepth	= 4;
short	flag	= 033;
short	eppos	= 64;
short	bkpos	= 4;
short	wkpos	= 60;
short	edge[] =
{
	040, 020, 010, 0, 0, 1, 2, 4
};
short	board[]=
{
	4, 2, 3, 5, 6, 3, 2, 4,
	1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-4, -2, -3, -5, -6, -3, -2, -4,
};
