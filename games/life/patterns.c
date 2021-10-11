#ifndef lint
static  char sccsid[] = "@(#)patterns.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems Inc.
 */

#include <stdio.h>
#include <suntool/tool_hs.h>
#include <suntool/optionsw.h>
#include <suntool/msgsw.h>
#include <suntool/menu.h>
#include "life.h"

#define MAXEXAMPLE

static int xarr[200];
static int yarr[200];
static int cnt;

handlemenu(ie)
	struct inputevent	ie;
{

	init();
	switch(ie.ie_code) {
		case RUN:
			run_proc();
			return;
		case RESET:
			reset_proc();
			return;
		case GLIDER:
			paint(1,3);
			paint(2,3);
			paint(3,3);
			paint(2,1);
			paint(3,2);
			break;
		case EIGHT: 
			paint(6,6);
			paint(6,7);
			paint(6,8);
			paint(7,6);
			paint(7,7);
			paint(7,8);
			paint(8,6);
			paint(8,7);
			paint(8,8);
			paint(9,9);
			paint(9,10);
			paint(9,11);
			paint(10,9);
			paint(10,10);
			paint(10,11);
			paint(11,9);
			paint(11,10);
			paint(11,11);
			break;
		case PULSAR:
			paint(10,10);
			paint(11,10);
			paint(12,10);
			paint(13,10);
			paint(14,10);
			paint(10,11);
			paint(14,11);
			break;
		case MUCHNICK:
			paint(10,10);
			paint(11,10);
			paint(12,10);
			paint(13,10);
			paint(14,10);
			paint(15,10);
			paint(10,11);
			paint(15,11);

			paint(10,4);
			paint(11,4);
			paint(12,4);
			paint(13,4);
			paint(14,4);
			paint(15,4);
			paint(10,3);
			paint(15,3);
			break;
		case GUN:
			paint(2,6);
			paint(3,5);
			paint(3,6);
			paint(12,4);
			paint(12,5);
			paint(12,6);
			paint(13,3);
			paint(13,7);
			paint(14,2);
			paint(14,8);
			paint(15,2);
			paint(15,8);
			paint(25,5);
			paint(25,9);
			paint(26,4);
			paint(26,5);
			paint(26,9);
			paint(26,10);
			paint(28,6);
			paint(28,7);
			paint(28,8);
			paint(29,6);
			paint(29,7);
			paint(29,8);
			paint(30,7);
			paint(36,8);
			paint(37,7);
			paint(37,8);
			break;
		case ESCORT:
			paint(4,11);
			paint(5,10);
			paint(6,10);
			paint(7,10);
			paint(8,5);
			paint(8,10);
			paint(8,17);
			paint(9,6);
			paint(9,10);
			paint(9,18);
			paint(10,6);
			paint(10,10);
			paint(10,18);
			paint(11,6);
			paint(11,10);
			paint(11,18);
			paint(12,6);
			paint(12,10);
			paint(12,18);
			paint(13,3);
			paint(13,6);
			paint(13,10);
			paint(13,15);
			paint(13,18);
			paint(14,4);
			paint(14,5);
			paint(14,6);
			paint(14,10);
			paint(14,16);
			paint(14,17);
			paint(14,18);
			paint(15,10);
			paint(15,13);
			paint(16,10);
			paint(16,11);
			paint(16,12);
			break;
		case BARBER:
			paint(2,2);
			paint(2,3);
			paint(3,2);
			paint(4,3);
			paint(4,5);
			paint(6,5);
			paint(6,7);
			paint(8,7);
			paint(8,9);
			paint(10,9);
			paint(10,11);
			paint(12,11);
			paint(12,13);
			paint(13,14);
			paint(14,13);
			paint(14,14);
			break;
		case HERTZ:
			paint(2,6);
			paint(2,7);
			paint(2,9);
			paint(2,10);
			paint(3,6);
			paint(3,10);
			paint(4,7);
			paint(4,8);
			paint(4,9);
			paint(6,7);
			paint(6,8);
			paint(6,9);
			paint(7,3);
			paint(7,4);
			paint(7,6);
			paint(7,8);
			paint(7,10);
			paint(7,12);
			paint(7,13);
			paint(8,3);
			paint(8,4);
			paint(8,6);
			paint(8,10);
			paint(8,12);
			paint(8,13);
			paint(9,6);
			paint(9,10);
			paint(10,6);
			paint(10,10);
			paint(11,7);
			paint(11,8);
			paint(11,9);
			paint(13,7);
			paint(13,8);
			paint(13,9);
			paint(14,6);
			paint(14,10);
			paint(15,6);
			paint(15,7);
			paint(15,9);
			paint(15,10);
			break;
		case TUMBLER:
			paint(2,6);
			paint(2,7);
			paint(2,8);
			paint(3,3);
			paint(3,4);
			paint(3,8);
			paint(4,3);
			paint(4,4);
			paint(4,5);
			paint(4,6);
			paint(4,7);
			paint(6,3);
			paint(6,4);
			paint(6,5);
			paint(6,6);
			paint(6,7);
			paint(7,3);
			paint(7,4);
			paint(7,8);
			paint(8,6);
			paint(8,7);
			paint(8,8);
			break;
		case PUFFER:
			paint(4,3);
			paint(4,8);
			paint(4,17);
			paint(5,4);
			paint(5,9);
			paint(5,12);
			paint(5,18);
			paint(6,4);
			paint(6,9);
			paint(6,10);
			paint(6,11);
			paint(6,18);
			paint(7,1);
			paint(7,4);
			paint(7,15);
			paint(7,18);
			paint(8,2);
			paint(8,3);
			paint(8,4);
			paint(8,16);
			paint(8,17);
			paint(8,18);
			break;
		default:
			return;
	}
	doit();
}

#define MAXINT 0x7fffffff
#define MININT 0x80000000

static left;
static right;
static top;
static bottom;

static
paint(x,y)
{
	if (x < left)
		left = x;
	if (x > right)
		right = x;
	if (y < top)
		top = y;
	if (y > bottom)
		bottom = y;
	xarr[cnt] = x;
	yarr[cnt++] = y;
}

static
realpaint(l, r)
{
	int i, x, y;
	
	for (i = 0; i < cnt; i++) {
		x = xarr[i] + l;
		y = yarr[i] + r;
		addpoint(x - leftoffset, y - upoffset);
		if (x > rightedge-1 || y > bottomedge-1)
			return;
		paint_stone(x, y, INITCOLOR);
	}
}

static
init()
{
	left = MAXINT;
	right = MININT;
	top = MAXINT;
	bottom = MININT;
	cnt = 0;
}

static
doit()
{
	realpaint(-left + (rightedge - (right - left))/2,
	     (-top + (bottomedge - (bottom - top))/2));
}
