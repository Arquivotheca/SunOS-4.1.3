
#ifndef lint
static  char sccsid[] = "@(#)subtest_list.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


#include <gttest.h>

extern
char		
		/* DIRECT PORT */
		*fb_video_mem_host(),
		*fb_luts_shawdow_ram(),

		/* ACCELERATOR PORT */
		*fe_local_data_mem(),
		*rp_shared_ram(),
		*rp_ew_si_asics(),
		*fb_video_mem_i860(),
		*fb_output_section();
extern
char		/* INTEGRATION TEST */
		*hdl_vectors(),
		*hdl_triangles(),
		*hdl_pgons(),
		*hdl_splines(),
		*hdl_aa_vectors(),
		*hdl_aa_triangles(),
		*hdl_aa_pgons(),
		*hdl_clip(),
		*hdl_hidden(),
		*hdl_surf_fill(),
		*hdl_xsparency(),
		*hdl_lite_shad(),
		*hdl_pgon_edge_hili(),
		*hdl_depth_cueing(),
		*hdl_text(),
		*hdl_markers(),
		*hdl_picking(),
		*hdl_arbitration(),
		*hdl_stereo(),
		*hdl_lightpen();



struct test subtests[] = {
		
/* DIRECT PORT */

		{	"Video Memory (Direct Port)",
			0,
			fb_video_mem_host,
			&subtests[1],
		},

		{	"CLUTs & WLUT (Direct Port)",
			0,
			fb_luts_shawdow_ram,
			&subtests[2],
		},

/* ACCELERATOR PORT */

		{	"Local Data Memory (Front-End based)",
			1,
			fe_local_data_mem,
			&subtests[3],
		},

		{	"SU Shared RAM (Front-End based)",
			1,
			rp_shared_ram,
			&subtests[4],
		},

		{	"Rendering Pipeline (Front-End based)",
			1,
			rp_ew_si_asics,
			&subtests[5],
		},

		{	"Video Memory (Front-End based)",
			1,
			fb_video_mem_i860,
			&subtests[6],
		},

		{	"FB Output Section (Front-End based)",
			1,
			fb_output_section,
			&subtests[7],
		},

/* INTEGRATION TEST */

		{	"Vectors",
			1,
			hdl_vectors,
			&subtests[8],
		},

		{	"Triangles",
			1,
			hdl_triangles,
			&subtests[9],
		},

		{	"Spline Curves",
			1,
			hdl_splines,
			&subtests[10],
		},

		{	"Viewport Clipping",
			1,
			hdl_clip,
			&subtests[11],
		},

		{	"Hidden Surface Removal",
			1,
			hdl_hidden,
			&subtests[12],
		},

		{	"Edge Highlighting",
			1,
			hdl_pgon_edge_hili,
			&subtests[13],
		},

		{	"Transparency",
			1,
			hdl_xsparency,
			&subtests[14],
		},

		{	"Depth-Cueing",
			1,
			hdl_depth_cueing,
			&subtests[15],
		},

		{	"Lighting and Shading",
			1,
			hdl_lite_shad,
			&subtests[16],
		},

		{	"Text",
			1,
			hdl_text,
			&subtests[17],
		},

		{	"Picking",
			1,
			hdl_picking,
			&subtests[18],
		},

		{	"Arbitration",
			1,
			hdl_arbitration,
			&subtests[19],
		},

		{	"Stereo",
			1,
			hdl_stereo,
			&subtests[20],
		},

		{	"Light Pen",
			0,
			hdl_lightpen,
			0,
		},
};
