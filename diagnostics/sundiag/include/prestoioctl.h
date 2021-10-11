/*      @(#)prestoioctl.h	1.1  7/30/92    */
/*
 * Copyright (c) 1989, Legato Systems Incorporated.
 *
 * All rights reserved.
 */

/*
 * Definitions for the ``presto'' device driver.
 */

/*
 * Presto is initially down until an PRSETSTATE ioctl cmd
 * with arg = PRUP is done.  When presto is down, nothing is
 * being cached and everything is sync'd back to the real disk.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/ioccom.h>

/*
 *  The following declariations were taken from prestoctl.h
 */
#ifndef MAX_BATTERIES	/* a "guard" against duplicates from prestoctl.h */
enum battery {
	BAT_GOOD = 0,
	BAT_LOW = 1,
	BAT_DISABLED = 2,
	BAT_IGNORE = 3,
};
typedef enum battery battery;

#define MAX_BATTERIES 8

enum prstates {
	PRDOWN = 0,
	PRUP = 1,
	PRERROR = 2,
};
typedef enum prstates prstates;

struct io {
	u_int total;
	u_int hitclean;
	u_int hitdirty;
	u_int pass;
	u_int alloc;
};
typedef struct io io;

struct presto_status {
	prstates pr_state;	/* if not PRUP, pass all rw commands thru */
	u_int	pr_battcnt;
	battery	pr_batt[MAX_BATTERIES];	/* array of battery status flags */
	u_int	pr_maxsize;	/* total memory size in bytes available */
	u_int	pr_cursize;	/* current presto memory size */
	u_int	pr_ndirty;	/* current number of dirty presto buffers */
	u_int	pr_nclean;	/* current number of clean presto buffers */
	u_int	pr_ninval;	/* current number of invalid presto buffers */
	u_int	pr_nactive;	/* current number of active presto buffers */
	/* the io stats are zeroed each time presto is reenabled */
	io 	pr_rdstats;	/* presto read statistics */
	io 	pr_wrstats;	/* presto write statistics */
	u_int	pr_seconds;	/* seconds of stats taken */
};
typedef struct presto_status presto_status;

struct presto_modstat {
	int ps_status;
	union {
		char *ps_errmsg;
		struct presto_status ps_new;
	} presto_modstat_u;
};
typedef struct presto_modstat presto_modstat;
#endif /* MAX_BATTERIES */

/*
 * Get the current presto status information.
 */
#define PRGETSTATUS	_IOR(p, 1, struct presto_status)

/*
 * Set the presto state.  Legal values are PRDOWN and PRUP.
 * When presto is enabled, all the io stats are zeroed.  If
 * presto is in the PRERROR state, it cannot be changed.
 */
#define PRSETSTATE	_IOW(p, 2, int)

/*
 * Set the current presto memory size in bytes.
 */
#define PRSETMEMSZ	_IOW(p, 3, int)

/*
 * Reset the entire presto state.  If there are any pending writes
 * back to the real disk which cannot be completed due to IO errors,
 * these writes will be lost forever.  Using this ioctl is the only
 * way presto will evert lose any dirty data if disk errors develops
 * behind presto.  Presto will be left in the PRDOWN state unless
 * all batteries are currently low.
 */
#define PRRESET		_IO(p, 4)

/*
 * Enable presto on a particular presto'ized filesystem.
 * This ioctl is performed on the presto control device, passing in
 * the *block* device number of the presto'ized filesystem to enable.
 */
#define PRENABLE	_IOW(p, 5, dev_t)

/*
 * Disable presto on a particular presto'ized filesystem.
 * This ioctl is performed on the presto control device, passing in
 * the *block* device number of the presto'ized filesystem to disable.
 */
#define PRDISABLE	_IOW(p, 6, dev_t)

struct prbits {
	unsigned char	bits[256/NBBY];	/* bit for each minor bdevno */
};

#define PR_BOUNCEIO	1

struct prtab {
	int (*strategy)();	/* per major device */
	dev_t bmajordev;	/* block major device numer */
	int flags;		/* per device set of flag bits */
	struct prbits enabled; 	/* per minor device boolean bit vector */
	struct prbits flushing; /* per minor device boolean bit vector */
	struct prbits error; 	/* per minor device boolean bit vector */
};

#define PRNEXTPRTAB		_IOWR(p, 7, struct prtab)
#define PRGETPRTAB		_IOWR(p, 8, struct prtab)

#define IOCTL_NUM(x)	((x) & 0xff)

#define PRDEV	"/dev/pr0"
