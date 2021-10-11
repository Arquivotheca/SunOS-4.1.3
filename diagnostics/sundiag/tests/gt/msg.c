
#ifndef lint
static  char sccsid[] = "@(#)msg.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */



char	*routine_msg = "%s specific arguments [defaults]:\n\n\
	    D=device_name :\n\
		      Full pathname of the device under test [/dev/gt0].\n\n\
	    S=n   : Subtests to be run. The following subtests\n\
		      can be selected:\n\n\
		      DIRECT PORT:\n\
		      - Video memories			(n = 0x000001)\n\
		      - CLUTs & WID LUT			(n = 0x000002)\n";

char	*routine_msg1 = "\
		      ACCELERATOR PORT:\n\
		      - FE Local Data Memory		(n = 0x000004)\n\
		      - RP SU Shared RAM		(n = 0x000008)\n\
		      - Rendering Pipeline		(n = 0x000010)\n\
		      - Video memories			(n = 0x000020)\n\
		      - Frame Buffer Output Section	(n = 0x000040)\n\
		      INTEGRATION TEST:\n";

char	*routine_msg2 = "\
		      - Vectors				(n = 0x000080)\n\
		      - Triangles			(n = 0x000100)\n\
		      - Spline Curves			(n = 0x000200)\n\
		      - Viewport Clipping		(n = 0x000400)\n\
		      - Hidden Surface Removal		(n = 0x000800)\n";

char    *routine_msg3 = "\
		      - Polygon Edges Highlighting	(n = 0x001000)\n\
		      - Transparency			(n = 0x002000)\n\
		      - Depth-Cueing			(n = 0x004000)\n\
		      - Lighting and Shading		(n = 0x008000)\n\
		      - Texts				(n = 0x010000)\n\
		      - Picking				(n = 0x020000)\n\
		      - Arbitration			(n = 0x040000)\n\
		      - Stereo (interactive)		(n = 0x080000)\n\
		      - Light Pen (interactive)		(n = 0x100000)\n\n";
char    *routine_msg4 = "\
		      More than one subtest can be selected by ORing\n\
		      their subtest numbers. Example: n = 0x180 means\n\
		      Vectors and Triangles Tests. A hex number must be\n\
		      preceeded by 0x, decimal numbers are acceptable.\n\
		      [n = -1].\n\n\
	    F=n    : # of loops for each subtest [n = 1].\n\n\
	    B=n    : # of loops for each test sequence [n = 1].\n\n";

char	*test_usage_msg = "[D=dev_name] [S=n] [F=n] [B=n]";

char	*lightpen_msg[] =  {

	"               CALIBRATION PROCEDURE",
	"               ---------------------",
	"\n",
	"The Lightpen needs to be calibrated before",
	"the test can begin.",
	"Point the lightpen exactly to the RED crosshair",
	"in the middle of the screen and click the light-",
	"pen button ONCE. If the procedure is successful,",
	"the screen will be cleared and the test can begin.",
	"\n",
	"                  TEST PROCEDURE",
	"                  --------------",
	"\n",
	"Use the lightpen to draw on screen surface. Clicking",
	"the lightpen button cycles the drawing colors.",
	"The drawing surface is cleared every time the light-",
	"pen does not detect light for appr. TWO seconds,",
	"       *** To end the test, type q. ***",

	"",
	
	};

