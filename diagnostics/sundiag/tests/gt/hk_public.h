#ifndef _HK_PUBLIC_
#define _HK_PUBLIC_

/*
 ***********************************************************************
 * WARNING: The context is frozen at HK_CTX_VERSION = 0x0201.  Any
 *	further changes must be made in a completely backward
 *	compatible manner.  The size of the Hk_context and Hk_state
 *	structures as well as the offsets of all elements *must* remain
 *	constant in order for existing programs to continue to run with
 *	new firmware.
 ***********************************************************************
 *
 *
 * hk_public.h
 *
 *	@(#)hk_public.h 1.1 92/07/30 17:41:11
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 *	Hawk Instruction Set and Front End Processor algorithms (hk_fe).
 *
 *	This header file contains public constants and types used
 *	by the Hawk simulator and by any program (e.g. hasm, test_hdl,
 *	toy_alouette) building a display list using the Hawk Instruction
 *	Set.
 *
 *  4-Apr-89 Kevin C. Rushforth   Split files for revision 3.
 * 10-Apr-89 Scott R. Nelson	  Change vector_list to polyline.
 *				  Add hk_blend_op_aa.  Add culling flags.
 * 11-Apr-89 Scott R. Nelson	  Add bundled attributes.
 * 12-Apr-89 Kevin C. Rushforth	  Re-aranged a couple of things between
 *				  fe_globals and fe_public.
 * 13-Apr-89 Scott R. Nelson	  Removed enable_light from light structure.
 * 18-Apr-89 Scott R. Nelson	  Added state information for push and pop
 *				  state instructions.
 * 20-Apr-89 Scott R. Nelson	  Fix cmt_to_gmt instruction.
 * 24-Apr-89 Scott R. Nelson	  Added depth_cue_color, re-ordered commands.
 *  3-May-89 Scott R. Nelson	  Cache the registers.
 *  4-May-89 Scott R. Nelson	  Modify blend_op values.
 *  5-May-89 Gene Pinkston	  added triangle structures
 *  8-May-89 Scott R. Nelson	  Restore blend_op to previous valus.
 *  9-May-89 Kevin C. Rushforth	  Added HK_ to header constants.
 * 10-May-89 Scott R. Nelson	  Move raster_op into the state.
 * 15-May-89 Josh Lee		  Changed viewport_bound values type to 'int'.
 * 17-May-89 Josh Lee		  Added x_,y_jitter attributes.
 *  7-Jun-89 Ranjit Oberoi	  Added picking related commands
 * 22-Jun-89 Ranjit Oberoi	  Added Marker_table.
 * 25-Jun-89 Ranjit Oberoi	  Added picking results structures.
 * 30-Jun-89 Ranjit Oberoi	  Added STATE defines for filters etc.
 *  6-Jul-89 Ranjit Oberoi	  Deleted dot references.
 * 11-Jul-89 Kevin C. Rushforth	  Added ifdefs for Jet-C, moved XFORM
 *				  macros to fe_globals.h
 * 23-Jul-89 Allen M. Harris	  Added typedefs for line pattern
 * 23-Aug-89 Ranjit Oberoi	  Changes for user defined markers.
 * 24-Aug-89 Scott R. Nelson	  Remove marker_line_routine.
 * 29-Aug-89 Scott R. Nelson	  Add the blend_op_transp command.
 * 30-Aug-89 Josh Lee		  Added stochastic sampling stuff.
 * 31-Aug-89 Scott R. Nelson	  Changed rop_foreground, rop_background
 *				  and window_background to use 3 floats.
 *				  Added pre/post concat vt/ivt.
 *  1-Sep-89 Ranjit Oberoi	  A change in the marker table definition.
 * 11-Sep-89 Scott R. Nelson	  Re-number all attribute codes for the
 *				  new dispatch scheme.  Delete the attribute
 *				  size constants (now in fe_attribute.c).
 *				  Rename and add context variables.
 * 18-Sep-89 Nelson, Rushforth	  Add virtual communication register stuff.
 *				  Changed the name from fe_public to hk_public.
 *				  Put HK_ on ALL names in the module.
 * 22-Sep-89 Scott R. Nelson	  Move text_font and text_character_set
 *				  to special attribute processing.
 * 22-Sep-89 Kevin C. Rushforth	  Moved stuff from hk_text.h to hk_public.h
 *				  and renamed them to start with HK_
 * 25-Sep-89 Kevin C. Rushforth	  Removed obsolete SET_LIGHT command
 * 29-Sep-89 Ranjit Oberoi	  Added nurbs/parametric defines.
 * 10-Oct-89 Scott R. Nelson	  Added commands.
 * 31-Oct-89 Kevin C. Rushforth	  Changed dpc in context to unsigned.
 *  6-Nov-89 Ranjit Oberoi	  Added new edge and hollow polygon attributes.
 * 25-Nov-89 Ranjit Oberoi	  Added tri replacement control bits define.
 * 28-Nov-89 Kevin C. Rushforth	  MANY changes for HIS 1.3 upgrade.
 * 30-Nov-89 Allen M. Harris	  Added linestyle constants for HIS 1.3 upgrade
 *  1-Dec-89 Allen M. Harris	  Added maximum pattern size for ldm buffers
 *  8-Dec-89 Kevin C. Rushforth	  Changed vp_z{front,back} to unsigned int.
 * 19-Dec-89 Scott R. Nelson	  Add JMPR instruction.
 *				  Add a set of error codes.
 * 15-Jan-90 Scott R. Nelson	  Update virtual communication register
 *				  definitions.
 * 17-Jan-90 Kevin C. Rushforth	  Removed quad meshes
 * 19-Jan-90 Scott R. Nelson	  Rearranged vcomm bits, added hk_illegal_attr
 * 23-Jan-90 Josh Lee		  Revised stochastic sampling attributes.
 * 30-Jan-90 Ranjit Oberoi	  Added interrupts related stuff.
 * 30-Jan-90 Josh Lee		  Added store_image_8 instruction.
 * 13-Feb-90 Rushforth, Nelson	  Update to match the 1.4 version of the
 *				  Hawk Instruction Set.
 * 21-Feb-90 Ranjit Oberoi	  Updates for HIS rev 1.4.
 *  5-Apr-90 Ranjit Oberoi	  Deleted *filter_flags from Hk_state; these 
 *				  were for the exclusive use by Hawk firmware.
 *  9-Apr-90 Kevin C. Rushforth	  Added padding to structures containing
 *				  double-precision floats.  Re-arranged
 *				  the members of Hk_state so all structres
 *				  containing embedded doubles are first.
 *				  Added a magic number and version to CTX/MCB.
 * 12-Apr-90 Kevin C. Rushforth	  Removed "*_stopped" flags from context,
 *				  Changed processor_running to hawk_stopped
 *				  in MCB.
 * 12-Apr-90 Scott R. Nelson	  Added hk_stereo_mode.
 * 17-Apr-90 Scott R. Nelson	  Added lookup table stuff.
 * 19-Apr-90 Kevin C. Rushforth	  Added new error codes.
 * 25-Apr-90 Kevin C. Rushforth	  Bump CTX version number for stereo change.
 * 27-Apr-90 Kevin C. Rushforth	  Added print_buffer field in MCB.
 *  4-May-90 Kevin C. Rushforth	  Added hk_scratch_buffer.
 * 22-May-90 Kevin C. Rushforth	  Bumped MCB version due to hk_comm change.
 * 16-Jul-90 Ranjit Oberoi	  Added additional HKERR_* codes.
 * 25-Jul-90 Kevin C. Rushforth	  Split out MCB related stuff to "gtmcb.h"
 * 31-Jul-90 Vic Tolomei	  Add diag_escape
 *  2-Aug-90 Michelle Feraud	  Put LOCORE flag around C declarations
 * 27-Aug-90 Scott R. Nelson	  Fixed blend constants to use maximum
 *				  wait states.
 *  9-Sep-90 Kevin C. Rushforth	  Fixed the HK_INV_* constants to match HIS
 *
 *  3-Oct-90 Ranjit Oberoi	  Deleted parent_needs and added 
 *				  need_from_childern arrays to the context.
 * 10-Oct-90 Kevin C. Rushforth	  Changes for new window_boundary semantics.
 * 21-Oct-90 Kevin C. Rushforth	  More efficient version of HK_EXTRACT_OP()
 * 25-Oct-90 Kevin C. Rushforth	  Updated blend program timing values.
 *  3-Jan-91 Nelson, Rushforth	  UPDATES FOR HIS 1.5
 *  9-Jan-91 Kevin C. Rushforth	  Completed HIS 1.5 version
 * 17-Jan-91 Kevin C. Rushforth	  Added HK_NO_TRANSPARENCY value
 * 25-Jan-91 Kevin C. Rushforth	  Added two new EXTRACT macros
 *  6-Feb-91 Kevin C. Rushforth	  Added some missing HIS 1.5 constants
 * 24-Mar-91 Kevin C. Rushforth	  Added room for expansion to Hk_state
 *				  and Hk_context.
 * 25-Mar-91 Kevin C. Rushforth	  Added clipping limits attribute.  Added
 *				  save area for previous pick info.
 *  2-May-91 Ranjit/Kevin	  Deleted model_planes from the state part
 *				  of the context, thereby creating additional
 *				  64 reserved words for future expansion.
 * 28-May-91 Ranjit Oberoi        define HK_TRI_IMPLICIT_CEN for triangle_list.
 * 12-Jun-91 Kevin C. Rushforth	  Documentation cleanup.
 * 14-Jun-91 Kevin C. Rushforth	  Added definitions for polyline with header.
 * 20-Jun-91 Kevin C. Rushforth	  Added EPS leaf node optimization.
 * 21-Jun-91 Ranjit Oberoi	  Added QUM support.
 * 24-Jun-91 Kevin C. Rushforth	  Added conditional CEN increment.
 * 25-Jun-91 Kevin C. Rushforth	  Added NOTIFY_KERNEL instruction.
 * 27-Jun-91 Kevin C. Rushforth	  Moved user visible changes to context
 *				  for new pick echo attributes ahead of
 *				  non-user visible fields.  This breaks
 *				  compatibility with release 9.4.1, but
 *				  since PHIGS isn't using these new
 *				  attributes yet they have assured us
 *				  that it is ok.
 * 16-Aug-91 Ranjit Oberoi	  Added cen_state in state part of the context,
 *				  and leaf_cen_state in the fixed part.
 * 15-Nov-91 Kevin C. Rushforth	  Added new HK_UNDRAW boolean attribute.
 ***********************************************************************
 * WARNING: The context is frozen at HK_CTX_VERSION = 0x0201.  Any
 *	further changes must be made in a completely backward
 *	compatible manner.  The size of the Hk_context and Hk_state
 *	structures as well as the offsets of all elements *must* remain
 *	constant in order for existing programs to continue to run with
 *	new firmware.
 ***********************************************************************
 */

#define HK_CTX_VERSION	0x0201		/* Current version of Hk_context */



/*
 * Magic number for Hk_context data structure.  This should NOT be
 * modified.
 */

#define HK_CTX_MAGIC	0xA7880102	/* Hawk context: Hk_context */



/*
 *--------------------------------------------------------------
 *
 * Instruction word definitions for primary opcode
 *
 *	The instructions are in approximately the same order as found
 *	in the document: Hawk Instruction Set 1.5
 *
 *	Changes here must also be made in fe_jump_tables.c
 *
 *--------------------------------------------------------------
 */

/* Instructions (in approx. the same order to the Instruction Set document) */

#define HK_OP_ILLEGAL			0x00	/* Illegal opcode (must be 0) */
#define HK_OP_RESERVED_M		0x01	/* Reserved */

