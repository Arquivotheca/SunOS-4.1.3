 
/*
 * @(#)rawtest.h - Rev 1.1 - 7/30/92
 *
 * rawtest.h:  header file for rawtest.c.
 *
 */
/* 
 * Copyright (c) 1989 by Sun Microsystems, Inc. 
 */

/* 
 * error numbers, Sundiag gets 1 and 2
 */
#define SET			0
#define GET			1
#define RESET			2
#define BAD_DEVICE_NAME		3
#define OPEN_ERROR              81
#define CLOSE_ERROR             82
#define WRITE_ERROR             84
#define READ_ERROR              85
#define CHECK_ERROR             86
#define MOUNTED_ERROR           87
#define IOCTL_ERROR             88

/*
 * other rawtest defines
 */
#define BLOCKSIZE       	512
#define NBLOCKS   		126

extern int      	process_rawdisk_args();
extern int		routine_usage();


static char		*which_io;
static char		device_diskname[32]="";
static char		device_file[20]="";
static char 		*device;
static char 		exit_message[200];
static char		ok_to_write = 0;
static char		need_to_restore = 0;

static int 		child_pid[25];
static char 		parent=FALSE;
static int 		children;
static int 		stopped_child_pid;
static int 		blkno;
static int		spec_size = -1;	/* set by user "S=< > MB" */

static u_long		lcheck();
static u_long 		pattern;

typedef struct _dsk_info_ {
	int diskfd;
	int startblk;
} dsk_info; 

u_char 			*backup_buf;
u_char 			*dev_buf;
dsk_info 		*dskp;

static char get_part(), partition = 'c';
