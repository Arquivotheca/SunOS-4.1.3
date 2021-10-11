/*      @(#)probe_sundiag.h 1.1 92/07/30 SMI          */
 
/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc. 
 */

/* maximum number of found_dev structs that xdr_found_dev will handle */
#define MAXDEVS	100

/* status for disk */
#define NOLABEL -1
#define DISKPROB 0
#define DISKOK   1
/* status for net */
#define NETDOWN -1
#define NETPROB	 0
#define NETUP	 1
/* status for tape */
#define OFFLINE -1
#define TAPEPROB 0
#define ONLINE   1
#define	FLT_COMP 2		/* supports compression mode */
/* status for mcp/alm2 */
#define IFDDEVONLY -1
#define NOMCPTTYDEV 0
#define MCPOK	    1
/* status for fpa3x */
#define FPA3X_EXIST	 2
/* status for fpu2 */
#define FPU2_EXIST	 2

/* enumerated types for discriminated union, as per XDR */
enum utype { 	GENERAL_DEV=1,
		MEM_DEV=2,
		TAPE_DEV=3,
		DISK_DEV=4	};

/* generalized device info */
struct gendev_info {
	int status;
};

/* memory device info */
struct mem_info {
	int amt;
};

/* tape device info */
struct tape_info {
	int status;
	char *ctlr;
	int ctlr_num;
	short t_type;
};

/* disk device info */
struct disk_info {
	int amt;
	int status;
	char *ctlr;
	int ctlr_num;
	int ctlr_type;		/* index to the controller name array */
};

/* discriminated union, as per XDR */
struct u_tag {
	enum utype utype;
	union {
	    struct gendev_info devinfo;
	    struct disk_info diskinfo;
	    struct mem_info meminfo;
	    struct tape_info tapeinfo;
	} uval;
};

/*
 * found_dev struct contains device_name, as per the above abbreviations, unit
 * number (0 if not strictly applicable), and u_tag of device-specific info
 */
struct found_dev {
        char *device_name;
        int unit;
	struct u_tag u_tag;
};

/*
 * f_devs struct contains the cpuname, number (num) of found_dev structs 
 * (indicating the number of devices found), and then the found_dev struct(s)
 */
struct f_devs { 
	int cputype;
        char *cpuname; 
        u_int num;   
        struct found_dev *found_dev;
};

/* XDR function to encode/decode the f_devs struct */
int 	xdr_f_devs();