/* Data manipulation instructions */
#define HK_OP_NOPD			0x02
#define HK_OP_LD			0x03
#define HK_OP_LDU			0x04
#define HK_OP_LDD			0x05
#define HK_OP_ST			0x06
#define HK_OP_STU			0x07
#define HK_OP_STD			0x08
#define HK_OP_LDSTU			0x09
#define HK_OP_SWAP			0x0a
#define HK_OP_PUSH			0x0b
#define HK_OP_POP			0x0c
#define HK_OP_BLOCK_MOVE		0x0d
#define HK_OP_MOVE			0x0e
#define HK_OP_ADD			0x0f
#define HK_OP_ADDI			0x10
#define HK_OP_SUB			0x11
#define HK_OP_SUBI			0x12
#define HK_OP_MULT			0x13
#define HK_OP_DIV			0x14
#define HK_OP_MOD			0x15
#define HK_OP_AND			0x16
#define HK_OP_OR			0x17
#define HK_OP_XOR			0x18
#define HK_OP_ABS			0x19
#define HK_OP_NOT			0x1a
#define HK_OP_SLL			0x1b
#define HK_OP_SRL			0x1c
#define HK_OP_SRA			0x1d
#define HK_OP_FLOAT			0x1e
#define HK_OP_FIX			0x1f
#define HK_OP_FADD			0x20
#define HK_OP_FSUB			0x21
#define HK_OP_FMULT			0x22
#define HK_OP_FDIV			0x23
#define HK_OP_FMOD			0x24
#define HK_OP_FABS			0x25

/* NOTE: opcodes 0x26-0x2f are unused */

/* Geometry instructions */
#define HK_OP_POLYLINE			0x30
#define HK_OP_POLYMARKER		0x31
#define HK_OP_RECTANGULAR_MARKER_GRID	0x32
#define HK_OP_RADIAL_MARKER_GRID	0x33
#define HK_OP_TRIANGLE_LIST		0x34
#define HK_OP_POLYEDGE			0x35
#define HK_OP_PARAMETRIC_CURVE		0x36
#define HK_OP_NURB_CURVE		0x37
#define HK_OP_RTEXT			0x38
#define HK_OP_ATEXT			0x39
#define HK_OP_ANNOTATION_POLYLINE	0x3a
#define HK_OP_ANNOTATION_TRIANGLE_LIST	0x3b

/* NOTE: opcodes 0x3c-0x3f are unused */

/*
 * Set attribute instruction.  The 8-bit attribute code overlaps the
 * main op-code by 3 bits, but the attributes are now grouped in sets
 * of 32, so it could now be considered a 5-bit code.  HASM would have
 * to change first if the 5-bit code is used.  Note that bundle-able
 * attributes can have three different main op-codes for one attribute
 * code.
 */
#define HK_OP_SET_ATTR_I		0x40	/* Set attr inst (individual) */
#define HK_OP_SET_ATTR_I1		0x41	/* Include all the variants */
#define HK_OP_SET_ATTR_I2		0x42	/* Non-ASF in 1st group only */
#define HK_OP_SET_ATTR_I3		0x43
#define HK_OP_SET_ATTR_I4		0x44
#define HK_OP_SET_ATTR_I5		0x45
#define HK_OP_SET_ATTR_I6		0x46
#define HK_OP_SET_ATTR_I7		0x47
#define HK_OP_SET_ATTR_B		0x48	/* Set attr inst (bundled) */
#define HK_OP_SET_ATTR_B1		0x49	/* Include all the variants */
#define HK_OP_SET_ATTR_B2		0x4a
#define HK_OP_SET_ATTR_B3		0x4b
#define HK_OP_SET_ATTR_B4		0x4c
#define HK_OP_SET_ATTR_B5		0x4d
#define HK_OP_SET_ATTR_B6		0x4e
#define HK_OP_SET_ATTR_B7		0x4f
#define HK_OP_SET_ATTR_A		0x50	/* Set ASF inst */
#define HK_OP_SET_ATTR_A1		0x51	/* Include all the variants */
#define HK_OP_SET_ATTR_A2		0x52
#define HK_OP_SET_ATTR_A3		0x53
#define HK_OP_SET_ATTR_A4		0x54
#define HK_OP_SET_ATTR_A5		0x55
#define HK_OP_SET_ATTR_A6		0x56
#define HK_OP_SET_ATTR_A7		0x57

/* NOTE: opcodes 0x58-0x5f are unused */

/* Control instructions */
#define HK_OP_JMPL			0x60
#define HK_OP_JMPR			0x61
#define HK_OP_BLEG			0x62
#define HK_OP_FBLEG			0x63
#define HK_OP_BINV			0x64
#define HK_OP_JPS			0x65
#define HK_OP_EPS			0x66
#define HK_OP_RPS			0x67
#define HK_OP_PUSH_STATE		0x68
#define HK_OP_POP_STATE			0x69
#define HK_OP_BOUNDING_BOX_TEST		0x6a
#define HK_OP_NORMAL_BUNCH_TEST		0x6b
#define HK_OP_EVAL_NURB_SURFACE		0x6c
#define HK_OP_EVAL_NURB_CURVE		0x6d
#define HK_OP_WAIT_FOR_VERTICAL_RETRACE	0x6e
#define HK_OP_FLUSH_CONTEXT		0x6f
#define HK_OP_FLUSH_PARTIAL_CONTEXT	0x70
#define HK_OP_FLUSH_RENDERING_PIPE	0x71
#define HK_OP_LOAD_CONTEXT		0x72
#define HK_OP_MOVE_CONTEXT_POINTER	0x73
#define HK_OP_SLEEP			0x74
#define HK_OP_WAIT			0x75
#define HK_OP_TRAP			0x76
#define HK_OP_TRAP_DRAW			0x77
#define HK_OP_TRAP_KERNEL		0x78
#define HK_OP_UPDATE_LUT		0x79
#define HK_OP_NOTIFY_KERNEL		0x7a

/* NOTE: opcodes 0x7b-0x7f are unused */

/* Raster instructions */
#define HK_OP_WINDOW_CLEAR		0x80
#define HK_OP_VIEWPORT_CLEAR		0x81
#define HK_OP_VIEWPORT_COPY_FRONT	0x82
#define HK_OP_DRAW_IMAGE_32		0x83
#define HK_OP_XFORM_DRAW_IMAGE_32	0x84
#define HK_OP_SAVE_IMAGE_32		0x85
#define HK_OP_DRAW_IMAGE_8		0x86
#define HK_OP_XFORM_DRAW_IMAGE_8	0x87
#define HK_OP_SAVE_IMAGE_8		0x88
#define HK_OP_DRAW_IMAGE_1		0x89
#define HK_OP_XFORM_DRAW_IMAGE_1	0x8a
#define HK_OP_ZOOM_NEAREST_NEIGHBOR	0x8b
#define HK_OP_ZOOM_INTERPOLATED		0x8c
#define HK_OP_STOCHASTIC_STEP		0x8d
#define HK_OP_STOCHASTIC_DISPLAY	0x8e

/* NOTE: opcode 0x8f is unused */

/* Undocumented DEBUG-ONLY instructions! MUST BE LAST! */
#define HK_OP_PASS_TO_SU		0x90
#define HK_OP_DIAG_ESCAPE		0x91



/*
 *--------------------------------------------------------------
 *
 * Attribute sub-opcodes
 *
 *	Attribute bit patterns are carefully grouped into sets of
 *	32 to allow nearly all of the attributes to be processed
 *	with table-driven code.  Any changes to these patterns require
 *	corresponding changes to the tables in fe_jump_tables.c and
 *	could affect code in fe_attribute.c also.
 *
 *	To build a proper attribute command, use the base attribute
 *	definition (HK_OP_SET_ATTR_I, HK_OP_SET_ATTR_B or HK_OP_SET_ATTR_A).
 *	To dispatch on the codes, use the full 8-bit pattern (already in
 *	op_jump_table).
 *
 *	The attributes are in a similar order to that found in the
 *	document: Hawk Instruction Set 1.5.  However, they are first
 *	grouped by bundle-able vs. unbundled, single-word vs. multi-word,
 *	and those requiring special processing.
 *
 *--------------------------------------------------------------
 */

/* Single word - bundle-able attributes (with op-codes 0x40, 0x48, 0x50) */

#define HK_LINE_ANTIALIASING		0x00
#define HK_LINE_SHADING_METHOD		0x01
#define HK_LINE_STYLE			0x02
#define HK_LINE_PATTERN			0x03
#define HK_LINE_WIDTH			0x04
#define HK_LINE_CAP			0x05
#define HK_LINE_JOIN			0x06
#define HK_LINE_MITER_LIMIT		0x07
#define HK_FRONT_TRANSPARENCY_DEGREE	0x08
#define HK_BACK_TRANSPARENCY_DEGREE	0x09
#define HK_FRONT_SHADING_METHOD		0x0a
#define HK_BACK_SHADING_METHOD		0x0b
#define HK_FRONT_LIGHTING_DEGREE	0x0c
#define HK_BACK_LIGHTING_DEGREE		0x0d
#define HK_FRONT_INTERIOR_STYLE		0x0e
#define HK_BACK_INTERIOR_STYLE		0x0f
#define HK_HOLLOW_ANTIALIASING		0x10
#define HK_EDGE				0x11
#define HK_EDGE_ANTIALIASING		0x12
#define HK_EDGE_STYLE			0x13
#define HK_EDGE_PATTERN			0x14
#define HK_EDGE_WIDTH			0x15
#define HK_EDGE_CAP			0x16
#define HK_EDGE_JOIN			0x17
#define HK_EDGE_MITER_LIMIT		0x18
#define HK_TEXT_ANTIALIASING		0x19
#define HK_TEXT_EXPANSION_FACTOR	0x1a
#define HK_TEXT_SPACING			0x1b
#define HK_TEXT_LINE_WIDTH		0x1c
#define HK_TEXT_CAP			0x1d
#define HK_TEXT_JOIN			0x1e
#define HK_TEXT_MITER_LIMIT		0x1f

/* Note: this table is full.  Overflows must go in the next table. */

/* Multiple word - bundle-able attributes (with op-codes 0x41, 0x49, 0x51) */

#define HK_MARKER_ANTIALIASING		0x20	/* This is really single */
#define HK_MARKER_SIZE			0x21	/* This is really single */
#define HK_MARKER_TYPE			0x22	/* This is really single */
#define HK_LINE_COLOR			0x23
#define HK_CURVE_APPROX			0x24
#define HK_FRONT_SURFACE_COLOR		0x25
#define HK_BACK_SURFACE_COLOR		0x26
#define HK_FRONT_MATERIAL_PROPERTIES	0x27
#define HK_BACK_MATERIAL_PROPERTIES	0x28
#define HK_FRONT_SPECULAR_COLOR		0x29
#define HK_BACK_SPECULAR_COLOR		0x2a
#define HK_FRONT_GENERAL_STYLE		0x2b
#define HK_BACK_GENERAL_STYLE		0x2c
#define HK_FRONT_HATCH_STYLE		0x2d
#define HK_BACK_HATCH_STYLE		0x2e
#define HK_ISO_CURVE_INFO		0x2f
#define HK_SURF_APPROX			0x30
#define HK_TRIM_APPROX			0x31
#define HK_EDGE_COLOR			0x32
#define HK_TEXT_COLOR			0x33
#define HK_TEXT_CHARACTER_SET		0x34
#define HK_TEXT_FONT			0x35
#define HK_MARKER_COLOR			0x36

/* Single word - unbundled attributes (with op-codes 0x42, 0x43) */

