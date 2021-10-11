/*      @(#)probe.h 1.1 92/07/30 SMI          */
 
/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc. 
 */

#include <fcntl.h>

#ifdef SVR4
#include <unistd.h>
#include <dirent.h>
#include <sys/devinfo.h>
#endif SVR4

/* device name abbreviations, as per Unix config file and otherwise */
#define AR      "ar"
#define BWONE   "bwone"
#define BWTWO   "bwtwo"
#define CGONE	"cgone"
#define CGTWO   "cgtwo"
#define CGTHREE "cgthree"
#define CGFOUR  "cgfour"
#define CGFIVE	"cgfive"
#define CGSIX	"cgsix"
#define CGEIGHT "cgeight"
#define CGNINE	"cgnine"
#define CGTWELVE "cgtwelve"
#define GT	"gt"
#define	TVONE	"tvone"
#define DCP     "dcp"
#define DES     "des"
#define FDDI    "fddi"
#define TRNET	"tr"
#define OMNI_NET      "ne"
#define FPA     "fpa"
#define FPUU    "fpu"
#define GPONE   "gpone"
#define HSS   	"hs"
#define HSI   	"HSI"
#define KMEM    "kmem"
#define IE      "ie"
#define IP      "ip"
#define ID      "id"
#define LE      "le"
#define MC68882 "68882"
#define MC68881 "68881"
#define MCP     "mcp"
#define ME      "me"
#define MEM     "mem"
#define MT      "mt"
#define MTI     "mti"
#define IPC     "pc"
#define	FD	"fd"
#define FDTWO   "fdtwo"
#define	SF	"sf"
#define SD      "sd"
#define SPIF    "spif"
#define ST      "st"
#define	CDROM	"sr"
#define TAAC    "taac"
#define XD      "xd"
#define XT      "xt"
#define XY      "xy"
#define ZS      "zs"
#define	PP	"pp"
#define PR  	"pr"
#define AUDIO   "audio"
#define ZEBRA1  "bpp"
#define ZEBRA2  "lpvi"
#define MP4	"mp"
#define VFC	"vfc"
#define DBRI	"DBRI"

/* For sun3 architectures */
#ifndef	CPU_SUN3X_470
#define	CPU_SUN3X_470	0x41
#endif

#ifndef	CPU_SUN3X_80
#define	CPU_SUN3X_80	0x42
#endif

#ifndef	CPU_SUN3_160
#define	CPU_SUN3_160	0x11
#endif

#ifndef	CPU_SUN3_50
#define	CPU_SUN3_50	0x12
#endif

#ifndef	CPU_SUN3_260
#define	CPU_SUN3_260	0x13
#endif

#ifndef	CPU_SUN3_110
#define	CPU_SUN3_110	0x14
#endif

#ifndef	CPU_SUN3_60
#define	CPU_SUN3_60	0x17
#endif

#ifndef	CPU_SUN3_E
#define	CPU_SUN3_E	0x18
#endif

/* For sun4 architectures */
#ifndef	CPU_SUN4_260	
#define	CPU_SUN4_260	0x21
#endif

#ifndef	CPU_SUN4_110	
#define	CPU_SUN4_110	0x22
#endif

#ifndef	CPU_SUN4_330
#define	CPU_SUN4_330	0x23
#endif

#ifndef	CPU_SUN4_460
#define	CPU_SUN4_460	0x24
#endif

#ifndef	CPU_SUN4_890
#define	CPU_SUN4_890	0x26
#endif

#ifndef	CPU_SUN4C_60
#define	CPU_SUN4C_60	0x51
#endif

#ifndef	CPU_SUN4C_40
#define	CPU_SUN4C_40	0x52
#endif

#ifndef	CPU_SUN4C_65
#define	CPU_SUN4C_65	0x53
#endif

#ifndef	CPU_SUN4C_20
#define	CPU_SUN4C_20	0x54
#endif

#ifndef	CPU_SUN4C_75
#define	CPU_SUN4C_75	0x55
#endif

#ifndef	CPU_SUN4C_30
#define	CPU_SUN4C_30	0x56
#endif

#ifndef CPU_SUN4C_25
#define CPU_SUN4C_25    CPU_SUN4C_30
#endif

#ifndef	CPU_SUN4C_50
#define	CPU_SUN4C_50	0x57
#endif

#ifndef	SUN4C_ARCH
#define	SUN4C_ARCH	0x50
#endif

#ifndef GALAXY
#define GALAXY          0x71
#endif

#ifndef C2
#define C2              0x72
#endif

#ifdef SVR4
#define CPUNAME 0
#define NAMELEN 80
struct mb_devinfo
{
        struct dev_info info;
        char name[NAMELEN];
};
#else SVR4
#ifdef	_sun_openprom_h
#define	NAMELEN	80
struct mb_devinfo
{
	struct dev_info info;
	char name[NAMELEN];
};
#endif	_sun_openprom_h
#endif SVR4

enum dev_type {

    character=0,
    block=1,
};
typedef enum dev_type Dev_type;

enum special_dev {
    no=0,
    scsi=1,
    mcp=2,
    scp=3
};
typedef enum special_dev Special_dev;

extern int    debug;
extern struct mb_device *mbdinit;

extern	struct	mb_devinfo mbdinfo[];
extern	int	numdev;
extern	int	cpu_type;
extern  char    buff[];
extern  char    tmp_devdir[];
extern  char    *tmpname;

extern	char *get_hostname(), *do_probe(), *errmsg();
extern	int init_clnt(), send_log_msg(), send_probe_msg(), end_clnt();
extern	int check_bwone_dev(), check_bwtwo_dev();
extern	int check_cgtwo_dev(), check_cgfour_dev(), check_cgfive_dev();
extern	int check_cgone_dev(), check_taac_dev(), check_printer_device();
extern	int check_cgthree_dev(), check_cgeight_dev(), check_hss_dev();
extern  int check_cgtwelve_dev(), check_presto_dev();
extern	int check_des_dev(), check_fpa_dev(), check_dev();
extern	int check_pc_dev(), check_net();
extern	int check_mcp_dev(), check_mti_dev(), check_dcp_dev(), check_sp_dev();
extern	int check_disk_dev();
extern	int check_ar_dev(), check_mt_dev(), check_st_dev();
