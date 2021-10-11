#ifndef lint
static char sccsid[] = "@(#)glass.c 1.1 92/07/30 SMI";
#endif

#include "cgidefs.h"

static Ccoorlist martinilist;
static Ccoor glass_coords[10] = {
	0, 0,
	-10, 0,
	-1, 1,
	-1, 20,
	-15, 35,
	15, 35,
	1, 20,
	1, 1,
	10, 0,
	0, 0
};

static Ccoor water_coords[2] = {
	-12, 33,
	12, 33
};

static Ccoor vpll = {-50, -10};
static Ccoor vpur = {50, 80};

Cint name;
Cvwsurf device;

main()
{
	device.dd = PIXWINDD;
	open_cgi();			/* initialize CGI */
	open_vws(&name, &device);	/* open view surface */
	vdc_extent(&vpll, &vpur);	/* reset CDC space */
	martinilist.ptlist = glass_coords; /* load polyline structure */
	martinilist.n = 10;
	polyline(&martinilist);		/* draw glass */
	martinilist.ptlist = water_coords;
	martinilist.n = 2;		/* draw waterline */
	polyline(&martinilist);
	sleep(10);			/* display for 10 seconds */
	close_vws(name);		/* exit */
	close_cgi();
	exit(0);
}