#define HK_LINE_GEOM_FORMAT		0x40
#define HK_TRI_GEOM_FORMAT		0x41
#define HK_TRANSPARENCY_QUALITY		0x42
#define HK_FACE_CULLING_MODE		0x43
#define HK_USE_BACK_PROPS		0x44
#define HK_SILHOUETTE_EDGE		0x45
#define HK_EDGE_Z_OFFSET		0x46
#define HK_TEXT_FONT_TABLE		0x47
#define HK_RTEXT_PATH			0x48
#define HK_RTEXT_HEIGHT			0x49
#define HK_RTEXT_SLANT			0x4a
#define HK_ATEXT_PATH			0x4b
#define HK_ATEXT_HEIGHT			0x4c
#define HK_ATEXT_SLANT			0x4d
#define HK_ATEXT_STYLE			0x4e
#define HK_MARKER_GEOM_FORMAT		0x4f
#define HK_MARKER_SNAP_TO_GRID		0x50
#define HK_WINDOW_BOUNDARY		0x51
#define HK_GUARD_BAND			0x52
#define HK_Z_BUFFER_COMPARE		0x53
#define HK_Z_BUFFER_UPDATE		0x54
#define HK_DEPTH_CUE_ENABLE		0x55
#define HK_MCP_MASK			0x56
#define HK_NUM_MCP			0x57
#define HK_TRAVERSAL_MODE		0x58
#define HK_PICK_RESULTS_BUFFER		0x59
#define HK_HSVN				0x5a
#define HK_CSVN				0x5b
#define HK_CEN				0x5c
#define HK_UPICK_ID			0x5d
#define HK_ECHO_SVN			0x5e
#define HK_ECHO_EN			0x5f
#define HK_ECHO_UPICK_ID		0x60
#define HK_ECHO_TYPE			0x61
#define HK_NON_ECHO_INVISIBLE		0x62
#define HK_ECHO_INVISIBLE		0x63
#define HK_FORCE_ECHO_COLOR		0x64
#define HK_RASTER_OP			0x65
#define HK_PLANE_GROUP			0x66
#define HK_PLANE_MASK			0x67
#define HK_DRAW_BUFFER			0x68
#define HK_FAST_CLEAR_SET		0x69
#define HK_CURRENT_WID			0x6a
#define HK_WID_CLIP_MASK		0x6b
#define HK_WID_WRITE_MASK		0x6c
#define HK_STEREO_MODE			0x6d
#define HK_INVISIBILITY			0x6e
#define HK_TRANSPARENCY_MODE		0x6f
#define HK_STROKE_ANTIALIASING		0x70
#define HK_STOCHASTIC_ENABLE		0x71
#define HK_STENCIL_TRANSPARENT		0x72
#define HK_STACK_LIMIT			0x73
#define HK_SCRATCH_BUFFER		0x74
#define HK_ECHO_STRUCTURE		0x75
#define HK_ECHO_END_EN			0x76
#define HK_UNDRAW			0x77
/* Note: The above group has more than 32 entries, so it uses two op-codes */
/* in the main dispatch table */

/* Multiple word - unbundled attributes (with op-codes 0x44, 0x45) */

#define HK_LINE_OFF_COLOR		0x80
#define HK_SURFACE_NORMAL		0x81
#define HK_EDGE_OFF_COLOR		0x82
#define HK_RTEXT_UP_VEC			0x83
#define HK_RTEXT_ALIGNMENT		0x84
#define HK_ATEXT_UP_VEC			0x85
#define HK_ATEXT_ALIGNMENT		0x86
#define HK_LOAD_GMT			0x87
#define HK_LOAD_LMT			0x88
#define HK_PRE_CONCAT_LMT		0x89
#define HK_POST_CONCAT_LMT		0x8a
#define HK_CMT_TO_GMT			0x8b
#define HK_ASPECT_RATIO			0x8c
#define HK_VIEW				0x8d
#define HK_Z_RANGE			0x8e
#define HK_DEPTH_CUE_PARAMETERS		0x8f
#define HK_DEPTH_CUE_COLOR		0x90
#define HK_LIGHT_MASK			0x91
#define HK_LIGHT_SOURCE			0x92
#define HK_ADD_MCP			0x93
#define HK_NAME_SET			0x94
#define HK_INVISIBILITY_FILTER		0x95
#define HK_HIGHLIGHTING_FILTER		0x96
#define HK_PICK_FILTER			0x97
#define HK_CLEAR_PICK_RESULTS		0x98
#define HK_PICK_BOX			0x99
#define HK_ECHO_COLOR			0x9a
#define HK_WINDOW_BG_COLOR		0x9b
#define HK_STOCHASTIC_SETUP		0x9c
#define HK_HIGHLIGHT_COLOR		0x9d
#define HK_STENCIL_FG_COLOR		0x9e
#define HK_STENCIL_BG_COLOR		0x9f
#define HK_CLIPPING_LIMITS		0xa0
/* Note: The above group has more than 32 entries, so it uses two op-codes */
/* in the main dispatch table */

/* The last one is reserved to help catch certain error conditions: */
#define HK_ILLEGAL_ATTRIBUTE		0xff



/*
 *--------------------------------------------------------------
 *
 * Lookup table sub-opcodes
 *
 *	These are the few sub-opcodes for the UPDATE_LUT instruction.
 *
 *--------------------------------------------------------------
 */

#define HK_UPDATE_WLUT		0x00
#define HK_UPDATE_CLUT		0x01
#define HK_UPDATE_FCBG		0x02



/*
 *--------------------------------------------------------------
 *
 * Various size constants
 *
 *--------------------------------------------------------------
 */

/* Number of state bits for push/pop state and restore partial context */
#define HKST_NUM_WORDS		8

/* Constant for 20x16 screen door (20 words each using the lower 16 bits) */
#define HK_SD_MAX		20

/* Maximum lights that may be defined */
#define HK_MAX_LIGHTS		32

/* Maximum model planes that may be defined */
#define HK_MAX_MODEL_PLANES	16

/* Maximum segments in a pattern definition */
#define HK_PATTERN_MAX_SEGS	64

/* Maximum curve order */
#define HK_MAXORD		10

/* Maximum number of knots and control points in a curve */
#define HK_MAX_KNOTS		100
#define HK_MAX_CNTRL_PTS   	100

/* Number of 32-bit words for phigs filters/name_set.  (1024 bits total) */
#define HK_NUMBER_FILTER_WORDS	32

/* Offsets into the (linear) matrix */
#define HKM_0_0	0
#define HKM_0_1	1
#define HKM_0_2	2
#define HKM_0_3	3
#define HKM_1_0	4
#define HKM_1_1	5
#define HKM_1_2	6
#define HKM_1_3	7
#define HKM_2_0	8
#define HKM_2_1	9
#define HKM_2_2	10
#define HKM_2_3	11
#define HKM_3_0	12
#define HKM_3_1	13
#define HKM_3_2	14
#define HKM_3_3	15

/*
 * Stochastic Sample anti-aliasing related.  Precision reflects the
 * number of bits *per color* used for the convolution arithmetic.
 */
#define HKSS_FILTER_SIZE_X	3
#define HKSS_FILTER_SIZE_Y	HKSS_FILTER_SIZE_X
#define HKSS_FILTER_SIZE	(HKSS_FILTER_SIZE_X * HKSS_FILTER_SIZE_Y)
#define HKSS_FILTER_PRECISION  	24
#define HKSS_ACCUM_PRECISION   	16
#define HKSS_RASTER_PRECISION   8
#define HKSS_FIRST_PASS		0	/* value for ss_pass_num on 1st pass */

/* Z buffer range */
#define HK_Z_RANGE_SHIFT	8		/* Number of bits to shift */
#define HK_Z_BITS		24		/* Number of bits in Z buffer */
#define HK_NUM_Z_VALUES		(1 << HK_Z_BITS) /* Range of Z buffer values */

/* diag_escape magic number */
#define HK_ESC_I860_MAGIC	0x014D0000	/* magic number i860 coff */
#define HK_ESC_I860_MAGIC_MASK	0xFFFF0000	/* magic number mask */



/*-------------------------------------------------------------
 *
 *  Invisibility register reserved bit patterns
 *
 *------------------------------------------------------------
 */

#define HK_INV_INVIS_FILTER	0x00010000
#define HK_INV_PICK_FILTER	0x00020000
#define HK_INV_ECHO		0x00040000
#define HK_INV_NON_ECHO		0x00080000


/*
 *--------------------------------------------------------------
 *
 * Enumerations for attributes
 *
 *--------------------------------------------------------------
 */

/* Line geometry format */
#ifndef LOCORE
typedef int Hk_line_geom_format;
#endif !LOCORE
#define HK_LINE_XYZ			0
#define HK_LINE_XYZ_RGB			1
#define HK_LINE_H_XYZ			2
#define HK_LINE_H_XYZ_RGB		3
/* Bit positions within line_geom_format */
#define HK_LINE_RGB_PRESENCE		0x1
#define HK_LINE_HEADER_PRESENCE		0x2

/* Line shading method */
#ifndef LOCORE
typedef int Hk_line_shading_method;
#endif !LOCORE
#define HK_LINE_SHADING_NONE		0
#define HK_LINE_SHADING_INTERPOLATE	1

/* Line style */
#ifndef LOCORE
typedef int Hk_line_style;
#endif !LOCORE
#define HK_LINESTYLE_SOLID		0
#define HK_LINESTYLE_UNBALANCE		4
#define HK_LINESTYLE_UNBALANCE_OFFCOLOR 5
#define HK_LINESTYLE_BALANCE		6
#define HK_LINESTYLE_BALANCE_OFFCOLOR	7

/* Line cap */
#ifndef LOCORE
typedef int Hk_line_cap;
#endif !LOCORE
#define HK_CAP_BUTT			0
#define HK_CAP_SQUARE			1
#define HK_CAP_ROUND			2

/* Line join */
#ifndef LOCORE
typedef int Hk_line_join;
#endif !LOCORE
#define HK_JOIN_BUTT			0
#define HK_JOIN_BEVEL			1
#define HK_JOIN_MITER			2
#define HK_JOIN_ROUND			3

/* Curve approximation type */
#ifndef LOCORE
typedef int Hk_curve_type;
#endif !LOCORE
#define HK_CONST_PARAM_SUBDIVISION	0
#define HK_CONST_PARAM_BETWEEN_KNOTS	1
#define HK_METRIC_WC			2
#define HK_METRIC_NPC			3
#define HK_CHORDAL_DEV_WC		4
#define HK_CHORDAL_DEV_NPC		5

/* Surface approximation type */
#ifndef LOCORE
typedef int Hk_surf_type;
#endif !LOCORE
#define HK_SURF_CONST_PARAM_SUBDIVISION	0
#define HK_SURF_CONST_PARAM_BETWEEN_KNOTS 1
#define HK_SURF_METRIC_WC		2
#define HK_SURF_METRIC_NPC		3
#define HK_SURF_PLANAR_DEV_WC		4
#define HK_SURF_PLANAR_DEV_NPC		5

/* Triangle geometry format */
#ifndef LOCORE
typedef int Hk_tri_geom_format;
#endif !LOCORE
#define HK_TRI_VERTEX_NORMAL_PRESENCE	0x01
#define HK_TRI_FACET_NORMAL_PRESENCE	0x02
#define HK_TRI_VERTEX_COLOR_PRESENCE	0x04
#define HK_TRI_FACET_COLOR_PRESENCE	0x08

#define HK_TRI_XYZ_VNORMAL		HK_TRI_VERTEX_NORMAL_PRESENCE
#define HK_TRI_XYZ_FNORMAL		HK_TRI_FACET_NORMAL_PRESENCE
#define HK_TRI_XYZ_VFNORMAL \
	(HK_TRI_VERTEX_NORMAL_PRESENCE | HK_TRI_FACET_NORMAL_PRESENCE)
#define HK_TRI_XYZ_VNORMAL_VCOLOR \
	(HK_TRI_VERTEX_NORMAL_PRESENCE | HK_TRI_VERTEX_COLOR_PRESENCE)
#define HK_TRI_XYZ_FNORMAL_VCOLOR \
	(HK_TRI_FACET_NORMAL_PRESENCE | HK_TRI_VERTEX_COLOR_PRESENCE)
#define HK_TRI_XYZ_VNORMAL_FCOLOR \
	(HK_TRI_VERTEX_NORMAL_PRESENCE | HK_TRI_FACET_COLOR_PRESENCE)
#define HK_TRI_XYZ_FNORMAL_FCOLOR \
	(HK_TRI_FACET_NORMAL_PRESENCE | HK_TRI_FACET_COLOR_PRESENCE)
