
#ifndef lint
static  char sccsid[] = "@(#)msg.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */



char *errmsg_list[] = {
	/*0 */	"not implemented.\n",
	/*1 */	"Unable to open the graphics processor. Broken?\n",
	/*2 */	"%s: caught signal %s, code %x *** test killed.\n",
	/*3 */	"%s: caught signal %d, code %x *** test killed.\n",
	/*4 */  "Unable to open %s.\n",
	/*5 */  "%s: 0x%X 0x%X 0x%X\n",
	/*6 */  "Can't find test name to get check sum",
	/*7 */  "checksum(s) not matched.\n",
	/*8 */  "Failed to fork test process: %s.\n",
	/*9 */  "Failed to fork test process.\n",
	/*10*/  "fork() failed.",
	/*11*/  "%s: failed to start.\n",
	/*12*/  "Failed to post GPSI commands.\n",
	/*13*/  "Failed to fork test process",
	/*14*/  "Can't kill child process",
	/*15*/  "Can't kill child process.\n",
	/*16*/  "failed with unknown error code.\n",
	/*17*/  "memory error.\n",
	/*18*/  "checksum error.\n",
	/*19*/  "Can't open Pixrect Device %s.\n",
	/*20*/  "Can not start C30 test code.\n",
	/*21*/	"C30 code test never returned.\n",
	/*22*/	"Can not start C30 test code.\n",
	/*23*/	"Could not find correct frame buffer.\n",
	/*24*/	"gp1_get_static_block failed.\n",
	/*25*/	"Invalid ctx_set argument list.\n",
	/*26*/	"gp1_post failed.\n",
	/*27*/	"esd_int: can't write to logfile\n",
	/*28*/	"can't open logfile %s. Run esd -n if you are not root.\n",
	/*29*/	"Failed to create unnamed pipe.\n",
	/*30*/	"can't write to logfile\n",
	/*31*/	"Can't open memory pixrect.\n",
	/*32*/	"Pixrect error at 0x%x.\n",
	/*33*/	"Buffer A: ",
	/*34*/	"Buffer B: ",
	/*35*/	"Simultaneous write to both buffers, read from buffer A: ",
	/*36*/	"Simultaneous write to both buffers, read from buffer B: ",
	/*37*/	"Pixrect error: ",
	/*38*/	"Pixrect Visual test: ",
	/*39*/	"%s Lookup Table error at index %d, expect %x, see %x.\n",
	/*40*/	"esd_sb: can't write to logfile\n",
	/*41*/	"Registers Test failed.\n",
	/*42*/	"On-Chip Memory Test failed.\n",
	/*43*/	"Integer Instructions Test failed.\n",
	/*44*/	"Float Instructions Test failed.\n",
	/*45*/	"SRAM error in Page 0.\n",
	/*46*/	"SRAM error in Page 1.\n",
	/*47*/	"SRAM error in Page 2.\n",
	/*48*/	"SRAM error in Page 3.\n",
	/*49*/	"DRAM error.\n",
	/*50*/	"no wids for double buffering available.\n",
	/*51*/	"Error in loading color stripes into frame buffer.\n",
	/*52*/	"Failed to allocate CG12 virtual buffer - gp1_alloc() error.\n",
	};


char	*routine_msg = "%s specific arguments [defaults]:\n\n\
	    D=device_name :\n\
		      Full pathname of the device under test [/dev/cgtwelve0].\n\n\
	    S=n   : Subtests to be run. The following subtests\n\
		      can be selected:\n\n\
		      - DSP			(n = 1)\n\
		      - SRAM & DRAM		(n = 2)\n\
		      - Video Memories		(n = 4)\n\
		      - Lookup Tables		(n = 8)\n\
		      - Vectors generation	(n = 16)\n\
		      - Polygons generation	(n = 32)\n";

char	*routine_msg1 = "\
		      - Transformations		(n = 64)\n\
		      - Clipping & Hidden	(n = 128)\n\
		      - Depth cueing		(n = 256)\n\
		      - Lighting & Shading	(n = 512)\n\
		      - Arbitration		(n = 1024)\n\n\
		      More than one subtest can be selected by Oring\n\
		      their subtest #'s. Example: n = 3 means DSP and\n\
		      SRAM & DRAM subtests. [n = -1].\n\n\
	    F=n    : # of loops for each subtest [n = 1].\n\n\
	    B=n    : # of loops for each board [n = 1].\n\n";

char	*test_usage_msg = "[D=dev_name] [S=n] [F=n] [B=n]";
