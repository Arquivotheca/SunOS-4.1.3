#ifndef lint
static  char sccsid[] = "@(#)bw2_conf.c 1.1 92/07/30 SMI";
#endif

#include "bwtwo.h"
#include "win.h"
#if NBWTWO > 0

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <sundev/bw2reg.h>

int nbwtwo = NBWTWO;
struct bw2_softc bw2_softc[NBWTWO];
struct  mb_device *bwtwoinfo[NBWTWO];

#endif NBWTWO > 0
