#ifndef lint
static char     sccsid[] = "@(#)gp2test_msgs.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
	/* gp2test.c */
char *stopped_msg = "Stopped.";
char *rootfd_msg = 
    "Couldn't create new 'rootfd' screen for '%s', %s.";
char *winfd_msg = 
    "Couldn't create new 'winfd' screen for '%s', %s.";
char *devtopfd_msg = 
   "Couldn't create new 'devtopfd' screen for '%s', %s.";
	/* gp2_probe.c */
char *gp2_open_msg = "Couldn't open GP2.\n";
	/* gp2gpcitest.c */
char *opengp_msg1 = "Can't open device %s\n";
char *opengp_msg2 = "Can't get a GP static block\n";
char *init_blk_msg1 = "Error Init static blk: gp1 alloc failed\n";
char *init_blk_msg2 = "Error Init static blk: post failed\n";
char *hardware_msg1 = "Error Test Hardware: gp1 alloc failed\n";
char *hardware_msg2 = "Error Test Hardware end: post failed\n";
char *hardware_msg3 = "Error Hardware end: sync failed\n";
char *mul_matrix_msg1 = "Error 3D Matrix: gp1 alloc failed\n";
char *mul_matrix_msg2 = "Error Mul point 2: post failed\n";
char *mul_point_msg1 = "Error Mul Point: gp1 alloc failed\n";
char *mul_point_msg2 = "Error Mul point 3: post failed\n";
char *proc_line_msg1 = "Error Proc line: gp1 alloc failed\n";
char *proc_line_msg2 = "Error Proc line end: post failed\n";
char *init_blk1_msg1 = "Error Init static blk 1: gp1 alloc failed\n";
char *init_blk1_msg2 = "Error Init static blk 1: post failed\n";
char *init_blk2_msg1 = "Error Init static blk 2: gp1 alloc failed\n";
char *init_blk2_msg2 = "Error Init static blk 2: post failed\n";
char *init_arbiter_msg1 = "Error Init Arbiter : gp1 alloc failed\n";
char *init_arbiter_msg2 = "Error Init Arbiter : post failed\n";
char *clrzb_msg1 = "Error Clear Zbuffer: gp1 alloc failed\n";
char *clrzb_msg2 = "Error Clear Zbuffer: post failed\n";
char *gp1poly3_msg1 = "Error gp1poly3: gp1 alloc failed\n";
char *gp1poly3_msg2 = "Error gp1poly3: post failed\n";
char *gp1poly3_msg3 = "Polygon: Overflow!!!\n";
char *gp1poly3_msg4 = "Error gp1poly3 end: post failed\n";
char *gp2_arb_poly3_msg1 = "Error gp2_arb_poly3: gp1 alloc failed\n";
char *gp2_arb_poly3_msg2 = "Error gp2_arb_poly3: post failed\n";
	/* gp2_hardware.c */
char *data_err_msg1 = "\t  exp %x obs %x  No errors: %d\n";
char *data_err_msg2 = "\t    exp %x obs %x\n";
char *xp_local_ram_msg1 = "\t*** Error XP Local Ram Test Did NOT START\n";
char *xp_local_ram_msg2 = "\t*** Error XP Local Ram Test: failed @ %x\n";
char *xp_shared_ram_msg1 = "\t*** Error XP Shared Ram Test Did NOT START\n";
char *xp_shared_ram_msg2 = "\t*** Error XP Shared Ram Test: failed @ %x\n";
char *xp_sequencer_msg1 = "\t*** Error XP Sequencer Test Did NOT START\n";
char *xp_sequencer_msg2 = 
		"\t*** Error XP Sequencer Test:   exp %x obs %x i %x\n";
char *xp_alu_msg1 = "\t*** Error XP Alu Test Did NOT START\n";
char *xp_alu_msg2 = "\t*** Error XP ALU Test:   exp %x obs %x\n";
char *xp_rp_fifo_msg1 = "\t*** Error XP RP Fifo Test Did NOT START\n";
char *xp_rp_fifo_msg2 = "\t*** Error XP XP -> RP Fifo Test:   Errors %d\n";
char *rp_local_ram_msg1 = "\t*** Error RP Local Ram Test: XP Did NOT START\n";
char *rp_local_ram_msg2 = "\t*** Error RP Local Ram Test: RP Did NOT START\n";
char *rp_local_ram_msg3 = "\t*** Error RP Local Ram Test: failed @ %x\n";
char *rp_shared_ram_msg1 = 
		"\t*** Error RP Shared Ram Test: XP Did NOT START\n";
