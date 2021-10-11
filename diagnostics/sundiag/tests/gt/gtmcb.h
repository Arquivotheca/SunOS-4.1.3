/*	@(#)gtmcb.h	1.6   91/06/27 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifndef _gtmcb_h
#define _gtmcb_h

/*
 * This header file contains public constants and types used
 * by the gt device driver and user programs to communicate
 * with the gt virtual machine.
 *
 * WARNING 1: Never declare data to be char or short.  The gt graphics
 * processor cannot reference datatypes of less than 32 bits.
 *
 * WARNING 2: Anytime the MCB is updated (either the user or kernel MCB),
 * the appropriate MCB version number (HK_UMCB_VERSION or
 * HK_KMCB_VERSION) must be modified.
 */

/*
 * Version numbers for user and kernel MCBs
 */
#define HK_UMCB_VERSION	0x0203		/* Current version of Hkvc_umcb */
#define HK_KMCB_VERSION	0x0203		/* Current version of Hkvc_kmcb */


/*
 * Magic numbers for Hkvc_kmcb and Hkvc_umcb data structures.  These
 * should NOT be modified.
 */
#define HK_UMCB_MAGIC	0xA7880103	/* Message control block: Hkvc_umcb */
#define HK_KMCB_MAGIC	0xA7880104	/* Message control block: Hkvc_kmcb */

/*
 * Size of print buffer (in bytes)
 */
#define	HKKVS_PRINT_STRING_SIZE	4096

/*
 * Size of notification buffer (in words)
 */
#define	HKKVS_NOTIFY_BUFSIZE	32

/*
 * WARNING: The following typedefs must be kept in sync with the
 * corresponding HIS typedefs: Gt_buffer, Gt_plane_group,
 * Gt_wlut_update, and Gt_clut_update.
 */

/*
 * Plane group
 */
typedef int Gt_plane_group;

#define GT_8BIT_RED			0
#define GT_8BIT_GREEN			1
#define GT_8BIT_BLUE			2
#define GT_8BIT_OVERLAY			3
#define GT_24BIT			4


/*
 * Draw buffer
 */
typedef int Gt_buffer;

#define GT_BUFFER_A			0
#define GT_BUFFER_B			1


/*
 * Window lookup table update structures
 */
typedef struct Gt_wlut_update	Gt_wlut_update;

struct Gt_wlut_update {
	int		entry;
	unsigned	mask;
	Gt_buffer	image_buffer;
	Gt_buffer	overlay_buffer;
	Gt_plane_group	plane_group;
	int		clut;
	int		fast_clear_set;
};


typedef struct Gt_kernel_wlut_update	Gt_kernel_wlut_update;

struct Gt_kernel_wlut_update {
	int		table;		/* Image or overlay */
	int		num_entries;	/* number of entries to update */
	Gt_wlut_update	*upd;		/* Array of entries */
};


/*
 * Color lookup table update structure
 *
 * The color format is:
 *	bits 31-24:  undefined
 *	bits 23-16:  blue
 *	bits 15-8:   green
 *	bits  7-0:   red
 */
typedef struct Gt_clut_update	Gt_clut_update;

struct Gt_clut_update {
	int	table;
	int	start_entry;
	int	num_entries;
	unsigned *colors;
};



/*
 * Structure definitions for virtual communication register
 *
 *	Message control block has data, command and status words have
 *	control bits.  Hardware interrupt bits are defined elsewhere.
 *	There should be one MCB for kernel communication and one for
 *	user (including the signal handler) communication.
 */

/*
 * The Message Control Blocks
 */

/* Number of state bits for push/pop state and restore partial context */
#define HKST_NUM_WORDS		8

typedef struct Hkvc_umcb	Hkvc_umcb;

struct Hkvc_umcb {
	/*
	 * WARNING:  The first two members of the Hkvc_umcb structure are
	 * inviolate.  Do *not* change them for any reason.  The magic number
	 * and version are included as a consistency check.
	 */
	unsigned magic;			/* Magic number for Hkvc_umcb */
	unsigned version;		/* Version number for Hkvc_umcb */

