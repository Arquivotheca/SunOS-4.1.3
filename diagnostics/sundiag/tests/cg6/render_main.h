/******** @(#)render_main.h 1.1 7/30/92 Copyright 1988 Sun Micro *******/
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include <sys/types.h>

#include <pixrect/pixrect_hs.h>
#include "cg6var.h"


#include "wings.h"


extern int *fbc_base;
extern int *fhc_base;
extern int *tec_base;
extern int *thc_base;
extern int *dfb_base;
extern int *eprom_base;
extern int *dac_base;
#define FULL            0x20000000
#define BUSY            0x10000000
#define VBLANK_OCCURED  0x00080000


#define WATER   12
#define SKY     13
#define BGND    254
#define FGND    255


#define	X	0
#define	Y	1
#define	Z	2
#define	W	3

#define SINE_POWER      12
#define SINE_SIZE       4096
#define COSINE_POWER    12
#define COSINE_SIZE     4096

#define read_lego_fbc(addr)             (*(addr))
#define write_lego_fbc(addr, datum)     ((*(addr)) = (datum))
#define read_lego_tec(addr)             (*(addr))
#define write_lego_tec(addr, datum)     ((*(addr)) = (datum))
