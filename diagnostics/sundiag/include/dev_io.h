
/*      @(#)dev_io.h 1.1 92/07/30 SMI          */
 
/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc. 
 */
#define OPEN_ERROR              81
#define CLOSE_ERROR             82
#define SEEK_ERROR              83
#define WRITE_ERROR             84
#define READ_ERROR              85
#define COMPARE_ERROR           86
#define NBLOCKS                 126
#define BLOCKSIZE               512
#define ENOENT_ERROR		2
#define SECS_1                  1
#define SECS_2                  2
#define PASS_SLEEP_MOD          5

enum mt_com_type {
    fsf = 0,
    bsf = 1,
    rewind = 2,
    tape_status = 3,
    retension = 4,
    erase = 5
};
typedef enum mt_com_type Mt_com_type;

enum error_type {
    no_error = 0,
    eof_error = 1,
    eom_error = 2
};
typedef enum error_type Error_type;

enum test_mode {
    writing = 0,
    reading = 1,
    read_compare = 2
};
typedef enum test_mode Test_mode;

int write_file(), read_file(); 
int open_dev(), close_dev();