	/*
	 * Host-to-gt section  (only the host can write these)
	 */
	unsigned command;			/* Command bits */
	unsigned *context;			/* Pointer to context */
	unsigned state_list[HKST_NUM_WORDS];	/* Flush specifiers */
	unsigned breakpoint_address;		/* Used w/ breakpoint enable */
	unsigned reserved1[4];			/* reserved for future use */

	/*
	 * gt-to-host section  (only the gt can write these)
	 */
	unsigned gt_stopped;		/* zero: running  nonzero: halted */
	unsigned status;		/* Interrupt source bits */
	unsigned dpc;			/* DPC at time of TRAP or Error */
	unsigned errorcode;		/* Error code */
	unsigned trap_instruction;	/* Trap instruction */
	unsigned gt_flags;		/* Flags for virtual machine info */
	unsigned instruction_count;	/* HIS count */
	unsigned reserved2[3];		/* reserved for future use */
};


/*
 * Structure used by the FB_CONNECT ioctl
 */
struct gt_connect {
	Hkvc_umcb *gt_pmsgblk;		/* Address of user MCB   */
	unsigned  *gt_puvcr;		/* Address of user VCR   */
	unsigned  *gt_psvcr;		/* Address of signal VCR */
};


typedef struct Hkvc_kmcb   Hkvc_kmcb;

struct Hkvc_kmcb {

	/*
	 * WARNING:  The first two members of the Hkvc_kmcb structure are
	 * inviolate.  Do *not* change them for any reason.  The magic number
	 * and version are included as a consistency check.
	 */
	unsigned magic;			/* Magic number for Hkvc_kmcb   */
	unsigned version;		/* Version number for Hkvc_kmcb */

	/*
	 * Host-to-gt section  (only the host can write these)
	 */
	unsigned   	 command;		/* Command bits */
	Hkvc_umcb 	 *user_mcb;		/* Pointer to user MCB */
	unsigned   	 wlut_image_bits;	/* Number of bits for image */
	Gt_kernel_wlut_update	*wlut_info;	/* WLUT update information */
	Gt_clut_update		*clut_info;	/* CLUT update information */
	unsigned	 light_pen_enable;	/* Enable/disable light pen */
	unsigned	 light_pen_distance;	/* X+Y delta tolerance */
	unsigned	 light_pen_time;	/* Nbr of frames tolerance */
	int		 screen_width;		/* Width of visible FB */
	int		 screen_height;		/* Height of visible FB */
	int		 fb_width;		/* FB width value (enum) */
	unsigned	 *print_buffer;		/* Ptr to (4K) print buffer */
	unsigned	 *notify_buffer;	/* Ptr to notificatoin buffer */
	unsigned	 reserved1[3];		/* reserved for future use */

	/*
	 * gt-to-host section  (only the gt can write these)
	 */
	unsigned gt_stopped;		/* zero=running, non-zero=halted */
	unsigned status;		/* Interrupt source bits */
	unsigned dpc;			/* DPC at time of TRAP or Error */
	unsigned errorcode;		/* Error code */
	unsigned trap_instruction;	/* Trap instruction */
	unsigned vrt_count;		/* Vertical retrace counter */
	unsigned light_pen_hit_x;	/* X position of light pen hit */
	unsigned light_pen_hit_y;	/* Y position of light pen hit */
	unsigned reserved2[4];		/* reserved for future use */
};


/*
 * Command bits for communicating from the kernel to the gt
 */
#define HKKVC_SET_IMAGE_WLUT_BITS	0x00000001	/* Kernel only */
#define HKKVC_UPDATE_WLUT		0x00000002
#define HKKVC_UPDATE_CLUT		0x00000004
#define HKKVC_ENABLE_VRT_INTERRUPT	0x00000008	/* Kernel only */
#define HKKVC_DISABLE_VRT_INTERRUPT	0x00000010	/* Kernel only */
#define HKKVC_SET_LIGHT_PEN_PARAMETERS	0x00000080	/* Kernel only */
#define HKKVC_LOAD_USER_MCB		0x00000100	/* Kernel only */
#define HKKVC_LOAD_CONTEXT		0x00000200
#define HKKVC_FLUSH_RENDERING_PIPE	0x00000800
#define HKKVC_FLUSH_CONTEXT_SUBSET	0x00001000
#define HKKVC_FLUSH_FULL_CONTEXT	0x00002000
#define HKKVC_PAUSE_WITHOUT_INTERRUPT	0x00004000
#define HKKVC_PAUSE_WITH_INTERRUPT	0x00008000
#define HKKVC_SET_FB_SIZE		0x00010000	/* Kernel only */

