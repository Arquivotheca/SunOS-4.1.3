
/*      @(#)gp2_polygon.h 1.1 88/04/21 SMI          */
 
/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc. 
 */
#define ITERATIONS (1)
#define MAXPOLY 3210
#define MAXBNDSPERPOLY 1
#define MAXVERT 2200*5

struct point modellst_1[] = {
    {0.025, 0.0,   0.2, 0x10000},
    {0.0,   0.025, 0.2, 0x48000},
    {0.025, 0.05,  0.7, 0x6ffff},
};
int cmodellst_1[] = { 0xff0000, 0x00ff00, 0x0000ff };
int modelnvert_1[1] = {3};
struct polygon modelpoly_1 = {1, modelnvert_1, modellst_1, 0.025, 0.05};

struct point modellst_2[] = {
    {0.0, 0.0125,  0.2, 0x50000},
    {0.05, 0.025, 0.7, 0x6ffff},
    {0.05, 0.0, 0.2, 0x6ffff}
};
int cmodellst_2[] = { 0xff0000, 0x00ff00, 0x0000ff };
int modelnvert_2[1] = {3};
struct polygon modelpoly_2 = {1, modelnvert_2, modellst_2, 0.05, 0.025};


struct point modellst_3[] = {
    {0.0, 0.0,  0.2, 0x10000},
    {0.1, 0.1, 0.7, 0x48000},
    {0.2, 0.0, 0.2, 0x6ffff}
};
int cmodellst_3[] = { 0xff0000, 0x00ff00, 0x0000ff };
int modelnvert_3[1] = {3};
struct polygon modelpoly_3 = {1, modelnvert_3, modellst_3, 0.2, 0.1};

struct point modellst_4[] = {
    {0.125, 0.0,     0.8, 0x10000},
    {0.15,  0.075,   0.6, 0x20000},
    {0.25,  0.075,   0.4, 0x30000},
    {0.175, 0.125,   0.6, 0x40000},
    {0.225, 0.25,    0.6, 0x5FFFF},
    {0.125, 0.15,    0.6, 0x50000},
    {0.025, 0.25,    0.6, 0x5FFFF},
    {0.075, 0.125,   0.6, 0x40000},
    {0.0,   0.075,   0.6, 0x30000},
    {0.100, 0.075,   0.6, 0x20000},
};
int cmodellst_4[] = { 0xff0000, 0x00ff00, 0x0000ff, 0xffff00, 0xff00ff,
	0x00ffff, 0x000000, 0xffffff, 0xff0000, 0x0000ff };
int modelnvert_4[1] = {10};
struct polygon modelpoly_4 = {1, modelnvert_4, modellst_4, 0.25, 0.25};

struct point modellst_0[] = {
    {0.0, 0.05,  0.8, 0x10000},
    {0.05, 0.1, 0.6, 0x48000},
    {0.1, 0.05, 0.4, 0x6FFFF},
    {0.05, 0.0, 0.6, 0x48000},
};
int cmodellst_0[] = { 0x10000, 0x48000, 0x6FFFF, 0x48000 };
int modelnvert_0[1] = {4};
struct polygon modelpoly_0 = {1, modelnvert_0, modellst_0, 0.1, 0.1};
 
#define POLYGON_LIST_MAX (12)

struct polygon *poly_list[] = {
    &modelpoly_0,
    &modelpoly_1,
    &modelpoly_2,
    &modelpoly_3,
    &modelpoly_4,
};

struct point coordlst[MAXVERT];
int nvert[MAXPOLY * MAXBNDSPERPOLY];
struct polygon polylst[MAXPOLY];
int npoly;

typedef float matrix_3d[4][4];

#define RED_BIT (1)
#define GREEN_BIT (2)
#define BLUE_BIT (4)
