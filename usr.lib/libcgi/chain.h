/*	@(#)chain.h 1.1 92/07/30 SMI	*/
#ifndef CHAIN
#define CHAIN

#define CHAINSIZE 14

struct pr_triangle
{
    struct pr_pos   A, B, C;
};

struct pr_band
{
    int             y0, y1;	/* y coord of top and bottom of band */
};
struct pr_curve
{
    char            x, y;
    short           bits;
};

#endif CHAIN
