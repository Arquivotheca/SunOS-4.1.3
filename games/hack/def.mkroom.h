/*	@(#)def.mkroom.h 1.1 92/07/30 SMI	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* def.mkroom.h - version 1.0.2 */

struct mkroom {
	xchar lx,hx,ly,hy;
	schar rtype,rlit,doorct,fdoor;
};

#define	MAXNROFROOMS	15
extern struct mkroom rooms[MAXNROFROOMS+1];

#define	DOORMAX	100
extern coord doors[DOORMAX];

/* values values of rtype */
/* 0: ordinary room; 8-15: various shops */
/* Note: some code assumes that >= 8 means shop, so be careful when adding
   new roomtypes */
#define	SWAMP	3
#define	VAULT	4
#define	BEEHIVE	5
#define	MORGUE	6
#define	ZOO	7
#define	SHOPBASE	8
#define	WANDSHOP	9
#define	GENERAL	15
