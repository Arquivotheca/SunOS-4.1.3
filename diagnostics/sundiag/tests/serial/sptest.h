/*
 * @(#)sptest.h - Rev 1.1 - 7/30/92
 *
 * sptest.h:  header file for sptest.c.
 *
 */

/*
 * error code for this test.
 */
#define TESTED_OK               0             /* tested OK! */
#define NO_SD_PORTS_SELECTED    4
#define DEV_NOT_OPEN            5
#define WRITE_FAILED            6
#define READ_FAILED             7
#define NOT_READY               8
#define DATA_ERROR              9
#define BUF_SIZE                1024           /* buffer size rvd name */
#define NUM_RETRIES             100000         /* number of retry for data */