#define HK_TRI_XYZ_VFNORMAL_VFCOLOR \
	(HK_TRI_VERTEX_NORMAL_PRESENCE | HK_TRI_FACET_NORMAL_PRESENCE | \
	 HK_TRI_VERTEX_COLOR_PRESENCE | HK_TRI_FACET_COLOR_PRESENCE)

/* Polygon interior style */
#ifndef LOCORE
typedef int Hk_interior_style;
#endif !LOCORE
#define HK_POLYGON_SOLID		0
#define HK_POLYGON_HOLLOW		1
#define HK_POLYGON_HATCHED		2
#define HK_POLYGON_EMPTY		3
#define HK_POLYGON_GENERAL		4

/* Polygon shading method */
#ifndef LOCORE
typedef int Hk_poly_shading_method;
#endif !LOCORE
#define HK_SHADING_FLAT			0
#define HK_SHADING_GOURAUD		1
#define HK_SHADING_PHONG		2

/* Polygon lighting degree */
#ifndef LOCORE
typedef int Hk_lighting_degree;
#endif !LOCORE
#define HK_NO_LIGHTING			0
#define HK_AMBIENT_LIGHTING		1
#define HK_DIFFUSE_LIGHTING		2
#define HK_SPECULAR_LIGHTING		3

/* Polygon culling mode */
#ifndef LOCORE
typedef int Hk_cull_mode;
#endif !LOCORE
#define HK_CULL_NONE			0
#define HK_CULL_BACKFACE		1
#define HK_CULL_FRONTFACE		2

/* Polygon transparency quality */
#ifndef LOCORE
typedef int Hk_transparency_quality;
#endif !LOCORE
#define HK_NO_TRANSPARENCY		(-1)
#define HK_SCREEN_DOOR			0
#define HK_ALPHA_BLEND			1

/* Polygon edge control */
#ifndef LOCORE
typedef int Hk_edge_control;
#endif !LOCORE
#define HK_EDGE_NONE			0
#define HK_EDGE_SELECTED		1
#define HK_EDGE_ALL			2

/* Text path */
#ifndef LOCORE
typedef int Hk_text_path;
#endif !LOCORE
#define HK_TP_RIGHT			0
#define HK_TP_LEFT			1
#define HK_TP_UP			2
#define HK_TP_DOWN			3

/* Text horizontal alignment */
#ifndef LOCORE
typedef int Hk_text_h_alignment;
#endif !LOCORE
#define HK_AH_NORMAL			0
#define HK_AH_LEFT			1
#define HK_AH_CENTER			2
#define HK_AH_RIGHT			3

/* Text vertical alignment */
#ifndef LOCORE
typedef int Hk_text_v_alignment;
#endif !LOCORE
#define HK_AV_NORMAL			0
#define HK_AV_TOP			1
#define HK_AV_CAP			2
#define HK_AV_HALF			3
#define HK_AV_BASE			4
#define HK_AV_BOTTOM			5

/* Annotation text style */
#ifndef LOCORE
typedef int Hk_atext_style;
#endif !LOCORE
#define HK_ATEXT_NONE			0
#define HK_ATEXT_LEAD_LINE		1

/* Aspect ratio type */
#ifndef LOCORE
typedef int Hk_aspect;
#endif !LOCORE
#define HK_ASPECT_NONE			0
#define HK_ASPECT_LOWER_LEFT		1

/* Z-buffer update */
#ifndef LOCORE
typedef int Hk_z_buffer_update;
#endif !LOCORE
#define HK_Z_UPDATE_NONE		0
#define HK_Z_UPDATE_NORMAL		1
#define HK_Z_UPDATE_ALL			2

/* Traversal mode */
#ifndef LOCORE
typedef int Hk_traversal_mode;
#endif !LOCORE
#define HK_RENDER			0
#define HK_PICK_WITHOUT_RENDER		1
#define HK_PICK_WITHOUT_RENDER_P	2
#define HK_PICK_WHILE_RENDER		3
#define HK_PICK_WHILE_RENDER_P		4
#define HK_PICK_ECHO			5

/* Echo type */
#ifndef LOCORE
typedef int Hk_echo_type;
#endif !LOCORE
#define HK_PICK_ECHO_EN			0
#define HK_PICK_ECHO_SVN		1
#define HK_PICK_ECHO_SVN_PLUS_DESCENDANTS 2
#define HK_PICK_ECHO_SVN_WITH_UPICK_ID	3
#define HK_PICK_ECHO_SVN_CONTIGUOUS_UPICK_ID 4
#define HK_PICK_ECHO_SVN_EN_RANGE	5
#define HK_PICK_ECHO_STRUCT_PLUS_DESCENDANTS 6
#define HK_PICK_ECHO_STRUCT_EN_RANGE_PLUS_DESCENDANTS 7

/* Light source type */
#ifndef LOCORE
typedef int Hk_light_type;
#endif !LOCORE
#define HK_AMBIENT_LIGHT_SOURCE		0
#define HK_DIRECTIONAL_LIGHT_SOURCE	1
#define HK_POSITIONAL_LIGHT_SOURCE	2
#define HK_SPOT_LIGHT_SOURCE		3

/* Transparency mode */
#ifndef LOCORE
typedef int Hk_transparency_mode;
#endif !LOCORE
#define HK_OPAQUE_VISIBLE		0
#define HK_TRANSPARENT_VISIBLE		1
#define HK_ALL_VISIBLE			2

/* Stroke antialiasing */
#ifndef LOCORE
typedef int Hk_stroke_antialiasing;
#endif !LOCORE
#define HK_AA_INDIVIDUAL		(-1)
#define HK_AA_NONE			0
#define HK_AA_CONSTANT			1
#define HK_AA_ARBITRARY			2

/* Raster-op */
#ifndef LOCORE
typedef int Hk_raster_op;
#endif !LOCORE
#define HK_ROP_CLR			0x00
#define HK_ROP_SRC_NOR_DST		0x01
#define HK_ROP_NSRC_AND_DST		0x02
#define HK_ROP_NOT_SRC			0x03
#define HK_ROP_SRC_AND_NDST		0x04
#define HK_ROP_NOT_DST			0x05
#define HK_ROP_SRC_XOR_DST		0x06
#define HK_ROP_SRC_NAND_DST		0x07
#define HK_ROP_SRC_AND_DST		0x08
#define HK_ROP_SRC_XNOR_DST		0x09
#define HK_ROP_DST			0x0a
#define HK_ROP_NSRC_OR_DST		0x0b
#define HK_ROP_SRC			0x0c
#define HK_ROP_SRC_OR_NDST		0x0d
#define HK_ROP_SRC_OR_DST		0x0e
#define HK_ROP_SET			0x0f

/*
 ***********************************************************************
 * WARNING: The following typedefs must be kept in sync with the
 * corresponding GT typedefs: Hk_buffer, Hk_plane_group, Hk_wlut_update,
 * and Hk_clut_update.
 ***********************************************************************
 */

/* Plane group */
#ifndef LOCORE
typedef int Hk_plane_group;
#endif !LOCORE
#define HK_8BIT_RED			0
#define HK_8BIT_GREEN			1
#define HK_8BIT_BLUE			2
#define HK_8BIT_OVERLAY			3
#define HK_24BIT			4

/* Draw buffer */
#ifndef LOCORE
typedef int Hk_buffer;
#endif !LOCORE
#define HK_BUFFER_A			0
#define HK_BUFFER_B			1

/* Stereo mode */
#ifndef LOCORE
typedef int Hk_stereo_mode;
#endif !LOCORE
#define HK_STEREO_NONE			0
#define HK_STEREO_LEFT			1
#define HK_STEREO_RIGHT			2

/* Spline basis function */
#ifndef LOCORE
typedef int Hk_basis;
#endif !LOCORE
#define HK_BASIS_BSPLINES		0
#define HK_BASIS_BEZIER			1

/* Curve placement */
#ifndef LOCORE
typedef int Hk_curve_placement;
#endif !LOCORE
#define HK_UNIFORM			0
#define HK_NON_UNIFORM			1

/* Matrix type */
#ifndef LOCORE
typedef int Hk_matrix_type;
#endif !LOCORE
#define HK_AFFINE_MATRIX		0
#define HK_NON_AFFINE_MATRIX		1

/* Window lookup table mask bits */
#define HK_WLUT_MASK_IMAGE		0x01
#define HK_WLUT_MASK_OVERLAY		0x02
#define HK_WLUT_MASK_PLANE_GROUP	0x04
#define HK_WLUT_MASK_CLUT		0x08
#define HK_WLUT_MASK_FCS		0x10

/* Fast clear none */
#define HK_FCS_NONE			(-1)

/* Default CLUT table for WLUT updates */
#define HK_DEFAULT_CLUT			(-1)



#ifndef LOCORE
/*
 *--------------------------------------------------------------
 *
 * Typedefs for geometry structures
 *
 *--------------------------------------------------------------
 */

/* Polyline vertex */
typedef struct Hk_vertex Hk_vertex;
struct Hk_vertex {
	float x, y, z;
};

/* Polyline vertex with color */
typedef struct Hk_rgb_vertex Hk_rgb_vertex;
struct Hk_rgb_vertex {
	float x, y, z;
	float r, g, b;
};

/* Polyline vertex with header */
typedef struct Hk_vertex_h Hk_vertex_h;
struct Hk_vertex_h {
	unsigned header;
	float x, y, z;
};

/* Polyline vertex with color and header */
typedef struct Hk_rgb_vertex_h Hk_rgb_vertex_h;
struct Hk_rgb_vertex_h {
	unsigned header;
	float x, y, z;
	float r, g, b;
};

/* Triangle vertex with normal */
typedef struct Hk_tri_normal Hk_tri_normal;
struct Hk_tri_normal {
	unsigned tri_header;
	float x, y, z;
	float nx, ny, nz;
};

/* Triangle vertex with normal and color */
typedef struct Hk_tri_normal_color Hk_tri_normal_color;
struct Hk_tri_normal_color {
	unsigned tri_header;
	float x, y, z;
	float nx, ny, nz;
	float r, g, b;
};

/* Triangle vertex with 2 normals */
typedef struct Hk_tri_vfnormal Hk_tri_vfnormal;
struct Hk_tri_vfnormal {
	unsigned tri_header;
	float x, y, z;
	float vnx, vny, vnz;
	float fnx, fny, fnz;
};

/* Triangle vertex with 2 normals and 2 colors */
typedef struct Hk_tri_vfnormal_vfcolor Hk_tri_vfnormal_vfcolor;
struct Hk_tri_vfnormal_vfcolor {
	unsigned tri_header;
	float x, y, z;
	float vnx, vny, vnz;
	float fnx, fny, fnz;
	float vr, vg, vb;
	float fr, fg, fb;
};

/*
 * WARNING:  The following structures contains doubles and must be padded
 * so that all doubles start on an even 8 byte (2 word) boundary, and
 * the sizeof(struct) is a multiple of 8 bytes.
 */

/* Curve control point */
typedef struct Hk_dcoord4 Hk_dcoord4;
struct Hk_dcoord4 {
	double x, y, z, w;
};



/*
 *--------------------------------------------------------------
 *
 * Typedefs for attribute structures
 *
 *--------------------------------------------------------------
 */

/* Color */
typedef struct Hk_rgb Hk_rgb;
struct Hk_rgb {
	float r;
	float g;
	float b;
};

/* Position vector */
typedef struct Hk_xyz Hk_xyz;
struct Hk_xyz {
	float x;
	float y;
	float z;
};

/* Direction vector */
typedef struct Hk_normal Hk_normal;
struct Hk_normal {
	float nx;
	float ny;
	float nz;
};

/* Line pattern */
typedef struct Hk_pattern Hk_pattern;
struct Hk_pattern {
	float offset;
	int num_segments;
	float on_off_seg[1];
};

/*
 * WARNING:  The following structures contains doubles and must be
 * padded so that all doubles start on an even 8 byte (2 word)
 * boundary, and the sizeof(struct) is a multiple of 8 bytes.
 */

