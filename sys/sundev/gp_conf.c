#ifndef lint
static char sccsid[] = "@(#)gp_conf.c	1.1 92/07/30 SMI";
#endif

#include "gpone.h"
#include "cgnine.h"
#include "cgtwo.h"
#include <sys/types.h>
#include <sys/buf.h>
#include <sys/param.h>
#include <sundev/mbvar.h>

#ifdef lint
#define	Min(a, b)	((a) + (b))
#define	Max(a, b)	((a) + (b))
#else lint
#define	Min(a, b)	((a) <= (b) ? (a) : (b))
#define	Max(a, b)	((a) >= (b) ? (a) : (b))
#endif lint

#define NCG (NCGTWO + NCGNINE)
#define NFBMAX		Min(NCG, 4)

int             gp1_nfbmax = NFBMAX;
int             gp1_ncg = NCG;
dev_t		gp1_cgfb[NFBMAX];

int
gp1_findfb(fbaddr, cgdev)
    caddr_t         fbaddr;
    dev_t          *cgdev;
{
#if NGPONE > 0
#if NCGNINE > 0
#define CG9_MAJOR 68
    {
	extern struct mb_device *cgnineinfo[];
	int             unit;

	if ((unit = cg_compare(fbaddr, cgnineinfo, NCGNINE)) >= 0) {
	    *cgdev = makedev(CG9_MAJOR, unit);
	    return 0;
	}
    }
#endif NCGNINE > 0
#if NCGTWO > 0
#define CG2_MAJOR 31
    {
	extern struct mb_device *cgtwoinfo[];
	int             unit;

	if ((unit = cg_compare(fbaddr, cgtwoinfo, NCGTWO)) >= 0) {
	    *cgdev = makedev(CG2_MAJOR, unit);
	    return 0;
	}
    }
#endif	NCGTWO > 0
#endif	NGPONE > 0
    return -1;
}