/*
 * Special for simulator use only
 */
#define HKKVC_EXIT_SIMULATOR		0x80000000	/* Simulator only */

/*
 * Command bits for communicating from the user to the gt
 */
#define HKUVC_ENABLE_BPT_INTERRUPT	0x00000020	/* User only */
#define HKUVC_DISABLE_BPT_INTERRUPT	0x00000040	/* User only */
#define HKUVC_LOAD_CONTEXT		0x00000200
#define HKUVC_EXECUTE_ONE_INSTRUCTION	0x00000400	/* User only */
#define HKUVC_FLUSH_RENDERING_PIPE	0x00000800
#define HKUVC_FLUSH_CONTEXT_SUBSET	0x00001000
#define HKUVC_FLUSH_FULL_CONTEXT	0x00002000
#define HKUVC_PAUSE_WITHOUT_INTERRUPT	0x00004000
#define HKUVC_PAUSE_WITH_INTERRUPT	0x00008000
#define HKUVC_ENABLE_AUTO_FLUSH		0x80000000
#define HKUVC_DISABLE_AUTO_FLUSH	0x40000000

/*
 * Stopped bits for "gt_stopped" field
 */
#define HK_COMM_INTERNAL_INTS		0x0008
#define HK_KERNEL_STOPPED		0x0004
#define HK_SIGNAL_STOPPED		0x0002
#define HK_USER_STOPPED			0x0001


/*
 * Status bits for communicating from gt to the kernel
 *
 * GT will continue following these
 */
#define HKKVS_VERTICAL_RETRACE		0x00000001	/* Kernel only */
#define HKKVS_LIGHT_PEN_HIT		0x00000002	/* Kernel only */
#define HKKVS_LIGHT_PEN_BUTTON_DOWN	0x00000004	/* Kernel only */
#define HKKVS_LIGHT_PEN_BUTTON_UP	0x00000008	/* Kernel only */
#define HKKVS_LIGHT_PEN_NO_HIT		0x00000010	/* Proximity lost */
#define HKKVS_PRINT_STRING		0x00000400	/* Kernel only */
#define HKKVS_NOTIFY			0x00001000	/* Kernel only */

/*
 * GT will pause following these
 */
#define HKKVS_HFLAG3_DONE		0x00002000	/* Kernel only */
#define HKKVS_DONE_WITH_REQUEST		0x00000100
#define HKKVS_ERROR_CONDITION		0x00000200
#define HKKVS_INSTRUCTION_TRAP		0x00000800


/*
 * Status bits for communicating from the gt to the user
 *
 * All must be acknowledged
 */
#define HKUVS_BREAKPOINT_TRAP     	0x00000020
#define HKUVS_DONE_WITH_REQUEST		0x00000100
#define HKUVS_ERROR_CONDITION		0x00000200
#define HKUVS_INSTRUCTION_TRAP		0x00000800


/*
 * Defines for the "gt_flags" word of the user MCB
 */
#define HKUVF_AUTO_FLUSH		0x00000001


/*
 * Error codes
 *
 * As new error conditions are detected, additional error codes
 * must be added here.  These are returned in the MCB above
 * for HKKVS_ERROR_CONDITION or HKUVS_ERROR_CONDITION.
 */
