
/*      @(#)tapetest.h 1.1 92/07/30 SMI          */
 
/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc. 
 */

/* 
 * error numbers, Sundiag gets 1 and 2
 */
#define BAD_DEVICE_NAME		3
#define PROBE_DEV_ERROR		4
#define MTIOCGET_ERROR		5
#define MTIOCTOP_ERROR		6
#define OPEN_ERROR              81
#define CLOSE_ERROR             82
#define SEEK_ERROR              83
#define WRITE_ERROR             84
#define READ_ERROR              85
#define COMPARE_ERROR           86

/*
 * other tapetest defines
 */
#define ST                      "st"
#define MT			"mt"
#define XT			"xt"
#define EOT			"eot"
#define NBLOCKS         	126
#define BLOCKSIZE       	512
#define ERROR_THRESHOLD 	2
#define FILE_1			1
#define FILE_2			2
#define FILE_3			3
#define FILE_4			4

#define NBLOCKS                 126
#define BLOCKSIZE               512
#define ENOENT_ERROR		2

/*
 * Recon defines
 */
#define TRY_AGAIN		127	/* Re-fork the process */
#define MAX_TIME		20	/* Child cannot wait longer than this */
#define MAX_RETRIES		5	/* Max. times to retry recon test */

/*  
 * error codes for recon test.
 */
#define	OPENTAPE_ERROR		6
#define	OPENDISK_ERROR		7
#define	RETEN_ERROR		8
#define	READDISK_ERROR		9
#define	SUPER_ERROR		10
#define BADSIG_ERROR		11
#define	FAIL_ERROR		12
#define ENVIRON_ERROR		13
#define TOOLATE_ERROR  		32

enum mt_com_type {
    fsf = 0,
    nbsf = 1,
    rewind = 2,
    tape_status = 3,
    retension = 4,
    erase = 5
};
typedef enum mt_com_type Mt_com_type;

/*
 * tapetest enums/typedefs
 */
enum tape_type {
    ar = 0,
    st = 1,
    mt = 2,
    xt = 3
};
typedef enum tape_type Tape_type;

/*
 * tapetest structs
 */
struct commands {
        char *c_name;
        int c_code;
        int c_ronly;
        int c_usecnt;
};
