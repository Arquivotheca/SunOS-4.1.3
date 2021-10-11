/*
 * @(#)fstest.h - Rev 1.1 - 7/30/92
 *
 * fstest.h:  header file for fstest.c.
 *
 */

#define FILE1_OPEN_ERROR        3
#define FILE2_OPEN_ERROR        4
#define FILE1_WRITE_ERROR       5
#define FILE2_WRITE_ERROR       6
#define FILE1_CLOSE_ERROR       7
#define FILE2_CLOSE_ERROR       8
#define FILE1_REOPEN_ERROR      9
#define FILE2_REOPEN_ERROR      10
#define FILE1_READ_ERROR        11
#define FILE2_READ_ERROR        12
#define FILE1_BAD               13
#define FILE1_CMP_ERROR         14
#define FILE2_CMP_ERROR         15
#define FILE1_RECLOSE_ERROR     16
#define FILE2_RECLOSE_ERROR     17
#define FILE1_UNLINK_ERROR      18
#define FILE2_UNLINK_ERROR      19
#define NO_SD_DRIVES            20
#define FS_PROBE_ERROR          22
#define NOT_MOUNTED_ERROR       23
#define NOT_WRITABLE_ERROR      24
#define NOT_ENOUGH_FREE_ERROR   25
#define LEAVE_FILES             30
#define AUTO_MNT_ERROR          31
#define AUTO_UNMNT_ERROR        32
#define OPENDEV_ERROR        	33
#define NOFS_ERROR        	34

#define OMODE1          (O_WRONLY | O_CREAT | O_TRUNC | O_SYNC)
#define OMODE2          (O_RDONLY | O_SYNC)
#define NAME1           "tmpdisk1.XXXXXX"
#define NAME2           "tmpdisk2.XXXXXX"

#define BSIZE           512
#define MAXBLOCK        1024
#define MAX_DRIVES      20
#define DRIVE_TYPES 	25

#define DATA_SEQUENTAL  0
#define DATA_ZERO       1
#define DATA_ONES       2
#define DATA_A          3
#define DATA_5          4
#define DATA_RANDOM     5

struct dev {
    char            device[8];
    char            directory[75];
    int             space;
    int             blocks;
    int             fd1;
    char            name1[100];
    int             file1_created;
    int             fd2;
    char            name2[100];
    int             file2_created;
    int             blocks_written;
    int             blocks_read;
}               testing[MAX_DRIVES] = {
{
"sd0",
"/usr/tmp",
700,
512,
0,  
"/usr/tmp/tmpdisk1.XXXXXX",
0,
0,
"/usr/tmp/tmpdisk2.XXXXXX",
0,
0,
0
},
{
"sd1",
"/usr2",
700,
100,
0,
"/usr2/tmpdisk1.XXXXXX",
0,
0,
"/usr2/tmpdisk2.XXXXXX",
0,
0,
0
},
{
"sd2",
"/tmp",
700,
1,
0,
"/tmp/tmpdisk1.XXXXXX",
0,
0,
"/tmp/tmpdisk2.XXXXXX",
0,
0,
0
},
{
""
}
};