/* Curve approximation */
typedef struct Hk_curve_approx Hk_curve_approx;
typedef struct Hk_curve_approx Hk_trim_approx;
struct Hk_curve_approx {
	double approx_value;
	Hk_curve_type approx_type;
	int padding;		/* Make sizeof(struct) on 8-byte boundary */
};

/* Surface approximation */
typedef struct Hk_surf_approx Hk_surf_approx;
struct Hk_surf_approx {
	double u_approx_value;
	double v_approx_value;
	Hk_surf_type approx_type;
	int padding;		/* Make sizeof(struct) on 8-byte boundary */
};

/* General style */
typedef struct Hk_general_style Hk_general_style;
struct Hk_general_style {
	Hk_interior_style real_style;
	unsigned general_info;
};

/* Isoparametric curve info */
typedef struct Hk_iso_curve_info Hk_iso_curve_info;
struct Hk_iso_curve_info {
	int flag;
	Hk_curve_placement placement;
	int u_count;
	int v_count;
};

/* Polygon material properties */
typedef struct Hk_material_properties Hk_material_properties;
struct Hk_material_properties {
	float ambient_reflection_coefficient;
	float diffuse_reflection_coefficient;
	float specular_reflection_coefficient;
	float specular_exponent;
};

/* Polygon hatch style */
typedef struct Hk_hatch_style Hk_hatch_style;
struct Hk_hatch_style {
	int hatch_transparency;
	unsigned hatch_pattern[HK_SD_MAX];
};

/* Glyph */
typedef struct Hk_glyph Hk_glyph;
struct Hk_glyph {
	float width;
	unsigned *char_pkt;		/* Pointer to polyline commands */
};

/* Font */
typedef struct Hk_font Hk_font;
struct Hk_font {
	float top_line;
	float bottom_line;
	float max_width;
	int num_ch;
	Hk_glyph *ch_data[1];		/* [num_ch] */
};

/* Font table */
typedef struct Hk_font_table Hk_font_table;
struct Hk_font_table {
	int num_char_sets;
	int num_font_ids;
	Hk_font *desc[1];		/* [num_char_sets][num_font_ids] */
};

/* Text character set */
typedef struct Hk_character_set Hk_character_set;
struct Hk_character_set {
	int codeset;
	int character_set;
};

/* Text font ID */
typedef struct Hk_font_id Hk_font_id;
struct Hk_font_id {
	int codeset;
	int font_id;
};

/* Text up vector */
typedef struct Hk_xy Hk_xy;
struct Hk_xy {
	float x;
	float y;
};

/* Text alignment */
typedef struct Hk_text_alignment Hk_text_alignment;
struct Hk_text_alignment {
	Hk_text_h_alignment halign;
	Hk_text_v_alignment valign;
};

/* Marker geometry */
typedef struct Hk_marker Hk_marker;
struct Hk_marker {
	float size;
	unsigned *packet;		/* Pointer to polyline commands */
};

/* Marker definition */
typedef struct Hk_marker_type Hk_marker_type;
struct Hk_marker_type {
	int num_tessellations;
	Hk_marker markers[1];		/* [num_tessellations] */
};

/* Depth-cue parameters */
typedef struct Hk_depth_cue_parameters Hk_depth_cue_parameters;
struct Hk_depth_cue_parameters {
	float z_front;
	float z_back;
	float scale_front;
	float scale_back;
};

/* Matrix definitions */
typedef float Hk_fmatrix[16];
typedef double Hk_dmatrix[16];

typedef struct Hk_model_transform Hk_model_transform;
struct Hk_model_transform {
	Hk_dmatrix matrix;
	double scale;
};

/* Aspect ratio definition */
typedef struct Hk_aspect_ratio Hk_aspect_ratio;
struct Hk_aspect_ratio {
	Hk_aspect flag;
	float ratio;
};

/* Window boundary */
typedef struct Hk_window_boundary Hk_window_boundary;
struct Hk_window_boundary {
	int xleft;
	int ytop;
	int width;
	int height;
};

/*
 * WARNING:  The following structure contains doubles and must be
 * padded so that all doubles start on an even 8 byte (2 word)
 * boundary, and the sizeof(struct) is a multiple of 8 bytes.
 */
/* Viewport definition */
typedef struct Hk_view Hk_view;
struct Hk_view {
	float xcenter;			/* Viewport center */
	float ycenter;
	float xsize;			/* Half size */
	float ysize;
	Hk_dmatrix vt;			/* Viewing matrix */
	Hk_dmatrix ivt;			/* Inverse of vt */
	double scale_xy;		/* Max of X and Y scale */
	double scale_z;			/* ZScale(vt) * Sign(vt) */
};

/* NPC clipping limits definition */
typedef struct Hk_clipping_limits Hk_clipping_limits;
struct Hk_clipping_limits {
	float xmin;
	float ymin;
	float zmin;
	float xmax;
	float ymax;
	float zmax;
};

/* Z-range */
typedef struct Hk_z_range Hk_z_range;
struct Hk_z_range {
	unsigned zfront;
	unsigned zback;
};

/* PHIGS filter */
typedef unsigned Hk_phigs_filter[HK_NUMBER_FILTER_WORDS];

/* Name set */
typedef struct Hk_name_set_update Hk_name_set_update;
struct Hk_name_set_update {
	Hk_phigs_filter del_mask;
	Hk_phigs_filter add_mask;
};

/* Filter pair */
typedef struct Hk_filter_pair Hk_filter_pair;
struct Hk_filter_pair {
	Hk_phigs_filter inclusion;
	Hk_phigs_filter exclusion;
};

/* Model clipping plane */
typedef struct Hk_mcp Hk_mcp;
struct Hk_mcp {
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
};

/* Pick hit */
typedef struct Hk_pick_hit Hk_pick_hit;
struct Hk_pick_hit {
	int picked_upick_id;
	int picked_svn;
	int start_en;
	int picked_en;
};

/* Pick results buffer */
typedef struct Hk_pick_hits Hk_pick_hits;
struct Hk_pick_hits {
	int max_num_pick_hits;
	int circular_writes;
	int pick_hits_detected;
	Hk_pick_hit pick_hit_item[1];	/* [max_num_pick_hits] */
};

/* Pick box */
typedef struct Hk_pick_box Hk_pick_box;
struct Hk_pick_box {
	float x_left;
	float y_bottom;
	float z_back;
	float x_right;
	float y_top;
	float z_front;
};

/* Light mask update */
typedef struct Hk_light_mask_update Hk_light_mask_update;
struct Hk_light_mask_update {
	unsigned light_mask_del;
	unsigned light_mask_add;
};

/* Light */
typedef struct Hk_light Hk_light;
struct Hk_light {
	Hk_light_type ls_type;
	float lr, lg, lb;		/* Light color (Hk_rgb) */
	float px, py, pz;		/* Light position (Hk_xyz) */
	float dx, dy, dz;		/* Light direction (Hk_xyz) */
	float ls_exp;
	float ls_atten1, ls_atten2;
	float ls_angle;
};

/* Light source */
typedef struct Hk_light_source Hk_light_source;
struct Hk_light_source {
	int light_num;
	Hk_light light;
};

/* Surface tessellation info */
typedef struct Hk_surface_tessellation Hk_surface_tessellation;
struct Hk_surface_tessellation {
	Hk_surface_tessellation	**next;
	Hk_surf_type		surf_approx_type;
	Hk_dmatrix		matrix;
	Hk_matrix_type		matrix_type;
	int			trim_curves_exist;
	double			trim_approx_value;
	Hk_curve_type		trim_approx_type;
	Hk_curve_placement	iso_curve_placement;
	int			iso_curve_ucount;
	int 			iso_curve_vcount;
	int			iso_curves_exist;
	float			cutoff_top_u;
	float			cutoff_bottom_u;
	float			cutoff_top_v;
	float			cutoff_bottom_v;
	unsigned		packet[1];
};

/* Curve tessellation info */
typedef struct Hk_curve_tessellation Hk_curve_tessellation;
struct Hk_curve_tessellation {
	Hk_curve_tessellation	**next;
	Hk_curve_type		approx_type;
	Hk_dmatrix		matrix;
	Hk_matrix_type		matrix_type;
	float			cutoff_top;
	float			cutoff_bottom;
	unsigned		packet[1];
};

/* Stochastic setup */
typedef struct Hk_stochastic_setup Hk_stochastic_setup;
struct Hk_stochastic_setup {
	int ss_pass_num;
	int ss_display_flag;
	float ss_norm_scale;
	float ss_x_offset;
	float ss_y_offset;
	float ss_filter_weights[HKSS_FILTER_SIZE_X][HKSS_FILTER_SIZE_Y];
};

/* Scratch buffer for annotation text and spline curves */
typedef struct Hk_scratch_buffer Hk_scratch_buffer;
struct Hk_scratch_buffer {
	int num_words;
	int buffer[1];		/* [num_words] */
};

/* Definition of state attribute mask (used by push/pop state, etc) */
typedef unsigned Hk_state_list[HKST_NUM_WORDS];

/*
 ***********************************************************************
 * WARNING: The following typedefs must be kept in sync with the
 * corresponding GT typedefs: Hk_buffer, Hk_plane_group, Hk_wlut_update,
 * and Hk_clut_update.
 ***********************************************************************
 */

/* Window lookup table update structures */
typedef struct Hk_wlut_update Hk_wlut_update;
struct Hk_wlut_update {
	int entry;
	unsigned mask;
	Hk_buffer image_buffer;
	Hk_buffer overlay_buffer;
	Hk_plane_group plane_group;
	int clut;
	int fast_clear_set;
};

/* Color lookup table update structure */
typedef struct Hk_clut_update Hk_clut_update;
struct Hk_clut_update {
	int table;
	int start_entry;
	int num_entries;
	unsigned colors[1];
};

/* Fast clear background color structure */
typedef struct Hk_fcbg_update Hk_fcbg_update;
struct Hk_fcbg_update {
	int fast_clear_set;
	unsigned color;			/* Packed RGB (actually abgr) */
};
#endif !LOCORE



/*
 *--------------------------------------------------------------
 *
 * Definition of state bits for push/pop state and restore partial ctx
 *
 * Several Hawk instructions take a bit mask which represents a list
 * of attributes to be saved/restored/flushed/etc.  This bit mask is
 * stored as eight 32-bit words and is organized as follows:
 *
 *	  +----------------------+
 *	0 | unbundled attributes |
 *	1 |                      |
 *	  +----------------------+
 *	2 | bundle-able attrs    |
 *	3 |    (individual)      |
 *	  +----------------------+
 *	4 | bundle-able attrs    |
 *	5 |     (bundled)        |
 *	  +----------------------+
 *	6 | bundle-able attrs    |
 *	7 |       (asf)          |
 *	  +----------------------+
 *
 *--------------------------------------------------------------
 */

/* Bit definitions for word 0 of state list (unbundled attributes) */

#define HKST0_OTHER				0x80000000
#define HKST0_LINE_GEOM_FORMAT			0x40000000
#define HKST0_LINE_OFF_COLOR			0x20000000
#define HKST0_TRI_GEOM_FORMAT			0x10000000
#define HKST0_USE_BACK_PROPS			0x08000000
#define HKST0_SILHOUETTE_EDGE			0x04000000
#define HKST0_FACE_CULLING_MODE			0x02000000
#define HKST0_EDGE_OFF_COLOR			0x01000000
#define HKST0_EDGE_Z_OFFSET			0x00800000
#define HKST0_RTEXT_UP_VEC			0x00400000
#define HKST0_RTEXT_PATH			0x00200000
#define HKST0_RTEXT_HEIGHT			0x00100000
#define HKST0_RTEXT_ALIGNMENT			0x00080000
#define HKST0_RTEXT_SLANT			0x00040000
#define HKST0_ATEXT_UP_VEC			0x00020000
#define HKST0_ATEXT_PATH			0x00010000
#define HKST0_ATEXT_HEIGHT			0x00008000
#define HKST0_ATEXT_ALIGNMENT			0x00004000
#define HKST0_ATEXT_STYLE			0x00002000
#define HKST0_ATEXT_SLANT			0x00001000
#define HKST0_MARKER_GEOM_FORMAT		0x00000800
#define HKST0_Z_BUFFER_COMPARE			0x00000400
#define HKST0_Z_BUFFER_UPDATE			0x00000200
#define HKST0_DEPTH_CUE_ENABLE			0x00000100
#define HKST0_DEPTH_CUE_PARAMETERS		0x00000080
#define HKST0_DEPTH_CUE_COLOR			0x00000040
#define HKST0_VIEW				0x00000020
#define HKST0_Z_RANGE				0x00000010
#define HKST0_GUARD_BAND			0x00000008
#define HKST0_LMT				0x00000004
#define HKST0_GMT				0x00000002
#define HKST0_CMT				0x00000001


