/* @(#)pstest.h 1.1 92/07/30 Copyright Sun Micro */

/* error code define */

#define	TESTEND_ERR		1
#define SUBTEST_ERR		2
#define IOCTL_ERR 		3
#define STATE_ERR 		4
#define BATTERY_ERR		5
#define PERFORMANCE_ERR		6
#define MMAP_ERR		7
#define MUNMAP_ERR		8
#define COMPARE_ERR		9
#define FLOCK_ERR		10
#define LSEEK_ERR		11
#define OPEN_ERR		12
#define WRITE_ERR		13
#define READ_ERR		14


/*
 * Configuration options:
 */
#define MEM_TEST_CYCLES 2       /* Number of times to run memory tests. */
#define IO_TEST_CYCLES 1        /* Number of times to run io tests. */
#define STARTLOC  16            /* first 4 long of memory reserved for
                                   battery status */
#define PASS_COUNT 1		/* try subtest once only */
#define RETRY_COUNT 1		/* retry one more time where error */

#define REPS 256	/* repeat 256 times of SZ bytes to exceed the
			   prestoserve cache (1M) to force flushing. */
#define SZ 8192         /* Buffer size - don't change.  Same as FS block size.*/
