/* @(#)vfctest_defs.h 1.1 92/07/30 SMI */

#define VFC_SIZE		0x8000
#define FIELD_SIZE      ( 360 * 625 )
#define GARBAGE_PIXELS	248
#define WRITE			1
#define READ			0
#define FRAM_MASK		0xfef00000

/* Error codes */
#define DEV_NOT_ACCESSIBLE    3
#define DEV_OPEN_ERR          4
#define DEV_MMAP_ERR          5
#define VFC_FATAL			  9
#define TOO_MANY_ERRS        10