/* Bit definitions for word 1 of state list (unbundled attributes) */

#define HKST1_NAME_SET				0x80000000
#define HKST1_MCP_MASK				0x40000000
#define HKST1_NUM_MCP				0x20000000
#define HKST1_MCP				0x10000000
#define HKST1_CSVN				0x08000000
#define HKST1_UPICK_ID				0x04000000
#define HKST1_LIGHT_MASK			0x02000000
#define HKST1_INVISIBILITY			0x01000000
#define HKST1_RASTER_OP				0x00800000
#define HKST1_PLANE_MASK			0x00400000
#define HKST1_CURRENT_WID			0x00200000
#define HKST1_WID_CLIP_MASK			0x00100000
#define HKST1_WID_WRITE_MASK			0x00080000
#define HKST1_HIGHLIGHT_COLOR			0x00040000


/*
 * Bit definitions for the following:
 *
 * Word 2 of state list (bundle-able attributes -- individual)
 * Word 4 of state list (bundle-able attributes -- bundled)
 * Word 6 of state list (bundle-able attributes -- asf)
 */

#define HKSTB0_LINE_COLOR			0x80000000
#define HKSTB0_LINE_ANTIALIASING		0x40000000
#define HKSTB0_LINE_SHADING_METHOD		0x20000000
#define HKSTB0_LINE_STYLE			0x10000000
#define HKSTB0_LINE_PATTERN			0x08000000
#define HKSTB0_LINE_WIDTH			0x04000000
#define HKSTB0_LINE_CAP				0x02000000
#define HKSTB0_LINE_JOIN			0x01000000
#define HKSTB0_LINE_MITER_LIMIT			0x00800000
#define HKSTB0_CURVE_APPROX			0x00400000
#define HKSTB0_TEXT_COLOR			0x00200000
#define HKSTB0_TEXT_ANTIALIASING		0x00100000
#define HKSTB0_TEXT_EXPANSION_FACTOR		0x00080000
#define HKSTB0_TEXT_SPACING			0x00040000
#define HKSTB0_TEXT_LINE_WIDTH			0x00020000
#define HKSTB0_TEXT_CAP				0x00010000
#define HKSTB0_TEXT_JOIN			0x00008000
#define HKSTB0_TEXT_MITER_LIMIT			0x00004000
#define HKSTB0_TEXT_CHARACTER_SET		0x00002000
#define HKSTB0_TEXT_FONT			0x00001000
#define HKSTB0_MARKER_COLOR			0x00000800
#define HKSTB0_MARKER_ANTIALIASING		0x00000400
#define HKSTB0_MARKER_SIZE			0x00000200
#define HKSTB0_MARKER_TYPE			0x00000100


/*
 * Bit definitions for the following:
 *
 * Word 3 of state list (bundle-able attributes -- individual)
 * Word 5 of state list (bundle-able attributes -- bundled)
 * Word 7 of state list (bundle-able attributes -- asf)
 */

#define HKSTB1_FRONT_INTERIOR_STYLE		0x80000000
#define HKSTB1_BACK_INTERIOR_STYLE		0x40000000
#define HKSTB1_FRONT_GENERAL_STYLE		0x20000000
#define HKSTB1_BACK_GENERAL_STYLE		0x10000000
#define HKSTB1_FRONT_SURFACE_COLOR		0x08000000
#define HKSTB1_BACK_SURFACE_COLOR		0x04000000
#define HKSTB1_FRONT_SHADING_METHOD		0x02000000
#define HKSTB1_BACK_SHADING_METHOD		0x01000000
#define HKSTB1_FRONT_LIGHTING_DEGREE		0x00800000
#define HKSTB1_BACK_LIGHTING_DEGREE		0x00400000
#define HKSTB1_FRONT_MATERIAL_PROPERTIES	0x00200000
#define HKSTB1_BACK_MATERIAL_PROPERTIES		0x00100000
#define HKSTB1_FRONT_SPECULAR_COLOR		0x00080000
#define HKSTB1_BACK_SPECULAR_COLOR		0x00040000
#define HKSTB1_FRONT_TRANSPARENCY_DEGREE	0x00020000
#define HKSTB1_BACK_TRANSPARENCY_DEGREE		0x00010000
#define HKSTB1_FRONT_HATCH_STYLE		0x00008000
#define HKSTB1_BACK_HATCH_STYLE			0x00004000
#define HKSTB1_HOLLOW_ANTIALIASING		0x00002000
#define HKSTB1_EDGE				0x00001000
#define HKSTB1_EDGE_ANTIALIASING		0x00000800
#define HKSTB1_EDGE_COLOR			0x00000400
#define HKSTB1_EDGE_STYLE			0x00000200
#define HKSTB1_EDGE_PATTERN			0x00000100
#define HKSTB1_EDGE_WIDTH			0x00000080
#define HKSTB1_EDGE_CAP				0x00000040
#define HKSTB1_EDGE_JOIN			0x00000020
#define HKSTB1_EDGE_MITER_LIMIT			0x00000010
#define HKSTB1_SURF_APPROX			0x00000008
#define HKSTB1_TRIM_APPROX			0x00000004
#define HKSTB1_ISO_CURVE_INFO			0x00000002



/*
 *--------------------------------------------------------------
 *
 * Definitions for bit positions within command word
 *
 * The general format for the Hawk data manipulation opcodes:
 *
 *	 3      2    2 1 1 1 1
 *	 1      4    1 9 8 6 5              0
 *	+--------+--+---+---+----------------+
 *	| op     |--|reg|am |       n        |
 *	+--------+--+---+---+----------------+
 *
 * where:
 *	op is the primary opcode
 *	reg is the destination register (source for a store instrustion).
 *	am is the addressing mode (or 2nd register for arithmetic instructions)
 *	n is the count or offset for the command
 *
 * The format for Hawk geometry commands:
 *
 *	 3      2 2      1 1
 *	 1      4 3      6 5                0
 *	+--------+--------+----------------+
 *	| op     | flags  |  vertex_count  |
 *	+--------+--------+----------------+
 *
 * where:
 *	op is the primary opcode
 *	flags is an optional geometry flags parameter
 *	vertex_count is the number of vertices that follow
 *
 * The format for Hawk set attribute opcodes:
 *
 *	 3   2 2      1 1 1 1
 *	 1   7 6      9 8 6 5              0
 *	+-----+--------+---+----------------+
 *	|  op |  attr  |am |     offset     |
 *	+-----+--------+---+----------------+
 *
 * where:
 *	op is the primary opcode (HK_OP_SET_ATTR_*)
 * 	    Note: op is 8 bits normally.
 *	attr is the secondary opcode (the attribute to set)
 *	am is the addressing mode
 *	offset is the offset for register indirect addressing
 *
 * The format for Hawk update lookup table opcodes:
 *
 *	 3      2 22     1 1 1
 *	 1      4 32     8 6 5              0
 *	+--------+--+---+---+----------------+
 *	|  op    |so|---|am |     offset     |
 *	+--------+--+---+---+----------------+
 *
 * where:
 *	op is the primary opcode (HK_OP_UPDATE_LUT)
 *	so is the sub-opcode
 *	am is the addressing mode
 *	offset is the offset for register indirect addressing
 *
 *--------------------------------------------------------------
 */

/* For all commands */
#define HK_OP_POS		24
#define HK_OP_BITS		8
#define HK_NUM_OP		(1 << HK_OP_BITS)
#define HK_OP_MASK		(HK_NUM_OP - 1)
/* Note: The definition of HK_EXTRACT_OP is being simplified to avoid
 * the overhead of masking off bits that are known to be 0.
 * The more general definition is:
 *
 *	#define HK_EXTRACT_OP(inst) (((inst) >> HK_OP_POS) & HK_OP_MASK)
 *
 * The following is guaranteed to work because a right shift of an
 * unsigned integer zero-fills from the left and assures that all
 * bits which we would have masked off are already zero.
 */
#define HK_EXTRACT_OP(inst) (((unsigned) (inst)) >> HK_OP_POS)

/* Register for arithmetic and load/store commands */
#define HK_REG_POS		19
#define HK_REG_BITS		3
#define HK_NUM_REG		(1 << HK_REG_BITS)
#define HK_REG_MASK		(HK_NUM_REG - 1)
#define HK_EXTRACT_REG(inst) (((inst) >> HK_REG_POS) & HK_REG_MASK)

/* Addressing mode or 2nd register */
#define HK_AM_POS		16
#define HK_AM_BITS		3
#define HK_NUM_AM		(1 << HK_AM_BITS)
#define HK_AM_MASK		(HK_NUM_AM - 1)
#define HK_EXTRACT_AM(inst) (((inst) >> HK_AM_POS) & HK_AM_MASK)

/* Signed offset or count */
#define HK_OFFSET_POS		0
#define HK_OFFSET_BITS		16		/* Must be 16 bits */
#define HK_NUM_OFFSET		(1 << HK_OFFSET_BITS)
#define HK_OFFSET_MASK		(HK_NUM_OFFSET - 1)
#define HK_EXTRACT_OFFSET(inst) (((inst) >> HK_OFFSET_POS) & HK_OFFSET_MASK)
#define HK_EXTRACT_SHORT_INT(inst) (((inst) >> HK_OFFSET_POS) & HK_OFFSET_MASK)
#define HK_MAX_OFFSET		(HK_OFFSET_MASK >> 1)	/* +32767 */
#define HK_MIN_OFFSET		(-HK_MAX_OFFSET - 1)	/* -32768 */

/* Unsigned vertex count */
#define HK_VCOUNT_POS		0
#define HK_VCOUNT_BITS		16
#define HK_NUM_VCOUNT		(1 << HK_VCOUNT_BITS)
#define HK_VCOUNT_MASK		(HK_NUM_VCOUNT - 1)
#define HK_EXTRACT_VCOUNT(inst) (((inst) >> HK_VCOUNT_POS) & HK_VCOUNT_MASK)

/* Geometry flags */
#define HK_GFLAGS_POS		16
#define HK_GFLAGS_BITS		8
#define HK_NUM_GFLAGS		(1 << HK_GFLAGS_BITS)
#define HK_GFLAGS_MASK		(HK_NUM_GFLAGS - 1)
#define HK_EXTRACT_GFLAGS(inst) (((inst) >> HK_GFLAGS_POS) & HK_GFLAGS_MASK)

/* Code for TRAP instructions */
#define HK_TCODE_POS		16
#define HK_TCODE_BITS		8
#define HK_NUM_TCODE		(1 << HK_TCODE_BITS)
#define HK_TCODE_MASK		(HK_NUM_TCODE - 1)
#define HK_EXTRACT_TCODE(inst) (((inst) >> HK_TCODE_POS) & HK_TCODE_MASK)

