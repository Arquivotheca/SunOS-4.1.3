#ifndef lint
static	char sccsid[] = "@(#)conf.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/acct.h>
#include <sys/stream.h>

extern int nulldev();
extern int nodev();

extern int simdopen(), simdstrategy(), simdsize();
extern int dumpread(), dumpwrite();

struct bdevsw	bdevsw[] =
{
	{ nodev, nodev, nodev, nodev, nodev, 0},			/*0*/
	{ simdopen,	nulldev,	simdstrategy,	nodev,		/*1*/
	  simdsize,	0 },
};
int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

extern int cnopen(), cnclose(), cnread(), cnwrite(), cnioctl(), cnselect();

extern int syopen(), syread(), sywrite(), syioctl(), syselect();

extern int mmopen(), mmread(), mmwrite(), mmmmap();
#define mmselect	seltrue

extern int swread(), swwrite();

extern struct streamtab simcstab;

extern int logopen(), logclose(), logread(), logioctl(), logselect();

#include "pty.h"
#if NPTY > 0
extern struct streamtab ptsinfo;
extern int ptcopen(), ptcclose(), ptcread(), ptcwrite(), ptcioctl();
extern int ptcselect();
#define ptstab  &ptsinfo
#else
#define ptstab  0
#define ptcopen         nodev
#define ptcclose        nodev
#define ptcread         nodev
#define ptcwrite        nodev
#define ptcioctl        nodev
#define ptcselect       nodev
#endif

#include "clone.h"
#if NCLONE > 0
extern int cloneopen();
#else
#define cloneopen       nodev
#endif

extern int seltrue();

struct cdevsw	cdevsw[] =
{
    {
	cnopen,		cnclose,	cnread,		cnwrite,	/*0*/
	cnioctl,	nulldev,	cnselect,	0,
	0,
    },
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*1*/
    {
	syopen,		nulldev,	syread,		sywrite,	/*2*/
	syioctl,	nulldev,	syselect,	0,
	0,
    },
    {
	mmopen,		nulldev,	mmread,		mmwrite,	/*3*/
	nodev,		nulldev,	mmselect,	mmmmap,
	0,
    },
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*4*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*5*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*6*/
    {
	nulldev,	nulldev,	swread,		swwrite,	/*7*/
	nodev,		nulldev,	nodev,		0,
	0,
    },
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*8*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*9*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*10*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*11*/
    {
	nodev,		nodev,		nodev,		nodev,		/*12*/
	nodev,		nodev,		nodev,		0,
	&simcstab,
    },
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*13*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*14*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*15*/
    {
	logopen,	logclose,	logread,	nodev,		/*16*/
	logioctl,	nulldev,	logselect,	0,
	0,
    },
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*17*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*18*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*19*/
    {
	nodev,		nodev,		nodev,		nodev,		/*20*/
	nodev,		nodev,		nodev,		0,
	ptstab,
    },
    {
	ptcopen,	ptcclose,	ptcread,	ptcwrite,	/*21*/
	ptcioctl,	nulldev,	ptcselect,	0,
	0,
    },
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*22*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*23*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*24*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*25*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*26*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*27*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*28*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*29*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*30*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*31*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*32*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*33*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*34*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*35*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*36*/
    {
	cloneopen,	nodev,		nodev,		nodev,		/*37*/
	nodev,		nodev,		nodev,		0,
	0,
    },
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*38*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*39*/
    { nodev, nodev, nodev, nodev, nodev, nodev, nodev, 0, 0,},		/*40*/
    {
	nulldev,	nulldev,	dumpread,	dumpwrite,	/*41*/
	nodev,		nulldev,	nodev,		0,
	0,
    },
};
int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

int	mem_no = 3;	/* major device number of memory special file */
