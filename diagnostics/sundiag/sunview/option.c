#ifndef	lint
static	char sccsid[] = "@(#)option.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */


#include <curses.h>
#include "sundiag.h"
#include "../../lib/include/probe_sundiag.h"
#include "struct.h"
#include <sundev/tmreg.h>
#include <sundev/xtreg.h>
#include <sundev/arreg.h>
#include "tty.h"
#include "disk_strings.h"
#include "sundiag_msg.h"
#include <netdb.h>

#ifndef	sun386
#include <sys/mtio.h>
struct	mt_tape_info	tapes[]=MT_TAPE_INFO;
#else
#include "mtio.h"
#include "tape_strings.h"
#endif

/************ following external variables are declared in tests.c ************/
extern	char	*sp_src;	/* default ALM loopback source */
extern	char	*sp_des;	/* default ALM loopback destination */
extern	char	*sp2_src;	/* default ALM2 loopback source */
extern	char	*sp2_des;	/* default ALM2 loopback destination */
extern	char	*scsisp_src; /* default SCSI serial port loopback source */
extern	char	*scsisp_des; /* default SCSI serial port loopback destination */
extern	char	*sunlink_src[];	/* default SunLink loopback source */
extern	char	*sunlink_des[];	/* default SunLink loopback destination */
/******************************************************************************/

extern	char	*malloc();
extern	char	*strcpy();
extern	char	*sprintf();
extern	char	*mktemp();

static  Panel_item item1,item2,item3,item4,item5,item6,item7,item8,item9,item10;
static  Panel_item item11, item12, item13, item14, item15, item16, item17;
static  Panel_item item18, item19, item20, item21, item22, item23;
static  Panel_item processor_item[30];
static  int	qic150;		/* if this is a qic150 drive */
static	int	exabyte_8200;	/* if this is a Exabyte  2 G.B drive */
static	int	exabyte_8500;	/* if this is a Exabyte  5 G.B drive */
static  int	hp_flt;		/* if this is a hp reel drive(front load) */
static	int	halfin;		/* if this is a 1/2"(magnetics) drive */
static	int	cart_tape;	/* if this is a cartridge tape drive */
static  int	ctlr_type;	/* to keep the SCSI controller types(SCSI2/3) */
static	hsi_mixport_proc();	/* forward declaration */
static	sbus_hsi_mixport_proc();	/* forward declaration */
static  hawk_subtest_loop_proc();       /* forward declaration */
static  disk_rawtest_proc();	/* forward declaration */
static  zebra_access_proc();    /* forward declaration */ 
static  lpvi_image_proc();      /* forward declaration */
static  spif_tests_proc();      /* forward declaration */
char audbri_ref_file[200];

struct	option_choice
{
  int	num;		/* number of choices */
  char	*label;		/* label of the choice */
  int	type;		/* item type: PANEL_CYCLE, PANEL_TOGGLE, PANEL_TEXT */
  char	com;		/* the single letter command for TTY mode(not f,d,h) */
  char	*choice[18];	/* up to 16 choices, must be terminated with 2 NULL's */
};

/* global variables */
int processors_mask;
int number_processors;

#define	CYCLE_ITEM	0
#define	TOGGLE_ITEM	1
#define	TEXT_ITEM	2

static	char	enable[]="Enable";
static	char	disable[]="Disable";

static  struct  option_choice	wait_choice=	/* Wait Time(VMEM) */
{
  7, "Wait Time:", CYCLE_ITEM, 'w',
  {"0", "5", "10", "15", "30", "60", "90", "random", NULL, NULL}
};
  
static	struct  option_choice	reserve_choice=	/* Reserve(VMEM) */
{
  1, "Reserve:  ", TEXT_ITEM, 'r',
  {NULL, NULL, NULL}
};
  
static	struct  option_choice	cpusp_choice=	/* Loopback(CPU_SP) */
{
  4, "Loopback:", CYCLE_ITEM, 'l',
  {"a", "b", "a b", "a-b", NULL, NULL}
};

static	struct  option_choice	cpusp1_choice=	/* Loopback(CPU_SP1) */
{
  4, "Loopback:", CYCLE_ITEM, 'l',
  {"c", "d", "c d", "c-d", NULL, NULL}
};

