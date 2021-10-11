#ifndef lint
static  char sccsid[] = "@(#)windevconf.c 1.1 92/07/30 SMI";
#endif

#include "win.h"
#include "dtop.h"

#include <sunwindowdev/wintree.h>

Desktop desktops[NDTOP];
int ndesktops = NDTOP;
Workstation workstations[NDTOP];
int nworkstations = NDTOP;

struct	window *winbufs[NWIN];
int win_nwindows = NWIN;
