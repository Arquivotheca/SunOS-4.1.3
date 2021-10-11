
#ifndef lint
static  char sccsid[] = "@(#)do_animation.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <sys/types.h>
#include <sys/timeb.h>
#include <esd.h>

extern
long		random();

extern
char		*initstate(),
		*setstate();

extern
time_t		time();

/**********************************************************************/
/* Static variables */
/**********************************************************************/

static
long rand_state[32]  =  {
		   3,  0x9a319039,  0x8999220b,       0x27fb47b9,
	  0x32d9c024,  0x9b663182,  0x5da1f342,       0x7449e56b,
          0xbeb1dbb0,  0xab5c5918,  0x946554fd,       0x8c2e680f,
          0xeb3d799f,  0xb11ee0b7,  0x2d436b86,       0xda672e2a,
          0x1588ca88,  0xe369735d,  0x904f35f7,       0xd7158fd6,
          0x6fa6f051,  0x616e6b96,  0xac94efdc,       0xde3b81e0,
          0xdf0a6fb5,  0xf103bc02,  0x48f340fb,       0x36413f93,
          0xc622c298,  0xf5a42ab8,  0x8a88d77b,       0xf5ad9d0e, };

static
time_t		ltime;

#define REVOLVER	256
/**********************************************************************/
do_animation()
/**********************************************************************/
/* keeps animation sequence going on on screen */

{
    char *centric_lines();

    double cx[REVOLVER];
    double cy[REVOLVER];
    double length[REVOLVER];
    int color[REVOLVER];
    register double x;
    register double y;
    register double l;
    register int c;
    register j;
    register double flong = (double)0x7fffffff / 2.0;
    register double fhlong = flong;
    register int i;


    func_name = "do_animation";
    TRACE_IN

    /* initialize state of the random generator */
    ltime = time((time_t *)0);
    (void)initstate((unsigned int)ltime, (char *)rand_state, 128);
    (void)setstate((char *)rand_state);

    for (i = 0 ; i < REVOLVER ; i++) {
	cx[i] = (((double)random()) / flong) - 1.0;
	cy[i] = (((double)random()) / flong) - 1.0;
	length[i] = ((double)random()) / fhlong;
	color[i] = random();
    }

    /* clear the 24-bit plane */
/*
    (void)clear_24bit_plane();
*/

    for(i = 128, j = i % REVOLVER ; i ; i--, j = i % REVOLVER){
	x = cx[j];
	y = cy[j];
	l = length[j];
	c = color[j];
	(void)ctx_set(CTX_SET_RGB_COLOR, c, 0);
	(void)centric_lines(x, y, l);
    }

    TRACE_OUT
    exit(0);
}