static	struct  option_choice	raw_choice=   /* Raw device test(DISKS) */
{
  2, "Rawtest ", TOGGLE_ITEM, 'r',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	fs_choice=    /* File system device test(DISKS) */
{
  2, "Filetest", TOGGLE_ITEM, 'w',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	raw_op_choice1=  /*Raw device test options(DISKS)*/
{
  2, "Rawtest Mode:     ", CYCLE_ITEM, 'o',
  {"Readonly", "Write/Read", NULL, NULL}
};

static	struct  option_choice	raw_op_choice2=  /*Raw device partition(DISKS)*/
{
  8, "Rawtest Partition:", CYCLE_ITEM, 'p',
  {"a", "b", "c", "d", "e", "f", "g", "h", NULL, NULL}
};

static	struct  option_choice	raw_op_choice3=  /*Raw device test size(DISKS)*/
{	/* All in Mega Bytes */
  7, "Rawtest Size (MB):", CYCLE_ITEM, 'm',
  {"100", "200", "400", "600", "800", "1000", "1200", NULL, NULL}
};

static	struct  option_choice	from_choice=	/* From(loopback assignment) */
{
  1, "From:", TEXT_ITEM, 'r',
  {NULL, NULL, NULL}
};

static	struct  option_choice	to_choice=	/* To(loopback assignment) */
{
  1, "  To:", TEXT_ITEM, 't',
  {NULL, NULL, NULL}
};

static	struct  option_choice	audio_choice=		/* audio ouput(AUDIO) */
{
  2, "Audio Output:", CYCLE_ITEM, 'a',
  {"Speaker", "Jack", NULL, NULL}
};

static	struct  option_choice	loud_choice=	/* audio volume(AUDIO) */
{
  1, "Volume:      ", TEXT_ITEM, 'v',
  {NULL, NULL, NULL}
};

static	struct  option_choice	cdtype_choice=		/* CD type(CDROM) */
{
  5, "CD Type:     ", CYCLE_ITEM, 'c',
  {"Cdassist", "Sony2", "Hitachi4", "Pdo", "Other" , NULL, NULL}
};

static	struct  option_choice	datatest1_choice=	/* data test(CDROM)*/
{
  2, "Data Mode 1: ", CYCLE_ITEM, 'x',
  {enable, disable, NULL, NULL}
};

static	struct  option_choice	datatest2_choice=	/* data test(CDROM)*/
{
  2, "Data Mode 2: ", CYCLE_ITEM, 'y',
  {enable, disable, NULL, NULL}
};

static	struct  option_choice	audiotest_choice=	/* audio test(CDROM) */
{
  2, "Audio Test:  ", CYCLE_ITEM, 'a',
  {enable, disable, NULL, NULL}
};

static	struct  option_choice	volume_choice=		/* volume(CDROM) */
{
  1, "Volume:      ", TEXT_ITEM, 'v',
  {NULL, NULL, NULL}
};

static	struct  option_choice	datatrack_choice=	/* datatrack(CDROM) */
{
  1, "% Data/Track:", TEXT_ITEM, 'p',
  {NULL, NULL, NULL}
};

static	struct  option_choice	readmode_choice=	/* read mode(CDROM) */
{
  2, "Read Mode:   ", CYCLE_ITEM, 'm',
  {"Random", "Sequential", NULL, NULL}
};

static  struct  option_choice   audbri_loop_type_choice=        /* AUDBRI */
{
  2, "Type:", CYCLE_ITEM, 't',
  {"Line-in/Line-out", "Line-in/Headphone", NULL, NULL}
};

static  struct  option_choice   audbri_loop_choice=     /* AUDBRI */
{
  2, "Loopback:", CYCLE_ITEM, 'e',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice   audbri_loop_calib_choice=       /* AUDBRI */
{
  2, "Calibration:", CYCLE_ITEM, 'c',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice   audbri_audio_choice=  /* AUDBRI */
{
  2, "Audio Test:", CYCLE_ITEM, 'm',
  {disable, enable, NULL, NULL}
};
static  struct  option_choice   audbri_crystal_choice=  /* AUDBRI */
{
  2, "Crystal Test:", CYCLE_ITEM, 'x',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice   audbri_controls_choice= /* AUDBRI */
{
  2, "Controls Test:", CYCLE_ITEM, 'o',
  {disable, enable, NULL, NULL}
};
 
static  struct  option_choice   audbri_ref_file_choice= /* AUDBRI */
{
  1, "Reference File:", TEXT_ITEM, 'f',
  {NULL, NULL, NULL}
};
 
static	struct  option_choice	format_choice=	/* format(SCSITAPE) */
{
  3, "Format:     ", CYCLE_ITEM, 'o',
  {"QIC-11", "QIC-24", "QIC-11&QIC-24", NULL, NULL}
};

static	struct  option_choice	density_choice=	/* density(SCSITAPE) */
{
  5, "Density:    ", CYCLE_ITEM, 'e',
  {"800-BPI", "1600-BPI", "6250-BPI", "All", "Compression", NULL, NULL}
};

static	struct  option_choice	density_choice_nodc=	/* density(SCSITAPE) */
{
  4, "Density:    ", CYCLE_ITEM, 'e',
  {"800-BPI", "1600-BPI", "6250-BPI", "All", NULL, NULL}
};

static	struct  option_choice	density_choice_halfin=	/* density(SCSITAPE) */
{
  3, "Density:    ", CYCLE_ITEM, 'e',
  {"Both", "1600-BPI", "6250-BPI", NULL, NULL}
};

static	struct  option_choice	density_choice_exb8500=	/* density(SCSITAPE) */
{
  4, "Density:    ", CYCLE_ITEM, 'e',
  {"EXB-8200", "EXB-8500", "All", "Compression", NULL, NULL}
};

static	struct  option_choice	density_choice_exb8500_nodc=	/* density(SCSITAPE) */
{
  3, "Density:    ", CYCLE_ITEM, 'e',
  {"EXB-8200", "EXB-8500", "Both", NULL, NULL}
};

static	struct  option_choice	mode_choice=	/* mode(SCSITAPE) */
{
  2, "Mode:       ", CYCLE_ITEM, 'm',
  {"Write/Read", "Readonly", NULL, NULL}
};

static	struct  option_choice	length_choice=	/* length(SCSITAPE) */
{
  4, "Length:     ", CYCLE_ITEM, 'l',
  {"EOT", "Specified", "Long", "Short", NULL, NULL}
};

static	struct  option_choice	block_choice=	/* block(SCSITAPE) */
{
  1, "# of Blocks:", TEXT_ITEM, 'b',
  {NULL, NULL, NULL}
};

static	struct  option_choice	file_choice=	/* file test(SCSITAPE) */
{
  2, "File Test:  ", CYCLE_ITEM, 'i',
  {enable, disable, NULL, NULL}
};

static	struct  option_choice	stream_choice=	/* streaming(SCSITAPE) */
{
  2, "Streaming:  ", CYCLE_ITEM, 's',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	recon_choice=	/* recon(SCSITAPE) */
{
  2, "Reconnect:  ", CYCLE_ITEM, 'r',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice   retension_choice=  /* retension(SCSITAPE) */
{
  2, "Retension:  ", CYCLE_ITEM, 't',
  {enable, disable, NULL, NULL}
};

static  struct  option_choice   clean_choice=	/* clean_head(SCSITAPE) */
{
  2, "Clean Head: ", CYCLE_ITEM, 'c',
  {enable, disable, NULL, NULL}
};

static	struct  option_choice	pass_choice=	/* pass(SCSITAPE) */
{
  1, "# of Passes:", TEXT_ITEM, 'p',
  {NULL, NULL, NULL}
};

static	struct  option_choice	floppy_choice=	/* floppy(IPC) */
{
  2, "Floppy Drive Test:", CYCLE_ITEM, 'y',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	printer_choice=	/* printer(IPC) */
{
  2, "Printer Port Test:", CYCLE_ITEM, 'p',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	sp_choice=	/* serial port(MCP) */
{
  2, "Serial Port Test ", TOGGLE_ITEM, 's',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	pp_choice=	/* printer port(MCP) */
{
  2, "Printer Port Test", TOGGLE_ITEM, 'p',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	clock_choice=	/* clock source(HSI) */
{
  2, "Clock Source:      ", CYCLE_ITEM, 'c',
  {"On-board", "External", NULL, NULL}
};

static	struct  option_choice	type_choice=	/* port type(HSI) */
{
  4, "Port Type:         ", CYCLE_ITEM, 't',
  {"RS449", "V.35", "RS449&V.35", "RS449-V.35", NULL, NULL}
};

static	struct  option_choice	loopback_choice=	/* loopback(HSI) */
{
  4, "Loopback:          ", CYCLE_ITEM, 'l',
  {"0", "1", "0 1", "0-1", NULL, NULL}
};

static	struct  option_choice	loopback_choice1=	/* loopback1(HSI) */
{
  4, "Loopback:          ", CYCLE_ITEM, 'l',
  {"2", "3", "2 3", "2-3", NULL, NULL}
};

/*
 *          -----------------------------------------------
 * ...ITEM# |    ITEM3    |             |   ITEM2  |ITEM1 |
 *          |-------------|-------------|----------|------|
 *   choice |   loopback  |             | internal |clock |
 *          |             |             | loopback |      |
 *          |-------------|-------------|----------|------|
 *  default |      4      |             |     0    |   0  |
 *          |-------------|-------------|----------|------|
 *   bit #  | 7  6  5  4  |  3       2  |     1    |   0  |
 *          -----------------------------------------------
 * 
 *   This version currently supports up to three boards.  Each board
 *   having 4 ports => 12 ports supported.
 */

static  struct  option_choice   sbus_clock_choice=      /* clock source(HSI) */
{
  2, "Clock Source:      ", CYCLE_ITEM, 'c',
  {"Baud (on-board)", "External", NULL, NULL}
};

static  struct  option_choice   internal_loopback_choice=
{
  2, "Internal Loopback: ", CYCLE_ITEM, 'i',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	sbus_loopback_choice= /* loopback(SBus-HSI0) */
{
  8, "Loopback:          ", CYCLE_ITEM, 'l',
  {"0", "1", "2", "3", "0,1,2,3", "0-1,2-3", "0-2,1-3", "0-3,1-2", NULL, NULL}
};

static	struct  option_choice	sbus_loopback_choice1=	/* loopback(SBus-HSI1) */
{
  8, "Loopback:          ", CYCLE_ITEM, 'l',
  {"4", "5", "6", "7", "4,5,6,7", "4-5,6-7", "4-6,5-7", "4-7,5-6", NULL, NULL}
};

static	struct  option_choice	sbus_loopback_choice2=	/* loopback(SBus-HSI2) */
{
  8, "Loopback:          ", CYCLE_ITEM, 'l',
  {"8", "9", "10", "11", "8,9,10,11", "8-9,10-11", "8-10,9-11", "8-11,9-10", NULL, NULL}
};

static	struct  option_choice	gb_choice=	/* graphics buffer(GP) */
{
  2, "Graphics Buffer:", CYCLE_ITEM, 'b',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice   presto_ratio_choice=    /* Presto (PRESTO) */
{
  2, "Performance Warning:", CYCLE_ITEM, 'e',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	ntsc_choice=	/* NTSC loopback(TV1) */
{
  2, "NTSC loopback:", CYCLE_ITEM, 'n',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	yc_choice=	/* YC loopback(TV1) */
{
  2, "  YC loopback:", CYCLE_ITEM, 'y',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	yuv_choice=	/* YUV loopback(TV1) */
{
  2, " YUV loopback:", CYCLE_ITEM, 'u',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	rgb_choice=	/* RGB loopback(TV1) */
{
  2, " RGB loopback:", CYCLE_ITEM, 'r',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	spray_choice=	/* spray test(ENET) */
{
  2, "Spray Test:     ", CYCLE_ITEM, 's',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	delay_choice=	/* spray delay(ENET) */
{
  1, "Spray Delay:    ", TEXT_ITEM, 't',
  {NULL, NULL, NULL}
};

static	struct  option_choice	driver_choice=	/* driver warning(ENET)*/
{
  2, "Driver Warning: ", CYCLE_ITEM, 'w',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice function_loop_choice= /*loops per function(CG12)*/
{
  1, "loops per function:  ", TEXT_ITEM, 's',
  {NULL, NULL, NULL}
};

static  struct  option_choice board_loop_choice=  /*loops per board(CG12)*/
{
  1, "loops per board:     ", TEXT_ITEM, 'b',
  {NULL, NULL, NULL}
};

static  struct  option_choice egret_sub1_choice=  /* functional test (CG12) */
{
  2, "DSP:                 ", CYCLE_ITEM, 'p',
  {enable, disable, NULL, NULL}
};

static  struct  option_choice egret_sub2_choice=  /* functional test (CG12) */
{
  2, "SRAM & DRAM:         ", CYCLE_ITEM, 'r',
  {enable, disable, NULL, NULL}
};

static  struct  option_choice egret_sub3_choice=  /* functional test (CG12) */
{
  2, "Video Memories:      ", CYCLE_ITEM, 'm',
  {enable, disable, NULL, NULL}
};

static  struct  option_choice egret_sub4_choice=  /* functional test (CG12) */
{
  2, "Lookup Tables:       ", CYCLE_ITEM, 'l',
  {enable, disable, NULL, NULL}
};

static  struct  option_choice egret_sub5_choice=  /* functional test (CG12) */
{
  2, "Vectors Generation:  ", CYCLE_ITEM, 'v',
  {enable, disable, NULL, NULL}
};

static  struct  option_choice egret_sub6_choice=  /* functional test (CG12) */
{
  2, "Polygons Generation: ", CYCLE_ITEM, 'y',
  {enable, disable, NULL, NULL}
};


static  struct  option_choice egret_sub7_choice=  /* functional test (CG12) */
{
  2, "Transformations:     ", CYCLE_ITEM, 't',
  {enable, disable, NULL, NULL}
};

static  struct  option_choice egret_sub8_choice=  /* functional test (CG12) */
{
  2, "Clipping & Hidden:   ", CYCLE_ITEM, 'c',
  {enable, disable, NULL, NULL}
};

static  struct  option_choice egret_sub9_choice=  /* functional test (CG12) */
{
  2, "Depth Cueing:        ", CYCLE_ITEM, 'u',
  {enable, disable, NULL, NULL}
};

static  struct  option_choice egret_sub10_choice=  /* functional test (CG12) */
{
  2, "Lighting & Shading:  ", CYCLE_ITEM, 'i',
  {enable, disable, NULL, NULL}
};

static  struct  option_choice egret_sub11_choice=  /* functional test (CG12) */
{
  2, "Arbitration:         ", CYCLE_ITEM, 'a',
  {enable, disable, NULL, NULL}
};

static  struct  option_choice subtest_loop_choice= /*loops per function(GT)*/
{
   1, "Loops per subtest:        ", TEXT_ITEM, 's',
   {NULL, NULL, NULL}
};

static  struct  option_choice test_sequence_loop_choice=  /*loops per board(GT)*/{
   1, "Loops per test sequence:  ", TEXT_ITEM, 'b',
   {NULL, NULL, NULL}
};


static  struct  option_choice hawk_sub1_choice=  /* functional test (GT) */
{
  2, "Video Memory:        ", CYCLE_ITEM, 'v',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub2_choice=  /* functional test (GT) */
{
  2, "CLUTs & WLUT:        ", CYCLE_ITEM, 'c',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub3_choice=  /* functional test (GT) */
{
  2, "FE Local Memory:     ", CYCLE_ITEM, 'e',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub4_choice=  /* functional test (GT) */
{
  2, "SU Shared RAM:       ", CYCLE_ITEM, 'u',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub5_choice=  /* functional test (GT) */
{
  2, "Rendering Pipeline:  ", CYCLE_ITEM, 'r',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub6_choice=  /* functional test (GT) */
{
  2, "Video Memory:        ", CYCLE_ITEM, 'o',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub7_choice=  /* functional test (GT) */
{
  2, "FB Output:           ", CYCLE_ITEM, 'p',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub8_choice=  /* functional test (GT) */
{
  2, "Vectors:             ", CYCLE_ITEM, 'w',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub9_choice=  /* functional test (GT) */
{
  2, "Triangles:           ", CYCLE_ITEM, 't',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub10_choice=  /* functional test (GT) */
{
  2, "Spline Curves:       ", CYCLE_ITEM, 'z',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub11_choice=  /* functional test (GT) */
{
  2, "Viewport Clipping:   ", CYCLE_ITEM, 'i',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub12_choice=  /* functional test (GT) */
{
  2, "Hidden Surface:      ", CYCLE_ITEM, 'j',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub13_choice=  /* functional test (GT) */
{
  2, "Edges Highlighting:  ", CYCLE_ITEM, 'y',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub14_choice=  /* functional test (GT) */
{
  2, "Transparency:        ", CYCLE_ITEM, 'n',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub15_choice=  /* functional test (GT) */
{
  2, "Depth-Cueing:        ", CYCLE_ITEM, 'q',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub16_choice=  /* functional test (GT) */
{
  2, "Lighting & Shading:  ", CYCLE_ITEM, 'g',
  {disable, enable, NULL, NULL}
};
static  struct  option_choice hawk_sub17_choice=  /* functional test (GT) */
{
  2, "Text:                ", CYCLE_ITEM, 'x',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub18_choice=  /* functional test (GT) */
{
  2, "Picking:             ", CYCLE_ITEM, 'k',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub19_choice=  /* functional test (GT) */
{
  2, "Arbitration:         ", CYCLE_ITEM, 'a',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub20_choice=  /* functional test (GT) */
{
  2, "Stereo:              ", CYCLE_ITEM, 'm',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice hawk_sub21_choice=  /* functional test (GT) */
{
  2, "LightPen:            ", CYCLE_ITEM, 'l',
  {disable, enable, NULL, NULL}
};

static  struct  option_choice mp4_shmem_choice=    /* mptest (MP4) */
{
  2, "Lock/Unlock:       ", CYCLE_ITEM, 's',
  {"enable", "disable", NULL, NULL}    
};
 
static  struct  option_choice mp4_dataio_choice=    /* mptest (MP4) */
{
  2, "Data I/O:          ", CYCLE_ITEM, 'i',
  {"enable", "disable", NULL, NULL}    
};
 
static  struct  option_choice mp4_FPU_choice=    /* mptest (MP4) */
{
  2, "FPU Check:         ", CYCLE_ITEM, 'u',
  {"enable", "disable", NULL, NULL}    
};

static  struct  option_choice mp4_cache_choice=  /*   mptest (MP4)  */
{ 
  2, "Cache Consistency: ", CYCLE_ITEM, 'c',
  {"enable", "disable", NULL, NULL}
};

struct option_choice mp4_processors_choice[30];    /* mptest (MP4) */

static  struct  option_choice bpp_access_choice=  /* bpp test (ZEBRA1) */
{
  2, "Access: ", CYCLE_ITEM, 'a',
  {"Writeonly", "Writeonly", NULL, NULL}
};

static  struct  option_choice bpp_mode_choice=     /* bpp test (ZEBRA1) */
{
  3, "Mode:   ", CYCLE_ITEM, 'm',
  {"Fast", "Medium", "Extended", NULL, NULL}
};

static  struct  option_choice lpvi_access_choice=  /* lpvi test (ZEBRA2) */
{
  2, "Access:     ", CYCLE_ITEM, 'a',
  {"Writeonly", "Writeonly", NULL, NULL}  
};
 
static  struct  option_choice lpvi_mode_choice=     /* lpvi test (ZEBRA2) */ 
{ 
  3, "Mode:       ", CYCLE_ITEM, 'm', 
  {"Fast", "Medium", "Extended", NULL, NULL} 
};

static  struct  option_choice lpvi_image_choice=   /* lpvi test (ZEBRA2) */
{
  3, "Image:      ", CYCLE_ITEM, 'i',
  {"Default", "57fonts", "UserDefined", NULL, NULL}  
};
 
static  struct  option_choice lpvi_resolution_choice=  /* lpvi test (ZEBRA2) */ 
{ 
  2, "Resolution: ", CYCLE_ITEM, 'r', 
  {"400", "300", NULL, NULL} 
};

static	struct  option_choice	internal_choice= /* internal port(SPIF) */
{
  2, "Internal:           ", CYCLE_ITEM, 'i',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	parallel_print_choice= /* printer port(SPIF) */
{
  2, "Parallel Print:     ", CYCLE_ITEM, 'p',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	sp96_choice= /* 96-pin loopback(SPIF) */
{
  2, "96-pin Loopback:    ", CYCLE_ITEM, 'z',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	db25_choice= /* 25-pin loopback(SPIF) */
{
  2, "25-pin Loopback:    ", CYCLE_ITEM, 'y',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	echo_tty_choice= /* echo tty(SPIF) */
{
  2, "Echo-TTY:           ", CYCLE_ITEM, 'e',
  {disable, enable, NULL, NULL}
};

static	struct  option_choice	baud_rate_choice= /* port baud rate(SPIF) */
{
  9, "Baud Rate:          ", CYCLE_ITEM, 'b',
  {"110", "300", "600", "1200", "2400", "4800", "9600", "19,200", "38,400", NULL, NULL}
};

static	struct  option_choice	char_size_choice=	/* char size(SPIF) */
{
  4, "Char Size:          ", CYCLE_ITEM, 'c',
  {"5", "6", "7", "8", NULL, NULL}
};

static	struct  option_choice	stop_bit_choice=	/* stop_bit (SPIF) */
{
  2, "Stop Bit:           ", CYCLE_ITEM, 's',
  {"1", "2", NULL, NULL}
};

static	struct  option_choice	parity_choice= /* parity(SPIF) */
{
  3, "Parity:             ", CYCLE_ITEM, 'r',
  {"none", "odd", "even", NULL, NULL}
};

static	struct  option_choice	flowctl_choice= /* flow control type(SPIF) */
{
  3, "Flow Control:       ", CYCLE_ITEM, 'w',
  {"xonoff", "rtscts", "both", NULL, NULL}
};

static	struct  option_choice	data_type_choice= /* test data type(SPIF) */
{
  3, "Data Type:          ", CYCLE_ITEM, 'a',
  {"0x55", "0xaa", "random", NULL, NULL}
};

static	struct  option_choice	serial_port_choice=  /* port type(SPIF)*/
{
  9, "Serial Port:        ", CYCLE_ITEM, 't',
  {"ttyz00", "ttyz01", "ttyz02", "ttyz03", "ttyz04", "ttyz05", "ttyz06",
  "ttyz07", "all", NULL, NULL}
};

static	struct  option_choice	serial_port_choice1=  /* port type(SPIF)*/
{
  9, "Serial Port:        ", CYCLE_ITEM, 't',
  {"ttyz08", "ttyz09", "ttyz0a", "ttyz0b", "ttyz0c", "ttyz0d", "ttyz0e",
  "ttyz0f", "all", NULL, NULL}
};

static	struct  option_choice	serial_port_choice2=  /* port type(SPIF)*/
{
  9, "Serial Port:        ", CYCLE_ITEM, 't',
  {"ttyz10", "ttyz11", "ttyz12", "ttyz13", "ttyz14", "ttyz15", "ttyz16",
  "ttyz17", "all", NULL, NULL}
};

static	struct  option_choice	serial_port_choice3=  /* port type(SPIF)*/
{
  9, "Serial Port:        ", CYCLE_ITEM, 't',
  {"ttyz18", "ttyz19", "ttyz1a", "ttyz1b", "ttyz1c", "ttyz1d", "ttyz1e",
  "ttyz1f", "all", NULL, NULL}
};

panel_set_processors()
{
    int i, num, b;
   
    for (b = 1, i = 0, num = 0; num < number_processors; b <<= 1, i++)
    {
	if (processors_mask & b)
	{
            (void)panel_set(processor_item[i], PANEL_VALUE, 0, 0);
            num++;
	}
    }
}

/******************************************************************************
 * option_default_proc, notify procedure for default button in option popups. *
 ******************************************************************************/
option_default_proc(item)
Panel_item item;
{
  int	test_id;
  int temp=0;

  if (!tty_mode)
    test_id = (int)panel_get(item, PANEL_CLIENT_DATA);
  else
    test_id = (int)item;

  switch (tests[test_id]->id)		/* depend on which test it is */
  {
    case PMEM:
	break;				/* does nothing */

    case VMEM:				/* wait time defaults to 0 */
	if (!tty_mode)
	{
	  (void)panel_set(item1, PANEL_VALUE, 0, 0);
	  (void)panel_set(item2, PANEL_VALUE, "0", 0);
	}
	else
	{
	  (int)tests[test_id]->data = 0; 
	  (int)tests[test_id]->special = 0; 
	}
	break;

    case AUDIO:
        if (tests[option_id]->conf->uval.devinfo.status == 0) {
                if (!tty_mode)
                {
                  (void)panel_set(item1, PANEL_VALUE, 0, 0);
                  (void)panel_set(item2, PANEL_VALUE, "100", 0);
                }
                else
                {
                  tests[test_id]->data = (caddr_t)0;
                  tests[test_id]->special = (caddr_t)100;
                }
        }         
        else {
                if (!tty_mode)
                {
                  (void)panel_set(item1, PANEL_VALUE, 0, 0);
                  (void)panel_set(item2, PANEL_VALUE, 0, 0);
                  (void)panel_set(item3, PANEL_VALUE, 0, 0);
                  (void)panel_set(item4, PANEL_VALUE, 0, 0);
                  (void)panel_set(item5, PANEL_VALUE, 0, 0);
                  (void)panel_set(item6, PANEL_VALUE, 1, 0);
                  (void)panel_set(item7, PANEL_VALUE, "", 0);
                }
                else
                {
                  tests[test_id]->special = (caddr_t)"";
                  tests[test_id]->data = (caddr_t)0x40;
                }
        }         
        break;
 
    case CPU_SP:			/* loopback defaults to a-b */
    case CPU_SP1:			/* loopback defaults to c-d */
	if (!tty_mode)
	  (void)panel_set(item1, PANEL_VALUE, 3, 0);
	else
	  (int)tests[test_id]->data = 3;
	break;

    case ENET0:
    case ENET1:
    case ENET2:
    case OMNI_NET:
    case FDDI:
    case TRNET:
	if (!tty_mode)
	{
	  (void)panel_set(item1, PANEL_VALUE, 1, 0);
	  (void)panel_set(item2, PANEL_VALUE, "10", 0);
	  (void)panel_set(item3, PANEL_VALUE, 0, 0);
	}
	else
	  (int)tests[test_id]->data = 1;
	  (int)tests[test_id]->special = 10;
	break;

    case CDROM:			/* default to other, datatests, audiotest */
	if (!tty_mode)
	{
	  (void)panel_set(item1, PANEL_VALUE, 4, 0);
	  (void)panel_set(item2, PANEL_VALUE, 0, 0);
	  (void)panel_set(item3, PANEL_VALUE, 0, 0);
	  (void)panel_set(item4, PANEL_VALUE, "255", 0);
	  (void)panel_set(item5, PANEL_VALUE, 0, 0);
	  (void)panel_set(item6, PANEL_VALUE, 0, 0);
	  (void)panel_set(item7, PANEL_VALUE, "100", 0);
	}
	else
	{
	  (int)tests[test_id]->data = 4+(100<<8); 
	  (int)tests[test_id]->special = 255;
	}
	break;

    case SCSIDISK1:		/* defaults to enabling filetest */
    case XYDISK1:		/* and raw read-only test.	 */
    case XDDISK1:
    case IPIDISK1:
    case IDDISK1:
    case SFDISK1:
    case OBFDISK1:
	temp = tests[test_id]->conf->uval.meminfo.amt;
	if ( temp <= 100 ) temp = 0;
	else
		temp = ((temp -1)/200) + 1;
	if (temp > 6) temp = 6;

	if (!tty_mode)
	{
	  (void)panel_set(item1, PANEL_TOGGLE_VALUE, 0, 1, 0);
	  (void)panel_set(item2, PANEL_TOGGLE_VALUE, 0, 1, 0);
	  (void)panel_set(item3, PANEL_VALUE, 0, 0);
	  (void)panel_set(item4, PANEL_VALUE, 2, 0);
	  (void)panel_set(item5, PANEL_VALUE, temp, 0);
	}
	else
	{
	  tests[test_id]->enable = TRUE;
	  tests[test_id+1]->enable = TRUE;
          (int)tests[test_id]->data &= ~0x7f;
          (int)tests[test_id]->data |= temp << 4;
	  (int)tests[test_id]->data |= 0x2;

	  if (!tests[test_id]->dev_enable) break;
	  /* the device was not enabled for testing */

	  select_test(test_id, tests[test_id]->enable);
	  select_test(test_id+1, tests[test_id+1]->enable);
	  print_status();
	}
	break;

    case MAGTAPE1:
    case MAGTAPE2:
    case SCSITAPE:
	if (!tty_mode)
	{
	  if (cart_tape) (void)panel_set(item1, PANEL_VALUE, 2, 0);
	  else if (hp_flt) (void)panel_set(item1, PANEL_VALUE, 3, 0);
	  else if (halfin) (void)panel_set(item1, PANEL_VALUE, 0, 0);

	  (void)panel_set(item2, PANEL_VALUE, 0, 0);
	  (void)panel_set(item3, PANEL_VALUE, 0, 0);
	  (void)panel_set(item4, PANEL_VALUE, "25300", 0);
	  if (exabyte_8200 || exabyte_8500) (void)panel_set(item5, PANEL_VALUE, 1, 0);
	  else (void)panel_set(item5, PANEL_VALUE, 0, 0);
	  (void)panel_set(item6, PANEL_VALUE, 1, 0);
	  if (ctlr_type == 3)
	    (void)panel_set(item7, PANEL_VALUE, 0, 0);
	  if (cart_tape || qic150)
	    (void)panel_set(item8, PANEL_VALUE, 0, 0);
	  (void)panel_set(item9, PANEL_VALUE, 0, 0);
	  if (hp_flt || halfin) (void)panel_set(item10, PANEL_VALUE, "25", 0);
	  else if (exabyte_8200 || exabyte_8500) (void)panel_set(item10, PANEL_VALUE, "8", 0);
	  else (void)panel_set(item10, PANEL_VALUE, "10", 0);
	}
	else
	{
	  if (hp_flt) (int)tests[test_id]->data = 0x190043;
	  else if(halfin) (int)tests[test_id]->data = 0x190040;
	  else if(exabyte_8200) (int)tests[test_id]->data = 0x80060;
	  else if(exabyte_8500) (int)tests[test_id]->data = 0x80060;
	  else (int)tests[test_id]->data = 0xa0042;
	  (unsigned)tests[test_id]->special = 25300;
	    /* 200 big blocks + 100 small blocks */
	}
	break;

    case TV1:	/* defaults to all loopback's disabled */
	if (!tty_mode)
	{
	  (void)panel_set(item1, PANEL_VALUE, 0, 0);
	  (void)panel_set(item2, PANEL_VALUE, 0, 0);
	  (void)panel_set(item3, PANEL_VALUE, 0, 0);
	  (void)panel_set(item4, PANEL_VALUE, 0, 0);
	}
	else
	  (int)tests[test_id]->data = 0;
	break;

    case IPC:	/* defaults to neither floppy drive test nor printer test */
	if (!tty_mode)
	{
	  (void)panel_set(item1, PANEL_VALUE, 0, 0);
	  (void)panel_set(item2, PANEL_VALUE, 0, 0);
	}
	else
	  (int)tests[test_id]->data = 0;
	break;

    case MCP:	/* defaults to enabling both serial and printer ports tests */
	if (!tty_mode)
	{
	  (void)panel_set(item1, PANEL_VALUE, sp2_src, 0);
	  (void)panel_set(item2, PANEL_VALUE, sp2_des, 0);
	  (void)panel_set(item3, PANEL_TOGGLE_VALUE, 0, 1, 0);
	  (void)panel_set(item4, PANEL_TOGGLE_VALUE, 0, 1, 0);
	}
	else
	{
          (void)strcpy(((struct loopback *)(tests[test_id]->data))->from,
				sp2_src);
          (void)strcpy(((struct loopback *)(tests[test_id]->data))->to,
				sp2_des);
	  tests[test_id]->enable = TRUE;
	  tests[test_id+1]->enable = TRUE;

	  if (!tests[test_id]->dev_enable) break;
	  /* the device was not enabled for testing */

	  select_test(test_id, tests[test_id]->enable);
	  select_test(test_id+1, tests[test_id+1]->enable);
	  print_status();
	}
	break;

    case MTI:		/* default for loopback configuration */
	if (!tty_mode)
	{
	  (void)panel_set(item1, PANEL_VALUE, sp_src, 0);
	  (void)panel_set(item2, PANEL_VALUE, sp_des, 0);
	}
	else
	{
          (void)strcpy(((struct loopback *)(tests[test_id]->data))->from,
				sp_src);
          (void)strcpy(((struct loopback *)(tests[test_id]->data))->to,
				sp_des);
	}
	break;

    case SCSISP1:	/* default for loopback configuration */
    case SCSISP2:
	if (!tty_mode)
	{
	  (void)panel_set(item1, PANEL_VALUE, scsisp_src, 0);
	  (void)panel_set(item2, PANEL_VALUE, scsisp_des, 0);
	}
	else
	{
          (void)strcpy(((struct loopback *)(tests[test_id]->data))->from,
				scsisp_src);
          (void)strcpy(((struct loopback *)(tests[test_id]->data))->to,
				scsisp_des);
	}
	break;

    case SCP:	/* default for loopback configuration */
	if (!tty_mode)
	{
	  (void)panel_set(item1, PANEL_VALUE, sunlink_src[0], 0);
	  (void)panel_set(item2, PANEL_VALUE, sunlink_des[0], 0);
	}
	else
	{
          (void)strcpy(((struct loopback *)(tests[test_id]->data))->from,
				sunlink_src[0]);
          (void)strcpy(((struct loopback *)(tests[test_id]->data))->to,
				sunlink_des[0]);
	}
	break;

    case SCP2:	/* default for loopback configuration */
	if (!tty_mode)
	{
	  (void)panel_set(item1, PANEL_VALUE,
			sunlink_src[tests[test_id]->unit], 0);
	  (void)panel_set(item2, PANEL_VALUE,
			sunlink_des[tests[test_id]->unit], 0);
	}
	else
	{
          (void)strcpy(((struct loopback *)(tests[test_id]->data))->from,
				sunlink_src[tests[test_id]->unit]);
          (void)strcpy(((struct loopback *)(tests[test_id]->data))->to,
				sunlink_des[tests[test_id]->unit]);
	}
	break;

    /* defaults to On-board clock, "0 1 2 3" loopback, no internal loopback */
    case SBUS_HSI:  /* ifd0-3 */
        if (!tty_mode)
        {
          (void)panel_set(item1, PANEL_VALUE, 0, 0);
          (void)panel_set(item2, PANEL_VALUE, 0, 0);
          (void)panel_set(item3, PANEL_VALUE, 4, 0);
        }
        else
          (int)tests[test_id]->data = 0x40;
        break;

    case HSI:	    /* defaults to On-board, RS449, and 0-1(2-3) */
	if (!tty_mode)
	{
	  (void)panel_set(item1, PANEL_VALUE, 0, 0);
	  (void)panel_set(item2, PANEL_VALUE, 0, 0);
	  (void)panel_set(item3, PANEL_VALUE, 3, 0);
	}
	else
	  (int)tests[test_id]->data = 3;
	break;

    case GP:		/* default to no Graphics Buffer */
	if (!tty_mode)
	  (void)panel_set(item1, PANEL_VALUE, 0, 0);
	else
	  (int)tests[test_id]->data = 0;
	break;

    case PRESTO:
        if (!tty_mode)
          (void)panel_set(item1, PANEL_VALUE, 0, 0);
        else
          (int)tests[test_id]->data = 0;
        break;

    case VFC:
	break;				/* does nothing */

    case CG12:
	if (!tty_mode)
	{
          (void)panel_set(item1, PANEL_VALUE, 0, 0);
          (void)panel_set(item2, PANEL_VALUE, 0, 0);
          (void)panel_set(item3, PANEL_VALUE, 0, 0);
          (void)panel_set(item4, PANEL_VALUE, 0, 0);
          (void)panel_set(item6, PANEL_VALUE, 0, 0);
          (void)panel_set(item5, PANEL_VALUE, 0, 0);
          (void)panel_set(item7, PANEL_VALUE, 0, 0);
          (void)panel_set(item8, PANEL_VALUE, 0, 0);
          (void)panel_set(item9, PANEL_VALUE, 0, 0);
          (void)panel_set(item10, PANEL_VALUE, 0, 0);
          (void)panel_set(item11, PANEL_VALUE, 0, 0);
          (void)panel_set(item13, PANEL_VALUE, "1", 0);	/*function_loop_choice*/
	  (void)panel_set(item14, PANEL_VALUE, "1", 0); /*board_loop_choice*/
	} else
	{
	  (int)tests[test_id]->data = 1<<12;	/* function loop count */
	  (int)tests[test_id]->special = 1;	/* board loop count */
	}
	break;
	
    case GT:
        if (!tty_mode)
        {
          (void)panel_set(item1,  PANEL_VALUE, 1, 0);
          (void)panel_set(item2,  PANEL_VALUE, 1, 0);
          (void)panel_set(item3,  PANEL_VALUE, 1, 0);
          (void)panel_set(item4,  PANEL_VALUE, 1, 0);
          (void)panel_set(item6,  PANEL_VALUE, 1, 0);
          (void)panel_set(item5,  PANEL_VALUE, 1, 0);
          (void)panel_set(item7,  PANEL_VALUE, 1, 0);
          (void)panel_set(item8,  PANEL_VALUE, 1, 0);
          (void)panel_set(item9,  PANEL_VALUE, 1, 0);
          (void)panel_set(item10, PANEL_VALUE, 1, 0);
          (void)panel_set(item11, PANEL_VALUE, 1, 0);
          (void)panel_set(item12, PANEL_VALUE, 1, 0);
          (void)panel_set(item13, PANEL_VALUE, 1, 0);
          (void)panel_set(item14, PANEL_VALUE, 1, 0);
          (void)panel_set(item15, PANEL_VALUE, 1, 0);
          (void)panel_set(item16, PANEL_VALUE, 1, 0);
          (void)panel_set(item17, PANEL_VALUE, 1, 0);
          (void)panel_set(item18, PANEL_VALUE, 1, 0);
          (void)panel_set(item19, PANEL_VALUE, 1, 0);
          (void)panel_set(item20, PANEL_VALUE, 0, 0);
          (void)panel_set(item21, PANEL_VALUE, 0, 0);
          (void)panel_set(item22, PANEL_VALUE, "1", 0);/*function_loop_choice*/
          (void)panel_set(item23, PANEL_VALUE, "1", 0);/*board_loop_choice*/
        } else
        {
          (int)tests[test_id]->data = 0x03FFFFF;    /* function loop count */
          (int)tests[test_id]->special = 1;     /* board loop count */
        }
        break;

    case MP4:
        if (!tty_mode)
        {
          panel_set_processors();
          (void)panel_set(item1, PANEL_VALUE, 0, 0);
          (void)panel_set(item2, PANEL_VALUE, 0, 0);
          (void)panel_set(item3, PANEL_VALUE, 0, 0);
          (void)panel_set(item4, PANEL_VALUE, 0, 0);
        } else
        {
          (int)tests[test_id]->data = 0;        /* enable all three tests */
        }
        break;

    case ZEBRA1:
	if (!tty_mode)
	{
          (void)panel_set(item1, PANEL_VALUE, 0, 0);	/* access_choice */
          (void)panel_set(item2, PANEL_VALUE, 0, 0);    /* mode_choice */
							/*Fast mode as default*/
	} else
	{
	  (int)tests[test_id]->data = 0<<2;	/* mode=0 for Fast */ 
	}
	break;

    case ZEBRA2:
	if (!tty_mode)
	{
          (void)panel_set(item1, PANEL_VALUE, 0, 0);	/* access_choice */
          (void)panel_set(item2, PANEL_VALUE, 0, 0);    /* mode_choice */
          (void)panel_set(item3, PANEL_VALUE, 0, 0);	/* image_choice */
          (void)panel_set(item4, PANEL_VALUE, 0, 0);    /* resolution_choice*/
	} else
	{
	  (int)tests[test_id]->data = 0<<2;    /* mode=0 for Fast */
	}
	break;

    case SPIF:	/* defaults to internal port */
	if (!tty_mode)
	{
	  (void)panel_set(item1, PANEL_VALUE, 1, 0);   /* internal port */
	  (void)panel_set(item2, PANEL_VALUE, 0, 0);   /* printer port */
	  (void)panel_set(item3, PANEL_VALUE, 0, 0);   /* 96-pin loopback */
	  (void)panel_set(item4, PANEL_VALUE, 0, 0);   /* 25-pin loopback */
	  (void)panel_set(item5, PANEL_VALUE, 0, 0);   /* echo tty */
	  (void)panel_set(item6, PANEL_VALUE, 6, 0);   /* port baud rate */
	  (void)panel_set(item7, PANEL_VALUE, 3, 0);   /* character size */
	  (void)panel_set(item8, PANEL_VALUE, 0, 0);   /* stop bit */
	  (void)panel_set(item9, PANEL_VALUE, 0, 0);   /* parity port */
	  (void)panel_set(item10, PANEL_VALUE, 1, 0);  /* flow control type */
	  (void)panel_set(item11, PANEL_VALUE, 0, 0);  /* serial port */
	  (void)panel_set(item12, PANEL_VALUE, 0, 0);  /* test data type */
	}
	else
	{
	  tests[test_id]->data = (caddr_t)0x103601;
	}
	break;

    default:
	return;				/* does nothing */
  }
}

/******************************************************************************
 * option_destroy_proc, sets up the arguments for the test according to       *
 * user's option selection before destroying the option popup without	      *
 * confirmer.								      *
 * Input: object handle(could also be the frame handle of the popup).	      *
 ******************************************************************************/
option_destroy_proc(item)
Panel_item item;
{
  int	test_id, tmp, i, num, b;

  test_id = (int)panel_get(item, PANEL_CLIENT_DATA);
  switch (tests[test_id]->id)		/* depend on which test it is */
  {
    case PMEM:
	break;				/* does nothing */

    case VMEM:
	tests[test_id]->data = (caddr_t)panel_get_value(item1);
	(int)tests[test_id]->special = atoi((char *)panel_get_value(item2));
	break;

    case AUDIO:
        if (tests[option_id]->conf->uval.devinfo.status == 0) {
		tmp = atoi((char *)panel_get_value(item2));
		if (tmp > 255) tmp = 255;
		else if (tmp < 0) tmp = 0;
		(int)tests[test_id]->special = tmp;
		tests[test_id]->data = (caddr_t)panel_get_value(item1);
        }
        else {
                sprintf(audbri_ref_file, "%s", panel_get_value(item7));
                tests[test_id]->special = audbri_ref_file;
                tests[test_id]->data = (caddr_t)(
                  ((int)panel_get_value(item6)<<6) |
                  ((int)panel_get_value(item5)<<5) |
                  ((int)panel_get_value(item4)<<4) |
                  ((int)panel_get_value(item3)<<3) |
                  ((int)panel_get_value(item2)<<2) |
                  (int)panel_get_value(item1));
        }
        break;

    case CPU_SP:
    case CPU_SP1:
	tests[test_id]->data = (caddr_t)panel_get_value(item1);
	break;

    case ENET0:
    case ENET1:
    case ENET2:
    case OMNI_NET:
    case FDDI:
    case TRNET:
	(int)tests[test_id]->data =
	  (int)panel_get_value(item1) | ((int)panel_get_value(item3)<<1);
	tmp = (atoi)((char *)panel_get_value(item2));
	if (tmp < 0) tmp = 0;
	(int)tests[test_id]->special = tmp;
	break;

    case CDROM:
	tmp = atoi((char *)panel_get_value(item7));
	if (tmp > 100) tmp = 100;
	else if (tmp < 0) tmp = 0;
	(int)tests[test_id]->data = tmp << 8;
	(int)tests[test_id]->data |= (int)panel_get_value(item1) |
	  ((int)panel_get_value(item2)<<3) | ((int)panel_get_value(item3)<<4) |
	  ((int)panel_get_value(item5)<<5) | ((int)panel_get_value(item6)<<6);
	tmp = atoi((char *)panel_get_value(item4));
	if (tmp > 255) tmp = 255;
	else if (tmp < 0) tmp = 0;
	(int)tests[test_id]->special = tmp;
	break;
	
    case SCSIDISK1:
    case XYDISK1:
    case XDDISK1:
    case IPIDISK1:
    case IDDISK1:
    case SFDISK1:
    case OBFDISK1:
	tests[test_id]->enable = (int)panel_get_value(item1);
	tests[test_id+1]->enable = (int)panel_get_value(item2);
        (int)tests[test_id]->data = (int)panel_get_value(item3) << 3 |
                                    (int)panel_get_value(item4) |
				    (int)panel_get_value(item5) << 4;

	if (!tests[test_id]->dev_enable) break;
	/* the device was not enabled for testing */

	select_test(test_id, tests[test_id]->enable);
	select_test(test_id+1, tests[test_id+1]->enable);
	select_test(test_id, tests[test_id]->data);
	print_status();
	break;

    case MAGTAPE1:
    case MAGTAPE2:
    case SCSITAPE:
	(int)tests[test_id]->data =
	  ((int)panel_get_value(item2)<<8) | ((int)panel_get_value(item3)<<3) |
	  ((int)panel_get_value(item5)<<5) | ((int)panel_get_value(item6)<<6) |
	  ((int)panel_get_value(item9)<<10)|
	  (atoi((char *)panel_get_value(item10))<<16);
	if (!qic150 && !exabyte_8200)	/* format */
	  (int)tests[test_id]->data |= (int)panel_get_value(item1);
	if (ctlr_type == 3)		/* take care of recon test option too */
	  (int)tests[test_id]->data |= ((int)panel_get_value(item7) << 7);
	if (cart_tape || qic150)
	  (int)tests[test_id]->data |= ((int)panel_get_value(item8) << 9);
	(unsigned)tests[test_id]->special =
			(unsigned)atoi((char *)panel_get_value(item4));
	break;

    case TV1:
	(int)tests[test_id]->data = (int)panel_get_value(item1) | 
				    ((int)panel_get_value(item2) << 1) |
				    ((int)panel_get_value(item3) << 2) |
				    ((int)panel_get_value(item4) << 3);
	break;

    case IPC:
	(int)tests[test_id]->data = (int)panel_get_value(item1) | 
				    ((int)panel_get_value(item2) << 1);
	break;

    case SCSISP1:
    case SCSISP2:
    case MCP:
    case MTI:
        (void)strcpy(((struct loopback *)(tests[test_id]->data))->from,
				(char *)panel_get_value(item1));
        (void)strcpy(((struct loopback *)(tests[test_id]->data))->to,
				(char *)panel_get_value(item2));

	if (tests[test_id]->id == MCP)
	{
	  tests[test_id]->enable = (int)panel_get_value(item3);
	  tests[test_id+1]->enable = (int)panel_get_value(item4);
	  /* test_id+1 is the printer test */

	  if (!tests[test_id]->dev_enable) break;
	  /* the device was not enabled for testing */

	  select_test(test_id, tests[test_id]->enable);
	  select_test(test_id+1, tests[test_id+1]->enable);
	  print_status();
	}
	break;

    case SCP:
    case SCP2:
        (void)strcpy(((struct loopback *)(tests[test_id]->data))->from,
				(char *)panel_get_value(item1));
        (void)strcpy(((struct loopback *)(tests[test_id]->data))->to,
				(char *)panel_get_value(item2));
	break;

    case SBUS_HSI:   /* refer to above sketch for data field arrangement */
        (int)tests[test_id]->data = (int)panel_get_value(item3) << 4 |
                                    (int)panel_get_value(item2) << 1 |
                                    (int)panel_get_value(item1);
        break;

    case HSI:
	(int)tests[test_id]->data = (int)panel_get_value(item1) << 4 | 
				    (int)panel_get_value(item2) << 2 |
				    (int)panel_get_value(item3);
	break;

    case GP:
	(int)tests[test_id]->data = (int)panel_get_value(item1);
	break;

    case PRESTO:
        (int)tests[test_id]->data = (int)panel_get_value(item1);
        break;

    case VFC:
	break;			/* does nothing */

    case CG12:
        tmp = atoi((char *)panel_get_value(item13));
        (int)tests[test_id]->data = tmp << 12;
        (int)tests[test_id]->data |= (int)panel_get_value(item1) |
          ((int)panel_get_value(item2)<<1) | ((int)panel_get_value(item3)<<2) |
          ((int)panel_get_value(item4)<<3) | ((int)panel_get_value(item5)<<4) |
	  ((int)panel_get_value(item6)<<5) | ((int)panel_get_value(item7)<<6) |
	  ((int)panel_get_value(item8)<<7) | ((int)panel_get_value(item9)<<8) |
	  ((int)panel_get_value(item10)<<9)| ((int)panel_get_value(item11)<<10);
        tmp = atoi((char *)panel_get_value(item14));
        (int)tests[test_id]->special = tmp;
        break;

    case GT:
        tmp = atoi((char *)panel_get_value(item22));
        (int)tests[test_id]->data = tmp << 21;
        (int)tests[test_id]->data |= (int)panel_get_value(item1) |
        ((int)panel_get_value(item2)<<1)   | ((int)panel_get_value(item3)<<2) |
        ((int)panel_get_value(item4)<<3)   | ((int)panel_get_value(item5)<<4) |
        ((int)panel_get_value(item6)<<5)   | ((int)panel_get_value(item7)<<6) |
        ((int)panel_get_value(item8)<<7)   | ((int)panel_get_value(item9)<<8) |
        ((int)panel_get_value(item10)<<9)  | ((int)panel_get_value(item11)<<10)|
        ((int)panel_get_value(item12)<<11) | ((int)panel_get_value(item13)<<12)|
        ((int)panel_get_value(item14)<<13) | ((int)panel_get_value(item15)<<14)|
        ((int)panel_get_value(item16)<<15) | ((int)panel_get_value(item17)<<16)|
        ((int)panel_get_value(item18)<<17) | ((int)panel_get_value(item19)<<18)|
        ((int)panel_get_value(item20)<<19) | ((int)panel_get_value(item21)<<20);
        tmp = atoi((char *)panel_get_value(item23));
        (int)tests[test_id]->special = tmp;
        break;

    case MP4:
        (int)tests[test_id]->data = (int)panel_get_value(item1) |
                                    (int)panel_get_value(item2)<<1 | 
                                    (int)panel_get_value(item3)<<2 |
                                    (int)panel_get_value(item4)<<3;
        for (b = 1, i = 0, num = 0; num < number_processors; b <<= 1, i++)
	{
	    if (processors_mask & b)
	    {
		(int)tests[test_id]->data |= 
                   ( ((int)panel_get_value(processor_item[i])) << (4+i) );
		num++;
	    }
	}
        break;

    case ZEBRA1:
	(int)tests[test_id]->data = (int)panel_get_value(item1) |
			(int)panel_get_value(item2)<<2;
	break;
    case ZEBRA2:
	(int)tests[test_id]->data = (int)panel_get_value(item1) |
	((int)panel_get_value(item2)<<2) | ((int)panel_get_value(item3)<<4) |
	((int)panel_get_value(item4)<<6);
	break;

    case SPIF:
	tests[test_id]->data = (caddr_t)(
	   (int)panel_get_value(item1)| 
	  ((int)panel_get_value(item2)<<1) | 
	  ((int)panel_get_value(item3)<<2) |
	  ((int)panel_get_value(item4)<<3) |
	  ((int)panel_get_value(item5)<<4) | 
	  ((int)panel_get_value(item8)<<5) | 
	  ((int)panel_get_value(item6)<<8) | 
	  ((int)panel_get_value(item7)<<12) | 
	  ((int)panel_get_value(item9)<<16) |
	  ((int)panel_get_value(item10)<<20) |
	  ((int)panel_get_value(item11)<<24) |
	  ((int)panel_get_value(item12)<<28) );
	break;

    default:
	break;
  }

  (void)window_set(option_frame, FRAME_NO_CONFIRM, TRUE, 0);
  (void)window_destroy(option_frame);
  option_frame = NULL;
}

/******************************************************************************
 * panel_label(), sets the label of the option popup.			      *
 * Input: label, the string to be the label of the popup.		      *
 ******************************************************************************/
panel_label(label)
char	*label;			/* the option label */
{
  if (!tty_mode)
    (void)window_set(option_frame, FRAME_LABEL, label, 0);
  else
    (void)mvwprintw(tty_option, 0, (58-strlen(label))/2,
			"<< Test Options - %s >>", label);	/* centerized */
}

/******************************************************************************
 * panel_printf() prints one line of message on the panel subwindow.	      *
 * Input: panel_handle, the panel to display the message on.		      *
 *	  msg, the message to be displayed.				      *
 *	  x, y, the starting character position of the message.		      *
 ******************************************************************************/
#define	TTY_COL_OFFSET	23	/* column offset from the SunView version */
#define	TTY_ROW_OFFSET	3	/* row offset from the SunView version */
panel_printf(panel_handle, msg, col, row)
Panel_item	panel_handle;	/* the panel to display the message on */
char		*msg;		/* message to be printed */
int		col, row;	/* starting character position of the message */
{
  if (!tty_mode)
    (void)panel_create_item(panel_handle, PANEL_MESSAGE,
		PANEL_LABEL_STRING,	msg,
		PANEL_ITEM_X,		ATTR_COL(col),
		PANEL_ITEM_Y,		ATTR_ROW(row),
		0);
  else
    (void)mvwaddstr(tty_option, row+TTY_ROW_OFFSET, col+TTY_COL_OFFSET, msg);
}

/******************************************************************************
 * option_item(), initialize the CYCLE item on option popup and prints its    *
 * value on the option window in TTY mode.				      *
 ******************************************************************************/
static	Panel_item	option_item(panel_handle, choice, value, col, row)
Panel	panel_handle;
struct	option_choice	*choice;
int	value;				/* index to choice->choice */
int	col, row;
{
  Panel_item	item;
  char	buf[82];

  if (tty_mode)
  {
    if (value >= choice->num) value = 0;
    (void)sprintf(buf, "[%c]%s%c  %s", choice->com-0x20, choice->label,
		choice->type==TOGGLE_ITEM?':':' ', choice->choice[value]);
    (void)mvwaddstr(tty_option, row+TTY_ROW_OFFSET, col+TTY_COL_OFFSET, buf);
    wclrtoeol(tty_option);
    return((Panel_item)0);
  }
  else
  {
    switch (choice->type)
    {
      case CYCLE_ITEM:
	item = panel_create_item(panel_handle,	PANEL_CYCLE,
		PANEL_LABEL_STRING,     choice->label,
		PANEL_ITEM_X,		ATTR_COL(col),
		PANEL_ITEM_Y,		ATTR_ROW(row),
		0);

	if (choice->num <= 4)
	  (void)panel_set(item, PANEL_CHOICE_STRINGS,
		choice->choice[0], choice->choice[1],
		choice->choice[2], choice->choice[3], 0, 0);
	else if (choice->num <= 8)
	  (void)panel_set(item, PANEL_CHOICE_STRINGS,
		choice->choice[0], choice->choice[1],
		choice->choice[2], choice->choice[3],
		choice->choice[4], choice->choice[5],
		choice->choice[6], choice->choice[7], 0, 0);
	else
	  (void)panel_set(item, PANEL_CHOICE_STRINGS,
		choice->choice[0], choice->choice[1],
		choice->choice[2], choice->choice[3],
		choice->choice[4], choice->choice[5],
		choice->choice[6], choice->choice[7],
		choice->choice[8], choice->choice[9],
		choice->choice[10], choice->choice[11],
		choice->choice[12], choice->choice[13],
		choice->choice[14], choice->choice[15], 0, 0);

	(void)panel_set(item, PANEL_VALUE, value, 0);

	break;

      case TOGGLE_ITEM:
	item = panel_create_item(panel_handle, PANEL_TOGGLE,
		PANEL_CHOICE_STRINGS,	choice->label, 0,
		PANEL_TOGGLE_VALUE,	0, value,
        	PANEL_ITEM_X,           ATTR_COL(col),
        	PANEL_ITEM_Y,           ATTR_ROW(row),
		0);
	break;

      case TEXT_ITEM:
	item = panel_create_item(panel_handle,	PANEL_TEXT,
		PANEL_LABEL_STRING,     choice->label,
		PANEL_VALUE,  		choice->choice[value],
		PANEL_ITEM_X,		ATTR_COL(col),
		PANEL_ITEM_Y,		ATTR_ROW(row),
		0);
	break;

      default:
	return((Panel_item)0);
    }

    return(item);
  }
}

/******************************************************************************
 * option_item2(), supports 2 column format in the option window in TTY mode. *
 * NOTE: extra arg column_num;  0 for first column and 1 for second column    *
 ******************************************************************************/
static	Panel_item	
option_item2(panel_handle, choice, value, col, row, column_num, row_num)
Panel	panel_handle;
struct	option_choice	*choice;
int	value;				/* index to choice->choice */
int	col;
int	row;
int     column_num;
int	row_num;
{
  Panel_item	item;
  char	buf[82];

  if (tty_mode)
  {
    if (value >= choice->num) value = 0;
    (void)sprintf(buf, "[%c]%s%c", choice->com-0x20, choice->label,
		choice->type==TOGGLE_ITEM?':':' ');
    (void)mvwaddstr(tty_option, row_num+TTY_ROW_OFFSET, (column_num?45:8), buf);
    wclrtoeol(tty_option);

    (void)sprintf(buf, "%s", choice->choice[value]);
    (void)mvwaddstr(tty_option, row_num+TTY_ROW_OFFSET, (column_num?66:28), buf);
    wclrtoeol(tty_option);

    return((Panel_item)0);
  }
  else
  {
	return(option_item(panel_handle, choice, value, col, row));
  }
}

int
init_processors_choice(panel_handle, test_id)
Panel panel_handle;
int test_id;
{
    int i, num; 
    unsigned int b; 
    char processor_name[30], processor_enable[30], processor_disable[30];

    for (i = 0, b = 1, num = 0; num < number_processors; b <<= 1, i++)
	if (processors_mask & b)
	{
	    mp4_processors_choice[i].num = 2;

            bzero(processor_name, 30);
            sprintf(processor_name, "Processor %d", i);
	    mp4_processors_choice[i].label = (char *)malloc(sizeof(processor_name));
	    strcpy(mp4_processors_choice[i].label, processor_name);

	    mp4_processors_choice[i].type = CYCLE_ITEM;

	    mp4_processors_choice[i].com = i + 'a';

            bzero(processor_enable, 30);
            sprintf(processor_enable, "enable");
	    mp4_processors_choice[i].choice[0] = (char *)malloc(sizeof(processor_enable));
	    strcpy(mp4_processors_choice[i].choice[0], processor_enable);

            bzero(processor_disable, 30);
            sprintf(processor_disable, "disable");
	    mp4_processors_choice[i].choice[1] = (char *)malloc(sizeof(processor_disable));
	    strcpy(mp4_processors_choice[i].choice[1], processor_disable);

	    mp4_processors_choice[i].choice[2] = NULL;
	    mp4_processors_choice[i].choice[3] = NULL;

            processor_item[i] = 
                        option_item(panel_handle, &mp4_processors_choice[i],
                        ((int)tests[test_id]->data&(0x10 << i)) >> (4+i), 4, 2+num);
	    num++;
	}
    return(num);
} 

/******************************************************************************
 * Initialize the items in the option popup, depends on which test it is.     *
 * Input: test_id, internal test number.				      *
 * Output: the popup panel's handle.					      *
 ******************************************************************************/
Panel	init_opt_panel(test_id)
int		test_id;
{
  int	done_row=0, add_row;
  int	unit, i;
  float amt;
  Panel	panel_handle;
  char	tmp[62];			/* buffer for messages on popup's */
  char	tmp1[62];			/* buffer for messages on popup's */
  char	*tapename;
  struct option_choice	*tmp_choice;
  char  *tmpname="/tmp/sundiag.XXXXXX";
  char  enet_buffer[120];               /* buffer for ENET calls */
  char  hostname[40];                   /* buffer stores hostname */
  char  domainname[40];                 /* buffer stores domainname */
  int   ypup;                           /* if yp running, & tmp pointer */
  static  long  hostid;                 /* to store hex value for ENET info */
  FILE  *enet_fp;                       /* points to tmpname */
  struct hostent *hp;                   /* host entry info for ENET info */

  if (!tty_mode)
    panel_handle = window_create(option_frame, PANEL, 0); /* create the panel */

  switch (tests[test_id]->id)		/* depend on which test it is */
  {
    case PMEM:
	panel_label("Physical Memory");
	panel_printf(panel_handle, "Configurations:", 1, 0);
	(void)sprintf(tmp, "Amount: %d MB",
		tests[test_id]->conf->uval.meminfo.amt);
	panel_printf(panel_handle, tmp, 4, 1);
	panel_printf(panel_handle, "Sub-tests:  None", 1, 3);
	panel_printf(panel_handle, "Options:    None", 1, 5);
	done_row = 6;
	break;

    case VMEM:
	panel_label("Virtual Memory");
	panel_printf(panel_handle, "Configurations:", 1, 0);
	(void)sprintf(tmp, "Amount: %d MB",
		tests[test_id]->conf->uval.meminfo.amt);
	panel_printf(panel_handle, tmp, 4, 1);
	panel_printf(panel_handle, "Sub-tests:  None", 1, 3);
	panel_printf(panel_handle, "Options:", 1, 5);

	item1 = option_item(panel_handle, &wait_choice,
				(int)tests[test_id]->data, 4, 6);

	(void)sprintf(tmp1, "%d", (int)tests[test_id]->special);
	reserve_choice.choice[0] = tmp1;
	item2 = option_item(panel_handle, &reserve_choice, 0, 4, 7);
	if (!tty_mode)
	  (void)panel_set(item2, 
		PANEL_VALUE_DISPLAY_LENGTH,	6,
		PANEL_VALUE_STORED_LENGTH,	6,
		0);

	done_row = 8;
	break;

    case AUDIO:
        if (tests[option_id]->conf->uval.devinfo.status == 0) {
        panel_label(tests[test_id]->label);
        panel_printf(panel_handle, "Configurations: None", 1, 0);
        panel_printf(panel_handle, "Options:", 1, 1);

        item1 = option_item(panel_handle, &audio_choice,
                                (int)tests[test_id]->data&1, -1, 2);
        (void)sprintf(tmp1, "%d", (int)tests[test_id]->special);
        loud_choice.choice[0] = tmp1;
        item2 = option_item(panel_handle, &loud_choice, 0, -1, 3);
        if (!tty_mode)
        {
          (void)panel_set(item2,
                PANEL_VALUE_DISPLAY_LENGTH,     3,
                PANEL_VALUE_STORED_LENGTH,      3,
                0);
        }

        done_row = 4;
        }
        else {
        panel_label(tests[test_id]->label);
        panel_printf(panel_handle, "Configurations: None", 1, 0);
        panel_printf(panel_handle, "Options:", 1, 1);
 
        item6 = option_item(panel_handle, &audbri_audio_choice,
                                ((int)tests[test_id]->data & 0x40) >> 6, 4, 7);
        item5 = option_item(panel_handle, &audbri_controls_choice,
                                ((int)tests[test_id]->data & 0x20) >> 5, 4, 6);
        item4 = option_item(panel_handle, &audbri_crystal_choice,
                                ((int)tests[test_id]->data & 0x10) >> 4, 4, 5);
        item3 = option_item(panel_handle, &audbri_loop_calib_choice,
                                ((int)tests[test_id]->data & 0x8) >> 3, 4, 4);
        item2 = option_item(panel_handle, &audbri_loop_choice,
                                ((int)tests[test_id]->data & 0x4) >> 2, 4, 3);
        item1 = option_item(panel_handle, &audbri_loop_type_choice,
                                (int)tests[test_id]->data & 0x1 , 4, 2);
         
        audbri_ref_file_choice.choice[0] =(char *)tests[test_id]->special;
        item7 = option_item(panel_handle, &audbri_ref_file_choice, 0, 4, 8);
        if (!tty_mode) {
          (void)panel_set(item7,
                PANEL_VALUE_DISPLAY_LENGTH,     25,
                PANEL_VALUE_STORED_LENGTH,      200,
                0);
        }
        done_row = 9;
        }
        break;
 
    case CPU_SP:
	panel_label("On-Board TTY Ports");
	panel_printf(panel_handle, "Configurations:", 1, 0);
	panel_printf(panel_handle, "Ports: ttya & ttyb", 4, 1);
	panel_printf(panel_handle, "Sub-tests:  None", 1, 3);
	panel_printf(panel_handle, "Options:", 1, 5);

	item1 = option_item(panel_handle, &cpusp_choice,
				(int)tests[test_id]->data, 4, 6);

	done_row = 7;
	break;

    case CPU_SP1:
	panel_label("On-Board TTY Ports");
	panel_printf(panel_handle, "Configurations:", 1, 0);
	panel_printf(panel_handle, "Ports: ttyc & ttyd", 4, 1);
	panel_printf(panel_handle, "Sub-tests:  None", 1, 3);
	panel_printf(panel_handle, "Options:", 1, 5);

	item1 = option_item(panel_handle, &cpusp1_choice,
				(int)tests[test_id]->data, 4, 6);

	done_row = 7;
	break;

    case ENET0:
    case ENET1:
    case ENET2:
    case OMNI_NET:
    case FDDI:
    case TRNET:
	(void)sprintf(tmp, "%s #%d Information", tests[test_id]->label, 
			tests[test_id]->unit);
	panel_label(tmp);

	panel_printf(panel_handle, "Configurations:", 1, 0);
        mktemp(tmpname);
        (void)sprintf(enet_buffer,"/usr/ucb/netstat -i | /bin/grep '^%s' | \
            /bin/awk '{ print $4 }' > %s ", tests[test_id]->devname, tmpname);
        system(enet_buffer);
        enet_buffer[0]= '\0';

        if (( enet_fp = fopen(tmpname, "r")) == NULL) {
                (void)sprintf(tmp1,"fopen error for %s\n", tmpname);
		logmsg(tmp1, 1, FAILED_NET_INFO);
        } 
        fgets(enet_buffer, 40, enet_fp);
        fclose(enet_fp);

        if (enet_buffer[0] != '\0') {
                for (i=0; i < 40 && enet_buffer[i] != '\n'; ++i);
                enet_buffer[i]='\0';
        } 

        (void)sprintf(tmp,"Host Name:   %s", enet_buffer);
        panel_printf(panel_handle, tmp, 8, 1);
 
        hostid = gethostid();
        (void)sprintf(tmp,"Host ID:   %x", hostid);
        panel_printf(panel_handle, tmp, 10, 2);
 
        if ((hp = gethostbyname(enet_buffer)) == NULL) {
		(void)sprintf(tmp1, "can't get host entry");
		logmsg(tmp1, 1, FAILED_NET_INFO);
        	(void)sprintf(tmp1,"Host Address:  No Host Entry");
        	panel_printf(panel_handle, tmp1, 5, 3);
        } else {
        	(void)sprintf(tmp,"Host Address:   %d.%d.%d.%d  ",
        	hp->h_addr[0] & 0xff, hp->h_addr[1] & 0xff, 
		hp->h_addr[2] & 0xff, hp->h_addr[3] & 0xff);
        	panel_printf(panel_handle, tmp, 5, 3);
	}
 
        if ((ypup = getdomainname(domainname, 40)) == -1) {
                (void)sprintf(tmp1, "Can't get domain entry YP not running");
		logmsg(tmp1, 1, FAILED_NET_INFO);
                (void)sprintf(tmp1, "Domain Name:   N/A - YP not running");
                panel_printf(panel_handle, tmp1, 6, 4);
        } else {
                (void)sprintf(tmp,"Domain Name:   %s", domainname);
                panel_printf(panel_handle, tmp, 6, 4);
        }

	panel_printf(panel_handle, "Sub-tests:  None", 1, 6);
	panel_printf(panel_handle, "Options:", 1, 8);
	item1 = option_item(panel_handle, &spray_choice,
			(int)tests[test_id]->data & 0x01, 4, 9);
	(void)sprintf(tmp1, "%d", (int)tests[test_id]->special);
	delay_choice.choice[0] = tmp1;
	item2 = option_item(panel_handle, &delay_choice,
			0, 4, 10);
	item3 = option_item(panel_handle, &driver_choice,
			((int)tests[test_id]->data & 0x02) >> 1, 4, 11);
        enet_buffer[0]= '\0';
        (void)sprintf(enet_buffer, "/bin/rm -rf %s", tmpname);
        system(enet_buffer);
        enet_buffer[0]= '\0';
        done_row = 12;

	if (!tty_mode)
	{
	  (void)panel_set(item2, 
		PANEL_VALUE_DISPLAY_LENGTH,	4,
		PANEL_VALUE_STORED_LENGTH,	4,
		0);
	}

        break;

    case CDROM:
	(void)sprintf(tmp,"%s%d",tests[test_id]->label, tests[test_id]->unit);
	panel_label(tmp);
	panel_printf(panel_handle, "Configurations:", 1, 0);
	(void)sprintf(tmp, "Capacity: %d MB",
		tests[test_id]->conf->uval.diskinfo.amt);
	panel_printf(panel_handle, tmp, 4, 1);
	unit = tests[test_id]->conf->uval.diskinfo.ctlr_type;
	(void)sprintf(tmp, "Controller: %s",
		disk_ctlr[unit>=NDISKCTLR?0:unit]);
	panel_printf(panel_handle, tmp, 4, 2);
	(void)sprintf(tmp, "Host Adaptor: %s%d",
		tests[test_id]->conf->uval.diskinfo.ctlr,
		tests[test_id]->conf->uval.diskinfo.ctlr_num);
	panel_printf(panel_handle, tmp, 4, 3);
	panel_printf(panel_handle, "Sub-tests:  None", 1, 4);
	panel_printf(panel_handle, "Options:", 1, 5);

	item1 = option_item(panel_handle, &cdtype_choice,
				(int)tests[test_id]->data & 7, 4, 6);
	item2 = option_item(panel_handle, &datatest1_choice,
				((int)tests[test_id]->data & 8) >> 3, 4, 7);
	item6 = option_item(panel_handle, &datatest2_choice,
				((int)tests[test_id]->data & 0x40) >> 6, 4, 8);
	(void)sprintf(tmp1, "%d", (int)tests[test_id]->data >> 8);
	datatrack_choice.choice[0] = tmp1;
	item7 = option_item(panel_handle, &datatrack_choice, 0, 4, 9);
	item5 = option_item(panel_handle, &readmode_choice,
				((int)tests[test_id]->data & 0x20) >> 5, 4, 10);

	item3 = option_item(panel_handle, &audiotest_choice,
				((int)tests[test_id]->data & 0x10) >> 4, 4, 11);
	(void)sprintf(tmp1, "%d", (int)tests[test_id]->special);
	volume_choice.choice[0] = tmp1;
	item4 = option_item(panel_handle, &volume_choice, 0, 4, 12);
	if (!tty_mode)
	{
	  (void)panel_set(item4, 
		PANEL_VALUE_DISPLAY_LENGTH,	3,
		PANEL_VALUE_STORED_LENGTH,	3,
		0);
	  (void)panel_set(item7, 
		PANEL_VALUE_DISPLAY_LENGTH,	3,
		PANEL_VALUE_STORED_LENGTH,	3,
		0);
	}

	done_row = 13;
	break;

    case SCSIDISK1:
    case XYDISK1:
    case XDDISK1:
    case IPIDISK1:
    case IDDISK1:
    case SFDISK1:
    case OBFDISK1:
	if (tests[test_id]->id != IDDISK1)
	  (void)sprintf(tmp,"%s%d",tests[test_id]->label, tests[test_id]->unit);
	else
          (void)sprintf(tmp,"%s%03x",
			tests[test_id]->label, tests[test_id]->unit);
	panel_label(tmp);
	panel_printf(panel_handle, "Configurations:", 1, 0);
	amt = tests[test_id]->conf->uval.diskinfo.amt;
	(void)sprintf(tmp, "Capacity: %.1f MB", amt/1000);
	panel_printf(panel_handle, tmp, 4, 1);
	unit = tests[test_id]->conf->uval.diskinfo.ctlr_type;
	(void)sprintf(tmp, "Controller: %s",
		disk_ctlr[unit>=NDISKCTLR?0:unit]);
	panel_printf(panel_handle, tmp, 4, 2);
	if (tests[test_id]->conf->uval.diskinfo.ctlr_num != -1)
	  (void)sprintf(tmp, "Host Adaptor: %s%d",
		tests[test_id]->conf->uval.diskinfo.ctlr,
		tests[test_id]->conf->uval.diskinfo.ctlr_num);
	else
	  (void)sprintf(tmp, "Host Adaptor: %s",
		tests[test_id]->conf->uval.diskinfo.ctlr);
	panel_printf(panel_handle, tmp, 4, 3);
	panel_printf(panel_handle, "Sub-tests:", 1, 5);

	item1 = option_item(panel_handle, &raw_choice,
				tests[test_id]->enable, 4, 6);

	item2 = option_item(panel_handle, &fs_choice,
				tests[test_id+1]->enable, 4, 7);

	panel_printf(panel_handle, "Options:", 1, 9);

	item3 = option_item(panel_handle, &raw_op_choice1,
				((int)tests[test_id]->data&0x8) >> 3, 4, 10);
	item4 = option_item(panel_handle, &raw_op_choice2,
				(int)tests[test_id]->data&0x7, 4, 11);
	item5 = option_item(panel_handle, &raw_op_choice3,
				((int)tests[test_id]->data&0x70) >> 4, 4, 12);
	if (!tty_mode)
	{
	  (void)panel_set(item2, PANEL_NOTIFY_PROC, disk_rawtest_proc, 0);
	  (void)panel_set(item3, PANEL_NOTIFY_PROC, disk_rawtest_proc, 0);
	}

	done_row = 12;
	break;

    case MAGTAPE1:
    case MAGTAPE2:
    case SCSITAPE:
	(void)sprintf(tmp, "%s%d", tests[test_id]->label, tests[test_id]->unit);
	panel_label(tmp);
	panel_printf(panel_handle, "Configurations:", 1, 0);

	tapename = "Unknown";
	qic150 = FALSE;
	exabyte_8200 = FALSE;
	exabyte_8500 = FALSE;
	hp_flt = FALSE;
	halfin = FALSE;
	cart_tape = FALSE;
	for (i=0; tapes[i].t_type != 0; ++i)
	  if (tests[test_id]->conf->uval.tapeinfo.t_type == tapes[i].t_type)
	  {
	    tapename = tapes[i].t_name;
	    if (tapes[i].t_type==MT_ISVIPER1 || tapes[i].t_type==MT_ISWANGTEK1)
	      qic150 = TRUE;
	    else if (tapes[i].t_type==MT_ISEXABYTE)
	      exabyte_8200 = TRUE;
#ifdef NEW
	    else if (tapes[i].t_type==MT_ISEXB8500)
	      exabyte_8500 = TRUE; /* Add support for 5 G.B. drives too */
#endif NEW
	    else if (tapes[i].t_type==MT_ISHP || tapes[i].t_type==MT_ISKENNEDY)
	      hp_flt = TRUE;
	    else if (tapes[i].t_type==MT_ISXY || tapes[i].t_type==MT_ISCPC)
	      halfin = TRUE;
	    else
	      cart_tape = TRUE;
	    break;
	  }
	
	if (tapes[i].t_type == 0)	/* can't find the name */
	{
	  if (strcmp(tests[test_id]->conf->uval.tapeinfo.ctlr, "xtc") == 0 ||
	      strcmp(tests[test_id]->conf->uval.tapeinfo.ctlr, "tm") == 0)
	    halfin = TRUE;
	  else
	    cart_tape = TRUE;		/* default to SCSI tape */
	}

	(void)sprintf(tmp, "Drive Type: %s%s", tapename,
	    tests[test_id]->conf->uval.tapeinfo.status==FLT_COMP?"(DC)":"");
	panel_printf(panel_handle, tmp, 4, 1);
	if (strcmp(tests[test_id]->conf->uval.tapeinfo.ctlr, "sc") != 0 &&
		(cart_tape || qic150))
	  ctlr_type = 3;			/* SCSI3, can do recon */
	else
	  ctlr_type = 2;			/* SCSI2 or other tape drives */
	(void)sprintf(tmp, "Host Adaptor: %s%d",
		tests[test_id]->conf->uval.tapeinfo.ctlr,
		tests[test_id]->conf->uval.tapeinfo.ctlr_num);
	panel_printf(panel_handle, tmp, 4, 2);

	done_row = 3;
	panel_printf(panel_handle, "Options:", 1, done_row++);

	if (exabyte_8500)
            if (tests[test_id]->conf->uval.tapeinfo.status == FLT_COMP)
	   	item1 = option_item(panel_handle, &density_choice_exb8500,
			(int)tests[test_id]->data & 3, 4, done_row++);
	    else
	   	item1 = option_item(panel_handle, &density_choice_exb8500_nodc,
			(int)tests[test_id]->data & 3, 4, done_row++);

	else if (!qic150 && !exabyte_8200)
	  if (hp_flt)
	  {
	    if (tests[test_id]->conf->uval.tapeinfo.status == FLT_COMP)
	      item1 = option_item(panel_handle, &density_choice,
			(int)tests[test_id]->data & 7, 4, done_row++);
	    else
	      item1 = option_item(panel_handle, &density_choice_nodc,
			(int)tests[test_id]->data & 3, 4, done_row++);
	  }
	  else if (halfin)
	    item1 = option_item(panel_handle, &density_choice_halfin,
			(int)tests[test_id]->data & 3, 4, done_row++);
	  else
	    item1 = option_item(panel_handle, &format_choice,
			(int)tests[test_id]->data & 3, 4, done_row++);

	item2 = option_item(panel_handle, &mode_choice,
			((int)tests[test_id]->data & 0x100) >> 8, 4,done_row++);
	item3 = option_item(panel_handle, &length_choice,
			((int)tests[test_id]->data & 0x18) >> 3, 4, done_row++);

	(void)sprintf(tmp, "%u", (unsigned)(tests[test_id]->special));
	block_choice.choice[0] = tmp;
	item4 = option_item(panel_handle, &block_choice, 0, 4, done_row++);
	if (!tty_mode)
	  (void)panel_set(item4, 
		PANEL_VALUE_DISPLAY_LENGTH,	10,
		PANEL_VALUE_STORED_LENGTH,	10,
		0);

	item5 = option_item(panel_handle, &file_choice,
			((int)tests[test_id]->data & 0x20) >> 5, 4, done_row++);
	item6 = option_item(panel_handle, &stream_choice,
			((int)tests[test_id]->data & 0x40) >> 6, 4, done_row++);

	if (ctlr_type == 3)	/* allow the SCSI-reconnect on SCSI3 only */
	  item7 = option_item(panel_handle, &recon_choice,
			((int)tests[test_id]->data & 0x80) >> 7, 4, done_row++);

	if (cart_tape || qic150)
	  item8 = option_item(panel_handle, &retension_choice,
			((int)tests[test_id]->data & 0x200) >> 9, 4,done_row++);

	item9 = option_item(panel_handle, &clean_choice,
			((int)tests[test_id]->data & 0x400) >> 10,4,done_row++);

	(void)sprintf(tmp1, "%u", (unsigned)(tests[test_id]->data) >> 16);
	pass_choice.choice[0] = tmp1;
	item10 = option_item(panel_handle, &pass_choice, 0, 4, done_row++);
	if (!tty_mode)
	  (void)panel_set(item10, 
		PANEL_VALUE_DISPLAY_LENGTH,	10,
		PANEL_VALUE_STORED_LENGTH,	10,
		0);
	break;

    case TV1:
	(void)strcpy(tmp, "Video Display Board");
	panel_label(tmp);
	panel_printf(panel_handle, "Configurations: None", 1, 0);
	panel_printf(panel_handle, "Sub-tests:  None", 1, 2);
	panel_printf(panel_handle, "Options:", 1, 4);
	item1 = option_item(panel_handle, &ntsc_choice,
			(int)tests[test_id]->data & 1, 4, 5);
	item2 = option_item(panel_handle, &yc_choice,
			((int)tests[test_id]->data & 2) >> 1, 4, 6);
	item3 = option_item(panel_handle, &yuv_choice,
			((int)tests[test_id]->data & 4) >> 2, 4, 7);
	item4 = option_item(panel_handle, &rgb_choice,
			((int)tests[test_id]->data & 8) >> 3, 4, 8);

	done_row = 9;
	break;

    case IPC:
	(void)sprintf(tmp, "%s%d", "Integrated PC #", tests[test_id]->unit);
	panel_label(tmp);
	panel_printf(panel_handle, "Configurations: None", 1, 0);
	panel_printf(panel_handle, "Sub-tests:  None", 1, 2);
	panel_printf(panel_handle, "Options:", 1, 4);
	item1 = option_item(panel_handle, &floppy_choice,
			(int)tests[test_id]->data & 1, 4, 5);
	item2 = option_item(panel_handle, &printer_choice,
			((int)tests[test_id]->data & 2) >> 1, 4, 6);

	done_row = 7;
	break;

    case SCSISP1:
    case SCSISP2:
    case MCP:
    case MTI:
    case SCP:
    case SCP2:
	if (tests[test_id]->id == MCP)
	{
	  (void)sprintf(tmp, "%s%d", "Asynchronous Line Mux 2 #",
		tests[test_id]->unit);
	  panel_label(tmp);
	  unit = (unsigned int)'h' + tests[test_id]->unit;
	  (void)sprintf(tmp, "Ports: tty%c0-tty%cf", unit, unit);
	  panel_printf(panel_handle, "Configurations:", 1, 0);
	  panel_printf(panel_handle, tmp, 4, 1);
	  panel_printf(panel_handle, "Sub-tests:", 1, 3);
	  done_row = 4;
	  item3 = option_item(panel_handle, &sp_choice,
			tests[test_id]->enable, 4, done_row++);
	  item4 = option_item(panel_handle, &pp_choice,
			tests[test_id+1]->enable, 4, done_row++);
	  ++done_row;
	}
	else			/* without "Sub-tests" */
	{
	  if (tests[test_id]->id == MTI)
	  {
	    (void)sprintf(tmp, "%s%d", "Asynchronous Line Mux #",
		tests[test_id]->unit);
	    (void)sprintf(tmp1, "Ports: tty%d0-tty%df",
		tests[test_id]->unit, tests[test_id]->unit);
	  }
	  else if (tests[test_id]->id == SCP)
	  {
	    (void)sprintf(tmp, "%s%d", "SunLink Communication Processor #",
		tests[test_id]->unit);
	    unit = (unsigned int)'a' + tests[test_id]->unit;
	    (void)sprintf(tmp1, "Ports: dcp%c0-dcp%c3            ",
								unit, unit);
	  }
	  else if (tests[test_id]->id == SCP2)
	  {
	    (void)sprintf(tmp, "%s%d", "SunLink Communication Processor 2 #",
		tests[test_id]->unit);
	    (void)sprintf(tmp1, "Ports: mcph%d-mcph%d              ",
		tests[test_id]->unit*4, tests[test_id]->unit*4 + 3);
	  }
	  else
	  {
	    if (tests[test_id]->id == SCSISP1)
	    {
	      unit = 's';
	      (void)strcpy(tmp, "First SCSI TTY Ports");
	    }
	    else
	    {
	      unit = 't';
	      (void)strcpy(tmp, "Second SCSI TTY Ports"); 
	    }
	    (void)sprintf(tmp1, "Ports: tty%c0-tty%c3", unit, unit);
	  }
	  panel_label(tmp);
	  panel_printf(panel_handle, "Configurations:", 1, 0);
	  panel_printf(panel_handle, tmp1, 4, 1);
	  panel_printf(panel_handle, "Sub-tests:  None", 1, 3);
	  done_row = 5;
	}

	panel_printf(panel_handle, "Options:", 1, done_row++);
	panel_printf(panel_handle, "Loopback Assignments:", 4, done_row++);

	from_choice.choice[0] =
		((struct loopback *)(tests[test_id]->data))->from;
	item1 = option_item(panel_handle, &from_choice, 0, 4, done_row++);

	to_choice.choice[0] =
		((struct loopback *)(tests[test_id]->data))->to;
	item2 = option_item(panel_handle, &to_choice, 0, 4, done_row++);

	if (!tty_mode)
	{
	  (void)panel_set(item1, 
		PANEL_VALUE_DISPLAY_LENGTH,	16,
		PANEL_VALUE_STORED_LENGTH,	21,
		0);

	  (void)panel_set(item2, 
		PANEL_VALUE_DISPLAY_LENGTH,	16,
		PANEL_VALUE_STORED_LENGTH,	21,
		0);
	}

	break;

    case SBUS_HSI:  
        (void)sprintf(tmp, "%s%d", "High Speed Serial Interface #",
                tests[test_id]->unit);
        panel_label(tmp);
        panel_printf(panel_handle, "Configurations:", 1, 0);

        (void)sprintf(tmp, "Ports:     %d, %d, %d, %d",
                tests[test_id]->unit*4, tests[test_id]->unit*4 + 1,
                tests[test_id]->unit*4 + 2, tests[test_id]->unit*4 + 3);
        panel_printf(panel_handle, tmp, 4, 1);

        (void)sprintf(tmp, "Port type:  RS449");
        panel_printf(panel_handle, tmp, 4, 2);

        (void)sprintf(tmp, "Protocol:   HDLC");
        panel_printf(panel_handle, tmp, 4, 3);

        panel_printf(panel_handle, "Sub-tests:  None", 1, 5);
        panel_printf(panel_handle, "Options:", 1, 7);

        item1 = option_item(panel_handle, &sbus_clock_choice,
                        ((int)tests[test_id]->data & 1), 4, 8);
 
        item2 = option_item(panel_handle, &internal_loopback_choice,
                        ((int)tests[test_id]->data & 2) >> 1, 4, 9);
 
        if (tests[test_id]->unit == 0)
          tmp_choice = &sbus_loopback_choice;
        else if (tests[test_id]->unit == 1)
          tmp_choice = &sbus_loopback_choice1;
        else if (tests[test_id]->unit == 2)
          tmp_choice = &sbus_loopback_choice2;

        item3 = option_item(panel_handle, tmp_choice,
                        ((int)tests[test_id]->data & 0x070) >> 4, 4, 10);

        if (!tty_mode)
        {
          (void)panel_set(item1, PANEL_NOTIFY_PROC, sbus_hsi_mixport_proc,
0);
          (void)panel_set(item2, PANEL_NOTIFY_PROC, sbus_hsi_mixport_proc,
0);
          (void)panel_set(item3, PANEL_NOTIFY_PROC, sbus_hsi_mixport_proc,
0);
        }

        done_row = 11;
        break;

    case HSI:
	(void)sprintf(tmp, "%s%d", "High Speed Serial Interface #",
		tests[test_id]->unit);
	panel_label(tmp);
	panel_printf(panel_handle, "Configurations:", 1, 0);

	(void)sprintf(tmp, "Ports: hss%d & hss%d",
		tests[test_id]->unit*2, tests[test_id]->unit*2 + 1);
	panel_printf(panel_handle, tmp, 4, 1);

	panel_printf(panel_handle, "Sub-tests:  None", 1, 3);
	panel_printf(panel_handle, "Options:", 1, 5);

	item1 = option_item(panel_handle, &clock_choice,
			((int)tests[test_id]->data & 16) >> 4, 4, 6);
	item2 = option_item(panel_handle, &type_choice,
			((int)tests[test_id]->data & 12) >> 2, 4, 7);

	if (tests[test_id]->unit == 0)
	  tmp_choice = &loopback_choice;
	else
	  tmp_choice = &loopback_choice1;

	item3 = option_item(panel_handle, tmp_choice,
			(int)tests[test_id]->data & 3, 4, 8);

	if (!tty_mode)
	{
	  (void)panel_set(item2, PANEL_NOTIFY_PROC, hsi_mixport_proc, 0);
	  (void)panel_set(item3, PANEL_NOTIFY_PROC, hsi_mixport_proc, 0);
	}

	done_row = 9;
	break;

    case GP:
	panel_label("Graphics Processor");
	panel_printf(panel_handle, "Configurations:  None", 1, 0);
	panel_printf(panel_handle, "Sub-tests:  None", 1, 2);
	panel_printf(panel_handle, "Options:", 1, 4);
	item1 = option_item(panel_handle, &gb_choice,
			(int)tests[test_id]->data & 1, 4, 5);
	done_row = 6;
	break;

    case PRESTO:
	panel_label("NFS Accelerator");
	panel_printf(panel_handle, "Configurations:", 1, 0);
        (void)sprintf(tmp, "Cache Size: %d KB",
                tests[test_id]->conf->uval.meminfo.amt);
        panel_printf(panel_handle, tmp, 4, 1);
        panel_printf(panel_handle, "Sub-tests:  None", 1, 3);
        panel_printf(panel_handle, "Options:    None", 1, 5);
        done_row = 6;
	item1 = option_item(panel_handle, &presto_ratio_choice,
			(int)tests[test_id]->data & 1, 4, 7);
        done_row = 8;
        break;
	
    case CG12:
        (void)sprintf(tmp, "%s%d", "Color FB GS #",
                tests[test_id]->unit);
        panel_label(tmp);
        panel_printf(panel_handle, "Options:", 1, 0);
        item1 = option_item(panel_handle, &egret_sub1_choice,
                        (int)tests[test_id]->data & 0x1 , 4, 1);
        item2 = option_item(panel_handle, &egret_sub2_choice,
                        ((int)tests[test_id]->data & 0x2) >> 1, 4, 2);
        item3 = option_item(panel_handle, &egret_sub3_choice,
                        ((int)tests[test_id]->data & 0x4) >> 2, 4, 3);
        item4 = option_item(panel_handle, &egret_sub4_choice,
                        ((int)tests[test_id]->data & 0x8) >> 3, 4, 4);
        item5 = option_item(panel_handle, &egret_sub5_choice,
                        ((int)tests[test_id]->data & 0x10) >> 4, 4, 5);
        item6 = option_item(panel_handle, &egret_sub6_choice,
                        ((int)tests[test_id]->data & 0x20) >> 5, 4, 6);
        item7 = option_item(panel_handle, &egret_sub7_choice,
                        ((int)tests[test_id]->data & 0x40) >> 6, 4, 7);
        item8 = option_item(panel_handle, &egret_sub8_choice,
                        ((int)tests[test_id]->data & 0x80) >> 7, 4, 8);
        item9 = option_item(panel_handle, &egret_sub9_choice,
                        ((int)tests[test_id]->data & 0x100) >> 8, 4, 9);
        item10 = option_item(panel_handle, &egret_sub10_choice,
                        ((int)tests[test_id]->data & 0x200) >> 9, 4, 10);
        item11 = option_item(panel_handle, &egret_sub11_choice,
                        ((int)tests[test_id]->data & 0x400) >> 10, 4, 11);

        (void)sprintf(tmp1, "%d", (int)tests[test_id]->data >> 12);
        function_loop_choice.choice[0] = tmp1;
        item13 = option_item(panel_handle, &function_loop_choice, 0, 4, 12);
        (void)sprintf(tmp1, "%d", (int)tests[test_id]->special);
        board_loop_choice.choice[0] = tmp1;
        item14 = option_item(panel_handle, &board_loop_choice, 0, 4, 13);
        if (!tty_mode)
        {
          (void)panel_set(item13,
                PANEL_VALUE_DISPLAY_LENGTH,     7,
                PANEL_VALUE_STORED_LENGTH,      7,
                0);
          (void)panel_set(item14,
                PANEL_VALUE_DISPLAY_LENGTH,     7,
                PANEL_VALUE_STORED_LENGTH,      7,
                0);
        }
        done_row = 14;
        break;

    case GT:
        (void)sprintf(tmp, "%s%d", "Graphics Tower GT#", tests[test_id]->unit);
        panel_label(tmp);
        if (!tty_mode)
        {
                panel_printf(panel_handle, "Options:", 1, 0);
                panel_printf(panel_handle, "DIRECT PORT:", 4, 1);
        }
        else
        {   
                panel_printf(panel_handle, "Options:", -18, 0);
                panel_printf(panel_handle, "DIRECT PORT:", -17, 1);
        }

        item1 = option_item2(panel_handle, &hawk_sub1_choice,
                        (int)tests[test_id]->data & 0x1 ,         4, 2, 0, 2);
        item2 = option_item2(panel_handle, &hawk_sub2_choice,
                        ((int)tests[test_id]->data & 0x2) >> 1,   4, 3, 0, 3);

        if (!tty_mode)
        {
                panel_printf(panel_handle, "ACCELERATOR PORT:", 4, 5);
        }
        else
        {   
                panel_printf(panel_handle, "ACCELERATOR PORT:", -15, 5);
        }

        item3 = option_item2(panel_handle, &hawk_sub3_choice,
                        ((int)tests[test_id]->data & 0x4) >> 2,   4, 6, 0, 5);
        item4 = option_item2(panel_handle, &hawk_sub4_choice,
                        ((int)tests[test_id]->data & 0x8) >> 3,   4, 7, 0, 6);
        item5 = option_item2(panel_handle, &hawk_sub5_choice,
                        ((int)tests[test_id]->data & 0x10) >> 4,  4, 8, 0, 7);
        item6 = option_item2(panel_handle, &hawk_sub6_choice,
                        ((int)tests[test_id]->data & 0x20) >> 5,  4, 9, 0, 8);
        item7 = option_item2(panel_handle, &hawk_sub7_choice,
                        ((int)tests[test_id]->data & 0x40) >> 6,  4, 10, 0,9);

        if (!tty_mode)
        {
                panel_printf(panel_handle, "INTEGRATION TEST:", 4, 12);
        }
        else
        {   
                panel_printf(panel_handle, "INTEGRATION TEST:", -17, 10);
        }

        item8 = option_item2(panel_handle, &hawk_sub8_choice,
                        ((int)tests[test_id]->data & 0x80) >> 7,  4, 13, 0, 11);
        item9 = option_item2(panel_handle, &hawk_sub9_choice,
                        ((int)tests[test_id]->data & 0x100) >> 8, 4, 14, 0, 12);
        item10 = option_item2(panel_handle, &hawk_sub10_choice,
                        ((int)tests[test_id]->data & 0x200) >> 9, 4, 15, 0, 13);
        item11 = option_item2(panel_handle, &hawk_sub11_choice,
                        ((int)tests[test_id]->data & 0x400) >> 10, 4, 16, 1, 0);
        item12 = option_item2(panel_handle, &hawk_sub12_choice,
                        ((int)tests[test_id]->data & 0x800) >> 11, 4, 17, 1, 1);
        item13 = option_item2(panel_handle, &hawk_sub13_choice,
                        ((int)tests[test_id]->data & 0x1000) >> 12, 4, 18,1,2);
        item14 = option_item2(panel_handle, &hawk_sub14_choice,
                        ((int)tests[test_id]->data & 0x2000) >> 13, 4, 19,1,3);
        item15 = option_item2(panel_handle, &hawk_sub15_choice,
                        ((int)tests[test_id]->data & 0x4000) >> 14, 4, 20,1,4);
        item16 = option_item2(panel_handle, &hawk_sub16_choice,
                        ((int)tests[test_id]->data & 0x8000) >> 15, 4, 21,1,5);
        item17 = option_item2(panel_handle, &hawk_sub17_choice,
                        ((int)tests[test_id]->data & 0x10000) >> 16,4, 22,1,6);
        item18 = option_item2(panel_handle, &hawk_sub18_choice,
                        ((int)tests[test_id]->data & 0x20000) >> 17,4, 23,1,7);
        item19 = option_item2(panel_handle, &hawk_sub19_choice,
                        ((int)tests[test_id]->data & 0x40000) >> 18, 4,24,1,8);
        item20 = option_item2(panel_handle, &hawk_sub20_choice,
                        ((int)tests[test_id]->data & 0x80000) >> 19, 4,25,1,9);
        item21 = option_item2(panel_handle, &hawk_sub21_choice,
                        ((int)tests[test_id]->data & 0x100000) >> 20,4,26,1,10);
        (void)sprintf(tmp1, "%d", (int)tests[test_id]->data >> 21);
        subtest_loop_choice.choice[0] = tmp1;
        item22 = option_item2(panel_handle, &subtest_loop_choice, 0,4,28,1,12);
        (void)sprintf(tmp1, "%d", (int)tests[test_id]->special);
        test_sequence_loop_choice.choice[0] = tmp1;
        item23 = option_item2(panel_handle, &test_sequence_loop_choice, 0, 4,29,1,13);
        if (!tty_mode)
        {
          (void)panel_set(item22,
                PANEL_VALUE_DISPLAY_LENGTH,     4,
                PANEL_VALUE_STORED_LENGTH,      4,
                0);
          (void)panel_set(item22, PANEL_NOTIFY_PROC, hawk_subtest_loop_proc, 0);
          (void)panel_set(item23,
                PANEL_VALUE_DISPLAY_LENGTH,     7,
                PANEL_VALUE_STORED_LENGTH,      7,
                0);
        }
        done_row = 30;
        break;

    case MP4:
        (void)sprintf(tmp,"%s", "Multiprocessors");
        panel_label(tmp);
        panel_printf(panel_handle, "Configurations:", 1, 0);
        (void)sprintf(tmp, "# of processors: %d",
                tests[test_id]->unit);
        panel_printf(panel_handle, tmp, 4, 1);
        add_row = init_processors_choice(panel_handle, test_id);
        panel_printf(panel_handle, "Sub-tests: None", 1, 3+add_row);
 
        panel_printf(panel_handle, "Options:", 1, 5+add_row);
        item1 = option_item(panel_handle, &mp4_shmem_choice,
                        (int)tests[test_id]->data & 0x1 , 4, 6+add_row);
        item2 = option_item(panel_handle, &mp4_dataio_choice,
                        ((int)tests[test_id]->data & 0x2) >> 1, 4, 7+add_row);
        item3 = option_item(panel_handle, &mp4_FPU_choice,
                        ((int)tests[test_id]->data & 0x4) >> 2, 4, 8+add_row);
        item4 = option_item(panel_handle, &mp4_cache_choice,
                        ((int)tests[test_id]->data & 0x8) >> 3, 4, 9+add_row);
        done_row = 10 + add_row;
        break;

    case ZEBRA1:
        (void)sprintf(tmp,"%s%d", "Parallel Port #", tests[test_id]->unit);
        panel_label(tmp);
        panel_printf(panel_handle, "Configurations:", 1, 0);
        (void)sprintf(tmp, "Printing Device: %s",
                tests[test_id]->devname);
        panel_printf(panel_handle, tmp, 4, 1);
        panel_printf(panel_handle, "Sub-tests: None", 1, 3);
 
        panel_printf(panel_handle, "Options:", 1, 5);
        item1 = option_item(panel_handle, &bpp_access_choice,
                        (int)tests[test_id]->data & 0x3 , 4, 6);
        item2 = option_item(panel_handle, &bpp_mode_choice,
                        ((int)tests[test_id]->data & 0xC) >> 2, 4, 7);

        if (!tty_mode)
        {
          (void)panel_set(item1, PANEL_NOTIFY_PROC, zebra_access_proc, 0);
        }
	done_row = 8;
	break;

    case ZEBRA2: 
        (void)sprintf(tmp,"%s%d", "Video Port #", tests[test_id]->unit);
        panel_label(tmp);
        panel_printf(panel_handle, "Configurations:", 1, 0);
        (void)sprintf(tmp, "Printing Device: %s",
                tests[test_id]->devname);
        panel_printf(panel_handle, tmp, 4, 1);   
        panel_printf(panel_handle, "Sub-tests: None", 1, 3);
 
        panel_printf(panel_handle, "Options:", 1, 5); 
        item1 = option_item(panel_handle, &lpvi_access_choice, 
                        (int)tests[test_id]->data & 0x3 , 4, 6); 
        item2 = option_item(panel_handle, &lpvi_mode_choice,    
                        ((int)tests[test_id]->data & 0xC) >> 2, 4, 7);
        item3 = option_item(panel_handle, &lpvi_image_choice,  
                        ((int)tests[test_id]->data & 0x30) >> 4 , 4, 8); 
        item4 = option_item(panel_handle, &lpvi_resolution_choice,     
                        ((int)tests[test_id]->data & 0xC0) >> 6, 4, 9);

        if (!tty_mode) 
        { 
          (void)panel_set(item1, PANEL_NOTIFY_PROC, zebra_access_proc, 0); 
          (void)panel_set(item3, PANEL_NOTIFY_PROC, lpvi_image_proc, 0); 

        }
        done_row = 10;
        break; 

    case SPIF:
	(void)strcpy(tmp, "Serial Parallel InterFace"); 
	panel_label(tmp);

	panel_printf(panel_handle, "Configurations:", 1, 0);
	if (tests[test_id]->unit == 0)
	   (void)sprintf(tmp1, "Ports: ttyz0-ttyz07");
	else if (tests[test_id]->unit == 1)
	   (void)sprintf(tmp1, "Ports: ttyz08-ttyz0f");
	else if (tests[test_id]->unit == 2)
	   (void)sprintf(tmp1, "Ports: ttyz10-ttyz17");
	panel_printf(panel_handle, tmp1, 4, 1);
	if (tests[test_id]->unit == 0)
	   (void)sprintf(tmp1, "  stclp0");
	else if (tests[test_id]->unit == 1)
	   (void)sprintf(tmp1, "  stclp1");
	else if (tests[test_id]->unit == 2)
	   (void)sprintf(tmp1, "  stclp2");
	panel_printf(panel_handle, tmp1, 9, 2);
	if (!tty_mode)
	   panel_printf(panel_handle, "Options:", 1, 3);
	else
	   panel_printf(panel_handle, "Options:", -18, 4);

	item3 = option_item2(panel_handle, &sp96_choice,
			(((int)tests[test_id]->data & 4) >> 2), 4, 4, 0, 5);
	item1 = option_item2(panel_handle, &internal_choice,
			((int)tests[test_id]->data & 1), 4, 5, 1, 5);
	item4 = option_item2(panel_handle, &db25_choice,
			((int)tests[test_id]->data & 8) >> 3, 4, 6, 0, 6);
	item2 = option_item2(panel_handle, &parallel_print_choice,
			((int)tests[test_id]->data & 2) >> 1, 4, 7, 1, 6);
	item5 = option_item2(panel_handle, &echo_tty_choice,
			((int)tests[test_id]->data & 16) >> 4, 4, 8, 0, 7);
	item7 = option_item2(panel_handle, &char_size_choice,
			((int)tests[test_id]->data & 0xf000) >> 12, 4, 9, 1, 7);
	item8 = option_item2(panel_handle, &stop_bit_choice,
			((int)tests[test_id]->data & 32) >> 5, 4, 10, 0, 8);
	item6 = option_item2(panel_handle, &baud_rate_choice,
			((int)tests[test_id]->data & 0xf00) >> 8, 4, 11, 1, 8);
	item9 = option_item2(panel_handle, &parity_choice,
		((int)tests[test_id]->data & 0xf0000) >> 16, 4, 12, 0, 9);
	item10 = option_item2(panel_handle, &flowctl_choice,
		((int)tests[test_id]->data & 0xf00000) >> 20, 4, 13, 1, 9);

	item12 = option_item2(panel_handle, &data_type_choice,
	     (int)((int)tests[test_id]->data & 0xf0000000) >> 28, 4, 14, 0, 10);

	if (tests[test_id]->unit == 0) {
		item11 = option_item2(panel_handle, &serial_port_choice,
		((int)tests[test_id]->data & 0xf000000) >> 24, 4, 15, 1, 10);
	}
	else if (tests[test_id]->unit == 1) {
		item11 = option_item2(panel_handle, &serial_port_choice1,
		((int)tests[test_id]->data & 0xf000000) >> 24, 4, 15, 1, 10);
	}
	else if (tests[test_id]->unit == 2) {
		item11 = option_item2(panel_handle, &serial_port_choice2,
		((int)tests[test_id]->data & 0xf000000) >> 24, 4, 15, 1, 10);
	}
	else if (tests[test_id]->unit == 3) {
		item11 = option_item2(panel_handle, &serial_port_choice3,
		((int)tests[test_id]->data & 0xf000000) >> 24, 4, 15, 1, 10);
	}

        if (!tty_mode) 
        { 
          (void)panel_set(item3, PANEL_NOTIFY_PROC, spif_tests_proc, 0); 
          (void)panel_set(item4, PANEL_NOTIFY_PROC, spif_tests_proc, 0); 
          (void)panel_set(item2, PANEL_NOTIFY_PROC, spif_tests_proc, 0); 
          (void)panel_set(item5, PANEL_NOTIFY_PROC, spif_tests_proc, 0); 
        }

	if (tty_mode)
	  done_row = 12;
	else
	  done_row = 17;
	break;

    default:
	break;
  }

  if (!tty_mode)
  {
    ++done_row;				/* get the row # for the confirmers */

    (void)panel_create_item(panel_handle, PANEL_BUTTON,
	PANEL_LABEL_IMAGE, panel_button_image(panel_handle, "Default",
						7, (Pixfont *)NULL),
	PANEL_ITEM_X,		ATTR_COL(1),
	PANEL_ITEM_Y,		ATTR_ROW(done_row),
	PANEL_CLIENT_DATA,	test_id,
	PANEL_NOTIFY_PROC,	option_default_proc,
	0);

    (void)panel_create_item(panel_handle, PANEL_BUTTON,
	PANEL_LABEL_IMAGE, panel_button_image(panel_handle, "Done",
						7, (Pixfont *)NULL),
	PANEL_CLIENT_DATA,	test_id,
	PANEL_NOTIFY_PROC,	option_destroy_proc,
	0);
  }
  else					/* display it on TTY screen */
  {
    touchwin(tty_option);
    (void)wrefresh(tty_option);
    (void)refresh();			/* restore the cursor */
  }

  return(panel_handle);
}

/******************************************************************************
 * Notify procedure for the "Rawtest" cycle in the disk test popup.           *
 ******************************************************************************/
/*ARGSUSED*/
static	disk_rawtest_proc(item, value, event)
Panel_item item;
int	value;
Event	*event;
{
  if (item == item3)	/* for "rawtest" option */
  {
    if (value == 1 && (int)panel_get_value(item2) == 1)
    /* if user is trying to select "write/read" option with fstest enabled */
    {
      (void)panel_set(item, PANEL_VALUE, 0, 0);	/* force back to "readonly" */
      (void)confirm(
	"\"Write/Read\" option is not available when Filetest is enabled!",
	TRUE);
    }
  }
  else			/* for "Filetest" selection */
  {
    if (value == 1 && (int)panel_get_value(item3) == 1)
    /* if user is trying to select fstest with "write/read" rawtest enabled */
    {
      (void)panel_set(item, PANEL_TOGGLE_VALUE,0,0,0);	/* force to "Disable" */
      (void)confirm(
        "Can't enable Filetest when \"Write/Read\" Rawtest option is selected!",
	TRUE);
    }
  }
}

/******************************************************************************
 * Notify procedure for the "Loops per subtest" option in GT popup.
 ******************************************************************************/
static  hawk_subtest_loop_proc(item, value, event)
Panel_item item;
int     value;
Event   *event;
{
  int loop_num_value;

  loop_num_value = (int)panel_get_value(item22);

  if (loop_num_value > 1023)
  {
        (void)panel_set(item22, PANEL_VALUE, "1023", 0);
  }
}

/******************************************************************************
 * Notify procedure for the "Port Type" & "Loopback" toggles in SBUS-HSI popup. ******************************************************************************/
static	sbus_hsi_mixport_proc(item, value, event)
Panel_item item;
int	value;
Event	*event;
{
  int clock_value, internal_lpbk_value, loopback_value;
 
  clock_value            = (int)panel_get_value(item1);
  internal_lpbk_value    = (int)panel_get_value(item2);
  loopback_value         = (int)panel_get_value(item3);
 
  if (item == item1)                                /* clock source item */  {
    if ((value == 1) && (internal_lpbk_value == 1)) 
        (void)panel_set(item2, PANEL_VALUE, 0, 0);  /* force internal lpbk=0  */
  }  
  else  if (item == item2)                          /* internal loopbk item   */
  {
    if ((value !=0) && ((loopback_value >= 5) || (clock_value))) 
        (void)panel_set(item2, PANEL_VALUE, 0, 0);  /* force intl lpbk=0 if it's */                                                         
  }                                                 /* port to port lpbk or  */                                                             
                                                    /* external clock source  */
 
  else                                              /* loopback item */
  {
    if (value >= 5)                                 /* if port to port, then  */                                                            
        (void)panel_set(item2, PANEL_VALUE, 0, 0);  /* force internal lpbk=0  */
  }  
}

/******************************************************************************
 * Notify procedure for the "Port Type" and "Loopback" toggles in HSI popup.   *
 ******************************************************************************/
/*ARGSUSED*/
static	hsi_mixport_proc(item, value, event)
Panel_item item;
int	value;
Event	*event;
{
  if (item == item2)		/* Port Type item */
  {
    if (value == 3)		/* mixed port loopback */
      (void)panel_set(item3, PANEL_VALUE, 3, 0);    /* force 0-1 looback */
  }
  else				/* must be Loopback item */
  {
    if (value != 3 && (int)panel_get_value(item2) == 3)
      (void)panel_set(item2, PANEL_VALUE, 0, 0);    /* force RS449 type */
  }
}

/******************************************************************************
 * Notify procedure for "access" cycle in ZEBRA popup.
 *****************************************************************************/ 
static  zebra_access_proc(item, value, event)
Panel_item item;
int     value;
Event   *event;
{
  if (item == item1 && value == 1)            /* access type  item */
  {
      (void)panel_set(item1, PANEL_VALUE, 0, 0);  /* force back to writeonly */
      (void)confirm("Only \"Writeonly\" option is supported at this time.",
		TRUE);
  }
}

/******************************************************************************
 * Notify procedure for "image" cycle in ZEBRA lpvi popup.
 *****************************************************************************/
static  lpvi_image_proc(item, value, event)
Panel_item item;
int     value;
Event   *event;
{
  if (item == item3 && value == 2)   /* image type item with UserDefined value*/
  {
      (void)confirm("Please use \"u_image\" as UserDefined filename.",
                TRUE);
  }
}
/******************************************************************************
 * Notify procedure for "SPIF" spiftest test selection popup.                 *
 ******************************************************************************/
/*ARGSUSED*/
static	spif_tests_proc(item, value, event)
Panel_item item;
int	value;
Event	*event;
{
  if (item == item3)	/* for "sp-96" option */
  {
    /* if user selecting "db-25, print or echo-tty" option with sp-96 enabled */
    if (value == 1 && ( ((int)panel_get_value(item2) == 1) || 
       ((int)panel_get_value(item4) == 1) || ((int)panel_get_value(item5) == 1) ) )
    {
      (void)panel_set(item, PANEL_VALUE, 0, 0);	/* force back to "readonly" */
      (void)confirm(
        "Can't enable SP-96 when DB-25, Print or echo-tty test option is selected!",
	TRUE);
    }
  }
  else if (item == item4)	/* for "db-25" selection */
  {
    /* if user selecting "db-25 or sp-96 and echo-tty" option */
    if (value == 1 && ( ((int)panel_get_value(item5) == 1) ||
       (int)panel_get_value(item3) == 1) )
    {
      (void)panel_set(item, PANEL_VALUE, 0, 0);	/* force back to "disable" */
      (void)confirm(
        "Can't enable DB-25 when echo-tty or SP-96 test option is selected!", 
	TRUE);
    }
  }
  else if (item == item5)	/* for "echo-tty" selection */
  {
    /* if user selecting "db-25 or sp-96 and echo-tty" option */
    if (value == 1 && ( ((int)panel_get_value(item4) == 1) ||
       (int)panel_get_value(item3) == 1) )
    {
      (void)panel_set(item, PANEL_VALUE, 0, 0);	/* force back to "disable" */
      (void)confirm(
        "Can't enable echo-tty when DB-25 or SP-96 test option is selected!", 
	TRUE);
    }
  }
  else if (item == item2)	/* for "Parallel Print" selection */
  {
    /* if user selecting "SP-96 and Print" option */
    if (value == 1 && ((int)panel_get_value(item3) == 1) )
    {
      (void)panel_set(item, PANEL_VALUE, 0, 0);	/* force back to "disable" */
      (void)confirm(
        "Can't enable Parallel Print when SP-96 test option is selected!", 
	TRUE);
    }
  }
}

/******************************************************************************
 * set_max_tests(), sets the "max" field in the groups[] structure to the     *
 *	value passed in.						      *
 * Input: max_num, the max. # of tests allowed to be concurrently running     *
 *			in a group.					      *
 * Output: none.							      *
 ******************************************************************************/
set_max_tests(max_num)
int	max_num;
{
  int	i;

  for (i=0; i != ngroups; ++i)
    groups[i].max_no = max_num;	/* that's it */
}

/******************************************************************************
 * tty_test_option_proc(), processes the TTY version of test option commands. *
 * Output: TRUE, if needs to update the screen afterward; otherwise FALSE.    *
 ******************************************************************************/
tty_test_option_proc()
{
  int	tmp, tmp1, tmp2;

  if (arg_no <= 2 && strlen(arg[0]) == 1)
  /* only single character and up-to-two-argument commands at this time */
  {
    switch (tests[option_id]->id)	/* depend on which test option it is */
    {
      case PMEM:
	break;			/* does nothing(print the error message) */

      case VMEM:
	if (arg_no == 1 && *arg[0] == wait_choice.com)
	{
	  ++(int)tests[option_id]->data;
	  if ((int)tests[option_id]->data == wait_choice.num)
	    (int)tests[option_id]->data = 0;
	  return(TRUE);
	}
	else if (arg_no == 2 && *arg[0] == reserve_choice.com)
	{
	  (int)tests[option_id]->special = atoi(arg[1]);
	  return(TRUE);
	}
	break;

      case AUDIO:
        if (tests[option_id]->conf->uval.devinfo.status == 0) {
                if (arg_no == 1 && *arg[0] == audio_choice.com)
                {
                  tests[option_id]->data = (caddr_t)((int)tests[option_id]->data + 1);
                  if ((int)tests[option_id]->data == audio_choice.num)
                    tests[option_id]->data = (caddr_t)0;
                  return(TRUE);
                } 
                else if (arg_no == 2 && *arg[0] == loud_choice.com)
                {
                  tmp = atoi(arg[1]);
                  if (tmp > 255) tmp = 255;
                  else if (tmp < 0) tmp = 0;
                  tests[option_id]->special = (char *)tmp;
                  return(TRUE);
                } 
        }
        else {
                if (arg_no == 1)
                {
                  if (*arg[0] == audbri_loop_type_choice.com)
                  {
                        tmp = (int)tests[option_id]->data & 0x1;
                        if (++tmp == audbri_loop_type_choice.num)
                                tmp = 0;
                        tests[option_id]->data = (caddr_t)(
                        ((int)tests[option_id]->data & ~0x1) | tmp);
                        return(TRUE);
                  }
                  if (*arg[0] == audbri_loop_choice.com)
                  {
                    tests[option_id]->data =
                        (caddr_t) ((int)tests[option_id]->data ^ 0x04);
     /* TOGGLE bit 2 */
                    return(TRUE);
                  }
                  else if (*arg[0] == audbri_loop_calib_choice.com)
                  {
                    tests[option_id]->data =
                        (caddr_t) ((int)tests[option_id]->data ^ 0x08);
     /* TOGGLE bit 3 */
                    return(TRUE);
                  }
                  else if (*arg[0] == audbri_crystal_choice.com)
                  {
                    tests[option_id]->data =
                        (caddr_t) ((int)tests[option_id]->data ^ 0x10);
     /* TOGGLE bit 4 */
                    return(TRUE);
                  }
                  else if (*arg[0] == audbri_controls_choice.com)
                  {
                    tests[option_id]->data =
                        (caddr_t) ((int)tests[option_id]->data ^ 0x20);
     /* TOGGLE bit 5 */
                    return(TRUE);
                  }
                  else if (*arg[0] == audbri_audio_choice.com)
                  {
                    tests[option_id]->data =
                        (caddr_t) ((int)tests[option_id]->data ^ 0x40);
     /* TOGGLE bit 6 */
                    return(TRUE);
                  }
                }   
                else if (arg_no == 2 && *arg[0] == audbri_ref_file_choice.com)
                {
                  sprintf(audbri_ref_file, "%s", arg[1]);
                  tests[option_id]->special = audbri_ref_file;
                  return(TRUE);
                }
        }         
 
        break;
 
      case CPU_SP:
      case CPU_SP1:		/* use the same "loopback" command as CPU_SP */
	if (arg_no == 1 && *arg[0] == cpusp_choice.com)
	{
	  ++(int)tests[option_id]->data;
	  if ((int)tests[option_id]->data == cpusp_choice.num)
	    (int)tests[option_id]->data = 0;
	  return(TRUE);
	}
	break;

      case ENET0:
      case ENET1:
      case ENET2:
      case OMNI_NET:
      case FDDI:
      case TRNET:
	if (arg_no == 1)
	{
	  if (*arg[0] == spray_choice.com)
	  {
	    (int)tests[option_id]->data ^= 0x01;
	    return(TRUE);
	  }
	  else if (*arg[0] == driver_choice.com)
	  {
	    (int)tests[option_id]->data ^= 0x02;
	    return(TRUE);
	  }
	}
	else if (arg_no == 2 && *arg[0] == delay_choice.com)
	{
	  tmp = atoi(arg[1]);
	  if (tmp < 0) tmp = 0;
	  (int)tests[option_id]->special = tmp;
	  return(TRUE);
	}
	break;

      case CDROM:
	if (arg_no == 1)
	{
	  if (*arg[0] == cdtype_choice.com)
	  {
	    tmp = (int)tests[option_id]->data & 7;
	    if (++tmp == cdtype_choice.num)
	      tmp = 0;
	    (int)tests[option_id]->data =
		((int)tests[option_id]->data & ~7) | tmp;
	    return(TRUE);
	  }
	  else if (*arg[0] == datatest1_choice.com)
	  {
	    (int)tests[option_id]->data ^= 8;
	    return(TRUE);
	  }
	  else if (*arg[0] == datatest2_choice.com)
	  {
	    (int)tests[option_id]->data ^= 0x40;
	    return(TRUE);
	  }
	  else if (*arg[0] == audiotest_choice.com)
	  {
	    (int)tests[option_id]->data ^= 0x10;
	    return(TRUE);
	  }
	  else if (*arg[0] == readmode_choice.com)
	  {
	    (int)tests[option_id]->data ^= 0x20;
	    return(TRUE);
	  }
	}
	else if (arg_no == 2 && *arg[0] == volume_choice.com)
	{
	  tmp = atoi(arg[1]);
	  if (tmp > 255) tmp = 255;
	  else if (tmp < 0) tmp = 0;
	  (int)tests[option_id]->special = tmp;
	  return(TRUE);
	}
	else if (arg_no == 2 && *arg[0] == datatrack_choice.com)
	{
	  tmp = atoi(arg[1]);
	  if (tmp > 100) tmp = 100;
	  else if (tmp < 0) tmp = 0;
	  (int)tests[option_id]->data = ((int)tests[option_id]->data & ~0xFF00)
					| (tmp << 8);
	  return(TRUE);
	}
	break;

      case SCSIDISK1:
      case XYDISK1:
      case XDDISK1:
      case IPIDISK1:
      case IDDISK1:
      case SFDISK1:
      case OBFDISK1:
	if (arg_no == 1)
	{
	  if (*arg[0] == raw_choice.com)
	  {
	    (int)tests[option_id]->enable = !(int)tests[option_id]->enable;
	    /* just toggle it */
	    if (tests[option_id]->dev_enable)
	    {
	      select_test(option_id, tests[option_id]->enable);
	      print_status();
	    }
	    return(TRUE);
	  }
	  else if (*arg[0] == fs_choice.com)
	  {
	    tmp = ((int)tests[option_id]->data & 0x8) >> 3;
	    if (!(int)tests[option_id+1]->enable)  /* trying to enable it */
	      if (tmp)	   /* write/read is selected */
	      {
		tty_message(
    "Can't enable Filetest when \"Write/Read\" Rawtest option is selected!");
		return(TRUE);
	      }

	    (int)tests[option_id+1]->enable = !(int)tests[option_id+1]->enable;
	    /* just toggle it */
	    if (tests[option_id]->dev_enable)
	    {
	      select_test(option_id+1, tests[option_id+1]->enable);
	      print_status();
	    }
	    return(TRUE);
	  }
	  else if (*arg[0] == raw_op_choice1.com)
	  {
	    tmp = ((int)tests[option_id]->data & 0x8) >> 3;
	    if (!(tmp))	/* trying to enable it */
	    {
	      if ((int)tests[option_id+1]->enable)	/* fstest enabled */
	      {
		tty_message(
	"\"Write/Read\" option is not available when Filetest is enabled!");
		return(TRUE);
	      }
	    }

	    (int)tests[option_id]->data ^= 8;	/* toggle the bit #3 */
	    return(TRUE);
	  }
	  else if (*arg[0] == raw_op_choice2.com)
	  {
	    tmp = (int)tests[option_id]->data & 0x7;

	    if (++tmp == raw_op_choice2.num)
	      tmp = 0;

            (int)tests[option_id]->data = 
                        ((int)tests[option_id]->data & ~0x07) | tmp;
	    return(TRUE);
	  }
	  else if (*arg[0] == raw_op_choice3.com)
	  {
	    tmp = ((int)tests[option_id]->data & 0x70) >> 4;

	    if (++tmp == raw_op_choice3.num)
	      tmp = 0;

            (int)tests[option_id]->data = 
                        ((int)tests[option_id]->data & ~0x70) | (tmp << 4);
	    return(TRUE);
	  }
	}
	break;

      case MAGTAPE1:
      case MAGTAPE2:
      case SCSITAPE:
	if (arg_no == 1)
	{
	  if (*arg[0] == format_choice.com)
	  {
	      if (!cart_tape) break;
	      tmp = (int)tests[option_id]->data & 3;
	      if (++tmp == format_choice.num)
	        tmp = 0;
	      (int)tests[option_id]->data =
			((int)tests[option_id]->data & ~3) | tmp;
	      return(TRUE);
	  }
	  else if (*arg[0] == density_choice.com)
	  {
	      if (qic150 || exabyte_8200 || cart_tape) break;
	      if (tests[option_id]->conf->uval.tapeinfo.status == FLT_COMP)
	      {
	        tmp = (int)tests[option_id]->data & 7;
	        if (++tmp == density_choice.num)
	          tmp = 0;
	        (int)tests[option_id]->data =
			((int)tests[option_id]->data & ~7) | tmp;
	      }
	      else
	      {
	        tmp = (int)tests[option_id]->data & 3;
	        if (++tmp == density_choice_nodc.num)
	          tmp = 0;
		if (halfin)
		  if (tmp == density_choice_halfin.num)
		    tmp = 0;
	        (int)tests[option_id]->data =
			((int)tests[option_id]->data & ~3) | tmp;
	      }
	      return(TRUE);
	  }
	  else if (*arg[0] == mode_choice.com)
	  {
	      (int)tests[option_id]->data ^= 0x100;	/* toggle the bit #8 */
	      return(TRUE);
	  }
	  else if (*arg[0] == length_choice.com)
	  {
	      tmp = ((int)tests[option_id]->data & 0x18) >> 3;
	      if (++tmp == length_choice.num)
	        tmp = 0;
	      (int)tests[option_id]->data =
			((int)tests[option_id]->data & ~0x18) | (tmp << 3);
	      return(TRUE);
	  }
	  else if (*arg[0] == file_choice.com)
	  {
	      (int)tests[option_id]->data ^= 0x20;
	      return(TRUE);
	  }
	  else if (*arg[0] == stream_choice.com)
	  {
	      (int)tests[option_id]->data ^= 0x40;
	      return(TRUE);
	  }
	  else if (*arg[0] == recon_choice.com)
	  {
	      (int)tests[option_id]->data ^= 0x80;
	      return(TRUE);
	  }
	  else if (*arg[0] == retension_choice.com)
	  {
	      (int)tests[option_id]->data ^= 0x200;	/* toggle the bit #9 */
	      return(TRUE);
	  }
	  else if (*arg[0] == clean_choice.com)
	  {
	      (int)tests[option_id]->data ^= 0x400;	/* toggle the bit #10 */
	      return(TRUE);
	  }
	}
	else if (arg_no == 2)
	{
	  if (*arg[0] == block_choice.com)
	  {
	      (unsigned)tests[option_id]->special = (unsigned)atoi(arg[1]);
	      return(TRUE);
	  }
	  else if (*arg[0] == pass_choice.com)
	  {
	      (unsigned)tests[option_id]->data=
		(unsigned)tests[option_id]->data&0xffff |
		((unsigned)atoi(arg[1])<<16);
	      return(TRUE);
	  }
	}

	break;

      case TV1:
	if (arg_no == 1)
	{
	  if (*arg[0] == ntsc_choice.com)
	  {
	    (int)tests[option_id]->data ^= 1;	/* toggle the bit #0 */
	    return(TRUE);
	  }
	  else if (*arg[0] == yc_choice.com)
	  {
	    (int)tests[option_id]->data ^= 2;	/* toggle the bit #1 */
	    return(TRUE);
	  }
	  else if (*arg[0] == yuv_choice.com)
	  {
	    (int)tests[option_id]->data ^= 4;	/* toggle the bit #2 */
	    return(TRUE);
	  }
	  else if (*arg[0] == rgb_choice.com)
	  {
	    (int)tests[option_id]->data ^= 8;	/* toggle the bit #3 */
	    return(TRUE);
	  }
	}
	break;

      case IPC:
	if (arg_no == 1)
	{
	  if (*arg[0] == floppy_choice.com)
	  {
	    (int)tests[option_id]->data ^= 1;	/* toggle the bit #0 */
	    return(TRUE);
	  }
	  else if (*arg[0] == printer_choice.com)
	  {
	    (int)tests[option_id]->data ^= 2;	/* toggle the bit #1 */
	    return(TRUE);
	  }
	}
	break;

      case MCP:
	if (arg_no == 1)
	{
	  if (*arg[0] == sp_choice.com)
	  {
	    (int)tests[option_id]->enable = !(int)tests[option_id]->enable;
	    /* just toggle it */
	    if (tests[option_id]->dev_enable)
	    {
	      select_test(option_id, tests[option_id]->enable);
	      print_status();
	    }
	    return(TRUE);
	  }
	  else if (*arg[0] == pp_choice.com)
	  {
	    (int)tests[option_id+1]->enable = !(int)tests[option_id+1]->enable;
	    /* just toggle it */
	    if (tests[option_id]->dev_enable)
	    {
	      select_test(option_id+1, tests[option_id+1]->enable);
	      print_status();
	    }
	    return(TRUE);
	  }
	}
	/* fall through to handle "From:" and "To:" */

      case MTI:
      case SCSISP1:
      case SCSISP2:
      case SCP:
      case SCP2:
	if (arg_no == 2)
	{
	  if (*arg[0] == from_choice.com)
	  {
            (void)strcpy(((struct loopback *)(tests[option_id]->data))->from,
				arg[1]);
	    return(TRUE);
	  }
	  else if (*arg[0] == to_choice.com)
	  {
            (void)strcpy(((struct loopback *)(tests[option_id]->data))->to,
				arg[1]);
	    return(TRUE);
	  }
	}
	break;

      case SBUS_HSI:  

        if (arg_no == 1)
        /* __choice.com = the single letter command for TTY mode(not f,d,h) */
        {
          if (*arg[0] == sbus_clock_choice.com)
          {
            tmp  = ((int)tests[option_id]->data & 1);           /* clock */            tmp1 = ((int)tests[option_id]->data & 2) >> 1 ;     /* intl lpbk */

            if (++tmp == sbus_clock_choice.num)
                tmp = 0;

            if ((tmp==1) && (tmp1==1))          /* force intl lpbk=0 if choosing */
                tmp1 = 0;                       /* external clock source */
            (int)tests[option_id]->data =
                        (((int)tests[option_id]->data & ~1))     | (tmp);

            (int)tests[option_id]->data =
                        (((int)tests[option_id]->data & ~2 ))    | (tmp1 << 1);

            return(TRUE);
          }
          else if (*arg[0] == internal_loopback_choice.com)
          {
            tmp  = ((int)tests[option_id]->data & 2) >> 1 ;     /* intl lpbk */
            tmp1 = ((int)tests[option_id]->data & 1);           /* clock */            tmp2 = ((int)tests[option_id]->data & 0x070) >> 4;  /* loopback */

            if (++tmp == internal_loopback_choice.num)
                tmp = 0;

            if ((tmp) && ((tmp1==1) || (tmp2 >= 5)))  /* can't choose intl
lpbk */
                tmp = 0;                       /* w/ external clock source
or */
                                               /* w/ port to port lpbk config */

            (int)tests[option_id]->data =
                        (((int)tests[option_id]->data & ~2 ))    | (tmp <<
1);

            (int)tests[option_id]->data =
                        (((int)tests[option_id]->data & ~1))     | (tmp1);

            (int)tests[option_id]->data =
                        (((int)tests[option_id]->data & ~0x070)) | (tmp2 << 4);

            return(TRUE);
          }
          else if (*arg[0] == sbus_loopback_choice.com)
          {
            tmp = ((int)tests[option_id]->data  & 0x070) >> 4;  /* loopback */
            tmp1 = ((int)tests[option_id]->data & 2) >> 1;      /* intnl lpbk */

            if (++tmp == sbus_loopback_choice.num)
                tmp = 0;

            if ((tmp >= 5))     /* if port to port, force internal lpbk=0 */
            {
                tmp1 = 0;
            }
            (int)tests[option_id]->data =
                        (((int)tests[option_id]->data & ~0x070))  | (tmp << 4);

            (int)tests[option_id]->data =
                        (((int)tests[option_id]->data & ~2))      | (tmp1 << 1);
            return(TRUE);
          }
        }  
        break;

      case HSI:
	if (arg_no == 1)
	{
	  if (*arg[0] == clock_choice.com)
	  {
	    (int)tests[option_id]->data ^= 16;	/* toggle the bit #4 */
	    return(TRUE);
	  }
	  else if (*arg[0] == type_choice.com)
	  {
	    tmp = ((int)tests[option_id]->data & 12) >> 2;
	    if (++tmp == type_choice.num)
	      tmp = 0;
	    if (tmp == 3) tmp = ((tmp << 2) | 3);	/* force 0-1 loopback */
	    else tmp = ((tmp << 2) | ((int)tests[option_id]->data & 3));
	    (int)tests[option_id]->data =
			((int)tests[option_id]->data & ~15) | tmp;
	    return(TRUE);
	  }
	  else if (*arg[0] == loopback_choice.com)
	  {
	    tmp = (int)tests[option_id]->data & 3;
	    tmp1 = ((int)tests[option_id]->data & 12) >> 2;
	    if (++tmp == loopback_choice.num)
	      tmp = 0;
	    if (tmp == 3 || tmp1 != 3)
	      tmp = (tmp | (tmp1 << 2));	/* force RS449, otherwise */
	    (int)tests[option_id]->data =
			((int)tests[option_id]->data & ~15) | tmp;
	    return(TRUE);
	  }
	}
	break;

      case GP:
	if (arg_no == 1)
	{
	  if (*arg[0] == gb_choice.com)
	  {
	    (int)tests[option_id]->data ^= 1;	/* toggle the bit #0 */
	    return(TRUE);
	  }
	}
	break;

      case PRESTO:
        if (arg_no == 1)
        {
          if (*arg[0] == presto_ratio_choice.com)
          {
            (int)tests[option_id]->data ^= 1;   /* toggle the bit #0 */
            return(TRUE);
          }
        }  
        break;

      case VFC:
	break;

      case CG12:
        if (arg_no == 1)
        {
          if (*arg[0] == egret_sub1_choice.com)
          {
            (int)tests[option_id]->data ^= 0x01;         /* TOGGLE bit 0 */
            return(TRUE);
          }
          else if (*arg[0] == egret_sub2_choice.com)
          {
            (int)tests[option_id]->data ^= 0x02;	 /* TOGGLE bit 1 */ 
            return(TRUE);
          }
          else if (*arg[0] == egret_sub3_choice.com)	 
          {
            (int)tests[option_id]->data ^= 0x04;	 /* TOGGLE bit 2 */
            return(TRUE);
          }
          else if (*arg[0] == egret_sub4_choice.com)	 /* TOGGLE bit 3 */
          {
            (int)tests[option_id]->data ^= 0x08;
            return(TRUE);
          }
          else if (*arg[0] == egret_sub5_choice.com)     /* TOGGLE bit 4 */
          {
            (int)tests[option_id]->data ^= 0x10;
            return(TRUE);
          }
          else if (*arg[0] == egret_sub6_choice.com)	 /* TOGGLE bit 5 */
          {
            (int)tests[option_id]->data ^= 0x20;
            return(TRUE);
          }
          else if (*arg[0] == egret_sub7_choice.com)  	 /* TOGGLE bit 6 */
          {
            (int)tests[option_id]->data ^= 0x40;
            return(TRUE);
          }
          else if (*arg[0] == egret_sub8_choice.com)	 /* TOGGLE bit 7 */
          {
            (int)tests[option_id]->data ^= 0x80;
            return(TRUE);
          }
          else if (*arg[0] == egret_sub9_choice.com)     /* TOGGLE bit 8 */ 
          { 
            (int)tests[option_id]->data ^= 0x100; 
            return(TRUE); 
          } 
          else if (*arg[0] == egret_sub10_choice.com)     /* TOGGLE bit 9 */ 
          { 
            (int)tests[option_id]->data ^= 0x200; 
            return(TRUE); 
          } 
          else if (*arg[0] == egret_sub11_choice.com)     /* TOGGLE bit 10 */ 
          { 
            (int)tests[option_id]->data ^= 0x400; 
            return(TRUE); 
          } 
        }
        if (arg_no == 2 && *arg[0] == function_loop_choice.com)
        {
          tmp = atoi(arg[1]);
          (int)tests[option_id]->data =  
		((int)tests[option_id]->data & ~0xFFFFF000) | (tmp << 12);
          return(TRUE);
        }
        else if (arg_no == 2 && *arg[0] == board_loop_choice.com)
        {
          tmp = atoi(arg[1]);
          (int)tests[option_id]->special = tmp;
          return(TRUE);
        }
        break;

      case GT:
        if (arg_no == 1)
        {
          if (*arg[0] == hawk_sub1_choice.com)
          {
            (int)tests[option_id]->data ^= 0x01;         /* TOGGLE bit 0 */
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub2_choice.com)
          {
            (int)tests[option_id]->data ^= 0x02;         /* TOGGLE bit 1 */
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub3_choice.com)
          {
            (int)tests[option_id]->data ^= 0x04;         /* TOGGLE bit 2 */
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub4_choice.com)      /* TOGGLE bit 3 */
          {
            (int)tests[option_id]->data ^= 0x08;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub5_choice.com)     /* TOGGLE bit 4 */
          {
            (int)tests[option_id]->data ^= 0x10;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub6_choice.com)      /* TOGGLE bit 5 */
          {
            (int)tests[option_id]->data ^= 0x20;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub7_choice.com)      /* TOGGLE bit 6 */
          {
            (int)tests[option_id]->data ^= 0x40;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub8_choice.com)      /* TOGGLE bit 7 */
          {
            (int)tests[option_id]->data ^= 0x80;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub9_choice.com)     /* TOGGLE bit 8 */
          {
            (int)tests[option_id]->data ^= 0x100;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub10_choice.com)     /* TOGGLE bit 9 */
          {
            (int)tests[option_id]->data ^= 0x200;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub11_choice.com)     /* TOGGLE bit 10 */
          {
            (int)tests[option_id]->data ^= 0x400;            
	    return(TRUE);
          }
          else if (*arg[0] == hawk_sub12_choice.com)     /* TOGGLE bit 11 */
          {
            (int)tests[option_id]->data ^= 0x800;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub13_choice.com)     /* TOGGLE bit 12 */
          {
            (int)tests[option_id]->data ^= 0x1000;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub14_choice.com)     /* TOGGLE bit 13 */
          {
            (int)tests[option_id]->data ^= 0x2000;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub15_choice.com)     /* TOGGLE bit 14 */
          {
            (int)tests[option_id]->data ^= 0x4000;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub16_choice.com)     /* TOGGLE bit 15 */
          {
            (int)tests[option_id]->data ^= 0x8000;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub17_choice.com)     /* TOGGLE bit 16 */
          {
            (int)tests[option_id]->data ^= 0x10000;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub18_choice.com)     /* TOGGLE bit 17 */
          {
            (int)tests[option_id]->data ^= 0x20000;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub19_choice.com)     /* TOGGLE bit 18 */
          {
            (int)tests[option_id]->data ^= 0x40000;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub20_choice.com)     /* TOGGLE bit 19 */
          {
            (int)tests[option_id]->data ^= 0x80000;
            return(TRUE);
          }
          else if (*arg[0] == hawk_sub21_choice.com)     /* TOGGLE bit 20 */
          {
            (int)tests[option_id]->data ^= 0x100000;
            return(TRUE);
          }
        }  
        if (arg_no == 2 && *arg[0] == subtest_loop_choice.com)
        {
          tmp = atoi(arg[1]);
          if (tmp > 1023)
          {
                tmp = 1023;             /* this is the max value allowed */
          }                             /* w/ the bits provided by data field */
          (int)tests[option_id]->data =
                ((int)tests[option_id]->data & ~0xFFE00000) | (tmp << 21);
          return(TRUE);
        }
        else if (arg_no == 2 && *arg[0] == test_sequence_loop_choice.com)
        {
          tmp = atoi(arg[1]);
          (int)tests[option_id]->special = tmp;
          return(TRUE);
        }
        break;

      case MP4:
        if (arg_no == 1)
        {
          if (*arg[0] == mp4_shmem_choice.com)
          {
            (int)tests[option_id]->data ^= 0x01;         /* TOGGLE bit 0 */
            return(TRUE);
          }
          else if (*arg[0] == mp4_dataio_choice.com)
          {
            (int)tests[option_id]->data ^= 0x02;         /* TOGGLE bit 1 */
            return(TRUE);
          }
          else if (*arg[0] == mp4_FPU_choice.com)
          {
            (int)tests[option_id]->data ^= 0x04;         /* TOGGLE bit 2 */
            return(TRUE);
          }
          else if (*arg[0] == mp4_cache_choice.com)
          {
            (int)tests[option_id]->data ^= 0x08;         /* TOGGLE bit 3 */
            return(TRUE);
          }
          else if (*arg[0] == mp4_processors_choice[0].com)
          {
            (int)tests[option_id]->data ^= 0x10;         /* TOGGLE bit 4 */
            return(TRUE);
          }
          else if (*arg[0] == mp4_processors_choice[1].com)
          {
            (int)tests[option_id]->data ^= 0x20;         /* TOGGLE bit 5 */
            return(TRUE);
          }
          else if (*arg[0] == mp4_processors_choice[2].com)
          {
            (int)tests[option_id]->data ^= 0x40;         /* TOGGLE bit 6 */
            return(TRUE);
          }
          else if (*arg[0] == mp4_processors_choice[3].com)
          {
            (int)tests[option_id]->data ^= 0x80;         /* TOGGLE bit 7 */
            return(TRUE);
          }
        }
        break;

      case ZEBRA1:
        if (arg_no == 1)
        {
          if (*arg[0] == bpp_access_choice.com)
          {
              tty_message(
                   "Only \"Writeonly\" option is supported at this time.");
/*** Not need to toggle bit #0 at this time. ***
              (int)tests[option_id]->data ^= 1;   
*****/
              return(TRUE);
          }
          else if (*arg[0] == bpp_mode_choice.com)
          {
              tmp = ((int)tests[option_id]->data & 0xC) >> 2;
              if (++tmp == bpp_mode_choice.num)
                tmp = 0;
              (int)tests[option_id]->data =
                        ((int)tests[option_id]->data & ~0xC) | (tmp << 2);
              return(TRUE);
          }
        }
        break;

      case ZEBRA2:
        if (arg_no == 1)
        { 
          if (*arg[0] == lpvi_access_choice.com)
	  {
              tty_message( 
                   "Only \"Writeonly\" option is supported at this time."); 

/*** No need to toggle bit #0 at this time. ***
              (int)tests[option_id]->data ^= 1;   
*****/    
              return(TRUE);
          }
          else if (*arg[0] == lpvi_mode_choice.com) 
          {
              tmp = ((int)tests[option_id]->data & 0xC) >> 2; 
              if (++tmp == lpvi_mode_choice.num) 
                tmp = 0;    
              (int)tests[option_id]->data =
                        ((int)tests[option_id]->data & ~0xC) | (tmp << 2); 
              return(TRUE); 
          }
          else if (*arg[0] == lpvi_image_choice.com)
          {
              tmp = ((int)tests[option_id]->data & 0x30) >> 4;
              if (++tmp == lpvi_image_choice.num)
                tmp = 0;    
	      else if (tmp == 2) 
		tty_message(
		     "Please use \"u_image\" as UserDefined filename.");

              (int)tests[option_id]->data =
                        ((int)tests[option_id]->data & ~0x30) | (tmp << 4);
              return(TRUE); 
          }
          else if (*arg[0] == lpvi_resolution_choice.com)
          {
              tmp = ((int)tests[option_id]->data & 0xC0) >> 6;
              if (++tmp == lpvi_resolution_choice.num)
                tmp = 0;    
              (int)tests[option_id]->data =
                        ((int)tests[option_id]->data & ~0xC0) | (tmp << 6);
              return(TRUE); 
          }
        } 
        break;

      case SPIF:
	if (arg_no == 1)
	{
	  if (*arg[0] == internal_choice.com)
	  {
	    tmp = ((int)tests[option_id]->data & 0x1);
	    if (++tmp == internal_choice.num)
	      tmp = 0;
	    tests[option_id]->data = (caddr_t)((int)tests[option_id]->data ^ 0x1);
	    return(TRUE);
	  }
	  else if (*arg[0] == parallel_print_choice.com)
	  {
	    tmp = ((int)tests[option_id]->data & 0x4) >> 2; /* SP-96 bit */
	    if (tmp) /* SP-96 enabled */
	    {
	      if (!(int)tests[option_id+1]->enable)      /* print disable */
	      {
		tty_message(
        	"Can't enable Parallel Print test option when SP-96 is selected!");
		return(TRUE);
	      }
	    }
	
	    tmp = ((int)tests[option_id]->data & 0x2) >> 1; 
	    if (++tmp == parallel_print_choice.num)
	      tmp = 0;
	    tests[option_id]->data = (caddr_t)(
		((int)tests[option_id]->data & ~0x2) | (tmp << 1) );
	    return(TRUE);
	  }
	  else if (*arg[0] == sp96_choice.com)
	  {
	    tmp  = ((int)tests[option_id]->data & 0x2) >> 1; /* Print bit */
	    tmp1 = ((int)tests[option_id]->data & 0x8) >> 3; /* DB-25 bit */
	    tmp2 = ((int)tests[option_id]->data & 0x16) >> 4; /* echo-tty bit */
	    if ( tmp || tmp1 || tmp2 ) /* Print, DB-25 or echo-tty enabled */
	    {
	      if (!(int)tests[option_id+1]->enable)      /* SP-96 disable */
	      {
		if(tmp)
		{
		  tty_message(
        	  "Can't enable SP-96 test option when Print is selected!");
		  return(TRUE);
		}
		else if(tmp1)
		{
		  tty_message(
        	  "Can't enable SP-96 test option when DB-25 is selected!");
		  return(TRUE);
		}
		else 
		{
		  tty_message(
        	  "Can't enable SP-96 test option when echo-tty is selected!");
		  return(TRUE);
		}
	      }
	    }
	
	    tmp = ((int)tests[option_id]->data & 0x4) >> 2; 
	    if (++tmp == sp96_choice.num)
	      tmp = 0;
	    tests[option_id]->data = (caddr_t)(
		((int)tests[option_id]->data & ~0x4) | (tmp << 2) );
	    return(TRUE);
	  }
	  else if (*arg[0] == db25_choice.com)
	  {
	    tmp = ((int)tests[option_id]->data & 0x4) >> 2; /* SP-96 bit */
	    if (tmp) /* SP-96 enabled */
	    {
	      if (!(int)tests[option_id+1]->enable)      /* DB-25 disable */
	      {
		tty_message(
        	"Can't enable DB-25 test option when SP-96 is selected!");
		return(TRUE);
	      }
	    }
	
	    tmp = ((int)tests[option_id]->data & 16) >> 4; /* echo-tty bit */
	    if (tmp) /* echo-tty enabled */
	    {
	      if (!(int)tests[option_id+1]->enable)      /* DB-25 disable */
	      {
		tty_message(
        	"Can't enable DB-25 test option when echo-tty is selected!");
		return(TRUE);
	      }
	    }
	
	    tmp = ((int)tests[option_id]->data & 0x8) >> 3; 
	    if (++tmp == db25_choice.num)
	      tmp = 0;
	    tests[option_id]->data = (caddr_t)(
		((int)tests[option_id]->data & ~0x8) | (tmp << 3) );
	    return(TRUE);
	  }
	  else if (*arg[0] == echo_tty_choice.com)
	  {
	    tmp = ((int)tests[option_id]->data & 0x4) >> 2; /* SP-96 bit */
	    if (tmp) /* SP-96 enabled */
	    {
	      if (!(int)tests[option_id+1]->enable)      /* echo-tty disable */
	      {
		tty_message(
        	"Can't enable echo-tty test option when SP-96 is selected!");
		return(TRUE);
	      }
	    }
	
	    tmp = ((int)tests[option_id]->data & 0x8) >> 3; /* DB-25 bit */
	    if (tmp) /* DB-25 enabled */
	    {
	      if (!(int)tests[option_id+1]->enable)      /* echo-tty disable */
	      {
		tty_message(
        	"Can't enable echo-tty test option when DB-25 is selected!");
		return(TRUE);
	      }
	    }
	
	    tmp = ((int)tests[option_id]->data & 0x10) >> 4; 
	    if (++tmp == echo_tty_choice.num)
	      tmp = 0;
	    tests[option_id]->data = (caddr_t)(
		((int)tests[option_id]->data & ~0x10) | (tmp << 4) );
	    return(TRUE);
	  }
	  else if (*arg[0] == stop_bit_choice.com)
	  {
	    tmp = ((int)tests[option_id]->data & 32) >> 5; 
	    if (++tmp == stop_bit_choice.num)
	      tmp = 0;
	    tests[option_id]->data = (caddr_t)(
		((int)tests[option_id]->data & ~32) | (tmp << 5) );
	    return(TRUE);
	  }
	  else if (*arg[0] == baud_rate_choice.com)
	  {
	    tmp = ((int)tests[option_id]->data & 0xf00) >> 8;
	    if (++tmp == baud_rate_choice.num)
	      tmp = 0;
	    tests[option_id]->data = (caddr_t)(
			((int)tests[option_id]->data & ~0xf00) | (tmp << 8));
	    return(TRUE);
	  }
	  else if (*arg[0] == char_size_choice.com)
	  {
	    tmp = ((int)tests[option_id]->data & 0xf000) >> 12;
	    if (++tmp == char_size_choice.num)
	      tmp = 0;
	    tests[option_id]->data = (caddr_t)(
			((int)tests[option_id]->data & ~0xf000) | (tmp << 12));
	    return(TRUE);
	  }
	  else if (*arg[0] == parity_choice.com)
	  {
	    tmp = ((int)tests[option_id]->data & 0xf0000) >> 16;
	    if (++tmp == parity_choice.num)
	      tmp = 0;
	    tests[option_id]->data = (caddr_t)(
			((int)tests[option_id]->data & ~0xf0000) | (tmp << 16));
	    return(TRUE);
	  }
	  else if (*arg[0] == flowctl_choice.com)
	  {
	    tmp = ((int)tests[option_id]->data & 0xf00000) >> 20;
	    if (++tmp == flowctl_choice.num)
	      tmp = 0;
	    tests[option_id]->data = (caddr_t)(
			((int)tests[option_id]->data & ~0xf00000) | (tmp << 20));
	    return(TRUE);
	  }
	  else if (*arg[0] == serial_port_choice.com)
	  {
	    tmp = ((int)tests[option_id]->data & 0xf000000) >> 24;
	    if (++tmp == serial_port_choice.num)
	      tmp = 0;
	    tests[option_id]->data = (caddr_t)(
			((int)tests[option_id]->data & ~0xf000000) | (tmp << 24));
	    return(TRUE);
	  }
	  else if (*arg[0] == data_type_choice.com)
	  {
	    tmp = ((int)tests[option_id]->data & 0xf0000000) >> 28;
	    if (++tmp == data_type_choice.num)
	      tmp = 0;
	    tests[option_id]->data = (caddr_t)(
			((int)tests[option_id]->data & ~0xf0000000) | (tmp << 28));
	    return(TRUE);
	  }
	}
	break;
    } /* end of switch */

    tty_message(com_err);
    return(FALSE);
  }

  tty_message(format_err);
  return(FALSE);
}

