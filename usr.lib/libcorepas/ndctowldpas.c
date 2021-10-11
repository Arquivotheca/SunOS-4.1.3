/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)ndctowldpas.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

int map_ndc_to_world_2();
int map_ndc_to_world_3();
int map_world_to_ndc_2();
int map_world_to_ndc_3();

int mapndctoworld2(ndcx, ndcy, wldx, wldy)
double ndcx, ndcy, *wldx, *wldy;
	{
	float tx,ty;
	int f;
	f=map_ndc_to_world_2(ndcx, ndcy, &tx, &ty);
	*wldx=tx;
	*wldy=ty;
	return(f);
	}

int mapndctoworld3(ndcx, ndcy, ndcz, wldx, wldy, wldz)
double ndcx, ndcy, ndcz, *wldx, *wldy, *wldz;
	{
	float tx,ty,tz;
	int f;
	f=map_ndc_to_world_3(ndcx, ndcy, ndcz, &tx, &ty, &tz);
	*wldx=tx;
	*wldy=ty;
	*wldz=tz;
	return(f);
	}

int mapworldtondc2(wldx, wldy, ndcx, ndcy)
double wldx, wldy, *ndcx, *ndcy;
	{
	float tx,ty;
	int f;
	f=map_world_to_ndc_2(wldx, wldy, &tx, &ty);
	*ndcx=tx;
	*ndcy=ty;
	return(f);
	}

int mapworldtondc3(wldx, wldy, wldz, ndcx, ndcy, ndcz)
double wldx, wldy, wldz, *ndcx, *ndcy, *ndcz;
	{
	float tx,ty,tz;
	int f;
	f=map_world_to_ndc_3(wldx, wldy, wldz, &tx, &ty, &tz);
	*ndcx=tx;
	*ndcy=ty;
	*ndcz=tz;
	return(f);
	}
