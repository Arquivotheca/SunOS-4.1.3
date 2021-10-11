
#ifndef lint
static  char sccsid[] = "@(#)sb_testlist.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


#include <esd.h>

extern
char		*dsp(),
		*sram_dram(),
		*video_mem(),
		*lut(),
		*lines(),
		*poligons(),
		*xf(),
		*clip_hid(),
		*depth_cueing(),
		*shad(),
		*animation();


struct test sb_testlist[] = {

		{
			"DSP",
			1,
			dsp,
			&sb_testlist[1],
		},

		{
			"SRAM and DRAM",
			1,
			sram_dram,
			&sb_testlist[2],
		},

		{
			"Video Memories",
			1,
			video_mem,
			&sb_testlist[3],
		},

		{
			"Lookup Tables",
			1,
			lut,
			&sb_testlist[4],
		},

		{
			"Vectors generation",
			1,
			lines,
			&sb_testlist[5],
		},
		{
			"Polygons generation",
			1,
			poligons,
			&sb_testlist[6],
		},
		{
			"Transformations",
			1,
			xf,
			&sb_testlist[7],
		},
		{
			"Clip & Hidden surface removal && Picking",
			1,
			clip_hid,
			&sb_testlist[8],
		},
		{
			"Depth Cueing",
			1,
			depth_cueing,
			&sb_testlist[9],
		},
		{
			"Lighting & shading",
			1,
			shad,
			&sb_testlist[10],
		},
		{
			"Arbitration",
			1,
			animation,
			0,
		},

};