#define HKERR_NO_ERROR			0
#define HKERR_ILLEGAL_OP_CODE		1
#define HKERR_ILLEGAL_ATTRIBUTE		2
#define HKERR_LIGHT_OVERFLOW		3
#define HKERR_MCP_OVERFLOW		4
#define HKERR_JMPL_IMMEDIATE		5
#define HKERR_JMPR_ABSOLUTE		6
#define HKERR_JPS_IMMEDIATE		7
#define HKERR_ST_IMMEDIATE		8
#define HKERR_STU_IMMEDIATE		9
#define HKERR_PARA_ORDER_OVER_6		10	/* Parametric order over 6 */
#define HKERR_KNOT_INDEX_OUTARANGE	11	/* Parametric order over 6 */
#define HKERR_NURB_ORDER_OVER_10	12	/* Nurb order over 10 */
#define HKERR_NURB_DATA_ERROR		13	/* Nurb input data error */
#define HKERR_NO_MARKER_TABLE		14	/* Marker table not defined */
#define HKERR_NO_FONT_TABLE		15	/* Font table not defined */
#define HKERR_NO_NORMAL_IN_TRIANGLE	16	/* Tri-mode needs a normal */
#define HKERR_TWO_NORMALS_OR_COLORS	17	/* Tri-mode only gets one... */
#define HKERR_SS_ACC_IMMEDIATE		18	/* Stochastic_* immed addr */
#define HKERR_LOAD_CONTEXT_IMMEDIATE	19
#define HKERR_LDSTU_IMMEDIATE		20
#define HKERR_SWAP_IMMEDIATE		21
#define HKERR_MIA_BUS_TIME_OUT 		22
#define HKERR_MIA_BUS_ERR		23
#define HKERR_MIA_SIZE			24
#define HKERR_LOCAL_BUS_TIME_OUT 	25
#define HKERR_DIAG_ESC_SMALL		26	/* diag esc 860 pgm too small */
#define HKERR_DIAG_ESC_MAGIC		27	/* diag esc invalid magic num */
#define HKERR_DIAG_ESC_LARGE		28	/* diag esc 860 pgm too large */
#define HKERR_DIAG_ESC_CHECKSUM		29	/* diag esc invalid checksum  */

						/* Illegal addressing modes:  */
#define HKERR_SI32_IMMEDIATE		30	/* 	for store-image_32    */
#define HKERR_SI8_IMMEDIATE		31	/* 	for store-image_8     */
#define HKERR_STD_IMMEDIATE		32	/* 	for std		      */

#define HKERR_STACK_UNDERFLOW		33	/* hk_stack_limit underflow   */

/* New (post Beta-freeze) errors */
#define HKERR_NULL_POINTER		34	/* Illegal NULL pointer	      */
#define HKERR_BAD_POINTER		35	/* Illegal pointer	      */
#define HKERR_CLUT_INDEX		36	/* Illegal CLUT index	      */
#define HKERR_SCRATCH_BUFFER		37	/* Illegal/NULL scratch buf   */
#define HKERR_SINGULAR_CMT		38	/* Composite MT is singular   */
#define HKERR_BAD_GEOM_FORMAT		39	/* Illegal *_geom_format      */
#define HKERR_BAD_NORMAL		40	/* Illegal normal for MCP     */
#define HKERR_WLUT_INDEX		41	/* Illegal WLUT index	      */
#define HKERR_BAD_COUNT			42	/* Illegal count field        */

/*
 * Hardware errors (to kernel)
 */
#define HKERR_SU_SYNC			100	/* Setup sync. error */
#define HKERR_EW_SYNC			101	/* Edge walker sync. error */
#define HKERR_SI_SYNC			102	/* Span inter. sync. error */
#define HKERR_RP_SEMAPHORE		103	/* RP semaphore not seen */

/*
 * Version errors (user or kernel)
 */
#define HKERR_BAD_MCB_MAGIC		200	/* Bad MCB magic number */
#define HKERR_BAD_MCB_VERSION		201	/* Bad MCB version number */
#define HKERR_BAD_CTX_MAGIC		202	/* Bad CTX magic number */
#define HKERR_BAD_CTX_VERSION		203	/* Bad CTX version number */

/*
 * Front end code assertion failure (to kernel)
 */
#define HKERR_ASSERTION_FAILED		300	/* fe_assert() failed */

/*
 * Other errors
 */
#define HKERR_OTHER			(-1)

#endif _gtmcb_h