char *rp_shared_ram_msg2 = 
		"\t*** Error RP Shared Ram Test: RP Did NOT START\n";
char *rp_shared_ram_msg3 = "\t*** Error RP Shared Ram Test: failed @ %x\n";
char *rp_sequencer_msg1 = "\t*** Error RP Sequencer Test: XP Did NOT START\n";
char *rp_sequencer_msg2 = "\t*** Error RP Sequencer Test: RP Did NOT START\n";
char *rp_sequencer_msg3 = 
		"\t*** Error RP Sequencer Test %d:   exp %x obs %x\n";
char *rp_alu_msg1 = "\t*** Error RP ALU Test: XP Did NOT START\n";
char *rp_alu_msg2 = "\t*** Error RP ALU Test: RP Did NOT START\n";
char *rp_alu_msg3 = "\t*** Error RP ALU Test:   exp %x obs %x\n";
char *rp_pp_fifo_msg1 = "\t*** Error RP PP Fifo Test: XP Did NOT START\n";
char *rp_pp_fifo_msg2 = "\t*** Error RP PP Fifo Test: RP Did NOT START\n";
char *rp_pp_fifo_msg3 = "\t*** Error RP RP -> PP Fifo Test:   Errors %d\n";
char *pp_ldx_ago_msg1 = "\t*** Error PP LDX AGO Test: XP Did NOT START\n";
char *pp_ldx_ago_msg2 = "\t*** Error PP LDX AGO Test: RP Did NOT START\n";
char *pp_ldx_ago_msg3 = "\t*** Error PP LDX AGO Test: PP Did NOT Finish\n";
char *pp_ldx_ago_msg4 = "\t*** Error PP LDX AGO:  exp %x obs %x\n";
char *pp_ady_ago_msg1 = "\t*** Error PP ADY AGO Test: XP Did NOT START\n";
char *pp_ady_ago_msg2 = "\t*** Error PP ADY AGO Test: RP Did NOT START\n";
char *pp_ady_ago_msg3 = "\t*** Error PP ADY AGO Test: PP Did NOT Finish\n";
char *pp_ady_ago_msg4 = "\t*** Error PP ADY AGO:  exp %x obs %x\n";
char *pp_adx_ago_msg1 = "\t*** Error PP ADX AGO Test: XP Did NOT START\n";
char *pp_adx_ago_msg2 = "\t*** Error PP ADX AGO Test: RP Did NOT START\n";
char *pp_adx_ago_msg3 = "\t*** Error PP ADX AGO Test: PP Did NOT Finish\n";
char *pp_adx_ago_msg4 = "\t*** Error PP ADX AGO:  exp %x obs %x\n";
char *pp_sequencer_msg1 = "\t*** Error PP Sequencer Test: XP Did NOT START\n";
char *pp_sequencer_msg2 = "\t*** Error PP Sequencer Test: RP Did NOT START\n";
char *pp_sequencer_msg3 = "\t*** Error PP Sequencer Test: PP Did NOT Finish\n";
char *pp_sequencer_msg4 = 
		"\t*** Error PP Sequencer Test %d:   exp %x obs %x\n";
char *pp_alu_msg1 = "\t*** Error PP ALU Test: XP Did NOT START\n";
char *pp_alu_msg2 = "\t*** Error PP ALU Test: RP Did NOT START\n";
char *pp_alu_msg3 = "\t*** Error PP ALU Test: PP Did NOT Finish\n";
char *pp_alu_msg4 = "\t*** Error PP ALU Test %d:   exp %x obs %x\n";
char *pp_rw_zbuf_msg1 = 
		"\t*** Error PP one R/W Zbuffer Test: XP Did NOT START\n";
char *pp_rw_zbuf_msg2 = 
		"\t*** Error PP one R/W Zbuffer Test: RP Did NOT START\n";
char *pp_rw_zbuf_msg3 = 
		"\t*** Error PP one R/W Zbuffer Test: PP Did NOT Finish\n";
