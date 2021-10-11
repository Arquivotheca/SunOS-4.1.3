/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)circle.c 1.1 92/07/30 SMI"; /* from UCB 5.1 6/7/85 */
#endif not lint

circle(x,y,r){
	arc(x,y,x+r,y,x+r,y);
}