/* Attribute to set for SET_ATTR command */
#define HK_ATTR_POS		19
#define HK_ATTR_BITS		8
#define HK_NUM_ATTR		(1 << HK_ATTR_BITS)
#define HK_ATTR_MASK		(HK_NUM_ATTR - 1)
/* Bits to look at once the main command is decoded */
#define HK_ATTR_SUB_BITS	5
#define HK_NUM_SUB_ATTR		(1 << HK_ATTR_SUB_BITS)
#define HK_ATTR_SUB_MASK	(HK_NUM_SUB_ATTR - 1)
#define HK_EXTRACT_ATTR(inst)	(((inst) >> HK_ATTR_POS) & HK_ATTR_SUB_MASK)

/* ASF bit position for the SET_ATTR command */
#define HK_ASF_POS		0
#define HK_ASF_BITS		1
#define HK_NUM_ASF		(1 << HK_ASF_BITS)
#define HK_ASF_MASK		(HK_NUM_ASF - 1)
#define HK_EXTRACT_ASF(inst)	(((inst) >> HK_ASF_POS) & HK_ASF_MASK)

/* Sub-opcode field for lookup table instructions */
#define HK_SO_POS		22
#define HK_SO_BITS		2
#define HK_NUM_SO		(1 << HK_SO_BITS)
#define HK_SO_MASK		(HK_NUM_SO - 1)
#define HK_EXTRACT_SO(inst)	(((inst) >> HK_SO_POS) & HK_SO_MASK)



/*
 *--------------------------------------------------------------
 *
 * Bit definitions for polyline with header.
 *
 *--------------------------------------------------------------
 */

/*
 * Bits for polyline command word.
 */

/* Implicit current element increment for batched polylines */
#define HK_LINE_IMPLICIT_CEN	0x00020000

/*
 * Bits for polyline header words.
 */

/* Polyline move/draw (i.e. restart) bit */
#define HK_LINE_MOVE		0x00000000	/* Header for line move */
#define HK_LINE_DRAW		0x08000000	/* Header for line draw */

/* Increment CEN bit */
#define HK_LINE_INC_CEN		0x01000000	/* Increment CEN on LINE_MOVE */



/*
 *--------------------------------------------------------------
 *
 * Bit definitions for generalized triangle strips
 *
 *--------------------------------------------------------------
 */

/*
 * Bits for triangle list command word.
 */

/* Triangle list edge flag */
#define HK_TRI_EDGE_ENABLE	0x00010000	/* Use tri_list edge bits */
#define HK_TRI_EDGE_DISABLE	0x00000000	/* DON'T use edge bits */
/* Implicit current element increment for batched triangles in a triangle list */
#define HK_TRI_IMPLICIT_CEN	0x00020000

/*
 * Bits for triangle header words.
 */

/* Triangle replacement bits */
#define HK_TRI_START		0x00000000	/* Start a triangle strip */
#define HK_TRI_R1		0x08000000	/* Replace oldest vertex */
#define HK_TRI_R2		0x10000000	/* Replace 2nd oldest vertex */
#define HK_TRI_COUNTER_CLOCKWISE 0x80000000	/* Triangle is not clockwise */
#define HK_TRI_START_CCW	(HK_TRI_START | HK_TRI_COUNTER_CLOCKWISE)
#define HK_TRI_CLOCKWISE	0x00000000	/* Triangle is clockwise */
#define HK_TRI_START_CW		(HK_TRI_START | HK_TRI_CLOCKWISE)
#define HK_TRI_REPL_MASK	0x18000000
#define HK_TRI_REPL_SHIFT	27
#define HK_EXTRACT_TRI_REPL(inst) ((inst) & HK_TRI_REPL_MASK)

/* Edge highlight bits */
#define HK_TRI_EDGE_HILITE1	0x40000000 /* Highlight current-to-oldest edge*/
#define HK_TRI_EDGE_HILITE2	0x20000000 /* Highlight current-to-2nd edge */
#define HK_TRI_EDGE_MASK	0x60000000
#define HK_TRI_EDGE_SHIFT	29
#define HK_EXTRACT_TRI_EDGE(inst) ((inst) & HK_TRI_EDGE_MASK)

/* Increment CEN bit */
#define HK_TRI_INC_CEN		0x01000000	/* Increment CEN on TRI_START */

/* Hollow polygon bits */
#define HK_TRI_EDGE_INTER1	0x00800000 /* Internal current-to-oldest edge */
#define HK_TRI_EDGE_INTER2	0x00400000 /* Internal current-to-2nd edge */



/*
 *--------------------------------------------------------------
 *
 * Bit definitions for Enter PHIGS Structure (EPS) instruction
 * word.
 *
 *--------------------------------------------------------------
 */

#define HK_EPS_NO_SIS		0x00400000
#define HK_EPS_LEAF		0x00800000



/*
 *--------------------------------------------------------------
 *
 * Hawk "RISC" registers
 *
 *--------------------------------------------------------------
 */

#define HK_NUM_RISC_REGS	8	/* Number of RISC registers */
#define HK_RISC_R0		0x0	/* NOTE: cannot be used as index reg */
#define HK_RISC_R1		0x1	/* NOTE: cannot be used as index reg */
#define HK_RISC_R2		0x2
#define HK_RISC_R3		0x3
#define HK_RISC_R4		0x4
#define HK_RISC_R5		0x5
#define HK_RISC_R6		0x6
#define HK_RISC_R7		0x7
#define HK_RISC_SP		HK_RISC_R7



/*
 *--------------------------------------------------------------
 *
 * Addressing modes
 *
 *--------------------------------------------------------------
 */

#define HK_AM_IMMEDIATE			0x0
#define HK_AM_ABSOLUTE			0x1
#define HK_AM_R2_INDIRECT		0x2
#define HK_AM_R3_INDIRECT		0x3
#define HK_AM_R4_INDIRECT		0x4
#define HK_AM_R5_INDIRECT		0x5
#define HK_AM_R6_INDIRECT		0x6
#define HK_AM_R7_INDIRECT		0x7



/*
 *--------------------------------------------------------------
 *
 * State structure
 *
 *	These are the values that may be pushed onto the stack from
 *	the context.
 *
 *
 *
 *--------------------------------------------------------------
 */

/*
 ***********************************************************************
 * WARNING:  Because the Hk_state structure contains doubles, it must
 * be padded such that ALL embedded structures which contain doubles
 * start on an 8 byte (2 word) boundary.  Additionally, the
 * sizeof(Hk_state) must be a multiple of 8 bytes.  If ANYTHING changes
 * in the Hk_state structure, alignment must again be verified.
 *
 * Additionally, because the Hk_context structure includes Hk_state,
 * the sizeof(Hk_context) must be a multiple of 8 bytes.  If ANYTHING
 * changes in the Hk_context structure, alignment must again be
 * verified.
 *
 * The defined constants HK_STATE_PADDING and HK_CTX_PADDING are used
 * to specify the padding for Hk_state and Hk_context, respectively.
 * These values must be either 1 or 2.  Initialization code will verify
 * that the specified value of both HK_STATE_PADDING and HK_CTX_PADDING
 * are correct.
 *
 * The values HK_SZ_STATE_RES and HK_SZ_CTX_RES specify the number of
 * words in the state and context, respectively, which are reserved
 * for future expansion.  These numbers should be decremented as
 * appropriate when adding new fields to the context.  The value
 * HK_SZ_STATE_RES2 is an additional reserved area in the state which
 * may be used if the first area is exhausted.
 ***********************************************************************
 */

#define HK_STATE_PADDING	1	/* Padding (1 or 2) */
#define HK_CTX_PADDING		1	/* Padding (1 or 2) */

#define HK_SZ_STATE_RES		32	/* Number of reserved words */
#define HK_SZ_STATE_RES2	64	/* Number of reserved words */
#define HK_SZ_CTX_RES		27	/* Number of reserved words */


#ifndef LOCORE
typedef struct Hk_state Hk_state;

struct Hk_state {

/*
 *----------------------------------------------------------------------
 * WARNING:  The following embedded structures contain doubles and must
 * be aligned on an even 8 byte (2 word) boundary.  If ANYTHING changes
 * prior to this point in the Hk_state structure, alignment must again
 * be verified.
 *----------------------------------------------------------------------
 */

/* Curve and surface approximation */

    Hk_curve_approx hki_curve_approx;
    Hk_curve_approx hkb_curve_approx;
    int hka_curve_approx;

    int padding1;			/* Align to 8-byte boundary */

    Hk_surf_approx hki_surf_approx;
    Hk_surf_approx hkb_surf_approx;
    int hka_surf_approx;

    int padding2;			/* Align to 8-byte boundary */

    Hk_trim_approx hki_trim_approx;
    Hk_trim_approx hkb_trim_approx;
    int hka_trim_approx;

    int padding3;			/* Align to 8-byte boundary */

/* Transformations */

    Hk_model_transform gmt;		/* Global modeling transform */
    Hk_model_transform lmt;		/* Local modeling transform */
    Hk_model_transform cmt;		/* Current modeling transform */

    Hk_view view;

/*
 *----------------------------------------------------------------------
 * End of structures containing embedded doubles.
 *----------------------------------------------------------------------
 */

/* PHIGS_state flags */

    unsigned attributes_saved[HKST_NUM_WORDS];
    unsigned need_from_children[HKST_NUM_WORDS];

/* Lines */

    Hk_line_geom_format line_geom_format;

    Hk_stroke_antialiasing hki_line_antialiasing;
    Hk_stroke_antialiasing hkb_line_antialiasing;
    int hka_line_antialiasing;

    Hk_rgb hki_line_color;
    Hk_rgb hkb_line_color;
    int hka_line_color;

    Hk_line_shading_method hki_line_shading_method;
    Hk_line_shading_method hkb_line_shading_method;
    int hka_line_shading_method;

    Hk_line_style hki_line_style;
    Hk_line_style hkb_line_style;
    int hka_line_style;

    Hk_rgb line_off_color;

    Hk_pattern *hki_line_pattern;	/* Pointer to line pattern */
    Hk_pattern *hkb_line_pattern;
    int hka_line_pattern;

    float hki_line_width;
    float hkb_line_width;
    int hka_line_width;

    Hk_line_cap hki_line_cap;
    Hk_line_cap hkb_line_cap;
    int hka_line_cap;

    Hk_line_join hki_line_join;
    Hk_line_join hkb_line_join;
    int hka_line_join;

    float hki_line_miter_limit;
    float hkb_line_miter_limit;
    int hka_line_miter_limit;

/* Surface attributes */

    Hk_tri_geom_format tri_geom_format;

    Hk_cull_mode face_culling_mode;

    int use_back_props;

    Hk_rgb hki_front_surface_color;
    Hk_rgb hkb_front_surface_color;
    int hka_front_surface_color;

    Hk_rgb hki_back_surface_color;
    Hk_rgb hkb_back_surface_color;
    int hka_back_surface_color;

    /* Front material properties */
    Hk_material_properties hki_front_material_properties;
    Hk_material_properties hkb_front_material_properties;
    int hka_front_material_properties;

    /* Back material properties */
    Hk_material_properties hki_back_material_properties;
    Hk_material_properties hkb_back_material_properties;
    int hka_back_material_properties;

    Hk_rgb hki_front_specular_color;
    Hk_rgb hkb_front_specular_color;
    int hka_front_specular_color;

    Hk_rgb hki_back_specular_color;
    Hk_rgb hkb_back_specular_color;
    int hka_back_specular_color;

    float hki_front_transparency_degree;
    float hkb_front_transparency_degree;
    int hka_front_transparency_degree;

    float hki_back_transparency_degree;
    float hkb_back_transparency_degree;
    int hka_back_transparency_degree;

    Hk_poly_shading_method hki_front_shading_method;
    Hk_poly_shading_method hkb_front_shading_method;
    int hka_front_shading_method;

    Hk_poly_shading_method hki_back_shading_method;
    Hk_poly_shading_method hkb_back_shading_method;
    int hka_back_shading_method;

    Hk_lighting_degree hki_front_lighting_degree;
    Hk_lighting_degree hkb_front_lighting_degree;
    int hka_front_lighting_degree;

    Hk_lighting_degree hki_back_lighting_degree;
    Hk_lighting_degree hkb_back_lighting_degree;
    int hka_back_lighting_degree;

