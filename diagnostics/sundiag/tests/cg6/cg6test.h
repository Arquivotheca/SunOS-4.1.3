/******* @(#)cg6test.h 1.1 7/30/92 Copyright 1988 Sun Micro *****/
#include <stdio.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <sgtty.h>
#include <sun/fbio.h>
#include <sys/types.h>
#include <pixrect/pixrect_hs.h>
#include "cg6var.h"
#include <sunwindow/win_cursor.h>
#include <suntool/sunview.h>
#include <suntool/gfx_hs.h>

extern Pixrect *prfd, *sprfd;
extern int width, height, depth;
extern unsigned char red1[256], grn1[256], blu1[256];
extern Pixrect *mexp, *mobs;

extern char randvals[];