char *pp_rw_zbuf_msg4 = "\t*** Error PP one R/W Zbuffer: Failed Pattern %x \n";
char *pp_zbuf_msg1 = "\t*** Error PP ZBuffer Test: XP Did NOT START\n";
char *pp_zbuf_msg2 = "\t*** Error PP ZBuffer Test: RP Did NOT START\n";
char *pp_zbuf_msg3 = "\t*** Error PP ZBuffer Test: PP Did NOT Finish\n";
char *pp_zbuf_msg4 = "\t*** Error PP ZBuffer:  Downward Scan No Errors %x\n";
char *pp_zbuf_msg5 = "\t*** Error PP ZBuffer:  Upward Scan No Errors %x\n";
char *post_mem_msg1 = "Error Memory: post failed\n";
char *post_mem_msg2 = "Error Memory: sync failed\n";
char *post_alu_msg1 = "Error Alu: post failed\n";
char *post_alu_msg2 = "Error Alu: sync failed\n";
char *post_fifo_msg1 = "Error fifo: post failed\n";
char *post_fifo_msg2 = "Error fifo: sync failed\n";
char *post_pp_ago_msg1 = "Error pp_ago: post failed\n";
char *post_pp_ago_msg2 = "Error pp_ago: sync failed\n";
char *post_ppld_msg1 = "Error PPld_reg: post failed\n";
char *post_ppld_msg2 = "Error PPld_reg: sync failed\n";
	/* gp2_matrix.c */
char *rotmat_msg = "Bad axis %c\n";
char *set_mat_msg1 = "Error Matrix 1: post failed\n";
char *set_mat_msg2 = "Error Matrix 1: sync failed\n";
char *chk_matrix_msg = "Mul Matrix: exp: %f obs: %f mat[%d][%d] %x %x %e\n";
	/* gp2_point.c */
char *get_matrix_msg1 = "Error Mul point: post failed\n";
char *get_matrix_msg2 = "Error Mul point: sync failed\n";
char *set_pts_msg1 = "Error Mul point 2: post failed\n";
char *set_pts_msg2 = "Error Mul point 2: sync failed\n";
char *multiply_pts_msg = "Mul Point: exp %f obs %f pointnum %d %x %x\n";
char *set_matrix_msg1 = "Error Proc line: post failed\n";
char *set_matrix_msg2 = "Error Proc line: sync failed\n";
char *set_line_msg1 = "Error Proc line: post failed\n";
char *set_line_msg2 = "Error Proc line: sync failed\n";
char *set_line_msg3 = "Proc Line Error: Proc_Line_FLT_3D DID NOT Finish\n";
char *check_line_msg1 = "Proc Line Clip_flag: exp 0 obs %x\n";
char *check_line_msg2 = "Proc Line Clip_flag: exp 8000 obs %x\n";
char *check_line_msg3 = "Proc Line Clip_flag: exp 1 obs %x\n";
char *check_line_msg4 = "Proc line Clip_flag: exp 2 obs %x\n";
char *check_line_msg5 = "Proc line Clip_flag: %x\n";
char *start_string = "Proc Line Start";
char *end_string = "Proc Line End";
char *cmp_clip_msg = "Error %s Point: exp %f obs %d pointnum %d Testno %d\n";
	/* gp2_polygon.c */
char *create_screen_msg = "Not Enough Memory to Create Mem Pixrect\n";
char *paint_polygons_msg1 = "Error Test Polygon 1: sync failed\n";
char *paint_polygons_msg2 = "Error Test Polygon 2: sync failed\n";
char *paint_polygons_msg3 = "Error Test Polygon 3: sync failed\n";
char *paint_polygons_msg4 = "Error Test Polygon 4: sync failed\n";
char *paint_polygons_msg5 = "Error Polygon chk sum does not match: 1st chksum %x 2nd chksum %x polygon %d hsr %s\n";
char *do_arbitration_msg1 = "Error do_arbitration : sync failed\n";
char *do_arbitration_msg2 = "Error do_arbitration : check sum error\n";
char *pix_clr_msg = 
		"Error Polygon Pixel: Failed @ %x data %x polygon %d hsr %s\n";
char *display_msg1 = "Error Test Polygon 3: sync failed\n";
char *display_msg2 = "Error Test Polygon 4: sync failed\n";
char *display_msg3 = "Error Test Polygon 5: sync failed\n";