    Hk_interior_style hki_front_interior_style;
    Hk_interior_style hkb_front_interior_style;
    int hka_front_interior_style;

    Hk_interior_style hki_back_interior_style;
    Hk_interior_style hkb_back_interior_style;
    int hka_back_interior_style;

    Hk_general_style hki_front_general_style;
    Hk_general_style hkb_front_general_style;
    int hka_front_general_style;

    Hk_general_style hki_back_general_style;
    Hk_general_style hkb_back_general_style;
    int hka_back_general_style;

    Hk_hatch_style hki_front_hatch_style;
    Hk_hatch_style hkb_front_hatch_style;
    int hka_front_hatch_style;

    Hk_hatch_style hki_back_hatch_style;
    Hk_hatch_style hkb_back_hatch_style;
    int hka_back_hatch_style;

    Hk_iso_curve_info hki_iso_curve_info;
    Hk_iso_curve_info hkb_iso_curve_info;
    int hka_iso_curve_info;

    Hk_stroke_antialiasing hki_hollow_antialiasing;
    Hk_stroke_antialiasing hkb_hollow_antialiasing;
    int hka_hollow_antialiasing;

/* Edge attributes */

    int silhouette_edge;

    Hk_edge_control hki_edge;
    Hk_edge_control hkb_edge;
    int hka_edge;

    float edge_z_offset;		/* In HNPC coords */

    Hk_stroke_antialiasing hki_edge_antialiasing;
    Hk_stroke_antialiasing hkb_edge_antialiasing;
    int hka_edge_antialiasing;

    Hk_rgb hki_edge_color;
    Hk_rgb hkb_edge_color;
    int hka_edge_color;

    Hk_line_style hki_edge_style;
    Hk_line_style hkb_edge_style;
    int hka_edge_style;

    Hk_rgb edge_off_color;

    Hk_pattern *hki_edge_pattern;	/* Pointer to edge pattern */
    Hk_pattern *hkb_edge_pattern;
    int hka_edge_pattern;

    float hki_edge_width;
    float hkb_edge_width;
    int hka_edge_width;

    Hk_line_cap hki_edge_cap;
    Hk_line_cap hkb_edge_cap;
    int hka_edge_cap;

    Hk_line_join hki_edge_join;
    Hk_line_join hkb_edge_join;
    int hka_edge_join;

    float hki_edge_miter_limit;
    float hkb_edge_miter_limit;
    int hka_edge_miter_limit;

/* Text attributes */

    Hk_stroke_antialiasing hki_text_antialiasing;
    Hk_stroke_antialiasing hkb_text_antialiasing;
    int hka_text_antialiasing;

    Hk_rgb hki_text_color;
    Hk_rgb hkb_text_color;
    int hka_text_color;

    float hki_text_expansion_factor;
    float hkb_text_expansion_factor;
    int hka_text_expansion_factor;

    float hki_text_spacing;
    float hkb_text_spacing;
    int hka_text_spacing;

    float hki_text_line_width;
    float hkb_text_line_width;
    int hka_text_line_width;

    Hk_line_cap hki_text_cap;
    Hk_line_cap hkb_text_cap;
    int hka_text_cap;

    Hk_line_join hki_text_join;
    Hk_line_join hkb_text_join;
    int hka_text_join;

    float hki_text_miter_limit;
    float hkb_text_miter_limit;
    int hka_text_miter_limit;

    int hki_text_character_set[4];
    int hkb_text_character_set[4];
    int hka_text_character_set;

    int hki_text_font[4];
    int hkb_text_font[4];
    int hka_text_font;

    Hk_xy rtext_up_vector;

    Hk_text_path rtext_path;

    float rtext_height;

    Hk_text_alignment rtext_alignment;

    float rtext_slant;

    Hk_xy atext_up_vector;

    Hk_text_path atext_path;

    float atext_height;

    Hk_text_alignment atext_alignment;

    float atext_slant;

    Hk_atext_style atext_style;

/* Marker attributes */

    Hk_line_geom_format marker_geom_format;

    Hk_stroke_antialiasing hki_marker_antialiasing;
    Hk_stroke_antialiasing hkb_marker_antialiasing;
    int hka_marker_antialiasing;

    Hk_rgb hki_marker_color;
    Hk_rgb hkb_marker_color;
    int hka_marker_color;

    float hki_marker_size;
    float hkb_marker_size;
    int hka_marker_size;

    Hk_marker_type *hki_marker_type;	/* Pointer to marker type */
    Hk_marker_type *hkb_marker_type;
    int hka_marker_type;

/* Depth attributes */

    Hk_z_range z_range;

    float guard_band;

    int z_buffer_compare;

    Hk_z_buffer_update z_buffer_update;

    int depth_cue_enable;

    Hk_depth_cue_parameters dc_parameters;

    Hk_rgb dc_color;

/* Light source attributes */

    unsigned light_mask;

/* Model clipping attributes */

    int mcp_mask;

    int num_mcp;

    float mp_points[HK_MAX_MODEL_PLANES][3]; /* Points for model plane defs */
    float mp_norms[HK_MAX_MODEL_PLANES][3]; /* Normals for model plane defs */

/*
 * Extra expansion information.  These values are reserved and must be set 
 * to 0 by the user prior to loading the context.  THIS SHOULD ONLY BE
 * USED AFTER EXHAUSTING "state_reserved".
 */
    unsigned state_reserved2[HK_SZ_STATE_RES2];


/* PHIGS_filter related */

    Hk_phigs_filter name_set;

/* Picking attributes */

    int csvn;

    int upick_id;

/* Frame buffer control attributes */

    Hk_raster_op raster_op;

    unsigned plane_mask;

    int current_wid;

    unsigned wid_clip_mask;

    unsigned wid_write_mask;

/* Other attributes */

    unsigned invisibility;

    Hk_rgb highlight_color;

/* NPC Clipping limits */

    Hk_clipping_limits clipping_limits;

/*
 *********************************************************************
 * Add user visible attributes above this line since the non-user
 * visible ones do not have to maintain the same relative position.
 * Non-user visible additions go below this line.  In either case, be
 * sure to adjust the definition of HK_SZ_STATE_RES.  Caution should
 * be exercised when doing this!
 *********************************************************************
 */

/* Cached state information that may not be set or examined by the user */

/* Current structure address */
    
    unsigned struct_address;

/* Current element number */

    int cen_state;

/*
 * Expansion information.  These values are reserved and must be set 
 * to 0 by the user prior to loading the context
 */
    unsigned state_reserved[HK_SZ_STATE_RES];

/*
 * WARNING:  Because the Hk_state structure contains doubles, it must
 * be padded such that the sizeof(Hk_state) is a multiple of 8 bytes (2
 * words).  If ANYTHING changes in the Hk_state structure, alignment
 * must again be verified.
 *
 * HK_STATE_PADDING must be either 1 or 2.  Initializion code will
 * verify that the specified value of HK_STATE_PADDING is correct.
 */

    int padding[HK_STATE_PADDING];

}; /* End of Hk_state */



/*
 *--------------------------------------------------------------
 *
 * Hk_context
 *
 * This the context block.
 *
 * The order is unimportant except that model clipping planes must be
 * last so that they can be conditionally left off when the state is
 * saved.  An attempt has been made to group like attributes.
 *
 * Attributes that may be bundled (i.e. bundle-able attributes) have
 * an additional value that begins with "hki_" (for individual), "hkb_"
 * (for bundled) and "hka_" (for ASF).  These also have a corresponding
 * cached value defined in fe_globals.c.
 *
 *--------------------------------------------------------------
 */

typedef struct Hk_context Hk_context;

struct Hk_context {
/*
 * WARNING:  The first three members of the Hk_context structure are
 * inviolate.  Do *not* change them for any reason.  The magic number
 * and version are included as a consistency check and *must* ocupy two
 * 32-bit words to provide proper alignment for the Hk_state
 * structure.
 */
    unsigned magic;			/* Magic number for Hk_context */
    unsigned version;			/* Version number for Hk_context */
    Hk_state s;				/* All of the state values */

/* Execution values */

    unsigned dpc;			/* Display program counter */
    int risc_regs[HK_NUM_RISC_REGS];	/* RISC registers */
    int cpu_time;
    int elapsed_time;

/* Non-state attribute values */

    Hk_transparency_quality transparency_quality;

    Hk_normal surface_normal;		/* Surface normal for polyedge */

    Hk_font_table *text_font_table;	/* Pointer to text font table */

    int marker_snap_to_grid;

    Hk_window_boundary *ptr_window_boundary;

    Hk_aspect_ratio aspect_ratio;

    Hk_light lights[HK_MAX_LIGHTS];	/* Light source array */

    Hk_filter_pair invisibility_filter;

    Hk_filter_pair highlighting_filter;

    Hk_filter_pair pick_filter;

    Hk_traversal_mode traversal_mode;

    Hk_pick_hits *pick_results_buffer;	/* Pointer to pick results buffer */

    int hsvn;

    int cen;

    Hk_pick_box pick_box;

    int echo_upick_id;

    int echo_svn;

    int echo_en;

    Hk_echo_type echo_type;

    int non_echo_invisible;

    int echo_invisible;

    int force_echo_color;

    Hk_rgb echo_color;

    Hk_plane_group plane_group;

    Hk_buffer draw_buffer;

    int fast_clear_set;

    Hk_stereo_mode stereo_mode;

    Hk_rgb window_bg_color;

    Hk_transparency_mode transparency_mode;

    Hk_stroke_antialiasing stroke_antialiasing;

    int stochastic_enable;

    Hk_stochastic_setup stochastic_setup;

    Hk_rgb stencil_fg_color;

    Hk_rgb stencil_bg_color;

    int stencil_transparent;

    unsigned stack_limit;

    Hk_scratch_buffer *scratch_buffer;	/* Pointer to scratch buffer */

/* Pick echo attributes to support QUM */

    unsigned echo_struct_address;
    int  echo_end_en;

/* Undraw attribute */

    int undraw;

/*
 *********************************************************************
 * Add user visible attributes above this line since the non-user
 * visible ones do not have to maintain the same relative position.
 * Non-user visible additions go below this line.  In either case, be
 * sure to adjust the definition of HK_SZ_CTX_RES.  Caution should be
 * exercised when doing this!
 *********************************************************************
 */

/* Cached context information that may not be set or examined by the user */

    unsigned jps_address;	/* After a jps call, used for QUM */

    /* Booleans */
    unsigned pick_echo_active;
    unsigned echo_svn_plus_descendents;
    unsigned echo_contig_upick_id_active;

    /* Previous pick information */
    unsigned previous_picked_svn;
    unsigned previous_picked_en;
    unsigned previous_start_en;
    unsigned previous_picked_upick_id;

    /* EPS information */
    unsigned eps_leaf_no_sis;
    unsigned leaf_return_addr;
    unsigned leaf_csvn;
    unsigned leaf_struct_address;

    /* QUM support through pick_echo */
    unsigned echo_en_range_active;	/* Boolean */

    /* EPS information */
    int leaf_cen_state;
/*
 * Expansion information.  These values are reserved and must be set 
 * to 0 by the user prior to loading the context
 */
    unsigned ctx_reserved[HK_SZ_CTX_RES];

/*
 * WARNING:  Because the Hk_context structure contains structures which
 * contains doubles, Hk_context must be padded such that the
 * sizeof(Hk_context) is a multiple of 8 bytes (2 words).  If ANYTHING
 * changes in the Hk_context structure, alignment must again be
 * verified.
 *
 * HK_CTX_PADDING must be either 1 or 2.  Initializion code will verify
 * that the specified value of HK_CTX_PADDING is correct.
 */

    int padding[HK_CTX_PADDING];

}; /* End of Hk_context */
#endif !LOCORE



/*
 * Stuff to keep certain modules happy
 */

#ifndef NULL
#define NULL	0
#endif

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#endif _HK_PUBLIC_

/* End of hk_public.h */
